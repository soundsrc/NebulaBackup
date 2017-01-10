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

#include <memory>
#include <boost/filesystem.hpp>
#include <stdint.h>
#include "OutputStream.h"

namespace Nebula
{
	class InputStream;
	class MemoryOutputStream;
	class FileStream;

	/**
	 * Temporary file stream.
	 * If small enough, the entire stream will written in memory.
	 * Otherwise, the stream will bae stored to a temp file.
	 * The temp file is removed on destruction.
	 */
	class TempFileStream : public OutputStream
	{
	public:
		TempFileStream();
		virtual ~TempFileStream();

		virtual void write(const void *data, size_t size) override;
		
		std::shared_ptr<InputStream> inputStream();
	private:
		enum { TEMP_BUFFER_SIZE = 4 * 1024 * 1024 };

		uint8_t *mBuffer;
		std::unique_ptr<MemoryOutputStream> mMemStream;
		std::unique_ptr<FileStream> mFileStream;
		boost::filesystem::path mTmpFile;
	};
}
