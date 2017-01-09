/*
 * Copyright (c) 2016 Sound <sound@sagaforce.com>
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

#pragma once

#include <mutex>
#include <string>
#include <map>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <openssl/sha.h>
#include "ProgressFunction.h"
#include "FileInfo.h"
#include "ZeroedString.h"
#include "ZeroedAllocator.h"

namespace Nebula
{
	class OutputStream;
	class FileStream;
	class Repository;

	/**
	 * A snapshot is a collection of files and metadata in the backup.
	 */
	class Snapshot
	{
	public:
		Snapshot();
		~Snapshot();

		struct BlockHash
		{
			uint8_t hmac256[SHA256_DIGEST_LENGTH];
		};
		
		struct FileEntry
		{
			int pathIndex; // pathname to the file
			int userIndex; // unix user id
			int groupIndex; // unix group name
			uint16_t mode; // mode
			uint64_t size; // size of file in bytes
			time_t mtime; // modify time
			uint8_t type; // flags
			uint8_t blockSizeLog;
			uint16_t numBlocks;
			uint8_t sha256[SHA256_DIGEST_LENGTH];
			int blockIndex;
		};

		void addFileEntry(const char *path,
						  const char *user,
						  const char *group,
						  FileType type,
						  uint16_t mode,
						  uint64_t size,
						  time_t mtime,
						  uint8_t blockSizeLog,
						  const uint8_t *sha256,
						  int numBlocks,
						  const BlockHash *blockHashes);
		
		const FileEntry *getFileEntry(const char *path);
	
		//void listFileEntries();
		//void deleteFileEntry(const char *path);
		
		
		const char *indexToString(int n) const { return (const char *)&mStringBuffer[n]; }
		const BlockHash *indexToBlockHash(int n) const { return &mBlockHashes[n]; }
	private:

		class StringComparer
		{
		public:
			bool operator ()(const char *a, const char *b) const {
				return strcmp(a, b) < 0;
			}
		};

		//
		std::map<const char *, int, StringComparer> mStringTable;
		std::vector<char, ZeroedAllocator<char>> mStringBuffer;
		std::vector<BlockHash, ZeroedAllocator<BlockHash>> mBlockHashes;
		std::map<std::string, FileEntry, std::less<std::string>, ZeroedAllocator<FileEntry>> mFiles;

		std::mutex mMutex;
	
		int insertStringTable(const char *str);
		int addBlockHashes(const BlockHash *blockHashes, int count);
	};
}
