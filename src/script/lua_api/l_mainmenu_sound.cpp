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

#include "l_mainmenu_sound.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "gui/guiEngine.h"

// sound_play(spec, loop)
int ModApiMainMenuSound::l_sound_play(lua_State *L)
{
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	spec.loop = readParam<bool>(L, 2);

	ISoundManager &sound_manager = *getGuiEngine(L)->m_sound_manager;

	s32 handle = sound_manager.allocateId(2); // TODO: userdata for gc, and 0 if ephemeral
	sound_manager.playSound(handle, spec);

	lua_pushinteger(L, handle);

	return 1;
}

// sound_stop(handle)
int ModApiMainMenuSound::l_sound_stop(lua_State *L)
{
	u32 handle = luaL_checkinteger(L, 1);

	ISoundManager &sound_manager = *getGuiEngine(L)->m_sound_manager;

	sound_manager.stopSound(handle);
	sound_manager.freeId(handle, 1);

	return 1;
}

void ModApiMainMenuSound::Initialize(lua_State *L, int top)
{
	API_FCT(sound_play);
	API_FCT(sound_stop);
}