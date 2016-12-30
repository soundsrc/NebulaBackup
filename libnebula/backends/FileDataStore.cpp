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
#include "OutputStream.h"
#include "InputStream.h"
#include "DataStore.h"
#include "Exception.h"
#include "ScopedExit.h"
#include "FileDataStore.h"

namespace Nebula
{
	FileDataStore::FileDataStore(const char *storeDirectory)
	{
		mStoreDirectory = storeDirectory;
	}
	
	AsyncProgress<bool> FileDataStore::get(const char *path, OutputStream& stream)
	{
		using namespace boost;
		
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();
		
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;
		std::async([this, fullPath, &stream, &progress]() -> void {
			try {
				
				FILE *fp = fopen(fullPath.c_str(), "rb");
				if(!fp) {
					switch(errno) {
						case ENOENT:
							progress->setReady(false);
							return;
						default: throw FileIOException(fullPath.string() + ": " + strerror(errno)); break;
					}
				}
				
				ScopedExit onExit([fp] { fclose(fp); });

				char buffer[4096];
				ssize_t n;
				while((n = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
					if(progress->cancelRequested()) {
						progress->setCancelled();
						return;
					}
					if(stream.write(buffer, (int)n) < n) {
						throw DiskQuotaExceededException("Output stream does not have enough space.");
					}
				}

				if(n == 0 && ferror(fp)) {
					throw FileIOException(fullPath.string() + ": Read error.");
				}

				progress->setReady(true);
			} catch(const std::exception& e) {
				progress->setError(e.what());
			}
		});
		
		return AsyncProgress<bool>(progress);
	}
	
	AsyncProgress<bool> FileDataStore::put(const char *path, InputStream& stream)
	{
		using namespace boost;
		
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();
		
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;
		std::async([this, fullPath, &stream, &progress]() -> void {
			try {
				// do not allow overwrite
				if(filesystem::exists(fullPath)) {
					progress->setReady(false);
					return;
				}

				if(!filesystem::exists(fullPath.parent_path()))
				{
					filesystem::create_directories(fullPath.parent_path());
				}
				
				FILE *fp = fopen(fullPath.c_str(), "wb");
				if(!fp) {
					throw FileIOException(fullPath.string() + ": " + strerror(errno));
				}

				ScopedExit onExit([fp] { if(fp) fclose(fp); });

				int n;
				char buffer[4096];
				while((n = stream.read(buffer, sizeof(buffer))) >= 0) {
					if(fwrite(buffer, 1, n, fp) < n) {
						if(ferror(fp)) {
							throw FileIOException(fullPath.string() + ": Write error.");
						}
					}
					
					// if cancel has been requested, remove the in progress file
					if(progress->cancelRequested()) {
						fclose(fp);
						fp = nullptr;
						filesystem::remove(fullPath);
						progress->setCancelled();
						return;
					}
				}
				
				progress->setReady(true);
			} catch(const std::exception& e) {
				progress->setError(e.what());
			}
		});
		
		return AsyncProgress<bool>(progress);
	}

	AsyncProgress<bool> FileDataStore::list(const char *path, std::function<void (const char *, void *)> listCallback, void *userData)
	{
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();

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
		
		progress->setReady(true);
		return AsyncProgress<bool>(progress);
	}
	
	AsyncProgress<bool> FileDataStore::unlink(const char *path)
	{
		using namespace boost;

		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();
		
		filesystem::path fullPath = filesystem::path(mStoreDirectory) / path;
		if(!filesystem::remove(fullPath)) {
			progress->setError(fullPath.string() + ": Failed to remove object.");
		} else {
			progress->setReady(true);
		}
		return AsyncProgress<bool>(progress);
	}
}
