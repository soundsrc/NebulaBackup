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
#include <stdint.h>
#include <strstream>
#include "Exception.h"
#include "OutputStream.h"
#include "InputStream.h"

namespace Nebula
{
	InputStream::~InputStream()
	{
		close();
	}

	void InputStream::close()
	{
	}
	
	bool InputStream::canRewind() const
	{
		return false;
	}
	
	void InputStream::rewind()
	{
		throw InvalidOperationException("Input stream does not support rewind().");
	}
	
	void InputStream::copyTo(OutputStream& outStream)
	{
		uint8_t buffer[16384];
		size_t n;
		
		while((n = this->read(buffer, sizeof(buffer))) > 0) {
			outStream.write(buffer, n);
		}
	}
	
	void InputStream::readExpected(void *data, size_t size)
	{
		size_t n;
		if((n = this->read(data, size)) != size) {
			std::strstream ss;
			ss << "InputStream::readExpected() attempted to read "
				<< size << " bytes but instead got " << n << " bytes.";
			throw ShortReadException(ss.str());
		}
	}
}
