/*
 * Copyright (c) 2017 Sound <sound@sagaforce.com>
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
#include "FileInfo.h"
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif
#include <boost/filesystem.hpp>
#include "Exception.h"

namespace Nebula
{
	FileInfo::FileInfo(const char *path)
	: mType(FileType::FileNotFound)
	{
#ifndef _WIN32
		/* On posix systems, use stat() to obtain file info */
		 
		struct stat st;
		if(stat(path, &st) < 0) {
			// file not found or error
			return;
		}
	
		struct passwd *pwd = getpwuid(st.st_uid);
		struct group *grp = getgrgid(st.st_gid);
		
		mUid = st.st_uid;
		if(pwd) {
			mUserName = pwd->pw_name;
		}
		
		mGid = st.st_gid;
		if(grp) {
			mGroupName = grp->gr_name;
		}
		
		mLength = st.st_size;
		mLastModifyTime = st.st_mtime;
		
		mMode = st.st_mode & 07777;
		switch(st.st_mode & S_IFMT) {
			case S_IFREG: mType = FileType::RegularFile; break;
			case S_IFDIR: mType = FileType::Directory; break;
			case S_IFLNK: mType = FileType::SymbolicLink; break;
			case S_IFBLK: mType = FileType::BlockDevice; break;
			case S_IFCHR: mType = FileType::CharacterDevice; break;
			case S_IFIFO: mType = FileType::Fifo; break;
			case S_IFSOCK: mType = FileType::Socket; break;
		}
#else
		using namespace boost;

		filesystem::file_status st = filesystem::status(path);
		switch(st.type()) {
			default:
			case filesystem::file_not_found: return;
			case filesystem::regular_file: mType = FileType::RegularFile; break;
			case filesystem::directory_file: mType = FileType::Directory; break;
			case filesystem::symlink_file: mType = FileType::SymbolicLink; break;
			case filesystem::block_file: mType = FileType::BlockDevice; break;
			case filesystem::character_file: mType = FileType::CharacterDevice; break;
			case filesystem::fifo_file: mType = FileType::Fifo; break;
			case filesystem::socket_file: mType = FileType::Socket; break;
		}
		
		mMode = st.permissions() & filesystem::perms_mask;

		// if not unix, there is no uid/gid info
		mUid = 0;
		mGid = 0;
		mUserName = "";
		mGroupName = "";
		
		mLength = filesystem::file_size(path);
		mLastModifyTime = filesystem::last_write_time(path);
#endif
	}

}
