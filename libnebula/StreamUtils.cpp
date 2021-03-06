/*
 * Copyright (c) 2017 Sound <sound@sagaforce.com>
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
#include "StreamUtils.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <memory>
#include "MultiInputStream.h"
#include "LZMAInputStream.h"
#include "Exception.h"
#include "TempFileStream.h"
#include "CompressionType.h"
#include "EncryptedOutputStream.h"
#include "DecryptedInputStream.h"
#include "MemoryInputStream.h"

namespace Nebula
{
	class EncryptedHMACStream : public InputStream
	{
	public:
		EncryptedHMACStream(const uint8_t *hmac, std::shared_ptr<TempFileStream> tmpStream, std::shared_ptr<InputStream> encStream)
		: mHMACStream(mHMAC, sizeof(mHMAC))
		, mTmpStream(tmpStream)
		, mEncStream(encStream)
		, mInputStream{&mHMACStream, mEncStream.get()}
		{
			memcpy(mHMAC, hmac, sizeof(mHMAC));
		}
	private:
		uint8_t mHMAC[SHA256_DIGEST_LENGTH];
		MemoryInputStream mHMACStream;
		std::shared_ptr<TempFileStream> mTmpStream;
		std::shared_ptr<InputStream> mEncStream;
		MultiInputStream mInputStream;
		
		virtual size_t read(void *data, size_t size) override;
		virtual long size() const override;
		
		virtual bool canRewind() const override;
		virtual void rewind() override;
	};
	
	size_t EncryptedHMACStream::read(void *data, size_t size)
	{
		return mInputStream.read(data, size);
	}
	
	long EncryptedHMACStream::size() const
	{
		return mInputStream.size();
	}
	
	bool EncryptedHMACStream::canRewind() const
	{
		return mInputStream.canRewind();
	}
	
	void EncryptedHMACStream::rewind()
	{
		mInputStream.rewind();
	}
	
	std::shared_ptr<InputStream> StreamUtils::compressEncryptHMAC(CompressionType compressType, const EVP_CIPHER *cipher, const uint8_t *encKey, const uint8_t *macKey, InputStream& inStream)
	{
		std::shared_ptr<TempFileStream> tmpStream = std::make_shared<TempFileStream>();
		EncryptedOutputStream encStream(*tmpStream, cipher, encKey);
		switch(compressType) {
			case CompressionType::NoCompression:
				inStream.copyTo(encStream);
				break;
			case CompressionType::LZMA2:
				LZMAUtils::compress(inStream, encStream, nullptr);
				break;
			default:
				throw InvalidArgumentException("Invalid compression type.");
		}
		
		encStream.close();

		auto encryptedStream = tmpStream->inputStream();
		
		// take the HMAC of the encrypted stream
		HMAC_CTX hctx;
		HMAC_CTX_init(&hctx);
		std::unique_ptr<HMAC_CTX, decltype(HMAC_CTX_cleanup) *>
			onExit(&hctx, HMAC_CTX_cleanup);
		
		if(!HMAC_Init(&hctx, macKey, SHA256_DIGEST_LENGTH, EVP_sha256())) {
			throw EncryptionFailedException("Failed to initialize HMAC.");
		}
		
		uint8_t buffer[8192];
		size_t n;
		while((n = encryptedStream->read(buffer, sizeof(buffer))) > 0) {
			if(!HMAC_Update(&hctx, buffer, n)) {
				throw EncryptionFailedException("Failed to update HMAC.");
			}
		}
		
		uint8_t hmac[SHA256_DIGEST_LENGTH];
		if(!HMAC_Final(&hctx, hmac, nullptr)) {
			throw EncryptionFailedException("Failed to compute HMAC.");
		}
		
		encryptedStream->rewind();
		
		return std::make_shared<EncryptedHMACStream>(hmac, tmpStream, encryptedStream);
	}

	void StreamUtils::decompressDecryptHMAC(CompressionType compressType, const EVP_CIPHER *cipher, const uint8_t *encKey, const uint8_t *macKey, InputStream& inStream, OutputStream& outStream)
	{
		if(!inStream.canRewind()) {
			throw InvalidArgumentException("Input stream must be rewindable.");
		}

		uint8_t hmac1[SHA256_DIGEST_LENGTH];
		
		inStream.readExpected(hmac1, sizeof(hmac1));
		
		HMAC_CTX hctx;
		HMAC_CTX_init(&hctx);
		std::unique_ptr<HMAC_CTX, decltype(HMAC_CTX_cleanup) *>
			onExit(&hctx, HMAC_CTX_cleanup);
		
		if(!HMAC_Init(&hctx, macKey, SHA256_DIGEST_LENGTH, EVP_sha256())) {
			throw EncryptionFailedException("Failed to initialize HMAC.");
		}
		
		uint8_t buffer[8192];
		size_t n;
		while((n = inStream.read(buffer, sizeof(buffer))) > 0) {
			if(!HMAC_Update(&hctx, buffer, n)) {
				throw EncryptionFailedException("Failed to update HMAC.");
			}
		}
		
		uint8_t hmac2[SHA256_DIGEST_LENGTH];
		if(!HMAC_Final(&hctx, hmac2, nullptr)) {
			throw EncryptionFailedException("Failed to compute HMAC.");
		}
		
		// verify hmac
		if(memcmp(hmac1, hmac2, sizeof(hmac1)) != 0) {
			throw VerificationFailedException("Failed to verify HMAC.");
		}
		
		inStream.rewind();
		inStream.readExpected(hmac1, sizeof(hmac1));

		switch(compressType) {
			case CompressionType::NoCompression:
			{
				DecryptedInputStream decStream(inStream, EVP_aes_256_cbc(), encKey);
				decStream.copyTo(outStream);
			}
				break;
			case CompressionType::LZMA2:
			{
				DecryptedInputStream decStream(inStream, EVP_aes_256_cbc(), encKey);
				LZMAInputStream lzStream(decStream);
				lzStream.copyTo(outStream);
			}
				break;
			default:
				throw InvalidArgumentException("Invalid compression type.");
		}
	}
}
