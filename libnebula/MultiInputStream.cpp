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
#include "MultiInputStream.h"

namespace Nebula
{
	MultiInputStream::MultiInputStream(const std::vector<InputStream *>& l)
	: mStreamList(l)
	, mCurStream(mStreamList.begin())
	{
		
	}
	
	MultiInputStream::MultiInputStream(std::initializer_list<InputStream *> l)
	: mStreamList(l)
	, mCurStream(mStreamList.begin())
	{
	}
	
	size_t MultiInputStream::read(void *data, size_t size)
	{
		if(mCurStream == mStreamList.end()) return 0;
		
		size_t bytesRead = 0;
		char *pData = (char *)data;
		while(bytesRead < size) {
			size_t n;
			n = (*mCurStream)->read(pData, size - bytesRead);
			if(n == 0) {
				++mCurStream;
				if(mCurStream == mStreamList.end()) break;
			}
			bytesRead += n;
		}
		
		return bytesRead;
	}
}
