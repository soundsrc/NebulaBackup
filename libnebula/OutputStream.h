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
	/**
	 * Output stream interface
	 */
	class OutputStream
	{
	public:
		OutputStream() = default;
		~OutputStream();

		/**
		 * Writes @a size bytes to the output stream.
		 * If not all bytes could be written an exception is thrown.
		 */
		virtual void write(const void *data, size_t size) = 0;
		
		/**
		 * Flushes unwritten bytes to the output stream
		 */
		virtual void flush();
		
		/**
		 * Closes the output stream.
		 * Manually calling close() is generally not required, but in certain
		 * cases like encryption and hashing, close() will finalize the stream
		 * for immediate processing.
		 * Output cannot be written to after close().
		 */
		virtual void close();
		
		/**
		 * Write typed data in native-endian
		 */
		template<typename T>
		void writeType(const T& v) {
			this->write(&v, sizeof(T));
		}
	};
}
