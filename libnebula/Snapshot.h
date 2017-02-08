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
#include <openssl/md5.h>
#include "ProgressFunction.h"
#include "FileInfo.h"
#include "ZeroedString.h"
#include "ZeroedAllocator.h"
#include "CompressionType.h"

namespace Nebula
{
	class InputStream;
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

		struct ObjectID
		{
			uint8_t id[SHA256_DIGEST_LENGTH];
		};
		
		struct FileEntry
		{
			uint32_t nameIndex; // filename
			uint32_t pathIndex; // parent path to the file
			uint32_t userIndex; // unix user id
			uint32_t groupIndex; // unix group name
			uint16_t mode; // mode
			uint8_t compression;
			uint64_t size; // size of file in bytes
			time_t mtime; // modify time
			uint8_t type; // flags
			uint8_t rollingHashBits;
			uint16_t objectCount;
			uint8_t md5[MD5_DIGEST_LENGTH];
			uint32_t offset;
			uint32_t packLength;
			uint32_t objectIdIndex;
		};

		void addFileEntry(const char *path,
						  const char *user,
						  const char *group,
						  FileType type,
						  uint16_t mode,
						  CompressionType compression,
						  uint64_t size,
						  time_t mtime,
						  uint8_t rollingHashBits,
						  const uint8_t *md5,
						  uint32_t offset,
						  uint32_t packLength,
						  int objectCount,
						  const ObjectID *objectIds);

		const FileEntry *getFileEntry(const char *path);
	

		void forEachFileEntry(const std::function<void (const FileEntry&)>& callback);
		
		void forEachFileEntry(const char *subpath, const std::function<void (const FileEntry&)>& callback);

		void deleteFileEntry(const char *path);
		
		void save(OutputStream& outStream);
		void load(InputStream& inStream);
		
		const char *indexToString(int n) const;
		const ObjectID *indexToObjectID(int n) const;
	private:
		//
		std::map<std::string, int> mStringTable;
		std::vector<char, ZeroedAllocator<char>> mStringBuffer;
		std::vector<ObjectID, ZeroedAllocator<ObjectID>> mObjectIDs;
		std::map<std::string, FileEntry, std::less<std::string>, ZeroedAllocator<FileEntry>> mFiles;

		std::recursive_mutex mMutex;
	
		int insertStringTable(const char *str);
		int addObjectIds(const ObjectID *objectIds, int count);
	};
}
