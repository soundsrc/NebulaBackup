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

#include <memory>
#include <vector>
#include <functional>
#include <inttypes.h>
#include "ProgressFunction.h"
#include "Snapshot.h"

namespace Nebula
{
	class DataStore;
	
	/**
	 * Represents a backup repository. The repository is backed by a data store
	 * backend (i.e. SSH, S3, etc..).
	 */
	class FileInputStream;
	class Repository
	{
	public:
		/**
		 * Creates a new instance of repository, supplying a backend data store.
		 * If the repository is an existing repository, then you should call
		 * unlockRepository
		 */
		Repository(DataStore *dataStore);
		~Repository();
		
		/**
		 * Initializes and creates a new repository.
		 * The files and data structures will be setup using
		 * the supplied password.
		 */
		void initializeRepository(const char *password, int rounds = 4096);
		
		/**
		 * Unlocks an existing repository. This should be called before
		 * any other operation except the creation of a new repository.
		 * Returns true if the supplied password is able to unlock the
		 * correct.
		 */
		bool unlockRepository(const char *password);

		/**
		 * Compacts a repository.
		 * The repository is scanned for snapshots and each object is checked
		 * to ensure there is a reference to the object. At the end of the scan,
		 * objects which have no references are removed.
		 */
		bool compactRepository();
		
		/**
		 * Initiates a change password operation.
		 * @param oldPassword
		 */
		bool changePassword(const char *oldPassword, const char *newPassword);

		/**
		 * Creates a new snapshot. A snapshot is a collection of file.
		 */
		Snapshot *createSnapshot();
		
		/**
		 * Creates a snapshot based on an existing snapshot.
		 * Useful for making incremental backups.
		 */
		Snapshot *loadSnapshot(const char *name, ProgressFunction progress = DefaultProgressFunction);

		/**
		 * Lists the files in the snapshot.
		 * The callback function is invoked for each file.
		 */
		void listFiles(Snapshot *snapshot, const std::function<void (const char *, void *)>& listCallback, void *userData = nullptr, ProgressFunction progress = DefaultProgressFunction);
		
		/**
		 * Uploads a file to the repository. Adds the entry to the snapshot.
		 * Until a snapshot is committed, it is loose file which may be
		 * compacted via the compactRepository() method.
		 */
		void uploadFile(Snapshot *snapshot, const char *destPath, FileStream& fileStream, ProgressFunction progress = DefaultProgressFunction);
		
		/**
		 * Downloads a file to the output stream.
		 */
		bool downloadFile(Snapshot *snapshot, const char *srcPath, OutputStream& fileStream, ProgressFunction progress = DefaultProgressFunction);
		
		
		/**
		 * Commits the snapshot to the repository.
		 */
		void commitSnapshot(Snapshot *snapshot, const char *name, ProgressFunction progress = DefaultProgressFunction);

	private:
		DataStore *mDataStore;
		uint8_t *mEncKey;
		uint8_t *mMacKey;
		uint8_t *mHashKey;
		uint8_t *mRollKey;
		
		struct MallocDeletor;

		void compressEncryptAndUploadBlock(const uint8_t *block, size_t size, uint8_t *outhmac, ProgressFunction progress);
		
		char nibbleToHex(uint8_t nb) const;
		int hexToNibble(char h) const;
		std::string hmac256ToString(const uint8_t *hmac) const;
		void hmac256strToHmac(const char *str, uint8_t *outHmac);
	};
}
