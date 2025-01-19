// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "catch.h"
#include "script/sscsm_serialize.h"

using namespace sscsm;

template <typename T>
static T serialize_and_deserialize(const T &val)
{
	using serializer = Serializer<T>;

	std::vector<u8> buf;
	buf.resize(serializer::static_size);
	serializer::serialize(val, 0, buf);

	const u8 *dyn_begin = buf.data() + serializer::static_size;
	const u8 *dyn_end = buf.data() + buf.size();
	T ret = serializer::deSerialize(buf.data(), &dyn_begin, dyn_end);
	CHECK(dyn_begin == dyn_end);
	CHECK(ret == val);
	return ret;
}

namespace {

struct B
{
	std::vector<u32> bv;
	s8 bi;

	bool operator==(const B &other) const
	{
		return bv == other.bv && bi == other.bi;
	}
};

struct A
{
	s16 ai;
	std::vector<B> av;
	B ab;

	bool operator==(const A &other) const
	{
		return ai == other.ai && av == other.av && ab == other.ab;
	}
};

struct C
{
	u8 c1;
	u8 c2;
	u8 c3;

	bool operator==(const C &other) const
	{
		return c1 == other.c1 && c2 == other.c2 && c3 == other.c3;
	}
};

enum E1
{
	E1_A,
	E1_B,
	E1_C,
};

enum E2 : u16
{
	E2_A,
	E2_B,
	E2_C,
};

enum class E3
{
	A,
	B,
	C,
};

}

namespace sscsm {

template <>
struct Serializer<B> : SerializerSimpleStruct<B,
		&B::bv,
		&B::bi
	>
{};

template <>
struct Serializer<A> : SerializerSimpleStruct<A,
		&A::ai,
		&A::av,
		&A::ab
	>
{};

template <>
struct Serializer<C> : SerializerSimpleStruct<C,
		&C::c1,
		&C::c2,
		&C::c3
	>
{};

template <>
struct Serializer<E1> : SerializerEnum<E1, int>
{};

template <>
struct Serializer<E2> : SerializerEnum<E2, u16>
{};

template <>
struct Serializer<E3> : SerializerEnum<E3, int>
{};

}

TEST_CASE("sscsm_ipc_serialization") {

SECTION("Serializer") {

    SECTION("primitives") {
		serialize_and_deserialize((u8)0);
		serialize_and_deserialize((u8)14);
		serialize_and_deserialize((u16)234);
		serialize_and_deserialize((s16)-234);
		serialize_and_deserialize((s64)1248234989234);
		serialize_and_deserialize((size_t)42128);
		serialize_and_deserialize(false);
		serialize_and_deserialize(true);
	}

    SECTION("std::string") {
		using serializer = Serializer<std::string>;

		std::string s1 = "Hi, thank you for reviewing my PR!";

		std::vector<u8> buf1;
		buf1.resize(serializer::static_size);
		serializer::serialize(s1, 0, buf1);

		auto buf_sv = std::string_view(reinterpret_cast<char *>(buf1.data()), buf1.size());
		CHECK(buf_sv.substr(sizeof(size_t)) == s1);
		CHECK(*reinterpret_cast<size_t *>(buf1.data()) == s1.size());

		std::string s2 = "I've already thanked you, so now you have to finish reviewing it. Checkmate!";
		serialize_and_deserialize(s2);

		serialize_and_deserialize(std::string(""));
    }

    SECTION("std::vector") {
		std::vector<u8> v0 = {};
		serialize_and_deserialize(v0);
		std::vector<u8> v1 = {23, 1, 5, 2};
		serialize_and_deserialize(v1);
		std::vector<u16> v2 = {23, 1, 5, 51531};
		serialize_and_deserialize(v2);
		std::vector<s64> v3 = {231213523, 1, 5, 51531};
		serialize_and_deserialize(v3);
		std::vector<std::vector<u32>> v4 = {{231213523, 1, 5, 51531}, {}, {23, 1, 5, 2}};
		serialize_and_deserialize(v4);
	}

    SECTION("simple struct") {
		auto val1 = A{123, {B{{4}, 0}, B{{}, 5}}, B{{123, 2, 3}, 4}};
		serialize_and_deserialize(val1);

		auto cval1 = C{1, 2, 3};
		serialize_and_deserialize(cval1);

		std::vector<u8> buf_cval1;
		buf_cval1.resize(Serializer<C>::static_size);
		Serializer<C>::serialize(cval1, 0, buf_cval1);
		auto buf_cval1_sv = std::string_view(reinterpret_cast<char *>(buf_cval1.data()), buf_cval1.size());
		CHECK(buf_cval1_sv == "\x01\x02\x03");
	}

    SECTION("enum") {
		serialize_and_deserialize(E1_A);
		serialize_and_deserialize(E1_B);
		serialize_and_deserialize(E2_C);
		serialize_and_deserialize(E3::A);
    }

	//TODO: pair, tuple, unordered_map, enum, tagged union, optional, variant
}

}
