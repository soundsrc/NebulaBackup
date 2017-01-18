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

#include "RollingHash.h"
#include <stdint.h>
#include <string.h>
#include <memory>
#include <openssl/evp.h>
#include "ScopedExit.h"
#include "Exception.h"

namespace Nebula
{
	RollingHash::RollingHash(const uint8_t *key, int windowSize)
	: mHash(0)
	, mConstant(33)
	, mWindowSize(windowSize)
	, mIndex(windowSize - 1)
	{
		mWindow.resize(windowSize);

		// mConstantPowWinSize = (mConstant ^ windowSize) mod (2^64-1)

		mConstantPowWinSize = 1;
		uint32_t base = mConstant;
		while(windowSize > 0) {
			if(windowSize & 1) {
				mConstantPowWinSize = (base * mConstantPowWinSize);
			}
			base = base * base;
			windowSize >>= 1;
		}

		memcpy(mKey.data(), key, 32);
		
		EVP_CIPHER_CTX ctx;
		
		EVP_CIPHER_CTX_init(&ctx);
		std::unique_ptr<EVP_CIPHER_CTX, decltype(EVP_CIPHER_CTX_cleanup) *>
			onExit(&ctx, EVP_CIPHER_CTX_cleanup);
		
		uint8_t iv[EVP_MAX_IV_LENGTH];
		memset(iv, 0xAA, sizeof(iv));
		if(!EVP_CipherInit(&ctx, EVP_aes_256_cbc(), mKey.data(), iv, 1)) {
			throw EncryptionFailedException("Failed to initialize cipher.");
		}
		
		uint8_t encBlock[256 * sizeof(uint32_t) - 16];
		memset(encBlock, 0x33, sizeof(encBlock));
		
		int outLen = 256 * sizeof(uint32_t);
		if(!EVP_CipherUpdate(&ctx, (uint8_t *)mSubTable.data(), &outLen, encBlock, sizeof(encBlock))) {
			throw EncryptionFailedException("Failed to encrypt rolling hash.");
		}
		
		outLen = 256 * sizeof(uint32_t) - outLen;
		if(!EVP_CipherFinal(&ctx, (uint8_t *)mSubTable.data() + outLen, &outLen)) {
			throw EncryptionFailedException("Failed to encrypt rolling hash.");
		}
	}
	
	RollingHash::~RollingHash()
	{
	}

	uint64_t RollingHash::roll(uint8_t c)
	{
		mIndex = (mIndex + 1) % mWindowSize;
		uint64_t sub = mConstantPowWinSize * mSubTable[mWindow[mIndex]];
		mWindow[mIndex] = c;
		mHash = (mHash * mConstant) + mSubTable[c] - sub;

		return mHash;
	}
}
