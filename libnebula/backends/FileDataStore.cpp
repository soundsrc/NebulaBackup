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
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
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
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();
		
		std::string fullPath = mStoreDirectory + "/" + path;
		std::async([this, fullPath, &stream, &progress]() -> void {
			try {
				
				int fd = ::open(fullPath.c_str(), O_RDONLY);
				if(fd < 0) {
					switch(errno) {
						case ENOENT:
							progress->setReady(false);
							return;
						default: throw FileIOException(fullPath + ": " + strerror(errno)); break;
					}
				}
				
				ScopedExit onExit([fd] { close(fd); });

				char buffer[4096];
				ssize_t n;
				while((n = ::read(fd, buffer, sizeof(buffer))) > 0) {
					if(progress->cancelRequested()) {
						progress->setCancelled();
						return;
					}
					if(stream.write(buffer, (int)n) < n) {
						throw DiskQuotaExceededException("Output stream does not have enough space.");
					}
				}

				if(n < 0) {
					throw FileIOException(fullPath + ": " + strerror(errno));
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
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();
		
		std::string fullPath = mStoreDirectory + "/" + path;
		std::async([this, fullPath, &stream, &progress]() -> void {
			try {
				int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
				if(fd < 0) {
					throw FileIOException(fullPath + ": " + strerror(errno));
				}

				ScopedExit onExit([fd] { close(fd); });

				int n;
				char buffer[4096];
				while((n = stream.read(buffer, sizeof(buffer))) >= 0) {
					if(::write(fd, buffer, n) < 0) {
						switch(errno) {
							case EDQUOT: throw DiskQuotaExceededException(fullPath + ": " + strerror(errno)); break;
							default: throw FileIOException(fullPath + ": " + strerror(errno)); break;
						}
					}
				}
				
				progress->setReady(true);
			} catch(const std::exception& e) {
				progress->setError(e.what());
			}
		});
		
		return AsyncProgress<bool>(progress);
	}
	
	AsyncProgress<bool> FileDataStore::list(const char *path, void (*listCallback)(const char *, void *), void *userData)
	{
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();

		DIR *dirp;
		struct dirent *dp;
		dirp = opendir(mStoreDirectory.c_str());
		while((dp = readdir(dirp)) != NULL) {
			if(strcmp(dp->d_name,".") && strcmp(dp->d_name,"..")) {
				listCallback(dp->d_name, userData);
			}
		}
		closedir(dirp);
		
		listCallback(nullptr, userData);
		
		progress->setReady(true);
		return AsyncProgress<bool>(progress);
	}
	
	AsyncProgress<bool> FileDataStore::unlink(const char *path)
	{
		std::shared_ptr<AsyncProgressData<bool>> progress = std::make_shared<AsyncProgressData<bool>>();
		
		std::string fullPath = mStoreDirectory + "/" + path;
		if(::unlink(fullPath.c_str()) != 0) {
			switch(errno) {
				case ENOENT: {
					progress->setReady(false);
					return AsyncProgress<bool>(progress);
				}
				default: throw FileIOException(fullPath + ": " + strerror(errno));
			}
		}
		
		progress->setReady(true);
		return AsyncProgress<bool>(progress);
	}
}
