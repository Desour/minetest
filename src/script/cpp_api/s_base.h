/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "common/helper.h"
#include "cpp_api/s_core.h"
#include "util/basic_macros.h"

extern "C" {
#include <lua.h>
#include <lualib.h>
}

#include "irrlichttypes.h"
#include "common/c_types.h"
#include "common/c_internal.h"
#include "debug.h"
#include "config.h"

#define SCRIPTAPI_LOCK_DEBUG
#define SCRIPTAPI_DEBUG

// MUST be an invalid mod name so that mods can't
// use that name to bypass security!
#define BUILTIN_MOD_NAME "*builtin*"

#define PCALL_RES(RES) {                    \
	int result_ = (RES);                    \
	if (result_ != 0) {                     \
		scriptError(result_, __FUNCTION__); \
	}                                       \
}

#define runCallbacks(nargs, mode) \
	runCallbacksRaw((nargs), (mode), __FUNCTION__)

#define setOriginFromTable(index) \
	setOriginFromTableRaw(index, __FUNCTION__)

class ServerActiveObject;
struct PlayerHPChangeReason;

class ScriptApiBase : public ScriptApiCore, protected LuaHelper {
public:
	ScriptApiBase(ScriptingType type);
	ScriptApiBase() : ScriptApiCore() {};
	virtual ~ScriptApiBase();
	DISABLE_CLASS_COPY(ScriptApiBase);

	// These throw a ModError on failure
	void loadMod(const std::string &script_path, const std::string &mod_name);
	void loadScript(const std::string &script_path);

#ifndef SERVER
	void loadModFromMemory(const std::string &mod_name);
#endif

	void runCallbacksRaw(int nargs,
		RunCallbacksMode mode, const char *fxn);

	/* object */
	void addObjectReference(ServerActiveObject *cobj);
	void removeObjectReference(ServerActiveObject *cobj);

	void setOriginFromTableRaw(int index, const char *fxn);

	void clientOpenLibs(lua_State *L);

protected:
	friend class LuaABM;
	friend class LuaLBM;
	friend class InvRef;
	friend class ObjectRef;
	friend class NodeMetaRef;
	friend class ModApiBase;
	friend class ModApiEnvMod;
	friend class LuaVoxelManip;

	lua_State* getStack()
		{ return m_luastack; }

	void realityCheck();
	void scriptError(int result, const char *fxn);
	void stackDump(std::ostream &o);

	void objectrefGetOrCreate(lua_State *L, ServerActiveObject *cobj);

	void pushPlayerHPChangeReason(lua_State *L, const PlayerHPChangeReason& reason);

	std::recursive_mutex m_luastackmutex;
	bool            m_secure = false;
#ifdef SCRIPTAPI_LOCK_DEBUG
	int             m_lock_recursion_count{};
	std::thread::id m_owning_thread;
#endif

private:
	static int luaPanic(lua_State *L);

	lua_State      *m_luastack = nullptr;
};
