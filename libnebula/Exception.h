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

#include <stdexcept>
#include <string>

namespace Nebula
{
#define DEFINE_EXCEPTION(Exception) \
class Exception : public std::runtime_error \
{ \
public: \
	Exception(const std::string& err) : std::runtime_error(err) { } \
}
	
	DEFINE_EXCEPTION(FileIOException);
	DEFINE_EXCEPTION(DiskQuotaExceededException);
	DEFINE_EXCEPTION(InvalidArgumentException);
	DEFINE_EXCEPTION(RepositoryLockedException);
	DEFINE_EXCEPTION(InvalidRepositoryException);
	DEFINE_EXCEPTION(InvalidDataException);
	DEFINE_EXCEPTION(EncryptionFailedException);
	DEFINE_EXCEPTION(InsufficientBufferSizeException);
	DEFINE_EXCEPTION(LZMAException);
	
#undef DEFINE_EXCEPTION

}
