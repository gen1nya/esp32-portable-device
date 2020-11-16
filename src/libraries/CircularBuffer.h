// CircularBuffer.h - Circular buffer library for Arduino
// Copyright 2015-2016 abouvier <abouvier@student.42.fr>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H

#include <stddef.h>

template <typename T, size_t Size>
class CircularBuffer
{
public:
	typedef T value_type;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef value_type *pointer;
	typedef size_t size_type;

private:
	value_type m_buffer[Size];
	pointer m_head;
	pointer m_end;
	pointer m_tail;
	size_type m_size;

public:
	CircularBuffer() :
		m_head(m_buffer),
		m_end(m_buffer + Size),
		m_tail(m_end - 1),
		m_size(0)
	{
	}

	CircularBuffer(const CircularBuffer<T, Size> &that) : CircularBuffer()
	{
		*this = that;
	}

	template <typename U, size_type ThatSize>
	CircularBuffer(const CircularBuffer<U, ThatSize> &that) : CircularBuffer()
	{
		*this = that;
	}

	CircularBuffer &operator=(const CircularBuffer<T, Size> &that)
	{
		if (&that != this)
		{
			clear();
			while (size() < that.size())
				push_back(that[size()]);
		}
		return *this;
	}

	template <typename U, size_type ThatSize>
	CircularBuffer &operator=(const CircularBuffer<U, ThatSize> &that)
	{
		clear();
		for (size_type i = that.size() - min(that.size(), capacity());
			i < that.size(); i++)
			push_back(that[i]);
		return *this;
	}

	reference operator[](size_type offset)
	{
		return const_cast<reference>(
			const_cast<const CircularBuffer &>(*this)[offset]
		);
	}

	const_reference operator[](size_type offset) const
	{
		if (offset >= static_cast<size_type>(m_end - m_head))
			offset -= capacity();
		return m_head[offset];
	}

	void push_back(const_reference value)
	{
		if (full())
			pop_front();
		if (++m_tail == m_end)
			m_tail = m_buffer;
		*m_tail = value;
		m_size++;
	}

	void push_front(const_reference value)
	{
		if (full())
			pop_back();
		if (m_head-- == m_buffer)
			m_head += capacity();
		*m_head = value;
		m_size++;
	}

	void pop_back()
	{
		if (m_tail-- == m_buffer)
			m_tail += capacity();
		m_size--;
	}

	void pop_front()
	{
		if (++m_head == m_end)
			m_head = m_buffer;
		m_size--;
	}

	reference back()
	{
		return *m_tail;
	}

	const_reference back() const
	{
		return *m_tail;
	}

	reference front()
	{
		return *m_head;
	}

	const_reference front() const
	{
		return *m_head;
	}

	void clear()
	{
		m_head = m_buffer;
		m_tail = m_end - 1;
		m_size = 0;
	}

	size_type capacity() const
	{
		return Size;
	}

	size_type size() const
	{
		return m_size;
	}

	bool empty() const
	{
		return !size();
	}

	bool full() const
	{
		return size() == capacity();
	}
};

#endif
