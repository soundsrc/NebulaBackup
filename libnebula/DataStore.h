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

namespace Nebula
{
	class OutputStream;
	class InputStream;

	/**
	 * Interface for data storage systems.
	 * Supports get, put, list and unlink.
	 */
	class DataStore
	{
		/**
		 * Retrives a file from the data store for the given path, and writes
		 * the result to the output stream
		 * @param path Path to object to retrieve
		 * @param stream Output stream
		 * @returns False if the object was not found at the path, true otherwise
		 */
		virtual bool get(const char *path, OutputStream& stream) = 0;
		
		/**
		 * Writes a file to the data store given the supplied data stream
		 */
		virtual void put(const char *path, InputStream& stream) = 0;
		
		/**
		 * Lists the files at the given path, recursively. The callback is
		 * invoked for each file listed and invoked with nullptr when no more
		 * paths are to be listed.
		 */
		virtual void list(const char *path, void (*listCallback)(const char *, void *), void *userData = nullptr) = 0;
		 
		/**
		 * Removes a file from the data store.
		 */
		virtual bool unlink(const char *path) = 0;
	};
}
