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

class TestRotNum : public TestBase {
public:
	TestRotNum() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestRotNum"; }

	void runTests(IGameDef *gamedef);

	void testDirNum();
	void testMul();
};

static TestRotNum g_test_instance;

void TestRotNum::runTests(IGameDef *gamedef)
{
	TEST(testDirNum);
	TEST(testMul);
}

////////////////////////////////////////////////////////////////////////////////

void TestRotNum::testDirNum()
{
	using namespace std::string_view_literals;

	UASSERTEQ(std::string_view,    DirNum::x.humanReadable(), "+x"sv);
	UASSERTEQ(std::string_view, (-DirNum::x).humanReadable(), "-x"sv);
	UASSERTEQ(std::string_view,    DirNum::y.humanReadable(), "+y"sv);
	UASSERTEQ(std::string_view, (-DirNum::y).humanReadable(), "-y"sv);
	UASSERTEQ(std::string_view,    DirNum::z.humanReadable(), "+z"sv);
	UASSERTEQ(std::string_view, (-DirNum::z).humanReadable(), "-z"sv);

	for (auto dn : DirNum::allDirs())
		UASSERTEQ(DirNum, -(-dn), dn);
}

void TestRotNum::testMul()
{
	//TODO
}


