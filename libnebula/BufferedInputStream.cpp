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
#include "BufferedInputStream.h"
#include <string.h>

namespace Nebula
{
	BufferedInputStream::BufferedInputStream(InputStream& stream)
	: mStream(stream)
	, mNext(mBuffer)
	, mBytesInBuffer(0)
	{
	}
	
	bool BufferedInputStream::reload()
	{
		if(mBytesInBuffer == 0)
		{
			mBytesInBuffer = mStream.read(mBuffer, sizeof(mBuffer));
			mNext = mBuffer;
		}
		
		return mBytesInBuffer != 0;
	}
	
	size_t BufferedInputStream::read(void *data, size_t size)
	{
		size_t totalBytes = 0;
		uint8_t *pData = (uint8_t *)data;
		
		while(size >= mBytesInBuffer)
		{
			memcpy(pData, mNext, mBytesInBuffer);
			pData += mBytesInBuffer;
			size -= mBytesInBuffer;
			totalBytes += mBytesInBuffer;
			mBytesInBuffer = 0;
			
			if(!reload()) return totalBytes;
		}

		memcpy(pData, mNext, size);
		mNext += size;
		mBytesInBuffer -= size;
		totalBytes += size;
		
		return totalBytes;
	}
	
	long BufferedInputStream::size() const
	{
		return mStream.size();
	}
}
