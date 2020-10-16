/**
 * Functions for luajit ffi.
 *
 * Note: There is no header file for this file, as these functions should only
 * be called from lua.
 *
 * See also: builtin/game/ffi_replacements.lua
 */

#include <stdio.h>
#include "log.h"
#include "itemdef.h"
#include "nodedef.h"
#include "common/c_internal.h"
#include "cpp_api/s_base.h"
#include "gamedef.h"
#include "serverenvironment.h"
#include "map.h"
#include "server.h"

#define GET_ENV_PTR                                              \
	ServerEnvironment *env = (ServerEnvironment *)sab->getEnv(); \
	if (env == NULL)                                             \
		return 0
		//~ return {false}

extern "C" {

extern const int luaffi_CUSTOM_RIDX_SCRIPTAPI = CUSTOM_RIDX_SCRIPTAPI;
extern const bool luaffi_INDIRECT_SCRIPTAPI_RIDX = INDIRECT_SCRIPTAPI_RIDX;

static ScriptApiBase *s_script_api_base = NULL;
void luaffi_set_script_api_base(ScriptApiBase *sab)
{
	s_script_api_base = sab;
}

void luaffi_test()
{
	//~ printf("test() called.\n");
	errorstream << "test() called." << std::endl;
}

void luaffi_print_to_errstream(const char *s)
{
	errorstream << s << std::endl;
}

int luaffi_get_content_id(ScriptApiBase *sab, const char *name_c)
{
//~ int luaffi_get_content_id(const char *name_c)
//~ {
	//~ ScriptApiBase *sab = s_script_api_base;
	std::string name(name_c);

	const IItemDefManager *idef = sab->getGameDef()->getItemDefManager();
	const NodeDefManager *ndef = sab->getGameDef()->getNodeDefManager();

	// If this is called at mod load time, NodeDefManager isn't aware of
	// aliases yet, so we need to handle them manually
	std::string alias_name = idef->getAlias(name);

	content_t content_id;
	if (alias_name != name) {
		if (!ndef->getId(alias_name, content_id))
			throw LuaError("Unknown node: " + alias_name +
					" (from alias " + name + ")");
	} else if (!ndef->getId(name, content_id)) {
		throw LuaError("Unknown node: " + name);
	}

	return content_id;
}

//~ typedef enum {
	//~ luaffi_ReturnStatus_fail,
	//~ luaffi_ReturnStatus_none,
	//~ luaffi_ReturnStatus_some,
//~ } luaffi_ReturnStatus;
//~ typedef enum {
	//~ luaffi_None,
	//~ luaffi_Some,
//~ } luaffi_Option;

//~ typedef struct {
	//~ bool is_some;

	//~ const char *name;
	//~ u64 name_l;
	//~ u8 param1;
	//~ u8 param2;
//~ } luaffi_OptionNode;

typedef struct {
	const char *name;
	u64 name_l;
	u8 param1;
	u8 param2;
} luaffi_Node;

// returns bitmask of which return values were written
int luaffi_get_node_raw(luaffi_Node &ret1, ScriptApiBase *sab, s16 x, s16 y, s16 z)
{
	GET_ENV_PTR;

	MapNode n = env->getMap().getNode(v3s16(x, y, z));

	// Return node
	const std::string &name = env->getGameDef()->ndef()->get(n).name;
	ret1.name = name.c_str();
	ret1.name_l = name.length();
	ret1.param1 = n.getParam1();
	ret1.param2 = n.getParam2();

	return 1;
}

//~ luaffi_OptionNode luaffi_get_node_raw(ScriptApiBase *sab, s16 x, s16 y, s16 z)
//~ {
	//~ GET_ENV_PTR;

	//~ MapNode n = env->getMap().getNode(v3s16(x, y, z));

	//~ // Return node
	//~ const std::string &name = env->getGameDef()->ndef()->get(n).name;
	//~ return {true, name.c_str(), name.length(), n.getParam1(), n.getParam2()};
//~ }

//~ luaffi_OptionNode luaffi_get_node_or_nil_raw(ScriptApiBase *sab, s16 x, s16 y, s16 z)
//~ {
	//~ GET_ENV_PTR;

	//~ bool pos_ok;
	//~ MapNode n = env->getMap().getNode(v3s16(x, y, z), &pos_ok);

	//~ if (!pos_ok)
		//~ return {false};

	//~ // Return node
	//~ const std::string &name = env->getGameDef()->ndef()->get(n).name;
	//~ return {true, name.c_str(), name.length(), n.getParam1(), n.getParam2()};
//~ }

} // extern "C"
