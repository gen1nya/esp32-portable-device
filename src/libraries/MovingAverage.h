// MovingAverage.h - Moving average library for Arduino
// Copyright 2015 abouvier <abouvier@student.42.fr>
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

#ifndef MOVINGAVERAGE_H
#define MOVINGAVERAGE_H

#include <libraries/CircularBuffer.h>

template <typename T, size_t Size>
class MovingAverage
{
public:
	typedef CircularBuffer<T, Size> container_type;
	typedef typename container_type::reference reference;
	typedef typename container_type::const_reference const_reference;
	typedef typename container_type::value_type value_type;
	typedef typename container_type::size_type size_type;

private:
	container_type m_buffer;
	value_type m_sum;

public:
	MovingAverage() : m_sum(0)
	{
	}

	void push(const_reference value)
	{
		if (m_buffer.full())
			m_sum -= m_buffer.front();
		m_buffer.push_back(value);
		m_sum += value;
	}

	reference back()
	{
		return m_buffer.back();
	}

	const_reference back() const
	{
		return m_buffer.back();
	}

	size_type capacity() const
	{
		return m_buffer.capacity();
	}

	size_type size() const
	{
		return m_buffer.size();
	}

	bool empty() const
	{
		return m_buffer.empty();
	}

	bool full() const
	{
		return m_buffer.full();
	}

	value_type sum() const
	{
		return m_sum;
	}

	value_type average() const
	{
		return empty() ? 0 : sum() / size();
	}
};

#endif
