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

extern "C" {

extern const int luaffi_CUSTOM_RIDX_SCRIPTAPI = CUSTOM_RIDX_SCRIPTAPI;
extern const bool luaffi_INDIRECT_SCRIPTAPI_RIDX = INDIRECT_SCRIPTAPI_RIDX;

static ScriptApiBase *s_script_api_base = NULL;
void luaffi_set_script_api_base(ScriptApiBase *sab) {
	s_script_api_base = sab;
}

void luaffi_test() {
	//~ printf("test() called.\n");
	errorstream << "test() called." << std::endl;
}

void luaffi_print_to_errstream(const char *s) {
	errorstream << s << std::endl;
}

int luaffi_get_content_id(ScriptApiBase *sab, const char *name_c) {
//~ int luaffi_get_content_id(const char *name_c) {
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

} // extern "C"
