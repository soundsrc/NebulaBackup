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
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <vector>
extern "C" {
#include "compat/string.h"
}
#include "libnebula/RollingHash.h"
#include "gtest/gtest.h"

TEST(RollingHashTests, Roll) {
	using namespace Nebula;
	
	std::vector<uint8_t> mHash;
	
	mHash.resize(16384 * 2);
	
	arc4random_buf(&mHash[0], 16384);
	memcpy(&mHash[16384], &mHash[0], 16384);
	
	uint8_t key[32];
	arc4random_buf(key, sizeof(key));

	RollingHash h(key, 16384);
	uint32_t hash1, hash2;
	for(int i = 0; i < 16384; ++i) {
		 h.roll(mHash[i]);
	}
	hash1 = h.hash();
	for(int i = 16384; i < 16384 * 2; ++i) {
		h.roll(mHash[i]);
	}
	hash2 = h.hash();
	
	ASSERT_EQ(hash1, hash2);
}

TEST(RollingHashTests, Distribution)
{
	using namespace Nebula;

	srand(time(nullptr));
	
	uint8_t key[32];
	arc4random_buf(key, sizeof(key));
	RollingHash h(key, 16384);
	uint64_t sum = 0;
	for(int i = 0; i < 100; ++i) {
		uint64_t bytes = 0;
		while((h.roll(rand() & 0xFF) & ((1 << 20) - 1)) != 0) {
			++bytes;
		}
		sum += bytes;
		printf("Rolling size: %llu bytes\n", bytes);
	}
	printf("Average rolling size: %llu bytes\n", sum / 100);
}
