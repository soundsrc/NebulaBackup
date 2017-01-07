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
#include <memory>
#include <math.h>
#include <ctype.h>
#include <boost/filesystem.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include "Exception.h"
#include "BufferedInputStream.h"
#include "MemoryInputStream.h"
#include "MemoryOutputStream.h"
#include "FileStream.h"
#include "Repository.h"
#include "LZMAUtils.h"
#include "EncryptedOutputStream.h"
#include "ScopedExit.h"
#include "RollingHash.h"
#include "Snapshot.h"

namespace Nebula
{
	Snapshot::Snapshot(Repository& repo)
	: mRepository(repo)
	{
	}
	
	Snapshot::~Snapshot()
	{
	}
	
	char Snapshot::nibbleToHex(uint8_t nb) const
	{
		if(nb >= 0 && nb <= 9) return '0' + nb;
		if(nb < 16) {
			return 'a' + (nb - 10);
		}
		return 0;
	}
	
	int Snapshot::hexToNibble(char h) const
	{
		char lh = tolower(h);
		if(lh >= '0' && lh <= '9') return lh - '0';
		if(lh >= 'a' && lh <= 'a') return lh - 'a' + 10;
		return 0;
	}

	std::string Snapshot::hmac256ToString(uint8_t *hmac) const
	{
		std::string hmacStr;
		hmacStr.reserve(64);
		for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
			hmacStr += nibbleToHex(hmac[i] >> 4);
			hmacStr += nibbleToHex(hmac[i] & 0xF);
		}
		
		return hmacStr;
	}
	
	void Snapshot::hmac256strToHmac(const char *str, uint8_t *outHmac)
	{
		for(int i = 0; i < SHA256_DIGEST_LENGTH * 2; i += 2) {
			outHmac[i >> 1] = hexToNibble(str[i]) << 4 | hexToNibble(str[i]);
		}
	}
	
	struct Snapshot::MallocDeletor
	{
		inline void operator ()(uint8_t *p) {
			free(p);
		}
	};

	AsyncProgress<bool> Snapshot::uploadBlock(const uint8_t *block, size_t size)
	{
		auto progress = std::shared_ptr<AsyncProgressData<bool>>();
		
		uint8_t filemac[SHA256_DIGEST_LENGTH];
		if(!HMAC(EVP_sha256(), mRepository.hashKey(), SHA256_DIGEST_LENGTH, block, size, filemac, nullptr)) {
			throw EncryptionFailedException("Failed to HMAC block.");
		}
		
		MemoryInputStream blockStream(block, size);
		
		std::unique_ptr<uint8_t, MallocDeletor>
			compressData( (uint8_t *)malloc(size * 2) );

		MemoryOutputStream compressedStream(compressData.get(), size * 2);
		LZMAUtils::compress(blockStream, compressedStream, nullptr);
		compressedStream.close();
		
		{
			std::unique_ptr<uint8_t, MallocDeletor>
				encryptedData( (uint8_t *)malloc(compressedStream.size() + EVP_MAX_BLOCK_LENGTH) );
			MemoryInputStream encInStream(compressedStream.data(), compressedStream.size());
			MemoryOutputStream encOutStream(encryptedData.get(), compressedStream.size() + EVP_MAX_BLOCK_LENGTH);
			encInStream.copyTo(encOutStream);
		}
		
//		mRepository.dataStore()->put("/data/" + hmac256ToString(filemac))
		
		return AsyncProgress<bool>(progress);
	}

	AsyncProgress<bool> Snapshot::uploadFile(const char *path, FileStream& fileStream)
	{
		using namespace boost;

		RollingHash rh(mRepository.rollKey(), 8192);
		
		size_t fileSize = fileStream.length();

		// don't bother with splitting the file if it's < 1MB
		// just upload as is
		if(fileSize < 1024 * 1024) {
			
			std::unique_ptr<uint8_t, MallocDeletor>
				buffer( (uint8_t *)malloc(fileSize) );
			if(fileStream.read(buffer.get(), fileSize) != fileSize) {
				throw FileIOException("Failed to read file.");
			}

			return uploadBlock(buffer.get(), fileSize);
		}
	


		// determine how to split
		uint32_t blockSizeLog = ceil(log(fileSize / 8) / log(2));
		if(blockSizeLog < 16) blockSizeLog = 16;
		uint32_t hashMask = (1 << blockSizeLog) - 1;

		std::vector<uint8_t> blockBuffer;
		blockBuffer.reserve(1 << (1 + blockSizeLog));
		
		BufferedInputStream bufferedFile(fileStream);
		while(!bufferedFile.isEof()) {
			uint8_t b = bufferedFile.readByte();
			if((rh.roll(b) & hashMask) == 0) {
				blockBuffer.push_back(b);
				
				// upload block
				uploadBlock(&blockBuffer[0], blockBuffer.size());
				blockBuffer.clear();
			}
		}
	}
}
