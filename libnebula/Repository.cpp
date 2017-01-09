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
#include <string>
#include <stdlib.h>
#include <math.h>
#include <boost/filesystem.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
extern "C" {
#include "compat/stdlib.h"
#include "compat/string.h"
}
#include "ScopedExit.h"
#include "FileInfo.h"
#include "RollingHash.h"
#include "EncryptedOutputStream.h"
#include "DecryptedInputStream.h"
#include "LZMAInputStream.h"
#include "ZeroedArray.h"
#include "Exception.h"
#include "DataStore.h"
#include "BufferedInputStream.h"
#include "FileStream.h"
#include "MemoryOutputStream.h"
#include "MemoryInputStream.h"
#include "LZMAUtils.h"
#include "Repository.h"

namespace Nebula
{
	Repository::Repository(DataStore *dataStore)
	: mDataStore(dataStore)
	{
		mEncKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		mMacKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		mHashKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		mRollKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
	}
	
	Repository::~Repository()
	{
		explicit_bzero(mEncKey, EVP_MAX_KEY_LENGTH);
		explicit_bzero(mMacKey, EVP_MAX_KEY_LENGTH);
		explicit_bzero(mHashKey, EVP_MAX_KEY_LENGTH);
		explicit_bzero(mRollKey, EVP_MAX_KEY_LENGTH);
		free(mMacKey);
		free(mEncKey);
		free(mHashKey);
		free(mRollKey);
	}
	
	struct Repository::MallocDeletor
	{
		inline void operator ()(uint8_t *p) {
			free(p);
		}
	};
	
	void Repository::initializeRepository(const char *password, int rounds)
	{
		// derive key from password
		ZeroedArray<uint8_t, EVP_MAX_MD_SIZE> derivedKey;
		
		uint8_t salt[PKCS5_SALT_LEN];
		arc4random_buf(salt, sizeof(salt));
		if(!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), rounds, EVP_sha512(), derivedKey.size(), derivedKey.data())) {
			throw EncryptionFailedException("Failed to set password.");
		}
		
		// random generate the encryption key
		arc4random_buf(mEncKey, EVP_MAX_KEY_LENGTH);
		arc4random_buf(mMacKey, EVP_MAX_KEY_LENGTH);
		arc4random_buf(mHashKey, EVP_MAX_KEY_LENGTH);
		arc4random_buf(mRollKey, EVP_MAX_KEY_LENGTH);
		
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

		keyStream.write(salt, PKCS5_SALT_LEN);

		// encrypt key with the derived key
		uint8_t encBytes[EVP_MAX_KEY_LENGTH * 4 + EVP_MAX_BLOCK_LENGTH];
		MemoryOutputStream outStream(encBytes, sizeof(encBytes));
		EncryptedOutputStream encStream(outStream, EVP_aes_256_cbc(), derivedKey.data());
		encStream.write(mEncKey, EVP_MAX_KEY_LENGTH);
		encStream.write(mMacKey, EVP_MAX_KEY_LENGTH);
		encStream.write(mHashKey, EVP_MAX_KEY_LENGTH);
		encStream.write(mRollKey, EVP_MAX_KEY_LENGTH);
		encStream.close();
		outStream.close();
		
		keyStream.write(outStream.data(), outStream.size());

		uint8_t hmac[EVP_MAX_MD_SIZE];
		unsigned int hmacLen = sizeof(hmac);
		if(!HMAC(EVP_sha256(), derivedKey.data(), derivedKey.size(),
				 outStream.data(), outStream.size(), hmac, &hmacLen)) {
			throw EncryptionFailedException("Failed to HMAC encrypted block.");
		}
		keyStream.write(hmac, sizeof(hmac));
		keyStream.close();

		MemoryInputStream keyStreamResult(keyStream.data(), keyStream.size());
		mDataStore->put("/key", keyStreamResult);
	}
	
	bool Repository::unlockRepository(const char *password)
	{
		uint8_t keyData[1024];
		MemoryOutputStream keyStream(keyData, sizeof(keyData));
		
		if(!mDataStore->get("/key", keyStream)) {
			throw InvalidDataException("Repository does not exist at this path.");
		}

		char magic[12];
		MemoryInputStream stream(keyStream.data(), keyStream.size());
		stream.read(magic, sizeof(magic));
		if(strncmp(magic, "NEBULABACKUP", 12) != 0) {
			throw InvalidDataException("Back up stream is invalid.");
		}

		uint32_t version = 0;
		stream.read(&version, sizeof(version));
		if(version != 0x00010000) {
			throw InvalidDataException("Invalid version number.");	
		}
		
		uint32_t algorithm;
		stream.read(&algorithm, sizeof(algorithm)); // algorithm
		if(algorithm != 0) {
			throw InvalidDataException("Invalid algorithm.");
			
		}
		
		uint32_t rounds;
		stream.read(&rounds, sizeof(rounds));
		if(rounds > 100000) {
			throw InvalidDataException("Invalid number of rounds.");
		}
		
		uint8_t salt[PKCS5_SALT_LEN];
		stream.read(salt, sizeof(salt));
		ZeroedArray<uint8_t, EVP_MAX_KEY_LENGTH> derivedKey;
		if(!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), rounds, EVP_sha512(), derivedKey.size(), derivedKey.data())) {
			throw EncryptionFailedException("Failed to derive key from password.");
		}

		uint8_t encKeyData[EVP_MAX_KEY_LENGTH * 4 + EVP_MAX_BLOCK_LENGTH];
		uint8_t hmac1[EVP_MAX_MD_SIZE];
		uint8_t hmac2[EVP_MAX_MD_SIZE];
		stream.read(encKeyData, sizeof(encKeyData));
		stream.read(hmac1, sizeof(hmac1));
		unsigned int hmacKeyLen = sizeof(hmac2);
		
		if(!HMAC(EVP_sha256(), derivedKey.data(), derivedKey.size(), encKeyData, sizeof(encKeyData), hmac2, &hmacKeyLen)) {
			throw EncryptionFailedException("Failed to unlock using password. HMAC failure.");
		}

		// password failure
		if(memcmp(hmac1, hmac2, hmacKeyLen) != 0) {
			return false;
		}

		MemoryInputStream encKeyStream(encKeyData, sizeof(encKeyData));

		DecryptedInputStream decStream(encKeyStream, EVP_aes_256_cbc(), derivedKey.data());
		decStream.read(mEncKey, EVP_MAX_KEY_LENGTH);
		decStream.read(mMacKey, EVP_MAX_KEY_LENGTH);
		decStream.read(mHashKey, EVP_MAX_KEY_LENGTH);
		decStream.read(mRollKey, EVP_MAX_KEY_LENGTH);

		return true;
	}
	
	
	Snapshot *Repository::createSnapshot()
	{
		return new Snapshot();
	}
	
	void Repository::compressEncryptAndUploadBlock(const uint8_t *block, size_t size, uint8_t *outhmac, ProgressFunction progress)
	{
		if(!HMAC(EVP_sha256(), mHashKey, SHA256_DIGEST_LENGTH, block, size, outhmac, nullptr)) {
			throw EncryptionFailedException("Failed to HMAC block.");
		}
		
		// if the block already exists in the repository, skip the upload
		std::string uploadPath = "/data/" + hmac256ToString(outhmac);
		if(mDataStore->exist(uploadPath.c_str())) {
			return;
		}
		
		MemoryInputStream blockStream(block, size);
		
		std::unique_ptr<uint8_t, MallocDeletor> compressData( (uint8_t *)malloc(size * 2) );
		
		MemoryOutputStream compressedStream(compressData.get(), size * 2);
		LZMAUtils::compress(blockStream, compressedStream, nullptr);
		compressedStream.close();
		
		size_t maxEncryptedLength = EVP_MAX_IV_LENGTH + compressedStream.size() + EVP_MAX_BLOCK_LENGTH;
		std::unique_ptr<uint8_t, MallocDeletor> encryptedData( (uint8_t *)malloc(SHA256_DIGEST_LENGTH + maxEncryptedLength) );
		MemoryInputStream encInStream(compressedStream.data(), compressedStream.size());
		MemoryOutputStream encOutStreamMem(encryptedData.get() + SHA256_DIGEST_LENGTH, maxEncryptedLength);
		EncryptedOutputStream encOutStream(encOutStreamMem, EVP_aes_256_cbc(), mEncKey);
		encInStream.copyTo(encOutStream);
		encOutStream.close();

		if(!HMAC(EVP_sha256(), mMacKey, SHA256_DIGEST_LENGTH, encOutStreamMem.data(), encOutStreamMem.size(), encryptedData.get(), nullptr)) {
			throw EncryptionFailedException("Failed to HMAC encrypted block.");
		}
		
		MemoryInputStream uploadStream(encryptedData.get(), SHA256_DIGEST_LENGTH + encOutStreamMem.size());
		mDataStore->put(uploadPath.c_str(), uploadStream);
	}

	void Repository::uploadFile(Snapshot *snapshot, const char *destPath, FileStream& fileStream, ProgressFunction progress)
	{
		using namespace boost;
		
		RollingHash rh(mRollKey, 8192);
		uint8_t fileSHA256[SHA256_DIGEST_LENGTH];
	
		FileInfo fileInfo( fileStream.path().c_str() );
		if(!fileInfo.exists()) {
			throw FileNotFoundException("File not found.");
		}

		std::vector<Snapshot::BlockHash> blockHashes;
		blockHashes.reserve(32);
		
		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		
		size_t fileLength = fileInfo.length();
		int blockSizeLog = 0;

		// don't bother with splitting the file if it's < 1MB
		// just upload as is
		if(fileLength < 1024 * 1024) {
			
			std::unique_ptr<uint8_t, MallocDeletor> buffer( (uint8_t *)malloc(fileLength) );

			if(fileStream.read(buffer.get(), fileLength) != fileLength) {
				throw FileIOException("Failed to read file.");
			}
			
			SHA256_Update(&sha256, buffer.get(), fileLength);
			
			Snapshot::BlockHash blockHash;
			compressEncryptAndUploadBlock(buffer.get(), fileLength, blockHash.hmac256, progress);
			blockHashes.push_back(blockHash);
			
		} else {
			
			// the block size is dynamically determined based on file length
			// and increases bigger for larger file sizes
			// TODO: make these values configurable
			blockSizeLog = ceil(log(fileLength / 8) / log(2));
			if(blockSizeLog  < 16) blockSizeLog  = 16;
			if(blockSizeLog  > 25) blockSizeLog  = 25;
			uint32_t hashMask = (1 << blockSizeLog) - 1;
			
			std::vector<uint8_t> blockBuffer;
			blockBuffer.reserve(1 << (1 + blockSizeLog));
			
			BufferedInputStream bufferedFile(fileStream);
			while(!bufferedFile.isEof()) {
				uint8_t b = bufferedFile.readByte();
				blockBuffer.push_back(b);
				if((rh.roll(b) & hashMask) == 0) {
					
					SHA256_Update(&sha256, &blockBuffer[0], blockBuffer.size());
					
					Snapshot::BlockHash blockHash;
					// upload block
					compressEncryptAndUploadBlock(&blockBuffer[0], blockBuffer.size(), blockHash.hmac256, progress);
					blockHashes.push_back(blockHash);
					blockBuffer.clear();
				}
			}
			
			if(!blockBuffer.empty()) {
				Snapshot::BlockHash blockHash;
				SHA256_Update(&sha256, &blockBuffer[0], blockBuffer.size());
				compressEncryptAndUploadBlock(&blockBuffer[0], blockBuffer.size(), blockHash.hmac256, progress);
				blockHashes.push_back(blockHash);
			}
		}
		
		// write the SHA256
		SHA256_Final(fileSHA256, &sha256);
		
		// update the index
		snapshot->addFileEntry(destPath,
							   fileInfo.userName().c_str(),
							   fileInfo.groupName().c_str(),
							   fileInfo.type(),
							   fileInfo.mode(),
							   fileInfo.length(),
							   fileInfo.lastModifyTime(),
							   blockSizeLog,
							   fileSHA256,
							   blockHashes.size(),
							   &blockHashes[0]);
	}
	
	bool Repository::downloadFile(Snapshot *snapshot, const char *srcPath, OutputStream& fileStream, ProgressFunction progress)
	{
		using namespace boost;

		const Snapshot::FileEntry *fe = snapshot->getFileEntry(srcPath);
		if(!fe) {
			return false;
		}
		
		const Snapshot::BlockHash *blockHashes = snapshot->indexToBlockHash(fe->blockIndex);
		for(int i = 0; i < fe->numBlocks; ++i) {
			std::string objectPath = "/data/" + hmac256ToString(blockHashes[i].hmac256);
			
			filesystem::path tmpPath = filesystem::unique_path();
			scopedExit([tmpPath] { filesystem::remove(tmpPath); });

			// download the object to tmpFile
			FileStream tmpStream(tmpPath.c_str(), FileMode::Write);
			mDataStore->get(objectPath.c_str(), tmpStream);
			tmpStream.close();

			// read the HMAC
			FileStream tmpInStream(tmpPath.c_str(), FileMode::Read);
			uint8_t hmac[SHA256_DIGEST_LENGTH];
			if(tmpInStream.read(hmac, sizeof(hmac)) != sizeof(hmac)) {
				throw FileIOException("Failed to decrypt file.");
			}
			
			uint8_t buffer[8192];
			size_t n;

			// first pass: verify HMAC
			{
				HMAC_CTX hctx;
				HMAC_CTX_init(&hctx);
				scopedExit([&hctx] { HMAC_CTX_cleanup(&hctx); });

				if(!HMAC_Init(&hctx, mMacKey, SHA256_DIGEST_LENGTH, EVP_sha256())) {
					throw VerificationFailedException("Failed to verify data block.");
				}
				
				while((n = tmpInStream.read(buffer, sizeof(buffer))) > 0) {
					if(!HMAC_Update(&hctx, buffer, n)) {
						throw VerificationFailedException("Failed to verify data block.");
					}
				}
				
				uint8_t fileHmac[SHA256_DIGEST_LENGTH];
				if(!HMAC_Final(&hctx, fileHmac, nullptr)) {
					throw VerificationFailedException("Failed to verify data block.");
				}
				
				if(memcmp(fileHmac, hmac, SHA256_DIGEST_LENGTH) != 0) {
					throw VerificationFailedException("Failed to verify data block.");
				}
				
				tmpInStream.seek(SHA256_DIGEST_LENGTH);
			}
			
			// sceond pass: decrypt and uncompress
			DecryptedInputStream decStream(tmpInStream, EVP_aes_256_cbc(), mEncKey);
			LZMAInputStream lzStream(decStream);

			while((n = lzStream.read(buffer, sizeof(buffer))) > 0) {
				fileStream.write(buffer, n);
			}
		}
		
		return true;
	}

	char Repository::nibbleToHex(uint8_t nb) const
	{
		if(nb >= 0 && nb <= 9) return '0' + nb;
		if(nb < 16) {
			return 'a' + (nb - 10);
		}
		return 0;
	}
	
	int Repository::hexToNibble(char h) const
	{
		char lh = tolower(h);
		if(lh >= '0' && lh <= '9') return lh - '0';
		if(lh >= 'a' && lh <= 'a') return lh - 'a' + 10;
		return 0;
	}
	
	std::string Repository::hmac256ToString(const uint8_t *hmac) const
	{
		std::string hmacStr;
		hmacStr.reserve(64);
		for(int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
			hmacStr += nibbleToHex(hmac[i] >> 4);
			hmacStr += nibbleToHex(hmac[i] & 0xF);
		}
		
		return hmacStr;
	}
	
	void Repository::hmac256strToHmac(const char *str, uint8_t *outHmac)
	{
		for(int i = 0; i < SHA256_DIGEST_LENGTH * 2; i += 2) {
			outHmac[i >> 1] = hexToNibble(str[i]) << 4 | hexToNibble(str[i]);
		}
	}
}
