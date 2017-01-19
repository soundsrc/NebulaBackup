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
#include "LZMAUtils.h"
#include <stdlib.h>
#include <memory>
#include <type_traits>
#include "Lzma2Enc.h"
#include "Lzma2Dec.h"
#include "Exception.h"
#include "InputStream.h"
#include "OutputStream.h"

namespace Nebula
{
	void *LzmaAlloc::lzmaAlloc(void *p, size_t size)
	{
		return malloc(size);
	}
	
	void LzmaAlloc::lzmaFree(void *p, void *address)
	{
		return free(address);
	}
	
	size_t LzmaOutputStream::lzmaWrite(void *p, const void *buf, size_t size)
	{
		LzmaOutputStream *stream = (LzmaOutputStream *)p;
		return stream->write(buf, size);
	}
	
	size_t LzmaOutputStream::write(const void *buf, size_t size)
	{
		mStream.write(buf, size);
		return size;
	}
	
	SRes LzmaInputStream::lzmaRead(void *p, void *buf, size_t *size)
	{
		LzmaInputStream *stream = (LzmaInputStream *)p;
		return stream->read(buf, size);
	}
	
	SRes LzmaInputStream::read(void *buf, size_t *size)
	{
		*size = mStream.read(buf, *size);
		return SZ_OK;
	}

	SRes LZMAProgress::lzmaProgress(void *p, UInt64 inSize, UInt64 outSize)
	{
		LZMAProgress *progress = (LZMAProgress *)p;
		return progress->progress(inSize, outSize);
	}
	
	SRes LZMAProgress::progress(UInt64 inSize, UInt64 outSize)
	{
		if(mProgress) {
			mProgress(inSize, outSize);
		}
		return SZ_OK;
	}
	
	namespace LZMAUtils
	{
		void compress(InputStream& inStream, OutputStream& outStream, std::function<void (uint64_t, uint64_t)> progress)
		{
			LzmaAlloc alloc;
			
			std::unique_ptr<std::remove_pointer<CLzma2EncHandle>::type, decltype(Lzma2Enc_Destroy) *>
				enc( Lzma2Enc_Create(&alloc, &alloc), Lzma2Enc_Destroy );

			CLzma2EncProps props;
			Lzma2EncProps_Init(&props);
			props.lzmaProps.writeEndMark = 1;
			props.lzmaProps.level = 9;
			
			if(Lzma2Enc_SetProps(enc.get(), &props) != SZ_OK) {
				throw LZMAException("Failed to set ZLMA2 prop.");
			}

			Byte prop = Lzma2Enc_WriteProperties(enc.get());
			outStream.write(&prop, sizeof(prop));
			
			LzmaOutputStream out(outStream);
			LzmaInputStream in(inStream);
			LZMAProgress prog(progress);

			if(Lzma2Enc_Encode(enc.get(), &out, &in, &prog) != SZ_OK) {
				throw LZMAException("Failed to compress.");
			}
		}
	}
}
