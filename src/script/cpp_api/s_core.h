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

#include "irrlichttypes.h"
#include "debug.h"

#include "util/basic_macros.h"
#include "config.h"

enum class ScriptingType: u8 {
	Async,
	Client,
	MainMenu,
	Server
};

class Server;
#ifndef SERVER
class Client;
#endif
class IGameDef;
class Environment;
class GUIEngine;

class ScriptApiCore {
public:
	ScriptApiCore(ScriptingType type) : m_type(type) {};
	// fake constructor to allow script API classes (e.g ScriptApiEnv) to virtually inherit from this one.
	ScriptApiCore()
	{
		FATAL_ERROR("ScriptApiCore created without ScriptingType!");
	}
	DISABLE_CLASS_COPY(ScriptApiCore);

	IGameDef *getGameDef() { return m_gamedef; }
	Server *getServer();
	ScriptingType getType() { return m_type; }
#ifndef SERVER
	Client *getClient();
#endif

	Environment *getEnv() { return m_environment; }

	std::string getOrigin() { return m_last_run_mod; }
	void setOriginDirect(const char *origin);

protected:
	void setGameDef(IGameDef *gamedef) { m_gamedef = gamedef; }

	void setEnv(Environment *env) { m_environment = env; }

#ifndef SERVER
	GUIEngine *getGuiEngine() { return m_guiengine; }
	void setGuiEngine(GUIEngine *guiengine) { m_guiengine = guiengine; }
#endif

	std::string     m_last_run_mod;

private:
	IGameDef       *m_gamedef = nullptr;
	Environment    *m_environment = nullptr;
#ifndef SERVER
	GUIEngine      *m_guiengine = nullptr;
#endif
	ScriptingType  m_type;
};
