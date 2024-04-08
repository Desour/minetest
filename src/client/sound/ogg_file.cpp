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
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "ogg_file.h"

#include <cassert>
#include <cstring> // memcpy

namespace sound {

/*
 * OggVorbisBufferSource struct
 */

size_t OggVorbisBufferSource::read_func(void *ptr, size_t size, size_t nmemb,
		void *datasource) noexcept
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	size_t copied_size = MYMIN(s->buf.size() - s->cur_offset, size);
	memcpy(ptr, s->buf.data() + s->cur_offset, copied_size);
	s->cur_offset += copied_size;
	return copied_size;
}

int OggVorbisBufferSource::seek_func(void *datasource, ogg_int64_t offset, int whence) noexcept
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	if (whence == SEEK_SET) {
		if (offset < 0 || (size_t)offset > s->buf.size()) {
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
	} else if (whence == SEEK_END) {
		if (offset > 0 || (size_t)-offset > s->buf.size()) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset = s->buf.size() - offset;
		return 0;
	}
	return -1;
}

int OggVorbisBufferSource::close_func(void *datasource) noexcept
{
	auto s = reinterpret_cast<OggVorbisBufferSource *>(datasource);
	delete s;
	return 0;
}

long OggVorbisBufferSource::tell_func(void *datasource) noexcept
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

std::optional<OggFileDecodeInfo> RAIIOggFile::getDecodeInfo(const std::string &filename_for_logging)
{
	OggFileDecodeInfo ret;
	ret.name_for_logging = filename_for_logging;

	long num_logical_bitstreams = ov_streams(&m_file);
	if (num_logical_bitstreams != 1) {
		warningstream << "Audio: Only files with 1 logical bitstream supported, but has "
				<< num_logical_bitstreams << ", playback may be broken: "
				<< ret.name_for_logging << std::endl;
	}

	vorbis_info *pInfo = ov_info(&m_file, 0);
	if (!pInfo) {
		warningstream << "Audio: Can't decode. ov_info failed for: "
				<< ret.name_for_logging << std::endl;
		return std::nullopt;
	}

	if (pInfo->channels == 1) {
		ret.is_stereo = false;
		ret.format = AL_FORMAT_MONO16;
		ret.bytes_per_sample = 2;
	} else if (pInfo->channels == 2) {
		ret.is_stereo = true;
		ret.format = AL_FORMAT_STEREO16;
		ret.bytes_per_sample = 4;
	} else {
		warningstream << "Audio: Can't decode. Sound is neither mono nor stereo: "
				<< ret.name_for_logging << std::endl;
		return std::nullopt;
	}

	ret.freq = pInfo->rate;

	if (!ov_seekable(&m_file)) {
		warningstream << "Audio: Can't decode. Sound is not seekable, can't get length: "
				<< ret.name_for_logging << std::endl;
		return std::nullopt;
	}
	if (ov_pcm_total(&m_file, -1) < 0) {
		warningstream << "huh? "
				<< ret.name_for_logging << std::endl;
	}
	if (ret.name_for_logging == "mcl_sounds_place_node_water.ogg") {
		warningstream << "len: " << ov_pcm_total(&m_file, -1) << std::endl;
	}
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

	if (pcm_end > decode_info.length_samples) {
		errorstream << "Audio: pcm_end too high ("
				<< pcm_end << " > " << decode_info.length_samples << "): "
				<< decode_info.name_for_logging << std::endl;
	}

	assert(pcm_end <= decode_info.length_samples);
	assert(pcm_start <= decode_info.length_samples);

	// seek
	if (ov_pcm_tell(&m_file) != pcm_start) {
		if (ov_pcm_seek(&m_file, pcm_start) != 0) {
			warningstream << "Audio: Error decoding (could not seek) "
					<< decode_info.name_for_logging << std::endl;
			return RAIIALSoundBuffer();
		}
		assert(ov_pcm_tell(&m_file) == pcm_start);
	}
	long last_pcm_tell = ov_pcm_tell(&m_file);

	const size_t size = static_cast<size_t>(pcm_end - pcm_start)
			* decode_info.bytes_per_sample;

	std::unique_ptr<char[]> snd_buffer(new char[size]);

	// read size bytes
	size_t read_count = 0;
	int bitstream;
	//~ bool tmp_clear = true;
	while (read_count < size) {
		// Read up to a buffer's worth of decoded sound data
		long num_bytes = ov_read(&m_file, &snd_buffer[read_count], size - read_count,
				endian, word_size, word_signed, &bitstream);

		if (num_bytes <= 0) {

			long num_bytes2 = ov_read(&m_file, &snd_buffer[read_count], size - read_count,
					endian, word_size, word_signed, &bitstream);
			if (bitstream != 0)
				errorstream << "last bitstream: " << bitstream << std::endl;

			std::string_view errstr = [&]{
				switch (num_bytes) {
				case 0: return "EOF";
				case OV_HOLE: return "OV_HOLE";
				case OV_EBADLINK: return "OV_EBADLINK";
				case OV_EINVAL: return "OV_EINVAL";
				default: return "unknown";
				}
			}();
			warningstream << "Audio: Error decoding \""
					<< decode_info.name_for_logging
					<< "\": " << errstr << " (" << num_bytes << ")" << std::endl;
			warningstream << "read_count: " << read_count << std::endl;
			warningstream << "size: " << size << std::endl;
			warningstream << "num_bytes2: " << num_bytes2 << std::endl;
			warningstream << "ov_pcm_tell: " << ov_pcm_tell(&m_file) << std::endl;
			warningstream << "decode_info.length_samples: " << decode_info.length_samples << std::endl;
			return RAIIALSoundBuffer();
		}

		//~ if (tmp_clear) {
			//~ memset(&snd_buffer[read_count], 0, num_bytes);
		//~ }
		//~ tmp_clear = true;

		// only one logical bitstream supported
		assert(bitstream == 0);

		long current_pcm_tell = ov_pcm_tell(&m_file);
		long current_expected_byte_offset = last_pcm_tell * decode_info.bytes_per_sample + num_bytes;
		assert(num_bytes % decode_info.bytes_per_sample == 0);
		if (num_bytes >= 0
				&& current_pcm_tell * decode_info.bytes_per_sample != current_expected_byte_offset) {
			warningstream << "=== ov_read lies ===" << std::endl;
			warningstream << "last_pcm_tell: " << last_pcm_tell << std::endl;
			warningstream << "ov_pcm_tell: " << ov_pcm_tell(&m_file) << std::endl;
			warningstream << "diff: " << (ov_pcm_tell(&m_file) - last_pcm_tell) << std::endl;
			warningstream << "num_bytes: " << num_bytes << std::endl;
			warningstream << "num samples: " << (num_bytes / decode_info.bytes_per_sample) << std::endl;
			warningstream << "requested bytes: " << (size - read_count) << std::endl;

			//~ long to_fill_up = current_pcm_tell * decode_info.bytes_per_sample - current_expected_byte_offset;
			//~ memset(&snd_buffer[read_count + num_bytes], 0, to_fill_up);
			//~ num_bytes += to_fill_up;
			assert(ov_pcm_seek(&m_file, current_expected_byte_offset / decode_info.bytes_per_sample) == 0);
			assert(ov_pcm_tell(&m_file) == current_expected_byte_offset / decode_info.bytes_per_sample);
			//~ tmp_clear = false;
		}
		last_pcm_tell = ov_pcm_tell(&m_file);

		read_count += num_bytes;
	}

	// load buffer to openal
	RAIIALSoundBuffer snd_buffer_id = RAIIALSoundBuffer::generate();
	alBufferData(snd_buffer_id.get(), decode_info.format, &(snd_buffer[0]), size,
			decode_info.freq);

	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		warningstream << "Audio: OpenAL error: " << getAlErrorString(error)
				<< "preparing sound buffer for sound \""
				<< decode_info.name_for_logging << "\"" << std::endl;
	}

	return snd_buffer_id;
}

} // namespace sound
