/*
        Copyright 2019 Ian Tester

        This file is part of Photo Finish.

        Voxel is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        Voxel is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with Voxel.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

template <std::size_t N, typename T>
struct _vec {
  typedef T type __attribute__ ((vector_size (N * sizeof(T))));
  typedef std::conditional_t<sizeof(T) >= 8,
			     std::int64_t,
			     std::conditional_t<sizeof(T) == 4,
						std::int32_t,
						std::conditional_t<sizeof(T) == 2,
								   std::int16_t,
								   std::int8_t> > > masktype __attribute__ ((vector_size (N * sizeof(T))));

}; // class vec

template <std::size_t N, typename T>
using vec = typename _vec<N, T>::type;

template <std::size_t N, typename T>
using vecmask = typename _vec<N, T>::masktype;
