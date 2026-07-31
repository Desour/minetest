// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

#include "test.h"
#include "voxel.h"

class TestVoxelIter : public TestBase
{
public:
	TestVoxelIter() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestVoxelIter"; }

	void runTests(IGameDef *gamedef);

	void test_relpos();
};

static TestVoxelIter g_test_instance;

void TestVoxelIter::runTests(IGameDef *gamedef)
{
	TEST(test_relpos);
}

void TestVoxelIter::test_relpos()
{
	VoxelArea area(v3s16(-1447, -9547, -875), v3s16(-147, 8854, 669));

	VoxelIter iter1(area);

	auto pos = v3s16(0, 1, 2);
	iter1.resetRel(pos);
	UASSERTEQ(v3s16, iter1.relPos(), pos);
}

//TODO: more tests
//TODO: use catch2
