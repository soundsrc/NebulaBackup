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

#include "EncryptedOutputStream.h"
#include <openssl/evp.h>
#include "ZeroedArray.h"
#include "Exception.h"

namespace Nebula
{
	EncryptedOutputStream::EncryptedOutputStream(OutputStream& stream, const EVP_CIPHER *cipher, const uint8_t *key)
	: mStream(stream)
	{
		mCtx = EVP_CIPHER_CTX_new();
		if(!mCtx) {
			throw EncryptionFailedException("Failed to create cipher.");
		}
		
		uint8_t iv[EVP_MAX_IV_LENGTH];
		arc4random_buf(iv, sizeof(iv));

		if(!EVP_EncryptInit_ex(mCtx, cipher, nullptr, key, iv)) {
			EVP_CIPHER_CTX_free(mCtx);
			throw EncryptionFailedException("Failed to initialize encryption.");
		}
		
		// write the iv
		stream.write(iv, sizeof(iv));
	}
	
	EncryptedOutputStream::~EncryptedOutputStream()
	{
		EVP_CIPHER_CTX_free(mCtx);
	}
	
	void EncryptedOutputStream::write(const void *data, size_t size)
	{
		ZeroedArray<uint8_t, 4096> buffer;
		uint8_t encBuffer[4096 + EVP_MAX_BLOCK_LENGTH];
		int outLen;
		
		int bytesWritten = 0;
		
		const uint8_t *pData = (const uint8_t *)data;
		while(size > 0) {
			int bufSize = size > buffer.size() ? buffer.size() : size;
			memcpy(buffer.data(), pData, bufSize);
			pData += bufSize;
			size -= bufSize;

			if(!EVP_EncryptUpdate(mCtx, encBuffer, &outLen, buffer.data(), bufSize)) {
				throw EncryptionFailedException("Failed to encrypt data.");
			}
			
			mStream.write(encBuffer, outLen);
			bytesWritten += outLen;
		}
	}
	
	void EncryptedOutputStream::flush()
	{
		mStream.flush();
	}
	
	void EncryptedOutputStream::close()
	{
		ZeroedArray<uint8_t, EVP_MAX_BLOCK_LENGTH> padBuffer;
		int outLen;
		if(!EVP_EncryptFinal_ex(mCtx, padBuffer.data(), &outLen)) {
			throw EncryptionFailedException("Failed to write padding.");
		}
		
		mStream.write(padBuffer.data(), outLen);
		mStream.flush();
	}
}
