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
#include <openssl/hmac.h>
#include "Exception.h"
#include "ZeroedArray.h"
#include "ScopedExit.h"
#include "InputStream.h"
#include "OutputStream.h"
#include "Crypto.h"

namespace Nebula
{
	namespace Crypto
	{
		void encrypt(const uint8_t *key, const uint8_t *iv, InputStream& inputStream, OutputStream& outputStream)
		{
			EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
			if(!ctx) {
				throw EncryptionFailedException("Failed to create cipher.");
			}
			
			ScopedExit onExit([ctx] { EVP_CIPHER_CTX_free(ctx); });
			
			if(!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
				throw EncryptionFailedException("Failed to initialize encryption.");
			}
			
			ZeroedArray<uint8_t, 4096> buffer;
			ZeroedArray<uint8_t, 4096 + 32> outputBuffer;
			size_t n;
			int encryptedLen;
			while((n = inputStream.read(buffer.data(), buffer.size())) > 0) {
				if(!EVP_EncryptUpdate(ctx, outputBuffer.data(), &encryptedLen, buffer.data(), n)) {
					throw EncryptionFailedException("Failed to encrypt key.");
				}
				
				outputStream.write(outputBuffer.data(), encryptedLen);
			}
			
			if(!EVP_EncryptFinal_ex(ctx, outputBuffer.data(), &encryptedLen)) {
				throw EncryptionFailedException("Failed to encrypt key.");
			}

			outputStream.write(outputBuffer.data(), encryptedLen);
		}
		
		void decrypt(const uint8_t *key, const uint8_t *iv, InputStream& inputStream, OutputStream& outputStream)
		{
			EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
			if(!ctx) {
				throw EncryptionFailedException("Failed to create cipher.");
			}
			
			ScopedExit onExit([ctx] { EVP_CIPHER_CTX_free(ctx); });
			
			if(!EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv)) {
				throw EncryptionFailedException("Failed to initialize encryption.");
			}
			
			ZeroedArray<uint8_t, 4096> buffer;
			ZeroedArray<uint8_t, 4096 + 32> outputBuffer;
			size_t n;
			int encryptedLen;
			while((n = inputStream.read(buffer.data(), buffer.size())) > 0) {
				if(!EVP_DecryptUpdate(ctx, outputBuffer.data(), &encryptedLen, buffer.data(), n)) {
					throw EncryptionFailedException("Failed to encrypt key.");
				}
				
				outputStream.write(outputBuffer.data(), encryptedLen);			}
			
			if(!EVP_DecryptFinal_ex(ctx, outputBuffer.data(), &encryptedLen)) {
				throw EncryptionFailedException("Failed to encrypt key.");
			}
			
			outputStream.write(outputBuffer.data(), encryptedLen);
		}
		
		void hmac(const uint8_t *key, InputStream& inputStream, uint8_t *signature)
		{
			HMAC_CTX ctx;
			
			ScopedExit onExit([&ctx] { HMAC_CTX_cleanup(&ctx); });
			
			if(!HMAC_Init_ex(&ctx, key, 32, EVP_sha256(), nullptr)) {
				throw EncryptionFailedException("HMAC_Init_ex failed.");
			}
			
			size_t n;
			ZeroedArray<uint8_t, 4096> buffer;
			while((n = inputStream.read(buffer.data(), buffer.size())) > 0)
			{
				if(!HMAC_Update(&ctx, buffer.data(), n))
				{
					throw EncryptionFailedException("HMAC_update failed.");
				}
			}
			
			unsigned int mdLen;
			if(!HMAC_Final(&ctx, signature, &mdLen)) {
				throw EncryptionFailedException("HMAC_Final failed.");
			}
		}
	}
}
