/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "l_client_sound.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "client/client.h"
#include "client/sound.h"

/* ModApiClientSound */

// sound_play(spec, parameters)
int ModApiClientSound::l_sound_play(lua_State *L)
{
	ISoundManager *sound_manager = getClient(L)->getSoundManager();

	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);

	SoundLocation type = SoundLocation::Local;
	float gain = 1.0f;
	v3f position;

	if (lua_istable(L, 2)) {
		getfloatfield(L, 2, "gain", gain);
		getfloatfield(L, 2, "pitch", spec.pitch);
		getboolfield(L, 2, "loop", spec.loop);

		lua_getfield(L, 2, "pos");
		if (!lua_isnil(L, -1)) {
			position = read_v3f(L, -1);
			type = SoundLocation::Position;
			lua_pop(L, 1);
		}
	}

	spec.gain *= gain;

	sound_handle_t handle = sound_manager->allocateId(2);

	if (type == SoundLocation::Local)
		sound_manager->playSound(handle, spec);
	else
		sound_manager->playSoundAt(handle, spec, position);

	ClientSoundRef::create(L, handle);
	return 1;
}

void ModApiClientSound::Initialize(lua_State *L, int top)
{
	API_FCT(sound_play);
}

/* ClientSoundRef */

ClientSoundRef *ClientSoundRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	return *(ClientSoundRef**)ud; // unbox pointer
}

int ClientSoundRef::gc_object(lua_State *L)
{
	std::unique_ptr<ClientSoundRef> o(*(ClientSoundRef **)(lua_touserdata(L, 1)));
	if (getClient(L) && getClient(L)->getSoundManager())
		getClient(L)->getSoundManager()->freeId(o->m_handle, 1);
	return 0;
}

// :stop()
int ClientSoundRef::l_stop(lua_State *L)
{
	ClientSoundRef *o = checkobject(L, 1);
	getClient(L)->getSoundManager()->stopSound(o->m_handle);
	return 0;
}

// :fade(step, gain)
int ClientSoundRef::l_fade(lua_State *L)
{
	ClientSoundRef *o = checkobject(L, 1);
	float step = readParam<float>(L, 2);
	float gain = readParam<float>(L, 3);
	getClient(L)->getSoundManager()->fadeSound(o->m_handle, step, gain);
	return 0;
}

void ClientSoundRef::create(lua_State *L, sound_handle_t handle)
{
	ClientSoundRef *o = new ClientSoundRef(handle);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ClientSoundRef::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_register(L, nullptr, methods);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable
}

const char ClientSoundRef::className[] = "ClientSoundRef";
const luaL_Reg ClientSoundRef::methods[] = {
	luamethod(ClientSoundRef, stop),
	luamethod(ClientSoundRef, fade),
	{0,0}
};
