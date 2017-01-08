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
#include <set>
#include <vector>
#include <string.h>
#include <boost/filesystem.hpp>
#include "libnebula/Exception.h"
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
		scopedExit([tmpPath] { remove_all(tmpPath); });

		FileDataStore ds(tmpPath.c_str());

		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		EXPECT_NO_THROW(ds.put("/dddddd", dataStream));

		MemoryOutputStream outStream(data2, sizeof(data2));
		EXPECT_TRUE(ds.get("/dddddd", outStream));
		
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
		scopedExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		
		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		EXPECT_NO_THROW(ds.put("/subpath/dddddd", dataStream));

		MemoryOutputStream outStream(data2, sizeof(data2));
		EXPECT_TRUE(ds.get("/subpath/dddddd", outStream));
		
		EXPECT_TRUE(memcmp(data1, data2, sizeof(data1)) == 0);
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, Existance) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		scopedExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		
		uint8_t data1[4096];
		uint8_t data2[4096];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		EXPECT_NO_THROW(ds.put("/subpath/xxx", dataStream));
		
		EXPECT_TRUE(ds.exist("/subpath/xxx"));
		EXPECT_FALSE(ds.exist("/subpath/xxx1"));
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
		EXPECT_FALSE(ds.get("/notfound", outStream));
	}
}

TEST(DataStoreTests, GetNotEnoughSpace) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		scopedExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		
		uint8_t data1[4096];
		uint8_t data2[12];
		
		arc4random_buf(data1, sizeof(data1));
		arc4random_buf(data2, sizeof(data2));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		EXPECT_NO_THROW(ds.put("/ewfewfe", dataStream));
		
		MemoryOutputStream outStream(data2, sizeof(data2));
		EXPECT_THROW(ds.get("/ewfewfe", outStream), InsufficientBufferSizeException);
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, List) {
	
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		scopedExit([tmpPath] { remove_all(tmpPath); });
		uint8_t buffer[32];
		arc4random_buf(buffer, sizeof(buffer));
		
		FileDataStore ds(tmpPath.c_str());
		
		static const char *paths[] = {
			"/blah",
			"/sub/a.txt",
			"/sub/b.txt",
			"/sub/C.txt",
			"/sub2/a.txt"
			"/sub2/b.txt",
			"/sub2/x/a.txt",
			"/sub2/x/b.txt",
			"/sub2/x/c.txt",
			"/x.txt",
			"/a.txt"
		};
		
		std::set<std::string> pathSet;
		for(int i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i)
		{
			MemoryInputStream m(buffer, sizeof(buffer));
			EXPECT_NO_THROW(ds.put(paths[i], m));
			pathSet.insert(paths[i]);
		}
		
		ds.list("/", [&pathSet](const char *path, void*) {
			if(path) {
				auto it = pathSet.find(path);
				EXPECT_NE(it, pathSet.end());
				if(it != pathSet.end()) {
					pathSet.erase(it);
				}
			}
		}, nullptr);
		EXPECT_EQ(pathSet.size(), 0);
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(DataStoreTests, Deletion) {
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		scopedExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());

		// delete non-existant object
		EXPECT_FALSE(ds.unlink("/22333"));

		uint8_t data1[4096];
		arc4random_buf(data1, sizeof(data1));
		
		MemoryInputStream dataStream(data1, sizeof(data1));
		EXPECT_NO_THROW(ds.put("/22333", dataStream));

		EXPECT_TRUE(ds.unlink("/22333"));
	}
	
	EXPECT_FALSE(exists(tmpPath));

}
