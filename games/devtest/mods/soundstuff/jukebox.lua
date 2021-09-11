
--[[
The jukebox has lua code to play sounds (a special GUI would have no benefit).
The code can return a handle that can be used to stop the sound via the Stop button.
This node could be generalized to a lua block at some point.

Btw., this code contains much insecure stuff, please don't blindly copy to a mod.
]]

-- sound handle by hased node pos
local playing_handles = {}

local function stop_sound_at(pos)
	local hash = minetest.hash_node_position(pos)
	local handle = playing_handles[hash]
	if handle then
		minetest.sound_stop(handle)
		playing_handles[hash] = nil
	end
end

-- for some reason minetest.write_json returns "null" for {}, so we need to fix
-- this special case
local function my_write_json(list)
	if #list == 0 then
		return "[]"
	else
		return minetest.write_json(list)
	end
end

-- do not change indentation
-- (tabs don't work well in textarea, therefore spaces are used)
-- (and if you indent everything, including the `[[` and `]]`, the string will
-- also be indented)
local default_code = [[
local data = ...
local player = data.player -- the clicker
local playername = player:get_player_name()
local pos = data.pos -- pos of jukebox

local spec = {
  name = "soundstuff_mono",
  gain = 1.0,
  pitch = 1.0,
}

local parameters = {
  gain  = 1.0,
  pitch = 1.0,
  fade  = 0.0,
  to_player = nil,
  loop = false,
  pos = nil,
  max_hear_distance = 32,
  time_offset = 0.0,
  object = nil,
  exclude_player = nil,
}

local handle = minetest.sound_play(spec, parameters, false)

if false then
  local after = 0.5
  local fade = 1.0
  local gain = 2.0
  minetest.after(after, minetest.sound_fade, handle, fade, gain)
end

return {handle = handle}
]]

local function show_formspec(pos, player)
	local meta = minetest.get_meta(pos)
	if meta:get_string("version") ~= "1.0" then
		minetest.log("error", "Unknown jukebox version. Please place a new one.")
		return
	end

	local stored_sounds = assert(minetest.parse_json(meta:get_string("stored_sounds")))
	local selected_sound_idx = meta:get_int("selected_sound")
	local selected_sound = stored_sounds[selected_sound_idx]
			or {name = "<unnamed>", code = default_code}

	local fs = {}
	local function fs_add(str)
		table.insert(fs, str)
	end

	fs_add([[
		formspec_version[5]
		size[20,15]
	]])

	fs_add("textlist[0,0;4,15;txtlst;")
	for i = 1, #stored_sounds do
		if i ~= 1 then
			fs_add(",")
		end
		fs_add(minetest.formspec_escape(stored_sounds[i].name))
	end
	fs_add(";")
	fs_add(tostring(selected_sound_idx))
	fs_add(";false]")

	fs_add("container[4,0]")

	fs_add([[
		container[7,13]
			button[0,0;2,1;btn_stop;Stop]
			button[3,0;2,1;btn_play;Play]
			button_exit[6,0;2,1;btn_save;Save]
		container_end[]
		container[0.5,10.5]
			button[0,0;3,1;btn_new;New]
			button[0,1.5;3,1;btn_delete;Delete]
			button[0,3;3,1;btn_duplicate;Duplicate]
		container_end[]
	]])

	fs_add("field[0.5,1;3,1;fld_name;Name;")
	fs_add(minetest.formspec_escape(selected_sound.name))
	fs_add("]")
	fs_add("field_close_on_enter[fld_name;false]")

	fs_add("box[4,1;11,11;#000F]") -- background for txtarea_code
	fs_add("style[txtarea_code;font=mono]")
	fs_add("textarea[4,1;11,11;txtarea_code;Lua code here:;")
	fs_add(minetest.formspec_escape(selected_sound.code))
	fs_add("]")

	fs_add("container_end[]")

	fs = table.concat(fs)
	minetest.show_formspec(player:get_player_name(),
			string.format("soundstuff:jukebox_%d", minetest.hash_node_position(pos)),
			fs)
end

minetest.register_node("soundstuff:jukebox", {
	description = "Jukebox",
	tiles = {"soundstuff_jukebox.png"},
	groups = {dig_immediate = 2},

	on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("version", "1.0")
		meta:set_string("stored_sounds", "[]") -- json
		meta:set_int("selected_sound", "0") -- indices start at 1
		meta:mark_as_private({"version", "stored_sounds", "selected_sound"})
	end,

	on_rightclick = function(pos, node, clicker, itemstack, pointed_thing)
		show_formspec(pos, clicker)
	end,
})

minetest.register_on_player_receive_fields(function(player, formname, fields)
	if formname:sub(1, 19) ~= "soundstuff:jukebox_" then
		return
	end

	local pos_hash = tonumber(formname:sub(20))
	local pos = minetest.get_position_from_hash(pos_hash)
	assert(minetest.get_node(pos).name == "soundstuff:jukebox")

	local meta = minetest.get_meta(pos)

	local stored_sounds = assert(minetest.parse_json(meta:get_string("stored_sounds")))
	local selected_sound_idx = meta:get_int("selected_sound")
	local selected_sound = stored_sounds[selected_sound_idx]

	if not selected_sound then
		selected_sound = {name = "<unnamed>", code = default_code}
		selected_sound_idx = #stored_sounds + 1
		stored_sounds[selected_sound_idx] = selected_sound
		meta:set_int("selected_sound", selected_sound_idx)
		fields.new = false
		fields.duplicate = false
	end

	if fields.fld_name then
		selected_sound.name = fields.fld_name
	end

	if fields.txtarea_code then
		selected_sound.code = fields.txtarea_code
	end

	if fields.btn_new then
		table.insert(stored_sounds, {name = "<unnamed>", code = default_code})
		meta:set_int("selected_sound", #stored_sounds)
	end

	if fields.btn_duplicate then
		local new_idx = math.min(selected_sound_idx + 1, #stored_sounds)
		-- no need to table.copy because we serialize it to json anyways
		table.insert(stored_sounds, new_idx, selected_sound)
		meta:set_int("selected_sound", new_idx)
	end

	if fields.btn_delete then
		table.remove(stored_sounds, selected_sound_idx)
		meta:set_int("selected_sound", math.min(selected_sound_idx, #stored_sounds))
	end

	meta:set_string("stored_sounds", assert(my_write_json(stored_sounds)))

	if fields.txtlst then
		local explosion = minetest.explode_textlist_event(fields.txtlst)
		if explosion.type == "CHG" then
			-- go to other sound
			meta:set_int("selected_sound", explosion.index)
		end
	end

	if fields.btn_stop then
		stop_sound_at(pos)
	end

	if fields.btn_play then
		stop_sound_at(pos)
		local f, err = loadstring(selected_sound.code, "jukebox")
		if not f then
			minetest.chat_send_player(player:get_player_name(),
					"Error in your jukebox code: " .. err)
		else
			local ret = f({player = player, pos = pos})
			if ret and ret.handle then
				playing_handles[pos_hash] = ret.handle
			end
		end
	end

	if not fields.quit then
		show_formspec(pos, player)
	end

	return true
end)
