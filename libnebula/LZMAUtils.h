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

#include <functional>
#include <stdint.h>
#include "7zTypes.h"

namespace Nebula
{
	class InputStream;
	class OutputStream;
	
	class LzmaAlloc : public ISzAlloc
	{
	public:
		LzmaAlloc()
		{
			this->Alloc = lzmaAlloc;
			this->Free = lzmaFree;
		}
		
		LzmaAlloc(void *(*alloc)(void *, size_t), void (*free)(void *, void *))
		{
			this->Alloc = alloc;
			this->Free = free;
		}
	private:
		static void *lzmaAlloc(void *p, size_t size);
		static void lzmaFree(void *p, void *address);
	};
	
	class LzmaOutputStream : public ISeqOutStream
	{
	public:
		LzmaOutputStream(OutputStream& stream) : mStream(stream) {
			this->Write = lzmaWrite;
		}
		
		size_t write(const void *buf, size_t size);
	private:
		OutputStream& mStream;
		static size_t lzmaWrite(void *p, const void *buf, size_t size);
		
	};
	
	class LzmaInputStream : public ISeqInStream
	{
	public:
		LzmaInputStream(InputStream& stream) : mStream(stream) {
			this->Read = lzmaRead;
		}
		
		SRes read(void *buf, size_t *size);
	private:
		InputStream& mStream;
		static SRes lzmaRead(void *p, void *buf, size_t *size);
		
	};
	
	class LZMAProgress : public ICompressProgress
	{
	public:
		typedef std::function<void (uint64_t, uint64_t)> ProgressFunc;
		
		LZMAProgress(ProgressFunc func)
		: mProgress(func)
		{
			this->Progress = lzmaProgress;
		}
		SRes progress(UInt64 inSize, UInt64 outSize);
	private:
		ProgressFunc mProgress;
		static SRes lzmaProgress(void *p, UInt64 inSize, UInt64 outSize);
	};
	
	namespace LZMAUtils
	{
		void compress(InputStream& inStream, OutputStream& outStream, std::function<void (uint64_t, uint64_t)> progress);
	}
}
