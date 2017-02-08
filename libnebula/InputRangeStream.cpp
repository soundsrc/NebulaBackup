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

#include "InputRangeStream.h"
#include "InputStream.h"

namespace Nebula
{
	InputRangeStream::InputRangeStream(InputStream& stream, long offset, long size)
	: mStream(stream)
	, mOffset(offset)
	, mEndOffset(offset + size)
	, mPosition(offset)
	{
		mStream.skip(mOffset);
	}
	
	size_t InputRangeStream::read(void *data, size_t size)
	{
		if(mPosition + size >= mEndOffset) {
			size = mEndOffset - mOffset;
			if(size == 0) return 0;
		}

		size_t n = mStream.read(data, size);
		mPosition += n;
		
		return n;
	}
	
	bool InputRangeStream::canRewind() const
	{
		return mStream.canRewind();
	}
	
	void InputRangeStream::rewind()
	{
		if(canRewind()) {
			mStream.rewind();
			mStream.skip(mOffset);
			mPosition = mOffset;
		}
	}
	
	long InputRangeStream::size() const
	{
		return mEndOffset - mOffset;
	}
}
