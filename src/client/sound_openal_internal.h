/*
Minetest
Copyright (C) 2022 DS
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
OpenAL support based on work by:
Copyright (C) 2011 Sebastian 'Bahamada' Rühl
Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; ifnot, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "log.h"
#include "porting.h"
#include "sound_openal.h"
#include "util/basic_macros.h"
#include "util/Optional.h"

#if defined(_WIN32)
	#include <al.h>
	#include <alc.h>
	//#include <alext.h>
#elif defined(__APPLE__)
	#define OPENAL_DEPRECATED
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	//#include <OpenAL/alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif
#include <vorbis/vorbisfile.h>

#include <unordered_map>
#include <vector>


/*
 * TODO: more doc
 *
 * The coordinate space for sounds:
 * --------------------------------
 *
 * * The functions from ISoundManager (see sound.h) take spatial vectors in the
 *   object space, that is node coordinate space multiplied by the BS factor.
 * * All other `v3f`s here are, if not told otherwise, in a sapce that you get by
 *   mirroring the above space along the x-axis.
 *   (This is needed because OpenAL uses a right-handed coordinate system.)
 * * Use `swap_handedness()` to convert between those two coordinate spaces.
 *
 */


/**
 * RAII wrapper for openal sound buffers.
 */
struct RAIIALSoundBuffer final
{
	RAIIALSoundBuffer() = default;
	RAIIALSoundBuffer(ALuint buffer) : m_buffer(buffer) {};

	~RAIIALSoundBuffer() noexcept { reset(0); }

	DISABLE_CLASS_COPY(RAIIALSoundBuffer)

	RAIIALSoundBuffer(RAIIALSoundBuffer &&other) : m_buffer(other.release()) {}
	RAIIALSoundBuffer &operator=(RAIIALSoundBuffer &&other);

	ALuint get() { return m_buffer; }

	ALuint release();

	void reset(ALuint buf);

	static RAIIALSoundBuffer generate();

private:
	// according to openal specification:
	// > Deleting buffer name 0 is a legal NOP.
	ALuint m_buffer = 0;
};

/**
 * For vorbisfile to read from our buffer instead of from a file.
 */
struct OggVorbisBufferSource {
	std::string buf;
	size_t cur_offset = 0;

	static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource);
	static int seek_func(void *datasource, ogg_int64_t offset, int whence);
	static int close_func(void *datasource);
	static long tell_func(void *datasource);

	static ov_callbacks s_ov_callbacks;
};

/**
 * TODO
 */
struct OggFileDecodeInfo {
	std::string name_for_logging;
	ALenum format; // AL_FORMAT_MONO16 or AL_FORMAT_STEREO16
	size_t bytes_per_sample;
	ALsizei freq;
	ALuint length_samples = 0;
	float length_seconds = 0.0f;
};

/**
 * RAII wrapper for OggVorbis_File.
 */
struct RAIIOggFile {
	bool m_needs_clear = false;
	OggVorbis_File m_file;

	RAIIOggFile() = default;

	DISABLE_CLASS_COPY(RAIIOggFile)

	~RAIIOggFile() noexcept
	{
		if (m_needs_clear)
			ov_clear(&m_file);
	}

	OggVorbis_File *get() { return &m_file; }

	Optional<OggFileDecodeInfo> getDecodeInfo(const std::string &filename_for_logging);

	/**
	 * Main function for loading ogg vorbis sounds. //TODO
	 *
	 * @param oggfile An opened ogg file.
	 * @param filename_for_logging Used for logging warnings when something is wrong.
	 * @param time_limit If != 0.0, determines the maximum number of seconds that are
	 *                   loaded.
	 * @param is_all_read If != nullptr and reading was successful, this will contain
	 *                    whether the file was read completely. (If not, the file pos
	 *                    will advance.)
	 * @param time_end If != nullptr and reading was successful, this will contain the
	 *                 time in seconds until which the file was loaded.
	 * @return 0 if not successful, otherwise an openal buffer name.
	 */
	RAIIALSoundBuffer loadBuffer(const OggFileDecodeInfo &decode_info, ALuint pcm_start,
			ALuint pcm_end);
};


/**
 * Class for the openal device and context
 */
class SoundManagerSingleton
{
public:
	struct AlcDeviceDeleter {
		void operator()(ALCdevice *p)
		{
			alcCloseDevice(p);
		}
	};

	struct AlcContextDeleter {
		void operator()(ALCcontext *p)
		{
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(p);
		}
	};

	using unique_ptr_alcdevice = std::unique_ptr<ALCdevice, AlcDeviceDeleter>;
	using unique_ptr_alccontext = std::unique_ptr<ALCcontext, AlcContextDeleter>;

	unique_ptr_alcdevice  m_device;
	unique_ptr_alccontext m_context;

public:
	bool init();

	~SoundManagerSingleton()
	{
		infostream << "Audio: Global Deinitialized." << std::endl;
	}
};


/**
 * TODO: doc
 */
struct ISoundDataOpen
{
	OggFileDecodeInfo m_decode_info;

	ISoundDataOpen(const OggFileDecodeInfo &decode_info) : m_decode_info(decode_info) {}

	virtual ~ISoundDataOpen() = default;

	/**
	 * Iff the data is streaming, there is more than one buffer.
	 * @return Whether it's streaming data.
	 */
	virtual bool isStreaming() = 0;

	/**
	 * TODO
	 *
	 * `offset_in_buffer == 0` is guaranteed if some loaded buffer ends at `offset`.
	 *
	 * @param offset TODO
	 * @return {buffer, buffer_end, offset_in_buffer}
	 */
	virtual std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) = 0;

	static std::shared_ptr<ISoundDataOpen> fromOggFile(std::unique_ptr<RAIIOggFile> oggfile,
		const std::string &filename_for_logging);
};

/**
 * TODO: doc
 */
struct ISoundDataUnopen
{
	virtual ~ISoundDataUnopen() = default;

	virtual std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && = 0;
};

struct SoundDataUnopenBuffer;

/**
 * Sound file is in a memory buffer.
 */
struct SoundDataUnopenBuffer final : ISoundDataUnopen
{
	std::string m_buffer;

	explicit SoundDataUnopenBuffer(std::string &&buffer) : m_buffer(std::move(buffer)) {}

	virtual std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && override;
};

/**
 * Sound file is in file system.
 */
struct SoundDataUnopenFile final : ISoundDataUnopen
{
	std::string m_path;

	explicit SoundDataUnopenFile(const std::string &path) : m_path(path) {}

	virtual std::shared_ptr<ISoundDataOpen> open(const std::string &sound_name) && override;
};

/**
 * TODO: doc
 */
struct SoundDataOpenSinglebuf final : ISoundDataOpen
{
	RAIIALSoundBuffer m_buffer;

	SoundDataOpenSinglebuf(std::unique_ptr<RAIIOggFile> oggfile,
			const OggFileDecodeInfo &decode_info);

	virtual bool isStreaming() override { return false; }

	virtual std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override
	{
		if (offset >= m_decode_info.length_samples)
			return {0, 0, 0};
		return {m_buffer.get(), m_decode_info.length_samples, offset};
	}
};

/**
 * TODO: doc
 */
struct SoundDataOpenStream final : ISoundDataOpen
{
	/**
	 * An OpenAL buffer that goes until `m_end` (exclusive).
	 */
	struct SoundBufferUntil final
	{
		ALuint m_end;
		RAIIALSoundBuffer m_buffer;
	};

	/**
	 * A sorted non-empty vector of contiguous buffers.
	 * The start (inclusive) of each buffer is the end of its predecessor, or
	 * `m_start` for the first buffer.
	 */
	struct ContiguousBuffers final {
		ALuint m_start;
		std::vector<SoundBufferUntil> m_buffers;
	};

	std::unique_ptr<RAIIOggFile> m_oggfile;
	// A sorted vector of non-overlapping, non-contiguous `ContiguousBuffers`s.
	std::vector<ContiguousBuffers> m_bufferss;

	SoundDataOpenStream(std::unique_ptr<RAIIOggFile> oggfile,
			const OggFileDecodeInfo &decode_info);

	virtual bool isStreaming() override { return true; }

	virtual std::tuple<ALuint, ALuint, ALuint> getOrLoadBufferAt(ALuint offset) override;

private:
	// offset must be before after_it's m_start and after (--after_it)'s last m_end
	// new buffer will be inserted into m_buffers before after_it
	// returns same as getOrLoadBufferAt
	std::tuple<ALuint, ALuint, ALuint> loadBufferAt(ALuint offset,
			std::vector<ContiguousBuffers>::iterator after_it);
};


/**
 * TODO: doc
 */
class PlayingSound final
{
	ALuint m_source_id;
	std::shared_ptr<ISoundDataOpen> m_data;
	ALuint m_next_sample_pos = 0;
	bool m_looping;

public:
	PlayingSound(ALuint source_id, std::shared_ptr<ISoundDataOpen> data, bool loop,
			float time_offset);

	~PlayingSound() noexcept
	{
		alDeleteSources(1, &m_source_id);
	}

	DISABLE_CLASS_COPY(PlayingSound)

	// return false means streaming finished (TODO)
	bool stepStream();

	// pos_ and vel_ are left-handed.
	void updatePosVel(const v3f &pos_, const v3f &vel_);

	void makePositionless();

	void setGain(float gain) { alSourcef(m_source_id, AL_GAIN, gain); }

	float getGain()
	{
		ALfloat gain;
		alGetSourcef(m_source_id, AL_GAIN, &gain);
		return gain;
	}

	void setPitch(float pitch) { alSourcef(m_source_id, AL_PITCH, pitch); }

	bool isStreaming() { return m_data->isStreaming(); }

	void play() { alSourcePlay(m_source_id); }

	bool isPlaying()
	{
		ALint state;
		alGetSourcei(m_source_id, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}
};


/*
 * The public ISoundManager interface
 */

class OpenALSoundManager final : public ISoundManager
{
private:
	std::unique_ptr<SoundLocalFallbackPathsGiver> m_local_fallback_paths_giver;

	ALCdevice *m_device;
	ALCcontext *m_context;

	// used to create new ids. sound_handle_t (=int) will hopefully never overflow
	sound_handle_t m_next_id;

	// loaded sounds
	std::unordered_map<std::string, std::unique_ptr<ISoundDataUnopen>> m_sound_datas_unopen;
	std::unordered_map<std::string, std::shared_ptr<ISoundDataOpen>> m_sound_datas_open;
	// sound groups
	std::unordered_map<std::string, std::vector<std::string>> m_sound_groups;

	// currently playing sounds
	std::unordered_map<sound_handle_t, std::shared_ptr<PlayingSound>> m_sounds_playing;

	//~ /* * Streaming sounds always have 3 buffers enqueued.
	 //~ * * The streams are maintained in big-steps of 0.5 minimum buffer lengths.
	 //~ * * The sounds in m_sounds_streaming_treating are treated at
	 //~ * * In the steps in each big-step,
	 //~ * * If there's no buffer to dequeue, a sound is added to m_sounds_streaming_waiting,
	 //~ *   and nothing happens to it until the next stream step.
	 //~ *
	 //~ */
	//~ // sounds inserted in here have at least 2 full buffers remaining
	//~ std::vector<std::weak_ptr<PlayingSound>> m_sounds_streaming_waiting;
	//~ // sounds inserted in here have at least 1 full buffer remaining
	//~ std::vector<std::weak_ptr<PlayingSound>> m_sounds_streaming_treating;
	//~ // TODO
	//~ float m_stream_timer; // = STREAM_BIG_STEP_TIME; MIN_BUF_SIZE_SECS * 0.5

	std::vector<std::weak_ptr<PlayingSound>> m_sounds_streaming;

	struct FadeState {
		FadeState() = default;

		FadeState(float step, float current_gain, float target_gain):
			step(step),
			current_gain(current_gain),
			target_gain(target_gain) {}
		float step;
		float current_gain;
		float target_gain;
	};

	std::unordered_map<int, FadeState> m_sounds_fading;

private:
	sound_handle_t newSoundID();

public:
	OpenALSoundManager(SoundManagerSingleton *smg,
			std::unique_ptr<SoundLocalFallbackPathsGiver> local_fallback_paths_giver);

	~OpenALSoundManager() override;

	DISABLE_CLASS_COPY(OpenALSoundManager)

	void step(float dtime) override //TODO: does not seem to be called unfocused
	{
		doFades(dtime);
		stepStreams(dtime);
	}

	void stepStreams(float dtime);

	/**
	 * TODO
	 */
	std::shared_ptr<ISoundDataOpen> openSingleSound(const std::string &sound_name);

	/**
	 * Gets a random sound name from a group.
	 *
	 * @param group_name The name of the sound group.
	 * @return The name of a sound in the group, or "" on failure. Getting the
	 * sound with openSingleSound directly afterwards will not fail.
	 */
	std::string getLoadedSoundNameFromGroup(const std::string &group_name);

	/**
	 * Same as getLoadedSoundNameFromGroup, but if sound does not exist, try to
	 * load from local files.
	 */
	std::string getOrLoadLoadedSoundNameFromGroup(const std::string &group_name);

	// pos_opt is left-handed
	std::shared_ptr<PlayingSound> createPlayingSound(const std::string &sound_name,
			bool loop, float volume, float pitch, float time_offset, Optional<v3f> pos_opt);

	// pos_opt is left-handed
	sound_handle_t playSoundGeneric(const std::string &group_name, bool loop,
			float volume, float fade, float pitch, bool use_local_fallback, float time_offset,
			Optional<v3f> pos_opt);

	void deleteSound(sound_handle_t id)
	{
		m_sounds_playing.erase(id);
	}

	// Removes stopped sounds
	void maintain();

	/* Interface */

	bool loadSoundFile(const std::string &name, const std::string &filepath) override;

	bool loadSoundData(const std::string &name, std::string &&filedata) override;

	void addSoundToGroup(const std::string &sound_name, const std::string &group_name) override;

	void updateListener(const v3f &pos_, const v3f &vel_, const v3f &at, const v3f &up) override;

	void setListenerGain(float gain) override
	{
		alListenerf(AL_GAIN, gain);
	}

	sound_handle_t playSound(const std::string &group_name, bool loop, float volume, float fade,
			float pitch, bool use_local_fallback, float time_offset) override
	{
		return playSoundGeneric(group_name, loop, volume, fade, pitch, use_local_fallback,
				time_offset, nullopt);
	}

	sound_handle_t playSoundAt(const std::string &group_name, bool loop, float volume, v3f pos,
			float pitch, bool use_local_fallback, float time_offset) override
	{
		// AL_REFERENCE_DISTANCE was once reduced from 3*BS to 1*BS.
		// We compensate this by multiplying the volume by 3.
		// Note that this is just done to newly created sounds (and not in
		// updateSoundGain) because it was like this for many versions, so someone
		// might depend on this (inconsistent) behaviour.
		volume *= 3.0f;

		return playSoundGeneric(group_name, loop, volume, 0.0f /* no fade */,
				pitch, use_local_fallback, time_offset, pos);
	}

	void stopSound(sound_handle_t sound) override
	{
		maintain();
		deleteSound(sound);
	}

	void fadeSound(sound_handle_t soundid, float step, float gain) override;

	void doFades(float dtime);

	bool soundExists(sound_handle_t sound) override
	{
		maintain();
		return (m_sounds_playing.count(sound) != 0);
	}

	void updateSoundPosition(sound_handle_t id, v3f pos) override
	{
		auto i = m_sounds_playing.find(id);
		if (i == m_sounds_playing.end())
			return;
		i->second->updatePosVel(pos, v3f(0.0f, 0.0f, 0.0f));
	}

	bool updateSoundGain(sound_handle_t id, float gain) override
	{
		auto i = m_sounds_playing.find(id);
		if (i == m_sounds_playing.end())
			return false;
		i->second->setGain(gain);
		return true;
	}

	float getSoundGain(sound_handle_t id) override
	{
		auto i = m_sounds_playing.find(id);
		if (i == m_sounds_playing.end())
			return 0.0f;

		return i->second->getGain();
	}
};
