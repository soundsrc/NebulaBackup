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
#include <stdlib.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
extern "C" {
#include "compat/stdlib.h"
#include "compat/string.h"
}
#include "EncryptedOutputStream.h"
#include "ZeroedArray.h"
#include "Exception.h"
#include "DataStore.h"
#include "MemoryOutputStream.h"
#include "MemoryInputStream.h"
#include "Repository.h"

namespace Nebula
{
	Repository::Repository(DataStore *dataStore)
	: mDataStore(dataStore)
	{
		mEncKey = (uint8_t *)malloc(32);
		mMacKey = (uint8_t *)malloc(32);
	}
	
	Repository::~Repository()
	{
		explicit_bzero(mEncKey, 32);
		explicit_bzero(mMacKey, 32);
		free(mMacKey);
		free(mEncKey);
	}
	
	AsyncProgress<bool> Repository::initializeRepository(const char *password, int rounds)
	{
		std::shared_ptr<AsyncProgressData<bool>> asyncProgress = std::make_shared<AsyncProgressData<bool>>();

		// derive key from password
		ZeroedArray<uint8_t, 32> derivedKey;
		
		uint8_t salt[32];
		arc4random_buf(salt, sizeof(32));
		if(!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), rounds, EVP_sha256(), derivedKey.size(), derivedKey.data())) {
			throw EncryptionFailedException("Failed to set password.");
		}
		
		// random generate the encryption key
		arc4random_buf(mEncKey, 32);
		arc4random_buf(mMacKey, 32);
		
		// encrypt the encryption key with the derived key
		uint8_t keyData[1024];
		MemoryOutputStream keyStream(keyData, sizeof(keyData));

		keyStream.write("NEBULABACKUP", 12);
		uint32_t v;
		v = 0x00010000;
		keyStream.write(&v, sizeof(v));
		v = 0;
		keyStream.write(&v, sizeof(v));
		v = rounds;
		keyStream.write(&v, sizeof(v));

		keyStream.write(salt, 32);

		// encrypt key with the derived key
		EncryptedOutputStream encStream(keyStream, EVP_aes_256_cbc(), derivedKey.data());
		encStream.write(mEncKey, 32);
		encStream.write(mMacKey, 32);
		encStream.close();
		keyStream.close();

		MemoryInputStream keyStreamResult(keyStream.data(), keyStream.size());
		AsyncProgress<bool> putProgress = mDataStore->put("/key", keyStreamResult)
			.onDone([&asyncProgress](bool ready) {
				asyncProgress->setReady(ready);
			})
			.onProgress([&asyncProgress](float progress) {
				asyncProgress->setProgress(progress);
			})
			.onError([&asyncProgress](const std::string& errorMessage) {
				asyncProgress->setError(errorMessage);
			})
			.onCancelled([&asyncProgress] {
				asyncProgress->setCancelled();
			});

		return AsyncProgress<bool>(asyncProgress);
	}
}
