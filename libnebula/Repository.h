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
#include "CompressionType.h"

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
		struct Options
		{
			/**
			 * Small file size setting.
			 * If the upload file size is smaller than this, do not split 
			 */
			int smallFileSize;
			
			/**
			 * Maximum block size.
			 */
			int maxBlockSize;
			
			/// Minimum block size log
			int minRollingHashBits;
			
			/// Maximum block size log
			int maxRollingHashBits;
			
			/// when dynamically splitting blocks, increasing the blockSplitCount
			/// increases the average number of chunks the file with be split 
			int blockSplitCount;
			
			Options();
		};
		
		struct PackUploadState;
		struct PackFileInfo;

		/**
		 * Creates a new instance of repository, supplying a backend data store.
		 * If the repository is an existing repository, then you should call
		 * unlockRepository
		 */
		Repository(DataStore *dataStore, Options *options = nullptr);
		~Repository();
		
		/**
		 * Initializes and creates a new repository.
		 * The files and data structures will be setup using
		 * the supplied password.
		 */
		void initializeRepository(const char *password, uint8_t logRounds = 17, ProgressFunction progress = DefaultProgressFunction);
		
		/**
		 * Unlocks an existing repository. This should be called before
		 * any other operation except the creation of a new repository.
		 * Returns true if the supplied password is able to unlock the
		 * correct.
		 */
		bool unlockRepository(const char *password, ProgressFunction progress = DefaultProgressFunction);

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
		bool changePassword(const char *oldPassword, const char *newPassword, uint8_t logRounds = 17, ProgressFunction progress = DefaultProgressFunction);

		/**
		 * Lists available snapshots in the repository
		 */
		void listSnapshots(const std::function<void (const char *)>& callback, ProgressFunction progress = DefaultProgressFunction);

		/**
		 * Creates a new snapshot. A snapshot is a collection of file.
		 */
		std::shared_ptr<Snapshot> createSnapshot();
		
		/**
		 * Creates a snapshot based on an existing snapshot.
		 * Useful for making incremental backups.
		 */
		std::shared_ptr<Snapshot> loadSnapshot(const char *name, ProgressFunction progress = DefaultProgressFunction);

		/**
		 * Uploads a file to the repository. Adds the entry to the snapshot.
		 * Until a snapshot is committed, it is loose file which may be
		 * compacted via the compactRepository() method.
		 */
		void uploadFile(std::shared_ptr<Snapshot> snapshot, const char *destPath, FileStream& fileStream, FileTransferProgressFunction progress = DefaultFileTransferProgressFunction);
		
		/**
		 * Downloads a file to the output stream.
		 */
		bool downloadFile(std::shared_ptr<Snapshot> snapshot, const char *srcPath, OutputStream& fileStream, FileTransferProgressFunction progress = DefaultFileTransferProgressFunction);

		std::shared_ptr<PackUploadState> createPackState();
		void uploadFile(std::shared_ptr<PackUploadState> packUploadState, std::shared_ptr<Snapshot> snapshot, const char *destPath, FileStream& fileStream, FileTransferProgressFunction progress = DefaultFileTransferProgressFunction);
		void finalizePack(std::shared_ptr<Snapshot> snapshot, std::shared_ptr<PackUploadState> uploadPackState, FileTransferProgressFunction progress = DefaultFileTransferProgressFunction);
		
		/**
		 * Commits the snapshot to the repository.
		 */
		void commitSnapshot(std::shared_ptr<Snapshot> snapshot, const char *name, ProgressFunction progress = DefaultProgressFunction);

	private:
		enum { MAX_LOG_ROUNDS = 31 };

		DataStore *mDataStore;
		Options mOptions;

		uint8_t *mEncKey;
		uint8_t *mMacKey;
		uint8_t *mHashKey;
		uint8_t *mRollKey;

		void computeBlockHMAC(const uint8_t *block, size_t size, uint8_t compression, uint8_t *outHMAC);
		void compressEncryptAndUploadBlock(CompressionType compressType, const Snapshot::ObjectID& objectId, const uint8_t *block, size_t size, ProgressFunction progress);

		std::string objectIdToString(const Snapshot::ObjectID& objectId) const;
		
		void writeRepositoryKey(const char *password, uint8_t logRounds = 17, ProgressFunction progress = DefaultProgressFunction);
	};
}
