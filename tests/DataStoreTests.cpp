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
#include <memory>
#include <vector>
#include <string.h>
#include <boost/filesystem.hpp>
#include "libnebula/DataStore.h"
#include "libnebula/backends/FileDataStore.h"
#include "libnebula/MemoryInputStream.h"
#include "libnebula/MemoryOutputStream.h"
#include "libnebula/Repository.h"
#include "libnebula/ScopedExit.h"
#include "gtest/gtest.h"

TEST(DataStoreTests, PutAndGet) {

	using namespace boost::filesystem;
	using namespace Nebula;

	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });

		FileDataStore ds(tmpPath.c_str());

		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		AsyncProgress<bool> progress = ds.put("/dddddd", dataStream);
		progress.wait();
		EXPECT_FALSE(progress.hasError());
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
		
		MemoryOutputStream outStream(data2, sizeof(data2));
		progress = ds.get("/dddddd", outStream);
		progress.wait();
		EXPECT_FALSE(progress.hasError());
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
		
		EXPECT_TRUE(memcmp(data1, data2, sizeof(data1)) == 0);
	}

	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, PutAndGetWithSubPaths) {
	
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		
		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		AsyncProgress<bool> progress = ds.put("/subpath/dddddd", dataStream);
		progress.wait();
		EXPECT_FALSE(progress.hasError());
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
		
		MemoryOutputStream outStream(data2, sizeof(data2));
		progress = ds.get("/subpath/dddddd", outStream);
		progress.wait();
		EXPECT_FALSE(progress.hasError());
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
		
		EXPECT_TRUE(memcmp(data1, data2, sizeof(data1)) == 0);
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, PutOverwrite) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		
		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		AsyncProgress<bool> progress = ds.put("/subpath/xxx", dataStream);
		progress.wait();
		EXPECT_FALSE(progress.hasError());
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
		
		// put should return false status if attempt overwrite
		MemoryInputStream dataStream2(data2, sizeof(data2));
		progress = ds.put("/subpath/xxx", dataStream2);
		progress.wait();
		EXPECT_FALSE(progress.hasError());
		EXPECT_TRUE(progress.isReady());
		EXPECT_FALSE(progress.result());
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, GetNotFound) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	{
		FileDataStore ds(".");

		uint8_t data[12];
		
		MemoryOutputStream outStream(data, sizeof(data));
		AsyncProgress<bool> progress = ds.get("/notfound", outStream);
		progress.wait();
		EXPECT_TRUE(progress.isReady());
		EXPECT_FALSE(progress.result());
	}
}

TEST(DataStoreTests, GetNotEnoughSpace) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		
		uint8_t data1[4096];
		uint8_t data2[12];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		AsyncProgress<bool> progress = ds.put("/ewfewfe", dataStream);
		progress.wait();
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
		
		MemoryOutputStream outStream(data2, sizeof(data2));
		progress = ds.get("/ewfewfe", outStream);
		progress.wait();
		EXPECT_FALSE(progress.isReady());
		EXPECT_TRUE(progress.hasError());
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, Deletion) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());

		// delete non-existant object
		AsyncProgress<bool> progress = ds.unlink("/22333");
		progress.wait();
		EXPECT_TRUE(progress.hasError());

		uint8_t data1[4096];
		arc4random_buf(data1, sizeof(data1));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		progress = ds.put("/22333", dataStream);
		progress.wait();
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());

		progress = ds.unlink("/22333");
		progress.wait();
		EXPECT_TRUE(progress.isReady());
		EXPECT_TRUE(progress.result());
	}
	
	EXPECT_FALSE(exists(tmpPath));

}
