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

#include <stdint.h>
#include "RollingHash.h"

namespace Nebula
{
	RollingHash::RollingHash(int windowSize)
	: mHash(0)
	, mConstantPowWinSize(1)
	, mWindowSize(windowSize)
	, mIndex(windowSize - 1)
	{
		mWindow.resize(windowSize);
		
		// there is definately a faster way to calculate this
		for(int i = 0; i < windowSize; ++i) {
			mConstantPowWinSize *= Constant;
		}
	}

	uint32_t RollingHash::roll(uint8_t c)
	{
		mIndex = (mIndex + 1) % mWindowSize;
		uint32_t sub = mConstantPowWinSize * mWindow[mIndex];
		mWindow[mIndex] = c;
		mHash = (mHash * Constant) + c - sub;
		return mHash;
	}
}
