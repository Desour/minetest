/*
Minetest
Copyright (C) 2018 bendeutsch, Ben Deutsch <ben@bendeutsch.de>

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

#include <algorithm>
#include "database/database-files.h"
#include "database/database-sqlite3.h"
#include "util/string.h"
#include "filesys.h"

namespace
{
// Anonymous namespace to create classes that are only
// visible to this file
//
// These are helpers that return a *AuthDatabase and
// allow us to run the same tests on different databases and
// database acquisition strategies.

class AuthDatabaseProvider
{
public:
	virtual ~AuthDatabaseProvider() = default;
	virtual AuthDatabase *getAuthDatabase() = 0;
};

class FixedProvider : public AuthDatabaseProvider
{
public:
	FixedProvider(AuthDatabase *auth_db) : auth_db(auth_db){};
	~FixedProvider() override {};
	AuthDatabase *getAuthDatabase() override { return auth_db; };

private:
	AuthDatabase *auth_db;
};

class FilesProvider : public AuthDatabaseProvider
{
public:
	FilesProvider(const std::string &dir) : dir(dir){};
	~FilesProvider() override = default;
	AuthDatabase *getAuthDatabase() override
	{
		auth_db.reset();
		auth_db = std::make_unique<AuthDatabaseFiles>(dir);
		return auth_db.get();
	};

private:
	std::string dir;
	std::unique_ptr<AuthDatabase> auth_db;
};

class SQLite3Provider : public AuthDatabaseProvider
{
public:
	SQLite3Provider(const std::string &dir) : dir(dir){};
	~SQLite3Provider() override = default;
	AuthDatabase *getAuthDatabase() override
	{
		auth_db.reset();
		auth_db = std::make_unique<AuthDatabaseSQLite3>(dir);
		return auth_db.get();
	};

private:
	std::string dir;
	std::unique_ptr<AuthDatabase> auth_db;
};
}

class TestAuthDatabase : public TestBase
{
public:
	TestAuthDatabase() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestAuthDatabase"; }

	void runTests(IGameDef *gamedef);
	void runTestsForCurrentDB();

	void testRecallFail();
	void testCreate();
	void testRecall();
	void testChange();
	void testRecallChanged();
	void testChangePrivileges();
	void testRecallChangedPrivileges();
	void testListNames();
	void testDelete();

private:
	std::unique_ptr<AuthDatabaseProvider> auth_provider;
};

static TestAuthDatabase g_test_instance;

void TestAuthDatabase::runTests(IGameDef *gamedef)
{
	// fixed directory, for persistence
	thread_local const std::string test_dir = getTestTempDirectory();

	// Each set of tests is run twice for each database type:
	// one where we reuse the same AuthDatabase object (to test local caching),
	// and one where we create a new AuthDatabase object for each call
	// (to test actual persistence).

	std::unique_ptr<AuthDatabase> auth_db;

	rawstream << "-------- Files database (same object)" << std::endl;

	auth_db = std::make_unique<AuthDatabaseFiles>(test_dir);
	auth_provider = std::make_unique<FixedProvider>(auth_db.get());

	runTestsForCurrentDB();

	auth_provider.reset();
	auth_db.reset();

	// reset database
	fs::DeleteSingleFileOrEmptyDirectory(test_dir + DIR_DELIM + "auth.txt");

	rawstream << "-------- Files database (new objects)" << std::endl;

	auth_provider = std::make_unique<FilesProvider>(test_dir);

	runTestsForCurrentDB();

	auth_provider.reset();

	rawstream << "-------- SQLite3 database (same object)" << std::endl;

	auth_db = std::make_unique<AuthDatabaseSQLite3>(test_dir);
	auth_provider = std::make_unique<FixedProvider>(auth_db.get());

	runTestsForCurrentDB();

	auth_provider.reset();
	auth_db.reset();

	// reset database
	fs::DeleteSingleFileOrEmptyDirectory(test_dir + DIR_DELIM + "auth.sqlite");

	rawstream << "-------- SQLite3 database (new objects)" << std::endl;

	auth_provider = std::make_unique<SQLite3Provider>(test_dir);

	runTestsForCurrentDB();

	auth_provider.reset();
}

////////////////////////////////////////////////////////////////////////////////

void TestAuthDatabase::runTestsForCurrentDB()
{
	TEST(testRecallFail);
	TEST(testCreate);
	TEST(testRecall);
	TEST(testChange);
	TEST(testRecallChanged);
	TEST(testChangePrivileges);
	TEST(testRecallChangedPrivileges);
	TEST(testListNames);
	TEST(testDelete);
	TEST(testRecallFail);
}

void TestAuthDatabase::testRecallFail()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	// no such user yet
	UASSERT(!auth_db->getAuth("TestName", authEntry));
}

void TestAuthDatabase::testCreate()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	authEntry.name = "TestName";
	authEntry.password = "TestPassword";
	authEntry.privileges.emplace_back("shout");
	authEntry.privileges.emplace_back("interact");
	authEntry.last_login = 1000;
	UASSERT(auth_db->createAuth(authEntry));
}

void TestAuthDatabase::testRecall()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	UASSERT(auth_db->getAuth("TestName", authEntry));
	UASSERTEQ(std::string, authEntry.name, "TestName");
	UASSERTEQ(std::string, authEntry.password, "TestPassword");
	// the order of privileges is unimportant
	std::sort(authEntry.privileges.begin(), authEntry.privileges.end());
	UASSERTEQ(std::string, str_join(authEntry.privileges, ","), "interact,shout");
}

void TestAuthDatabase::testChange()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	UASSERT(auth_db->getAuth("TestName", authEntry));
	authEntry.password = "NewPassword";
	authEntry.last_login = 1002;
	UASSERT(auth_db->saveAuth(authEntry));
}

void TestAuthDatabase::testRecallChanged()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	UASSERT(auth_db->getAuth("TestName", authEntry));
	UASSERTEQ(std::string, authEntry.password, "NewPassword");
	// the order of privileges is unimportant
	std::sort(authEntry.privileges.begin(), authEntry.privileges.end());
	UASSERTEQ(std::string, str_join(authEntry.privileges, ","), "interact,shout");
	UASSERTEQ(u64, authEntry.last_login, 1002);
}

void TestAuthDatabase::testChangePrivileges()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	UASSERT(auth_db->getAuth("TestName", authEntry));
	authEntry.privileges.clear();
	authEntry.privileges.emplace_back("interact");
	authEntry.privileges.emplace_back("fly");
	authEntry.privileges.emplace_back("dig");
	UASSERT(auth_db->saveAuth(authEntry));
}

void TestAuthDatabase::testRecallChangedPrivileges()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	AuthEntry authEntry;

	UASSERT(auth_db->getAuth("TestName", authEntry));
	// the order of privileges is unimportant
	std::sort(authEntry.privileges.begin(), authEntry.privileges.end());
	UASSERTEQ(std::string, str_join(authEntry.privileges, ","), "dig,fly,interact");
}

void TestAuthDatabase::testListNames()
{

	AuthDatabase *auth_db = auth_provider->getAuthDatabase();
	std::vector<std::string> list;

	AuthEntry authEntry;

	authEntry.name = "SecondName";
	authEntry.password = "SecondPassword";
	authEntry.privileges.emplace_back("shout");
	authEntry.privileges.emplace_back("interact");
	authEntry.last_login = 1003;
	auth_db->createAuth(authEntry);

	auth_db->listNames(list);
	// not necessarily sorted, so sort before comparing
	std::sort(list.begin(), list.end());
	UASSERTEQ(std::string, str_join(list, ","), "SecondName,TestName");
}

void TestAuthDatabase::testDelete()
{
	AuthDatabase *auth_db = auth_provider->getAuthDatabase();

	UASSERT(!auth_db->deleteAuth("NoSuchName"));
	UASSERT(auth_db->deleteAuth("TestName"));
	// second try, expect failure
	UASSERT(!auth_db->deleteAuth("TestName"));
}
