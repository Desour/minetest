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

#pragma once

#include "irr_v3d.h"
#include "log.h"
#include "mapnode.h"
#include "modchannels.h"
#include <stddef.h>

// controller -> script

enum CSMC2SMsgType {
	CSM_C2S_INVALID = -1,

	CSM_C2S_RUN_SHUTDOWN,
	CSM_C2S_RUN_CLIENT_READY,
	CSM_C2S_RUN_CAMERA_READY,
	CSM_C2S_RUN_MINIMAP_READY,
	CSM_C2S_RUN_SENDING_MESSAGE,
	CSM_C2S_RUN_RECEIVING_MESSAGE,
	CSM_C2S_RUN_HP_MODIFICATION,
	CSM_C2S_RUN_MODCHANNEL_MESSAGE,
	CSM_C2S_RUN_MODCHANNEL_SIGNAL,
	CSM_C2S_RUN_FORMSPEC_INPUT,
	CSM_C2S_RUN_INVENTORY_OPEN,
	CSM_C2S_RUN_STEP,
};

struct CSMC2SRunHPModification {
	CSMC2SMsgType type = CSM_C2S_RUN_HP_MODIFICATION;
	u16 hp;
};

struct CSMC2SRunModchannelMessage {
	CSMC2SMsgType type = CSM_C2S_RUN_MODCHANNEL_MESSAGE;
	size_t channel_size, sender_size, message_size;
	// string channel, sender, and message follow
};

struct CSMC2SRunModchannelSignal {
	CSMC2SMsgType type = CSM_C2S_RUN_MODCHANNEL_SIGNAL;
	ModChannelSignal signal;
	// string channel follows
};

struct CSMC2SRunStep {
	CSMC2SMsgType type = CSM_C2S_RUN_STEP;
	float dtime;
};

struct CSMC2SGetNode {
	MapNode n;
	bool pos_ok;
};

// script -> controller

enum CSMS2CMsgType {
	CSM_S2C_INVALID = -1,

	CSM_S2C_DONE,
	CSM_S2C_LOG,
	CSM_S2C_GET_NODE,
	CSM_S2C_ADD_NODE,
};

struct CSMS2CDoneBool {
	CSMS2CMsgType type = CSM_S2C_DONE;
	bool value;
};

struct CSMS2CLog {
	CSMS2CMsgType type = CSM_S2C_LOG;
	LogLevel level;
	// message string follows
};

struct CSMS2CGetNode {
	CSMS2CMsgType type = CSM_S2C_GET_NODE;
	v3s16 pos;
};

struct CSMS2CAddNode {
	CSMS2CMsgType type = CSM_S2C_ADD_NODE;
	MapNode n;
	v3s16 pos;
	bool remove_metadata = true;
};
