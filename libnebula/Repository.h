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

#include <vector>
#include <inttypes.h>
#include "AsyncProgress.h"
#include "Snapshot.h"

namespace Nebula
{
	class DataStore;
	
	/**
	 * Represents a backup repository. The repository is backed by a data store
	 * backend (i.e. SSH, S3, etc..).
	 */
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
		AsyncProgress<bool> initializeRepository(const char *password, int rounds = 4096);
		
		/**
		 * Unlocks an existing repository. This should be called before
		 * any other operation except the creation of a new repository.
		 * Returns true if the supplied password is able to unlock the
		 * correct.
		 */
		AsyncProgress<bool> unlockRepository(const char *password);

		/**
		 * Compacts a repository.
		 * The repository is scanned for snapshots and each object is checked
		 * to enture there is a reference to the object. At the end of the scan,
		 * objects which have no references are removed.
		 */
		AsyncProgress<bool> compactRepository();
		
		/**
		 * Initiates a change password operation.
		 * @param oldPassword
		 */
		AsyncProgress<bool> changePassword(const char *oldPassword, const char *newPassword);

	private:
		DataStore *mDataStore;
		uint8_t *mEncKey;
		uint8_t *mMacKey;
	};
}
