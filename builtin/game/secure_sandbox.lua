
--[[
This file contains things for making the global env sandbox secure that weren't
done in C++.
]]

-- Overwrite debug.getmetatable and debug.setmetatable, so that they have the same
-- semantics with the metatable field "__metatable_debug" as getmetatable and
-- setmetatable have with the "__metatable" field.
-- If mod security is off, they are not restricted.
if minetest.settings:get_bool("secure.enable_security", true) then
	local debug_getmetatable_orig = debug.getmetatable
	local debug_setmetatable_orig = debug.setmetatable
	-- store error in local variable, so that mods can't overwrite it later
	local error = error

	debug.getmetatable = function(obj)
		local mt = debug_getmetatable_orig(obj)
		if mt.__metatable_debug == nil then
			return mt
		end
		return mt.__metatable_debug
	end

	debug.setmetatable = function(obj, mt)
		local current_mt = debug_getmetatable_orig(obj)
		if current_mt.__metatable_debug == nil then
			return debug_setmetatable_orig(obj, mt)
		end
		error("Overwriting a metatable with __metatable_debug is not allowed for debug.setmetatable.")
	end
end