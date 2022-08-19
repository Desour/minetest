/*
Minetest
Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "collector.h"
#include <stdexcept>
#include "log.h"
#include "client/mesh.h"

static MapBlockMesh::Side check_side_hint(const TileLayer &layer, MapBlockMesh::Side side_hint)
{
	if (!(layer.material_flags & MATERIAL_FLAG_BACKFACE_CULLING))
		return MapBlockMesh::SIDE_ALWAYS;
	// TODO: waving
	return side_hint;
}

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, MapBlockMesh::Side side_hint)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, side_hint,
				layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor color, u8 light_source, MapBlockMesh::Side side_hint)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, pos, color,
				light_source, side_hint, layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, MapBlockMesh::Side side_hint,
		u8 layernum, bool use_scale)
{
	auto side = check_side_hint(layer, side_hint);
	PreMeshBuffer &pmb = findBuffer(layer, side, layernum, numVertices);

	f32 scale = 1.0f;
	if (use_scale)
		scale = 1.0f / layer.scale;

	u32 vertex_count = pmb.vertices.size();
	for (u32 i = 0; i < numVertices; i++)
		pmb.vertices.emplace_back(vertices[i].Pos, vertices[i].Normal,
				vertices[i].Color, scale * vertices[i].TCoords);

	for (u32 i = 0; i < numIndices; i++)
		pmb.indices.push_back(indices[i] + vertex_count);
}

void MeshCollector::append(const TileLayer &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor color, u8 light_source, MapBlockMesh::Side side_hint, u8 layernum,
		bool use_scale)
{
	auto side = check_side_hint(layer, side_hint);
	PreMeshBuffer &pmb = findBuffer(layer, side, layernum, numVertices);

	f32 scale = 1.0f;
	if (use_scale)
		scale = 1.0f / layer.scale;

	u32 vertex_count = pmb.vertices.size();
	for (u32 i = 0; i < numVertices; i++) {
		if (!light_source)
			applyFacesShading(color, vertices[i].Normal);
		pmb.vertices.emplace_back(vertices[i].Pos + pos, vertices[i].Normal, color,
				scale * vertices[i].TCoords);
	}

	for (u32 i = 0; i < numIndices; i++)
		pmb.indices.push_back(indices[i] + vertex_count);
}

PreMeshBuffer &MeshCollector::findBuffer(const TileLayer &layer, MapBlockMesh::Side side,
		u8 layernum, u32 numVertices)
{
	if (numVertices > U16_MAX)
		throw std::invalid_argument(
				"Mesh can't contain more than 65536 vertices");
	std::vector<PreMeshBuffer> &buffers =
			prebuffers_per_side[side][layernum];
	for (PreMeshBuffer &pmb : buffers)
		if (pmb.layer == layer && pmb.vertices.size() + numVertices <= U16_MAX)
			return pmb;
	buffers.emplace_back(layer);
	return buffers.back();
}
