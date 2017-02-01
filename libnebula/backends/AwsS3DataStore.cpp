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
#include "AwsS3DataStore.h"
#include <stdlib.h>
#include <ios>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <aws/core/Aws.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/HeadObjectResult.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/PutObjectResult.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectResult.h>
#include <aws/transfer/TransferManager.h>
#include <aws/s3/S3Client.h>
#include "libnebula/InputStream.h"
#include "libnebula/OutputStream.h"
#include "libnebula/Exception.h"

namespace Nebula
{
	bool AwsS3DataStore::sDoOnce = true;
	Aws::SDKOptions AwsS3DataStore::sAwsOptions;
	
	void AwsS3DataStore::awsExit()
	{
		Aws::ShutdownAPI(sAwsOptions);
	}
	
	const char *AwsS3DataStore::ALLOC_TAG = "Nebula";

	AwsS3DataStore::AwsS3DataStore(const char *bucket, ZeroedString accessKeyId, ZeroedString secretKey)
	: mBucket(bucket)
	{
		if(sDoOnce) {
			Aws::InitAPI(sAwsOptions);
			atexit(awsExit);
			sDoOnce = false;
		}
		
		Aws::Auth::AWSCredentials creds(accessKeyId.c_str(), secretKey.c_str());
		
		mClient = Aws::MakeShared<Aws::S3::S3Client>(ALLOC_TAG, creds);
	}
	
	AwsS3DataStore::~AwsS3DataStore()
	{
	}
	
	bool AwsS3DataStore::exist(const char *path, ProgressFunction progress)
	{
		Aws::S3::Model::HeadObjectRequest request;
		request.SetBucket(mBucket);
		request.SetKey(path);
		Aws::S3::Model::HeadObjectOutcome result = mClient->HeadObject(request);
		return result.IsSuccess();
	}
	
	class AwsS3DataStore::IOStreamInputBuf : public std::streambuf
	{
	public:
		IOStreamInputBuf(InputStream& inStream)
		: mStream(inStream)
		{
			mOffset = 0;
			setg(mBuffer, mBuffer, mBuffer);
		}
	private:
		InputStream& mStream;
		char mBuffer[8192];
		off_t mOffset;

		virtual int_type underflow() override;
		pos_type seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
	};
	
	std::streambuf::int_type AwsS3DataStore::IOStreamInputBuf::underflow()
	{
		if (gptr() < egptr()) {
			return traits_type::to_int_type(*gptr());
		}

		size_t n;
		if(!(n = mStream.read(mBuffer, sizeof(mBuffer)))) {
			return traits_type::eof();
		}
		mOffset += n;

		setg(mBuffer, mBuffer, mBuffer + n);

		return traits_type::to_int_type(*gptr());
	}
	
	std::streambuf::pos_type AwsS3DataStore::IOStreamInputBuf::seekoff(off_type off, std::ios_base::seekdir way, std::ios_base::openmode which)
	{
		if(which & std::ios_base::in) {
			if(off == 0) {
				if(way == std::ios_base::end) {
					mOffset = mStream.size();
					setg(mBuffer, mBuffer, mBuffer);
					return mOffset;
				} else if(way == std::ios_base::cur) {
					off_t off = mOffset - (egptr() - gptr());
					return off;
				} else if(way == std::ios_base::beg && mStream.canRewind()) {
					mOffset = 0;
					mStream.rewind();
					setg(mBuffer, mBuffer, mBuffer);
					return 0;
				}
			}
		}
		return -1;
	}

	class AwsS3DataStore::IOStreamOutputBuf : public std::streambuf
	{
	public:
		IOStreamOutputBuf(OutputStream& outStream)
		: mStream(outStream)
		{
			setp(mBuffer, mBuffer + sizeof(mBuffer));
		}
	private:
		OutputStream& mStream;
		char mBuffer[8192];
		
		virtual int_type overflow(int_type __c = traits_type::eof()) override;
		virtual int sync() override;
	};

	std::streambuf::int_type AwsS3DataStore::IOStreamOutputBuf::overflow(int_type c)
	{
		if(c == traits_type::eof()) return c;
		
		if(pptr() == epptr()) {
			sync();
		}
		
		*pptr() = c;
		pbump(1);
		
		return c;
	}
	
	int AwsS3DataStore::IOStreamOutputBuf::sync()
	{
		if(pptr() != pbase()) {
			mStream.write(pbase(), (uintptr_t)pptr() - (uintptr_t)pbase());
			setp(mBuffer, mBuffer + sizeof(mBuffer));
		}
		return 0;
	}

	void AwsS3DataStore::get(const char *path, OutputStream& stream, ProgressFunction progress)
	{
		using namespace Nebula;
		
		IOStreamOutputBuf outStreambuf(stream);
		Aws::Transfer::TransferManagerConfiguration transferConfig;
		transferConfig.s3Client = mClient;

		std::shared_ptr<Aws::Transfer::TransferHandle> transferHandle;

		transferConfig.downloadProgressCallback =
			[&progress, &transferHandle](const Aws::Transfer::TransferManager *transferManager, const Aws::Transfer::TransferHandle& handle)
			{
				if(!progress(handle.GetBytesTransferred(), handle.GetBytesTotalSize())) {
					transferHandle->Cancel();
				}
			};
		
		Aws::Transfer::TransferManager transferManager(transferConfig);
		transferHandle = transferManager.DownloadFile(mBucket, path, [&outStreambuf]() -> Aws::IOStream * {
			return Aws::New<Aws::IOStream>(ALLOC_TAG, &outStreambuf);
		});
		transferHandle->WaitUntilFinished();
		if(transferHandle->GetStatus() != Aws::Transfer::TransferStatus::COMPLETED)
		{
			throw FileIOException(transferHandle->GetLastError().GetMessage().c_str());
		}
	}
	
	
	void AwsS3DataStore::put(const char *path, InputStream& stream, ProgressFunction progress)
	{
		using namespace Nebula;

		IOStreamInputBuf inStreamBuf(stream);
		std::shared_ptr<Aws::IOStream> ioStream(Aws::MakeShared<Aws::IOStream>(ALLOC_TAG, &inStreamBuf));
		Aws::Transfer::TransferManagerConfiguration transferConfig;
		transferConfig.s3Client = mClient;

		std::shared_ptr<Aws::Transfer::TransferHandle> transferHandle;

		transferConfig.uploadProgressCallback =
			[&progress, &transferHandle](const Aws::Transfer::TransferManager *transferManager, const Aws::Transfer::TransferHandle& handle)
			{
				if(!progress(handle.GetBytesTransferred(), handle.GetBytesTotalSize())) {
					transferHandle->Cancel();
				}
			};
		Aws::Transfer::TransferManager transferManager(transferConfig);
		Aws::Map<Aws::String, Aws::String> metaData;
		if(stream.size() >= 0) {
			metaData["Content-Length"] = std::to_string(stream.size()).c_str();
		}

		// if can rewind, 
		if(stream.canRewind()) {
			stream.rewind();
			EVP_MD_CTX ctx;
			EVP_DigestInit(&ctx, EVP_md5());

			uint8_t buffer[8192];
			size_t n;
			
			while((n = stream.read(buffer, sizeof(buffer))) > 0) {
				EVP_DigestUpdate(&ctx, buffer, n);
			}
			Aws::Utils::ByteBuffer md5(MD5_DIGEST_LENGTH);
			EVP_DigestFinal(&ctx, md5.GetUnderlyingData(), nullptr);
			
			metaData["Content-MD5"] = Aws::Utils::HashingUtils::Base64Encode(md5);
			
			stream.rewind();
		}

		transferHandle = transferManager.UploadFile(ioStream, mBucket, path, "application/octet-stream", metaData);
		transferHandle->WaitUntilFinished();

		if(transferHandle->GetStatus() != Aws::Transfer::TransferStatus::COMPLETED)
		{
			throw FileIOException(transferHandle->GetLastError().GetMessage().c_str());
		}
	}
	
	void AwsS3DataStore::list(const char *path, std::function<void (const char *, void *)> listCallback, void *userData, ProgressFunction progress)
	{
		
	}
	
	bool AwsS3DataStore::unlink(const char *path, ProgressFunction progress)
	{
		using namespace Nebula;
		
		Aws::S3::Model::DeleteObjectRequest request;
		
		request.SetBucket(mBucket);
		request.SetKey(path);
		
		Aws::S3::Model::DeleteObjectOutcome result = mClient->DeleteObject(request);
		if(!result.IsSuccess()) {
			throw FileIOException(result.GetError().GetMessage().c_str());
		}
		
		return true;
	}
}
