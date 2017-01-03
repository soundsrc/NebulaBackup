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
#include "libnebula/Exception.h"
#include "libnebula/LZMAUtils.h"
#include "libnebula/LZMAInputStream.h"
#include "libnebula/MemoryInputStream.h"
#include "libnebula/MemoryOutputStream.h"
#include "libnebula/EncryptedOutputStream.h"
#include "libnebula/DecryptedInputStream.h"
#include "gtest/gtest.h"


static void copyStream(Nebula::InputStream& inStream, Nebula::OutputStream& outStream, size_t bufferSize)
{
	std::vector<uint8_t> buffer;
	buffer.resize(bufferSize);
	
	size_t n;
	while((n = inStream.read(&buffer[0], buffer.size())) > 0) {
		outStream.write(&buffer[0], n);
	}
}



static void testReadWriteStream(Nebula::OutputStream& outStream,
								size_t dataSize,
								std::function<Nebula::InputStream *()> createInput,
								size_t bufferSize)
{
	using namespace Nebula;
	
	std::vector<uint8_t> testData, resultData;
	testData.resize(dataSize);
	resultData.resize(dataSize);
	arc4random_buf(&testData[0], dataSize);
	
	MemoryInputStream inData(&testData[0], dataSize);
	copyStream(inData, outStream, bufferSize);
	
	
	outStream.close();

	std::unique_ptr<InputStream> inStream( createInput() );
	MemoryOutputStream resultStream(&resultData[0], resultData.size());
	
	copyStream(*inStream, resultStream, bufferSize);
	EXPECT_TRUE(memcmp(&testData[0], &resultData[0], dataSize) == 0);
	
	uint8_t b = 0;
	EXPECT_THROW(resultStream.write(&b, 1), InsufficientBufferSizeException);
}

TEST(StreamTests, MemoryStreamReadWrite)
{
	using namespace Nebula;

	std::vector<uint8_t> buffer;
	buffer.reserve(1000000);;
	
	srand(time(nullptr));
	for(int i = 0; i < 16; ++i) {
		size_t size = 1 + (rand() % 1000000);
		size_t bufferSize = 1 + (rand() % 1024);
		
		buffer.resize(size);
		
		MemoryOutputStream outStream(&buffer[0], size);
		
		testReadWriteStream(outStream, size,
							[&buffer, size]() -> InputStream * {
								return new MemoryInputStream(&buffer[0], size);
							}, bufferSize);
	}
}

TEST(StreamTests, EncryptionStreamReadWrite)
{
	using namespace Nebula;

	uint8_t key[32];
	uint8_t iv[32];
	
	std::vector<uint8_t> buffer;
	buffer.reserve(1000000 + 32);
	
	srand(time(nullptr));
	for(int i = 0; i < 16; ++i) {
		size_t size = 1 + (rand() % 1000000);
		size_t bufferSize = 1 + (rand() % 1024);
		
		buffer.resize(size + 32);
		
		arc4random_buf(key, sizeof(key));
		arc4random_buf(iv, sizeof(iv));
		
		MemoryOutputStream outStream(&buffer[0], size + 32);
		EncryptedOutputStream encOutStream(outStream, EVP_aes_256_cbc(), key, iv);
		std::unique_ptr<MemoryInputStream> inStream;

		testReadWriteStream(encOutStream, size,
							[&inStream, &outStream, &size, &key, &iv]() -> InputStream * {
								std::unique_ptr<MemoryInputStream> memStream( new MemoryInputStream (outStream.data(), outStream.size()) );
								inStream = std::move(memStream);
								return new DecryptedInputStream(*inStream, EVP_aes_256_cbc(), key, iv);
							}, bufferSize);
	}
}

TEST(StreamTests, LZMAStreamReadWrite)
{
	using namespace Nebula;

	std::vector<uint8_t> inData, compressedData, outData;
	srand(time(nullptr));
	
	for(int i = 0; i < 16; ++i) {
		size_t size = 1 + (rand() % 100000);
		inData.resize(size);
		compressedData.resize(size + 1000);
		outData.resize(size);
		
		arc4random_buf(&inData[0], inData.size());
		MemoryInputStream inStream(&inData[0], inData.size());
		MemoryOutputStream outStream(&compressedData[0], compressedData.size());
		
		LZMAUtils::compress(inStream, outStream, nullptr);
		
		MemoryInputStream inStream2(outStream.data(), outStream.size());
		LZMAInputStream lzStream(inStream2);
		MemoryOutputStream outStream2(&outData[0], outData.size());
		
		copyStream(lzStream, outStream2, 1 + (rand() % 4096));
		
		EXPECT_TRUE(memcmp(&inData[0], &outData[0], size) == 0);
	}
}
