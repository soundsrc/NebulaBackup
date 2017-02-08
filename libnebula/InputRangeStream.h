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

#include "InputStream.h"

namespace Nebula
{
	class InputRangeStream : public InputStream
	{
	public:
		InputRangeStream(InputStream& stream, long offset, long size);
		
		virtual size_t read(void *data, size_t size) override;

		/**
		 * True if the stream can be rewind
		 */
		virtual bool canRewind() const override;
		
		/**
		 * Rewinds the stream to the beginning, if supported.
		 */
		virtual void rewind() override;
		
		/**
		 * Returns the size of the input stream if available.
		 * Returns -1 if not available
		 */
		virtual long size() const override;
	private:
		InputStream& mStream;
		long mOffset;
		long mEndOffset;
		long mPosition;

	};
}
