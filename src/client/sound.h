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
#include "util/numeric.h"
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
 * 0 is for sounds that are never modified after creation.
 * Negative numbers are invalid.
 */
using sound_handle_t = int;

constexpr sound_handle_t SOUND_HANDLE_T_MAX = std::numeric_limits<sound_handle_t>::max();

class ISoundManager
{
private:
	std::unordered_set<sound_handle_t> m_occupied_ids;
	sound_handle_t m_next_id = 1;
	std::vector<sound_handle_t> m_removed_sounds;

protected:
	void reportRemovedSound(sound_handle_t id)
	{
		if (id > 0)
			m_removed_sounds.push_back(id);
	}

public:
	virtual ~ISoundManager() = default;

	virtual void step(f32 dtime) = 0;

	/**
	 * @param pos In node-space.
	 * @param vel In node-space.
	 * @param at Vector pointing forwards.
	 * @param up Vector pointing upwards, orthogonal to `at`.
	 */
	virtual void updateListener(const v3f &pos, const v3f &vel, const v3f &at,
			const v3f &up) = 0;
	virtual void setListenerGain(f32 gain) = 0;

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
	virtual void playSound(sound_handle_t id, const SimpleSoundSpec &spec) = 0;
	/**
	 * Same as `playSound`, but at a position.
	 * @param pos In node-space.
	 */
	virtual void playSoundAt(sound_handle_t id, const SimpleSoundSpec &spec, const v3f &pos) = 0;
	/**
	 * Request the sound to be stopped.
	 * The id will later be given back via pollRemovedSounds.
	 */
	virtual void stopSound(sound_handle_t sound) = 0;
	/**
	 * @param pos In node-space.
	 */
	virtual void updateSoundPosition(sound_handle_t sound, const v3f &pos) = 0;
	virtual void updateSoundGain(sound_handle_t id, f32 gain) = 0; // TODO: do we need this? (we can fade instead)
	virtual void fadeSound(sound_handle_t sound, f32 step, f32 target_gain) = 0;

	/**
	 * Get and reset the list of sounds that were stopped.
	 * Ids can be freed afterwards.
	 */
	std::vector<sound_handle_t> pollRemovedSounds()
	{
		return std::move(m_removed_sounds);
	}

	/**
	 * Returns a positive id.
	 * The id will be returned again until freeId is called.
	 */
	sound_handle_t allocateId() //TODO: smart ptrs
	{
		while (m_occupied_ids.find(m_next_id) != m_occupied_ids.end()
				|| m_next_id == SOUND_HANDLE_T_MAX) {
			m_next_id = static_cast<s32>(
					myrand() % static_cast<u32>(SOUND_HANDLE_T_MAX - 1) + 1);
		}
		return m_next_id++;
	}

	/**
	 * Free an id allocated via allocateId.
	 */
	void freeId(sound_handle_t id)
	{
		m_occupied_ids.erase(id);;
	}
};

class DummySoundManager final : public ISoundManager
{
public:
	void step(f32 dtime) override {}

	void updateListener(const v3f &pos, const v3f &vel, const v3f &at, const v3f &up) override {}
	void setListenerGain(f32 gain) override {}

	bool loadSoundFile(const std::string &name, const std::string &filepath) override { return true; }
	bool loadSoundData(const std::string &name, std::string &&filedata) override { return true; }
	void addSoundToGroup(const std::string &sound_name, const std::string &group_name) override {};

	void playSound(sound_handle_t id, const SimpleSoundSpec &spec) override { reportRemovedSound(id); }
	void playSoundAt(sound_handle_t id, const SimpleSoundSpec &spec, const v3f &pos) override { reportRemovedSound(id); }
	void stopSound(sound_handle_t sound) override {}
	void updateSoundPosition(sound_handle_t sound, const v3f &pos) override {}
	void updateSoundGain(sound_handle_t id, f32 gain) override {}
	void fadeSound(sound_handle_t sound, f32 step, f32 target_gain) override {}
};

// Global DummySoundManager singleton
extern DummySoundManager dummySoundManager; // TODO: remove this
