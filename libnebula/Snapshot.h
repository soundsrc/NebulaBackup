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
#include <set>
#include <map>
#include <vector>
#include <stdint.h>
#include <string.h>
#include "ProgressFunction.h"
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
		Snapshot(Repository& repo);
		~Snapshot();
		
		/**
		 * Uploads a file to the backup.
		 * The upload algorithm may optionally split the file into multiple
		 * blocks and upload each individually, and may choose to 
		 * A file maybe uploaded to the backup, however, it won't have an
		 * reference until commit() is called.
		 */
		void uploadFile(const char *path, FileStream& fileStream, ProgressFunction progress = DefaultProgressFunction);
		
		/**
		 * Download a file.
		 */
		void downloadFile(const char *path, OutputStream& outputStream, ProgressFunction progress = DefaultProgressFunction);

		/**
		 * Loads an existing snapshot from the repo.
		 */
		void load(const char *name);

		/**
		 * A snapshot is not complete until we commit the snapshot data.
		 * The snapshot is
		 * @param name Snapshot name, which should be unique from other snapshots.
		 */
		void commit(const char *name);
	private:
		Repository& mRepository;
		std::string mName;
		
		struct BlockHash
		{
			int size;
			uint8_t hmac256[32];
		};

		struct FileEntry
		{
			int path; // pathname to the file
			int user; // unix user id
			int group; // unix group name
			uint16_t mode; // mode
			uint64_t size; // size of file in bytes
			time_t mtime; // modify time
			uint8_t type; // flags
			uint8_t blockSizeLog;
			uint16_t numBlocks;
			uint8_t sha256[32];
			BlockHash *blockList;
		};
		
		class StringComparer
		{
		public:
			bool operator ()(const char *a, const char *b) const {
				return strcmp(a, b) < 0;
			}
		};
		
		class FileEntryComparer
		{
		public:
			explicit FileEntryComparer(const std::vector<char, ZeroedAllocator<char>>& stringTable) : mStringTable(stringTable) { }
			
			bool operator ()(const FileEntry& a, const FileEntry& b) {
				return strcmp(&mStringTable[a.path], &mStringTable[b.path]) < 0;
			}
		private:
			const std::vector<char, ZeroedAllocator<char>>& mStringTable;
		};
		
		struct MallocDeletor;

		//
		std::map<const char *, int, StringComparer> mStringTable;
		std::vector<char, ZeroedAllocator<char>> mStringBuffer;
		std::vector<BlockHash, ZeroedAllocator<BlockHash>> mBlockHashes;
		std::set<FileEntry, FileEntryComparer, ZeroedAllocator<FileEntry>> mFiles;
		
		std::mutex mMutex;
	
		char nibbleToHex(uint8_t nb) const;
		int hexToNibble(char h) const;
		std::string hmac256ToString(uint8_t *hmac) const;
		void hmac256strToHmac(const char *str, uint8_t *outHmac);
		void uploadBlock(const uint8_t *block, size_t size, uint8_t *outhmac, ProgressFunction progress = DefaultProgressFunction);
		int insertStringTable(const char *str);
	};
}
