/*
 * Copyright (c) 2017 Sound <sound@sagaforce.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "Snapshot.h"
#include <math.h>
#include <ctype.h>
#include <stdint.h>
#include <memory>
#include <mutex>
#include <boost/filesystem.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include "Exception.h"
#include "DataStore.h"
#include "TempFileStream.h"
#include "BufferedInputStream.h"
#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"
#include "FileStream.h"
#include "FileInfo.h"
#include "Repository.h"
#include "LZMAUtils.h"
#include "EncryptedOutputStream.h"
#include "RollingHash.h"

namespace Nebula
{
	Snapshot::Snapshot()
	{
	}
	
	Snapshot::~Snapshot()
	{
	}

	void Snapshot::addFileEntry(const char *path,
					  const char *user,
					  const char *group,
					  FileType type,
					  uint16_t mode,
					  CompressionType compression,
					  uint64_t size,
					  time_t mtime,
					  uint8_t blockSizeLog,
					  const uint8_t *md5,
					  int numBlocks,
					  const BlockHash *blockHashes)
	{
		using namespace boost;

		std::lock_guard<std::recursive_mutex> lock(mMutex);
		
		if(numBlocks > std::numeric_limits<uint16_t>::max()) {
			throw InvalidArgumentException("Too many blocks. Try a bigger block size.");
		}

		filesystem::path pathname(path);
		
		FileEntry fe;
		fe.nameIndex = insertStringTable(pathname.filename().c_str());
		fe.pathIndex = insertStringTable(pathname.parent_path().c_str());
		fe.userIndex = insertStringTable(user);
		fe.groupIndex = insertStringTable(group);
		fe.type = (uint8_t)type;
		fe.mode = mode;
		fe.compression = (uint8_t)compression;
		fe.size = size;
		fe.mtime = mtime;
		fe.blockSizeLog = blockSizeLog;
		memcpy(fe.md5, md5, MD5_DIGEST_LENGTH);
		fe.numBlocks = numBlocks;
		fe.blockIndex = addBlockHashes(blockHashes, numBlocks);
		mFiles.insert(std::make_pair(path, fe));
	}
	
	const Snapshot::FileEntry *Snapshot::getFileEntry(const char *path)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		
		auto f = mFiles.find(path);
		if(f == mFiles.end()) {
			return nullptr;
		}
		
		return &f->second;
	}
	
	void Snapshot::forEachFileEntry(const std::function<void (const FileEntry&)>& callback)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		for(auto fe : mFiles) {
			callback(fe.second);
		}
	}
	
	void Snapshot::forEachFileEntry(const char *subpath, const std::function<void (const FileEntry&)>& callback)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		std::string searchPath = subpath;
		while(!searchPath.empty() && searchPath.back() == '/') {
			searchPath.pop_back();
		}

		size_t n = searchPath.size();
		auto found = mFiles.lower_bound(searchPath.c_str());
		while(found != mFiles.end()) {
			std::string filePath = std::string(indexToString(found->second.pathIndex)) + "/" + indexToString(found->second.nameIndex);
			if(strncmp(searchPath.c_str(), filePath.c_str(), n) != 0) break;
			if(!searchPath.empty() && filePath[n] != 0 && filePath[n] != '/') break;
			callback(found->second);
			++found;
		}
	}
	
	void Snapshot::deleteFileEntry(const char *path)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutex);

		auto entry = mFiles.find(path);
		if(entry != mFiles.end()) {
			mFiles.erase(entry);
		}
	}
	
	void Snapshot::load(InputStream& inStream)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutex);
		
		uint32_t numFiles = inStream.readType<uint32_t>();
		uint32_t stringTableSize = inStream.readType<uint32_t>() * 4;
		uint32_t hashesCount = inStream.readType<uint32_t>();
		
		mStringBuffer.resize(stringTableSize);
		inStream.readExpected(&mStringBuffer[0], stringTableSize);
		
		// build string table
		const char *p = &mStringBuffer[0];
		const char *end = p + stringTableSize;
		while(p < end) {
			mStringTable[p] = p - &mStringBuffer[0];
			size_t n = strlen(p);
			p += n + 1;
			while(!*p && p < end) ++p;
		}
		
		mBlockHashes.resize(hashesCount);
		for(int i = 0; i < hashesCount; ++i) {
			inStream.readExpected(mBlockHashes[i].hmac256, SHA256_DIGEST_LENGTH);
		}
		
		for(int i = 0; i < numFiles; ++i) {
			FileEntry fe;
			fe.nameIndex = inStream.readType<uint32_t>();
			fe.pathIndex = inStream.readType<uint32_t>();
			fe.userIndex = inStream.readType<uint32_t>();
			fe.groupIndex = inStream.readType<uint32_t>();
			fe.mode = inStream.readType<uint16_t>();
			fe.compression = inStream.readType<uint8_t>();
			inStream.readType<uint8_t>();
			fe.type = inStream.readType<uint8_t>();
			fe.blockSizeLog = inStream.readType<uint8_t>();
			fe.numBlocks = inStream.readType<uint16_t>();
			fe.size = inStream.readType<uint64_t>();
			fe.mtime = inStream.readType<uint64_t>();
			inStream.readExpected(fe.md5, MD5_DIGEST_LENGTH);
			fe.blockIndex = inStream.readType<uint32_t>();
			
			std::string path = std::string(indexToString(fe.pathIndex)) + "/" + indexToString(fe.nameIndex);
			mFiles[path] = fe;
		}
	}
	
	void Snapshot::save(OutputStream& outStream)
	{
		std::lock_guard<std::recursive_mutex> lock(mMutex);

		// file size, string table size, and block count
		outStream.writeType<uint32_t>(mFiles.size());
		outStream.writeType<uint32_t>((mStringBuffer.size() + 3) / 4); // string size is / 4
		outStream.writeType<uint32_t>(mBlockHashes.size());
		
		// string table
		outStream.write(&mStringBuffer[0], (mStringBuffer.size() + 3) & ~3);
		
		// block hashes
		for(int i = 0; i < mBlockHashes.size(); ++i) {
			outStream.write(mBlockHashes[i].hmac256, SHA256_DIGEST_LENGTH);
		}
		
		for(auto& fkv : mFiles)
		{
			const FileEntry& fe = fkv.second;
			outStream.writeType<uint32_t>(fe.nameIndex);
			outStream.writeType<uint32_t>(fe.pathIndex);
			outStream.writeType<uint32_t>(fe.userIndex);
			outStream.writeType<uint32_t>(fe.groupIndex);
			outStream.writeType<uint16_t>(fe.mode);
			outStream.writeType<uint8_t>(fe.compression);
			outStream.writeType<uint8_t>(0);
			outStream.writeType<uint8_t>(fe.type);
			outStream.writeType<uint8_t>(fe.blockSizeLog);
			outStream.writeType<uint16_t>(fe.numBlocks);
			outStream.writeType<uint64_t>(fe.size);
			outStream.writeType<uint64_t>(fe.mtime);
			outStream.write(fe.md5, MD5_DIGEST_LENGTH);
			outStream.writeType<uint32_t>(fe.blockIndex);
		}
	}

	const char *Snapshot::indexToString(int n) const
	{
		if(n < 0 || n >= mStringBuffer.size()) {
			throw IndexOutOfBoundException("Index is out of bound.");
		}
		return (const char *)&mStringBuffer[n];
	}
	
	const Snapshot::BlockHash *Snapshot::indexToBlockHash(int n) const
	{
		if(n < 0 || n >= mBlockHashes.size()) {
			throw IndexOutOfBoundException("Index is out of bound.");
		}
		return &mBlockHashes[n];
	}
	
	int Snapshot::insertStringTable(const char *str)
	{
		auto strIndex = mStringTable.find(str);
		if(strIndex != mStringTable.end()) {
			return strIndex->second;
		}

		size_t n = strlen(str) + 1;
		int idx = mStringBuffer.size();
		mStringBuffer.resize((idx + n + 3) & ~3);
		strncpy(&mStringBuffer[idx], str, n);

		mStringTable[str] = idx;

		return idx;
	}
	
	int Snapshot::addBlockHashes(const BlockHash *blockHashes, int count)
	{
		int idx = mBlockHashes.size();
		std::copy(blockHashes, blockHashes + count, std::back_inserter(mBlockHashes));
		return idx;
	}
}
