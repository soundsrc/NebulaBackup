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
		uint64_t base = mConstant;
		int u = windowSize;
		while(u > 0) {
			if(u & 1) {
				mConstantPowWinSize = (base * mConstantPowWinSize);
			}
			base = base * base;
			u >>= 1;
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
		
		uint8_t inBlock[256 * 4], outBlock[256 * 4 + EVP_MAX_BLOCK_LENGTH];
		memset(inBlock, 0x33, sizeof(inBlock));
		
		int outLen = sizeof(outBlock);
		if(!EVP_CipherUpdate(&ctx, outBlock, &outLen, inBlock, sizeof(inBlock))) {
			throw EncryptionFailedException("Failed to encrypt rolling hash.");
		}

		int finalLen = sizeof(outBlock) - outLen;
		if(!EVP_CipherFinal(&ctx, outBlock + outLen, &finalLen)) {
			throw EncryptionFailedException("Failed to encrypt rolling hash.");
		}

		for(int i = 0; i < 256; ++i) {
			const uint8_t *p = outBlock + (i << 2);
			mSubTable[i] = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
		}

		// compute the initial hash
		mHash = 0;
		for(int i = 0; i < windowSize; ++i) {
			mHash = (mHash * mConstant) + mSubTable[0];
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
