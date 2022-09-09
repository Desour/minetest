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

#include "l_server_sound.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "server.h"

// sound_play(spec, parameters, [ephemeral])
int ModApiServerSound::l_sound_play(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ServerPlayingSound params;
	read_soundspec(L, 1, params.spec);
	read_server_sound_params(L, 2, params);
	bool ephemeral = lua_gettop(L) > 2 && readParam<bool>(L, 3);
	if (ephemeral) {
		getServer(L)->playSound(std::move(params), true);
		lua_pushnil(L);
	} else {
		s32 handle = getServer(L)->playSound(std::move(params));
		lua_pushinteger(L, handle);
	}
	return 1;
}

// sound_stop(handle)
int ModApiServerSound::l_sound_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	s32 handle = luaL_checkinteger(L, 1);
	getServer(L)->stopSound(handle);
	return 0;
}

// sound_fade(handle, step, gain)
int ModApiServerSound::l_sound_fade(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	s32 handle = luaL_checkinteger(L, 1);
	float step = readParam<float>(L, 2);
	float gain = readParam<float>(L, 3);
	getServer(L)->fadeSound(handle, step, gain);
	return 0;
}

void ModApiServerSound::Initialize(lua_State *L, int top)
{
	API_FCT(sound_play);
	API_FCT(sound_stop);
	API_FCT(sound_fade);
}
