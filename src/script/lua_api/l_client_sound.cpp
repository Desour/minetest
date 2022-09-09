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

// sound_play(spec, parameters)
int ModApiClientSound::l_sound_play(lua_State *L)
{
	ISoundManager *sound = getClient(L)->getSoundManager();

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

	s32 handle = sound->allocateId(2); // TODO: use 0 if ephemeral
	if (type == SoundLocation::Local)
		sound->playSound(handle, spec);
	else
		sound->playSoundAt(handle, spec, position);

	lua_pushinteger(L, handle); // TODO: put into userdata for garbage collection
	return 1;
}

// sound_stop(handle)
int ModApiClientSound::l_sound_stop(lua_State *L)
{
	s32 handle = luaL_checkinteger(L, 1);

	ISoundManager *sound_manager = getClient(L)->getSoundManager();
	sound_manager->stopSound(handle);
	sound_manager->freeId(handle, 1);

	return 0;
}

// sound_fade(handle, step, gain)
int ModApiClientSound::l_sound_fade(lua_State *L)
{
	s32 handle = luaL_checkinteger(L, 1);
	float step = readParam<float>(L, 2);
	float gain = readParam<float>(L, 3);
	getClient(L)->getSoundManager()->fadeSound(handle, step, gain);
	return 0;
}

void ModApiClientSound::Initialize(lua_State *L, int top)
{
	API_FCT(sound_play);
	API_FCT(sound_stop);
	API_FCT(sound_fade);
}
