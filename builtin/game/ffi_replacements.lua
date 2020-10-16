
-- If luajit is enabled, replace commonly used functions with ffi calls, for speed.
-- see also: src/script/ffi_for_lua.cpp

local insecure_env = ...

if not rawget(_G, "jit") then
	-- luajit is not enabled
	return
end

if not insecure_env or not insecure_env.require then
	error("*builtin* is missing insecure environment")
end

-- uncomment to turn off
--~ do return end

-- for benchmarking:
-- https://github.com/Desour/luajit_ffi_test_test

core.log("info", "using luajit ffi")

local ffi = insecure_env.require("ffi")

--~ print(dump(ffi))

ffi.cdef[[
void *dereference(void **ptr) {
	return *ptr;
}

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef struct ScriptApiBase ScriptApiBase;

extern const int luaffi_CUSTOM_RIDX_SCRIPTAPI;
extern const bool luaffi_INDIRECT_SCRIPTAPI_RIDX;

int printf(const char *fmt, ...);
void luaffi_test();
void luaffi_print_to_errstream(const char *s);

void luaffi_set_script_api_base(ScriptApiBase *sab);

int luaffi_get_content_id(ScriptApiBase *sab, const char *name_c);
//int luaffi_get_content_id(const char *name_c);


typedef struct {
	const char *name;
	u64 name_l;
	u8 param1;
	u8 param2;
} luaffi_Node;

//luaffi_OptionNode luaffi_get_node_raw(ScriptApiBase *sab, s16 x, s16 y, s16 z);
int luaffi_get_node_raw(luaffi_Node &ret1, ScriptApiBase *sab, s16 x, s16 y, s16 z);
]]

local C = ffi.C

C.printf("Hello %s!\n", "world")
C.luaffi_test()
C.luaffi_print_to_errstream("test2")


local s_rawget = insecure_env.rawget -- _G.rawget can be overridden

local registry = insecure_env.debug.getregistry()

local get_script_api_base
local CUSTOM_RIDX_SCRIPTAPI = C.luaffi_CUSTOM_RIDX_SCRIPTAPI
local sab_type = ffi.typeof("ScriptApiBase *")
if not C.luaffi_INDIRECT_SCRIPTAPI_RIDX then
	function get_script_api_base()
		local ud = s_rawget(registry, CUSTOM_RIDX_SCRIPTAPI)
		print(type(ud))
		print(ffi.alignof(sab_type))
		print(ffi.sizeof(sab_type))
		print(ffi.istype(sab_type, ud))
		return sab_type(ud)
		--~ return sab_type(s_rawget(registry, CUSTOM_RIDX_SCRIPTAPI))
	end
else
	function get_script_api_base()
		return sab_type(C.dereference(s_rawget(registry, CUSTOM_RIDX_SCRIPTAPI)))
	end
end

local sab = get_script_api_base()
C.luaffi_set_script_api_base(sab)
print(type(sab))

function core.get_content_id(name)
	--~ return C.luaffi_get_content_id(get_script_api_base(), name)
	--~ return C.luaffi_get_content_id(sab, name)
	--~ local s = get_script_api_base()
	--~ local result = C.luaffi_get_content_id(s, name)
	local result = C.luaffi_get_content_id(sab, name)
	--~ local result = C.luaffi_get_content_id(name)
	return result
end

local luaffi_Node = ffi.typeof("luaffi_Node")

function core.get_node(pos)
	local ret1 = luaffi_Node()
	local rets = C.luaffi_get_node_raw(ret1, sab, pos.x, pos.y, pos.z)
	return rets == 1 and
			{name = ffi.string(ret1.name, ret1.name_l), param1 = ret1.param1, param2 = ret1.param2}
		or nil
end

--~ function core.get_node(pos)
	--~ local val = C.luaffi_get_node_raw(sab, pos.x, pos.y, pos.z)
	--~ return val.is_some and
			--~ {name = ffi.string(val.name, val.name_l), param1 = val.param1, param2 = val.param2}
		--~ or nil
--~ end

--~ function core.get_node_or_nil(pos)
	--~ local val = C.luaffi_get_node_or_nil_raw(sab, pos.x, pos.y, pos.z)
	--~ return val.is_some and
			--~ {name = ffi.string(val.name, val.name_l), param1 = val.param1, param2 = val.param2}
		--~ or nil
--~ end
