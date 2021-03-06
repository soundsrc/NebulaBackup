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

#include <stddef.h>
#include <stdint.h>
#include "OutputStream.h"

namespace Nebula
{
	/**
	 * Output stream interface
	 */
	class MemoryOutputStream : public OutputStream
	{
	public:
		MemoryOutputStream(uint8_t *buffer, size_t size);
		
		const uint8_t *data() const { return mStart; }
		size_t size() const { return mNext - mStart; }

		void reset() { mNext = mStart; }

		virtual void write(const void *data, size_t size) override;
	private:
		uint8_t *mStart;
		uint8_t *mNext;
		uint8_t *mEnd;
	};
}
