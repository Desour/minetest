// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 DS

#include "al_extensions.h"

#include "settings.h"
#include "util/string.h"
#include <unordered_set>

namespace sound {

ALExtensions::ALExtensions(ALCdevice *alc_device [[maybe_unused]])
{
	auto blacklist_vec = str_split(g_settings->get("sound_extensions_blacklist"), ',');
	for (auto &s : blacklist_vec) {
		s = trim(s);
	}
	std::unordered_set<std::string> blacklist;
	blacklist.insert(blacklist_vec.begin(), blacklist_vec.end());

	auto query_ext_existence = [&](bool is_alc, const char *ext_name, bool compiled_with) {
		bool blacklisted = blacklist.find(ext_name) != blacklist.end();
		if (blacklisted)
			infostream << "ALExtensions: Blacklisted: " << ext_name << std::endl;

		if (!compiled_with)
			infostream << "ALExtensions: Not compiled with: " << ext_name << std::endl;

		bool found = is_alc ? (bool)alcIsExtensionPresent(alc_device, ext_name)
			: (bool)alIsExtensionPresent(ext_name);
		if (found)
			infostream << "ALExtensions: Found: " << ext_name << std::endl;
		else
			infostream << "ALExtensions: Not found: " << ext_name << std::endl;

		return !blacklisted && compiled_with && found;
	};

	auto query_func = [&](bool is_alc, const char *fname) {
		void *fptr = is_alc ? alcGetProcAddress(alc_device, fname) : alGetProcAddress(fname);
		if (!fptr) {
			errorstream << "ALExtensions: Could not find function that should exist: "
					<< fname << std::endl;
		}
		return fptr;
	};

	auto query_al_ext_existence = [&](const char *ext_name, bool compiled_with) {
		return query_ext_existence(false, ext_name, compiled_with);
	};
	auto query_alc_ext_existence = [&](const char *ext_name, bool compiled_with) {
		return query_ext_existence(true, ext_name, compiled_with);
	};
	[[maybe_unused]]
	auto query_al_func = [&](const char *fname) {
		return query_func(false, fname);
	};
	auto query_alc_func = [&](const char *fname) {
		return query_func(true, fname);
	};

	have_ext_ALC_ENUMERATION_EXT = query_alc_ext_existence("ALC_ENUMERATION_EXT", true);

	// TODO: do I need ENUMERATE_ALL?
#ifdef ALC_ENUMERATE_ALL_EXT
	have_ext_ALC_ENUMERATE_ALL_EXT = query_alc_ext_existence("ALC_ENUMERATE_ALL_EXT", true);
#else
	query_alc_ext_existence("ALC_ENUMERATE_ALL_EXT", false);
#endif

#ifdef ALC_SOFT_reopen_device
	while (query_alc_ext_existence("ALC_SOFT_reopen_device", true)) {
		alcReopenDeviceSOFT = reinterpret_cast<LPALCREOPENDEVICESOFT>(query_alc_func("alcReopenDeviceSOFT"));
		if (!alcReopenDeviceSOFT)
			break;
		have_ext_ALC_SOFT_reopen_device = true;
		break;
	}
#else
	query_alc_ext_existence("ALC_SOFT_reopen_device", false);
#endif

#ifdef AL_SOFT_direct_channels_remix
	have_ext_AL_SOFT_direct_channels_remix = query_al_ext_existence("AL_SOFT_direct_channels_remix", true);
#else
	query_al_ext_existence("AL_SOFT_direct_channels_remix", false);
#endif
}

}
