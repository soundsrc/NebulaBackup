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

#include <openssl/evp.h>
#include "ZeroedArray.h"
#include "Exception.h"
#include "EncryptedOutputStream.h"

namespace Nebula
{
	EncryptedOutputStream::EncryptedOutputStream(OutputStream& stream, const EVP_CIPHER *cipher, const uint8_t *key, const uint8_t *iv)
	: mStream(stream)
	{
		mCtx = EVP_CIPHER_CTX_new();
		if(!mCtx) {
			throw EncryptionFailedException("Failed to create cipher.");
		}

		if(!EVP_EncryptInit_ex(mCtx, cipher, nullptr, key, iv)) {
			EVP_CIPHER_CTX_free(mCtx);
			throw EncryptionFailedException("Failed to initialize encryption.");
		}
	}
	
	EncryptedOutputStream::~EncryptedOutputStream()
	{
		EVP_CIPHER_CTX_free(mCtx);
	}
	
	int EncryptedOutputStream::write(const void *data, int size)
	{
		ZeroedArray<uint8_t, 4096> buffer;
		uint8_t encBuffer[4096 + PaddingExtra];
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
			
			int n = mStream.write(encBuffer, outLen);
			if(n >= 0) {
				bytesWritten += n;
			}
		}
		
		return bytesWritten;
	}
	
	void EncryptedOutputStream::flush()
	{
		mStream.flush();
	}
	
	void EncryptedOutputStream::close()
	{
		ZeroedArray<uint8_t, 128> padBuffer;
		int outLen;
		if(!EVP_EncryptFinal_ex(mCtx, padBuffer.data(), &outLen)) {
			throw EncryptionFailedException("Failed to write padding.");
		}
		
		mStream.write(padBuffer.data(), outLen);
		mStream.flush();
	}
}