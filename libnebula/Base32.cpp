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
#include <ctype.h>
#include "Exception.h"
#include "Base32.h"

namespace Nebula
{
	static const char base32hex[33] = "abcdefghijklmnopqrstuvwxyz234567";
	
	static int hex32ToInt(char c)
	{
		c = tolower(c);
		if(c >= 'a' && c <= 'z') {
			return c - 'a';
		}
		
		if(c >= '2' && c <= '7') {
			return c - '2' + 26;
		}
		
		throw InvalidArgumentException("Invalid hex32 character.");
		return 0;
	}

	void base32encode(const uint8_t *data, size_t len, char *outString, size_t outLen)
	{
		unsigned int value = 0;
		int validBits = 0;
		int i;
		
		for(i = 0; i < len; ++i) {
			value <<= 8;
			value |= data[i];
			validBits += 8;
			
			while(validBits >= 5) {
				unsigned int shift = (validBits - 5);
				unsigned int idx = (value >> shift) & 0x1F;
				
				if(outLen <= 0) {
					throw InsufficientBufferSizeException("Not enough buffer size to encode output string.");
				}

				*outString++ = base32hex[idx];
				outLen--;
				
				value &= ((1 << shift) - 1);
				validBits -= 5;
			}
		}
		
		// handle the remainder
		if(validBits > 0) {
			if(outLen <= 0) {
				throw InsufficientBufferSizeException("Not enough buffer size to encode output string.");
			}

			value = (value << (5 - validBits)) & 0x1F;
			*outString++ = base32hex[value];
			--outLen;
		}
		
		if(outLen <= 0) {
			throw InsufficientBufferSizeException("Not enough buffer size to encode output string.");
		}
		*outString++ = 0;
	}
	
	size_t base32decode(const char *base32string, size_t inLen, uint8_t *data, size_t outLen)
	{
		size_t outBytes = 0;
		int extra = 0;
		
		switch(inLen & 7) {
			default:
				throw InvalidArgumentException("Invalid input length size.");
				break;
			case 0: extra = 0; break;
			case 2: extra = 1; break;
			case 4: extra = 2; break;
			case 5: extra = 3; break;
			case 7: extra = 4; break;
		}
		if(outLen < (inLen >> 3) * 5 + extra) {
			throw InsufficientBufferSizeException("Output length is not long enough.");
		}
		
		int i;
		for(i = 0; i < (inLen & ~7); i += 8) {
			int a = hex32ToInt(base32string[i]);
			int b = hex32ToInt(base32string[i + 1]);
			int c = hex32ToInt(base32string[i + 2]);
			int d = hex32ToInt(base32string[i + 3]);
			int e = hex32ToInt(base32string[i + 4]);
			int f = hex32ToInt(base32string[i + 5]);
			int g = hex32ToInt(base32string[i + 6]);
			int h = hex32ToInt(base32string[i + 7]);
			
			data[0] = (a << 3) | (b >> 2);
			data[1] = ((b & 3) << 6) | (c << 1) | (d >> 4);
			data[2] = ((d & 0xF) << 4) | (e >> 1);
			data[3] = ((e & 1) << 7) | (f << 2) | (g >> 3);
			data[4] = ((g & 7) << 5) | h;
			
			data += 5;
			outBytes += 5;
		}
		
		if(i < inLen) {
			switch(inLen & 7) {
				default:
					throw InvalidArgumentException("Invalid input length size.");
					break;
				case 2:
				{
					int a = hex32ToInt(base32string[i]);
					int b = hex32ToInt(base32string[i + 1]);
					data[0] = (a << 3) | (b >> 2);
					outBytes += 1;
				}
					break;
				case 4:
				{
					int a = hex32ToInt(base32string[i]);
					int b = hex32ToInt(base32string[i + 1]);
					int c = hex32ToInt(base32string[i + 2]);
					int d = hex32ToInt(base32string[i + 3]);
					data[0] = (a << 3) | (b >> 2);
					data[1] = ((b & 3) << 6) | (c << 1) | (d >> 4);
					outBytes += 2;
				}
					break;
				case 5:
				{
					int a = hex32ToInt(base32string[i]);
					int b = hex32ToInt(base32string[i + 1]);
					int c = hex32ToInt(base32string[i + 2]);
					int d = hex32ToInt(base32string[i + 3]);
					int e = hex32ToInt(base32string[i + 4]);
					data[0] = (a << 3) | (b >> 2);
					data[1] = ((b & 3) << 6) | (c << 1) | (d >> 4);
					data[2] = ((d & 0xF) << 4) | (e >> 1);
					outBytes += 3;
				}
					break;
				case 7:
				{
					int a = hex32ToInt(base32string[i]);
					int b = hex32ToInt(base32string[i + 1]);
					int c = hex32ToInt(base32string[i + 2]);
					int d = hex32ToInt(base32string[i + 3]);
					int e = hex32ToInt(base32string[i + 4]);
					int f = hex32ToInt(base32string[i + 5]);
					int g = hex32ToInt(base32string[i + 6]);
					data[0] = (a << 3) | (b >> 2);
					data[1] = ((b & 3) << 6) | (c << 1) | (d >> 4);
					data[2] = ((d & 0xF) << 4) | (e >> 1);
					data[3] = ((e & 1) << 7) | (f << 2) | (g >> 3);
					outBytes += 4;
				}
					break;
			}
		}
		
		return outBytes;
	}
}
