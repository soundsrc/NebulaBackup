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
#include <string.h>
#include <boost/filesystem.hpp>
#include "libnebula/backends/FileDataStore.h"
#include "libnebula/MemoryInputStream.h"
#include "libnebula/MemoryOutputStream.h"
#include "libnebula/Repository.h"
#include "libnebula/ScopedExit.h"
#include "gtest/gtest.h"

TEST(DataStoreTests, Put) {

	using namespace boost::filesystem;
	using namespace Nebula;

	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });

		FileDataStore ds(tmpPath.c_str());
		
		printf("%s\n", tmpPath.c_str());
		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		ds.put("/dddddd", dataStream).wait();
		
		MemoryOutputStream outStream(data2, sizeof(data2));
		ds.get("/dddddd", outStream);
		
		EXPECT_TRUE(memcmp(data1, data2, sizeof(data1)) == 0);
	}

	EXPECT_FALSE(exists(tmpPath));
}
