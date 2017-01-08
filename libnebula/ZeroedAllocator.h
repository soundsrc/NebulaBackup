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

extern "C" {
#include "compat/string.h"
}

namespace Nebula
{
	template<typename T>
	struct ZeroedAllocator
	{
		typedef size_t     size_type;
		typedef ptrdiff_t  difference_type;
		typedef T*         pointer;
		typedef const T*   const_pointer;
		typedef T&        reference;
		typedef const T&    const_reference;
		typedef T          value_type;
		
		template<typename U>
		struct rebind
		{
			typedef ZeroedAllocator<U> other;
		};

		
		ZeroedAllocator() { }
		template <class U> ZeroedAllocator(const ZeroedAllocator<U>& other) { }

		pointer allocate(size_type n)
		{
			return (T *)::operator new (n * sizeof(T));
		}
		
		void deallocate(pointer p, size_type n)
		{
			explicit_bzero(p, n);
			::operator delete(p);
		}
	};
	
	template <class T, class U>
	inline bool operator==(const ZeroedAllocator<T>&, const ZeroedAllocator<U>&)
	{
		return true;
	}
	
	template <class T, class U>
	inline bool operator!=(const ZeroedAllocator<T>&, const ZeroedAllocator<U>&)
	{
		return false;
	}
}
