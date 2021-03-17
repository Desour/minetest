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

/**
 * Functions for luajit ffi.
 *
 * Note: There is no header file for this file, as these functions should only
 * be called from lua.
 * Also note: All symbol names must start with "ffi_", see src/dynamic_list_file.txt .
 *
 * See also: builtin/game/ffi_replacements.lua
 *
 * Most ffi functions return an int (ffi_retint) containing the number of return
 * values written or a negative number in case of an error (eg. an exception occured).
 * In case of a negative return value, the lua wrapper function shall raise an error
 * with the error message gotten from ffi_get_errstr.
 */

#include "log.h"
#include "cpp_api/s_core.h"
#include "cpp_api/s_base.h"
#include "config.h"
#include "nodedef.h"
#include "gamedef.h"
#include "serverenvironment.h"
#include "map.h"
#include "server.h"
#include "server/luaentity_sao.h"
#include "server/player_sao.h"
#include "porting.h"
#include "util/numeric.h"
#include "lua_api/l_object.h"
#include "remoteplayer.h"

extern "C" {

// copied from common/c_converter.cpp
#define CHECK_FLOAT_RANGE(value, name) \
if (value < F1000_MIN || value > F1000_MAX) { \
	std::ostringstream error_text; \
	error_text << "Invalid float vector dimension range '" name "' " << \
	"(expected " << F1000_MIN << " < " name " < " << F1000_MAX << \
	" got " << value << ")." << std::endl; \
	throw LuaError(error_text.str()); \
}

#if !defined(SERVER) && !defined(NDEBUG)
#define FFI_DEBUG_ASSERT_NO_CLIENTAPI                \
	FATAL_ERROR_IF(getClient(L) != nullptr, "Tried " \
		"to retrieve ServerEnvironment on client")
#else
#define FFI_DEBUG_ASSERT_NO_CLIENTAPI ((void)0)
#endif

#define FFI_GET_ENV_PTR                                          \
	FFI_DEBUG_ASSERT_NO_CLIENTAPI;                               \
	ServerEnvironment *env = (ServerEnvironment *)sac->getEnv(); \
	if (env == NULL)                                             \
		return 0

#define FFI_EXCEPTS_TO_ERRS(block) \
	try                            \
		block                      \
	catch (const char *s) {        \
		errstr = s;                \
	} catch (std::exception &e) {  \
		errstr = e.what();         \
	}                              \
	return -1

typedef int ffi_retint;

/**
 * A not owned string.
 * Does not need to be freed.
 */
typedef struct {
	const char *data;
	u64 length;
} ffi_StringRef;

static void stringref_into_ffi_stringref(ffi_StringRef &ret, const std::string &str)
{
	ret.data = str.c_str();
	ret.length = str.length();
}

static std::string ffi_stringref_into_string(const ffi_StringRef &str)
{
	return std::string(str.data, str.length);
}

/**
 * A not owned node.
 * Does not need to be freed.
 */
typedef struct {
	ffi_StringRef name;
	u8 param1;
	u8 param2;
} ffi_NodeRef;

typedef struct {
	double x;
	double y;
	double z;
} ffi_v3d;

static thread_local std::string errstr = "";

const char *ffi_get_errstr()
{
	return errstr.c_str();
}

void ffi_test()
{
	//~ printf("test() called.\n");
	errorstream << "test() called." << std::endl;
}

// returns 1 if successful, otherwise 0
static ffi_retint set_or_swap_node(ScriptApiCore *sac, double x, double y, double z,
		ffi_NodeRef *node, bool is_swap)
{
	FFI_EXCEPTS_TO_ERRS({
		FFI_GET_ENV_PTR;

		v3s16 p = doubleToInt(v3d(x, y, z), 1.0);
		std::string name = ffi_stringref_into_string(node->name);

		const NodeDefManager *ndef = sac->getGameDef()->ndef();
		ServerMap &map = env->getServerMap();

		content_t id = CONTENT_IGNORE;
		if (!ndef->getId(name, id))
			throw LuaError("\"" + name + "\" is not a registered node!");

		MapNode n(id, node->param1, node->param2);

		// Replace node
		if (!map.addNodeWithEvent(p, n, !is_swap))
			return 0;

		// Update active VoxelManipulator if a mapgen thread
		map.updateVManip(p);

		return 1;
	});
}

// does not call callbacks, like on_destruct
// returns 1 if successful, otherwise 0
ffi_retint ffi_set_node_raw(ScriptApiCore *sac, double x, double y, double z, ffi_NodeRef *node)
{
	return set_or_swap_node(sac, x, y, z, node, false);
}

// returns 1 if successful, otherwise 0
ffi_retint ffi_swap_node(ScriptApiCore *sac, double x, double y, double z, ffi_NodeRef *node)
{
	return set_or_swap_node(sac, x, y, z, node, true);
}

// does not call callbacks, like on_destruct
// returns 1 if successful, otherwise 0
ffi_retint ffi_remove_node_raw(ScriptApiCore *sac, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		FFI_GET_ENV_PTR;

		v3s16 p = doubleToInt(v3d(x, y, z), 1.0);

		ServerMap &map = env->getServerMap();

		// Replace with air
		// This is slightly optimized compared to addNodeWithEvent(air)
		if (!map.removeNodeWithEvent(p))
			return 0;

		// Update active VoxelManipulator if a mapgen thread
		map.updateVManip(p);

		return 1;
	});
}

ffi_retint ffi_get_node(ScriptApiCore *sac, ffi_NodeRef *ret1, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		FFI_GET_ENV_PTR;

		v3s16 p = doubleToInt(v3d(x, y, z), 1.0);

		MapNode n = env->getMap().getNode(p);

		// Return node
		const std::string &name = sac->getGameDef()->ndef()->get(n).name;
		stringref_into_ffi_stringref(ret1->name, name);
		ret1->param1 = n.getParam1();
		ret1->param2 = n.getParam2();

		return 1;
	});
}

ffi_retint ffi_get_node_or_nil(ScriptApiCore *sac, ffi_NodeRef *ret1, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		FFI_GET_ENV_PTR;

		v3s16 pos = doubleToInt(v3d(x, y, z), 1.0);
		bool pos_ok;

		MapNode n = env->getMap().getNode(pos, &pos_ok);

		if (!pos_ok)
			return 0;

		// Return node
		const std::string &name = sac->getGameDef()->ndef()->get(n).name;
		stringref_into_ffi_stringref(ret1->name, name);
		ret1->param1 = n.getParam1();
		ret1->param2 = n.getParam2();

		return 1;
	});
}

u64 ffi_get_us_time()
{
	return porting::getTimeUs();
}

void ffi_set_last_run_mod(ScriptApiCore *sac, const char *mod)
{
#ifdef SCRIPTAPI_DEBUG
	sac->setOriginDirect(mod);
#endif
}

static v3f check_float_pos(double x, double y, double z)
{
	v3f v(x, y, z);

	CHECK_FLOAT_RANGE(v.X, "x");
	CHECK_FLOAT_RANGE(v.Y, "y");
	CHECK_FLOAT_RANGE(v.Z, "z");

	return v * BS;
}

ffi_retint ffi_objref_get_pos(ffi_v3d *ret1, ObjectRef *ref)
{
	FFI_EXCEPTS_TO_ERRS({
		ServerActiveObject *sao = ObjectRef::getobject(ref);
		if (!sao)
			return 0;

		v3f p = sao->getBasePosition() / BS;
		ret1->x = p.X;
		ret1->y = p.Y;
		ret1->z = p.Z;
		return 1;
	});
}

ffi_retint ffi_objref_set_pos(ObjectRef *ref, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		ServerActiveObject *sao = ObjectRef::getobject(ref);
		if (!sao)
			return 0;

		v3f pos = check_float_pos(x, y, z);
		sao->setPos(pos);

		return 0;
	});
}

ffi_retint ffi_objref_move_to(ObjectRef *ref, double x, double y, double z, bool continuous)
{
	FFI_EXCEPTS_TO_ERRS({
		ServerActiveObject *sao = ObjectRef::getobject(ref);
		if (!sao)
			return 0;

		v3f pos = check_float_pos(x, y, z);
		sao->moveTo(pos, continuous);

		return 0;
	});
}

ffi_retint ffi_objref_add_velocity(ScriptApiCore *sac, ObjectRef *ref, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		ServerActiveObject *sao = ObjectRef::getobject(ref);
		if (!sao)
			return 0;

		v3f vel = check_float_pos(x, y, z);
		if (sao->getType() == ACTIVEOBJECT_TYPE_LUAENTITY) {
			LuaEntitySAO *entitysao = dynamic_cast<LuaEntitySAO *>(sao);
			entitysao->addVelocity(vel);
		} else if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			PlayerSAO *playersao = dynamic_cast<PlayerSAO *>(sao);
			playersao->setMaxSpeedOverride(vel);
			sac->getServer()->SendPlayerSpeed(playersao->getPeerID(), vel);
		}

		return 0;
	});
}

ffi_retint ffi_objref_set_velocity(ObjectRef *ref, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		LuaEntitySAO *sao = ObjectRef::getluaobject(ref);
		if (!sao)
			return 0;

		v3f vel = check_float_pos(x, y, z);
		sao->setVelocity(vel);

		return 0;
	});
}

ffi_retint ffi_objref_get_velocity(ffi_v3d *ret1, ObjectRef *ref)
{
	FFI_EXCEPTS_TO_ERRS({
		ServerActiveObject *sao = ObjectRef::getobject(ref);
		if (!sao)
			return 0;

		v3f vel;
		if (sao->getType() == ACTIVEOBJECT_TYPE_LUAENTITY) {
			LuaEntitySAO *entitysao = dynamic_cast<LuaEntitySAO*>(sao);
			vel = entitysao->getVelocity() / BS;
		} else if (sao->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			RemotePlayer *player = dynamic_cast<PlayerSAO*>(sao)->getPlayer();
			vel = player->getSpeed() / BS;
		} else {
			return 0;
		}

		ret1->x = vel.X;
		ret1->y = vel.Y;
		ret1->z = vel.Z;

		return 1;
	});
}

ffi_retint ffi_objref_set_acceleration(ObjectRef *ref, double x, double y, double z)
{
	FFI_EXCEPTS_TO_ERRS({
		LuaEntitySAO *sao = ObjectRef::getluaobject(ref);
		if (!sao)
			return 0;

		v3f acc = check_float_pos(x, y, z);
		sao->setAcceleration(acc);

		return 0;
	});
}

ffi_retint ffi_objref_get_acceleration(ffi_v3d *ret1, ObjectRef *ref)
{
	FFI_EXCEPTS_TO_ERRS({
		LuaEntitySAO *sao = ObjectRef::getluaobject(ref);
		if (!sao)
			return 0;

		v3f acc = sao->getAcceleration() / BS;

		ret1->x = acc.X;
		ret1->y = acc.Y;
		ret1->z = acc.Z;

		return 1;
	});
}

} // extern "C"
