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

#include "test.h"

#include "util/rotnum.h"
#include "irrlicht_changes/printing.h"

class TestRotNum : public TestBase {
public:
	TestRotNum() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestRotNum"; }

	void runTests(IGameDef *gamedef);

	void testDirNumPrint();
	void testDirNumNegate();
	void testDirNumInteractionWithVector3d();
	void testDirNumDirectionOf();

	void testRotNumMul();
};

static TestRotNum g_test_instance;

void TestRotNum::runTests(IGameDef *gamedef)
{
	TEST(testDirNumPrint);
	TEST(testDirNumNegate);
	TEST(testDirNumInteractionWithVector3d);
	TEST(testDirNumDirectionOf);

	TEST(testRotNumMul);
}

////////////////////////////////////////////////////////////////////////////////

void TestRotNum::testDirNumPrint()
{
	using namespace std::string_view_literals;

	UASSERTEQ(std::string_view,    DirNum::x().humanReadable(), "+x"sv);
	UASSERTEQ(std::string_view, (-DirNum::x()).humanReadable(), "-x"sv);
	UASSERTEQ(std::string_view,    DirNum::y().humanReadable(), "+y"sv);
	UASSERTEQ(std::string_view, (-DirNum::y()).humanReadable(), "-y"sv);
	UASSERTEQ(std::string_view,    DirNum::z().humanReadable(), "+z"sv);
	UASSERTEQ(std::string_view, (-DirNum::z()).humanReadable(), "-z"sv);
}

void TestRotNum::testDirNumNegate()
{
	for (auto dn : DirNum::allDirs())
		UASSERTEQ(DirNum, -(-dn), dn);
}

void TestRotNum::testDirNumInteractionWithVector3d()
{
	UASSERTEQ(v3s16, (-DirNum::z()).toVector3d<s16>(), v3s16(0,0,-1));
	UASSERTEQ(v3d, (-DirNum::z()).toVector3d<double>(), v3d(0,0,-1.0));

	UASSERTEQ(double v3d::*, (-DirNum::y()).memberPtrInVector3d<double>(), &v3d::Y);

	UASSERTEQ(v3s16, v3s16(1,2,3) + DirNum::z(), v3s16(1,2,4));
	UASSERTEQ(v3s16, DirNum::z() + v3s16(1,2,3), v3s16(1,2,4));
	UASSERTEQ(v3s16, -DirNum::z() + v3s16(1,2,3), v3s16(1,2,2));
	UASSERTEQ(v3s16, v3s16(1,2,3) - DirNum::z(), v3s16(1,2,2));
	UASSERTEQ(v3s16, v3s16(1,2,3) - (s16)2 * DirNum::z(), v3s16(1,2,1));
}

void TestRotNum::testDirNumDirectionOf()
{
	// (std::optional is not yet printable)
	UASSERT(DirNum::directionOf(v3s16(-1,0,0)) == -DirNum::x());
	UASSERT(DirNum::directionOf(v3s16(0,42,0)) == DirNum::y());
	UASSERT(DirNum::directionOf(v3s16(0,0,-13)) == -DirNum::z());
	UASSERT(DirNum::directionOf(v3s16(1,0,-13)) == std::nullopt);
	UASSERT(DirNum::directionOf(v3s16(0,0,0)) == std::nullopt);
}

////////////////////////////////////////////////////////////////////////////////

void TestRotNum::testRotNumMul()
{
	//TODO
}


