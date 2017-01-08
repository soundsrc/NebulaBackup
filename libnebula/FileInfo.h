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

#pragma once

#include <string>
#include <time.h>

namespace Nebula
{
	enum class FileType
	{
		FileNotFound,
		RegularFile,
		Directory,
		CharacterDevice,
		BlockDevice,
		Fifo,
		SymbolicLink,
		Socket
	};

	class FileInfo
	{
	public:
		explicit FileInfo(const char *path);
		
		bool exists() const { return mType != FileType::FileNotFound; }

		FileType type() const { return mType; }

		unsigned int mode() const { return mMode; }
		unsigned int uid() const { return mUid; }
		unsigned int gid() const { return mGid; }
		
		std::string userName() const { return mUserName; }
		std::string groupName() const { return mGroupName; }
		
		size_t length() const { return mLength; }
		time_t lastModifyTime() const { return mLastModifyTime; }
	private:
		FileType mType;

		unsigned int mMode;
		unsigned int mUid;
		unsigned int mGid;
		
		std::string mUserName;
		std::string mGroupName;
		
		size_t mLength;
		
		time_t mLastModifyTime;
	};
}
