// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "tile.h"

// Sets everything else except the texture in the material
void TileLayer::applyMaterialOptions(video::SMaterial &material) const
{
	switch (material_type) {
	case TILE_MATERIAL_OPAQUE:
	case TILE_MATERIAL_LIQUID_OPAQUE:
	case TILE_MATERIAL_WAVING_LIQUID_OPAQUE:
		material.MaterialType = video::EMT_SOLID;
		break;
	case TILE_MATERIAL_BASIC:
	case TILE_MATERIAL_WAVING_LEAVES:
	case TILE_MATERIAL_WAVING_PLANTS:
	case TILE_MATERIAL_WAVING_LIQUID_BASIC:
		material.MaterialTypeParam = 0.5;
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		break;
	case TILE_MATERIAL_ALPHA:
	case TILE_MATERIAL_LIQUID_TRANSPARENT:
	case TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT:
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		break;
	default:
		break;
	}
	material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) != 0;
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
		material.TextureLayers[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	}
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
		material.TextureLayers[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

void TileLayer::applyMaterialOptionsWithShaders(video::SMaterial &material) const
{
	material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) != 0;
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
		material.TextureLayers[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayers[1].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	}
	if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
		material.TextureLayers[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayers[1].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	}
}

#include "filesys.h"
#include "log.h"
#include "client/client.h"
#include "settings.h"
#include "json/json.h"
#include "convert_json.h"
#include "nodedef.h"

void dump_nodedefs(Client *client)
{
	std::string dump_nodedefs_path = g_settings->get("secure.dump_nodedefs_path");
	if (dump_nodedefs_path.empty())
		return;

	if (fs::PathExists(dump_nodedefs_path)) {
		actionstream << "Didn't dump nodedefs, file already exits: "
				<< dump_nodedefs_path << std::endl;
		return;
	}
	auto os = open_ofstream(dump_nodedefs_path.c_str(), true);
	if (!os.good())
		return;

	Json::Value json_root;

	auto *nodedefmgr = client->getNodeDefManager();

	for (content_t id = 0; id < CONTENT_MAX; ++id) {
		auto &f = nodedefmgr->get(id);
		if (id > CONTENT_UNKNOWN && f.name == "unknown") {
			break;
		}

		Json::Value json_f;
		json_f["id"] = id;
		json_f["name"] = f.name;
		Json::Value json_minimap_color;
		json_minimap_color["r"] = f.minimap_color.getRed();
		json_minimap_color["g"] = f.minimap_color.getGreen();
		json_minimap_color["b"] = f.minimap_color.getBlue();
		json_minimap_color["a"] = f.minimap_color.getAlpha();
		json_f["minimap_color"] = std::move(json_minimap_color);

		json_root.append(std::move(json_f));
	}

	fastWriteJson(json_root, os);

	actionstream << "Dumped nodedefs to: " << dump_nodedefs_path << std::endl;
}
