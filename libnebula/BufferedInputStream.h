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

#include <stdint.h>
#include "InputStream.h"

namespace Nebula
{
	class BufferedInputStream : public InputStream
	{
	public:
		BufferedInputStream(InputStream& stream);
		
		uint8_t readByte() {
			if(mBytesInBuffer == 0) reload();
			if(mBytesInBuffer > 0) {
				uint8_t b = *mNext++;
				--mBytesInBuffer;
				return b;
			}
			return 0;
		}
		
		bool isEof() { return !reload(); }

	private:
		InputStream& mStream;

		bool reload();
		virtual size_t read(void *data, size_t size) override;

		uint8_t mBuffer[8192];
		const uint8_t *mNext;
		int mBytesInBuffer;
	};
}
