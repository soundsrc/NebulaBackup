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
#include <openssl/hmac.h>
extern "C" {
#include "compat/stdlib.h"
#include "compat/string.h"
}
#include "EncryptedOutputStream.h"
#include "DecryptedInputStream.h"
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
		arc4random_buf(salt, sizeof(salt));
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
		uint8_t encBytes[128];
		MemoryOutputStream outStream(encBytes, sizeof(encBytes));
		EncryptedOutputStream encStream(outStream, EVP_aes_256_cbc(), derivedKey.data());
		encStream.write(mEncKey, 32);
		encStream.write(mMacKey, 32);
		encStream.close();
		outStream.close();
		
		keyStream.write(outStream.data(), outStream.size());

		uint8_t hmac[32];
		unsigned int hmacLen = sizeof(hmac);
		if(!HMAC(EVP_sha256(), derivedKey.data(), derivedKey.size(),
				 outStream.data(), outStream.size(), hmac, &hmacLen)) {
			throw EncryptionFailedException("Failed to HMAC encrypted block.");
		}
		keyStream.write(hmac, sizeof(hmac));
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
	
	AsyncProgress<bool> Repository::unlockRepository(const char *password)
	{
		std::shared_ptr<AsyncProgressData<bool>> asyncProgress = std::make_shared<AsyncProgressData<bool>>();

		uint8_t keyData[1024];
		MemoryOutputStream keyStream(keyData, sizeof(keyData));
		
		AsyncProgress<bool> getProgress = mDataStore->get("/key", keyStream)
			.onDone([this, &asyncProgress, &keyStream, password](bool ready) {
				if(ready) {
					char magic[12];
					MemoryInputStream stream(keyStream.data(), keyStream.size());
					stream.read(magic, sizeof(magic));
					if(strncmp(magic, "NEBULABACKUP", 12) != 0) {
						asyncProgress->setError("Back up stream is invalid.");
						return;
					}
					uint32_t version = 0;
					stream.read(&version, sizeof(version));
					if(version != 0x00010000) {
						asyncProgress->setError("Invalid version number.");
						return;
					}
					
					uint32_t algorithm;
					stream.read(&algorithm, sizeof(algorithm)); // algorithm
					if(algorithm != 0) {
						asyncProgress->setError("Invalid algorithm.");
						return;
					}
					
					uint32_t rounds;
					stream.read(&rounds, sizeof(rounds));
					if(rounds > 100000) {
						asyncProgress->setError("Invalid numebr of rounds.");
						return;
					}
					
					uint8_t salt[32];
					stream.read(salt, sizeof(salt));
					ZeroedArray<uint8_t, 32> derivedKey;
					if(!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), rounds, EVP_sha256(), derivedKey.size(), derivedKey.data())) {
						asyncProgress->setError("Failed to unlock using password.");
						return;
					}

					uint8_t encKeyData[112];
					uint8_t hmac1[32];
					uint8_t hmac2[32];
					stream.read(encKeyData, sizeof(encKeyData));
					stream.read(hmac1, sizeof(hmac1));
					unsigned int hmacKeyLen = sizeof(hmac2);
					
					if(!HMAC(EVP_sha256(), derivedKey.data(), derivedKey.size(), encKeyData, sizeof(encKeyData), hmac2, &hmacKeyLen)) {
						asyncProgress->setError("Failed to unlock using password. HMAC failure.");
						return;
					}

					// password failure
					if(memcmp(hmac1, hmac2, sizeof(hmac1)) != 0) {
						asyncProgress->setReady(false);
						return;
					}

					MemoryInputStream encKeyStream(encKeyData, sizeof(encKeyData));

					DecryptedInputStream decStream(encKeyStream, EVP_aes_256_cbc(), derivedKey.data());
					decStream.read(mEncKey, 32);
					decStream.read(mMacKey, 32);
				}
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
		getProgress.wait();
		
		return AsyncProgress<bool>(asyncProgress);
	}
}
