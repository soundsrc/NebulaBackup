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
#include <set>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
extern "C" {
#include "compat/stdlib.h"
#include "compat/string.h"
#include "compat/resolv.h"
}
#include "Base32.h"
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
#include "Snapshot.h"
#include "InputRangeStream.h"
#include "StreamUtils.h"

namespace Nebula
{
	struct Repository::PackFileInfo
	{
		std::string path;
		std::string user;
		std::string group;
		FileType type;
		uint16_t mode;
		CompressionType compression;
		uint64_t size;
		time_t mtime;
		uint8_t rollingHashBits;
		uint8_t md5[MD5_DIGEST_LENGTH];
		uint32_t offset;
		uint32_t packLength;
		Snapshot::ObjectID objectId;
	};
	
	struct Repository::PackUploadState
	{
		HMAC_CTX hmac;
		std::vector<PackFileInfo> fileInfos;
		std::vector<std::vector<uint8_t>> packData;
		std::unique_ptr<RollingHash> rollingHash;

		PackUploadState();
		~PackUploadState();

		void addFile(const uint8_t *data,
					 size_t sizeData,
					 const std::string& path,
					 const std::string& user,
					 const std::string& group,
					 FileType type,
					 uint16_t mode,
					 CompressionType compression,
					 uint64_t size,
					 time_t mtime,
					 uint8_t rollingHashBits,
					 const uint8_t *md5);
	};
	
	Repository::PackUploadState::PackUploadState()
	{
		HMAC_CTX_init(&hmac);
	}
	
	Repository::PackUploadState::~PackUploadState()
	{
		HMAC_CTX_cleanup(&hmac);
	}
	
	void Repository::PackUploadState::addFile(const uint8_t *data,
								  size_t sizeData,
								  const std::string& path,
								  const std::string& user,
								  const std::string& group,
								  FileType type,
								  uint16_t mode,
								  CompressionType compression,
								  uint64_t size,
								  time_t mtime,
								  uint8_t rollingHashBits,
								  const uint8_t *md5)
	{
		if(!HMAC_Update(&hmac, data, sizeData)) {
			throw EncryptionFailedException("HMAC_Update failed.");
		}
		
		Repository::PackFileInfo pi;
		pi.path = path;
		pi.user = user;
		pi.group = group;
		pi.type = type;
		pi.mode = mode;
		pi.compression = compression;
		pi.size = size;
		pi.mtime = mtime;
		pi.rollingHashBits = rollingHashBits;
		memcpy(pi.md5, md5, MD5_DIGEST_LENGTH);
		pi.offset = packData.size();
		
		this->fileInfos.push_back(pi);
		std::vector<uint8_t> buffer;
		buffer.reserve(sizeData);
		std::copy(data, data + sizeData, std::back_inserter(buffer));
		this->packData.push_back(std::move(buffer));
	}
	
	Repository::Options::Options()
	: smallFileSize(512000)
	, maxBlockSize(50000000)
	, minRollingHashBits(18)
	, maxRollingHashBits(25)
	, blockSplitCount(8)
	{
	}
	
	Repository::Repository(DataStore *dataStore, Options *options)
	: mDataStore(dataStore)
	{
		mEncKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		mMacKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		mHashKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		mRollKey = (uint8_t *)malloc(EVP_MAX_KEY_LENGTH);
		
		if(options) {
			mOptions = *options;
		}
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
	
	std::shared_ptr<Snapshot> Repository::createSnapshot()
	{
		return std::make_shared<Snapshot>();
	}
	
	std::shared_ptr<Snapshot> Repository::loadSnapshot(const char *name, ProgressFunction progress)
	{
		std::shared_ptr<Snapshot> snapshot(std::make_shared<Snapshot>());
		
		TempFileStream tmpSnapshotStream;
		mDataStore->get((std::string("/snapshot/") + name).c_str(), tmpSnapshotStream, progress);
		
		TempFileStream snapshotStream;
		StreamUtils::decompressDecryptHMAC(CompressionType::LZMA2, EVP_aes_256_cbc(), mEncKey, mMacKey, *tmpSnapshotStream.inputStream(), snapshotStream);
		snapshot->load(*snapshotStream.inputStream());
		
		return snapshot;
	}
	
	void Repository::commitSnapshot(std::shared_ptr<Snapshot> snapshot, const char *name, ProgressFunction progress)
	{
		TempFileStream tmpStream;
		snapshot->save(tmpStream);
		auto snapshotStream = StreamUtils::compressEncryptHMAC(CompressionType::LZMA2, EVP_aes_256_cbc(), mEncKey, mMacKey, *tmpStream.inputStream());
		mDataStore->put((std::string("/snapshot/") + name).c_str(), *snapshotStream, progress);
	}
	
	void Repository::computeBlockHMAC(const uint8_t *block, size_t size, uint8_t compression, uint8_t *outHMAC)
	{
		HMAC_CTX ctx;
		std::unique_ptr<HMAC_CTX, decltype(HMAC_CTX_cleanup) *> onExit(&ctx, HMAC_CTX_cleanup);

		if(!HMAC_Init(&ctx, mHashKey, SHA256_DIGEST_LENGTH, EVP_sha256())) {
			throw EncryptionFailedException("HMAC_Init failed.");
		}
		
		// hmac which compression is used since that changes the block data
		if(!HMAC_Update(&ctx, &compression, sizeof(compression))) {
			throw EncryptionFailedException("HMAC_Update failed.");
		}
		
		if(!HMAC_Update(&ctx, block, size)) {
			throw EncryptionFailedException("HMAC_Update failed.");
		}
		
		if(!HMAC_Final(&ctx, outHMAC, nullptr)) {
			throw EncryptionFailedException("HMAC_Final failed.");
		}
	}
	
	void Repository::compressEncryptAndUploadBlock(CompressionType compressType, const Snapshot::ObjectID& objectId, const uint8_t *block, size_t size, ProgressFunction progress)
	{
		// if the block already exists in the repository, skip the upload
		std::string uploadPath = "/data/" + objectIdToString(objectId);
		if(mDataStore->exist(uploadPath.c_str())) {
			if(!progress(size, size)) {
				throw CancelledException("User cancelled.");
			}
			return;
		}
		
		MemoryInputStream blockStream(block, size);
		auto encryptedStream = StreamUtils::compressEncryptHMAC(compressType, EVP_aes_256_cbc(), mEncKey, mMacKey, blockStream);
		
		mDataStore->put(uploadPath.c_str(), *encryptedStream, progress);
	}
	
	std::shared_ptr<Repository::PackUploadState> Repository::createPackState()
	{
		return std::make_shared<PackUploadState>();
	}
	
	void Repository::uploadFile(std::shared_ptr<Snapshot> snapshot, const char *destPath, FileStream& fileStream, FileTransferProgressFunction progress)
	{
		uploadFile(nullptr, snapshot, destPath, fileStream);
	}

	void Repository::uploadFile(std::shared_ptr<PackUploadState> packUploadState, std::shared_ptr<Snapshot> snapshot, const char *destPath, FileStream& fileStream, FileTransferProgressFunction progress)
	{
		using namespace boost;
		
		CompressionType compressionType = CompressionType::LZMA2;

		RollingHash rh(mRollKey, 8192);
		uint8_t fileMD5[MD5_DIGEST_LENGTH];
	
		FileInfo fileInfo( fileStream.path().c_str() );
		if(!fileInfo.exists()) {
			throw FileNotFoundException("File not found.");
		}

		std::vector<Snapshot::ObjectID> objectIds;
		objectIds.reserve(32);
		
		EVP_MD_CTX md5;
		EVP_MD_CTX_init(&md5);
		std::unique_ptr<EVP_MD_CTX, decltype(EVP_MD_CTX_cleanup) *>
			cleanup(&md5, EVP_MD_CTX_cleanup);
		
		if(!EVP_DigestInit(&md5, EVP_md5())) {
			throw EncryptionFailedException("EVP_DigestInit failed.");
		}
		
		size_t fileLength = fileInfo.length();
		int rollingHashBits = 0;
		
		std::string ext = fileInfo.extension();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		// default extension exclusion list from compression
		// helps speed up things since these types generally don't compress well
		static std::set<std::string> noCompressExt = {
			".png", ".jpg", ".jpeg", ".gif",
			".mov",	".mkv",	".avi",	".mp2",	".mp3",	".mp4",
			".mpg",	".m4a",	".m4v",	".3gp",	".3gpp", ".3g2", ".3gpp2",
			".wma",	".wmv",
			".ogg",	".aac",
			".zip",	".gz", ".bz2", ".xz", ".lz", ".lzo", ".lzma",
			".z", ".7z", ".s7z", ".rar",
			".apk", ".ipa", ".jar"
		};
		
		if(noCompressExt.find(ext) != noCompressExt.end()) {
			compressionType = CompressionType::NoCompression;
		}

		// normalize destPath
		filesystem::path normalizedPath = filesystem::path(destPath).relative_path().lexically_normal();

		// don't bother with splitting the file if it's < 1MB
		// just upload as is
		if(fileLength < mOptions.smallFileSize) {
			
			std::unique_ptr<uint8_t, decltype(free) *> buffer( (uint8_t *)malloc(fileLength), free );

			fileStream.readExpected(buffer.get(), fileLength);
			
			if(!EVP_DigestUpdate(&md5, buffer.get(), fileLength)) {
				throw EncryptionFailedException("EVP_DigestUpdate failed.");
			}
			
			Snapshot::ObjectID objectId;
			computeBlockHMAC(buffer.get(), fileLength, (uint8_t)compressionType, objectId.id);
			
			
			if(packUploadState) {
				bool writePack = false;

				if(!packUploadState->rollingHash) {
					packUploadState->rollingHash.reset(new RollingHash(mRollKey, 8192));
					if(!HMAC_Init(&packUploadState->hmac, mHashKey, SHA256_DIGEST_LENGTH, EVP_sha256())) {
						throw EncryptionFailedException("HMAC_Init failed.");
					}
				}

				HMAC_Update(&packUploadState->hmac, buffer.get(), fileLength);
				const uint8_t *bufPtr = buffer.get();
				const uint8_t *bufEnd = bufPtr + fileLength;
				RollingHash& rollingHash = *packUploadState->rollingHash;
				while (bufPtr < bufEnd) {
					if((rollingHash.roll(*bufPtr++) & ((1 << 20) - 1)) == 0) {
						writePack = true;
					}
				}

				uint8_t md5[MD5_DIGEST_LENGTH];
				MD5(buffer.get(), fileLength, md5);

				packUploadState->addFile(buffer.get(),
										 fileLength,
										 normalizedPath.string(),
										 fileInfo.userName(),
										 fileInfo.groupName(),
										 fileInfo.type(),
										 fileInfo.mode(),
										 compressionType,
										 fileInfo.length(),
										 fileInfo.lastModifyTime(),
										 0,
										 md5);
				
				if(writePack) {
					finalizePack(snapshot, packUploadState, progress);
				} else {
					if(!progress(0, 1, 0, 0)) {
						throw CancelledException("User cancelled.");
					}
				}
				
				return;
			} else {
				compressEncryptAndUploadBlock(compressionType, objectId, buffer.get(), fileLength,
											  [&progress](long bytesUploaded, long bytesTotal) -> bool {
												  return progress(0, 1, bytesUploaded, bytesTotal);
											  });
				objectIds.push_back(objectId);
			}
			
		} else {
			
			// the block size is dynamically determined based on file length
			// and increases bigger for larger file sizes
			rollingHashBits = ceil(log(fileLength / mOptions.blockSplitCount) / log(2));
			if(rollingHashBits < mOptions.minRollingHashBits) rollingHashBits = mOptions.minRollingHashBits;
			if(rollingHashBits > mOptions.maxRollingHashBits) rollingHashBits = mOptions.maxRollingHashBits;
			uint64_t hashMask = (1 << rollingHashBits) - 1;
			
			// the minimum block size, sometimes with the rolling hash algorithm
			// you can get a stream of blocks sizes of 1 due to the right combination
			// of bytes repeating for a long while
			// define the min block size to 4kib, or filesize/65535 as the max
			// number of blocks is 65535
			long minBlockSize = std::max(4096, (int)((fileLength + 65534) / 65535));

			std::vector<uint8_t> blockBuffer;
			blockBuffer.reserve(std::min(1 << (1 + rollingHashBits), mOptions.maxBlockSize));

			int blockCount = 0;
			BufferedInputStream bufferedFile(fileStream);
			while(!bufferedFile.isEof()) {
				uint8_t b = bufferedFile.readByte();
				blockBuffer.push_back(b);
				if(blockBuffer.size() >= minBlockSize &&
				   ((rh.roll(b) & hashMask) == 0 || blockBuffer.size() >= mOptions.maxBlockSize)) {
					
					if(!EVP_DigestUpdate(&md5, &blockBuffer[0], blockBuffer.size())) {
						throw EncryptionFailedException("EVP_DigestUpdate failed.");
					}
					
					Snapshot::ObjectID objectId;
					// upload block
					computeBlockHMAC(&blockBuffer[0], blockBuffer.size(), (uint8_t)compressionType, objectId.id);
					compressEncryptAndUploadBlock(compressionType, objectId, &blockBuffer[0], blockBuffer.size(),
												  [&progress, blockCount](long bytesUploaded, long bytesTotal) -> bool {
													  return progress(blockCount, -1, bytesUploaded, bytesTotal);
												  });
					++blockCount;

					objectIds.push_back(objectId);
					blockBuffer.clear();
				}
			}
			
			if(!blockBuffer.empty()) {
				Snapshot::ObjectID objectId;
				if(!EVP_DigestUpdate(&md5, &blockBuffer[0], blockBuffer.size())) {
					throw EncryptionFailedException("EVP_DigestUpdate failed.");
				}
				
				computeBlockHMAC(&blockBuffer[0], blockBuffer.size(), (uint8_t)compressionType, objectId.id);
				compressEncryptAndUploadBlock(compressionType, objectId, &blockBuffer[0], blockBuffer.size(),
											  [&progress, blockCount](long bytesUploaded, long bytesTotal) -> bool {
												  return progress(blockCount, -1, bytesUploaded, bytesTotal);
											  });
				++blockCount;
				objectIds.push_back(objectId);
			}
		}
		
		// write the digest
		if(!EVP_DigestFinal(&md5, fileMD5, nullptr)) {
			throw EncryptionFailedException("EVP_DigestFinal failed.");
		}
		
		// update the index
		snapshot->addFileEntry(normalizedPath.c_str(),
							   fileInfo.userName().c_str(),
							   fileInfo.groupName().c_str(),
							   fileInfo.type(),
							   fileInfo.mode(),
							   compressionType,
							   fileInfo.length(),
							   fileInfo.lastModifyTime(),
							   rollingHashBits,
							   fileMD5,
							   0,
							   0,
							   objectIds.size(),
							   &objectIds[0]);
	}
	
	void Repository::finalizePack(std::shared_ptr<Snapshot> snapshot, std::shared_ptr<PackUploadState> uploadPackState, FileTransferProgressFunction progress)
	{
		if(!uploadPackState->rollingHash) {
			return;
		}

		Snapshot::ObjectID objectId;
		if(!HMAC_Final(&uploadPackState->hmac, objectId.id, nullptr)) {
			throw EncryptionFailedException("HMAC_Final failed.");
		}
		
		int numObjects = uploadPackState->fileInfos.size();
		std::string objPath = "/data/" + objectIdToString(objectId);
		if(!mDataStore->exist(objPath.c_str())) {
			std::vector<std::shared_ptr<InputStream>> inStreamList;
			std::vector<InputStream *> inStreamListPtr;
			inStreamList.reserve(numObjects);
			uint32_t offset = 0;
			for(int i = 0; i < numObjects; ++i)
			{
				PackFileInfo& fi = uploadPackState->fileInfos[i];
				const std::vector<uint8_t>& data = uploadPackState->packData[i];
				
				MemoryInputStream stream(&data[0], data.size());
				auto encryptedStream = StreamUtils::compressEncryptHMAC(fi.compression, EVP_aes_256_cbc(), mEncKey, mMacKey, stream);
				inStreamList.push_back(encryptedStream);
				inStreamListPtr.push_back(encryptedStream.get());
				
				fi.offset = offset;
				fi.packLength = encryptedStream->size();

				offset += fi.packLength;
			}
			
			
			MultiInputStream multiStream(inStreamListPtr);
			mDataStore->put(objPath.c_str(), multiStream);
		}

		
		for(int i = 0; i < numObjects; ++i)
		{
			const PackFileInfo& fi = uploadPackState->fileInfos[i];

			snapshot->addFileEntry(fi.path.c_str(),
								   fi.user.c_str(),
								   fi.group.c_str(),
								   fi.type,
								   fi.mode,
								   fi.compression,
								   fi.size,
								   fi.mtime,
								   fi.rollingHashBits,
								   fi.md5,
								   fi.offset,
								   fi.packLength,
								   1,
								   &objectId);
		}
		
		uploadPackState->fileInfos.clear();
		uploadPackState->packData.clear();
		uploadPackState->rollingHash.reset();
	}
	
	bool Repository::downloadFile(std::shared_ptr<Snapshot> snapshot, const char *srcPath, OutputStream& fileStream, FileTransferProgressFunction progress)
	{
		using namespace boost;

		const Snapshot::FileEntry *fe = snapshot->getFileEntry(srcPath);
		if(!fe) {
			return false;
		}

		const Snapshot::ObjectID *objectIds = snapshot->indexToObjectID(fe->objectIdIndex);
		for(int i = 0; i < fe->objectCount; ++i) {
			std::string objectPath = "/data/" + objectIdToString(objectIds[i]);

			// download the object to tmpFile
			TempFileStream tmpStream;
			mDataStore->get(objectPath.c_str(), tmpStream,
							[&progress, i, fe] (long bytesDownloaded, long bytesTotal) -> bool {
								return progress(i, fe->objectCount, bytesDownloaded, bytesTotal);
							});

			auto downloadedStream = tmpStream.inputStream();
			
			if(fe->packLength > 0) {
				InputRangeStream rangeStream(*downloadedStream, fe->offset, fe->packLength);
				StreamUtils::decompressDecryptHMAC((CompressionType)fe->compression, EVP_aes_256_cbc(), mEncKey, mMacKey, rangeStream, fileStream);
			} else {
				StreamUtils::decompressDecryptHMAC((CompressionType)fe->compression, EVP_aes_256_cbc(), mEncKey, mMacKey, *downloadedStream, fileStream);
			}
		}
		
		return true;
	}
	
	std::string Repository::objectIdToString(const Snapshot::ObjectID& objectId) const
	{
		char outStr[53];
		base32encode(objectId.id, SHA256_DIGEST_LENGTH, outStr, sizeof(outStr));
		return std::string(outStr, 2) + "/" + std::string(outStr + 2);
	}
}
