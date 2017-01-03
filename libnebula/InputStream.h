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

#include <stddef.h>

namespace Nebula
{
	class OutputStream;

	/**
	 * Inteface for an input stream
	 */
	class InputStream
	{
	public:
		InputStream() = default;
		~InputStream();

		/**
		 * Reads @a size bytes from the input stream.
		 * The method will wait for all bytes to be available before returning
		 * and returning short amount of bytes once EOF has been reached.
		 * Returns the number of bytes read or 0 on EOF.
		 */
		virtual size_t read(void *data, size_t size) = 0;

		/**
		 * Closes the input stream.
		 * Calling close() manually is generally not required.
		 */
		virtual void close();
		
		/**
		 * Copies the content of the input stream to the output stream
		 */
		void copyTo(OutputStream& outStream);
	};
}
