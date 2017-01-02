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

#pragma once

#include <stdint.h>
#include <vector>
#include "Lzma2Dec.h"
#include "LzmaUtils.h"
#include "InputStream.h"

namespace Nebula
{
	class InputStream;
	class LZMAInputStream : public InputStream
	{
	public:
		LZMAInputStream(InputStream& stream);
		virtual ~LZMAInputStream();

		virtual size_t read(void *data, size_t size) override;
	private:
		enum { INPUT_BUFFER_SIZE = 16384 };
		InputStream& mStream;
		LzmaAlloc mAlloc;
		CLzma2Dec mDec;
		std::vector<uint8_t> mInputBuffer;
		uint8_t *mBufferPtr;
		size_t mBytesInBuffer;
	};
}
