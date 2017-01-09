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
#include <boost/filesystem.hpp>
#include "libnebula/Repository.h"
#include "libnebula/DataStore.h"
#include "libnebula/backends/FileDataStore.h"
#include "libnebula/ScopedExit.h"
#include "libnebula/FileStream.h"
#include "libnebula/MemoryOutputStream.h"
#include "libnebula/ScopedExit.h"

#include "gtest/gtest.h"

TEST(RepositoryTests, CreateAndUnlock) {
	
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		scopedExit([tmpPath] { remove_all(tmpPath); });

		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		EXPECT_NO_THROW(repo.initializeRepository("randomPassword1234"));
		EXPECT_TRUE(repo.unlockRepository("randomPassword1234"));
		EXPECT_FALSE(repo.unlockRepository("randomPassword1234w"));
		EXPECT_FALSE(repo.unlockRepository("randomPassword123"));
		EXPECT_FALSE(repo.unlockRepository("randompassword1234"));
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(RepositoryTests, SmallFileUploadTest)
{
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		scopedExit([tmpPath] { remove_all(tmpPath); });
		
		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		EXPECT_NO_THROW(repo.initializeRepository("@&*^%#bh1237"));
		
		std::unique_ptr<Snapshot> snapshot(repo.createSnapshot());
		
		uint8_t randomData1[8197], randomData2[8197];
		arc4random_buf(randomData1, sizeof(randomData1));
		
		path tmpFile = unique_path();
		scopedExit([tmpFile] { remove(tmpFile); });
		
		FILE *fp = fopen(tmpFile.c_str(), "wb");
		EXPECT_TRUE(fp);
		if(fp) {
			fwrite(&randomData1[0], 1, sizeof(randomData1), fp);
			fclose(fp);
		
			FileStream fs(tmpFile.c_str(), FileMode::Read);
			EXPECT_NO_THROW(repo.uploadFile(snapshot.get(), "/random/path/to/file", fs));
			
			MemoryOutputStream readStream(randomData2, sizeof(randomData2));
			EXPECT_FALSE(repo.downloadFile(snapshot.get(), "/not/exist", readStream));
			EXPECT_TRUE(repo.downloadFile(snapshot.get(), "/random/path/to/file", readStream));
			EXPECT_TRUE(memcmp(randomData1, randomData2, sizeof(randomData1)) == 0);
		}
	}
	
	EXPECT_FALSE(exists(tmpPath));
}
