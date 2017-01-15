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
#include "FileStream.h"
#include <assert.h>
#include "Exception.h"

namespace Nebula
{
	FileStream::FileStream() : mFp(nullptr)
	{
	}
	
	FileStream::FileStream(const char *path, FileMode mode) : mFp(nullptr)
	{
		if(!open(path, mode)) {
			throw FileNotFoundException("File was not found.");
		}
	}
	
	FileStream::~FileStream()
	{
		close();
	}
	
	long FileStream::length() const
	{
		long length;
		long curPos = ftell(mFp);
		if(curPos < 0) goto error;
		
		if(fseek(mFp, 0L, SEEK_END) < 0) goto error;

		length = ftell(mFp);
		if(length < 0) goto error;
		
		if(fseek(mFp, curPos, SEEK_SET) < 0) goto error;
		
		return length;
error:
		throw FileIOException("Failed to determine file length.");
		return 0;
	}
	
	bool FileStream::open(const char *path, FileMode mode)
	{
		close();

		switch(mode) {
			case FileMode::Read:
				mFp = fopen(path, "rb");
				break;
			case FileMode::Write:
				mFp = fopen(path, "wb");
				break;
			case FileMode::ReadWrite:
				mFp = fopen(path, "r+b");
				break;
			default:
				assert(false);
				throw InvalidArgumentException("Invalid file mode.");
		}
		
		if(!mFp) {
			return false;
		}
		
		mPath = path;

		return true;
	}

	bool FileStream::canRewind() const
	{
		return true;
	}
	
	void FileStream::rewind()
	{
		if(fseek(mFp, 0L, SEEK_SET) < 0) {
			throw FileIOException("Failed to rewind stream.");
		}
	}
	
	long FileStream::seek(long offset)
	{
		if(fseek(mFp, offset, SEEK_SET) < 0) {
			throw FileIOException("Failed to see to offset.");
		}
		
		long pos = ftell(mFp);
		if(pos < 0) {
			throw FileIOException("Failed to determine file position.");
		}
		
		return pos;
	}

	size_t FileStream::read(void *data, size_t size)
	{
		size_t n = fread(data, 1, size, mFp);
		if(n == 0) {
			if(ferror(mFp)) {
				throw FileIOException("There was an error reading the file.");
			}
		}
		return n;
	}
	
	void FileStream::write(const void *data, size_t size)
	{
		size_t n = fwrite(data, 1, size, mFp);
		if(n != size) {
			throw FileIOException("There was an error writing the file.");
		}
	}

	void FileStream::flush()
	{
		fflush(mFp);
	}
	
	void FileStream::close()
	{
		if(mFp) {
			fclose(mFp);
			mFp = nullptr;
			mPath = "";
		}
	}
}
