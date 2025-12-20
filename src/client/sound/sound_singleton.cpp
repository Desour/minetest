// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2022 DS
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2011 Sebastian 'Bahamada' Rühl
// Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
// Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

#include "sound_singleton.h"
#include "settings.h"

#include "al_extensions.h"
#include <cstring>

namespace sound {

bool SoundManagerSingleton::init()
{
	//TOOD:tmp
	ALExtensions extensions = ALExtensions(nullptr);
	if (extensions.have_ext_ALC_ENUMERATION_EXT) {
		const ALCchar *devices = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
		errorstream << "Available ALC devices:";
		while (*devices != '\0') {
			errorstream << " " << devices << ",";
			devices += strlen(devices) + 1;
		}
		errorstream << std::endl;
	} else {
		errorstream << "No ALC_ENUMERATION_EXT.\n";
	}
#ifdef ALC_ENUMERATE_ALL_EXT
	if (extensions.have_ext_ALC_ENUMERATE_ALL_EXT) {
		const ALCchar *devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
		errorstream << "All available ALC devices:";
		while (*devices != '\0') {
			errorstream << " " << devices << ",";
			devices += strlen(devices) + 1;
		}
		errorstream << std::endl;
	} else
#endif
	{
		errorstream << "No ALC_ENUMERATE_ALL_EXT.\n";
	}

	std::string setting_sound_device = g_settings->get("sound_device"); // TODO: settingtypes, defaultsettings

	if (!setting_sound_device.empty()) {
		if (!(m_device = unique_ptr_alcdevice(alcOpenDevice(setting_sound_device.c_str())))) {
			warningstream << "Audio: Global Initialization: Failed to open device "
					<< setting_sound_device << ". Trying again with default device..."
					<< std::endl;
		}
	}

	if (!m_device && !(m_device = unique_ptr_alcdevice(alcOpenDevice(nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to open default device." << std::endl;
		return false;
	}

	if (!(m_context = unique_ptr_alccontext(alcCreateContext(m_device.get(), nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to create context." << std::endl;
		return false;
	}

	if (!alcMakeContextCurrent(m_context.get())) {
		errorstream << "Audio: Global Initialization: Failed to make current context." << std::endl;
		return false;
	}

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	// Speed of sound in nodes per second
	// FIXME: This value assumes 1 node sidelength = 1 meter, and "normal" air.
	//        Ideally this should be mod-controlled.
	alSpeedOfSound(343.3f);

	// doppler effect turned off for now, for best backwards compatibility
	alDopplerFactor(0.0f);

	if (alGetError() != AL_NO_ERROR) {
		errorstream << "Audio: Global Initialization: OpenAL Error " << alGetError() << std::endl;
		return false;
	}

	infostream << "Audio: Global Initialized: OpenAL " << alGetString(AL_VERSION)
		<< ", using " << alcGetString(m_device.get(), ALC_DEVICE_SPECIFIER)
		<< std::endl;

	return true;
}

SoundManagerSingleton::~SoundManagerSingleton()
{
	infostream << "Audio: Global Deinitialized." << std::endl;
}

} // namespace sound
