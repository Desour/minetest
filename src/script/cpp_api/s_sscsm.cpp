// SPDX-FileCopyrightText: 2025 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "s_sscsm.h"

#include "s_internal.h"
#include "script/sscsm/sscsm_environment.h"

void ScriptApiSSCSM::add_itemdefs()
{
	SCRIPTAPI_PRECHECKHEADER

	infostream << "Adding itemdefs..." << std::endl;

	lua_newtable(L);
	int idx_registered_items = lua_gettop(L);
	lua_newtable(L);
	int idx_registered_nodes = lua_gettop(L);
	lua_newtable(L);
	int idx_registered_tools = lua_gettop(L);
	lua_newtable(L);
	int idx_registered_craftitems = lua_gettop(L);
	lua_newtable(L);
	int idx_registered_aliases = lua_gettop(L);

	lua_getglobal(L, "core");
	lua_pushvalue(L, idx_registered_items);
	lua_setfield(L, -2, "registed_items");
	lua_pushvalue(L, idx_registered_nodes);
	lua_setfield(L, -2, "registed_nodes");
	lua_pushvalue(L, idx_registered_tools);
	lua_setfield(L, -2, "registed_tools");
	lua_pushvalue(L, idx_registered_craftitems);
	lua_setfield(L, -2, "registed_craftitems");
	lua_pushvalue(L, idx_registered_aliases);
	lua_setfield(L, -2, "registed_aliases");
}

void ScriptApiSSCSM::load_mods(const std::vector<std::pair<std::string, std::string>> &mods)
{
	infostream << "Loading SSCSMs:" << std::endl;
	for (const auto &m : mods) {
		infostream << "Loading SSCSM " << m.first << std::endl;
		loadModFromMemory(m.first, m.second);
	}
}

void ScriptApiSSCSM::environment_step(float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_globalsteps
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
}
