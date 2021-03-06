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

#pragma once

#include <string>
#include <stdio.h>
#include "InputStream.h"
#include "OutputStream.h"

namespace Nebula
{
	enum class FileMode
	{
		Read,
		Write,
		ReadWrite
	};
	class FileStream : public InputStream, public OutputStream
	{
	public:
		FileStream();
		FileStream(const char *path, FileMode mode);
		~FileStream();
		
		bool open(const char *path, FileMode mode);
		virtual long size() const override;
		
		const std::string& path() const { return mPath; }

		virtual bool canRewind() const override;
		virtual void rewind() override;
		long seek(long offset);
		virtual size_t read(void *data, size_t size) override;
		virtual size_t skip(size_t size) override;
		virtual void write(const void *data, size_t size) override;
		virtual void flush() override;
		virtual void close() override;
	private:
		FILE *mFp;
		std::string mPath;
	};
}
