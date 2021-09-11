/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "sound.h"

#include "filesys.h"
#include "log.h"
#include "porting.h"
#include <algorithm>
#include <string>
#include <vector>

// Global DummySoundManager singleton
DummySoundManager dummySoundManager;

std::vector<std::string> SoundLocalFallbackPathsGiver::
		getLocalFallbackPathsForSoundname(const std::string &name)
{
	std::vector<std::string> paths;

	// only try each name once
	if (m_done_names.count(name))
		return paths;
	m_done_names.insert(name);

	//~ // check for '.'. might be evil "../something" or "something.ogg"
	//~ if (name.find('.') != name.npos) {
		//~ errorstream << "SoundLocalFallbackPathsGiver: Sound names may not contain '.', but this one does: \""
				//~ << name << "\"" << std::endl;
		//~ return paths;
	//~ }

	addThePaths(name, paths);

	// remove duplicates
	std::sort(paths.begin(), paths.end());
	auto end = std::unique(paths.begin(), paths.end());
	paths.erase(end, paths.end());

	return paths;
}

void SoundLocalFallbackPathsGiver::addAllAlternatives(const std::string &common,
		std::vector<std::string> &paths)
{
	paths.reserve(paths.size() + 11);
	for (auto &&ext : {".ogg", ".0.ogg", ".1.ogg", ".2.ogg", ".3.ogg", ".4.ogg",
			".5.ogg", ".6.ogg", ".7.ogg", ".8.ogg", ".9.ogg", }) {
		paths.push_back(common + ext);
	}
}

void SoundLocalFallbackPathsGiver::addThePaths(const std::string &name,
		std::vector<std::string> &paths)
{
	addAllAlternatives(porting::path_share + DIR_DELIM + "sounds" + DIR_DELIM + name, paths);
	addAllAlternatives(porting::path_user + DIR_DELIM + "sounds" + DIR_DELIM + name, paths);
}
