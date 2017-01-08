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

#include <functional>

namespace Nebula
{
	template<typename Func>
	class ScopedExit
	{
	public:
		ScopedExit(Func&& exitHandler)
		: mExitHandler(std::move(exitHandler))
		, mValid(true)
		{
		}

		ScopedExit(ScopedExit&& rhs)
		: mExitHandler(std::move(rhs.mExitHandler))
		, mValid(true)
		{
			rhs.mValid = false;
		}

		ScopedExit(const ScopedExit&) = delete;
		ScopedExit& operator =(const ScopedExit&) = delete;

		void release() { mValid = false; }

		inline ~ScopedExit() noexcept(noexcept(mExitHandler())) {
			if(mValid) {
				mExitHandler();
			}
		}
	private:
		Func mExitHandler;
		bool mValid;
	};
	
	template<typename Func>
	ScopedExit<Func> makeScopedExit(Func&& f)
	{
		return ScopedExit<Func>(std::forward<Func>(f));
	}

#define SCOPED_EXIT_STR_CAT(x, y) x ## y
#define SCOPED_EXIT_UNIQUE_VAR(lineNo) SCOPED_EXIT_STR_CAT(__xx_scopedExitVar_, lineNo)

#define scopedExit(lambda) auto SCOPED_EXIT_UNIQUE_VAR(__LINE__) = makeScopedExit(lambda)

}
