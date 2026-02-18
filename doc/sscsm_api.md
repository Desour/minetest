# Server-sent client-side modding (SSCSM) API reference

**Warning:** SSCSM is very experimental. The API will break. Always start your
mod with a version check (i.e. at least check if `core.get_version().proto_max`
is (less or) equal to (any of) the tested version(s)).

In SSCSM, the server sends scripts to the client, which it executes
client-side (in a sandbox, see also `sscsm_security.md`).
As modder, you can add these scripts to your server-side mod, and tell the engine
to send them.

Please refer to `lua_api.md` for server-side modding.
(And refer to `client_lua_api.md` for client-provided client-side modding (CPCSM).)



## Loading mods

### Paths

SSCSM uses a virtual file system (just a dictionary of virtual paths (strings)
to file contents (strings)).

Each mod's files have paths of the form `modname:foo/bla.lua`.
Please don't rely on this, use `core.get_modpath()` instead.

The virtual file paths within a mod are meant to mimic the filepaths on the
server, for example `<modpath>/common/foo.lua` gets sent as `modname:common/foo.lua`.

The engine loads `modname:init.lua` for all mods, in server mod dependency order.

There is client and server builtin (modnames are `*client_builtin*` and
`*server_builtin*`). The server builtin is sent from the server, like any other
SSCSM, and the client builtin is located on the client.


### Mod sending API

Currently, you can not add any mods. There's only a small hardcoded preview script
in C++ which is loaded when you set `enable_sscsm` to `singleplayer`.



## Definition tables

### Item definition

The item definitions are initialized with the values set by the server mods.
Not all fields from the server are available on the client, those that are available
are listed here. Also, representation of values can differ (e.g. `{name = "mymod_foo.png"}`
vs. `"mymod_foo.png"`).
Significant differences, including client-only fields, are documented here. See
`lua_api.md` for omitted details of other fields.

FIXME: Some fields will be overwritable with `core.override_item`. They should
be marked as such below.

TODO: should `core.override_item` normalize the representation of overwritten values?
(would allow builtin and mods to rely on easy to read fields)

```lua
{
    description = "",
    short_description = "",
    groups = {},
    inventory_image = <Item image definition>,
    inventory_overlay = <Item image definition>,
    wield_image = <Item image definition>,
    wield_overlay = <Item image definition>,
    wield_scale = {x = 1, y = 1, z = 1},
    palette = "",
    color = "#ffffffff",
    stack_max = 99,
    range = 4.0,
    liquids_pointable = false,
    pointabilities = {
        nodes = {
            ["default:stone"] = "blocking",
            ["group:leaves"] = false,
        },
        objects = {
            ["modname:entityname"] = true,
            ["group:ghosty"] = true, -- (an armor group)
        },
    },
    light_source = 0,
    tool_capabilities = {
        full_punch_interval = 1.0,
        max_drop_level = 0,
        groupcaps = {
            -- For example:
            choppy = {times = {2.50, 1.40, 1.00}, uses = 20, maxlevel = 2},
        },
        damage_groups = {groupname = damage},
        punch_attack_uses = nil,
    },
    wear_color = {
        blend = "linear",
        color_stops = {
            [0.0] = "#ff0000",
            [0.5] = "#ffff00",
            [1.0] = "#00ff00",
        }
    },
    node_placement_prediction = nil,
    node_dig_prediction = "air", -- TODO: not only in nodedef?
    touch_interaction = <TouchInteractionMode> OR {
        pointed_nothing = <TouchInteractionMode>,
        pointed_node    = <TouchInteractionMode>,
        pointed_object  = <TouchInteractionMode>,
    },
    sound = {
        breaks = <SimpleSoundSpec>,
        eat = <SimpleSoundSpec>,
        punch_use = <SimpleSoundSpec>,
        punch_use_air = <SimpleSoundSpec>,
    },

    -- TODO: some callbacks should not be nil (even if set on server), others
    -- should, if set server-side, send an according interact packet

    _custom_field = whatever,
    -- Fields from the server will not be sent automatically.
}
```


### Node definition

Like item definitions, node definitions are initialized by the server, and some
fields can be overridden with `core.override_item`.

```lua
{
    -- <all fields allowed in item definitions>

    drawtype = "normal",
    visual_scale = 1.0,
    tiles = {tile definition 1, def2, def3, def4, def5, def6},
    overlay_tiles = {tile definition 1, def2, def3, def4, def5, def6},
    special_tiles = {tile definition 1, Tile definition 2},
    color = ColorSpec,
    use_texture_alpha = ..., --TODO: always string on client?
    palette = "",
    post_effect_color = "#00000000",
    post_effect_color_shaded = false,
    paramtype = "none",
    paramtype2 = "none",
    place_param2 = 0,
    wallmounted_rotate_vertical = false,
    is_ground_content = true,
    sunlight_propagates = false,
    walkable = true,
    pointable = true,
    diggable = true,
    climbable = false,
    move_resistance = 0,
    buildable_to = false,
    floodable = false,
    liquidtype = "none",
    liquid_alternative_flowing = "",
    liquid_alternative_source = "",
    liquid_viscosity = 0,
    liquid_renewable = true,
    liquid_move_physics = nil,
    leveled = 0,
    leveled_max = 127,
    liquid_range = 8,
    drowning = 0,
    damage_per_second = 0,
    node_box = {type = "regular"},
    connects_to = {},
    connect_sides = {},
    mesh = "",
    selection_box = nodebox,
    collision_box = nodebox,
    legacy_facedir_simple = false,
    legacy_wallmounted = false,
    waving = 0,
    sounds = {
        footstep = <SimpleSoundSpec>,
        dig = <SimpleSoundSpec> or "__group",
        dug = <SimpleSoundSpec>,
        place = <SimpleSoundSpec>,
        place_failed = <SimpleSoundSpec>,
        fall = <SimpleSoundSpec>,
    },
    drop = "",

    -- TODO: same as for itemdefs

    mod_origin = "modname",
}
```



## API

Unless noted otherwise, these work the same as in the server modding API.

Functions that take or return paths always use virtual paths.

### Global tables

#### Registered tables

* `core.registered_items`
* `core.registered_nodes`
* `core.registered_craftitems`
* `core.registered_tools`
* `core.registered_aliases`

#### Registered callback tables

* `core.registered_globalsteps`


### Global callbacks

* `core.register_globalstep(function(dtime))`


### SSCSM-specific API

* `core.get_node_or_nil(pos)`
* `core.get_content_id(name)`
* `core.get_name_from_content_id(id)`


### Util API

* `core.log([level,] text)`
* `core.get_us_time()`
  * Limited in precision.
* `core.parse_json(str[, nullvalue])`
* `core.write_json(data[, styled])`
* `core.is_yes(arg)`
* `core.compress(data, method, ...)`
* `core.decompress(data, method, ...)`
* `core.encode_base64(string)`
* `core.decode_base64(string)`
* `core.get_version()`
* `core.sha1(string, raw)`
* `core.sha256(string, raw)`
* `core.colorspec_to_colorstring(colorspec)`
* `core.colorspec_to_bytes(colorspec)`
* `core.colorspec_to_table(colorspec)`
* `core.time_to_day_night_ratio(time_of_day)`
* `core.get_last_run_mod()`
* `core.set_last_run_mod(modname)`
* `core.urlencode(value)`


### Other

* `core.get_current_modname()`
* `core.get_modpath(modname)`


### Builtin helpers

* `math.*` additions

* `vector.*`

* `core.global_exists(name)`

* `core.serialize(value)`
* `core.deserialize(str, safe)`

* `dump2(obj, name, dumped)`
* `dump(obj, dumped)`
* `string.*` additions
* `table.*` additions
* `core.formspec_escape(text)`
* `core.hypertext_escape(text)`
* `core.wrap_text(str, limit, as_table)`
* `core.explode_table_event(evt)`
* `core.explode_textlist_event(evt)`
* `core.explode_scrollbar_event(evt)`
* `core.rgba(r, g, b, a)`
* `core.pos_to_string(pos, decimal_places)`
* `core.string_to_pos(value)`
* `core.string_to_area(value, relative_to)`
* `core.get_color_escape_sequence(color)`
* `core.get_background_escape_sequence(color)`
* `core.colorize(color, message)`
* `core.strip_foreground_colors(str)`
* `core.strip_background_colors(str)`
* `core.strip_colors(str)`
* `core.translate(textdomain, str, ...)`
* `core.translate_n(textdomain, str, str_plural, n, ...)`
* `core.get_translator(textdomain)`
* `core.pointed_thing_to_face_pos(placer, pointed_thing)`
* `core.string_to_privs(str, delim)`
* `core.privs_to_string(privs, delim)`
* `core.is_nan(number)`
* `core.parse_relative_number(arg, relative_to)`
* `core.parse_coordinates(x, y, z, relative_to)`

* `core.inventorycube(img1, img2, img3)`
* `core.dir_to_facedir(dir, is6d)`
* `core.facedir_to_dir(facedir)`
* `core.dir_to_fourdir(dir)`
* `core.fourdir_to_dir(fourdir)`
* `core.dir_to_wallmounted(dir)`
* `core.wallmounted_to_dir(wallmounted)`
* `core.dir_to_yaw(dir)`
* `core.yaw_to_dir(yaw)`
* `core.is_colored_paramtype(ptype)`
* `core.strip_param2_color(param2, paramtype2)`

* `core.after(time, func, ...)`


### Lua standard library

* `assert`
* `collectgarbage`
* `error`
* `ipairs`
* `next`
* `pairs`
* `pcall`
* `print`
* `rawequal`
* `rawget`
* `rawset`
* `select`
* `getmetatable`
* `setmetatable`
* `tonumber`
* `tostring`
* `type`
* `unpack`
* `_VERSION`
* `xpcall`
* `dofile`
  * Overwritten: Loading bytecode is prohibited (like in SSM).
* `load`
  * As above.
* `loadfile`
  * As above.
* `loadstring`
  * As above.
* `coroutine.*`
* `table.*`
* `math.*`
* `string.*`
  * except `string.dump`
* `os.difftime`
* `os.time`
* `os.clock`
  * Reduced precision.
* `debug.traceback`


### LuaJIT `jit` library

* `jit.arch`
* `jit.flush`
* `jit.off`
* `jit.on`
* `jit.opt`
* `jit.os`
* `jit.status`
* `jit.version`
* `jit.version_num`


### Bit library

* `bit.*`


### API only for client builtin

* `core.get_builtin_path()`
  * Returns path, depending on which builtin currently loads, or `nil`.
* `debug.getinfo(...)`
* `INIT`
  * Is `"sscsm"`.
