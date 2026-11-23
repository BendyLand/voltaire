/**************************************************************************/
/*  video_stream_theora.cpp                                               */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/
// --- Inlined from modules/theora/video_stream_theora.h ---
#include "core/io/resource_loader.h"

class ResourceFormatLoaderTheora final : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;
};

#ifdef TOOLS_ENABLED
#include <theora/theoraenc.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include "core/io/file_access.h"
#include "servers/audio/audio_server.h"
#include "main/main.h"

class MovieWriterOGV final : public MovieWriter
{
	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;
	uint32_t audio_ch = 0;
	uint32_t audio_frames = 0;

	Ref<FileAccess> f;

	// Vorbis quality -0.1 to 1 (-0.1 yields smallest files but lowest fidelity; 1 yields highest
	// fidelity but large files. '0.2' is a reasonable default).
	float audio_quality = 0.5;

	// Bitrate target for Theora video.
	int video_bitrate = 0;

	// Theora quality selector from 0 to 1.0 (0 yields smallest files but lowest video quality. 1.0
	// yields highest fidelity but large files).
	float video_quality = 0.75;

	// Video stream keyframe frequency (one every N frames).
	ogg_uint32_t keyframe_frequency = 64;

	// Sets the encoder speed level. Higher speed levels favor quicker encoding over better quality
	// per bit. Depending on the encoding mode, and the internal algorithms used, quality may
	// actually improve with higher speeds, but in this case bitrate will also likely increase. The
	// maximum value, and the meaning of each value, are implementation-specific and may change
	// depending on the current encoding mode.
	int speed = 4;

	// Take physical pages, weld into a logical stream of packets.
	ogg_stream_state to;

	// Take physical pages, weld into a logical stream of packets.
	ogg_stream_state vo;

	// Theora encoding context.
	th_enc_ctx* td;

	// Theora bitstream information.
	th_info ti;

	// Theora comment information.
	th_comment tc;

	// Vorbis bitstream information.
	vorbis_info vi;

	// Vorbis comment information.
	vorbis_comment vc;

	// Central working state for the packet->PCM decoder.
	vorbis_dsp_state vd;

	// Local working space for packet->PCM decode.
	vorbis_block vb;

	// Video buffer.
	uint8_t *y, *u, *v;
	th_ycbcr_buffer ycbcr;

	bool audio_flag = false;
	bool video_flag = false;
	ogg_page audio_page;
	ogg_page video_page;
	ogg_page backup_page;
	unsigned int backup_page_size = 0;
	unsigned char* backup_page_data = nullptr;

	void write_to_file(bool p_finish = false);
	void push_audio(const int32_t* p_audio_data);
	void push_video(const Ref<Image>& p_image);
	void pull_audio(bool p_last = false);
	void pull_video(bool p_last = false);
	void save_page(ogg_page page);
	void restore_page(ogg_page* page);

	inline int ilog(unsigned _v)
	{
		int ret;
		for (ret = 0; _v; ret++) {
			_v >>= 1;
		}
		return ret;
	}

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String>* r_extensions) const override;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path) override;
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String& p_path) const override;

public:
	MovieWriterOGV();
};
#endif // TOOLS_ENABLED

// --- End modules/theora/video_stream_theora.h ---
#include "register_types.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"

#include <theora/theoradec.h>
#include <thirdparty/misc/yuv2rgb.h>
#include <vorbis/codec.h>

struct VideoStreamPlayback::TheoraData
{
	Image::Format format = Image::Format::FORMAT_L8;
	Vector<uint8_t> frame_data;
	int frames_pending = 0;
	Ref<FileAccess> file;
	String file_name;
	Point2i size;
	Rect2i region;

	float* audio_buffer = nullptr;
	int audio_buffer_size = 0;
	int audio_ptr_start = 0;
	int audio_ptr_end = 0;

	bool theora_eos = false;
	bool vorbis_eos = false;

	ogg_sync_state oy = {};
	ogg_stream_state vo = {};
	ogg_stream_state to = {};
	th_info ti = {};
	th_comment tc = {};
	th_dec_ctx* td = nullptr;
	vorbis_info vi = {};
	vorbis_dsp_state vd = {};
	vorbis_block vb = {};
	vorbis_comment vc = {};
	th_pixel_fmt px_fmt = TH_PF_420;
	double frame_duration = 0;
	double stream_length = 0;
	int64_t stream_data_offset = 0;
	int64_t stream_data_size = 0;

	int pp_level_max = 0;
	int pp_level = 0;
	int pp_inc = 0;

	bool playing = false;
	bool paused = false;

	bool dup_frame = false;
	bool has_video = false;
	bool has_audio = false;
	bool video_ready = false;
	bool video_done = false;
	bool audio_done = false;

	double time = 0;
	double next_frame_time = 0;
	double current_frame_time = 0;
	double delay_compensation = 0;

	Ref<ImageTexture> texture;
	int audio_track = 0;

	int buffer_data()
	{
		char* buffer = ogg_sync_buffer(&oy, 4096);
		uint64_t bytes = file->get_buffer((uint8_t*)buffer, 4096);
		ogg_sync_wrote(&oy, bytes);
		return bytes;
	}

	int queue_page(ogg_page* page)
	{
		ogg_stream_pagein(&to, page);
		if (to.e_o_s) {
			theora_eos = true;
		}
		if (has_audio) {
			ogg_stream_pagein(&vo, page);
			if (vo.e_o_s) {
				vorbis_eos = true;
			}
		}
		return 0;
	}

	int read_page(ogg_page* page)
	{
		int ret = 0;
		while (ret <= 0) {
			ret = ogg_sync_pageout(&oy, page);
			if (ret <= 0) {
				int bytes = buffer_data();
				if (bytes == 0) {
					return 0;
				}
			}
		}
		return ret;
	}

	double get_page_time(ogg_page* page)
	{
		uint64_t granulepos = ogg_page_granulepos(page);
		int page_serialno = ogg_page_serialno(page);
		double page_time = -1;

		if (page_serialno == to.serialno) {
			page_time = th_granule_time(td, granulepos);
		}
		if (has_audio && page_serialno == vo.serialno) {
			page_time = vorbis_granule_time(&vd, granulepos);
		}

		return page_time;
	}

	int feed_pages()
	{
		int pages = 0;
		ogg_page og;

		while (pages == 0) {
			while (ogg_sync_pageout(&oy, &og) > 0) {
				queue_page(&og);
				pages++;
			}
			if (pages == 0) {
				int bytes = buffer_data();
				if (bytes == 0) {
					break;
				}
			}
		}

		return pages;
	}

	int64_t seek_streams(
		double p_time, int64_t& cur_video_granulepos, int64_t& cur_audio_granulepos)
	{
		const int64_t min_seek = 512 * 1024;
		int64_t target_video_granulepos;
		int64_t target_audio_granulepos;
		double target_time = 0;
		int64_t seek_pos;

		if (p_time == -1) {
			seek_pos = MAX(stream_data_size - min_seek, stream_data_offset);
			target_video_granulepos = INT64_MAX;
			target_audio_granulepos = INT64_MAX;
		}
		else {
			int64_t video_frame = (int64_t)(p_time / frame_duration);
			target_video_granulepos = MAX(1LL, video_frame - (1LL << ti.keyframe_granule_shift))
									  << ti.keyframe_granule_shift;
			target_audio_granulepos = 0;
			seek_pos = MAX(((target_video_granulepos >> ti.keyframe_granule_shift) - 1) *
							   frame_duration * stream_data_size / stream_length,
				stream_data_offset);
			target_time = th_granule_time(td, target_video_granulepos);
			if (has_audio) {
				target_audio_granulepos = video_frame * frame_duration * vi.rate;
				target_time = MIN(target_time, vorbis_granule_time(&vd, target_audio_granulepos));
			}
		}

		int64_t video_seek_pos = seek_pos;
		int64_t audio_seek_pos = seek_pos;
		double backtrack_time = 0;
		bool video_catch = false;
		bool audio_catch = false;
		int64_t last_video_granule_seek_pos = seek_pos;
		int64_t last_audio_granule_seek_pos = seek_pos;

		cur_video_granulepos = -1;
		cur_audio_granulepos = -1;

		while (!video_catch || (has_audio && !audio_catch)) {
			if (seek_pos < stream_data_offset) {
				seek_pos = stream_data_offset;
			}
			file->seek(seek_pos);
			ogg_sync_reset(&oy);

			backtrack_time = 0;
			last_video_granule_seek_pos = seek_pos;
			last_audio_granule_seek_pos = seek_pos;
			while (!video_catch || (has_audio && !audio_catch)) {
				ogg_page page;
				uint64_t last_seek_pos = file->get_position() - oy.fill + oy.returned;
				int ret = read_page(&page);
				if (ret <= 0) {
					if (seek_pos < stream_data_offset) {
						return -1;
					}
					seek_pos -= min_seek;
					break;
				}
				int64_t cur_granulepos = ogg_page_granulepos(&page);
				if (cur_granulepos >= 0) {
					int page_serialno = ogg_page_serialno(&page);
					if (!video_catch && page_serialno == to.serialno) {
						if (cur_granulepos >= target_video_granulepos) {
							video_catch = true;
							if (cur_video_granulepos < 0) {
								backtrack_time = MAX(backtrack_time,
									1 + th_granule_time(td, cur_granulepos) - target_time);
							}
						}
						else {
							video_seek_pos = last_video_granule_seek_pos;
							cur_video_granulepos = cur_granulepos;
						}
						last_video_granule_seek_pos = last_seek_pos;
					}
					if ((has_audio && !audio_catch) && page_serialno == vo.serialno) {
						if (cur_granulepos >= target_audio_granulepos) {
							audio_catch = true;
							if (cur_audio_granulepos < 0) {
								backtrack_time = MAX(backtrack_time,
									1 + vorbis_granule_time(&vd, cur_granulepos) - target_time);
							}
						}
						else {
							audio_seek_pos = last_audio_granule_seek_pos;
							cur_audio_granulepos = cur_granulepos;
						}
						last_audio_granule_seek_pos = last_seek_pos;
					}
				}
			}
			if (backtrack_time > 0) {
				if (seek_pos <= stream_data_offset) {
					break;
				}
				int64_t delta_seek = MAX(backtrack_time * stream_data_size / stream_length, min_seek);
				seek_pos -= delta_seek;
			}
			video_catch = cur_video_granulepos != -1;
			audio_catch = cur_audio_granulepos != -1;
		}

		if (cur_video_granulepos < (1LL << ti.keyframe_granule_shift)) {
			video_seek_pos = stream_data_offset;
			cur_video_granulepos = 1LL << ti.keyframe_granule_shift;
		}
		if (has_audio) {
			if (cur_audio_granulepos == -1) {
				audio_seek_pos = stream_data_offset;
				cur_audio_granulepos = 0;
			}
			seek_pos = MIN(video_seek_pos, audio_seek_pos);
		}
		else {
			seek_pos = video_seek_pos;
		}

		return seek_pos;
	}

	void video_write(th_ycbcr_buffer yuv)
	{
		uint8_t* w = frame_data.ptrw();
		char* dst = (char*)w;
		uint32_t y_offset = region.position.y * yuv[0].stride + region.position.x;
		uint32_t uv_offset = 0;

		if (px_fmt == TH_PF_444) {
			uv_offset += region.position.y * yuv[1].stride + region.position.x;
			yuv444_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}
		else if (px_fmt == TH_PF_422) {
			uv_offset += region.position.y * yuv[1].stride + region.position.x / 2;
			yuv422_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}
		else if (px_fmt == TH_PF_420) {
			uv_offset += region.position.y * yuv[1].stride / 2 + region.position.x / 2;
			yuv420_2_rgb8888((uint8_t*)dst, (uint8_t*)yuv[0].data + y_offset,
				(uint8_t*)yuv[1].data + uv_offset, (uint8_t*)yuv[2].data + uv_offset, region.size.x,
				region.size.y, yuv[0].stride, yuv[1].stride, region.size.x << 2);
		}

		Ref<Image> img;
		img.instantiate(region.size.x, region.size.y, false, Image::FORMAT_RGBA8, frame_data);
		texture->update(img);
	}

	void clear()
	{
		if (!file.is_null()) {
			file.unref();
		}
		if (has_audio) {
			vorbis_block_clear(&vb);
			vorbis_dsp_clear(&vd);
			vorbis_comment_clear(&vc);
			vorbis_info_clear(&vi);
			ogg_stream_clear(&vo);
			if (audio_buffer_size) {
				memdelete_arr(audio_buffer);
			}
		}
		if (has_video) {
			th_decode_free(td);
			th_comment_clear(&tc);
			th_info_clear(&ti);
			ogg_stream_clear(&to);
			ogg_sync_clear(&oy);
		}

		audio_buffer = nullptr;
		playing = false;
		has_video = false;
		has_audio = false;
		theora_eos = false;
		vorbis_eos = false;
	}

	void find_streams(th_setup_info*& ts)
	{
		ogg_stream_state test;
		ogg_packet op;
		ogg_page og;
		int stateflag = 0;
		int audio_track_skip = audio_track;

		while (!stateflag) {
			int ret = buffer_data();
			if (!ret) {
				break;
			}
			while (ogg_sync_pageout(&oy, &og) > 0) {
				if (!ogg_page_bos(&og)) {
					queue_page(&og);
					stateflag = 1;
					break;
				}

				ogg_stream_init(&test, ogg_page_serialno(&og));
				ogg_stream_pagein(&test, &og);
				ogg_stream_packetout(&test, &op);

				if (!has_video && th_decode_headerin(&ti, &tc, &ts, &op) >= 0) {
					memcpy(&to, &test, sizeof(test));
					has_video = true;
				}
				else if (!has_audio && vorbis_synthesis_headerin(&vi, &vc, &op) >= 0) {
					if (audio_track_skip) {
						vorbis_info_clear(&vi);
						vorbis_comment_clear(&vc);
						ogg_stream_clear(&test);
						vorbis_info_init(&vi);
						vorbis_comment_init(&vc);
						audio_track_skip--;
					}
					else {
						memcpy(&vo, &test, sizeof(test));
						has_audio = true;
					}
				}
				else {
					ogg_stream_clear(&test);
				}
			}
		}
	}

	void read_headers(th_setup_info*& ts)
	{
		ogg_packet op;
		int theora_header_packets = 1;
		int vorbis_header_packets = 1;

		while (theora_header_packets < 3 || (has_audio && vorbis_header_packets < 3)) {
			while (theora_header_packets < 3 && ogg_stream_packetout(&to, &op) > 0) {
				if (th_decode_headerin(&ti, &tc, &ts, &op) > 0) {
					theora_header_packets++;
				}
			}

			while (has_audio && vorbis_header_packets < 3 && ogg_stream_packetout(&vo, &op) > 0) {
				if (!vorbis_synthesis_headerin(&vi, &vc, &op)) {
					vorbis_header_packets++;
				}
			}

			if (theora_header_packets < 3 || (has_audio && vorbis_header_packets < 3)) {
				ogg_page page;
				if (read_page(&page)) {
					queue_page(&page);
				}
				else {
					fprintf(stderr, "End of file while searching for codec headers.\n");
					break;
				}
			}
		}

		has_video = theora_header_packets == 3;
		has_audio = vorbis_header_packets == 3;
	}

	double get_time() const
	{
		return time - delay_compensation;
	}

	_FORCE_INLINE_ bool send_audio(AudioMixCallback p_callback, void* p_udata)
	{
		if (audio_ptr_end > 0) {
			int mixed = p_callback
				? p_callback(
					  p_udata, &audio_buffer[audio_ptr_start * vi.channels], audio_ptr_end - audio_ptr_start)
				: (audio_ptr_end - audio_ptr_start);
			audio_ptr_start += mixed;
			if (audio_ptr_start == audio_ptr_end) {
				audio_ptr_start = 0;
				audio_ptr_end = 0;
			}
			else {
				return false;
			}
		}
		return true;
	}
};

void VideoStreamPlayback::set_file(const String& p_file)
{
	ERR_FAIL_COND(data->playing);
	th_setup_info* ts = nullptr;

	data->clear();

	data->file = FileAccess::open(p_file, FileAccess::READ);
	ERR_FAIL_COND_MSG(data->file.is_null(), "Cannot open file '" + p_file + "'.");

	data->file_name = p_file;

	ogg_sync_init(&data->oy);

	vorbis_info_init(&data->vi);
	vorbis_comment_init(&data->vc);

	th_comment_init(&data->tc);
	th_info_init(&data->ti);

	memset(&data->to, 0, sizeof(data->to));
	memset(&data->vo, 0, sizeof(data->vo));

	data->find_streams(ts);
	data->read_headers(ts);

	if (!data->has_audio) {
		vorbis_comment_clear(&data->vc);
		vorbis_info_clear(&data->vi);
		if (!ogg_stream_check(&data->vo)) {
			ogg_stream_clear(&data->vo);
		}
	}

	if (!data->has_video) {
		th_setup_free(ts);
		th_comment_clear(&data->tc);
		th_info_clear(&data->ti);
		if (!ogg_stream_check(&data->to)) {
			ogg_stream_clear(&data->to);
		}
		data->file.unref();

		vorbis_comment_clear(&data->vc);
		vorbis_info_clear(&data->vi);
		if (!ogg_stream_check(&data->vo)) {
			ogg_stream_clear(&data->vo);
		}

		data->has_video = false;
		data->has_audio = false;
		ERR_FAIL_MSG("File '" + p_file + "' has no video stream.");
	}

	data->td = th_decode_alloc(&data->ti, ts);
	th_setup_free(ts);
	data->px_fmt = data->ti.pixel_fmt;
	switch (data->ti.pixel_fmt) {
	case TH_PF_420:
	case TH_PF_422:
	case TH_PF_444:
		break;
	default:
		WARN_PRINT(" video\n  (UNKNOWN Chroma sampling!)\n");
		break;
	}
	th_decode_ctl(
		data->td, TH_DECCTL_GET_PPLEVEL_MAX, &data->pp_level_max, sizeof(data->pp_level_max));
	data->pp_level = 0;
	th_decode_ctl(data->td, TH_DECCTL_SET_PPLEVEL, &data->pp_level, sizeof(data->pp_level));
	data->pp_inc = 0;

	data->size.x = data->ti.frame_width;
	data->size.y = data->ti.frame_height;
	data->region.position.x = data->ti.pic_x;
	data->region.position.y = data->ti.pic_y;
	data->region.size.x = data->ti.pic_width;
	data->region.size.y = data->ti.pic_height;

	Ref<Image> img = Image::create_empty(
		data->region.size.x, data->region.size.y, false, Image::FORMAT_RGBA8);
	data->texture->set_image(img);
	data->frame_data.resize(data->region.size.x * data->region.size.y * 4);

	data->frame_duration = (double)data->ti.fps_denominator / data->ti.fps_numerator;

	if (data->has_audio) {
		vorbis_synthesis_init(&data->vd, &data->vi);
		vorbis_block_init(&data->vd, &data->vb);
		data->audio_buffer_size = MIN(data->vi.channels, 8) * 1024;
		data->audio_buffer = memnew_arr(float, data->audio_buffer_size);
	}

	data->stream_data_offset = data->file->get_position() - data->oy.fill + data->oy.returned;
	data->stream_data_size = data->file->get_length() - data->stream_data_offset;

	int64_t seek_pos = MAX(data->stream_data_offset, (int64_t)data->file->get_length() - 64 * 1024);
	int64_t video_granulepos = INT64_MAX;
	int64_t audio_granulepos = INT64_MAX;
	data->file->seek(seek_pos);
	seek_pos = data->seek_streams(-1, video_granulepos, audio_granulepos);
	data->file->seek(seek_pos);
	ogg_sync_reset(&data->oy);

	data->stream_length = 0;
	ogg_page page;
	while (data->read_page(&page) > 0) {
		data->stream_length = MAX(data->stream_length, data->get_page_time(&page));
	}

	seek(0);
}

Ref<Texture2D> VideoStreamPlayback::get_texture() const { return data->texture; }

void VideoStreamPlayback::update(double p_delta)
{
	if (data->file.is_null()) {
		return;
	}

	if (!data->playing || data->paused) {
		return;
	}

	data->time += p_delta;

	double comp_time = data->get_time();
	bool audio_ready = false;

	while ((!audio_ready && !data->audio_done) || (!data->video_ready && !data->video_done)) {
		ogg_packet op;

		while (!audio_ready && !data->audio_done) {
			if (!data->send_audio(mix_callback, mix_udata)) {
				audio_ready = true;
				break;
			}

			float** pcm;
			int ret = vorbis_synthesis_pcmout(&data->vd, &pcm);
			if (ret > 0) {
				int frames_read = 0;
				while (frames_read < ret) {
					int m = MIN(data->audio_buffer_size / data->vi.channels, ret - frames_read);
					int count = 0;
					for (int j = 0; j < m; j++) {
						for (int i = 0; i < data->vi.channels; i++) {
							data->audio_buffer[count++] = pcm[i][frames_read + j];
						}
					}
					frames_read += m;
					data->audio_ptr_end = m;
					if (!data->send_audio(mix_callback, mix_udata)) {
						audio_ready = true;
						break;
					}
				}
				vorbis_synthesis_read(&data->vd, frames_read);
			}
			else {
				if (ogg_stream_packetout(&data->vo, &op) > 0) {
					if (vorbis_synthesis(&data->vb, &op) == 0) {
						vorbis_synthesis_blockin(&data->vd, &data->vb);
					}
				}
				else {
					data->audio_done = data->vorbis_eos;
					break;
				}
			}
		}

		while (!data->video_ready && !data->video_done) {
			if (ogg_stream_packetout(&data->to, &op) > 0) {
				if (op.granulepos >= 0) {
					th_decode_ctl(
						data->td, TH_DECCTL_SET_GRANPOS, &op.granulepos, sizeof(op.granulepos));
				}
				int64_t videobuf_granulepos;
				int ret = th_decode_packetin(data->td, &op, &videobuf_granulepos);
				if (ret == 0 || ret == TH_DUPFRAME) {
					data->next_frame_time = th_granule_time(data->td, videobuf_granulepos);
					if (data->next_frame_time > comp_time) {
						data->dup_frame = (ret == TH_DUPFRAME);
						data->video_ready = true;
					}
					else {
						data->pp_inc = data->pp_level > 0 ? -1 : 0;
					}
				}
			}
			else {
				data->video_done = data->theora_eos;
				break;
			}
		}

		if (!data->video_ready || !audio_ready) {
			int ret = data->feed_pages();
			if (ret == 0) {
				data->vorbis_eos = true;
				data->theora_eos = true;
				break;
			}
		}

		double tdiff = data->next_frame_time - comp_time;
		if (tdiff > data->ti.fps_denominator * 0.25 / data->ti.fps_numerator) {
			data->pp_inc = data->pp_level < data->pp_level_max ? 1 : 0;
		}
		else if (tdiff < data->ti.fps_denominator * 0.05 / data->ti.fps_numerator) {
			data->pp_inc = data->pp_level > 0 ? -1 : 0;
		}
	}

	if (!data->video_ready && data->video_done && data->audio_done) {
		stop();
		return;
	}

	if (data->video_ready && comp_time >= data->current_frame_time) {
		if (!data->dup_frame) {
			th_ycbcr_buffer yuv;
			th_decode_ycbcr_out(data->td, yuv);
			data->video_write(yuv);
		}
		data->dup_frame = false;
		data->video_ready = false;
		data->current_frame_time = data->next_frame_time;
	}
}

void VideoStreamPlayback::stop()
{
	data->playing = false;
	seek(0);
}

void VideoStreamPlayback::play()
{
	if (!data->has_video) {
		return;
	}
	if (!data->playing) {
		data->time = 0;
	}
	data->playing = true;
}

bool VideoStreamPlayback::is_playing() const { return data->playing; }

void VideoStreamPlayback::set_paused(bool p_paused) { data->paused = p_paused; }

bool VideoStreamPlayback::is_paused() const { return data->paused; }

double VideoStreamPlayback::get_length() const { return data->stream_length; }

double VideoStreamPlayback::get_playback_position() const { return data->get_time(); }

void VideoStreamPlayback::seek(double p_time)
{
	if (data->file.is_null()) {
		return;
	}
	if (p_time >= data->stream_length) {
		return;
	}

	data->video_ready = false;
	data->next_frame_time = 0;
	data->current_frame_time = -1;
	data->dup_frame = false;
	data->video_done = false;
	data->audio_done = !data->has_audio;
	data->theora_eos = false;
	data->vorbis_eos = false;
	data->audio_ptr_start = 0;
	data->audio_ptr_end = 0;

	ogg_stream_reset(&data->to);
	if (data->has_audio) {
		ogg_stream_reset(&data->vo);
		vorbis_synthesis_restart(&data->vd);
	}

	int64_t seek_pos;
	int64_t video_granulepos;
	int64_t audio_granulepos;
	seek_pos = data->seek_streams(p_time, video_granulepos, audio_granulepos);
	if (seek_pos < 0) {
		return;
	}
	data->file->seek(seek_pos);
	ogg_sync_reset(&data->oy);

	data->time = p_time;

	double last_audio_time = 0;
	double last_video_time = 0;
	bool first_frame_decoded = false;
	bool start_audio = (audio_granulepos == 0);
	bool start_video = (video_granulepos == (1LL << data->ti.keyframe_granule_shift));
	bool keyframe_found = false;
	uint64_t current_frame = 0;

	while ((data->has_audio && last_audio_time < p_time) || (last_video_time <= p_time)) {
		ogg_packet op;
		if (data->feed_pages() == 0) {
			break;
		}
		while (data->has_audio && last_audio_time < p_time && ogg_stream_packetout(&data->vo, &op) > 0) {
			if (start_audio) {
				if (vorbis_synthesis(&data->vb, &op) == 0) {
					vorbis_synthesis_blockin(&data->vd, &data->vb);
					float** pcm;
					int samples_left = ceil((p_time - last_audio_time) * data->vi.rate);
					int samples_read = vorbis_synthesis_pcmout(&data->vd, &pcm);
					int samples_consumed = MIN(samples_left, samples_read);
					vorbis_synthesis_read(&data->vd, samples_consumed);
					last_audio_time += (double)samples_consumed / data->vi.rate;
				}
			}
			else if (op.granulepos >= audio_granulepos) {
				last_audio_time = vorbis_granule_time(&data->vd, op.granulepos);
				if (vorbis_synthesis_trackonly(&data->vb, &op) == 0) {
					vorbis_synthesis_blockin(&data->vd, &data->vb);
				}
				start_audio = true;
			}
		}
		while (last_video_time <= p_time && ogg_stream_packetout(&data->to, &op) > 0) {
			if (!start_video &&
				(op.granulepos >= video_granulepos ||
					video_granulepos == (1LL << data->ti.keyframe_granule_shift))) {
				if (op.granulepos > 0) {
					current_frame = th_granule_frame(data->td, op.granulepos);
				}
				start_video = true;
			}
			if (start_video) {
				if (!keyframe_found && th_packet_iskeyframe(&op)) {
					keyframe_found = true;
					int64_t cur_granulepos = (current_frame + 1) << data->ti.keyframe_granule_shift;
					th_decode_ctl(
						data->td, TH_DECCTL_SET_GRANPOS, &cur_granulepos, sizeof(cur_granulepos));
				}
				if (keyframe_found) {
					int64_t videobuf_granulepos;
					if (op.granulepos >= 0) {
						th_decode_ctl(
							data->td, TH_DECCTL_SET_GRANPOS, &op.granulepos, sizeof(op.granulepos));
					}
					int ret = th_decode_packetin(data->td, &op, &videobuf_granulepos);
					if (ret == 0 || ret == TH_DUPFRAME) {
						last_video_time = th_granule_time(data->td, videobuf_granulepos);
						first_frame_decoded = true;
					}
				}
				else {
					current_frame++;
				}
			}
		}
	}

	if (first_frame_decoded) {
		if (is_playing()) {
			th_ycbcr_buffer yuv;
			th_decode_ycbcr_out(data->td, yuv);
			data->video_write(yuv);
			data->current_frame_time = last_video_time;
		}
		else {
			data->next_frame_time = data->current_frame_time;
			data->video_ready = true;
		}
	}
}

int VideoStreamPlayback::get_channels() const
{
	_channel_count = data->vi.channels;
	return data->vi.channels;
}

void VideoStreamPlayback::set_audio_track(int p_idx) { data->audio_track = p_idx; }

int VideoStreamPlayback::get_mix_rate() const { return data->vi.rate; }

VideoStreamPlayback::VideoStreamPlayback()
{
	data = memnew(TheoraData);
	data->texture.instantiate();
}

VideoStreamPlayback::~VideoStreamPlayback()
{
	data->clear();
	memdelete(data);
	data = nullptr;
}

Ref<VideoStreamPlayback> VideoStream::instantiate_playback()
{
	Ref<VideoStreamPlayback> pb = memnew(VideoStreamPlayback);
	pb->set_audio_track(audio_track);
	pb->set_file(file);
	return pb;
}

Ref<Resource> ResourceFormatLoaderTheora::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	Ref<VideoStream> stream;
	stream.instantiate();
	stream->set_file(p_path);

	if (r_error) {
		*r_error = OK;
	}

	return stream;
}

void ResourceFormatLoaderTheora::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ogv");
}

bool ResourceFormatLoaderTheora::handles_type(const String& p_type) const
{
	return p_type == "VideoStream";
}

String ResourceFormatLoaderTheora::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ogv")) {
		return "VideoStream";
	}
	return "";
}

#ifdef TOOLS_ENABLED
#include <cstdlib>

// For reference, see:
// - https://stackoverflow.com/a/9467305
// - https://en.wikipedia.org/wiki/YCbCr#Approximate_8-bit_matrices_for_BT.601

static void _rgb2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb, size_t width, size_t height, size_t pixel_size) {
	size_t uvpos = 0;
	size_t i = 0;
	for (size_t line = 0; line < height; line += 2) {
		for (size_t x = 0; x < width; x += 2) {
			uint8_t r = rgb[pixel_size * i];
			uint8_t g = rgb[pixel_size * i + 1];
			uint8_t b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

			u[uvpos] = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
			v[uvpos] = ((112 * r + -94 * g + -18 * b) >> 8) + 128;
			uvpos++;

			r = rgb[pixel_size * i];
			g = rgb[pixel_size * i + 1];
			b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		}
		for (size_t x = 0; x < width; x += 1) {
			uint8_t r = rgb[pixel_size * i];
			uint8_t g = rgb[pixel_size * i + 1];
			uint8_t b = rgb[pixel_size * i + 2];

			y[i++] = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
		}
	}
}

static void rgb2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgb, size_t width, size_t height) {
	_rgb2yuv420(y, u, v, rgb, width, height, 3);
}

static void rgba2yuv420(uint8_t *y, uint8_t *u, uint8_t *v, uint8_t *rgba, size_t width, size_t height) {
	_rgb2yuv420(y, u, v, rgba, width, height, 4);
}

#include <cstdlib>

void MovieWriterOGV::push_audio(const int32_t* p_audio_data)
{
	// Read and process more audio.
	float** vorbis_buffer = vorbis_analysis_buffer(&vd, audio_frames);

	// Deinterleave samples.
	uint32_t count = 0;
	for (uint32_t i = 0; i < audio_frames; i++) {
		for (uint32_t j = 0; j < audio_ch; j++) {
			vorbis_buffer[j][i] = p_audio_data[count] / 2147483647.f;
			count++;
		}
	}

	vorbis_analysis_wrote(&vd, audio_frames);
}

void MovieWriterOGV::pull_audio(bool p_last)
{
	ogg_packet op;

	while (vorbis_analysis_blockout(&vd, &vb) > 0) {
		// Analysis, assume we want to use bitrate management.
		vorbis_analysis(&vb, nullptr);
		vorbis_bitrate_addblock(&vb);

		// Weld packets into the bitstream.
		while (vorbis_bitrate_flushpacket(&vd, &op) > 0) {
			ogg_stream_packetin(&vo, &op);
		}
	}

	if (p_last) {
		vorbis_analysis_wrote(&vd, 0);
		pull_audio();
	}
}

void MovieWriterOGV::push_video(const Ref<Image>& p_image)
{
	PackedByteArray data = p_image->get_data();
	if (p_image->get_format() == Image::FORMAT_RGBA8) {
		rgba2yuv420(y, u, v, data.ptrw(), p_image->get_width(), p_image->get_height());
	}
	else {
		rgb2yuv420(y, u, v, data.ptrw(), p_image->get_width(), p_image->get_height());
	}
	th_encode_ycbcr_in(td, ycbcr);
}

void MovieWriterOGV::pull_video(bool p_last)
{
	ogg_packet op;

	int ret = 0;
	do {
		ret = th_encode_packetout(td, p_last, &op);
		if (ret > 0) {
			ogg_stream_packetin(&to, &op);
		}
	} while (ret > 0);
}

uint32_t MovieWriterOGV::get_audio_mix_rate() const { return mix_rate; }

AudioServer::SpeakerMode MovieWriterOGV::get_audio_speaker_mode() const { return speaker_mode; }

bool MovieWriterOGV::handles_file(const String& p_path) const
{
	return p_path.has_extension("ogv");
}

void MovieWriterOGV::get_supported_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("ogv");
}

Error MovieWriterOGV::write_begin(
	const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path)
{
	ERR_FAIL_COND_V_MSG((p_movie_size.width & 1) || (p_movie_size.height & 1), ERR_UNAVAILABLE,
		"Both video dimensions must be even.");
	base_path = p_base_path.get_basename();
	if (base_path.is_relative_path()) {
		base_path = "res://" + base_path;
	}
	base_path += ".ogv";

	f = FileAccess::open(base_path, FileAccess::WRITE_READ);
	ERR_FAIL_COND_V(f.is_null(), ERR_CANT_OPEN);

	fps = p_fps;

	audio_ch = 2;
	switch (speaker_mode) {
	case AudioServer::SPEAKER_MODE_STEREO:
		audio_ch = 2;
		break;
	case AudioServer::SPEAKER_SURROUND_31:
		audio_ch = 4;
		break;
	case AudioServer::SPEAKER_SURROUND_51:
		audio_ch = 6;
		break;
	case AudioServer::SPEAKER_SURROUND_71:
		audio_ch = 8;
		break;
	}
	audio_frames = mix_rate / fps;

	// Set up Ogg output streams.
	srand(time(nullptr));
	ogg_stream_init(&to, rand()); // Video.
	ogg_stream_init(&vo, rand()); // Audio.

	// Initialize Vorbis audio encoding.
	vorbis_info_init(&vi);
	int ret = vorbis_encode_init_vbr(&vi, audio_ch, mix_rate, audio_quality);
	ERR_FAIL_COND_V_MSG(ret, ERR_UNAVAILABLE,
		"The Ogg Vorbis encoder couldn't set up a mode according to the requested quality or "
		"bitrate.");

	vorbis_comment_init(&vc);
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	// Set up Theora encoder.
	// Theora has a divisible-by-16 restriction for the encoded frame size
	// scale the picture size up to the nearest /16 and calculate offsets.
	int pic_w = p_movie_size.width;
	int pic_h = p_movie_size.height;
	int frame_w = (pic_w + 15) & ~0xF;
	int frame_h = (pic_h + 15) & ~0xF;
	// Force the offsets to be even so that chroma samples line up like we expect.
	int pic_x = (frame_w - pic_w) / 2 & ~1;
	int pic_y = (frame_h - pic_h) / 2 & ~1;

	y = (uint8_t*)memalloc(pic_w * pic_h);
	u = (uint8_t*)memalloc(pic_w * pic_h / 4);
	v = (uint8_t*)memalloc(pic_w * pic_h / 4);

	// We submit the buffer using the size of the picture region.
	// libtheora will pad the picture region out to the full frame size for us,
	// whether we pass in a full frame or not.
	ycbcr[0].width = pic_w;
	ycbcr[0].height = pic_h;
	ycbcr[0].stride = pic_w;
	ycbcr[0].data = y;
	ycbcr[1].width = pic_w / 2;
	ycbcr[1].height = pic_h / 2;
	ycbcr[1].stride = pic_w / 2;
	ycbcr[1].data = u;
	ycbcr[2].width = pic_w / 2;
	ycbcr[2].height = pic_h / 2;
	ycbcr[2].stride = pic_w / 2;
	ycbcr[2].data = v;

	th_info_init(&ti);
	ti.frame_width = frame_w;
	ti.frame_height = frame_h;
	ti.pic_width = pic_w;
	ti.pic_height = pic_h;
	ti.pic_x = pic_x;
	ti.pic_y = pic_y;
	ti.fps_numerator = fps;
	ti.fps_denominator = 1;
	ti.aspect_numerator = 1;
	ti.aspect_denominator = 1;
	ti.colorspace = TH_CS_UNSPECIFIED;
	// Account for the Ogg page overhead.
	// This is 1 byte per 255 for lacing values, plus 26 bytes per 4096 bytes for
	// the page header, plus approximately 1/2 byte per packet (not accounted for here).
	ti.target_bitrate = (int)(64870 * (ogg_int64_t)video_bitrate >> 16);
	ti.quality = video_quality * 63;
	ti.pixel_fmt = TH_PF_420;
	td = th_encode_alloc(&ti);
	th_info_clear(&ti);
	ERR_FAIL_NULL_V_MSG(td, ERR_UNCONFIGURED,
		"Couldn't create a Theora encoder instance. Check that the video parameters are valid.");

	// Setting just the granule shift only allows power-of-two keyframe spacing.
	// Set the actual requested spacing.
	ret = th_encode_ctl(td, TH_ENCCTL_SET_KEYFRAME_FREQUENCY_FORCE, &keyframe_frequency,
		sizeof(keyframe_frequency));
	if (ret < 0) {
		ERR_PRINT("Couldn't set keyframe interval.");
	}

	// Speed should also be set after the current encoder mode is established,
	// since the available speed levels may change depending on the encoder mode.
	if (speed >= 0) {
		int speed_max;
		ret = th_encode_ctl(td, TH_ENCCTL_GET_SPLEVEL_MAX, &speed_max, sizeof(speed_max));
		if (ret < 0) {
			WARN_PRINT("Couldn't determine maximum speed level.");
			speed_max = 0;
		}
		ret = th_encode_ctl(td, TH_ENCCTL_SET_SPLEVEL, &speed, sizeof(speed));
		if (ret < 0) {
			if (ret < 0) {
				WARN_PRINT(vformat("Couldn't set speed level to %d of %d.", speed, speed_max));
			}
			if (speed > speed_max) {
				WARN_PRINT(vformat("Setting speed level to %d instead.", speed_max));
			}
			ret = th_encode_ctl(td, TH_ENCCTL_SET_SPLEVEL, &speed_max, sizeof(speed_max));
			if (ret < 0) {
				WARN_PRINT(vformat("Couldn't set speed level to %d of %d.", speed_max, speed_max));
			}
		}
	}

	// Write the bitstream header packets with proper page interleave.
	th_comment_init(&tc);
	// The first packet will get its own page automatically.
	ogg_packet op;
	if (th_encode_flushheader(td, &tc, &op) <= 0) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Theora library error.");
	}

	ogg_stream_packetin(&to, &op);
	if (ogg_stream_pageout(&to, &video_page) != 1) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
	}
	f->store_buffer(video_page.header, video_page.header_len);
	f->store_buffer(video_page.body, video_page.body_len);

	// Create the remaining Theora headers.
	while (true) {
		ret = th_encode_flushheader(td, &tc, &op);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Theora library error.");
		}
		else if (ret == 0) {
			break;
		}
		ogg_stream_packetin(&to, &op);
	}

	// Vorbis streams start with 3 standard header packets.
	ogg_packet id;
	ogg_packet comment;
	ogg_packet code;
	if (vorbis_analysis_headerout(&vd, &vc, &id, &comment, &code) < 0) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Vorbis library error.");
	}

	// ID header is automatically placed in its own page.
	ogg_stream_packetin(&vo, &id);
	if (ogg_stream_pageout(&vo, &audio_page) != 1) {
		ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
	}
	f->store_buffer(audio_page.header, audio_page.header_len);
	f->store_buffer(audio_page.body, audio_page.body_len);

	// Append remaining Vorbis header packets.
	ogg_stream_packetin(&vo, &comment);
	ogg_stream_packetin(&vo, &code);

	// Flush the rest of our headers. This ensures the actual data in each stream will start on a
	// new page, as per spec.
	while (true) {
		ret = ogg_stream_flush(&to, &video_page);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
		}
		else if (ret == 0) {
			break;
		}
		f->store_buffer(video_page.header, video_page.header_len);
		f->store_buffer(video_page.body, video_page.body_len);
	}

	while (true) {
		ret = ogg_stream_flush(&vo, &audio_page);
		if (ret < 0) {
			ERR_FAIL_V_MSG(ERR_UNCONFIGURED, "Internal Ogg library error.");
		}
		else if (ret == 0) {
			break;
		}
		f->store_buffer(audio_page.header, audio_page.header_len);
		f->store_buffer(audio_page.body, audio_page.body_len);
	}

	return OK;
}

// The order of the operations has been chosen so we're one frame behind writing to the stream so we
// can put the eos mark in the last frame. Flushing streams to the file every X frames is done to
// improve audio/video page interleaving thus avoiding large runs of video or audio pages.
Error MovieWriterOGV::write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data)
{
	ERR_FAIL_COND_V(f.is_null() || td == nullptr, ERR_UNCONFIGURED);

	frame_count++;

	pull_audio();
	pull_video();

	if ((frame_count % 8) == 0) {
		write_to_file();
	}

	push_audio(p_audio_data);
	push_video(p_image);

	return OK;
}

void MovieWriterOGV::save_page(ogg_page page)
{
	unsigned int page_size = page.header_len + page.body_len;
	if (page_size > backup_page_size) {
		backup_page_data = (unsigned char*)memrealloc(backup_page_data, page_size);
		backup_page_size = page_size;
	}
	backup_page.header = backup_page_data;
	backup_page.header_len = page.header_len;
	backup_page.body = backup_page_data + page.header_len;
	backup_page.body_len = page.body_len;
	memcpy(backup_page.header, page.header, page.header_len);
	memcpy(backup_page.body, page.body, page.body_len);
}

void MovieWriterOGV::restore_page(ogg_page* page)
{
	page->header = backup_page.header;
	page->header_len = backup_page.header_len;
	page->body = backup_page.body;
	page->body_len = backup_page.body_len;
}

// The added complexity here is because we have to ensure pages are written in ascending timestamp
// order. libOgg doesn't allow checking the next page granulepos without requesting the page, and
// once requested it can't be returned, thus, we need to save it so that it doesn't get erased by
// the next `ogg_stream_packetin` call.
void MovieWriterOGV::write_to_file(bool p_finish)
{
	if (audio_flag) {
		restore_page(&audio_page);
	}
	else {
		audio_flag = ogg_stream_flush(&vo, &audio_page);
	}
	if (video_flag) {
		restore_page(&video_page);
	}
	else {
		video_flag = ogg_stream_flush(&to, &video_page);
	}

	bool finishing = p_finish && (audio_flag || video_flag);
	while (finishing || (audio_flag && video_flag)) {
		double audiotime = vorbis_granule_time(&vd, ogg_page_granulepos(&audio_page));
		double videotime = th_granule_time(td, ogg_page_granulepos(&video_page));
		bool video_first = audiotime >= videotime;

		if (video_flag && video_first) {
			// Flush a video page.
			f->store_buffer(video_page.header, video_page.header_len);
			f->store_buffer(video_page.body, video_page.body_len);
			video_flag = ogg_stream_flush(&to, &video_page) > 0;
		}
		else {
			// Flush an audio page.
			f->store_buffer(audio_page.header, audio_page.header_len);
			f->store_buffer(audio_page.body, audio_page.body_len);
			audio_flag = ogg_stream_flush(&vo, &audio_page) > 0;
		}
		finishing = p_finish && (audio_flag || video_flag);
	}

	if (video_flag) {
		save_page(video_page);
	}
	else if (audio_flag) {
		save_page(audio_page);
	}
}

void MovieWriterOGV::write_end()
{
	pull_audio(true);
	pull_video(true);
	write_to_file(true);

	th_encode_free(td);

	ogg_stream_clear(&vo);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);

	ogg_stream_clear(&to);
	th_comment_clear(&tc);

	memfree(y);
	memfree(u);
	memfree(v);

	if (backup_page_data != nullptr) {
		memfree(backup_page_data);
	}

	if (f.is_valid()) {
		f.unref();
	}
}



MovieWriterOGV::MovieWriterOGV() {}
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
#endif

static Ref<ResourceFormatLoaderTheora> resource_loader_theora;
#ifdef TOOLS_ENABLED
static MovieWriterOGV* writer_ogv = nullptr;
#endif

void initialize_theora_module(ModuleInitializationLevel p_level)
{
	switch (p_level) {
	case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
		writer_ogv = memnew(MovieWriterOGV);
		MovieWriter::add_writer(writer_ogv);
#endif
	} break;

	case MODULE_INITIALIZATION_LEVEL_SCENE: {
		resource_loader_theora.instantiate();
		ResourceLoader::add_resource_format_loader(resource_loader_theora, true);
	} break;
	default:
		break;
	}
}

void uninitialize_theora_module(ModuleInitializationLevel p_level)
{
	switch (p_level) {
	case MODULE_INITIALIZATION_LEVEL_SCENE: {
		ResourceLoader::remove_resource_format_loader(resource_loader_theora);
		resource_loader_theora.unref();
	} break;

	case MODULE_INITIALIZATION_LEVEL_SERVERS: {
#ifdef TOOLS_ENABLED
		memdelete(writer_ogv);
#endif
	} break;
	default:
		break;
	}
}
