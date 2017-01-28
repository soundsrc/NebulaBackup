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

#pragma once

#include <string>
#include <memory>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include "libnebula/DataStore.h"
#include "libnebula/ZeroedString.h"
#include "libnebula/ProgressFunction.h"

namespace Nebula
{
	class AwsS3DataStore : public DataStore
	{
	public:
		AwsS3DataStore(const char *bucket, ZeroedString accessKeyId, ZeroedString secretKey);
		~AwsS3DataStore();
		
		virtual bool exist(const char *path, ProgressFunction progress = DefaultProgressFunction) override;
		virtual void get(const char *path, OutputStream& stream, ProgressFunction progress = DefaultProgressFunction) override;
		virtual void put(const char *path, InputStream& stream, ProgressFunction progress = DefaultProgressFunction) override;
		virtual void list(const char *path, std::function<void (const char *, void *)> listCallback, void *userData, ProgressFunction progress = DefaultProgressFunction) override;
		virtual bool unlink(const char *path, ProgressFunction progress = DefaultProgressFunction) override;
	private:
		static bool sDoOnce;
		static Aws::SDKOptions sAwsOptions;
		
		static void awsExit();
		
		std::shared_ptr<Aws::S3::S3Client> mClient;
		Aws::String mBucket;
		
		static const char *ALLOC_TAG;
		class IOStreamInputBuf;
		class IOStreamOutputBuf;
	};
}
