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
#include <boost/filesystem.hpp>
#include "libnebula/DataStore.h"
#include "libnebula/backends/FileDataStore.h"
#include "libnebula/ScopedExit.h"
#include "libnebula/Repository.h"
#include "gtest/gtest.h"

TEST(RepositoryTests, CreateAndUnlock) {
	
	using namespace boost::filesystem;
	using namespace Nebula;
	
	path tmpPath = unique_path();
	EXPECT_TRUE( create_directory(tmpPath) );
	
	{
		ScopedExit onExit([tmpPath] { remove_all(tmpPath); });

		FileDataStore ds(tmpPath.c_str());
		Repository repo(&ds);
		
		repo.initializeRepository("randomPassword1234").wait();
		AsyncProgress<bool> unlockResult = repo.unlockRepository("randomPassword1234");
		unlockResult.wait();
		EXPECT_TRUE(unlockResult.result());
		unlockResult = repo.unlockRepository("randomPassword1234w");
		unlockResult.wait();
		EXPECT_FALSE(unlockResult.result());
	}
	
	EXPECT_FALSE(exists(tmpPath));
}
