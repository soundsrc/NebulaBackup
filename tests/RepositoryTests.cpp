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
#include "libnebula/FileStream.h"
#include "libnebula/MemoryInputStream.h"
#include "libnebula/MemoryOutputStream.h"
#include "libnebula/TempFileStream.h"

#include "gtest/gtest.h"

TEST(RepositoryTests, CreateAndUnlock) {
	
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		std::unique_ptr<path, std::function<void (path *)>>
			onExit{ &tmpPath, [](path *p) { remove_all(*p); } };

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

TEST(RepositoryTests, ChangePassword)
{
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		std::unique_ptr<path, std::function<void (path *)>>
			onExit{ &tmpPath, [](path *p) { remove_all(*p); } };
		
		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		EXPECT_NO_THROW(repo.initializeRepository("randomPassword1234"));
		EXPECT_TRUE(repo.unlockRepository("randomPassword1234"));
		EXPECT_FALSE(repo.unlockRepository("randomPassword1234w"));
		EXPECT_FALSE(repo.changePassword("randomPassword1234w", "asdfasd"));
		EXPECT_TRUE(repo.changePassword("randomPassword1234", "newPassword"));
		
		EXPECT_FALSE(repo.unlockRepository("wrongPassword"));
		EXPECT_FALSE(repo.unlockRepository("randomPassword1234"));
		EXPECT_TRUE(repo.unlockRepository("newPassword"));
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
		std::unique_ptr<path, std::function<void (path *)>>
			onExit{ &tmpPath, [](path *p) { remove_all(*p); } };
		
		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		EXPECT_NO_THROW(repo.initializeRepository("@&*^%#bh1237"));
		
		std::shared_ptr<Snapshot> snapshot(repo.createSnapshot());
		
		uint8_t randomData1[8197], randomData2[8197];
		arc4random_buf(randomData1, sizeof(randomData1));
		
		path tmpFile = unique_path();
		std::unique_ptr<path, std::function<void (path *)>>
			onExit2{ &tmpFile, [](path *p) { remove_all(*p); } };
		
		FILE *fp = fopen(tmpFile.c_str(), "wb");
		EXPECT_TRUE(fp);
		if(fp) {
			fwrite(&randomData1[0], 1, sizeof(randomData1), fp);
			fclose(fp);
		
			FileStream fs(tmpFile.c_str(), FileMode::Read);
			EXPECT_NO_THROW(repo.uploadFile(snapshot, "/random/path/to/file", fs));
			
			MemoryOutputStream readStream(randomData2, sizeof(randomData2));
			EXPECT_FALSE(repo.downloadFile(snapshot, "/not/exist", readStream));
			EXPECT_TRUE(repo.downloadFile(snapshot, "random/path/to/file", readStream));
			EXPECT_TRUE(memcmp(randomData1, randomData2, sizeof(randomData1)) == 0);
		}
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(RepositoryTests, LargeFileUploadTest)
{
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		std::unique_ptr<path, std::function<void (path *)>>
			onExit{ &tmpPath, [](path *p) { remove_all(*p); } };
		
		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		EXPECT_NO_THROW(repo.initializeRepository("@&*^%#bh1237"));
		
		std::shared_ptr<Snapshot> snapshot(repo.createSnapshot());
		
		std::vector<uint8_t> randomData1, randomData2;
		randomData1.resize(4 * 1024 * 1024); // 4MB?
		arc4random_buf(&randomData1[0], randomData1.size());
		
		path tmpFile = unique_path();
		std::unique_ptr<path, std::function<void (path *)>>
			onExit2{ &tmpFile, [](path *p) { remove_all(*p); } };
		
		FILE *fp = fopen(tmpFile.c_str(), "wb");
		EXPECT_TRUE(fp);
		if(fp) {
			fwrite(&randomData1[0], 1, randomData1.size(), fp);
			fclose(fp);
			
			FileStream fs(tmpFile.c_str(), FileMode::Read);
			EXPECT_NO_THROW(repo.uploadFile(snapshot, "/random/path/to/file", fs));
			
			randomData2.resize(4 * 1024 * 1024);
			MemoryOutputStream readStream(&randomData2[0], randomData2.size());
			EXPECT_FALSE(repo.downloadFile(snapshot, "/not/exist", readStream));
			EXPECT_TRUE(repo.downloadFile(snapshot, "random/path/to/file", readStream));
			EXPECT_TRUE(memcmp(&randomData1[0], &randomData2[0], randomData1.size()) == 0);
		}
	}
	
	EXPECT_FALSE(exists(tmpPath));
}

TEST(RepositoryTests, SnapshotTest)
{
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		std::unique_ptr<path, std::function<void (path *)>>
			onExit{ &tmpPath, [](path *p) { remove_all(*p); } };
		
		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		EXPECT_NO_THROW(repo.initializeRepository("803487"));
		
		std::shared_ptr<Snapshot> snapshot(repo.createSnapshot());
		
		path tmpFile = unique_path();
		std::unique_ptr<path, std::function<void (path *)>>
			onExit2{ &tmpFile, [](path *p) { remove_all(*p); } };

		FileStream fout(tmpFile.c_str(), FileMode::Write);
		fout.writeType<uint8_t>(0);
		fout.close();
		
		FileStream fin(tmpFile.c_str(), FileMode::Read);
		repo.uploadFile(snapshot, "/file1", fin);
		fin.rewind();
		repo.uploadFile(snapshot, "/file2", fin);
		fin.rewind();
		repo.uploadFile(snapshot, "/file3", fin);
		
		EXPECT_NO_THROW(repo.commitSnapshot(snapshot, "test-snapshot"));
		
		std::shared_ptr<Snapshot> loadedSnapshot (repo.loadSnapshot("test-snapshot", DefaultProgressFunction));
		EXPECT_TRUE(loadedSnapshot->getFileEntry("/file1"));
		EXPECT_TRUE(loadedSnapshot->getFileEntry("/file2"));
		EXPECT_TRUE(loadedSnapshot->getFileEntry("/file3"));
		EXPECT_FALSE(loadedSnapshot->getFileEntry("/file4"));
		
	}
	EXPECT_FALSE(exists(tmpPath));
}

