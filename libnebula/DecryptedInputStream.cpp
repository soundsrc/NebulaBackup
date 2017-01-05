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
#include "Exception.h"
#include "DecryptedInputStream.h"

namespace Nebula
{
	DecryptedInputStream::DecryptedInputStream(InputStream& stream, const EVP_CIPHER *cipher, const uint8_t *key)
	: mStream(stream)
	, mBytesInBuffer(0)
	, mDone(false)
	{
		mCtx = EVP_CIPHER_CTX_new();
		if(!mCtx) {
			throw EncryptionFailedException("Failed to create cipher.");
		}
		
		uint8_t iv[EVP_MAX_IV_LENGTH];
		if(stream.read(iv, sizeof(iv)) != sizeof(iv)) {
			throw EncryptionFailedException("Failed to read encryption iv.");
		}
		
		if(!EVP_DecryptInit_ex(mCtx, cipher, nullptr, key, iv)) {
			EVP_CIPHER_CTX_free(mCtx);
			throw EncryptionFailedException("Failed to initialize encryption.");
		}
	}
	
	DecryptedInputStream::~DecryptedInputStream()
	{
		EVP_CIPHER_CTX_free(mCtx);
	}
	
	size_t DecryptedInputStream::read(void *data, size_t size)
	{
		uint8_t buffer[4096 + EVP_MAX_BLOCK_LENGTH];
		size_t n;
		uint8_t *p = (uint8_t *)data;
		size_t bytesRead = 0;

		while(size > mBytesInBuffer) {
			if(mBytesInBuffer > 0) {
				memcpy(p, mBufferPtr, mBytesInBuffer);
				size -= mBytesInBuffer;
				p += mBytesInBuffer;
				bytesRead += mBytesInBuffer;
				mBytesInBuffer = 0;
			}

			mBufferPtr = mBuffer.data();
			n = mStream.read(buffer, 4096);
			if(!n) {
				if(!mDone) {
					if(!EVP_DecryptFinal_ex(mCtx, mBuffer.data(), &mBytesInBuffer)) {
						throw EncryptionFailedException("EVP_DecryptFinal_ex: Decryption error.");
					}
					mDone = true;
					continue;
				}

				if(bytesRead > 0) return bytesRead;

				return 0;
			}

			if(!EVP_DecryptUpdate(mCtx, mBuffer.data(), &mBytesInBuffer, buffer, n)) {
				throw EncryptionFailedException("Decryption error.");
			}
		}

		memcpy(p, mBufferPtr, size);
		mBufferPtr += size;
		bytesRead += size;
		mBytesInBuffer -= size;

		return bytesRead;
	}
}
