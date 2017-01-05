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

#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
#include "ScopedExit.h"
#include "Exception.h"
#include "RollingHash.h"

namespace Nebula
{
	RollingHash::RollingHash(uint8_t *key, int windowSize)
	: mConstant(33)
	, mHash(0)
	, mEncHash(0)
	, mWindowSize(windowSize)
	, mIndex(windowSize - 1)
	{
		mWindow.resize(windowSize);
		
		// there is definately a faster way to calculate this
		for(int i = 0; i < windowSize; ++i) {
			mConstantPowWinSize *= Constant;
		}

		memcpy(mKey.data(), key, 32);
	}
	
	RollingHash::~RollingHash()
	{
	}

	uint32_t RollingHash::roll(uint8_t c)
	{
		mIndex = (mIndex + 1) % mWindowSize;
		uint32_t sub = mConstantPowWinSize * mWindow[mIndex];
		mWindow[mIndex] = c;
		mHash = (mHash * mConstant) + c - sub;
#if 0
		EVP_CIPHER_CTX ctx;
		
		EVP_CIPHER_CTX_init(&ctx);
		ScopedExit onExit([&ctx] { EVP_CIPHER_CTX_cleanup(&ctx); });
		
		if(!EVP_CipherInit(&ctx, EVP_aes_128_ecb(), mKey.data(), nullptr, 1)) {
			throw EncryptionFailedException("Failed to initialize cipher.");
		}
		
		uint8_t md[EVP_MAX_BLOCK_LENGTH];
		int outLen = EVP_MAX_BLOCK_LENGTH;
		if(!EVP_CipherUpdate(&ctx, md, &outLen, (unsigned char *)&mHash, sizeof(mHash))) {
			throw EncryptionFailedException("Failed to encrypt rolling hash.");
		}
		
		if(!EVP_CipherFinal(&ctx, md + outLen, &outLen)) {
			throw EncryptionFailedException("Failed to encrypt rolling hash.");
		}
		
		mEncHash = (md[0] << 24 | md[1] << 16 | md[2] << 8 | md[3]);
#else
		mEncHash = mHash;
#endif

		return mEncHash;
	}
}
