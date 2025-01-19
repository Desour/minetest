
#pragma once

#include "irrlichttypes.h"
#include "exceptions.h"
#include <vector>
#include <string>
#include <cstring>
#include <limits>
#include <type_traits>


// TODO: move this to some util header

template <typename T>
struct MembPtrTypes
{
};

template <typename T, typename U>
struct MembPtrTypes<T U::*>
{
	using C = U; // class
	using M = T; // member
};

template <typename T>
using MembPtrM = typename MembPtrTypes<T>::M;

template <typename T>
using MembPtrC = typename MembPtrTypes<T>::C;


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
	constexpr size_t max_n = std::numeric_limits<size_t>::max() / ELEM_SIZE;
	if (n >= max_n)
		throw IPCSerializationError("Container size too big.");
}

template <>
inline void check_container_size<0>(size_t n)
{
}

/** Serialization for the SSCSM IPC channel.
 *
 * Made to be:
 * * easy to use: helpers for common cases exist (SerializerSimpleStruct)
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
	static void serialize(const T &val, size_t static_offset, std::vector<u8> &buf);

	/** Deserializes a value of type T.
	 *
	 * @param static_begin Slice of size `static_size`.
	 * @param dyn_begin In-out ptr. Current head of dynamic slice. Needs to be
	 *                  incremented if used.
	 * @param dyn_end End of the dynamic slice.
	 * @return The deserialized value;
	 */
	static T deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end);
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

// Helpers

/** Auto-generate a Serializer specialization for a struct T.
 *
 * T needs to be default constructible.
 *
 * MPs are member pointers into T. The members are serialized in the given order.
 */
template <typename T, auto... MPs>
struct SerializerSimpleStruct
{
	static_assert((... && std::is_member_pointer_v<decltype((MPs))>),
			"MPs needs to be member pointers");

	static constexpr size_t static_size =
			(... + Serializer<MembPtrM<decltype((MPs))>>::static_size);

	static void serialize(const T &val, size_t static_offset, std::vector<u8> &buf)
	{
		(... , (
			Serializer<MembPtrM<decltype((MPs))>>::serialize(val.*((decltype((MPs)))(MPs)), static_offset, buf),
			static_offset += Serializer<MembPtrM<decltype((MPs))>>::static_size
		));
	}

	static T deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		T ret{};

		(... , (
			ret.*((decltype((MPs)))(MPs)) = Serializer<MembPtrM<decltype((MPs))>>::deSerialize(static_begin, dyn_begin, dyn_end),
			static_begin += Serializer<MembPtrM<decltype((MPs))>>::static_size
		));

		return ret;
	}
};

/** Auto-generate a Serializer specialization for an enum E.
 *
 * It is serialized by static_casting to B.
 */
template <typename E, typename B>
struct SerializerEnum
{
	static constexpr size_t static_size = Serializer<B>::static_size;

	static void serialize(const E &val, size_t static_offset, std::vector<u8> &buf)
	{
		Serializer<B>::serialize(static_cast<B>(val), static_offset, buf);
	}

	static E deSerialize(const u8 *static_begin, const u8 **dyn_begin, const u8 *dyn_end)
	{
		return static_cast<E>(Serializer<B>::deSerialize(static_begin, dyn_begin, dyn_end));
	}
};

// Containers

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
		if (n * elem_size > static_cast<size_t>(dyn_end - my_dyn_begin))
			throw IPCSerializationError("Out of bounds.");
		const u8 *my_dyn_end = my_dyn_begin + n * elem_size;
		*dyn_begin = my_dyn_end;

		return std::string(reinterpret_cast<const char *>(my_dyn_begin),
				reinterpret_cast<const char *>(my_dyn_end));
	}
};

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
		buf.resize(old_buf_size + n * elem_size); // TODO: is this exact, like reserve? if yes, replace with insert()
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
		if (n * elem_size > static_cast<size_t>(dyn_end - my_dyn_begin))
			throw IPCSerializationError("Out of bounds.");
		*dyn_begin = my_dyn_begin + n * elem_size;

		ret.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			ret.push_back(Serializer<T>::deSerialize(my_dyn_begin + i * elem_size, dyn_begin, dyn_end));
		}

		return ret;
	}
};

}
