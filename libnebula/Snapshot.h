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

#include <string>
#include <vector>
#include <stdint.h>
#include "libnebula/AsyncProgress.h"
#include "libnebula/ZeroedString.h"
#include "libnebula/ZeroedAllocator.h"

namespace Nebula
{
	class OutputStream;
	class InputStream;
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
		 * Uploads a file to the backup. The upload algorithm may optimize
		 * bandwidth here, by not retransferring existing data.
		 * A file maybe uploaded to the backup, however, it won't have an
		 * reference until commit() is called.
		 */
		AsyncProgress<bool> uploadFile(const char *path, InputStream& inputStream);
		
		/**
		 * Download a file.
		 */
		AsyncProgress<bool> downloadFile(const char *path, OutputStream& outputStream);

		/**
		 * Loads an existing snapshot from the repo.
		 */
		AsyncProgress<bool> load(const char *name);

		/**
		 * A snapshot is not complete until we commit the snapshot data.
		 * The snapshot is
		 * @param name Snapshot name, which should be unique from other snapshots.
		 */
		AsyncProgress<bool> commit(const char *name);
	private:
		std::string mName;
		
		enum class TypeFlag
		{
			NormalFile,
			SymLink,
			CharacterSpecial,
			BlockSpecial
		};
		
		struct BlockHash
		{
			int size;
			uint8_t sha256[32];
		};

		struct FileInfo
		{
			ZeroedString path; // pathname to the file
			ZeroedString uid; // unix user id
			ZeroedString group; // unix group name
			long size; // size of file in bytes
			time_t mtime; // modify time
			uint32_t type; // flags
			uint8_t sha256[32];
			int blockSize; // >0 defines block-size
			std::vector<BlockHash> blockList;
		};
		
		std::vector<FileInfo, ZeroedAllocator<FileInfo>> mFiles;
	};
}
