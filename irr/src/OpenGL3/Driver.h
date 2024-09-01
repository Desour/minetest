// Copyright (C) 2023 Vitaliy Lobachevskiy
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#pragma once
#include "OpenGL/Driver.h"
#include "EDriverTypes.h"
#include "OpenGL/Common.h"

namespace irr::io {
class IFileSystem;
}  // namespace irr::io
namespace irr::video {
class IContextManager;
class IVideoDriver;
}  // namespace irr::video

namespace irr
{
struct SIrrlichtCreationParameters;

namespace video
{

/// OpenGL 3+ driver
///
/// For OpenGL 3.2 and higher. Compatibility profile is required currently.
class COpenGL3Driver : public COpenGL3DriverBase
{
	friend IVideoDriver *createOpenGL3Driver(const SIrrlichtCreationParameters &params, io::IFileSystem *io, IContextManager *contextManager);

public:
	using COpenGL3DriverBase::COpenGL3DriverBase;
	E_DRIVER_TYPE getDriverType() const override;

protected:
	OpenGLVersion getVersionFromOpenGL() const override;
	void initFeatures() override;
};

}
}
