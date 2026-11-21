/**************************************************************************/
/*  register_server_types.cpp                                             */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "register_server_types.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/camera/camera_feed.h"
#include "servers/camera/camera_server.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/display/native_menu.h"
#include "servers/movie_writer/movie_writer.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_include_db.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/storage/render_scene_buffers.h"
#include "servers/text/text_server.h"
#include "servers/text/text_server_dummy.h"

// 2D physics and navigation.
#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED
#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_2D_DISABLED

// 3D physics and navigation.
#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED
#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

// XR
#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#include "servers/xr/xr_tracker.h"
#endif // XR_DISABLED

static bool has_server_feature_callback(const String& p_feature)
{
	if (Renderer::data) {
		if (Renderer::has_os_feature(p_feature)) {
			return true;
		}
	}

	return false;
}

static MovieWriterPNGWAV* writer_pngwav = nullptr;

void unregister_server_types()
{
	ShaderTypes::finalize();
}

void register_server_types()
{
	ShaderTypes::initialize();
}



// ============================================================================
// Merged from servers/camera/camera_server.cpp
// ============================================================================

#include "servers/camera/camera_server.h"
#include "servers/camera/camera_feed.h"

////////////////////////////////////////////////////////
// CameraServer

void CameraServer::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
	_initialize_backend();
}

void CameraServer::finalize()
{
	if (!data) {
		return;
	}
	_finalize_backend();
	memdelete(data);
	data = nullptr;
}

void CameraServer::set_monitoring_feeds_state(bool p_monitoring_feeds)
{
	ERR_FAIL_NULL(data);
	data->monitoring_feeds = p_monitoring_feeds;
}

void CameraServer::_default_set_monitoring_feeds(bool p_monitoring_feeds)
{
	set_monitoring_feeds_state(p_monitoring_feeds);
}

int CameraServer::get_free_id()
{
	ERR_FAIL_NULL_V(data, 0);
	MutexLock lock(_thread_safe_);
	bool id_exists = true;
	int newid = 0;

	// find a free id
	while (id_exists) {
		newid++;
		id_exists = false;
		for (int i = 0; i < data->feeds.size() && !id_exists; i++) {
			if (data->feeds[i]->get_id() == newid) {
				id_exists = true;
			}
		}
	}

	return newid;
}

int CameraServer::get_feed_index(int p_id)
{
	ERR_FAIL_NULL_V(data, -1);
	ERR_FAIL_COND_V_MSG(!data->monitoring_feeds, -1,
		"CameraServer is not actively monitoring feeds; call set_monitoring_feeds(true) first.");

	MutexLock lock(_thread_safe_);
	for (int i = 0; i < data->feeds.size(); i++) {
		if (data->feeds[i]->get_id() == p_id) {
			return i;
		}
	}

	return -1;
}

Ref<CameraFeed> CameraServer::get_feed_by_id(int p_id)
{
	ERR_FAIL_NULL_V(data, nullptr);
	ERR_FAIL_COND_V_MSG(!data->monitoring_feeds, nullptr,
		"CameraServer is not actively monitoring feeds; call set_monitoring_feeds(true) first.");

	MutexLock lock(_thread_safe_);
	int index = get_feed_index(p_id);

	if (index == -1) {
		return nullptr;
	}
	else {
		return data->feeds[index];
	}
}

void CameraServer::add_feed(const Ref<CameraFeed>& p_feed)
{
	ERR_FAIL_COND(p_feed.is_null());
	ERR_FAIL_NULL(data);

	MutexLock lock(_thread_safe_);
	data->feeds.push_back(p_feed);
}

void CameraServer::remove_feed(const Ref<CameraFeed>& p_feed)
{
	ERR_FAIL_COND(p_feed.is_null());
	ERR_FAIL_NULL(data);

	MutexLock lock(_thread_safe_);
	for (int i = 0; i < data->feeds.size(); i++) {
		if (data->feeds[i] == p_feed) {
			data->feeds.remove_at(i);
			break;
		}
	}
}

Ref<CameraFeed> CameraServer::get_feed(int p_index)
{
	ERR_FAIL_NULL_V(data, nullptr);
	ERR_FAIL_COND_V_MSG(!data->monitoring_feeds, nullptr,
		"CameraServer is not actively monitoring feeds; call set_monitoring_feeds(true) first.");

	MutexLock lock(_thread_safe_);
	ERR_FAIL_INDEX_V(p_index, data->feeds.size(), nullptr);

	return data->feeds[p_index];
}

int CameraServer::get_feed_count()
{
	ERR_FAIL_NULL_V(data, 0);
	ERR_FAIL_COND_V_MSG(!data->monitoring_feeds, 0,
		"CameraServer is not actively monitoring feeds; call set_monitoring_feeds(true) first.");

	MutexLock lock(_thread_safe_);
	return data->feeds.size();
}

RID CameraServer::feed_texture(int p_id, CameraServer::FeedImage p_texture)
{
	ERR_FAIL_NULL_V(data, RID());
	ERR_FAIL_COND_V_MSG(!data->monitoring_feeds, RID(),
		"CameraServer is not actively monitoring feeds; call set_monitoring_feeds(true) first.");

	MutexLock lock(_thread_safe_);
	int index = get_feed_index(p_id);
	ERR_FAIL_COND_V(index == -1, RID());

	Ref<CameraFeed> feed = get_feed(index);

	return feed->get_texture(p_texture);
}



// ============================================================================
// Merged from servers/camera/camera_feed.cpp
// ============================================================================

#include "servers/camera/camera_feed.h"
#include "servers/rendering/renderer.h"

int CameraFeed::get_id() const { return id; }

bool CameraFeed::is_active() const { return active; }

void CameraFeed::set_active(bool p_is_active)
{
	if (p_is_active == active) {
		// all good
	}
	else if (p_is_active) {
		// attempt to activate this feed
		if (activate_feed()) {
			active = true;
		}
	}
	else {
		// just deactivate it
		deactivate_feed();
		active = false;
	}
}

String CameraFeed::get_name() const { return name; }

void CameraFeed::set_name(String p_name) { name = p_name; }

int CameraFeed::get_base_width() const { return base_width; }

int CameraFeed::get_base_height() const { return base_height; }

CameraFeed::FeedDataType CameraFeed::get_datatype() const { return datatype; }

CameraFeed::FeedPosition CameraFeed::get_position() const { return position; }

void CameraFeed::set_position(CameraFeed::FeedPosition p_position) { position = p_position; }

Transform2D CameraFeed::get_transform() const { return transform; }

void CameraFeed::set_transform(const Transform2D& p_transform) { transform = p_transform; }

RID CameraFeed::get_texture(CameraServer::FeedImage p_which) { return texture[p_which]; }

uint64_t CameraFeed::get_texture_tex_id(CameraServer::FeedImage p_which)
{
	return Renderer::texture_get_native_handle(texture[p_which]);
}

CameraFeed::CameraFeed()
{
	// initialize our feed
	id = CameraServer::get_free_id();
	base_width = 0;
	base_height = 0;
	name = "???";
	active = false;
	datatype = CameraFeed::FEED_RGB;
	position = CameraFeed::FEED_UNSPECIFIED;
	transform = Transform2D(1.0, 0.0, 0.0, -1.0, 0.0, 1.0);
	texture[CameraServer::FEED_Y_IMAGE] =
		Renderer::texture_2d_placeholder_create();
	texture[CameraServer::FEED_CBCR_IMAGE] =
		Renderer::texture_2d_placeholder_create();
}

CameraFeed::CameraFeed(String p_name, FeedPosition p_position)
{
	// initialize our feed
	id = CameraServer::get_free_id();
	base_width = 0;
	base_height = 0;
	name = p_name;
	active = false;
	datatype = CameraFeed::FEED_NOIMAGE;
	position = p_position;
	transform = Transform2D(1.0, 0.0, 0.0, -1.0, 0.0, 1.0);
	texture[CameraServer::FEED_Y_IMAGE] =
		Renderer::texture_2d_placeholder_create();
	texture[CameraServer::FEED_CBCR_IMAGE] =
		Renderer::texture_2d_placeholder_create();
}

CameraFeed::~CameraFeed()
{
	// Free our textures
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(texture[CameraServer::FEED_Y_IMAGE]);
	Renderer::free_rid(texture[CameraServer::FEED_CBCR_IMAGE]);
}

CameraFeed::FeedFormat CameraFeed::get_format() const
{
	FeedFormat feed_format = {};
	return feed_format;
}

bool CameraFeed::activate_feed() { return false; }

void CameraFeed::deactivate_feed() {}

void CameraFeed::set_rgb_image(Ref<Image> const&) {}

void CameraFeed::set_ycbcr_images(Ref<Image> const&, Ref<Image> const&) {}

// ============================================================================
// Merged from servers/movie_writer/movie_writer.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/os/time.h"
#include "core/templates/rb_set.h"
#include "servers/movie_writer/movie_writer.h"
#include "scene/main/window.h"
#include "servers/audio/audio_server.h"
#include "servers/display/display_server_enums.h"
#include "servers/rendering/renderer.h"

MovieWriter* MovieWriter::writers[MovieWriter::MAX_WRITERS];
uint32_t MovieWriter::writer_count = 0;

void MovieWriter::add_writer(MovieWriter* p_writer)
{
	ERR_FAIL_COND(writer_count == MAX_WRITERS);
	writers[writer_count++] = p_writer;
}

MovieWriter* MovieWriter::find_writer_for_file(const String& p_file)
{
	for (int32_t i = writer_count - 1; i >= 0; i--) { // More recent last, to have override ability.
		if (writers[i]->handles_file(p_file)) {
			return writers[i];
		}
	}
	return nullptr;
}

uint32_t MovieWriter::get_audio_mix_rate() const
{
	uint32_t ret = 48000;
	return ret;
}

AudioServer::SpeakerMode MovieWriter::get_audio_speaker_mode() const
{
	AudioServer::SpeakerMode ret = AudioServer::SPEAKER_MODE_STEREO;
	return ret;
}

Error MovieWriter::write_begin(
	const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path)
{
	Error ret = ERR_UNCONFIGURED;
	return ret;
}

Error MovieWriter::write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data)
{
	Error ret = ERR_UNCONFIGURED;
	return ret;
}

void MovieWriter::get_supported_extensions(List<String>* r_extensions) const
{
	Vector<String> exts;
	for (int i = 0; i < exts.size(); i++) {
		r_extensions->push_back(exts[i]);
	}
}

void MovieWriter::add_frame()
{
	const int movie_time_seconds = Engine::get_frames_drawn() / fps;
	const int frame_remainder = Engine::get_frames_drawn() % fps;
	const String movie_time =
		vformat("%s:%s:%s:%s", String::num(movie_time_seconds / 3600, 0).pad_zeros(2),
			String::num((movie_time_seconds % 3600) / 60, 0).pad_zeros(2),
			String::num(movie_time_seconds % 60, 0).pad_zeros(2),
			String::num(frame_remainder, 0).pad_zeros(2));

	Window* main_window = Window::get_from_id(DisplayServerEnums::MAIN_WINDOW_ID);
	if (main_window) {
		main_window->set_title(vformat("MovieWriter: Frame %d (time: %s) - %s",
			Engine::get_frames_drawn(), movie_time, project_name));
	}

	RID main_vp_rid = Renderer::viewport_find_from_screen_attachment(
		DisplayServerEnums::MAIN_WINDOW_ID);
	RID main_vp_texture = Renderer::viewport_get_texture(main_vp_rid);
	Ref<Image> vp_tex = Renderer::texture_2d_get(main_vp_texture);

	if (vp_tex->get_size() != movie_size) {
		// Resize the texture to the output resolution if it differs from the current viewport size.
		// This ensures all frames have the same resolution, as not all video formats and players
		// support resolution changes during playback.

		const float src_aspect = vp_tex->get_size().aspect();
		const float dst_aspect = movie_size.aspect();

		int crop_width = vp_tex->get_size().width;
		int crop_height = vp_tex->get_size().height;
		int crop_x = 0;
		int crop_y = 0;

		// If the aspect ratio differs, crop the image to cover the base resolution's aspect ratio
		// in a way similar to `TextureRect.STRETCH_KEEP_ASPECT_COVERED`.
		if (src_aspect > dst_aspect) {
			// Source is wider, crop horizontally.
			crop_width = int(vp_tex->get_size().height * dst_aspect);
			crop_x = (vp_tex->get_size().width - crop_width) / 2;
			vp_tex->crop_from_point(crop_x, crop_y, crop_width, crop_height);
		}
		else if (src_aspect < dst_aspect) {
			// Source is taller, crop vertically.
			crop_height = int(vp_tex->get_size().width / dst_aspect);
			crop_y = (vp_tex->get_size().height - crop_height) / 2;
			vp_tex->crop_from_point(crop_x, crop_y, crop_width, crop_height);
		}

		vp_tex->resize(movie_size.width, movie_size.height, Image::INTERPOLATE_BILINEAR);
	}

	if (Renderer::viewport_is_using_hdr_2d(main_vp_rid)) {
		vp_tex->convert(Image::FORMAT_RGBA8);
		vp_tex->linear_to_srgb();
	}

	Renderer::viewport_set_measure_render_time(main_vp_rid, true);
	cpu_time +=
		Renderer::viewport_get_measured_render_time_cpu(main_vp_rid);
	cpu_time += Renderer::get_frame_setup_time_cpu();
	gpu_time +=
		Renderer::viewport_get_measured_render_time_gpu(main_vp_rid);

	AudioDriverManager::get_dummy_driver()->mix_audio(mix_rate / fps, audio_mix_buffer.ptr());

	uint64_t encoding_start_usec = Time::get_ticks_usec();
	write_frame(vp_tex, audio_mix_buffer.ptr());
	uint64_t encoding_end_usec = Time::get_ticks_usec();
	encoding_time_usec += encoding_end_usec - encoding_start_usec;
}

void MovieWriter::write_end() {}

bool MovieWriter::handles_file(const String& p_path) const { return true; }

void MovieWriter::end() {}



// ============================================================================
// Merged from servers/movie_writer/movie_writer_pngwav.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "servers/movie_writer/movie_writer.h"

uint32_t MovieWriterPNGWAV::get_audio_mix_rate() const { return mix_rate; }

AudioServer::SpeakerMode MovieWriterPNGWAV::get_audio_speaker_mode() const { return speaker_mode; }

void MovieWriterPNGWAV::get_supported_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("png");
}

bool MovieWriterPNGWAV::handles_file(const String& p_path) const
{
	return p_path.has_extension("png");
}

String MovieWriterPNGWAV::zeros_str(uint32_t p_index)
{
	char zeros[MAX_TRAILING_ZEROS + 1];
	for (uint32_t i = 0; i < MAX_TRAILING_ZEROS; i++) {
		uint32_t idx = MAX_TRAILING_ZEROS - i - 1;
		uint32_t digit = (p_index / uint32_t(Math::pow(double(10), double(idx)))) % 10;
		zeros[i] = '0' + digit;
	}
	zeros[MAX_TRAILING_ZEROS] = 0;
	return zeros;
}

Error MovieWriterPNGWAV::write_begin(
	const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path)
{
	// Quick & Dirty PNGWAV Code based on -
	// https://docs.microsoft.com/en-us/windows/win32/directshow/avi-riff-file-reference

	base_path = p_base_path.get_basename();
	if (base_path.is_relative_path()) {
		base_path = "res://" + base_path;
	}

	{
		// Remove existing files before writing anew
		uint32_t idx = 0;
		Ref<DirAccess> d = DirAccess::open(base_path.get_base_dir());
		ERR_FAIL_COND_V(d.is_null(), FAILED);

		String file = base_path.get_file();
		while (true) {
			String path = file + zeros_str(idx) + ".png";
			if (d->remove(path) != OK) {
				break;
			}
		}
	}

	f_wav = FileAccess::open(base_path + ".wav", FileAccess::WRITE_READ);
	ERR_FAIL_COND_V(f_wav.is_null(), ERR_CANT_OPEN);

	fps = p_fps;

	f_wav->store_buffer((const uint8_t*)"RIFF", 4);
	int total_size = 4 /* WAVE */ + 8 /* fmt+size */ + 16 /* format */ + 8 /* data+size */;
	f_wav->store_32(total_size); // will store final later
	f_wav->store_buffer((const uint8_t*)"WAVE", 4);

	/* FORMAT CHUNK */

	f_wav->store_buffer((const uint8_t*)"fmt ", 4);

	uint32_t channels = 2;
	switch (speaker_mode) {
	case AudioServer::SPEAKER_MODE_STEREO:
		channels = 2;
		break;
	case AudioServer::SPEAKER_SURROUND_31:
		channels = 4;
		break;
	case AudioServer::SPEAKER_SURROUND_51:
		channels = 6;
		break;
	case AudioServer::SPEAKER_SURROUND_71:
		channels = 8;
		break;
	}

	f_wav->store_32(16);	   // standard format, no extra fields
	f_wav->store_16(1);		   // compression code, standard PCM
	f_wav->store_16(channels); // CHANNELS: 2

	f_wav->store_32(mix_rate);

	/* useless stuff the format asks for */

	int bits_per_sample = 32;
	int blockalign = bits_per_sample / 8 * channels;
	int bytes_per_sec = mix_rate * blockalign;

	audio_block_size = (mix_rate / fps) * blockalign;

	f_wav->store_32(bytes_per_sec);
	f_wav->store_16(blockalign); // block align (unused)
	f_wav->store_16(bits_per_sample);

	/* DATA CHUNK */

	f_wav->store_buffer((const uint8_t*)"data", 4);

	f_wav->store_32(0); // data size... wooh
	wav_data_size_pos = f_wav->get_position();

	return OK;
}

Error MovieWriterPNGWAV::write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data)
{
	ERR_FAIL_COND_V(f_wav.is_null(), ERR_UNCONFIGURED);

	Vector<uint8_t> png_buffer = p_image->save_png_to_buffer();

	Ref<FileAccess> fi =
		FileAccess::open(base_path + zeros_str(frame_count) + ".png", FileAccess::WRITE);
	fi->store_buffer(png_buffer.ptr(), png_buffer.size());
	f_wav->store_buffer((const uint8_t*)p_audio_data, audio_block_size);

	frame_count++;

	return OK;
}

void MovieWriterPNGWAV::write_end()
{
	if (f_wav.is_valid()) {
		uint32_t total_size = 4 /* WAVE */ + 8 /* fmt+size */ + 16 /* format */ + 8 /* data+size */;
		uint32_t datasize = f_wav->get_position() - wav_data_size_pos;
		f_wav->seek(4);
		f_wav->store_32(total_size + datasize);
		f_wav->seek(0x28);
		f_wav->store_32(datasize);
	}
}


