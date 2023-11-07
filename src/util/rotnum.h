/*
Minetest
Copyright (C) 2023 DS

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <array>
#include <ostream>
#include <string_view>

// default-initialized DirNum is invalid
// value-initialized DirNum is +x-axis
// valid DirNums are 0 to 5
struct DirNum
{
	u8 num;

	// the axes
	// use operator- to get the negative directions, e.g. -DirNum::x
	static const DirNum x; //TODO: functions?
	static const DirNum y;
	static const DirNum z;

	// for iterating (TODO: remove?)
	static constexpr DirNum begin() { return {0}; };
	static constexpr DirNum end() { return {6}; };

	// alternative for iterating
	static constexpr std::array<DirNum, 6> allDirs()
	{
		return {{{0}, {1}, {2}, {3}, {4}, {5}}};
	}

	constexpr bool operator==(const DirNum &other) const
	{
		return num == other.num;
	}

	constexpr bool operator!=(const DirNum &other) const
	{
		return !(*this == other);
	}

	// mirror
	constexpr DirNum operator-() const
	{
		return {(u8)(num ^ 1)};
	}

	// for iterating (TODO: remove?)
	constexpr DirNum &operator++()
	{
		++num;
		return *this;
	}

	// for iterating (TODO: remove?)
	constexpr DirNum operator++(int)
	{
		DirNum ret = *this;
		++*this;
		return ret;
	}

	constexpr std::string_view humanReadable() const
	{
		return human_readable_names[num];
	}

private:
	static constexpr char human_readable_names[6][3] = {"+x", "-x", "+y", "-y", "+z", "-z"};
};

// TODO: remove comment?, and
// can't be defined in struct definition, because DirNum is an incomplete type there
// see also https://stackoverflow.com/questions/67300293/can-i-initialize-a-constexpr-static-member-outside-the-class
inline constexpr DirNum DirNum::x = {0};
inline constexpr DirNum DirNum::y = {2};
inline constexpr DirNum DirNum::z = {4};
inline constexpr DirNum begin = {0};
inline constexpr DirNum end = {6};


std::ostream &operator<<(std::ostream &os, DirNum dn)
{
	return os << dn.humanReadable();
}


/*
// default-initialized RotNum is invalid
// value-initialized RotNum sets every component to +x
struct RotNum
{
	u16 num;

	// identity
	static constexpr RotNum id = {0x210};

	// positive 90-degree rotation around axis
	static constexpr RotNum rotx = {0x160};
	static constexpr RotNum roty = {0x0}; //TODO
	static constexpr RotNum rotz = {0x0};

	// mirror along an axis
	static constexpr RotNum mirx = {0x214};
	static constexpr RotNum miry = {0x250};
	static constexpr RotNum mirz = {0x610};

	constexpr RotNum operator*(RotNum other) const noexcept
	{
		u16 signs = (num & 0x444) ^ (other.num & 0x444); //FIXME
		u16 x = ((other.num >> (((num & 0x003) >> 0) * 4)) & 0x003) << 0;
		u16 y = ((other.num >> (((num & 0x030) >> 4) * 4)) & 0x003) << 4;
		u16 z = ((other.num >> (((num & 0x300) >> 8) * 4)) & 0x003) << 8;
		return {signs | x | y | z};
	}

	template <typename T>
	constexpr vector3d<T> operator*(vector3d<T> v) const noexcept
	{
		auto get_comp = [&](u8 n) {
			static constexpr vector3d<T>::*T xyz[3] =
					{vector3d<T>::X, vector3d<T>::Y, vector3d<T>::Z};
			T ret = v.*xyz[n & 0x3];
			if (n & 0x4)
				ret = -ret;
			return ret;
		};
		return vector3d<T>(
			get_comp(num & 0x007),
			get_comp((num & 0x070) >> 4),
			get_comp((num & 0x700) >> 8)
		);
	}

	constexpr RotNum operator^(int exp) const noexcept
	{
		if (exp < 0)
			return this->inverse()^(-exp);
		else if (exp == 0)
			return RotNum::id;
		else
			return (*this) * (*this)^(exp-1)
	}

	// rotate a single dir
	// (to find out where a component came from: rn^-1 * d)
	constexpr DirNum operator*(DirNum d) const noexcept
	{
		u8 ret = num >> ((d.num >> 1) * 4)) & 0x7
		ret ^= (d.num & 1) << 2;
		return {((ret & 6) >> 1) | ((ret & 1) << 2)};
	}
};
*/
