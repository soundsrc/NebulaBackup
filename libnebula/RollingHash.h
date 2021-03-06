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
#include "ZeroedArray.h"

namespace Nebula
{
	/**
	 * Rolling hash.
	 */
	class RollingHash
	{
	public:
		/**
		 * Creates a rolling hash with the specified window size
		 */
		RollingHash(const uint8_t *key, int windowSize);
		~RollingHash();
		
		/**
		 * Updates the rolling hash with the byte.
		 */
		uint64_t roll(uint8_t c);
		
		uint64_t hash() const { return mHash; }
	private:
		uint64_t mHash;

		uint64_t mConstant;
		uint64_t mConstantPowWinSize;

		int mWindowSize;
		int mIndex;
		std::vector<uint8_t> mWindow;

		ZeroedArray<uint8_t, 32> mKey;
		ZeroedArray<uint32_t, 256> mSubTable;
	};
}
