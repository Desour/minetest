/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "csm_script_process.h"
#include "csm_gamedef.h"
#include "csm_message.h"
#include "csm_script_ipc.h"
#include "csm_scripting.h"
#include "cpp_api/s_base.h"
#include "debug.h"
#include "filesys.h"
#include "itemdef.h"
#include "log.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
#include "porting.h"
#include "sandbox.h"
#include "util/string.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utility>

namespace {

class CSMLogOutput : public ICombinedLogOutput {
public:
	void logRaw(LogLevel level, const std::string &line) override
	{
		if (m_can_log) {
			sendLog(level, line);
		} else {
			m_waiting_logs.emplace_back(level, line);
		}
	}

	void startLogging()
	{
		m_can_log = true;
		for (const auto &log : m_waiting_logs) {
			sendLog(log.first, log.second);
		}
		m_waiting_logs.clear();
	}

	void stopLogging()
	{
		m_can_log = false;
	}

private:
	void sendLog(LogLevel level, const std::string &line)
	{
		CSMS2CLog log;
		log.level = level;
		m_send_data.resize(sizeof(log) + line.size());
		memcpy(m_send_data.data(), &log, sizeof(log));
		memcpy(m_send_data.data() + sizeof(log), line.data(), line.size());
		CSM_IPC(exchange(m_send_data.size(), m_send_data.data()));
	}

	bool m_can_log;
	std::vector<std::pair<LogLevel, std::string>> m_waiting_logs;
	std::vector<u8> m_send_data;
};

CSMLogOutput g_log_output;

} // namespace

int csm_script_main(int argc, char *argv[])
{
	FATAL_ERROR_IF(argc < 4, "Too few arguments to CSM process");

	int shm = shm_open(argv[2], O_RDWR, 0);
	shm_unlink(argv[2]);
	FATAL_ERROR_IF(shm == -1, "CSM process unable to open shared memory");

	IPCChannelShared *ipc_shared = (IPCChannelShared *)mmap(nullptr, sizeof(IPCChannelShared),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	FATAL_ERROR_IF(ipc_shared == MAP_FAILED, "CSM process unable to map shared memory");

	g_csm_script_ipc = IPCChannelEnd::makeB(ipc_shared);

	// TODO: only send logs if they will actually be recorded.
	g_logger.addOutput(&g_log_output);
	g_logger.registerThread("CSM");

	CSM_IPC(recv());
	std::string initial_recv((const char *)csm_recv_data(), csm_recv_size());

	g_log_output.startLogging();

	CSMGameDef gamedef;

	{
		std::istringstream is(std::move(initial_recv), std::ios::binary);
		gamedef.getWritableItemDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
		gamedef.getWritableNodeDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
	}

	CSMScripting script(&gamedef);

	std::vector<std::string> mods;

	{
		std::string client_path = argv[3];
		std::string builtin_path = client_path + DIR_DELIM "csmbuiltin";
		std::string mods_path = client_path + DIR_DELIM "csm";

		gamedef.scanModIntoMemory(BUILTIN_MOD_NAME, builtin_path);
		std::vector<fs::DirListNode> mod_dirs = fs::GetDirListing(mods_path);
		for (const fs::DirListNode &dir : mod_dirs) {
			if (dir.dir) {
				size_t number = mystoi(dir.name.substr(0, 5), 0, 99999);
				std::string name = dir.name.substr(5);
				gamedef.scanModIntoMemory(name, mods_path + DIR_DELIM + dir.name);
				mods.resize(number + 1);
				mods[number] = std::move(name);
			}
		}
	}

	if (!start_sandbox())
		warningstream << "Unable to start process sandbox" << std::endl;

	script.loadModFromMemory(BUILTIN_MOD_NAME);
	script.checkSetByBuiltin();
	for (const std::string &mod : mods) {
		if (!mod.empty())
			script.loadModFromMemory(mod);
	}

	g_log_output.stopLogging();

	CSM_IPC(exchange(CSM_S2C_DONE));

	while (true) {
		size_t size = csm_recv_size();
		const void *data = csm_recv_data();
		CSMMsgType type = CSM_INVALID_MSG_TYPE;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
		switch (type) {
		case CSM_C2S_RUN_STEP:
			if (size >= sizeof(CSMC2SRunStep)) {
				CSMC2SRunStep msg;
				memcpy(&msg, data, sizeof(msg));
				g_log_output.startLogging();
				script.environment_Step(msg.dtime);
			}
			break;
		default:
			break;
		}
		g_log_output.stopLogging();
		CSM_IPC(exchange(CSM_S2C_DONE));
	}

	return 0;
}