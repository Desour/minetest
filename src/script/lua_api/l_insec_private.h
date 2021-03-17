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

#pragma once

#include "lua_api/l_base.h"

class ModApiInsecPrivate : public ModApiBase
{
private:
	/*
		NOTE:
		The functions in this module are available in the insec_private table
		in the insecure environment.
	*/

	// get_script_api_core()
	static int l_get_script_api_core(lua_State *L);

	// get_objref_metatable()
	static int l_get_objref_metatable(lua_State *L);

	// objref_check(obj)
	static int l_objref_check(lua_State *L);

public:
	static void Initialize(lua_State *L, int insec_top);
};
