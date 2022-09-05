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

#pragma once

#include "irr_v3d.h"
#include "../sound.h"
#include <string>
#include <unordered_set>
#include <vector>

class SoundLocalFallbackPathsGiver
{
public:
	virtual ~SoundLocalFallbackPathsGiver() = default;
	std::vector<std::string> getLocalFallbackPathsForSoundname(const std::string &name);
protected:
	virtual void addThePaths(const std::string &name, std::vector<std::string> &paths);
	// adds <common>.ogg, <common>.1.ogg, ..., <common>.9.ogg to paths
	void addAllAlternatives(const std::string &common, std::vector<std::string> &paths);
private:
	std::unordered_set<std::string> m_done_names;
};


/*
 * Sounds to be played can be:
 * * a single sound: just a sound. identified by a unique name. can not be played
 *   directly
 * * or a sound group: a set of single sounds. when played, one of these sounds is
 *   chosen by random
 *
 * TODO
 *
 */

/**
 * IDs for playing sounds.
 * Non-positive values are invalid.
 */
using sound_handle_t = int;

class ISoundManager
{
public:
	virtual ~ISoundManager() = default;

	virtual void step(float dtime) = 0;

	/**
	 * @param pos In node-space.
	 * @param vel In node-space.
	 * @param at Vector pointing forwards.
	 * @param up Vector pointing upwards, orthogonal to `at`.
	 */
	virtual void updateListener(const v3f &pos, const v3f &vel, const v3f &at,
			const v3f &up) = 0;
	virtual void setListenerGain(float gain) = 0;

	/**
	 * Adds a sound to load from a file (only OggVorbis).
	 * @param name The name of the sound. Must be unique, otherwise call fails.
	 * @param filepath The path for
	 * @return true on success, false on failure (ie. sound was already added or
	 *         file does not exist).
	 */
	virtual bool loadSoundFile(const std::string &name, const std::string &filepath) = 0;
	/**
	 * Same as `loadSoundFile`, but reads the OggVorbis file from memory.
	 */
	virtual bool loadSoundData(const std::string &name, std::string &&filedata) = 0;
	/**
	 * Adds sound with name sound_name to group `group_name`. Creates the group
	 * if non-existent.
	 * @param sound_name The name of the sound, as used in `loadSoundData`.
	 * @param group_name The name of the sound group.
	 */
	virtual void addSoundToGroup(const std::string &sound_name,
			const std::string &group_name) = 0;

	/** //TODO: doc this in lua_api.txt
	 * Plays a random sound from a sound group (position-less).
	 * @param group_name If == "", call is ignored without error.
	 * @param loop If true, sound is played in a loop.
	 * @param volume The gain of the sound (non-negative).
	 * @param fade If > 0.0f, the sound is faded in, with this value in gain per
	 *        second, until `volume` is reached.
	 * @param pitch Applies a pitch-shift to the sound. Each factor of 2.0f results
	 *        in a pitch-shift of +12 semitones. Must be positive.
	 * @param use_local_fallback If true, a local fallback (ie. from the user's
	 *        sound pack) is used if the sound-group does not exist.
	 * @param time_offset TODO
	 * @return -1 on failure, otherwise a handle to the sound.
	 */
	virtual sound_handle_t playSound(const SimpleSoundSpec &spec) = 0;
	/**
	 * Same as `playSound`, but at a position.
	 * @param pos In node-space.
	 */
	virtual sound_handle_t playSoundAt(const SimpleSoundSpec &spec, const v3f &pos) = 0;
	virtual void stopSound(sound_handle_t sound) = 0;
	virtual bool soundExists(sound_handle_t sound) = 0;
	/**
	 * @param pos In node-space.
	 */
	virtual void updateSoundPosition(sound_handle_t sound, const v3f &pos) = 0;
	virtual bool updateSoundGain(sound_handle_t id, float gain) = 0;
	virtual float getSoundGain(sound_handle_t id) = 0;
	virtual void fadeSound(sound_handle_t sound, float step, float gain) = 0;
};

class DummySoundManager final : public ISoundManager
{
public:
	void step(float dtime) override {}

	void updateListener(const v3f &pos, const v3f &vel, const v3f &at, const v3f &up) override {}
	void setListenerGain(float gain) override {}

	bool loadSoundFile(const std::string &name, const std::string &filepath) override { return true; }
	bool loadSoundData(const std::string &name, std::string &&filedata) override { return true; }
	void addSoundToGroup(const std::string &sound_name, const std::string &group_name) override {};

	sound_handle_t playSound(const SimpleSoundSpec &spec) override { return 0; }
	sound_handle_t playSoundAt(const SimpleSoundSpec &spec, const v3f &pos) override { return 0; }
	void stopSound(sound_handle_t sound) override {}
	bool soundExists(sound_handle_t sound) override { return false; }
	void updateSoundPosition(sound_handle_t sound, const v3f &pos) override {}
	bool updateSoundGain(sound_handle_t id, float gain) override { return false; }
	float getSoundGain(sound_handle_t id) override { return 0; }
	void fadeSound(sound_handle_t sound, float step, float gain) override {}
};

// Global DummySoundManager singleton
extern DummySoundManager dummySoundManager;
