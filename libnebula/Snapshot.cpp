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
#include <memory>
#include <mutex>
#include <math.h>
#include <ctype.h>
#include <boost/filesystem.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "Exception.h"
#include "DataStore.h"
#include "BufferedInputStream.h"
#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"
#include "FileStream.h"
#include "FileInfo.h"
#include "Repository.h"
#include "LZMAUtils.h"
#include "EncryptedOutputStream.h"
#include "ScopedExit.h"
#include "RollingHash.h"
#include "Snapshot.h"

namespace Nebula
{
	Snapshot::Snapshot(Repository& repo)
	: mRepository(repo)
	, mFiles(FileEntryComparer(mStringBuffer), ZeroedAllocator<FileEntry>())
	{
	}
	
	Snapshot::~Snapshot()
	{
	}
	
	char Snapshot::nibbleToHex(uint8_t nb) const
	{
		if(nb >= 0 && nb <= 9) return '0' + nb;
		if(nb < 16) {
			return 'a' + (nb - 10);
		}
		return 0;
	}
	
	int Snapshot::hexToNibble(char h) const
	{
		char lh = tolower(h);
		if(lh >= '0' && lh <= '9') return lh - '0';
		if(lh >= 'a' && lh <= 'a') return lh - 'a' + 10;
		return 0;
	}

	std::string Snapshot::hmac256ToString(uint8_t *hmac) const
	{
		std::string hmacStr;
		hmacStr.reserve(64);
		for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
			hmacStr += nibbleToHex(hmac[i] >> 4);
			hmacStr += nibbleToHex(hmac[i] & 0xF);
		}
		
		return hmacStr;
	}
	
	void Snapshot::hmac256strToHmac(const char *str, uint8_t *outHmac)
	{
		for(int i = 0; i < SHA256_DIGEST_LENGTH * 2; i += 2) {
			outHmac[i >> 1] = hexToNibble(str[i]) << 4 | hexToNibble(str[i]);
		}
	}
	
	struct Snapshot::MallocDeletor
	{
		inline void operator ()(uint8_t *p) {
			free(p);
		}
	};

	void Snapshot::uploadBlock(const uint8_t *block, size_t size, uint8_t *outhmac, ProgressFunction progress)
	{
		if(!HMAC(EVP_sha256(), mRepository.hashKey(), SHA256_DIGEST_LENGTH, block, size, outhmac, nullptr)) {
			throw EncryptionFailedException("Failed to HMAC block.");
		}
		
		// if the block already exists in the repository, skip the upload
		std::string uploadPath = "/data/" + hmac256ToString(outhmac);
		if(mRepository.dataStore()->exist(uploadPath.c_str())) {
			return;
		}
		
		MemoryInputStream blockStream(block, size);
		
		std::unique_ptr<uint8_t, MallocDeletor>
			compressData( (uint8_t *)malloc(size * 2) );

		MemoryOutputStream compressedStream(compressData.get(), size * 2);
		LZMAUtils::compress(blockStream, compressedStream, nullptr);
		compressedStream.close();

		std::unique_ptr<uint8_t, MallocDeletor>
			encryptedData( (uint8_t *)malloc(compressedStream.size() + EVP_MAX_BLOCK_LENGTH) );
		MemoryInputStream encInStream(compressedStream.data(), compressedStream.size());
		MemoryOutputStream encOutStream(encryptedData.get(), compressedStream.size() + EVP_MAX_BLOCK_LENGTH);
		encInStream.copyTo(encOutStream);
		encOutStream.close();
		
		MemoryInputStream uploadStream(encOutStream.data(), encOutStream.size());
		mRepository.dataStore()->put(("/data/" + hmac256ToString(outhmac)).c_str(), uploadStream);
	}

	void Snapshot::uploadFile(const char *path, FileStream& fileStream, ProgressFunction progress)
	{
		using namespace boost;

		RollingHash rh(mRepository.rollKey(), 8192);

		FileEntry fileEntry;
		
		FileInfo fileInfo( fileStream.path().c_str() );
		if(!fileInfo.exists()) {
			throw FileNotFoundException("File not found.");
		}

		fileEntry.size = fileInfo.length();
		fileEntry.mode = fileInfo.mode();
		fileEntry.type = (int)fileInfo.type();
		fileEntry.numBlocks = 0;
		
		std::vector<BlockHash> blockHashes;
		blockHashes.reserve(32);
		
		SHA256_CTX sha256;
		SHA256_Init(&sha256);

		// don't bother with splitting the file if it's < 1MB
		// just upload as is
		if(fileEntry.size < 1024 * 1024) {

			std::unique_ptr<uint8_t, MallocDeletor>
				buffer( (uint8_t *)malloc(fileEntry.size) );
			if(fileStream.read(buffer.get(), fileEntry.size) != fileEntry.size) {
				throw FileIOException("Failed to read file.");
			}
			
			SHA256_Update(&sha256, buffer.get(), fileEntry.size);

			BlockHash blockHash;
			blockHash.size = fileEntry.size;
			uploadBlock(buffer.get(), fileEntry.size, blockHash.hmac256, progress);
			blockHashes.push_back(blockHash);

		} else {

			// the block size is dynamically determined based on file length
			// and increases bigger for larger file sizes
			// TODO: make these values configurable
			fileEntry.blockSizeLog = ceil(log(fileEntry.size / 8) / log(2));
			if(fileEntry.blockSizeLog  < 16) fileEntry.blockSizeLog  = 16;
			if(fileEntry.blockSizeLog  > 25) fileEntry.blockSizeLog  = 25;
			uint32_t hashMask = (1 << fileEntry.blockSizeLog) - 1;

			std::vector<uint8_t> blockBuffer;
			blockBuffer.reserve(1 << (1 + fileEntry.blockSizeLog));
			
			BufferedInputStream bufferedFile(fileStream);
			while(!bufferedFile.isEof()) {
				uint8_t b = bufferedFile.readByte();
				blockBuffer.push_back(b);
				if((rh.roll(b) & hashMask) == 0) {
					
					SHA256_Update(&sha256, &blockBuffer[0], blockBuffer.size());
					
					BlockHash blockHash;
					blockHash.size = blockBuffer.size();
					// upload block
					uploadBlock(&blockBuffer[0], blockBuffer.size(), blockHash.hmac256, progress);
					blockHashes.push_back(blockHash);
					blockBuffer.clear();
				}
			}

			if(!blockBuffer.empty()) {
				SHA256_Update(&sha256, &blockBuffer[0], blockBuffer.size());
				uploadBlock(&blockBuffer[0], blockBuffer.size(), nullptr, progress);
				++fileEntry.numBlocks;
			}
		}

		// write the SHA256
		SHA256_Final(fileEntry.sha256, &sha256);

		// update the index
		std::lock_guard<std::mutex> lock(mMutex);

		fileEntry.pathIndex = insertStringTable(path);
		fileEntry.userIndex = insertStringTable(fileInfo.userName().c_str());
		fileEntry.groupIndex = insertStringTable(fileInfo.groupName().c_str());
		fileEntry.numBlocks = blockHashes.size();
		fileEntry.blockIndex = mBlockHashes.size();
		std::copy(blockHashes.begin(), blockHashes.end(), std::back_inserter(mBlockHashes));

		mFiles.insert(fileEntry);
	}
	
	int Snapshot::insertStringTable(const char *str)
	{
		auto strIndex = mStringTable.find(str);
		if(strIndex != mStringTable.end()) {
			return strIndex->second;
		}

		size_t n = strlen(str);
		int idx = mStringBuffer.size();
		mStringBuffer.resize((mStringBuffer.size() + n + 3) & ~3);
		strncpy(&mStringBuffer[idx], str, n + 1);

		return n;
	}
}
