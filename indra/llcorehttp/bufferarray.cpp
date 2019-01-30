// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/**
 * @file bufferarray.cpp
 * @brief Implements the BufferArray scatter/gather buffer
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "bufferarray.h"
#include "llexception.h"
#include "llmemory.h"


// BufferArray is a list of chunks, each a BufferArray::Block, of contiguous
// data presented as a single array.  Chunks are at least BufferArray::BLOCK_ALLOC_SIZE
// in length and can be larger.  Any chunk may be partially filled or even
// empty.
//
// The BufferArray itself is sharable as a RefCounted entity.  As shared
// reads don't work with the concept of a current position/seek value,
// none is kept with the object.  Instead, the read and write operations
// all take position arguments.  Single write/shared read isn't supported
// directly and any such attempts have to be serialized outside of this
// implementation.

namespace LLCore
{


// ==================================
// BufferArray::Block Declaration
// ==================================
template <std::size_t _size>
struct BufferArray::MemoryBlock
{
	typedef char			value_type;
	typedef char*			pointer;
	typedef const char*		const_pointer;
	typedef char*			iterator;
	typedef const char*		const_iterator;
	typedef char&			reference;
	typedef const char&		const_reference;
	typedef std::size_t		size_type;
	typedef std::ptrdiff_t	difference_type;
	//typedef std::reverse_iterator<iterator> reverse_iterator;
	//typedef std::reverse_iterator<const_iterator> const_reverse_interator;

	MemoryBlock() = default;
	~MemoryBlock() = default;

	MemoryBlock(const_pointer data, size_type length) {
		if (length > _size) {
			throw std::out_of_range("initialized with a length past container size");
		}

		memcpy(&(_data[0]), data, length);
		_pos = length;
	}

	MemoryBlock(const MemoryBlock<_size> &&other) noexcept {
		memcpy(&(_data[0]), &(other._data[0]), other._pos);
		_pos = other._pos;
	}

	MemoryBlock(const MemoryBlock<_size> &) = delete;
	MemoryBlock<_size>& operator=(const MemoryBlock<_size> &) = delete;

	void append(const_pointer data, size_type length) {
		if (_pos + length > _size) {
			throw std::out_of_range("append would go past array");
		}

		memcpy(&(_data[_pos]), data, length);
		_pos += length;
	}

	iterator begin() { return iterator(std::addressof(_data[0])); }
	const_iterator begin() const { return const_iterator(std::addressof(_data[0])); }

	pointer end() { return iterator(std::addressof(_data[_size])); }
	const_iterator end() const { return const_iterator(std::addressof(_data[_size])); }

	iterator data_end() { return iterator(std::addressof(_data[_pos])); }
	const_iterator data_end() const { return const_iterator(std::addressof(_data[_pos])); }

	reference operator[](size_type n) { return _data[n]; }
	const_reference operator[](size_type n) const { return _data[n]; }

	pointer data() { return std::addressof(_data[0]); }
	const_pointer data() const { return std::addressof(_data[0]); }

	size_type size() const { return _size; }

	size_type pos() { return _pos; }

	bool space_available() { return _pos < _size; }
private:
	char _data[_size ? _size : 1];
	size_t _pos;
};


// ==================================
// BufferArray Definitions
// ==================================

BufferArray::BufferArray()
	: LLCoreInt::RefCounted(true),
	  mLen(0)
{}


BufferArray::~BufferArray()
{
	mBlocks.clear();
}


size_t BufferArray::append(const void * src, size_t len)
{
	size_t result = 0;
	const char * c_src = static_cast<const char *>(src);
					  
	// check last block first
	if (len && !mBlocks.empty())
	{
		block_t& last = mBlocks.back();
		if (last.space_available())
		{
			const size_t copy_len = std::min(len, (last.size() - last.pos()));

			last.append(c_src, copy_len);
			mLen += copy_len;
			result += copy_len;

			c_src += copy_len;
			len -= copy_len;
		}
	}

	while (len > 0)
	{
		const size_t copy_len((std::min)(len, BLOCK_ALLOC_SIZE));

        try
        {
			if (mBlocks.size() >= mBlocks.capacity())
			{
				mBlocks.reserve(mBlocks.size() + 5);
			}
        }
        catch (const std::bad_alloc&)
        {
            LLMemory::logMemoryInfo(TRUE);

            //output possible call stacks to log file.
            LLError::LLCallStacks::print();

            LL_WARNS() << "Bad memory allocation in thrown by mBlock.reserve!" << LL_ENDL;
            break;
        }
		
		mBlocks.emplace_back(c_src, copy_len);
		mLen += copy_len;
		result += copy_len;

		c_src += copy_len;
		len -= copy_len;
	}

	return result;
}


size_t BufferArray::read(size_t pos, void * dst, size_t len)
{
	char *c_dst = static_cast<char *>(dst);
	if (pos >= mLen || len == 0)
		return 0;

	// limit the length
	len = std::min(len, mLen - pos);
	
	size_t result = 0, offset = 0;
	const int block_limit(mBlocks.size());

	for (int i = findBlock(pos, &offset); len && i < block_limit; ++i)
	{
		block_t& block = mBlocks[i];

		size_t block_limit_offset = block.pos() - offset;
		size_t block_len(std::min(block_limit_offset, len));

		memcpy(c_dst, &block[offset], block_len);
		result += block_len;

		c_dst += block_len;
		len -= block_len;

		offset = 0;
	}

	return result;
}


size_t BufferArray::write(size_t pos, const void * src, size_t len)
{
	const char *c_src = static_cast<const char *>(src);
	if (pos > mLen || 0 == len)
		return 0;
	
	size_t result = 0, offset = 0;
	const size_t block_limit = mBlocks.size();

	for (int i = findBlock(pos, &offset); len && i < block_limit; ++i)
	{
		block_t& block = mBlocks[i];
		size_t block_limit_offset = block.pos() - offset;
		size_t block_len = std::min(block_limit_offset, len);

		block.append(c_src, block_len);
		mLen += block_len;
		result += block_len;

		c_src += block_len;
		len -= block_len;

		offset = 0;
	}
	
	if (len)
	{
		// Some or all of the remaining write data will
		// be an append.
		result += append(c_src, len);
	}

	return result;
}
		

int BufferArray::findBlock(size_t pos, size_t * ret_offset)
{
	*ret_offset = 0;
	if (pos >= mLen)
		return -1;

	const int block_n = pos / BLOCK_ALLOC_SIZE;
	if (block_n < mBlocks.size())
	{
		*ret_offset = pos % BLOCK_ALLOC_SIZE;
		return block_n;
	}

	return -1;
}


bool BufferArray::getBlockStartEnd(int block, const char ** start, const char ** end)
{
	if (block < 0 || block >= mBlocks.size())
		return false;

	const block_t& b(mBlocks[block]);
	*start = b.begin();
	*end = b.data_end();
	return true;
}
	

}  // end namespace LLCore
