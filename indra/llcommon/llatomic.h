/** 
 * @file llatomic.h
 * @brief Definition of LLAtomic
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2009, Linden Research, Inc.
 * Copyright (c) 2012, Aleric Inglewood
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLATOMIC_H
#define LL_LLATOMIC_H

#include <atomic>

template<typename T>
struct impl_atomic_type { typedef std::atomic<T> type; };

template <typename Type> class LLAtomic32
{
public:
	LLAtomic32(void) { }
	LLAtomic32(LLAtomic32 const& atom) { mData = Type(atom.mData); }
	LLAtomic32(Type x) { mData = x; }
	LLAtomic32& operator=(LLAtomic32 const& atom) { mData = Type(atom.mData); return *this; }

	operator Type() const { return mData; }
	void operator=(Type x) { mData = x; }
	void operator-=(Type x) { mData -= x; }
	void operator+=(Type x) { mData += x; }
	Type operator++(int) { return mData++; } // Type++
	bool operator--() { return --mData; } // Returns (--Type != 0)

private:
	typename impl_atomic_type<Type>::type mData;
};

typedef LLAtomic32<U32> LLAtomicU32;
typedef LLAtomic32<S32> LLAtomicS32;
typedef std::atomic<bool> LLAtomicBool;

#endif
