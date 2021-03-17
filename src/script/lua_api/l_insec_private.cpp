/*
Minetest
Copyright (C) 2021 DS

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

#include "cpp_api/s_base.h"
#include "lua_api/l_insec_private.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_object.h"
#include "log.h"

// get_script_api_core()
int ModApiInsecPrivate::l_get_script_api_core(lua_State *L)
{
	MAP_LOCK_REQUIRED;
	ScriptApiCore *sac = getScriptApiBase(L);
	*(ScriptApiCore **)lua_newuserdata(L, sizeof(sac)) = sac;
	return 1;
}

int ModApiInsecPrivate::l_get_objref_metatable(lua_State *L)
{
	MAP_LOCK_REQUIRED;
	luaL_getmetatable(L, ObjectRef::className);
	return 1;
}

int ModApiInsecPrivate::l_objref_check(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ObjectRef::checkobject(L, 1);
	return 0;
}

void ModApiInsecPrivate::Initialize(lua_State *L, int insec_top)
{
	API_FCT_INSEC(get_script_api_core);
	API_FCT_INSEC(get_objref_metatable);
	API_FCT_INSEC(objref_check);
}
