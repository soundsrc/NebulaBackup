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
#include "Repository.h"
#include <stdlib.h>
#include <math.h>
#include <string>
#include <boost/filesystem.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
extern "C" {
#include "compat/stdlib.h"
#include "compat/string.h"
}
#include "FileInfo.h"
#include "RollingHash.h"
#include "EncryptedOutputStream.h"
#include "DecryptedInputStream.h"
#include "LZMAInputStream.h"
#include "ZeroedArray.h"
#include "Exception.h"
#include "DataStore.h"
#include "BufferedInputStream.h"
#include "MultiInputStream.h"
#include "TempFileStream.h"
#include "FileStream.h"
#include "MemoryOutputStream.h"
#include "MemoryInputStream.h"
#include "LZMAUtils.h"
#include "CompressionType.h"
#include "StreamUtils.h"

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
	
	void Repository::writeRepositoryKey(const char *password, uint8_t logRounds, ProgressFunction progress)
	{
		// derive key from password
		ZeroedArray<uint8_t, EVP_MAX_MD_SIZE> derivedKey;
		
		if(logRounds > MAX_LOG_ROUNDS) {
			throw InvalidArgumentException("Specified number of rounds too big.");
		}

		uint8_t salt[PKCS5_SALT_LEN];
		arc4random_buf(salt, sizeof(salt));
		if(!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), 1 << logRounds, EVP_sha512(), derivedKey.size(), derivedKey.data())) {
			throw EncryptionFailedException("Failed to set password.");
		}
		
		// encrypt the encryption key with the derived key
		TempFileStream keyStream;
		
		keyStream.write("NEBULABACKUP", 12);
		
		keyStream.writeType<uint32_t>(0x00010000);
		keyStream.writeType<uint32_t>(0);
		keyStream.writeType<uint32_t>(1 << logRounds);
		
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
		
		mDataStore->put("/key", *keyStream.inputStream(), progress);
	}

	void Repository::initializeRepository(const char *password, uint8_t logRounds, ProgressFunction progress)
	{
		// random generate the encryption key
		arc4random_buf(mEncKey, EVP_MAX_KEY_LENGTH);
		arc4random_buf(mMacKey, EVP_MAX_KEY_LENGTH);
		arc4random_buf(mHashKey, EVP_MAX_KEY_LENGTH);
		arc4random_buf(mRollKey, EVP_MAX_KEY_LENGTH);
		
		writeRepositoryKey(password, logRounds, progress);
	}
	
	bool Repository::unlockRepository(const char *password, ProgressFunction progress)
	{
		TempFileStream keyStream;
		
		mDataStore->get("/key", keyStream, progress);

		auto stream = keyStream.inputStream();

		char magic[12];
		stream->readExpected(magic, sizeof(magic));
		if(strncmp(magic, "NEBULABACKUP", 12) != 0) {
			throw InvalidRepositoryException("Back up stream is invalid.");
		}

		uint32_t version = stream->readType<uint32_t>();
		if(version != 0x00010000) {
			throw InvalidRepositoryException("Invalid version number.");
		}
		
		uint32_t algorithm = stream->readType<uint32_t>();
		if(algorithm != 0) {
			throw InvalidRepositoryException("Invalid algorithm.");
		}
		
		uint32_t rounds = stream->readType<uint32_t>();
		if(rounds > (1 << MAX_LOG_ROUNDS)) {
			throw InvalidRepositoryException("Invalid number of rounds.");
		}

		uint8_t salt[PKCS5_SALT_LEN];
		stream->read(salt, sizeof(salt));
		ZeroedArray<uint8_t, EVP_MAX_KEY_LENGTH> derivedKey;
		if(!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, sizeof(salt), rounds, EVP_sha512(), derivedKey.size(), derivedKey.data())) {
			throw EncryptionFailedException("Failed to derive key from password.");
		}

		uint8_t encKeyData[EVP_MAX_KEY_LENGTH * 4 + EVP_MAX_BLOCK_LENGTH];
		uint8_t hmac1[EVP_MAX_MD_SIZE];
		uint8_t hmac2[EVP_MAX_MD_SIZE];
		stream->read(encKeyData, sizeof(encKeyData));
		stream->read(hmac1, sizeof(hmac1));
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
	
	bool Repository::changePassword(const char *oldPassword, const char *newPassword, uint8_t logRounds, ProgressFunction progress)
	{
		if(!unlockRepository(oldPassword)) return false;
		writeRepositoryKey(newPassword, logRounds, progress);
		return true;
	}
	
	void Repository::listSnapshots(const std::function<void (const char *)>& callback, ProgressFunction progress)
	{
		mDataStore->list("/snapshot", [&callback](const char *snapshot, void *) { callback(snapshot); }, nullptr, progress);
	}
	
	Snapshot *Repository::createSnapshot()
	{
		return new Snapshot();
	}
	
	Snapshot *Repository::loadSnapshot(const char *name, ProgressFunction progress)
	{
		std::unique_ptr<Snapshot> snapshot(new Snapshot());
		
		TempFileStream tmpSnapshotStream;
		mDataStore->get((std::string("/snapshot/") + name).c_str(), tmpSnapshotStream, progress);
		
		TempFileStream snapshotStream;
		StreamUtils::decompressDecryptHMAC(CompressionType::LZMA2, EVP_aes_256_cbc(), mEncKey, mMacKey, *tmpSnapshotStream.inputStream(), snapshotStream);
		snapshot->load(*snapshotStream.inputStream());
		
		return snapshot.release();
	}
	
	void Repository::commitSnapshot(Snapshot *snapshot, const char *name, ProgressFunction progress)
	{
		TempFileStream tmpStream;
		snapshot->save(tmpStream);
		auto snapshotStream = StreamUtils::compressEncryptHMAC(CompressionType::LZMA2, EVP_aes_256_cbc(), mEncKey, mMacKey, *tmpStream.inputStream());
		mDataStore->put((std::string("/snapshot/") + name).c_str(), *snapshotStream, progress);
	}
	
	void Repository::compressEncryptAndUploadBlock(CompressionType compressType, const uint8_t *block, size_t size, uint8_t *outhmac, ProgressFunction progress)
	{
		// take the plain text HMAC and that is the block file name
		if(!HMAC(EVP_sha256(), mHashKey, SHA256_DIGEST_LENGTH, block, size, outhmac, nullptr)) {
			throw EncryptionFailedException("Failed to HMAC block.");
		}
		
		// if the block already exists in the repository, skip the upload
		std::string uploadPath = "/data/" + hmac256ToString(outhmac);
		if(mDataStore->exist(uploadPath.c_str())) {
			return;
		}
		
		MemoryInputStream blockStream(block, size);
		auto encryptedStream = StreamUtils::compressEncryptHMAC(compressType, EVP_aes_256_cbc(), mEncKey, mMacKey, blockStream);
		
		mDataStore->put(uploadPath.c_str(), *encryptedStream, progress);
	}

	void Repository::uploadFile(Snapshot *snapshot, const char *destPath, FileStream& fileStream, ProgressFunction progress)
	{
		using namespace boost;
		
		CompressionType compressionType = CompressionType::LZMA2;

		RollingHash rh(mRollKey, 8192);
		uint8_t fileSHA256[SHA256_DIGEST_LENGTH];
	
		FileInfo fileInfo( fileStream.path().c_str() );
		if(!fileInfo.exists()) {
			throw FileNotFoundException("File not found.");
		}

		std::vector<Snapshot::BlockHash> blockHashes;
		blockHashes.reserve(32);
		
		SHA256_CTX sha256;
		if(!SHA256_Init(&sha256)) {
			throw EncryptionFailedException("SHA256_Init failed.");
		}
		
		size_t fileLength = fileInfo.length();
		int blockSizeLog = 0;

		// don't bother with splitting the file if it's < 1MB
		// just upload as is
		if(fileLength < 1024 * 1024) {
			
			std::unique_ptr<uint8_t, MallocDeletor> buffer( (uint8_t *)malloc(fileLength) );

			if(fileStream.read(buffer.get(), fileLength) != fileLength) {
				throw FileIOException("Failed to read file.");
			}
			
			if(!SHA256_Update(&sha256, buffer.get(), fileLength)) {
				throw EncryptionFailedException("SHA256_Update failed.");
			}
			
			Snapshot::BlockHash blockHash;
			compressEncryptAndUploadBlock(compressionType, buffer.get(), fileLength, blockHash.hmac256, progress);
			blockHashes.push_back(blockHash);
			
		} else {
			
			// the block size is dynamically determined based on file length
			// and increases bigger for larger file sizes
			// TODO: make these values configurable
			blockSizeLog = ceil(log(fileLength / 8) / log(2));
			if(blockSizeLog  < 16) blockSizeLog  = 16;
			if(blockSizeLog  > 25) blockSizeLog  = 25;
			uint64_t hashMask = (1 << blockSizeLog) - 1;
			
			std::vector<uint8_t> blockBuffer;
			blockBuffer.reserve(1 << (1 + blockSizeLog));
			
			BufferedInputStream bufferedFile(fileStream);
			while(!bufferedFile.isEof()) {
				uint8_t b = bufferedFile.readByte();
				blockBuffer.push_back(b);
				if((rh.roll(b) & hashMask) == 0) {
					
					if(!SHA256_Update(&sha256, &blockBuffer[0], blockBuffer.size())) {
						throw EncryptionFailedException("SHA256_Update failed.");
					}
					
					Snapshot::BlockHash blockHash;
					// upload block
					compressEncryptAndUploadBlock(compressionType, &blockBuffer[0], blockBuffer.size(), blockHash.hmac256, progress);
					blockHashes.push_back(blockHash);
					blockBuffer.clear();
				}
			}
			
			if(!blockBuffer.empty()) {
				Snapshot::BlockHash blockHash;
				if(!SHA256_Update(&sha256, &blockBuffer[0], blockBuffer.size())) {
					throw EncryptionFailedException("SHA256_Update failed.");
				}
				compressEncryptAndUploadBlock(compressionType, &blockBuffer[0], blockBuffer.size(), blockHash.hmac256, progress);
				blockHashes.push_back(blockHash);
			}
		}
		
		// write the SHA256
		if(!SHA256_Final(fileSHA256, &sha256)) {
			throw EncryptionFailedException("SHA256_Final failed.");
		}
		
		// update the index
		snapshot->addFileEntry(destPath,
							   fileInfo.userName().c_str(),
							   fileInfo.groupName().c_str(),
							   fileInfo.type(),
							   fileInfo.mode(),
							   compressionType,
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

			// download the object to tmpFile
			TempFileStream tmpStream;
			mDataStore->get(objectPath.c_str(), tmpStream);

			StreamUtils::decompressDecryptHMAC((CompressionType)fe->compression, EVP_aes_256_cbc(), mEncKey, mMacKey, *tmpStream.inputStream(), fileStream);
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
