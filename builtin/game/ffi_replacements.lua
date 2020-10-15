
-- If luajit is enabled, replace commonly used functions with ffi calls, for speed.
-- see also: src/script/ffi_for_lua.cpp

local insecure_env = ...

if not rawget(_G, "jit") then
	-- luajit is not enabled
	return
end

if not insecure_env or not insecure_env.require then
	-- can not work, needs *builtin* in secure.trusted_mods
	return
end

-- uncomment to turn off
--~ do return end

-- for benchmarking:
--[[
local time_start = minetest.get_us_time()
for i = 1, 10000000 do
	minetest.get_content_id("default:dirt")
end
local dtime = minetest.get_us_time() - time_start
minetest.log("action", "time taken: " .. (dtime / 1000) .. " ms")
]]

core.log("info", "using luajit ffi")

local ffi = insecure_env.require("ffi")

--~ print(dump(ffi))

ffi.cdef[[
void *dereference(void **ptr) {
	return *ptr;
}

typedef struct ScriptApiBase ScriptApiBase;

extern const int luaffi_CUSTOM_RIDX_SCRIPTAPI;
extern const bool luaffi_INDIRECT_SCRIPTAPI_RIDX;

int printf(const char *fmt, ...);
void luaffi_test();
void luaffi_print_to_errstream(const char *s);

int luaffi_get_content_id(ScriptApiBase *sab, const char *name_c);
]]

local C = ffi.C

C.printf("Hello %s!\n", "world")
C.luaffi_test()
C.luaffi_print_to_errstream("test2")


local registry = insecure_env.debug.getregistry()

local get_script_api_base
local CUSTOM_RIDX_SCRIPTAPI = C.luaffi_CUSTOM_RIDX_SCRIPTAPI
if not C.luaffi_INDIRECT_SCRIPTAPI_RIDX then
	function get_script_api_base()
		return rawget(registry, CUSTOM_RIDX_SCRIPTAPI)
	end
else
	function get_script_api_base()
		return C.dereference(rawget(registry, CUSTOM_RIDX_SCRIPTAPI))
	end
end

local sab = get_script_api_base()

function core.get_content_id(name)
	--~ return C.luaffi_get_content_id(get_script_api_base(), name)
	return C.luaffi_get_content_id(sab, name)
end
