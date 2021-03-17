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

#include "cpp_api/s_core.h"

#include "server.h"
#ifndef SERVER
#include "client/client.h"
#endif

Server *ScriptApiCore::getServer()
{
	return dynamic_cast<Server *>(m_gamedef);
}
#ifndef SERVER
Client *ScriptApiCore::getClient()
{
	return dynamic_cast<Client *>(m_gamedef);
}
#endif

void ScriptApiCore::setOriginDirect(const char *origin)
{
	m_last_run_mod = origin ? origin : "??";
}
