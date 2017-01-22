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
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <vector>
extern "C" {
#include "compat/string.h"
}
#include "libnebula/Exception.h"
#include "libnebula/Base32.h"
#include "gtest/gtest.h"


TEST(Base32Test, TestEncodeDecode)
{
	using namespace Nebula;
	
	std::vector<uint8_t> data, verifyData;
	std::vector<char> encString;
	srand(time(nullptr));
	for(int i = 0; i < 1000; ++i) {
		int inSize = 1 + (rand() % 1000);
		int outSize = 1 + ((inSize + 4) / 5) * 8;
		data.resize(inSize);
		verifyData.resize(inSize);
		encString.resize(outSize);
		
		arc4random_buf(&data[0], data.size());
		
		EXPECT_THROW(base32encode(&data[0], data.size(), &encString[0], encString.size() - 8), InsufficientBufferSizeException);
		EXPECT_NO_THROW(base32encode(&data[0], data.size(), &encString[0], encString.size()));
		
		EXPECT_THROW(base32decode(&encString[0], strlen(&encString[0]), &verifyData[0], verifyData.size() - 1), InsufficientBufferSizeException);
		EXPECT_EQ(inSize, base32decode(&encString[0], strlen(&encString[0]), &verifyData[0], verifyData.size()));
		
		EXPECT_TRUE(memcmp(&data[0], &verifyData[0], data.size()) == 0);
	}
}
