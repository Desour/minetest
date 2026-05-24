local builtin_shared = ...

-- ----------------------
-- TODO: move somewhere else

-- TODO: implement these in C++
function core.send_interact(action, pointed_thing)
end
function core.send_inv_action(iaction)
end

function core.item_place(_itemstack, pointed_thing)
	-- TODO: check if node below is rightclickable, do prediction, and other stuff
	core.send_interact("place", pointed_thing)
end

function core.item_secondary_use(_itemstack, pointed_thing)
	core.send_interact("activate", pointed_thing)
end

function core.item_drop(single_item)
	-- TODO (see Game::dropSelectedItem())
	core.send_inv_action({
		type = "drop",
		count = single_item and 1 or 0,
		from_inv = nil, -- TODO: player inv
		from_list = "main",
		from_i = nil, -- TODO: selected item idx
	})
end

-- ----------------------


-- TODO
-- also TODO: probably won't need to overwrite in item_s.lua
function core.get_content_id(name)
	return tonumber(name)
end
function core.get_name_from_content_id(id)
	return tostring(id)
end

-- complete the itemdefs
for name, def in pairs(core.registered_items) do
	def.on_place = core.item_place -- TODO: do not split rightclick and place?
	def.on_secondary_use = core.item_secondary_use
	def.on_drop = core.item_drop
	--TODO: should maybe also have default for on_use
end

-- complete the nodedefs
for name, def in pairs(core.registered_nodes) do
	--TODO
end
