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
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

/** Helper for enumerating all 6 possible directions along axes.
 *
 * A DirNum encodes a vector.
 * Bits:
 * `0 0 0 0 0 a1 a0 s`
 * The lowest bit (`s`) encodes the sign (1 is -, 0 is +).
 * Bit 1 and 2 (`a1 a0`) encode the axis (0 is x, 1 is y, 2 is z).
 *
 * A default-initialized DirNum is invalid.
 * A value-initialized DirNum is the +x-axis.
 * Valid DirNums are 0 to 5.
 */
struct DirNum
{
	u8 num;

	/** The axes.
	 *
	 * Use operator- to get the negative directions, e.g. `-DirNum::x()`.
	 */
	static constexpr DirNum x() noexcept { return {0}; };
	static constexpr DirNum y() noexcept { return {2}; };
	static constexpr DirNum z() noexcept { return {4}; };

	//! Mirror.
	constexpr DirNum operator-() const noexcept
	{
		return {(u8)(num ^ 1)};
	}

	//! For iterating.
	static constexpr std::array<DirNum, 6> allDirs() noexcept
	{
		return {{{0}, {1}, {2}, {3}, {4}, {5}}};
	}

	constexpr bool operator==(const DirNum &other) const noexcept
	{
		return num == other.num;
	}

	constexpr bool operator!=(const DirNum &other) const noexcept
	{
		return !(*this == other);
	}

	//! +1 for +x, +y, +z, otherwise -1.
	constexpr s8 sign() const noexcept
	{
		return 1 - 2 * (num & 1);
	}

	template <typename T>
	core::vector3d<T> toVector3d() const noexcept
	{
		// FIXME: make vector3d constexpr
		static const core::vector3d<T> lut_vector3d[6] = {
			core::vector3d<T>(1,0,0), core::vector3d<T>(-1,0,0),
			core::vector3d<T>(0,1,0), core::vector3d<T>(0,-1,0),
			core::vector3d<T>(0,0,1), core::vector3d<T>(0,0,-1),
		};
		return lut_vector3d[num];
	}

	// same abbreviations as in irr_v3d.h
	v3f toV3f() const noexcept { return toVector3d<float>(); }
	v3d toV3d() const noexcept { return toVector3d<double>(); }
	v3s16 toV3s16() const noexcept { return toVector3d<s16>(); }
	v3u16 toV3u16() const noexcept { return toVector3d<u16>(); }
	v3s32 toV3s32() const noexcept { return toVector3d<s32>(); }

	/** Get the direction that a vector is pointing in.
	 *
	 * Not recommended for floating point vectors.
	 */
	template <typename T>
	static constexpr std::optional<DirNum> directionOf(const core::vector3d<T> &v) noexcept
	{
		if (v.X != 0) {
			if (v.Y != 0 || v.Z != 0)
				return std::nullopt;
			if (v.X > 0)
				return DirNum::x();
			else
				return -DirNum::x();
		} else if (v.Y != 0) {
			if (v.Z != 0)
				return std::nullopt;
			if (v.Y > 0)
				return DirNum::y();
			else
				return -DirNum::y();
		} else if (v.Z != 0) {
			if (v.Z > 0)
				return DirNum::z();
			else
				return -DirNum::z();
		} else {
			return std::nullopt;
		}
	}

	constexpr std::string_view humanReadable() const noexcept
	{
		return lut_human_readable_names[num];
	}

	/** Use this for accessing the wanted componend of a vector3d.
	 *
	 * Example:
	 * ```
	 * v3s16 v;
	 * DirNum dn;
	 * v.*dn.memberPtrInVector3d<s16>() = 42
	 * ```
	 */
	template <typename T>
	constexpr T core::vector3d<T>::* memberPtrInVector3d() const noexcept
	{
		constexpr T core::vector3d<T>::* lut_ptrs[3] =
				{&core::vector3d<T>::X, &core::vector3d<T>::Y, &core::vector3d<T>::Z};
		return lut_ptrs[num >> 1];
	}

private:
	// static variables in constexpr functions are not yet allowed, hence this is here
	static constexpr char lut_human_readable_names[6][3] = {"+x", "-x", "+y", "-y", "+z", "-z"};
};

static_assert(sizeof(DirNum) == 1);

// pretty printing
std::ostream &operator<<(std::ostream &os, DirNum dn)
{
	return os << dn.humanReadable();
}

// d + v
template <typename T>
core::vector3d<T> operator+(const DirNum &d, const core::vector3d<T> &v) noexcept
{
	auto ret = v;
	ret.*d.memberPtrInVector3d<T>() += d.sign();
	return ret;
}

// v + d
template <typename T>
core::vector3d<T> operator+(const core::vector3d<T> &v, const DirNum &d) noexcept
{
	return d + v;
}

// v - d
template <typename T>
core::vector3d<T> operator-(const core::vector3d<T> &v, const DirNum &d) noexcept
{
	return v + (-d);
}

// d * s
template <typename T>
core::vector3d<T> operator*(const DirNum &d, const T &s) noexcept
{
	return d.toVector3d<T>() * s;
}

// s * d
template <typename T>
core::vector3d<T> operator*(const T &s, const DirNum &d) noexcept
{
	return d * s;
}


// default-initialized RotNum is invalid
// value-initialized RotNum sets every component to +x
struct RotNum
{
	u16 num;

	// identity
	static constexpr RotNum id = {0x420};

	// positive 90-degree rotation around axis
	//~ static constexpr RotNum rotx = {0x250};
	static constexpr RotNum rotx = {DirNum::y(), -DirNum::z(), DirNum::x};
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

	std::string humanReadable() const noexcept
	{
		// example: "(+x/+y/+z)"
		return std::string("(") + row0().humanReadable()
				+ "/" + row1().humanReadable()
				+ "/" + row2().humanReadable() + ")";
	}
};

static_assert(sizeof(RotNum) == 2);
