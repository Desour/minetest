
This file provides information about what functions of the minetest lua api can be compiled by luajit 2.1 given that secure.luajit_ffi is enabled (similar to http://wiki.luajit.org/NYI).

Many functions that are marked as "never" might actually be compiled, the "never" just indicates that there probably won't be any effort on getting or keeping the function compiled.

"yes" means that the function is implemented in lua (can be a ffi wrapper) and can be completely compiled.

The purpose of this file is rather to act as a TODO-list than a reference to which functions to use.
Much of the supplied information might be outdated and wrong, and can only act as a hint. Please correct any wrong information.

The functions are ordered in the same way as in `lua_api.txt`.


# `minetest.*`

## Escape sequences

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_color_escape_sequence`              | yes        |
`colorize`                               | yes        |
`get_background_escape_sequence`         | yes        |
`strip_foreground_colors`                | no         |
`strip_background_colors`                | no         |
`strip_colors`                           | no         |

## Helper functions

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`wrap_text`                              | no         |
`pos_to_string`                          | yes        |
`string_to_pos`                          | no         |
`string_to_area`                         | no         |
`formspec_escape`                        | no         | documented twice (TODO)
`is_yes`                                 | no         |
`is_nan`                                 | yes        |
`get_us_time`                            | yes        |
`pointed_thing_to_face_pos`              | no         |
`get_dig_params`                         | no         |
`get_hit_params`                         | no         |

## Translations

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_translator`                         | no         |
`translate`                              | no         |
`get_translated_string`                  | no         |

## Utilities

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_current_modname`                    | no         |
`get_modpath`                            | no         |
`get_modnames`                           | no         |
`get_worldpath`                          | no         |
`is_singleplayer`                        | no         |
`has_feature`                            | partial    | only if arg is string
`get_player_information`                 | no         |
`mkdir`                                  | no         |
`get_dir_list`                           | no         |
`safe_file_write`                        | no         |
`get_version`                            | no         |
`sha1`                                   | no         |

## Logging

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`debug`                                  | no         |
`log`                                    | no         |

## Registration functions

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`register_node`                          | never      |
`register_craftitem`                     | never      |
`register_tool`                          | never      |
`override_item`                          | never      |
`unregister_item`                        | never      |
`register_entity`                        | never      |
`register_abm`                           | never      |
`register_lbm`                           | never      |
`register_alias`                         | never      |
`register_alias_force`                   | never      |
`register_ore`                           | never      |
`register_biome`                         | never      |
`unregister_biome`                       | never      |
`register_decoration`                    | never      |
`register_schematic`                     | never      |
`clear_registered_biomes`                | never      |
`clear_registered_decorations`           | never      |
`clear_registered_ores`                  | never      |
`clear_registered_schematics`            | never      |
`register_craft`                         | never      |
`clear_craft`                            | never      |
`register_chatcommand`                   | never      |
`override_chatcommand`                   | never      |
`unregister_chatcommand`                 | never      |
`register_privilege`                     | never      |
`register_authentication_handler`        | never      |

## Global callback registration functions

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`register_globalstep`                    | never      |
`register_on_mods_loaded`                | never      |
`register_on_shutdown`                   | never      |
`register_on_placenode`                  | never      |
`register_on_dignode`                    | never      |
`register_on_punchnode`                  | never      |
`register_on_generated`                  | never      |
`register_on_newplayer`                  | never      |
`register_on_punchplayer`                | never      |
`register_on_rightclickplayer`           | never      |
`register_on_player_hpchange`            | never      |
`register_on_dieplayer`                  | never      |
`register_on_respawnplayer`              | never      |
`register_on_prejoinplayer`              | never      |
`register_on_joinplayer`                 | never      |
`register_on_leaveplayer`                | never      |
`register_on_authplayer`                 | never      |
`register_on_auth_fail`                  | never      |
`register_on_cheat`                      | never      |
`register_on_chat_message`               | never      |
`register_on_chatcommand`                | never      |
`register_on_player_receive_fields`      | never      |
`register_on_craft`                      | never      |
`register_craft_predict`                 | never      |
`register_allow_player_inventory_action` | never      |
`register_on_player_inventory_action`    | never      |
`register_on_protection_violation`       | never      |
`register_on_item_eat`                   | never      |
`register_on_priv_grant`                 | never      |
`register_on_priv_revoke`                | never      |
`register_can_bypass_userlimit`          | never      |
`register_on_modchannel_message`         | never      |

## Setting-related

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`setting_get`                            | never      | deprecated
`setting_get_pos`                        | no         |
`setting_getbool`                        | never      | deprecated
`setting_save`                           | never      | deprecated
`setting_set`                            | never      | deprecated
`setting_setbool`                        | never      | deprecated

## Authentication

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`string_to_privs`                        | no         |
`privs_to_string`                        | no         |
`get_player_privs`                       | no         |
`check_player_privs`                     | no         |
`check_password_entry`                   | no         |
`get_password_hash`                      | no         |
`get_player_ip`                          | no         |
`get_auth_handler`                       | yes        |
`notify_authentication_modified`         | no         |
`set_player_password`                    | maybe      | depends on insalled auth handler
`set_player_privs`                       | maybe      | depends on insalled auth handler
`auth_reload`                            | maybe      | depends on insalled auth handler

## Chat

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`chat_send_all`                          | no         |
`chat_send_player`                       | no         |
`format_chat_message`                    | no         | function might be overridden

## Environment access

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`set_node`                               | yes        |
`add_node`                               | yes        |
`bulk_set_node`                          | no         |
`swap_node`                              | yes        |
`remove_node`                            | yes        |
`get_node`                               | yes        |
`get_node_or_nil`                        | yes        |
`get_node_light`                         | no         |
`get_natural_light`                      | no         |
`get_artificial_light`                   | no         |
`place_node`                             | no         |
`dig_node`                               | no         |
`punch_node`                             | no         |
`spawn_falling_node`                     | no         |
                                         |            |
`find_nodes_with_meta`                   | no         |
`get_meta`                               | no         |
`get_node_timer`                         | no         |
                                         |            |
`add_entity`                             | no         |
`add_item`                               | no         |
`get_player_by_name`                     | no         |
`get_objects_inside_radius`              | no         |
`get_objects_in_area`                    | no         |
`set_timeofday`                          | no         |
`get_timeofday`                          | no         |
`get_gametime`                           | no         |
`get_day_count`                          | no         |
`find_node_near`                         | no         |
`find_nodes_in_area`                     | no         |
`find_nodes_in_area_under_air`           | no         |
`get_perlin`                             | no         |
`get_perlin_map`                         | no         | (in lua api somewhere else)
`get_voxel_manip`                        | no         |
`set_gen_notify`                         | no         |
`get_gen_notify`                         | no         |
`get_decoration_id`                      | no         |
`get_mapgen_object`                      | no         |
`get_heat`                               | no         |
`get_humidity`                           | no         |
`get_biome_data`                         | no         |
`get_biome_id`                           | no         |
`get_biome_name`                         | no         |
`get_mapgen_params`                      | no         |
`set_mapgen_params`                      | no         |
`get_mapgen_setting`                     | no         |
`get_mapgen_setting_noiseparams`         | no         |
`set_mapgen_setting`                     | no         |
`set_mapgen_setting_noiseparams`         | no         |
`set_noiseparams`                        | no         |
`get_noiseparams`                        | no         |
`generate_ores`                          | no         |
`generate_decorations`                   | no         |
`clear_objects`                          | no         |
`load_area`                              | no         |
`emerge_area`                            | no         |
`delete_area`                            | no         |
`line_of_sight`                          | no         |
`raycast`                                | no         |
`find_path`                              | no         |
`spawn_tree`                             | no         |
`transforming_liquid_add`                | no         |
`get_node_max_level`                     | no         |
`get_node_level`                         | no         |
`set_node_level`                         | no         |
`add_node_level`                         | no         |
`fix_light`                              | no         |
`check_single_for_falling`               | no         |
`check_for_falling`                      | no         |
`get_spawn_level`                        | no         |

## Mod channels

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`mod_channel_join`                       | no         |

## Inventory

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_inventory`                          | no         |
`create_detached_inventory`              | no         |
`remove_detached_inventory`              | no         |
`do_item_eat`                            | no         |

## Formspec

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`show_formspec`                          | no         |
`close_formspec`                         | no         |
`formspec_escape`                        | ->         | documented twice (TODO)
`explode_table_event`                    | no         |
`explode_textlist_event`                 | no         |
`explode_scrollbar_event`                | no         |

## Item handling

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`inventorycube`                          | no         |
`get_pointed_thing_position`             | partial    | not for pointed things of type "object"
`dir_to_facedir`                         | yes        |
`facedir_to_dir`                         | yes        |
`dir_to_wallmounted`                     | yes        |
`wallmounted_to_dir`                     | yes        |
`dir_to_yaw`                             | yes        |
`yaw_to_dir`                             | yes        |
`is_colored_paramtype`                   | yes        |
`strip_param2_color`                     | yes        |
`get_node_drops`                         | no         |
`get_craft_result`                       | no         |
`get_craft_recipe`                       | no         |
`get_all_craft_recipes`                  | no         |
`handle_node_drops`                      | no         | can be overridden
`itemstring_with_palette`                | no         |
`itemstring_with_color`                  | no         |

## Rollback

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`rollback_get_node_actions`              | no         |
`rollback_revert_actions_by`             | no         |
`rollback_get_last_node_actor`           | never      | deprecated

## Defaults for the `on_place` and `on_drop` item definition functions

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`item_place_node`                        | no         |
`item_place_object`                      | no         |
`item_place`                             | no         |
`item_drop`                              | no         |
`item_eat`                               | no         |
`item_secondary_use`                     | yes        | undocumented function (TODO)

## Defaults for the `on_punch` and `on_dig` node definition callbacks

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`node_punch`                             | maybe      | depends on registered_on_punchnodes callbacks
`node_dig`                               | no         |

## Sounds

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`sound_play`                             | no         |
`sound_stop`                             | no         |
`sound_fade`                             | no         |

## Timing

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`after`                                  | no         | the cancel function in the returned table can not be compiled either

## Server

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`request_shutdown`                       | no         |
`cancel_shutdown_requests`               | no         |
`get_server_status`                      | no         |
`get_server_uptime`                      | no         |
`remove_player`                          | no         |
`remove_player_auth`                     | no         |
`dynamic_add_media`                      | no         |

## Bans

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_ban_list`                           | no         |
`get_ban_description`                    | no         |
`ban_player`                             | no         |
`unban_player_or_ip`                     | no         |
`kick_player`                            | no         |

## Particles

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`add_particle`                           | no         |
`add_particlespawner`                    | no         |
`delete_particlespawner`                 | no         |

## Schematics

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`create_schematic`                       | no         |
`place_schematic`                        | no         |
`place_schematic_on_vmanip`              | no         |
`serialize_schematic`                    | no         |
`read_schematic`                         | no         |

## HTTP Requests

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`request_http_api`                       | no         |

## Storage API

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_mod_storage`                        | no         |

## Misc.

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_connected_players`                  | no         |
`is_player`                              | partial    | for fake players maybe
`player_exists`                          | no         |
`hud_replace_builtin`                    | no         |
`send_join_message`                      | no         |
`send_leave_message`                     | no         |
`hash_node_position`                     | yes        |
`get_position_from_hash`                 | yes        |
`get_item_group`                         | yes        |
`get_node_group`                         | never      | deprecated
`raillike_group`                         | yes        |
`get_content_id`                         | no         |
`get_name_from_content_id`               | no         |
`parse_json`                             | no         |
`write_json`                             | no         |
`serialize`                              | no         |
`deserialize`                            | no         |
`compress`                               | no         |
`decompress`                             | no         |
`rgba`                                   | yes        |
`encode_base64`                          | no         |
`decode_base64`                          | no         |
`is_protected`                           | yes        | can be overridden
`record_protection_violation`            | maybe      | depends on registered callbacks
`is_creative_enabled`                    | yes        | can be overridden
`is_area_protected`                      | yes        | can be overridden
`rotate_and_place`                       | no         |
`rotate_node`                            | no         |
                                         |            |
`calculate_knockback`                    | no         | can be overridden
                                         |            |
`forceload_block`                        | no         |
`forceload_free_block`                   | no         |
                                         |            |
`request_insecure_environment`           | never      |
                                         |            |
`global_exists`                          | yes        |

## Undocumented functions (stuff from builtin)

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`craft_predict`                          | maybe      | depends on registered callbacks
`get_builtin_path`                       | no         |
`get_last_run_mod`                       | no         |
`get_player_radius_area`                 | no         |
`get_user_path`                          | no         |
`http_add_fetch`                         | no         |
`on_craft`                               | maybe      | depends on registered callbacks
`run_callbacks`                          | maybe      | depends on registered callbacks
`run_priv_callbacks`                     | maybe      | depends on registered callbacks
`set_last_run_mod`                       | yes        |
`spawn_item`                             | no         | (why is there .add_item ?! TODO)
`create_detached_inventory_raw`          | no         |
`remove_detached_inventory_raw`          | no         |
`register_item`                          | never      |
`register_on_mapgen_init`                | no         |
`register_playerevent`                   | never      |
`registered_on_player_hpchange`          | maybe      | depends on registered callbacks



# Other and classes

## Helper functions

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`dump2`                                  | never      |
`dump`                                   | never      |
`math.hypot`                             | yes        |
`math.sign`                              | yes        |
`math.factorial`                         | yes        |
`string.split`                           | yes        |
`string.trim`                            | no         |
`table.copy`                             | no         |
`table.indexof`                          | yes        |
`table.insert_all`                       | yes        |
`table.key_value_swap`                   | no         |
`table.shuffle`                          | yes        |

## `vector.*`

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`new`                                    | yes        |
`direction`                              | yes        |
`distance`                               | yes        |
`length`                                 | yes        |
`normalize`                              | yes        |
`floor`                                  | yes        |
`round`                                  | yes        |
`apply`                                  | yes        |
`equals`                                 | yes        |
`sort`                                   | yes        |
`angle`                                  | yes        |
`dot`                                    | yes        |
`cross`                                  | yes        |
`offset`                                 | yes        |
`add`                                    | yes        |
`subtract`                               | yes        |
`multiply`                               | yes        |
`divide`                                 | yes        |
`rotate`                                 | yes        |
`rotate_around_axis`                     | yes        |
`dir_to_rotation`                        | yes        |

## HTTPApiTable

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`fetch`                                  | no         |
`fetch_async`                            | no         |
`fetch_async_get`                        | no         |

## VoxelManip

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
constructor                              | no         |
`read_from_map`                          | no         |
`write_to_map`                           | no         |
`get_node_at`                            | no         |
`set_node_at`                            | no         |
`get_data`                               | no         |
`set_data`                               | no         |
`update_map`                             | no         |
`set_lighting`                           | no         |
`get_light_data`                         | no         |
`set_light_data`                         | no         |
`get_param2_data`                        | no         |
`set_param2_data`                        | no         |
`calc_lighting`                          | no         |
`update_liquids`                         | no         |
`was_modified`                           | no         |
`get_emerged_area`                       | no         |

## VoxelArea

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`new`                                    | yes        |
`getExtent`                              | yes        |
`getVolume`                              | yes        |
`index`                                  | yes        |
`indexp`                                 | yes        |
`position`                               | yes        |
`contains`                               | yes        |
`containsp`                              | yes        |
`containsi`                              | yes        |
`iter`                                   | no         | but the returned function can be compiled
`iterp`                                  | no         | same as above

## AreaStore

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_area`                               | no         |
`get_areas_for_pos`                      | no         |
`get_areas_in_area`                      | no         |
`insert_area`                            | no         |
`reserve`                                | no         |
`remove_area`                            | no         |
`set_cache_params`                       | no         |
`to_string`                              | no         |
`to_file`                                | no         |
`from_file`                              | no         |

## InvRef

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`is_empty`                               | no         |
`get_size`                               | no         |
`set_size`                               | no         |
`get_width`                              | no         |
`set_width`                              | no         |
`get_stack`                              | no         |
`set_stack`                              | no         |
`get_list`                               | no         |
`set_list`                               | no         |
`get_lists``                             | no         |
`set_lists`                              | no         |
`add_item`                               | no         |
`room_for_item`                          | no         |
`contains_item`                          | no         |
`remove_item`                            | no         |
`get_location`                           | no         |

## ItemStack

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`is_empty`                               | no         |
`get_name`                               | no         |
`set_name`                               | no         |
`get_count`                              | no         |
`set_count`                              | no         |
`get_wear`                               | no         |
`set_wear`                               | no         |
`get_meta`                               | no         |
`get_metadata`                           | no         |
`set_metadata`                           | no         |
`get_description`                        | no         |
`get_short_description`                  | no         |
`clear`                                  | no         |
`replace)`                               | no         |
`to_string`                              | no         |
`to_table`                               | no         |
`get_stack_max`                          | no         |
`get_free_space`                         | no         |
`is_known`                               | no         |
`get_definition`                         | no         |
`get_tool_capabilities`                  | no         |
`add_wear`                               | no         |
`add_item`                               | no         |
`item_fits`                              | no         |
`take_item`                              | no         |
`peek_item`                              | no         |

## ItemStackMetaRef

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`set_tool_capabilities`                  | no         |

## MetaDataRef

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`contains`                               | no         |
`get`                                    | no         |
`set_string`                             | no         |
`get_string`                             | no         |
`set_int`                                | no         |
`get_int`                                | no         |
`set_float`                              | no         |
`get_float`                              | no         |
`to_table`                               | no         |
`from_table`                             | no         |
`equals`                                 | no         |

## ModChannel

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`leave`                                  | no         |
`is_writeable`                           | no         |
`send_all`                               | no         |

## NodeMetaRef

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_inventory`                          | no         |
`mark_as_private`                        | no         |

## NodeTimerRef

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`set`                                    | no         |
`start`                                  | no         |
`stop`                                   | no         |
`get_timeout`                            | no         |
`get_elapsed`                            | no         |
`is_started`                             | no         |

## ObjectRef

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_pos`                                | yes        |
`set_pos`                                | yes        |
`get_velocity`                           | yes        |
`add_velocity`                           | yes        |
`move_to`                                | yes        |
`punch`                                  | no         |
`right_click`                            | no         |
`get_hp`                                 | no         |
`set_hp`                                 | no         |
`get_inventory`                          | no         |
`get_wield_list`                         | no         |
`get_wield_index`                        | no         |
`get_wielded_item`                       | no         |
`set_wielded_item`                       | no         |
`set_armor_groups`                       | no         |
`get_armor_groups`                       | no         |
`set_animation`                          | no         |
`get_animation`                          | no         |
`set_animation_frame_speed`              | no         |
`set_attach`                             | no         |
`get_attach`                             | no         |
`get_children`                           | no         |
`set_detach`                             | no         |
`set_bone_position`                      | no         |
`get_bone_position`                      | no         |
`set_properties`                         | no         |
`get_properties`                         | no         |
`is_player`                              | no         |
`get_nametag_attributes`                 | no         |
`set_nametag_attributes`                 | no         |
                                         |            |
`remove`                                 | no         |
`set_velocity`                           | yes        |
`set_acceleration`                       | yes        |
`get_acceleration`                       | yes        |
`set_rotation`                           | no         |
`get_rotation`                           | no         |
`set_yaw`                                | no         |
`get_yaw`                                | no         |
`set_texture_mod`                        | no         |
`get_texture_mod`                        | no         |
`set_sprite`                             | no         |
`get_entity_name`                        | never      | deprecated
`get_luaentity`                          | no         |
                                         |            |
`get_player_name`                        | no         |
`get_player_velocity`                    | no         |
`add_player_velocity`                    | no         |
`get_look_dir`                           | no         |
`get_look_vertical`                      | no         |
`get_look_horizontal`                    | no         |
`set_look_vertical`                      | no         |
`set_look_horizontal`                    | no         |
`get_look_pitch`                         | no         |
`get_look_yaw`                           | no         |
`set_look_pitch`                         | no         |
`set_look_yaw`                           | no         |
`get_breath`                             | no         |
`set_breath`                             | no         |
`set_fov`                                | no         |
`get_fov`                                | no         |
`set_attribute`                          | no         |
`get_attribute`                          | no         |
`get_meta`                               | no         |
`set_inventory_formspec`                 | no         |
`get_inventory_formspec`                 | no         |
`set_formspec_prepend`                   | no         |
`get_formspec_prepend`                   | no         |
`get_player_control`                     | no         |
`get_player_control_bits`                | no         |
`set_physics_override`                   | no         |
`get_physics_override`                   | no         |
`hud_add`                                | no         |
`hud_remove`                             | no         |
`hud_change`                             | no         |
`hud_get`                                | no         |
`hud_set_flags`                          | no         |
`hud_get_flags`                          | no         |
`hud_set_hotbar_itemcount`               | no         |
`hud_get_hotbar_itemcount`               | no         |
`hud_set_hotbar_image`                   | no         |
`hud_get_hotbar_image`                   | no         |
`hud_set_hotbar_selected_image`          | no         |
`hud_get_hotbar_selected_image`          | no         |
`set_minimap_modes`                      | no         |
`set_sky`                                | no         |
`get_sky`                                | no         |
`get_sky_color`                          | no         |
`set_sun`                                | no         |
`get_sun`                                | no         |
`set_moon`                               | no         |
`get_moon`                               | no         |
`set_stars`                              | no         |
`get_stars`                              | no         |
`set_clouds`                             | no         |
`get_clouds`                             | no         |
`override_day_night_ratio`               | no         |
`get_day_night_ratio`                    | no         |
`set_local_animation`                    | no         |
`get_local_animation`                    | no         |
`set_eye_offset`                         | no         |
`get_eye_offset`                         | no         |
`send_mapblock`                          | no         |

## PcgRandom

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`next`                                   | no         |
`rand_normal_dist`                       | no         |

## PerlinNoise

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
constructor                              | no         |
`get_2d`                                 | no         |
`get_3d`                                 | no         |

## PerlinNoiseMap

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
`get_2d_map`                             | no         |
`get_3d_map`                             | no         |
`get_2d_map_flat`                        | no         |
`get_3d_map_flat`                        | no         |
`calc_2d_map`                            | no         |
`calc_3d_map`                            | no         |
`get_map_slice`                          | no         |

## PseudoRandom

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
constructor                              | no         |
`next`                                   | no         |

## Raycast

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
constructor                              | no         |
`next`                                   | no         |

## SecureRandom

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
constructor                              | no         |
`next_bytes`                             | no         |

## Settings

Function                                 | Compiled?  | Remarks
-----------------------------------------|------------|-------------------------
constructor                              | no         |
`get`                                    | no         |
`get_bool`                               | no         |
`get_np_group`                           | no         |
`get_flags`                              | no         |
`set`                                    | no         |
`set_bool`                               | no         |
`set_np_group`                           | no         |
`remove`                                 | no         |
`get_names`                              | no         |
`write`                                  | no         |
`to_table`                               | no         |


# Misc.

- callbacks could use a pull-style api (eg. abms). this is corrently not done
