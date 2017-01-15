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
#include "LZMAInputStream.h"
#include "Lzma2Dec.h"
#include "Exception.h"
#include "InputStream.h"

namespace Nebula
{
	LZMAInputStream::LZMAInputStream(InputStream& stream)
	: mStream(stream)
	, mBufferPtr(&mInputBuffer[0])
	, mBytesInBuffer(0)
	{
		mInputBuffer.resize(INPUT_BUFFER_SIZE);

		Lzma2Dec_Construct(&mDec);
		Byte prop;
		if(stream.read(&prop, sizeof(prop)) != sizeof(prop)) {
			throw LZMAException("Failed to decompress LZMA input stream. Premature EOF.");
		}
		if(Lzma2Dec_Allocate(&mDec, prop, &mAlloc) != SZ_OK) {
			throw LZMAException("Failed to allocate prop for LZMA decoding.");
		}
		
		Lzma2Dec_Init(&mDec);
	}
	
	LZMAInputStream::~LZMAInputStream()
	{
		Lzma2Dec_Free(&mDec, &mAlloc);
	}
	
	size_t LZMAInputStream::read(void *data, size_t size)
	{
		uint8_t *dest = (uint8_t *)data;
		size_t bytesDecoded = 0;
		
		while(bytesDecoded < size)
		{
			if(mBytesInBuffer == 0) {
				mBufferPtr = &mInputBuffer[0];
				mBytesInBuffer = mStream.read(mBufferPtr, mInputBuffer.size());
				if(!mBytesInBuffer) return bytesDecoded;
			}

			size_t inSize = mBytesInBuffer;
			size_t outSize = size - bytesDecoded;
			ELzmaStatus status;
			
			if(Lzma2Dec_DecodeToBuf(&mDec, dest, &outSize, mBufferPtr, &inSize, LZMA_FINISH_ANY, &status) != SZ_OK) {
				throw LZMAException("LZMA decode error.");
			}
			
			mBufferPtr += inSize;
			mBytesInBuffer -= inSize;
			bytesDecoded += outSize;
			dest += outSize;
			
			switch (status) {
				default: break;
				case LZMA_STATUS_NEEDS_MORE_INPUT:
					break;
				case LZMA_STATUS_FINISHED_WITH_MARK:
				case LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK:
					return bytesDecoded;
					break;
			}
		}
		
		return bytesDecoded;
	}
}
