
#pragma once

#include "irrlichttypes.h"
#include "exceptions.h"
#include <vector>
#include <string>
#include <cstring>
#include <limits>

#include "tool.h" //tmp

namespace sscsm
{

/// Thrown when deserialization fails, i.e. if the other side is malicious.
class IPCSerializationError : public BaseException {
public:
	IPCSerializationError(const std::string &s): BaseException(s) {}
};

/// Makes sure that ELEM_SIZE * n does not overflow.
template <size_t ELEM_SIZE>
inline void check_container_size(size_t n)
{
	if constexpr (ELEM_SIZE <= 1)
		return;

	constexpr size_t max_n = std::numeric_limits<size_t>::max() / ELEM_SIZE;
	if (n >= max_n)
		throw IPCSerializationError("Container size too big.");
}

/** Serialization for the SSCSM IPC channel.
 *
 * Made to be:
 * * easy to use: helpers for common cases exist (TODO)
 * * lightweight: size checks only needed for dynamically sized parts (see below)
 * * secure: other side is the enlonged arm of the server
 * * used only for local IPC: no endianness conversion needed, host size_t used
 *
 * This struct needs to be specialized before use.
 *
 * A struct consists of a statically sized part and possibly multiple dynamically
 * sized parts.
 * The dynamic parts are not serialized in-place, but instead are always appended
 * to the end. This way, the static parts need no size checks.
 * A static or a dynamic part itself again consists of multiple static parts, which
 * can have their own dynamic parts, again at the current end.
 *
 * @Example:
 *
 * ```
 * struct B
 * {
 *     u8 bi;
 *     std::vector<u8> bv;
 * };
 * struct A
 * {
 *     std::vector<B> av;
 *     u8 ai;
 * };
 *
 * Value to serialize:
 * A{ .av = { B{ .bi = 1, .bv = { 10, 20 } }, B{ 2, { 210 } } }, .ai = 42 }
 *
 * Serialized:
 * {
 *     2 - av.size()
 *     42 - ai
 *     1 - av[0].bi
 *     2 - av[0].bv.size()
 *     2 - av[1].bi
 *     1 - av[1].bv.size()
 *     10 - av[0].bv[0]
 *     20 - av[0].bv[1]
 *     210 - av[1].bv[0]
 * }
 *
 * ```
 *
 */
template <typename T>
struct Serializer
{
	/// Size of the statically sized part.
	static constexpr size_t static_size = 0;

	/** Serializes a value of type T.
	 *
	 * @param val The value to serialize.
	 * @param static_offset Index into buf. Put statically sized part into here.
	 *                      buf is already big enough.
	 * @param buf The buffer to write into. Dynamically sized parts need to go
	 *            to its end.
	 */
	static void serialize(const T &val, size_t static_offset, std::vector<u8> &buf)
	{
	}

	/** Deserializes a value of type T.
	 *
	 * @param static_begin Slice of size `static_size`.
	 * @param dyn_begin In-out ptr. Current head of dynamic slice. Needs to be
	 *                  incremented if used.
	 * @param dyn_end End of the dynamic slice.
	 * @return The deserialized value;
	 */
	static T deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
	}

	static_assert(false, "Not specialized.");
};

// Primitive types

template <typename T>
struct SerializerPrimitive
{
	static constexpr size_t static_size = sizeof(T);

	static void serialize(const T &val, size_t static_offset, std::vector<u8> &buf)
	{
		u8 *static_begin = &buf[static_offset];
		*reinterpret_cast<T *>(static_begin) = val;
	}

	static T deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		return *reinterpret_cast<const T *>(static_begin);
	}
};

template <> struct Serializer<bool> : SerializerPrimitive<bool> {};

template <> struct Serializer<u8> : SerializerPrimitive<u8> {};
template <> struct Serializer<u16> : SerializerPrimitive<u16> {};
template <> struct Serializer<u32> : SerializerPrimitive<u32> {};
template <> struct Serializer<u64> : SerializerPrimitive<u64> {};

template <> struct Serializer<s8> : SerializerPrimitive<s8> {};
template <> struct Serializer<s16> : SerializerPrimitive<s16> {};
template <> struct Serializer<s32> : SerializerPrimitive<s32> {};
template <> struct Serializer<s64> : SerializerPrimitive<s64> {};

template <> struct Serializer<f32> : SerializerPrimitive<f32> {};
template <> struct Serializer<f64> : SerializerPrimitive<f64> {};

// Containers

template <typename T>
struct Serializer<std::vector<T>>
{
	static constexpr size_t static_size = Serializer<size_t>::static_size;

	static void serialize(const std::vector<T> &val, size_t static_offset, std::vector<u8> &buf)
	{
		constexpr size_t elem_size = Serializer<T>::static_size;

		size_t n = val.size();
		Serializer<size_t>::serialize(n, static_offset, buf);

		size_t old_buf_size = buf.size();
		buf.resize(old_buf_size + n * elem_size);
		for (size_t i = 0; i < n; ++i) {
			Serializer<T>::serialize(val[i], old_buf_size + i * elem_size, buf);
		}
	}

	static std::vector<T> deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		std::vector<T> ret;
		constexpr size_t elem_size = Serializer<T>::static_size;

		size_t n = Serializer<size_t>::deSerialize(static_begin, dyn_begin, dyn_end);
		check_container_size<elem_size>(n);

		const u8 *my_dyn_begin = *dyn_begin;
		const u8 *my_dyn_end = my_dyn_begin + n * elem_size;
		if (my_dyn_end > dyn_end)
			throw IPCSerializationError("Out of bounds.");
		*dyn_begin = my_dyn_end;

		ret.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			ret.push_back(Serializer<T>::deSerialize(my_dyn_begin + i * elem_size, dyn_begin, dyn_end));
		}

		return ret;
	}
};

template <>
struct Serializer<std::string>
{
	static constexpr size_t static_size = Serializer<size_t>::static_size;

	static void serialize(const std::string &val, size_t static_offset, std::vector<u8> &buf)
	{
		size_t n = val.size();
		Serializer<size_t>::serialize(n, static_offset, buf);

		if (n == 0)
			return;

		size_t old_buf_size = buf.size();
		buf.resize(old_buf_size + n);
		memcpy(&buf[old_buf_size], val.data(), n);
	}

	static std::string deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		constexpr size_t elem_size = 1;

		size_t n = Serializer<size_t>::deSerialize(static_begin, dyn_begin, dyn_end);
		check_container_size<elem_size>(n);

		if (n == 0)
			return "";

		const u8 *my_dyn_begin = *dyn_begin;
		const u8 *my_dyn_end = my_dyn_begin + n * elem_size;
		if (my_dyn_end > dyn_end)
			throw IPCSerializationError("Out of bounds.");
		*dyn_begin = my_dyn_end;

		return std::string(reinterpret_cast<const char *>(my_dyn_begin),
				reinterpret_cast<const char *>(my_dyn_end));
	}
};

// Helpers

template <typename T>
struct MemberPointerTypes
{
};

template <typename T, typename U>
struct MemberPointerTypes<T U::*>
{
	using ClassType = U;
	using MemberType = T;
};

template <typename T, typename U, T U::* v>
struct Dings
{
	using Class = U;
	using Member = T;
	static constexpr T U::* value = v;
};

template <typename T, typename U>
auto make_Dings(T U::* v)
{
}

template <auto... val>
auto asx()
{}

// template <typename T, typename U>
// make_Dings() -> Dings<T, U, v>;

// template <typename T, template<typename B> typename A>
// template <typename T, typename... MemberTs, MemberTs... MemberPtrs>
template <typename T, typename... Ms>
struct SerializerSimpleStruct
{
	static constexpr size_t static_size =
			// (... + Serializer<typename MemberPointerTypes<typename MemberTs::value_type>::MemberType>::static_size);
			(... + Serializer<typename Ms::Member>::static_size);

	static void serialize(const T &val, size_t static_offset, std::vector<u8> &buf)
	{
		(... , (
			Serializer<typename Ms::Member>::serialize(val.*Ms::value, static_offset, buf),
			static_offset += Serializer<typename Ms::Member>::static_size
		));
	}

	static T deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		T ret{};

		(... , (
			ret.*Ms::value = Serializer<typename Ms::Member>::deSerialize(static_begin, dyn_begin, dyn_end),
			static_begin += Serializer<typename Ms::Member>::static_size
		));

		return ret;
	}
};

/*
template <typename T, typename... MPs>
SerializerSimpleStruct<T, MPs...> make_SerializerSimpleStruct(MPs ...)
{
}*/

template <typename T, auto... MPs>
using MakeSerializerSimpleStruct = SerializerSimpleStruct<T, Dings<
	typename MemberPointerTypes<decltype(MPs)>::MemberType,
	typename MemberPointerTypes<decltype(MPs)>::ClassType,
	MPs
>...>;


// Tmp

template <>
struct Serializer<ToolCapabilities>
{
	static constexpr size_t static_size = 0;

	static void serialize(const ToolCapabilities &val, size_t static_offset, std::vector<u8> &buf)
	{
	}

	static ToolCapabilities deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		ToolCapabilities ret{};

		const u8 *elem_static_begin = static_begin;

		ret.full_punch_interval = Serializer<float>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<float>::static_size;

		ret.max_drop_level = Serializer<int>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<int>::static_size;

		ret.punch_attack_uses = Serializer<int>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<int>::static_size;

		return ret;
	}
};

template <>
struct Serializer<DigParams>
{
	static constexpr size_t static_size = 0
			+ Serializer<bool>::static_size
			+ Serializer<float>::static_size
			+ Serializer<u32>::static_size
			+ Serializer<std::string>::static_size;

	static void serialize(const DigParams &val, size_t static_offset, std::vector<u8> &buf)
	{
		size_t elem_static_offset = static_offset;

		Serializer<bool>::serialize(val.diggable, elem_static_offset, buf);
		elem_static_offset += Serializer<bool>::static_size;

		Serializer<float>::serialize(val.time, elem_static_offset, buf);
		elem_static_offset += Serializer<float>::static_size;

		Serializer<u32>::serialize(val.wear, elem_static_offset, buf);
		elem_static_offset += Serializer<u32>::static_size;

		Serializer<std::string>::serialize(val.main_group, elem_static_offset, buf);
		elem_static_offset += Serializer<std::string>::static_size;
	}

	static DigParams deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		DigParams ret{};

		const u8 *elem_static_begin = static_begin;

		ret.diggable = Serializer<bool>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<bool>::static_size;

		ret.time = Serializer<float>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<float>::static_size;

		ret.wear = Serializer<u32>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<u32>::static_size;

		ret.main_group = Serializer<std::string>::deSerialize(elem_static_begin, dyn_begin, dyn_end);
		elem_static_begin += Serializer<std::string>::static_size;

		return ret;
	}
};

struct DigParams2
{
	bool diggable;
	// Digging time in seconds
	float time;
	// Caused wear
	u32 wear; // u32 because wear could be 65536 (single-use tool)
	std::string main_group;

	DigParams2(bool a_diggable = false, float a_time = 0.0f, u32 a_wear = 0,
			const std::string &a_main_group = ""):
		diggable(a_diggable),
		time(a_time),
		wear(a_wear),
		main_group(a_main_group)
	{}
};

/*
template <>
struct Serializer<DigParams2> : decltype(SerializerSimpleStruct<DigParams2>{
		&DigParams2::diggable,
		&DigParams2::time,
		&DigParams2::wear,
		&DigParams2::main_group})
{};
*/

/*
template <>
struct Serializer<DigParams2> : SerializerSimpleStruct<DigParams2,
		Dings<bool, DigParams2, &DigParams2::diggable>,
		Dings<float, DigParams2, &DigParams2::time>,
		Dings<u32, DigParams2, &DigParams2::wear>,
		Dings<std::string, DigParams2, &DigParams2::main_group>
	>
{};
*/

template <>
struct Serializer<DigParams2> : MakeSerializerSimpleStruct<DigParams2,
		&DigParams2::diggable,
		&DigParams2::time,
		&DigParams2::wear,
		&DigParams2::main_group
	>
{};

}
