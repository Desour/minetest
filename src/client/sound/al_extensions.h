// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 DS

#pragma once

#include "al_helpers.h"

namespace sound {

/**
 * Struct for AL and ALC extensions
 */
struct ALExtensions
{
	/// Pass nullptr to not retrieve any device-specifics extensions.
	/// (Not used at all so far.)
	explicit ALExtensions(const ALCdevice *deviceHandle [[maybe_unused]]);

	// no macro for ALC_ENUMERATION_EXT (has no declarations)
	bool have_ext_ALC_ENUMERATION_EXT = false;
#ifdef ALC_ENUMERATE_ALL_EXT
	bool have_ext_ALC_ENUMERATE_ALL_EXT = false;
#endif
#ifdef ALC_SOFT_reopen_device
	bool have_ext_ALC_SOFT_reopen_device = false;
	LPALCREOPENDEVICESOFT alcReopenDeviceSOFT = nullptr;
#endif
#ifdef AL_SOFT_direct_channels_remix
	bool have_ext_AL_SOFT_direct_channels_remix = false;
#endif
};

}
