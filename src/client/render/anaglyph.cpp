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

#include "anaglyph.h"
#include <IrrlichtDevice.h>
#include "EMaterialProps.h"
#include "EVideoTypes.h"
#include "IVideoDriver.h"
#include "SMaterial.h"
#include "SOverrideMaterial.h"
#include "client/render/pipeline.h"
#include "client/render/plain.h"
#include "client/render/stereo.h"
#include "irrlichttypes_extrabloated.h"

class Client;


/// SetColorMaskStep step

SetColorMaskStep::SetColorMaskStep(int _color_mask)
	: color_mask(_color_mask)
{}

void SetColorMaskStep::run(PipelineContext &context)
{
	video::SOverrideMaterial &mat = context.device->getVideoDriver()->getOverrideMaterial();
	mat.reset();
	mat.Material.ColorMask = color_mask;
	mat.EnableProps = video::EMP_COLOR_MASK;
	mat.EnablePasses = scene::ESNRP_SKY_BOX | scene::ESNRP_SOLID |
			   scene::ESNRP_TRANSPARENT | scene::ESNRP_TRANSPARENT_EFFECT |
			   scene::ESNRP_SHADOW;
}

/// ClearDepthBufferTarget

ClearDepthBufferTarget::ClearDepthBufferTarget(RenderTarget *_target) :
	target(_target)
{}

void ClearDepthBufferTarget::activate(PipelineContext &context)
{
	target->activate(context);
	context.device->getVideoDriver()->clearBuffers(video::ECBF_DEPTH);
}

ConfigureOverrideMaterialTarget::ConfigureOverrideMaterialTarget(RenderTarget *_upstream, bool _enable) :
	upstream(_upstream), enable(_enable)
{
}

void ConfigureOverrideMaterialTarget::activate(PipelineContext &context)
{
	upstream->activate(context);
	context.device->getVideoDriver()->getOverrideMaterial().Enabled = enable;
}


void populateAnaglyphPipeline(RenderPipeline *pipeline, Client *client)
{
	// clear depth buffer every time 3D is rendered
	auto step3D = pipeline->own(create3DStage(client, v2f(1.0)));
	auto screen = pipeline->createOwned<ScreenTarget>();
	auto clear_depth = pipeline->createOwned<ClearDepthBufferTarget>(screen);
	auto enable_override_material = pipeline->createOwned<ConfigureOverrideMaterialTarget>(clear_depth, true);
	step3D->setRenderTarget(enable_override_material);

	// left eye
	pipeline->addStep<OffsetCameraStep>(false);
	pipeline->addStep<SetColorMaskStep>(video::ECP_RED);
	pipeline->addStep(step3D);

	// right eye
	pipeline->addStep<OffsetCameraStep>(true);
	pipeline->addStep<SetColorMaskStep>(video::ECP_GREEN | video::ECP_BLUE);
	pipeline->addStep(step3D);

	// reset
	pipeline->addStep<OffsetCameraStep>(0.0f);
	pipeline->addStep<SetColorMaskStep>(video::ECP_ALL);

	pipeline->addStep<DrawWield>();
	pipeline->addStep<MapPostFxStep>();
	pipeline->addStep<DrawHUD>();
}
