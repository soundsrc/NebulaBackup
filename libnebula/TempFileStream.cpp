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
#include "TempFileStream.h"
#include <stdlib.h>
#include <boost/filesystem.hpp>
extern "C" {
#include "compat/string.h"
}
#include "Exception.h"
#include "MemoryOutputStream.h"
#include "MemoryInputStream.h"
#include "FileStream.h"

namespace Nebula
{
	TempFileStream::TempFileStream()
	{
		mBuffer = (uint8_t *)malloc(TEMP_BUFFER_SIZE);
		mMemStream.reset(new MemoryOutputStream(mBuffer, TEMP_BUFFER_SIZE));
	}
	
	TempFileStream::~TempFileStream()
	{
		explicit_bzero(mBuffer, TEMP_BUFFER_SIZE);
		free(mBuffer);

		if(mFileStream) {
			mFileStream->close();

			// shred
			long length = boost::filesystem::file_size(mTmpFile);
			FILE *fp = fopen(mTmpFile.c_str(), "r+b");
			if(fp) {
				char buffer[4096];
				while(length > 0) {
					arc4random_buf(buffer, sizeof(buffer));
					fwrite(buffer, 1, length > sizeof(buffer) ? sizeof(buffer) : length, fp);
					length -= sizeof(buffer);
				}
				fclose(fp);
			}

			// delete
			boost::filesystem::remove(mTmpFile);
		}
	}
	
	void TempFileStream::write(const void *data, size_t size)
	{
		if(mMemStream) {
			if(mMemStream->size() + size > TEMP_BUFFER_SIZE)
			{
				using namespace boost::filesystem;
				mTmpFile = unique_path();
				mFileStream.reset(new FileStream(mTmpFile.c_str(), FileMode::Write));
				mFileStream->write(mMemStream->data(), mMemStream->size());
				mMemStream = nullptr;
			} else {
				mMemStream->write(data, size);
			}
		}
		if(mFileStream) {
			mFileStream->write(data, size);
		} else if(!mMemStream) {
			throw FileIOException("Cannot write to temp stream after input stream has been obtained.");
		}
	}
	
	std::shared_ptr<InputStream> TempFileStream::inputStream()
	{
		if(mMemStream) {
			return std::make_shared<MemoryInputStream>(mMemStream->data(), mMemStream->size());
		}
		if(mFileStream) {
			mFileStream->close();
			return std::make_shared<FileStream>(mTmpFile.c_str(), FileMode::Read);
		}

		return nullptr;
	}
}
