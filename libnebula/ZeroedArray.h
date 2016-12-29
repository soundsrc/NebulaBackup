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

extern "C" {
#include "compat/string.h"
}

namespace Nebula
{
	template<typename T, int Size>
	class ZeroedArray
	{
	public:
		ZeroedArray() = default;
		~ZeroedArray() {
			explicit_bzero(mBuffer, Size);
		}
		
		constexpr int size() const { return Size; }
		inline const T& operator[] (int n) const { return mBuffer[n]; }
		inline T& operator[] (int n) { return mBuffer[n]; }
		
		inline T *data() { return mBuffer; }
		inline const T *data() const { return mBuffer; }
	private:
		T mBuffer[Size];
	};
}
