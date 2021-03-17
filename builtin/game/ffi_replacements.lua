
--[[
Replace some of the minetest api functions with an implementation using luajit's
ffi library.

See also: doc/ffi_nyi.md

Warning: Do only modify if you know what you are doing.

Standard-library functions are accessed explicitly via _G to reduce the ease of
leaking stuff that we don't want to leak, ie.:
* the insecure environment _S
* anything created by the ffi library
]]

local _S = ...

-- Note: we know that core.settings:get_bool was not hooked because this is builtin
if not core.settings:get_bool("secure.use_luajit_ffi", false) then
	core.log("info", "Not luajit ffi (disabled)")
	return
end
if not core.global_exists("jit") then
	core.log("info", "secure.use_luajit_ffi enabled, but no luajit")
	return
end
core.log("info", "Using luajit ffi")

_S.assert(_S)

local ffi = _S.require("ffi")

ffi.cdef([[
typedef int8_t   u8;
typedef int16_t  u16;
typedef int32_t  u32;
typedef int64_t  u64;
typedef uint8_t  s8;
typedef uint16_t s16;
typedef uint32_t s32;
typedef uint64_t s64;

typedef struct ScriptApiCore ScriptApiCore;
typedef struct ObjectRef ObjectRef;

typedef int ffi_retint;

typedef struct {
	const char *data;
	u64 length;
} ffi_StringRef;

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

const char *ffi_get_errstr();

void ffi_test();

ffi_retint ffi_set_node_raw(ScriptApiCore *sac, double x, double y, double z, ffi_NodeRef *node);
ffi_retint ffi_swap_node(ScriptApiCore *sac, double x, double y, double z, ffi_NodeRef *node);
ffi_retint ffi_remove_node_raw(ScriptApiCore *sac, double x, double y, double z);
ffi_retint ffi_get_node(ScriptApiCore *sac, ffi_NodeRef *ret1, double x, double y, double z);
ffi_retint ffi_get_node_or_nil(ScriptApiCore *sac, ffi_NodeRef *ret1, double x, double y, double z);
u64 ffi_get_us_time();
void ffi_set_last_run_mod(ScriptApiCore *sac, const char *mod);
ffi_retint ffi_objref_get_pos(ffi_v3d *ret1, ObjectRef *ref);
ffi_retint ffi_objref_set_pos(ObjectRef *ref, double x, double y, double z);
ffi_retint ffi_objref_move_to(ObjectRef *ref, double x, double y, double z, bool continuous);
ffi_retint ffi_objref_add_velocity(ScriptApiCore *sac, ObjectRef *ref, double x, double y, double z);
ffi_retint ffi_objref_set_velocity(ObjectRef *ref, double x, double y, double z);
ffi_retint ffi_objref_get_velocity(ffi_v3d *ret1, ObjectRef *ref);
ffi_retint ffi_objref_set_acceleration(ObjectRef *ref, double x, double y, double z);
ffi_retint ffi_objref_get_acceleration(ffi_v3d *ret1, ObjectRef *ref);
]])

local C = ffi.C

local param_node_1 = ffi.new("ffi_NodeRef")
local param_v3d_1 = ffi.new("ffi_v3d")

C.ffi_test()

local sac = ffi.cast("ScriptApiCore **", _S.insec_private.get_script_api_core())[0]

local objref_metatable = _S.insec_private.get_objref_metatable()

-- {[<objref userdata>] = <ObjectRef * cdata>}
local objref_ptrs = {}
_S.setmetatable(objref_ptrs, {__mode = "k"}) -- make objref_ptrs key-weak

local function objref_check(obj)
	local ref = objref_ptrs[obj]
	if ref then
		return ref
	end
	_S.insec_private.objref_check(obj)
	ref = ffi.cast("ObjectRef **", obj)[0]
	objref_ptrs[obj] = ref
	return ref
end

local function handle_invalid_ffi_retint(ret)
	if ret == 0 then
		return nil
	elseif ret < 0 then
		_S.error(ffi.string(C.ffi_get_errstr()))
	else
		_S.error("invalid return value from ffi function")
	end
end

function core.set_node(pos, node)
	_S.assert(_S.type(node.name) == "string", "Node name is not set or is not a string!")

	-- do the same as ServerEnvironment::setNode (call on_destruct and co. callbacks)

	local n_old = core.get_node(pos)
	local ndef_old = minetest.registered_nodes[n_old.name]

	if ndef_old and ndef_old.on_destruct then
		ndef_old.on_destruct(vector.new(pos))
	end

	param_node_1.name.length = #node.name
	param_node_1.name.data = node.name
	param_node_1.param1 = node.param1 or 0
	param_node_1.param2 = node.param2 or 0

	local ret = C.ffi_set_node_raw(sac, pos.x or 0, pos.y or 0, pos.z or 0, param_node_1)

	if ret == 1 then
		-- it worked
	elseif ret == 0 then
		return false
	else
		return handle_invalid_ffi_retint(ret)
	end

	if ndef_old and ndef_old.after_destruct then
		ndef_old.after_destruct(vector.new(pos), n_old)
	end

	local ndef_new = minetest.registered_nodes[node.name]
	if ndef_new and ndef_new.on_construct then
		ndef_new.on_construct(vector.new(pos))
	end

	return true
end

function core.swap_node(pos, node)
	_S.assert(_S.type(node.name) == "string", "Node name is not set or is not a string!")

	param_node_1.name.length = #node.name
	param_node_1.name.data = node.name
	param_node_1.param1 = node.param1 or 0
	param_node_1.param2 = node.param2 or 0

	local ret = C.ffi_swap_node(sac, pos.x or 0, pos.y or 0, pos.z or 0, param_node_1)

	if ret == 1 then
		-- it worked
	elseif ret == 0 then
		return false
	else
		return handle_invalid_ffi_retint(ret)
	end

	return true
end

function core.add_node(pos, node)
	return core.set_node(pos, node)
end

function core.remove_node(pos)
	-- do the same as ServerEnvironment::removeNode (call on_destruct and co. callbacks)

	local n_old = core.get_node(pos)
	local ndef_old = minetest.registered_nodes[n_old.name]

	if ndef_old and ndef_old.on_destruct then
		ndef_old.on_destruct(vector.new(pos))
	end

	local ret = C.ffi_remove_node_raw(sac, pos.x or 0, pos.y or 0, pos.z or 0)

	if ret == 1 then
		-- it worked
	elseif ret == 0 then
		return false
	else
		return handle_invalid_ffi_retint(ret)
	end

	if ndef_old and ndef_old.after_destruct then
		ndef_old.after_destruct(vector.new(pos), n_old)
	end

	return true
end

function core.get_node(pos)
	local ret = C.ffi_get_node(sac, param_node_1, pos.x, pos.y, pos.z)
	if ret == 1 then
		return {
			name = ffi.string(param_node_1.name.data, param_node_1.name.length),
			param1 = param_node_1.param1,
			param2 = param_node_1.param2,
		}
	else
		return handle_invalid_ffi_retint(ret)
	end
end

function core.get_node_or_nil(pos)
	local ret = C.ffi_get_node_or_nil(sac, param_node_1, pos.x, pos.y, pos.z)
	if ret == 1 then
		return {
			name = ffi.string(param_node_1.name.data, param_node_1.name.length),
			param1 = param_node_1.param1,
			param2 = param_node_1.param2,
		}
	elseif ret == 0 then
		return nil
	else
		return handle_invalid_ffi_retint(ret)
	end
end

function core.get_us_time()
	return _S.tonumber(C.ffi_get_us_time())
end

function core.set_last_run_mod(mod)
	C.ffi_set_last_run_mod(sac, _S.type(mod) == "string" and mod or nil)
end


function objref_metatable.__index.get_pos(self)
	local ref = objref_check(self)
	local ret = C.ffi_objref_get_pos(param_v3d_1, ref)
	if ret == 1 then
		return vector.new(param_v3d_1.x, param_v3d_1.y, param_v3d_1.z)
	else
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.set_pos(self, pos)
	_S.assert(_S.type(pos) == "table")
	_S.assert(_S.type(pos.x) == "number")
	_S.assert(_S.type(pos.y) == "number")
	_S.assert(_S.type(pos.z) == "number")
	local ref = objref_check(self)
	local ret = C.ffi_objref_set_pos(ref, pos.x, pos.y, pos.z)
	if ret ~= 0 then
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.move_to(self, pos, continuous)
	_S.assert(_S.type(pos) == "table")
	_S.assert(_S.type(pos.x) == "number")
	_S.assert(_S.type(pos.y) == "number")
	_S.assert(_S.type(pos.z) == "number")
	local ref = objref_check(self)
	local ret = C.ffi_objref_move_to(ref, pos.x, pos.y, pos.z, continuous and true or false)
	if ret ~= 0 then
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.add_velocity(self, pos)
	_S.assert(_S.type(pos) == "table")
	_S.assert(_S.type(pos.x) == "number")
	_S.assert(_S.type(pos.y) == "number")
	_S.assert(_S.type(pos.z) == "number")
	local ref = objref_check(self)
	local ret = C.ffi_objref_add_velocity(ref, pos.x, pos.y, pos.z)
	if ret ~= 0 then
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.set_velocity(self, pos)
	_S.assert(_S.type(pos) == "table")
	_S.assert(_S.type(pos.x) == "number")
	_S.assert(_S.type(pos.y) == "number")
	_S.assert(_S.type(pos.z) == "number")
	local ref = objref_check(self)
	local ret = C.ffi_objref_set_velocity(ref, pos.x, pos.y, pos.z)
	if ret ~= 0 then
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.get_velocity(self)
	local ref = objref_check(self)
	local ret = C.ffi_objref_get_velocity(param_v3d_1, ref)
	if ret == 1 then
		return vector.new(param_v3d_1.x, param_v3d_1.y, param_v3d_1.z)
	else
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.set_acceleration(self, pos)
	_S.assert(_S.type(pos) == "table")
	_S.assert(_S.type(pos.x) == "number")
	_S.assert(_S.type(pos.y) == "number")
	_S.assert(_S.type(pos.z) == "number")
	local ref = objref_check(self)
	local ret = C.ffi_objref_set_acceleration(ref, pos.x, pos.y, pos.z)
	if ret ~= 0 then
		return handle_invalid_ffi_retint(ret)
	end
end

function objref_metatable.__index.get_acceleration(self)
	local ref = objref_check(self)
	local ret = C.ffi_objref_get_acceleration(param_v3d_1, ref)
	if ret == 1 then
		return vector.new(param_v3d_1.x, param_v3d_1.y, param_v3d_1.z)
	else
		return handle_invalid_ffi_retint(ret)
	end
end
