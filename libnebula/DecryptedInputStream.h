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

#include <stdint.h>
#include <openssl/evp.h>
#include "InputStream.h"
#include "ZeroedArray.h"

namespace Nebula
{
	class DecryptedInputStream : public InputStream
	{
	public:
		DecryptedInputStream(InputStream& stream, const EVP_CIPHER *cipher, const uint8_t *key);
		~DecryptedInputStream();
		
		virtual size_t read(void *data, size_t size) override;
	private:
		InputStream& mStream;
		EVP_CIPHER_CTX *mCtx;
		ZeroedArray<uint8_t, 4096 + EVP_MAX_BLOCK_LENGTH> mBuffer;
		const uint8_t *mBufferPtr;
		int mBytesInBuffer;
		bool mDone;
	};
}
