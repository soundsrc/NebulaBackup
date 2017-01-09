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
					  uint64_t size,
					  time_t mtime,
					  uint8_t blockSizeLog,
					  const uint8_t *sha256,
					  int numBlocks,
					  const BlockHash *blockHashes)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		
		FileEntry fe;
		fe.pathIndex = insertStringTable(path);
		fe.userIndex = insertStringTable(user);
		fe.groupIndex = insertStringTable(group);
		fe.type = (uint8_t)type;
		fe.mode = mode;
		fe.size = size;
		fe.mtime = mtime;
		fe.blockSizeLog = blockSizeLog;
		memcpy(fe.sha256, sha256, SHA256_DIGEST_LENGTH);
		fe.numBlocks = numBlocks;
		fe.blockIndex = addBlockHashes(blockHashes, numBlocks);
		mFiles.insert(std::make_pair(path, fe));
	}
	
	const Snapshot::FileEntry *Snapshot::getFileEntry(const char *path)
	{
		std::lock_guard<std::mutex> lock(mMutex);
		
		auto f = mFiles.find(path);
		if(f == mFiles.end()) {
			return nullptr;
		}
		
		return &f->second;
	}
	
	int Snapshot::insertStringTable(const char *str)
	{
		auto strIndex = mStringTable.find(str);
		if(strIndex != mStringTable.end()) {
			return strIndex->second;
		}

		size_t n = strlen(str) + 1;
		int idx = mStringBuffer.size();
		mStringBuffer.resize((mStringBuffer.size() + n + 3) & ~3);
		strncpy(&mStringBuffer[idx], str, n);

		return n;
	}
	
	int Snapshot::addBlockHashes(const BlockHash *blockHashes, int count)
	{
		int idx = mBlockHashes.size();
		std::copy(blockHashes, blockHashes + count, std::back_inserter(mBlockHashes));
		return idx;
	}
}
