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
#include <openssl/evp.h>
#include <memory>
#include <vector>
#include <random>
extern "C" {
#include "compat/string.h"
}
#include "libnebula/MemoryInputStream.h"
#include "libnebula/MemoryOutputStream.h"
#include "libnebula/EncryptedOutputStream.h"
#include "libnebula/DecryptedInputStream.h"
#include "gtest/gtest.h"

TEST(EncryptionStreamTests, EncryptDecrypt)
{
	uint8_t key[32];
	uint8_t iv[32];
	std::vector<uint8_t> encData, buffer, decData;

	srand(time(nullptr));
	for(int i = 0; i < 100; ++i) {
		int size = 1 + (rand() % 1000000);
		explicit_bzero(key, sizeof(key));
		explicit_bzero(iv, sizeof(iv));
		
		encData.resize(size);
		decData.resize(size);
		buffer.resize(size + 32);
		
		explicit_bzero(&encData[0], size);
		Nebula::MemoryOutputStream outStream(&buffer[0], buffer.size());

		Nebula::EncryptedOutputStream outEnc(outStream, EVP_aes_256_cbc(), key, iv);
		outEnc.write(&encData[0], size);
		outEnc.close();
		
		Nebula::MemoryInputStream encryptedStream(outStream.data(), outStream.size());
		Nebula::DecryptedInputStream inEnc(encryptedStream, EVP_aes_256_cbc(), key, iv);
		
		EXPECT_EQ(inEnc.read(&decData[0], size), size);
		EXPECT_TRUE(memcmp(&decData[0], &encData[0], size) == 0);
	}
}
