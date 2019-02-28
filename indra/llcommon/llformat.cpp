// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/** 
 * @file llformat.cpp
 * @date   January 2007
 * @brief string formatting utility
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llformat.h"

#include <boost/align/aligned_allocator.hpp>

// common used function with va_list argument
// wrapper for vsnprintf to be called from llformatXXX functions.
static void va_format(std::string& out, const char *fmt, va_list& va)
{
	typedef typename std::vector<char, boost::alignment::aligned_allocator<char, 1>> vec_t;
	static thread_local vec_t charvector(1024); // Evolves into charveleon
	#define vsnprintf(va) std::vsnprintf(charvector.data(), charvector.capacity(), fmt, va)
#ifdef LL_WINDOWS // We don't have to copy on windows
	#define va2 va
#else
	va_list va2;
	va_copy(va2, va);
#endif
	const auto smallsize(charvector.capacity());
	const auto size = vsnprintf(va);
	if (size < 0)
	{
		LL_ERRS() << "Encoding failed, code " << size << ". String hint: " << out << '/' << fmt << LL_ENDL;
	}
	else if (static_cast<vec_t::size_type>(size) >= smallsize) // Resize if we need more space
	{
		charvector.resize(1+size); // Use the String Stone
		vsnprintf(va2);
	}
#ifndef LL_WINDOWS
	va_end(va2);
#endif
	out.assign(charvector.data());
}

std::string llformat(const char *fmt, ...)
{
	std::string res;
	va_list va;
	va_start(va, fmt);
	va_format(res, fmt, va);
	va_end(va);
	return res;
}

std::string llformat_to_utf8(const char *fmt, ...)
{
	std::string res;
	va_list va;
	va_start(va, fmt);
	va_format(res, fmt, va);
	va_end(va);

#if LL_WINDOWS
	// made converting to utf8. See EXT-8318.
	res = ll_convert_string_to_utf8_string(res);
#endif
	return res;
}
