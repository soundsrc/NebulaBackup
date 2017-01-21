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
#include "MemoryInputStream.h"
#include <string.h>

namespace Nebula
{
	MemoryInputStream::MemoryInputStream(const uint8_t *buffer, size_t size)
	: mStart(buffer)
	, mNext(buffer)
	, mEnd(buffer + size)
	{
	}
	
	size_t MemoryInputStream::read(void *data, size_t size)
	{
		if(mNext >= mEnd) {
			return 0;
		}

		if(size > mEnd - mNext) {
			size = mEnd - mNext;
		}
		
		memcpy(data, mNext, size);
		mNext += size;

		return size;
	}
	
	bool MemoryInputStream::canRewind() const
	{
		return true;
	}
	
	void MemoryInputStream::rewind()
	{
		mNext = mStart;
	}
	
	long MemoryInputStream::size() const
	{
		return mEnd - mStart;
	}
}
