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

#include "sound_openal_internal.h"

#include "util/numeric.h" // myrand()
#include <algorithm>

// constants (TODO)

// maximum length in seconds that a sound can have without being streamed
constexpr double SOUND_DURATION_MAX_SINGLE = 3.0;
// time in seconds of a single buffer in a streamed sound
constexpr double STREAM_BUFFER_LENGTH = 1.0;
// remaining play time in seconds at which the next part of a stream is enqueued for loading
constexpr double STREAM_REMAINING_TIME_LOAD_ENQUEUE = 2.0;
// remaining play time in seconds at which the next part of a stream is loaded immediately
constexpr double STREAM_REMAINING_TIME_LOAD_NOW = 1.0;
constexpr ALuint MIN_STREAM_BUFFER_LENGTH_SAMPLES = 133713; //TODO


/*
 * Helpers
 */

static const char *getAlErrorString(ALenum err)
{
	switch (err) {
	case AL_NO_ERROR:
		return "no error";
	case AL_INVALID_NAME:
		return "invalid name";
	case AL_INVALID_ENUM:
		return "invalid enum";
	case AL_INVALID_VALUE:
		return "invalid value";
	case AL_INVALID_OPERATION:
		return "invalid operation";
	case AL_OUT_OF_MEMORY:
		return "out of memory";
	default:
		return "<unknown OpenAL error>";
	}
}

static ALenum warn_if_al_error(const char *desc)
{
	ALenum err = alGetError();
	if (err == AL_NO_ERROR)
		return err;
	warningstream << "[OpenAL Error] " << desc << ": " << getAlErrorString(err)
			<< std::endl;
	return err;
}

static bool can_open_file(const std::string &path)
{
	std::FILE *f = std::fopen(path.c_str(), "r");
	if (!f)
		return false;
	std::fclose(f);
	return true;
}

// transforms vectors from a left-handed coordinate system to a right-handed one
// and vice-versa
// (needed because Minetest uses a left-handed one and OpenAL a right-handed one)
static v3f swap_handedness(const v3f &v)
{
	return v3f(-v.X, v.Y, v.Z);
}

/*
 * RAIIALSoundBuffer struct
 */

RAIIALSoundBuffer &RAIIALSoundBuffer::operator=(RAIIALSoundBuffer &&other)
{
	if (&other != this)
		reset(other.release());
	return *this;
}

ALuint RAIIALSoundBuffer::release()
{
	auto buf = m_buffer;
	m_buffer = 0;
	return buf;
}

void RAIIALSoundBuffer::reset(ALuint buf)
{
	if (m_buffer != 0) {
		alDeleteBuffers(1, &m_buffer);
		warn_if_al_error("Failed to free sound buffer");
	}

	m_buffer = buf;
}

RAIIALSoundBuffer RAIIALSoundBuffer::generate()
{
	ALuint buf;
	alGenBuffers(1, &buf);
	return RAIIALSoundBuffer(buf);
}

/*
 * OggVorbisBufferSource struct
 */

size_t OggVorbisBufferSource::read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	size_t copied_size = MYMIN(s->buf.size() - s->cur_offset, size);
	memcpy(ptr, s->buf.data() + s->cur_offset, copied_size);
	s->cur_offset += copied_size;
	return copied_size;
}

int OggVorbisBufferSource::seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	if (whence == SEEK_SET) {
		if (offset < 0 || (size_t)MYMAX(offset, 0) >= s->buf.size()) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset = offset;
		return 0;
	} else if (whence == SEEK_CUR) {
		if ((size_t)MYMIN(-offset, 0) > s->cur_offset
				|| s->cur_offset + offset > s->buf.size()) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset += offset;
		return 0;
	}
	// invalid whence param (SEEK_END doesn't have to be supported)
	return -1;
}

int OggVorbisBufferSource::close_func(void *datasource)
{
	std::unique_ptr<OggVorbisBufferSource> s((OggVorbisBufferSource *)datasource);
	return 0;
}

long OggVorbisBufferSource::tell_func(void *datasource)
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	return s->cur_offset;
}

const ov_callbacks OggVorbisBufferSource::s_ov_callbacks = {
	&OggVorbisBufferSource::read_func,
	&OggVorbisBufferSource::seek_func,
	&OggVorbisBufferSource::close_func,
	&OggVorbisBufferSource::tell_func
};

/*
 * RAIIOggFile struct
 */

Optional<OggFileDecodeInfo> RAIIOggFile::getDecodeInfo(const std::string &filename_for_logging)
{
	OggFileDecodeInfo ret;

	vorbis_info *pInfo = ov_info(&m_file, -1);
	if (!pInfo)
		return nullopt;

	ret.name_for_logging = filename_for_logging;

	if (pInfo->channels == 1) {
		ret.format = AL_FORMAT_MONO16;
		ret.bytes_per_sample = 2;
	} else if (pInfo->channels == 2) {
		ret.format = AL_FORMAT_STEREO16;
		ret.bytes_per_sample = 4;
	} else {
		warningstream << "Audio: Can't decode. Sound is neither mono nor stereo: "
				<< ret.name_for_logging << std::endl;
		return nullopt;
	}

	ret.freq = pInfo->rate;

	ret.length_samples = static_cast<ALuint>(ov_pcm_total(&m_file, -1));
	ret.length_seconds = static_cast<f32>(ov_time_total(&m_file, -1));

	return ret;
}

RAIIALSoundBuffer RAIIOggFile::loadBuffer(const OggFileDecodeInfo &decode_info,
		ALuint pcm_start, ALuint pcm_end)
{
	constexpr int endian = 0; // 0 for Little-Endian, 1 for Big-Endian
	constexpr int word_size = 2; // we use s16 samples
	constexpr int word_signed = 1; // ^

	// seek
	if (ov_pcm_tell(&m_file) != pcm_start) {
		if (ov_pcm_seek(&m_file, pcm_start) != 0) {
			warningstream << "Audio: Error decoding (could not seek) "
					<< decode_info.name_for_logging << std::endl;
			return 0;
		}
	}

	const size_t size = static_cast<size_t>(pcm_end - pcm_start) * decode_info.bytes_per_sample;

	std::vector<char> snd_buffer;
	snd_buffer.resize(size); // TODO: do not 0-init

	// read size bytes
	size_t read_count = 0;
	int bitStream;
	while (read_count < size) {
		// Read up to a buffer's worth of decoded sound data
		long num_bytes = ov_read(&m_file, &snd_buffer[read_count], size - read_count,
				endian, word_size, word_signed, &bitStream);

		if (num_bytes <= 0) {
			warningstream << "Audio: Error decoding "
					<< decode_info.name_for_logging << std::endl;
			return 0;
		}

		read_count += num_bytes;
	}

	// load buffer to openal
	RAIIALSoundBuffer snd_buffer_id = RAIIALSoundBuffer::generate();
	alBufferData(snd_buffer_id.get(), decode_info.format, &(snd_buffer[0]), size,
			decode_info.freq);

	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		infostream << "Audio: OpenAL error: " << getAlErrorString(error)
				<< "preparing sound buffer for sound '"
				<< decode_info.name_for_logging << "'" << std::endl;
	}

	return snd_buffer_id;
}

/*
 * SoundManagerSingleton class
 */

bool SoundManagerSingleton::init()
{
	if (!(m_device = unique_ptr_alcdevice(alcOpenDevice(nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to open device" << std::endl;
		return false;
	}

	if (!(m_context = unique_ptr_alccontext(alcCreateContext(m_device.get(), nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to create context" << std::endl;
		return false;
	}

	if (!alcMakeContextCurrent(m_context.get())) {
		errorstream << "Audio: Global Initialization: Failed to make current context" << std::endl;
		return false;
	}

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	if (alGetError() != AL_NO_ERROR) {
		errorstream << "Audio: Global Initialization: OpenAL Error " << alGetError() << std::endl;
		return false;
	}

	infostream << "Audio: Global Initialized: OpenAL " << alGetString(AL_VERSION)
		<< ", using " << alcGetString(m_device.get(), ALC_DEVICE_SPECIFIER)
		<< std::endl;

	return true;
}

/*
 * ISoundDataOpen struct
 */

std::shared_ptr<ISoundDataOpen> ISoundDataOpen::fromOggFile(std::unique_ptr<RAIIOggFile> oggfile,
		const std::string &filename_for_logging)
{
	// Get some information about the OGG file
	Optional<OggFileDecodeInfo> decode_info = oggfile->getDecodeInfo(filename_for_logging);
	if (!decode_info.has_value()) {
		warningstream << "Audio: Error decoding "
				<< filename_for_logging << std::endl;
		return nullptr;
	}

	// use duration (in seconds) to decide whether to load all at once or to stream
	if (decode_info->length_seconds <= SOUND_DURATION_MAX_SINGLE) {
		return std::make_shared<SoundDataOpenSinglebuf>(std::move(oggfile), *decode_info);
	} else {
		return std::make_shared<SoundDataOpenStream>(std::move(oggfile), *decode_info);
	}
}

/*
 * SoundDataUnopenBuffer struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenBuffer::open(const std::string &sound_name) &&
{
	// load from m_buffer

	auto oggfile = std::make_unique<RAIIOggFile>();

	auto buffer_source = std::make_unique<OggVorbisBufferSource>();
	buffer_source->buf = std::move(m_buffer);

	oggfile->m_needs_clear = true;
	if (ov_open_callbacks(buffer_source.release(), oggfile->get(), nullptr, 0,
			OggVorbisBufferSource::s_ov_callbacks) != 0) {
		warningstream << "Audio: Error opening " << sound_name << " for decoding"
				<< std::endl;
		return nullptr;
	}

	return ISoundDataOpen::fromOggFile(std::move(oggfile), sound_name);
}

/*
 * SoundDataUnopenFile struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenFile::open(const std::string &sound_name) &&
{
	// load from file at m_path

	auto oggfile = std::make_unique<RAIIOggFile>();

	if (ov_fopen(m_path.c_str(), oggfile->get()) != 0) {
		warningstream << "Audio: Error opening " << m_path << " for decoding"
				<< std::endl;
		return nullptr;
	}
	oggfile->m_needs_clear = true;

	return ISoundDataOpen::fromOggFile(std::move(oggfile), sound_name);
}

/*
 * SoundDataOpenSinglebuf struct
 */

SoundDataOpenSinglebuf::SoundDataOpenSinglebuf(std::unique_ptr<RAIIOggFile> oggfile,
		const OggFileDecodeInfo &decode_info) : ISoundDataOpen(decode_info)
{
	m_buffer = oggfile->loadBuffer(m_decode_info, 0, m_decode_info.length_samples);
	if (m_buffer.get() == 0) {
		warningstream << "SoundDataOpenSinglebuf: failed to load sound '"
				<< m_decode_info.name_for_logging << "'" << std::endl;
		return;
	}
}

/*
 * SoundDataOpenStream struct
 */

SoundDataOpenStream::SoundDataOpenStream(std::unique_ptr<RAIIOggFile> oggfile,
		const OggFileDecodeInfo &decode_info) :
	ISoundDataOpen(decode_info), m_oggfile(std::move(oggfile))
{
	// do nothing here. buffers are loaded at getOrLoadBufferAt
}

std::tuple<ALuint, ALuint, ALuint> SoundDataOpenStream::getOrLoadBufferAt(ALuint offset)
{
	// find the right-most ContiguousBuffers, such that `m_start <= offset`
	// equivalent: the first element from the right such that `!(m_start > offset)`
	// (from the right, `offset` is a lower bound to the `m_start`s)
	auto lower_rit = std::lower_bound(m_bufferss.rbegin(), m_bufferss.rend(), offset,
			[](const ContiguousBuffers &bufs, ALuint offset) {
				return bufs.m_start > offset;
			});

	if (lower_rit != m_bufferss.rend()) {
		std::vector<SoundBufferUntil> &bufs = lower_rit->m_buffers;
		// find the left-most SoundBufferUntil, such that `m_end > offset`
		// equivalent: the first element from the left such that `m_end > offset`
		// (returns first element where comp gives true)
		auto upper_it = std::upper_bound(bufs.begin(), bufs.end(), offset,
				[](ALuint offset, const SoundBufferUntil &buf) {
					return offset < buf.m_end;
				});

		if (upper_it != bufs.end()) {
			ALuint start = upper_it == bufs.begin() ? lower_rit->m_start
					: (upper_it - 1)->m_end;
			return {upper_it->m_buffer.get(), upper_it->m_end, offset - start};
		}
	}

	// no loaded buffer starts before or at `offset`
	// or no loaded buffer (that starts before or at `offset`) ends after `offset`

	// lower_rit, but not reverse and 1 farther
	auto after_it = m_bufferss.begin() + (m_bufferss.rend() - lower_rit);

	return loadBufferAt(offset, after_it);
}

std::tuple<ALuint, ALuint, ALuint> SoundDataOpenStream::loadBufferAt(ALuint offset,
		std::vector<ContiguousBuffers>::iterator after_it)
{
	bool has_before = after_it != m_bufferss.begin();
	bool has_after = after_it != m_bufferss.end();

	ALuint end_before = has_before ? (after_it - 1)->m_buffers.back().m_end : 0;
	ALuint start_after = has_after ? after_it->m_start : m_decode_info.length_samples;

	//
	// 1) find the actual start and end of the new buffer
	//

	ALuint new_buf_start = offset;
	ALuint new_buf_end = offset + MIN_STREAM_BUFFER_LENGTH_SAMPLES;

	// don't load into next buffer
	if (new_buf_end > start_after) {
		new_buf_end = start_after;
		// also move start (for min buf size)
		if (new_buf_end - new_buf_start < MIN_STREAM_BUFFER_LENGTH_SAMPLES) {
			new_buf_start = new_buf_end < MIN_STREAM_BUFFER_LENGTH_SAMPLES ? 0
					: new_buf_end - MIN_STREAM_BUFFER_LENGTH_SAMPLES;
		}
	}

	// widen if space to right or left is smaller than min buf size
	if (new_buf_start - end_before < MIN_STREAM_BUFFER_LENGTH_SAMPLES)
		new_buf_start = end_before;
	if (start_after - new_buf_end < MIN_STREAM_BUFFER_LENGTH_SAMPLES)
		new_buf_end = start_after;

	//
	// 2) load [new_buf_start, new_buf_end)
	//

	// if it fails, we get a 0-buffer. we store it and won't try loading again
	RAIIALSoundBuffer new_buf = m_oggfile->loadBuffer(m_decode_info, new_buf_start,
			new_buf_end);

	//
	// 3) insert before after_it
	//

	// choose ContiguousBuffers to add the new SoundBufferUntil into:
	// * after_it - 1 (=before) if existent and there's no space between its
	//   last buffer and the new buffer
	// * a new ContiguousBuffers otherwise
	auto it = has_before && new_buf_start == end_before ? after_it - 1
			: m_bufferss.insert(after_it, ContiguousBuffers{new_buf_start, {}});

	// add the new SoundBufferUntil
	size_t new_buf_i = it->m_buffers.size();
	it->m_buffers.push_back(SoundBufferUntil{new_buf_end, std::move(new_buf)});

	if (has_after && new_buf_end == start_after) {
		// merge after into my ContiguousBuffers
		auto &bufs = it->m_buffers;
		auto &bufs_after = (it + 1)->m_buffers;
		bufs.insert(bufs.end(), std::make_move_iterator(bufs_after.begin()),
				std::make_move_iterator(bufs_after.end())); // TODO: impl swap?
		it = --m_bufferss.erase(++it);
	}

	return {it->m_buffers[new_buf_i].m_buffer.get(), new_buf_end, offset - new_buf_start};
}

/*
 * PlayingSound class
 */

PlayingSound::PlayingSound(ALuint source_id, std::shared_ptr<ISoundDataOpen> data,
		bool loop, f32 volume, f32 pitch, f32 time_offset, const Optional<v3f> &pos_opt)
	: m_source_id(source_id), m_data(std::move(data)), m_looping(loop)
{
	// calculate actual time_offset (see lua_api.txt for specs)
	f32 len_seconds = m_data->m_decode_info.length_seconds;
	f32 len_samples = m_data->m_decode_info.length_samples;
	if (!m_looping) {
		if (time_offset < 0.0f) {
			time_offset = std::fmax(time_offset + len_seconds, 0.0f);
		} else if (time_offset >= len_seconds) {
			// no sound
			m_next_sample_pos = len_samples;
			return;
		}
	} else {
		time_offset = time_offset - std::floor(time_offset / len_seconds) * len_seconds;
	}

	// queue first buffers

	m_next_sample_pos = (time_offset / len_seconds) * len_samples;

	if (!m_data->isStreaming()) {
		auto buf_nxtoffset_thisoffset_tpl = m_data->getOrLoadBufferAt(m_next_sample_pos);
		ALuint buf = std::get<0>(buf_nxtoffset_thisoffset_tpl);
		m_next_sample_pos = std::get<1>(buf_nxtoffset_thisoffset_tpl);
		ALuint sample_offset = std::get<2>(buf_nxtoffset_thisoffset_tpl);

		alSourcei(m_source_id, AL_BUFFER, buf);
		alSourcei(m_source_id, AL_SAMPLE_OFFSET, sample_offset);

		alSourcei(m_source_id, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);

		warn_if_al_error("when creating non-streaming sound");

	} else {
		// start with 2 buffers
		ALuint buf_ids[2];

		auto buf_nxtoffset_thisoffset_tpl1 = m_data->getOrLoadBufferAt(m_next_sample_pos);
		buf_ids[0] = std::get<0>(buf_nxtoffset_thisoffset_tpl1);
		m_next_sample_pos = std::get<1>(buf_nxtoffset_thisoffset_tpl1);
		ALuint sample_offset1 = std::get<2>(buf_nxtoffset_thisoffset_tpl1);

		auto buf_nxtoffset_thisoffset_tpl2 = m_data->getOrLoadBufferAt(m_next_sample_pos);
		buf_ids[1] = std::get<0>(buf_nxtoffset_thisoffset_tpl2);
		m_next_sample_pos = std::get<1>(buf_nxtoffset_thisoffset_tpl2);
		assert(std::get<2>(buf_nxtoffset_thisoffset_tpl2) == 0);

		alSourceQueueBuffers(m_source_id, 2, buf_ids);
		alSourcei(m_source_id, AL_SAMPLE_OFFSET, sample_offset1);

		// we can't use AL_LOOPING because more buffers are queued later
		// looping is therefore done manually

		// TODO:
		// * streaming
		// * looping

		m_stopped_means_dead = false;

		warn_if_al_error("when creating streaming sound");
	}

	// set initial pos, volume, pitch
	if (pos_opt.has_value()) {
		updatePosVel(*pos_opt, v3f(0.0f, 0.0f, 0.0f)); //TODO: velocity
	} else {
		// make position-less
		alSourcei(m_source_id, AL_SOURCE_RELATIVE, true);
		alSource3f(m_source_id, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(m_source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		warn_if_al_error("PlayingSound::PlayingSound at making position-less");
	}
	setGain(volume);
	setPitch(pitch);
}

bool PlayingSound::stepStream()
{
	if (isDead())
		return false;

	// unqueue finished buffers
	ALint num_unqueued_bufs = 0;
	alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &num_unqueued_bufs);
	if (num_unqueued_bufs == 0)
		return true;
	// we always have 2 buffers enqueued at most
	SANITY_CHECK(num_unqueued_bufs <= 2);
	ALuint unqueued_buffer_ids[2];
	alSourceUnqueueBuffers(m_source_id, num_unqueued_bufs, unqueued_buffer_ids);

	// fill up again
	for (ALint i = 0; i < num_unqueued_bufs; ++i) {
		if (m_next_sample_pos == m_data->m_decode_info.length_samples) {
			// reached end
			if (m_looping) {
				m_next_sample_pos = 0;
			} else {
				m_stopped_means_dead = true;
				return false;
			}
		}

		auto buf_nxtoffset_thisoffset_tpl = m_data->getOrLoadBufferAt(m_next_sample_pos);
		ALuint buf_id = std::get<0>(buf_nxtoffset_thisoffset_tpl);
		m_next_sample_pos = std::get<1>(buf_nxtoffset_thisoffset_tpl);
		assert(std::get<2>(buf_nxtoffset_thisoffset_tpl) == 0);

		alSourceQueueBuffers(m_source_id, 1, &buf_id);

		// start again if queue was empty and resulted in stop
		if (getState() == AL_STOPPED) {
			play();
			warningstream << "PlayingSound::stepStream: sound queue ran empty"
					<< std::endl;
		}
	}

	return true;
}

bool PlayingSound::fade(f32 step, f32 target_gain)
{
	bool already_fading = m_fade_state.has_value();

	target_gain = rangelim(target_gain, 0.0f, 1.0f);
	step = target_gain - getGain() > 0.0f ? std::abs(step) : -std::abs(step);

	m_fade_state = FadeState{step, target_gain};

	return !already_fading;
}

bool PlayingSound::doFade(f32 dtime)
{
	if (!m_fade_state || isDead())
		return false;

	FadeState &fade = *m_fade_state;
	assert(fade.step != 0.0f);

	f32 current_gain = getGain();
	current_gain += fade.step * dtime;

	if (fade.step < 0.0f)
		current_gain = std::max(current_gain, fade.target_gain);
	else
		current_gain = std::min(current_gain, fade.target_gain);

	if (current_gain <= 0.0f) {
		// stop sound
		m_stopped_means_dead = true;
		alSourceStop(m_source_id);

		m_fade_state = nullopt;
		return false;
	}

	setGain(current_gain);

	if (current_gain == fade.target_gain) {
		m_fade_state = nullopt;
		return false;
	} else {
		return true;
	}
}

void PlayingSound::updatePosVel(const v3f &pos, const v3f &vel)
{
	alSourcei(m_source_id, AL_SOURCE_RELATIVE, false);
	alSource3f(m_source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
	alSource3f(m_source_id, AL_VELOCITY, vel.X, vel.Y, vel.Z);
	// Using alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED) and setting reference
	// distance to clamp gain at <1 node distance avoids excessive volume when
	// closer.
	alSourcef(m_source_id, AL_REFERENCE_DISTANCE, 1.0f);

	warn_if_al_error("PlayingSound::updatePosVel");
}

/*
 * OpenALSoundManager class
 */

void OpenALSoundManager::stepStreams(f32 dtime)
{
	//~ // if m_stream_timer becomes 0 this step, issue all sounds, otherwise
	//~ // spread work across steps
	//~ size_t num_issued_sounds = std::ceil(m_sounds_streaming_treating.size()
			//~ * (m_stream_timer <= dtime ? 1.0f : m_stream_timer / dtime));

	//~ //TODO

	//~ m_stream_timer -= dtime;
	//~ if (m_stream_timer <= 0) {
		//~ m_stream_timer = STREAM_STEP_TIME; //TODO
		//~ std::swap(m_sounds_streaming_waiting, m_sounds_streaming_treating);
	//~ }

	for (size_t i = 0; i < m_sounds_streaming.size();) {
		std::shared_ptr<PlayingSound> snd = m_sounds_streaming[i].lock();
		if (snd) {
			if (snd->stepStream()) {
				++i;
				continue;
			}
		}
		// sound no longer needs to be streamed => remove
		m_sounds_streaming[i] = std::move(m_sounds_streaming.back());
		m_sounds_streaming.pop_back();
		// continue with same i
	}
}

void OpenALSoundManager::doFades(f32 dtime)
{
	for (size_t i = 0; i < m_sounds_fading.size();) {
		std::shared_ptr<PlayingSound> snd = m_sounds_fading[i].lock();
		if (snd) {
			if (snd->doFade(dtime)) {
				// needs more fading later, keep in m_sounds_fading
				++i;
				continue;
			}
		}

		// sound no longer needs to be faded
		m_sounds_fading[i] = std::move(m_sounds_fading.back());
		m_sounds_fading.pop_back();
		// continue with same i
	}
}

std::shared_ptr<ISoundDataOpen> OpenALSoundManager::openSingleSound(const std::string &sound_name)
{
	// if already open, nothing to do
	auto it = m_sound_datas_open.find(sound_name);
	if (it != m_sound_datas_open.end())
		return it->second;

	// find unopened data
	auto it_unopen = m_sound_datas_unopen.find(sound_name);
	if (it_unopen == m_sound_datas_unopen.end())
		return nullptr;
	std::unique_ptr<ISoundDataUnopen> unopn_snd = std::move(it_unopen->second);
	m_sound_datas_unopen.erase(it_unopen);

	// open
	std::shared_ptr<ISoundDataOpen> opn_snd = std::move(*unopn_snd).open(sound_name);
	if (!opn_snd)
		return nullptr;
	m_sound_datas_open.emplace(sound_name, opn_snd);
	return opn_snd;
}

std::string OpenALSoundManager::getLoadedSoundNameFromGroup(const std::string &group_name)
{
	std::string chosen_sound_name = "";

	auto it_groups = m_sound_groups.find(group_name);
	if (it_groups == m_sound_groups.end())
		return chosen_sound_name;

	std::vector<std::string> &group_sounds = it_groups->second;
	while (!group_sounds.empty()) {
		// choose one by random
		int j = myrand() % group_sounds.size();
		chosen_sound_name = group_sounds[j];

		// find chosen one
		std::shared_ptr<ISoundDataOpen> snd = openSingleSound(chosen_sound_name);
		if (snd)
			break;

		// it doesn't exist
		// remove it from the group and try again
		group_sounds[j] = std::move(group_sounds.back());
		group_sounds.pop_back();
	}

	return chosen_sound_name;
}

std::string OpenALSoundManager::getOrLoadLoadedSoundNameFromGroup(const std::string &group_name)
{
	std::string sound_name = getLoadedSoundNameFromGroup(group_name);
	if (!sound_name.empty())
		return sound_name;

	// load
	std::vector<std::string> paths = m_local_fallback_paths_giver
			->getLocalFallbackPathsForSoundname(group_name);
	for (const std::string &path : paths) {
		if (loadSoundFile(path, path))
			addSoundToGroup(path, group_name);
	}
	return getLoadedSoundNameFromGroup(group_name);
}

std::shared_ptr<PlayingSound> OpenALSoundManager::createPlayingSound(const std::string &sound_name,
		bool loop, f32 volume, f32 pitch, f32 time_offset, const Optional<v3f> &pos_opt)
{
	infostream << "OpenALSoundManager: Creating playing sound" << std::endl;
	warn_if_al_error("before createPlayingSound");

	std::shared_ptr<ISoundDataOpen> lsnd = openSingleSound(sound_name);
	if (!lsnd) {
		// does not happen because of the call to getLoadedSoundNameFromGroup
		errorstream << "OpenALSoundManager::createPlayingSound: " <<
				"sound '" << sound_name << "' disappeared." << std::endl;
		return nullptr;
	}

	ALuint source_id;
	alGenSources(1, &source_id);
	if (warn_if_al_error("createPlayingSound (alGenSources)") != AL_NO_ERROR) {
		// happens ie. if there are too many sources (out of memory)
		return nullptr;
	}

	auto sound = std::make_shared<PlayingSound>(source_id, std::move(lsnd), loop,
			volume, pitch, time_offset, pos_opt);

	sound->play();
	warn_if_al_error("createPlayingSound");
	return sound;
}

void OpenALSoundManager::playSoundGeneric(sound_handle_t id, const std::string &group_name,
		bool loop, f32 volume, f32 fade, f32 pitch, bool use_local_fallback,
		f32 time_offset, const Optional<v3f> &pos_opt)
{
	if (id == 0)
		id = allocateId(1);

	if (group_name.empty()) {
		reportRemovedSound(id);
		return;
	}

	// choose random sound name from group name
	std::string sound_name = use_local_fallback ?
			getOrLoadLoadedSoundNameFromGroup(group_name) :
			getLoadedSoundNameFromGroup(group_name);
	if (sound_name.empty()) {
		infostream << "OpenALSoundManager: \"" << group_name << "\" not found."
				<< std::endl;
		reportRemovedSound(id);
		return;
	}

	volume = std::max(0.0f, volume);

	if (!(pitch > 0.0f)) {
		warningstream << "OpenALSoundManager::playSoundGeneric: illegal pitch value: "
				<< time_offset << std::endl;
		pitch = 1.0f;
	}

	if (!std::isfinite(time_offset)) {
		warningstream << "OpenALSoundManager::playSoundGeneric: illegal time_offset value: "
				<< time_offset << std::endl;
		time_offset = 0.0f;
	}

	// play it
	std::shared_ptr<PlayingSound> sound = createPlayingSound(sound_name,
			loop, fade > 0.0f ? 0.0f : volume, pitch, time_offset, pos_opt);
	if (!sound) {
		reportRemovedSound(id);
		return;
	}

	// add to streaming sounds if streaming
	if (sound->isStreaming())
		m_sounds_streaming.push_back(sound);

	m_sounds_playing.emplace(id, std::move(sound));

	if (fade > 0.0f)
		fadeSound(id, fade, volume);
}

int OpenALSoundManager::removeDeadSounds()
{
	int num_deleted_sounds = 0;

	for (auto it = m_sounds_playing.begin(); it != m_sounds_playing.end();) {
		sound_handle_t id = it->first;
		PlayingSound &sound = *it->second;
		// If dead, remove it
		if (sound.isDead()) {
			it = m_sounds_playing.erase(it);
			reportRemovedSound(id);
			++num_deleted_sounds;
		} else {
			++it;
		}
	}

	return num_deleted_sounds;
}

OpenALSoundManager::OpenALSoundManager(SoundManagerSingleton *smg,
		std::unique_ptr<SoundLocalFallbackPathsGiver> local_fallback_paths_giver) :
	m_local_fallback_paths_giver(std::move(local_fallback_paths_giver)),
	m_device(smg->m_device.get()),
	m_context(smg->m_context.get())
{
	SANITY_CHECK(!!m_local_fallback_paths_giver);

	infostream << "Audio: Initialized: OpenAL " << std::endl;
}

OpenALSoundManager::~OpenALSoundManager()
{
	infostream << "Audio: Deinitializing..." << std::endl;

	//~ // first delete sources
	//~ m_sounds_playing.clear();

	//~ // then buffers
	//~ m_sound_datas_open.clear();

	//~ // other stuff
	//~ m_sound_datas_unopen.clear();

	//~ infostream << "Audio: Deinitialized." << std::endl;
}

/* Interface */

void OpenALSoundManager::step(f32 dtime)
{
	m_time_until_dead_removal -= dtime;
	if (m_time_until_dead_removal <= 0.0f) {
		if (!m_sounds_playing.empty()) {
			verbosestream << "OpenALSoundManager::step(): "
					<< m_sounds_playing.size() << " playing sounds, "
					<< m_sound_datas_unopen.size() << " unopen sounds, "
					<< m_sound_datas_open.size() << " open sounds and "
					<< m_sound_groups.size() << " sound groups loaded"
					<< std::endl;
		}

		int num_deleted_sounds = removeDeadSounds();

		if (num_deleted_sounds != 0)
			verbosestream << "OpenALSoundManager::step(): deleted "
					<< num_deleted_sounds << " dead playing sounds" << std::endl;

		m_time_until_dead_removal = REMOVE_DEAD_SOUNDS_INTERVAL;
	}

	doFades(dtime);
	stepStreams(dtime);
}

void OpenALSoundManager::updateListener(const v3f &pos_, const v3f &vel_, const v3f &at_, const v3f &up_)
{
	v3f pos = swap_handedness(pos_);
	v3f vel = swap_handedness(vel_);
	v3f at = swap_handedness(at_);
	v3f up = swap_handedness(up_);
	ALfloat orientation[6] = {at.X, at.Y, at.Z, up.X, up.Y, up.Z};

	alListener3f(AL_POSITION, pos.X, pos.Y, pos.Z);
	alListener3f(AL_VELOCITY, vel.X, vel.Y, vel.Z);
	alListenerfv(AL_ORIENTATION, orientation);
	warn_if_al_error("updateListener");
}

void OpenALSoundManager::setListenerGain(f32 gain)
{
	alListenerf(AL_GAIN, gain);
}

bool OpenALSoundManager::loadSoundFile(const std::string &name, const std::string &filepath)
{
	// do not add twice
	if (m_sound_datas_open.count(name) != 0 || m_sound_datas_unopen.count(name) != 0)
		return false;

	// coarse check
	if (!can_open_file(filepath))
		return false;

	// remember for lazy loading
	m_sound_datas_unopen.emplace(name, std::make_unique<SoundDataUnopenFile>(filepath));
	return true;
}

bool OpenALSoundManager::loadSoundData(const std::string &name, std::string &&filedata)
{
	// do not add twice
	if (m_sound_datas_open.count(name) != 0 || m_sound_datas_unopen.count(name) != 0)
		return false;

	// remember for lazy loading
	m_sound_datas_unopen.emplace(name, std::make_unique<SoundDataUnopenBuffer>(std::move(filedata)));
	return true;
}

void OpenALSoundManager::addSoundToGroup(const std::string &sound_name, const std::string &group_name)
{
	auto it_groups = m_sound_groups.find(group_name);
	if (it_groups != m_sound_groups.end())
		it_groups->second.push_back(sound_name);
	else
		m_sound_groups.emplace(group_name, std::vector<std::string>{sound_name});
}

void OpenALSoundManager::playSound(sound_handle_t id, const SimpleSoundSpec &spec)
{
	return playSoundGeneric(id, spec.name, spec.loop, spec.gain, spec.fade, spec.pitch,
			spec.use_local_fallback, spec.time_offset, nullopt); //TODO: pass spec?
}

void OpenALSoundManager::playSoundAt(sound_handle_t id, const SimpleSoundSpec &spec, const v3f &pos_)
{
	v3f pos = swap_handedness(pos_);

	// AL_REFERENCE_DISTANCE was once reduced from 3 nodes to 1 node.
	// We compensate this by multiplying the volume by 3.
	// Note that this is just done to newly created sounds (and not in
	// updateSoundGain) because it was like this for many versions, so someone
	// might depend on this (inconsistent) behaviour.
	f32 volume = spec.gain * 3.0f;

	return playSoundGeneric(id, spec.name, spec.loop, volume, 0.0f /* no fade */,
			spec.pitch, spec.use_local_fallback, spec.time_offset, pos);
}

void OpenALSoundManager::stopSound(sound_handle_t sound)
{
	m_sounds_playing.erase(sound);
	reportRemovedSound(sound);
}

void OpenALSoundManager::fadeSound(sound_handle_t soundid, f32 step, f32 target_gain)
{
	// Ignore the command if step isn't valid.
	if (step == 0.0f)
		return;
	auto sound_it = m_sounds_playing.find(soundid);
	if (sound_it == m_sounds_playing.end())
		return; // No sound to fade
	PlayingSound &sound = *sound_it->second;
	if (sound.fade(step, target_gain))
		m_sounds_fading.emplace_back(sound_it->second);
}

void OpenALSoundManager::updateSoundPosition(sound_handle_t id, const v3f &pos_)
{
	v3f pos = swap_handedness(pos_);

	auto i = m_sounds_playing.find(id);
	if (i == m_sounds_playing.end())
		return;
	i->second->updatePosVel(pos, v3f(0.0f, 0.0f, 0.0f));
}

void OpenALSoundManager::updateSoundGain(sound_handle_t id, f32 gain)
{
	auto i = m_sounds_playing.find(id);
	if (i == m_sounds_playing.end())
		return;
	i->second->setGain(gain);
}
