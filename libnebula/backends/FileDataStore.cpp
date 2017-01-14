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
#include <string>
#include <memory>
#include <future>
#include <stdio.h>
#include <errno.h>
#include <boost/filesystem.hpp>
#include "libnebula/OutputStream.h"
#include "libnebula/InputStream.h"
#include "libnebula/DataStore.h"
#include "libnebula/Exception.h"
#include "libnebula/ScopedExit.h"
#include "FileDataStore.h"

namespace Nebula
{
	FileDataStore::FileDataStore(const char *storeDirectory)
	{
		mStoreDirectory = storeDirectory;
	}
	
	bool FileDataStore::exist(const char *path, ProgressFunction progress)
	{
		using namespace boost;
		progress(1, 1);
		
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;
		return filesystem::exists(fullPath);
	}

	void FileDataStore::get(const char *path, OutputStream& stream, ProgressFunction progress)
	{
		using namespace boost;
		
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;
		
		FILE *fp = fopen(fullPath.c_str(), "rb");
		if(!fp) {
			switch(errno) {
				case ENOENT: throw FileNotFoundException(fullPath.string() + ": File not found."); break;
				default: throw FileIOException(fullPath.string() + ": " + strerror(errno)); break;
			}
		}
		
		long fileSize = filesystem::file_size(fullPath);
		
		scopedExit([fp] { fclose(fp); });

		progress(0, fileSize);

		char buffer[4096];
		ssize_t n;
		long bytesRead = 0;
		while((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {

			if(!progress(bytesRead, fileSize)) {
				throw CancelledException("User cancelled.");
			}

			bytesRead += n;

			stream.write(buffer, (size_t)n);
		}

		if(n == 0 && ferror(fp)) {
			throw FileIOException(fullPath.string() + ": Read error.");
		}

		progress(fileSize, fileSize);
	}
	
	void FileDataStore::put(const char *path, InputStream& stream, ProgressFunction progress)
	{
		using namespace boost;

		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;

		if(!filesystem::exists(fullPath.parent_path()))
		{
			filesystem::create_directories(fullPath.parent_path());
		}
		
		FILE *fp = fopen(fullPath.c_str(), "wb");
		if(!fp) {
			throw FileIOException(fullPath.string() + ": " + strerror(errno));
		}

		scopedExit([fp] { if(fp) fclose(fp); });

		long fileSize = -1;
		long bytesRead = 0;
		size_t n;
		char buffer[4096];
		while((n = stream.read(buffer, sizeof(buffer))) > 0) {
			if(fwrite(buffer, 1, n, fp) < n) {
				if(ferror(fp)) {
					throw FileIOException(fullPath.string() + ": Write error.");
				}
			}
			
			// if cancel has been requested, remove the in progress file
			if(!progress(bytesRead, fileSize)) {
				fclose(fp);
				fp = nullptr;
				filesystem::remove(fullPath);
				throw CancelledException("User cancelled.");
			}
		}

		progress(fileSize, fileSize);
	}

	void FileDataStore::list(const char *path, std::function<void (const char *, void *)> listCallback, void *userData, ProgressFunction progress)
	{
		using namespace boost;
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;

		filesystem::recursive_directory_iterator dirIterator(fullPath);

		for(auto& file : dirIterator)
		{
			if(!filesystem::is_directory(file))
			{
				filesystem::path relativePath = filesystem::relative(file.path(), fullPath);
				listCallback((filesystem::path("/") / relativePath).make_preferred().c_str(), userData);
			}
		}
		
		listCallback(nullptr, userData);
	}
	
	bool FileDataStore::unlink(const char *path, ProgressFunction progress)
	{
		using namespace boost;

		
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;
		if(!filesystem::remove(fullPath)) {
			return false;
		} else {
			return true;
		}
	}
}
