/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "factory.h"
#include "log.h"
#include "plain.h"
#include "anaglyph.h"
#include "interlaced.h"
#include "sidebyside.h"
#include "secondstage.h"
#include "client/shadows/dynamicshadowsrender.h"

struct CreatePipelineResult
{
	v2f virtual_size_scale;
	std::unique_ptr<ShadowRenderer> shadow_renderer;
	std::unique_ptr<RenderPipeline> pipeline;
};

void createPipeline(const std::string &stereo_mode, IrrlichtDevice *device,
		Client *client, Hud *hud, CreatePipelineResult &result);

std::unique_ptr<RenderingCore> createRenderingCore(const std::string &stereo_mode,
		IrrlichtDevice *device, Client *client, Hud *hud)
{
	CreatePipelineResult created_pipeline;
	createPipeline(stereo_mode, device, client, hud, created_pipeline);
	return std::make_unique<RenderingCore>(device, client, hud,
			std::move(created_pipeline.shadow_renderer),
			std::move(created_pipeline.pipeline),
			created_pipeline.virtual_size_scale);
}

void createPipeline(const std::string &stereo_mode, IrrlichtDevice *device,
		Client *client, Hud *hud, CreatePipelineResult &result)
{
	result.shadow_renderer = createShadowRenderer(device, client);
	result.virtual_size_scale = v2f(1.0f);
	result.pipeline = std::make_unique<RenderPipeline>();

	if (result.shadow_renderer)
		result.pipeline->addStep<RenderShadowMapStep>();

	if (stereo_mode == "none") {
		populatePlainPipeline(result.pipeline.get(), client);
		return;
	}
	if (stereo_mode == "anaglyph") {
		populateAnaglyphPipeline(result.pipeline.get(), client);
		return;
	}
	if (stereo_mode == "interlaced") {
		populateInterlacedPipeline(result.pipeline.get(), client);
		return;
	}
	if (stereo_mode == "sidebyside") {
		populateSideBySidePipeline(result.pipeline.get(), client, false, false,
				result.virtual_size_scale);
		return;
	}
	if (stereo_mode == "topbottom") {
		populateSideBySidePipeline(result.pipeline.get(), client, true, false,
				result.virtual_size_scale);
		return;
	}
	if (stereo_mode == "crossview") {
		populateSideBySidePipeline(result.pipeline.get(), client, false, true,
				result.virtual_size_scale);
		return;
	}

	// fallback to plain renderer
	errorstream << "Invalid rendering mode: " << stereo_mode << std::endl;
	populatePlainPipeline(result.pipeline.get(), client);
}
