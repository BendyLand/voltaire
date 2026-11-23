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
#include "main/main.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"
#include "servers/camera/camera_server.h"
#include "servers/camera/camera_server.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/display/native_menu.h"
#include "main/main.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/text/text_server.h"

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
#include "servers/xr/xr_server.h"
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
#include "servers/camera/camera_server.h"

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

#include "servers/camera/camera_server.h"
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
#include "main/main.h"
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
#include "main/main.h"

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



// ============================================================================
// Merged from servers/audio/audio_server.cpp
// ============================================================================

#include "servers/audio/audio_server.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "servers/audio/audio_server.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/pair.h"
#include "scene/main/node.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"

#ifdef TOOLS_ENABLED
#define MARK_EDITED this->obj->set_edited(true);
#else
#define MARK_EDITED
#endif

#ifdef DEBUG_ENABLED
void AudioDriver::start_counting_ticks() { prof_ticks.set(OS::get_ticks_usec()); }

void AudioDriver::stop_counting_ticks()
{
	prof_time.add(OS::get_ticks_usec() - prof_ticks.get());
}
#endif // DEBUG_ENABLED

void AudioDriver::audio_server_process(int p_frames, int32_t* p_buffer, bool p_update_mix_time)
{
	if (p_update_mix_time) {
		update_mix_time(p_frames);
	}

	if (AudioServer::is_initialized()) {
		AudioServer::_driver_process(p_frames, p_buffer);
	}
}

void AudioDriver::update_mix_time(int p_frames)
{
	_last_mix_frames = p_frames;
	if (OS::is_initialized()) {
		_last_mix_time = OS::get_ticks_usec();
	}
}

double AudioDriver::get_time_since_last_mix()
{
	lock();
	uint64_t last_mix_time = _last_mix_time;
	unlock();
	return (OS::get_ticks_usec() - last_mix_time) / 1000000.0;
}

double AudioDriver::get_time_to_next_mix()
{
	lock();
	uint64_t last_mix_time = _last_mix_time;
	uint64_t last_mix_frames = _last_mix_frames;
	unlock();
	double total = (OS::get_ticks_usec() - last_mix_time) / 1000000.0;
	double mix_buffer = last_mix_frames / (double)get_mix_rate();
	return mix_buffer - total;
}

void AudioDriver::input_buffer_init(int driver_buffer_frames)
{
	const int input_buffer_channels = 2;
	input_buffer.resize(driver_buffer_frames * input_buffer_channels * 4);
	input_position = 0;
	input_size = 0;
}

void AudioDriver::input_buffer_write(int32_t sample)
{
	if ((int)input_position < input_buffer.size()) {
		input_buffer.write[input_position++] = sample;
		if ((int)input_position >= input_buffer.size()) {
			input_position = 0;
		}
		if ((int)input_size < input_buffer.size()) {
			input_size++;
		}
	}
	else {
		// This protection was added in GH-26505 due to a "possible crash".
		// This cannot have happened unless two non-locked threads entered function simultaneously,
		// which was possible when multiple calls to `AudioDriver::input_start()` did not raise an
		// error condition.
		WARN_PRINT("input_buffer_write: Invalid input_position=" + itos(input_position) +
				   " input_buffer.size()=" + itos(input_buffer.size()));
	}
}

AudioDriver::SpeakerMode AudioDriver::get_speaker_mode_by_total_channels(int p_channels) const
{
	switch (p_channels) {
	case 4:
		return SPEAKER_SURROUND_31;
	case 6:
		return SPEAKER_SURROUND_51;
	case 8:
		return SPEAKER_SURROUND_71;
	}

	// Default to STEREO
	return SPEAKER_MODE_STEREO;
}

int AudioDriver::get_total_channels_by_speaker_mode(AudioDriver::SpeakerMode p_mode) const
{
	switch (p_mode) {
	case SPEAKER_MODE_STEREO:
		return 2;
	case SPEAKER_SURROUND_31:
		return 4;
	case SPEAKER_SURROUND_51:
		return 6;
	case SPEAKER_SURROUND_71:
		return 8;
	}

	ERR_FAIL_V(2);
}

PackedStringArray AudioDriver::get_output_device_list()
{
	PackedStringArray list;

	list.push_back("Default");

	return list;
}

String AudioDriver::get_output_device() { return "Default"; }

PackedStringArray AudioDriver::get_input_device_list()
{
	PackedStringArray list;

	list.push_back("Default");

	return list;
}

AudioDriverDummy AudioDriverManager::dummy_driver;
AudioDriver* AudioDriverManager::drivers[MAX_DRIVERS] = {
	&AudioDriverManager::dummy_driver,
};
int AudioDriverManager::driver_count = 1;
AudioDriver* AudioDriverManager::active_driver = &AudioDriverManager::dummy_driver;

void AudioDriverManager::add_driver(AudioDriver* p_driver)
{
	ERR_FAIL_COND(driver_count >= MAX_DRIVERS);
	drivers[driver_count - 1] = p_driver;

	// Last driver is always our dummy driver
	drivers[driver_count++] = &AudioDriverManager::dummy_driver;
}

int AudioDriverManager::get_driver_count() { return driver_count; }

AudioDriver* AudioDriverManager::get_driver(int p_driver)
{
	ERR_FAIL_INDEX_V(p_driver, driver_count, nullptr);
	return drivers[p_driver];
}

void AudioDriverManager::set_active_driver(AudioDriver* p_driver)
{
	active_driver = p_driver ? p_driver : &dummy_driver;
}

void AudioServer::_driver_process(int p_frames, int32_t* p_buffer)
{
	data->mix_count++;
	int todo = p_frames;

#ifdef DEBUG_ENABLED
	uint64_t prof_ticks = OS::get_ticks_usec();
#endif

	if (data->channel_count != get_channel_count()) {
		// Amount of channels changed due to a output_device change
		// reinitialize the buses channels and buffers
		init_channels_and_buffers();
	}

	ERR_FAIL_COND_MSG(data->buses.is_empty() && todo, "AudioServer bus count is less than 1.");
	while (todo) {
		if (data->to_mix == 0) {
			_mix_step();
		}

		int to_copy = MIN(data->to_mix, todo);

		Bus* master = data->buses[0];

		int from = data->buffer_size - data->to_mix;
		int from_buf = p_frames - todo;

		// master master, send to output
		int cs = master->channels.size();

		// Take away 1 from the stride, as we are manually incrementing by 1 for stereo.
		uintptr_t stride_minus_one = (cs * 2) - 1;

		for (int k = 0; k < cs; k++) {
			// The destination start for data will be the same in all cases.
			int32_t* dest = &p_buffer[from_buf * (cs * 2) + (k * 2)];

#ifdef DEBUG_ENABLED
			if (!data->debug_mute && master->channels[k].active) {
#else
			if (master->channels[k].active) {
#endif // DEBUG_ENABLED
				const AudioFrame* buf = master->channels[k].buffer.ptr();

				for (int j = 0; j < to_copy; j++) {
					float l = CLAMP(buf[from + j].left, -1.0, 1.0);
					int32_t vl = l * ((1 << 20) - 1);
					int32_t vl2 = (vl < 0 ? -1 : 1) * (Math::abs(vl) << 11);
					*dest = vl2;
					dest++;

					float r = CLAMP(buf[from + j].right, -1.0, 1.0);
					int32_t vr = r * ((1 << 20) - 1);
					int32_t vr2 = (vr < 0 ? -1 : 1) * (Math::abs(vr) << 11);
					*dest = vr2;
					dest += stride_minus_one;
				}

			}
			else {
				// Bizarrely, profiling indicates that detecting the common case of cs == 1,
				// k == 0, and using memset is SLOWER than setting them individually.
				// Perhaps it gets optimized to a faster instruction than memset.
				for (int j = 0; j < to_copy; j++) {
					*dest = 0;
					dest++;
					*dest = 0;
					dest += stride_minus_one;
				}
			}
		}

		todo -= to_copy;
		data->to_mix -= to_copy;
	}

#ifdef DEBUG_ENABLED
	data->prof_time.add(OS::get_ticks_usec() - prof_ticks);
#endif
}

void AudioServer::_mix_step()
{
	bool solo_mode = false;

	for (int i = 0; i < data->buses.size(); i++) {
		Bus* bus = data->buses[i];
		bus->index_cache = i; // might be moved around by editor, so..
		for (int k = 0; k < bus->channels.size(); k++) {
			bus->channels.write[k].used = false;
		}

		if (bus->solo) {
			// solo chain
			solo_mode = true;
			bus->soloed = true;
			do {
				if (bus != data->buses[0]) {
					// everything has a send save for master bus
					if (!data->bus_map.has(bus->send)) {
						bus = data->buses[0]; // send to master
					}
					else {
						int prev_index_cache = bus->index_cache;
						bus = data->bus_map[bus->send];
						if (prev_index_cache >= bus->index_cache) { // invalid, send to master
							bus = data->buses[0];
						}
					}

					bus->soloed = true;
				}
				else {
					bus = nullptr;
				}

			} while (bus);
		}
		else {
			bus->soloed = false;
		}
	}
	// This is legacy code from 3.x that allows video players and other audio sources that do not
	// implement AudioStreamPlayback to output audio.
	for (CallbackItem* ci : data->mix_callback_list) {
		ci->callback(ci->userdata);
	}

	// Main mixing loop for audio streams.
	// The basic idea here is to copy the samples returned by the AudioStreamPlayback's mix function
	// into the audio buffers,
	//  while always maintaining a lookahead buffer of size LOOKAHEAD_BUFFER_SIZE to allow fade-outs
	//  for sudden stoppages.
	for (AudioStreamPlaybackListNode* playback : data->playback_list) {
		// Paused streams are no-ops. Don't even mix audio from the stream playback.
		if (playback->state.load() == AudioStreamPlaybackListNode::PAUSED) {
			continue;
		}

		if (playback->stream_playback->get_is_sample()) {
			continue;
		}

		// If `fading_out` is true, we're in the process of fading out the stream playback.
		// TODO: Currently this sets the volume of the stream to 0 which creates a linear
		// interpolation between its previous volume and silence.
		//  A more punchy option for fading out could be to just use the lookahead buffer.
		bool fading_out =
			playback->state.load() == AudioStreamPlaybackListNode::FADE_OUT_TO_DELETION ||
			playback->state.load() == AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE;

		AudioFrame* buf = data->mix_buffer.ptrw();

		// Copy the old contents of the lookahead buffer into the beginning of the mix buffer.
		for (int i = 0; i < LOOKAHEAD_BUFFER_SIZE; i++) {
			buf[i] = playback->lookahead[i];
		}

		// Mix the audio stream.
		unsigned int mixed_frames = playback->stream_playback->mix(
			&buf[LOOKAHEAD_BUFFER_SIZE], playback->pitch_scale.get(), data->buffer_size);

		if (data->tag_used_audio_streams && playback->stream_playback->is_playing()) {
			playback->stream_playback->tag_used_streams();
		}

		// Check to see if the stream has run out of samples.
		if (mixed_frames != data->buffer_size) {
			// We know we have at least the size of our lookahead buffer for fade-out purposes.

			float fadeout_base = 0.94;
			float fadeout_coefficient = 1;
			static_assert(LOOKAHEAD_BUFFER_SIZE == 64,
				"Update fadeout_base and comment here if you change LOOKAHEAD_BUFFER_SIZE.");
			// 0.94 ^ 64 = 0.01906. There might still be a pop but it'll be way better than if we
			// didn't do this.
			for (unsigned int idx = mixed_frames; idx < data->buffer_size; idx++) {
				fadeout_coefficient *= fadeout_base;
				buf[idx] *= fadeout_coefficient;
			}
			AudioStreamPlaybackListNode::PlaybackState new_state;
			new_state = AudioStreamPlaybackListNode::AWAITING_DELETION;
			playback->state.store(new_state);
		}
		else {
			// Move the last little bit of what we just mixed into our lookahead buffer for the next
			// call to _mix_step.
			for (int i = 0; i < LOOKAHEAD_BUFFER_SIZE; i++) {
				playback->lookahead[i] = buf[data->buffer_size + i];
			}
		}

		// Get the bus details for this playback. This contains information about which buses the
		// playback is assigned to and the volume of the playback on each bus.
		AudioStreamPlaybackBusDetails* bus_details_ptr = playback->bus_details.load();
		ERR_FAIL_NULL(bus_details_ptr);
		// Make a copy of the bus details so we can modify it without worrying about other threads.
		AudioStreamPlaybackBusDetails bus_details = *bus_details_ptr;

		// Mix to any active buses.
		for (int idx = 0; idx < MAX_BUSES_PER_PLAYBACK; idx++) {
			if (!bus_details.bus_active[idx]) {
				continue;
			}
			// This is the AudioServer-internal index of the bus we're mixing to in this step of the
			// loop. Not to be confused with `idx` which is an index into
			// `AudioStreamPlaybackBusDetails` member var arrays.
			int bus_idx = thread_find_bus_index(bus_details.bus[idx]);

			// It's important to know whether or not this bus was active in the previous mix step of
			// this stream. If it was, we need to perform volume interpolation to avoid pops.
			int prev_bus_idx = -1;
			for (int search_idx = 0; search_idx < MAX_BUSES_PER_PLAYBACK; search_idx++) {
				if (!playback->prev_bus_details->bus_active[search_idx]) {
					continue;
				}
				// If the Strings of the buses match, we've found the previous bus index. This
				// indicates that this playback mixed to `prev_bus_details->bus[prev_bus_index]` in
				// the previous mix step, which gives us a way to look up the playback's previous
				// volume.
				if (playback->prev_bus_details->bus[search_idx].hash() ==
					bus_details.bus[idx].hash()) {
					prev_bus_idx = search_idx;
					break;
				}
			}

			// It's now time to mix to the bus. We do this by going through each channel of the bus
			// and mixing to it.
			//  The channels correspond to output channels of the audio device, e.g. stereo or 5.1.
			//  To reduce needless nesting, this is done with a helper method named
			//  `_mix_step_for_channel`.
			for (int channel_idx = 0; channel_idx < data->channel_count; channel_idx++) {
				AudioFrame* channel_buf = thread_get_channel_mix_buffer(bus_idx, channel_idx);
				// TODO: This `fading_out` check could be replaced with with an exponential fadeout
				// of the samples from the lookahead buffer for more punchy results.
				if (fading_out) {
					bus_details.volume[idx][channel_idx] = AudioFrame(0, 0);
				}
				AudioFrame channel_vol = bus_details.volume[idx][channel_idx];

				// If this bus was not active in the previous mix step, we want to start playback at
				// the full volume to avoid crushing transients.
				AudioFrame prev_channel_vol = channel_vol;
				// If this bus was active in the previous mix step, we need to interpolate between
				// the previous volume and the current volume to avoid pops. Set
				// `prev_channel_volume` accordingly.
				if (prev_bus_idx != -1) {
					prev_channel_vol =
						playback->prev_bus_details->volume[prev_bus_idx][channel_idx];
				}
				_mix_step_for_channel(channel_buf, buf, prev_channel_vol, channel_vol,
					playback->attenuation_filter_cutoff_hz.get(), playback->highshelf_gain.get(),
					&playback->filter_process[channel_idx * 2],
					&playback->filter_process[channel_idx * 2 + 1]);
			}
		}

		// Now go through and fade-out any buses that were being played to previously that we missed
		// by going through current data.
		for (int idx = 0; idx < MAX_BUSES_PER_PLAYBACK; idx++) {
			if (!playback->prev_bus_details->bus_active[idx]) {
				continue;
			}
			int bus_idx = thread_find_bus_index(playback->prev_bus_details->bus[idx]);

			int current_bus_idx = -1;
			for (int search_idx = 0; search_idx < MAX_BUSES_PER_PLAYBACK; search_idx++) {
				if (bus_details.bus[search_idx] == playback->prev_bus_details->bus[idx]) {
					current_bus_idx = search_idx;
				}
			}
			if (current_bus_idx != -1) {
				// If we found a corresponding bus in the current bus assignments, we've already
				// mixed to this bus.
				continue;
			}

			for (int channel_idx = 0; channel_idx < data->channel_count; channel_idx++) {
				AudioFrame* channel_buf = thread_get_channel_mix_buffer(bus_idx, channel_idx);
				AudioFrame prev_channel_vol = playback->prev_bus_details->volume[idx][channel_idx];
				// Fade out to silence. This could be replaced with an exponential fadeout of the
				// samples from the lookahead buffer for more punchy results.
				_mix_step_for_channel(channel_buf, buf, prev_channel_vol, AudioFrame(0, 0),
					playback->attenuation_filter_cutoff_hz.get(), playback->highshelf_gain.get(),
					&playback->filter_process[channel_idx * 2],
					&playback->filter_process[channel_idx * 2 + 1]);
			}
		}

		// Copy the bus details we mixed with to the previous bus details to maintain volume ramps.
		for (int i = 0; i < MAX_BUSES_PER_PLAYBACK; i++) {
			playback->prev_bus_details->bus_active[i] = bus_details.bus_active[i];
		}
		for (int i = 0; i < MAX_BUSES_PER_PLAYBACK; i++) {
			playback->prev_bus_details->bus[i] = bus_details.bus[i];
		}
		for (int i = 0; i < MAX_BUSES_PER_PLAYBACK; i++) {
			for (int j = 0; j < MAX_CHANNELS_PER_BUS; j++) {
				playback->prev_bus_details->volume[i][j] = bus_details.volume[i][j];
			}
		}

		switch (playback->state.load()) {
		case AudioStreamPlaybackListNode::AWAITING_DELETION:
		case AudioStreamPlaybackListNode::FADE_OUT_TO_DELETION:
			// Remove the playback from the list.
			_delete_stream_playback_list_node(playback);
			break;
		case AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE: {
			// Pause the stream.
			playback->state.store(AudioStreamPlaybackListNode::PAUSED);
		} break;
		case AudioStreamPlaybackListNode::PLAYING:
		case AudioStreamPlaybackListNode::PAUSED:
			// No-op!
			break;
		}
	}

	// Now that all of the buses have their audio sources mixed into them, we can process the
	// effects and bus sends.
	for (int i = data->buses.size() - 1; i >= 0; i--) {
		Bus* bus = data->buses[i];

		for (int k = 0; k < bus->channels.size(); k++) {
			if (bus->channels[k].active && !bus->channels[k].used) {
				// Buffer was not used, but it's still active, so it must be cleaned.
				AudioFrame* buf = bus->channels.write[k].buffer.ptrw();

				for (uint32_t j = 0; j < data->buffer_size; j++) {
					buf[j] = AudioFrame(0, 0);
				}
			}
		}

		// Process effects.
		if (!bus->bypass) {
			for (int j = 0; j < bus->effects.size(); j++) {
				if (!bus->effects[j].enabled) {
					continue;
				}

#ifdef DEBUG_ENABLED
				uint64_t ticks = OS::get_ticks_usec();
#endif

				for (int k = 0; k < bus->channels.size(); k++) {
					if (!(bus->channels[k].active ||
							bus->channels[k].effect_instances[j]->process_silence())) {
						continue;
					}
					bus->channels.write[k].effect_instances.write[j]->process(
						bus->channels[k].buffer.ptr(), data->temp_buffer.write[k].ptrw(), data->buffer_size);
				}

				// Swap buffers, so internal buffer always has the right data.
				for (int k = 0; k < bus->channels.size(); k++) {
					if (!(data->buses[i]->channels[k].active ||
							bus->channels[k].effect_instances[j]->process_silence())) {
						continue;
					}
					SWAP(bus->channels.write[k].buffer, data->temp_buffer.write[k]);
				}

#ifdef DEBUG_ENABLED
				bus->effects.write[j].prof_time += OS::get_ticks_usec() - ticks;
#endif
			}
		}

		// Process send.

		Bus* send = nullptr;

		if (i > 0) {
			// Everything has a send except for the master bus.
			if (!data->bus_map.has(bus->send)) {
				send = data->buses[0];
			}
			else {
				send = data->bus_map[bus->send];
				if (send->index_cache >= bus->index_cache) { // Invalid, send to master.
					send = data->buses[0];
				}
			}
		}

		for (int k = 0; k < bus->channels.size(); k++) {
			if (!bus->channels[k].active) {
				bus->channels.write[k].peak_volume =
					AudioFrame(AUDIO_MIN_PEAK_DB, AUDIO_MIN_PEAK_DB);
				continue;
			}

			AudioFrame* buf = bus->channels.write[k].buffer.ptrw();

			AudioFrame peak = AudioFrame(0, 0);

			float volume = Math::db_to_linear(bus->volume_db);

			if (solo_mode) {
				if (!bus->soloed) {
					volume = 0.0;
				}
			}
			else {
				if (bus->mute) {
					volume = 0.0;
				}
			}

			// Apply volume and compute peak.
			for (uint32_t j = 0; j < data->buffer_size; j++) {
				buf[j] *= volume;

				float l = Math::abs(buf[j].left);
				if (l > peak.left) {
					peak.left = l;
				}
				float r = Math::abs(buf[j].right);
				if (r > peak.right) {
					peak.right = r;
				}
			}

			bus->channels.write[k].peak_volume =
				AudioFrame(Math::linear_to_db(peak.left + AUDIO_PEAK_OFFSET),
					Math::linear_to_db(peak.right + AUDIO_PEAK_OFFSET));

			if (!bus->channels[k].used) {
				// See if any audio is contained, because channel was not used.

				if (MAX(peak.right, peak.left) > Math::db_to_linear(data->channel_disable_threshold_db)) {
					bus->channels.write[k].last_mix_with_audio = data->mix_frames;
				}
				else if (data->mix_frames - bus->channels[k].last_mix_with_audio >
						   data->channel_disable_frames) {
					bus->channels.write[k].active = false;
					continue; // went inactive, don't mix.
				}
			}

			if (send) {
				// If not master bus, send.
				AudioFrame* target_buf = thread_get_channel_mix_buffer(send->index_cache, k);

				for (uint32_t j = 0; j < data->buffer_size; j++) {
					target_buf[j] += buf[j];
				}
			}
		}
	}

	data->mix_frames += data->buffer_size;
	data->to_mix = data->buffer_size;
}

void AudioServer::_mix_step_for_channel(AudioFrame* p_out_buf, AudioFrame* p_source_buf,
	AudioFrame p_vol_start, AudioFrame p_vol_final, float p_attenuation_filter_cutoff_hz,
	float p_highshelf_gain, AudioFilterSW::Processor* p_processor_l,
	AudioFilterSW::Processor* p_processor_r)
{
	// TODO: In the future it could be nice to replace all of these hardcoded effects with something
	// a bit cleaner and more flexible, but for now this is what we do to support 3D audio players.
	if (p_highshelf_gain != 0) {
		AudioFilterSW filter;
		filter.set_mode(AudioFilterSW::HIGHSHELF);
		filter.set_sampling_rate(AudioServer::get_mix_rate());
		filter.set_cutoff(p_attenuation_filter_cutoff_hz);
		filter.set_resonance(1);
		filter.set_stages(1);
		filter.set_gain(p_highshelf_gain);

		ERR_FAIL_NULL(p_processor_l);
		ERR_FAIL_NULL(p_processor_r);

		bool is_just_started = p_vol_start.left == 0 && p_vol_start.right == 0;
		p_processor_l->set_filter(&filter, /* clear_history= */ is_just_started);
		p_processor_l->update_coeffs(data->buffer_size);
		p_processor_r->set_filter(&filter, /* clear_history= */ is_just_started);
		p_processor_r->update_coeffs(data->buffer_size);

		for (unsigned int frame_idx = 0; frame_idx < data->buffer_size; frame_idx++) {
			// TODO: Make lerp speed buffer-size-invariant if buffer_size ever becomes a project
			// setting to avoid very small buffer sizes causing pops due to too-fast lerps.
			float lerp_param = (float)frame_idx / data->buffer_size;
			AudioFrame vol = p_vol_final * lerp_param + (1 - lerp_param) * p_vol_start;
			AudioFrame mixed = vol * p_source_buf[frame_idx];
			p_processor_l->process_one_interp(mixed.left);
			p_processor_r->process_one_interp(mixed.right);
			p_out_buf[frame_idx] += mixed;
		}

	}
	else {
		for (unsigned int frame_idx = 0; frame_idx < data->buffer_size; frame_idx++) {
			// TODO: Make lerp speed buffer-size-invariant if buffer_size ever becomes a project
			// setting to avoid very small buffer sizes causing pops due to too-fast lerps.
			float lerp_param = (float)frame_idx / data->buffer_size;
			p_out_buf[frame_idx] += (p_vol_final * lerp_param + (1 - lerp_param) * p_vol_start) *
									p_source_buf[frame_idx];
		}
	}
}

AudioServer::AudioStreamPlaybackListNode* AudioServer::_find_playback_list_node(
	Ref<AudioStreamPlayback> p_playback)
{
	for (AudioStreamPlaybackListNode* playback_list_node : data->playback_list) {
		if (playback_list_node->stream_playback == p_playback) {
			return playback_list_node;
		}
	}
	return nullptr;
}

void AudioServer::_delete_stream_playback(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND(p_playback.is_null());
	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (playback_node) {
		_delete_stream_playback_list_node(playback_node);
	}
}

void AudioServer::_delete_stream_playback_list_node(AudioStreamPlaybackListNode* p_playback_node)
{
	// Remove the playback from the list, registering a destructor to be run on the main thread.
	data->playback_list.erase(p_playback_node, [](AudioStreamPlaybackListNode* p) {
		delete p->prev_bus_details;
		delete p->bus_details.load();
		p->stream_playback.unref();
		delete p;
	});
}

bool AudioServer::thread_has_channel_mix_buffer(int p_bus, int p_buffer)
{
	if (p_bus < 0 || p_bus >= data->buses.size()) {
		return false;
	}
	if (p_buffer < 0 || p_buffer >= data->buses[p_bus]->channels.size()) {
		return false;
	}
	return true;
}

AudioFrame* AudioServer::thread_get_channel_mix_buffer(int p_bus, int p_buffer)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), nullptr);
	ERR_FAIL_INDEX_V(p_buffer, data->buses[p_bus]->channels.size(), nullptr);

	AudioFrame* buf_data = data->buses.write[p_bus]->channels.write[p_buffer].buffer.ptrw();

	if (!data->buses[p_bus]->channels[p_buffer].used) {
		data->buses.write[p_bus]->channels.write[p_buffer].used = true;
		data->buses.write[p_bus]->channels.write[p_buffer].active = true;
		data->buses.write[p_bus]->channels.write[p_buffer].last_mix_with_audio = data->mix_frames;
		for (uint32_t i = 0; i < data->buffer_size; i++) {
			buf_data[i] = AudioFrame(0, 0);
		}
	}

	return buf_data;
}

int AudioServer::thread_get_mix_buffer_size() { return data->buffer_size; }

int AudioServer::thread_find_bus_index(const String& p_name)
{
	if (data->bus_map.has(p_name)) {
		return data->bus_map[p_name]->index_cache;
	}
	else {
		return 0;
	}
}

#ifdef DEBUG_ENABLED
void AudioServer::set_debug_mute(bool p_mute) { data->debug_mute = p_mute; }

bool AudioServer::get_debug_mute() { return data->debug_mute; }
#endif // DEBUG_ENABLED

int AudioServer::get_bus_count() { return data->buses.size(); }

String AudioServer::get_bus_name(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), String());
	return data->buses[p_bus]->name;
}

int AudioServer::get_bus_index(const String& p_bus_name)
{
	for (int i = 0; i < data->buses.size(); ++i) {
		if (data->buses[i]->name == p_bus_name) {
			return i;
		}
	}
	return -1;
}

float AudioServer::get_bus_volume_db(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	return data->buses[p_bus]->volume_db;
}

void AudioServer::set_bus_volume_linear(int p_bus, float p_volume_linear)
{
	set_bus_volume_db(p_bus, Math::linear_to_db(p_volume_linear));
}

float AudioServer::get_bus_volume_linear(int p_bus)
{
	return Math::db_to_linear(get_bus_volume_db(p_bus));
}

int AudioServer::get_bus_channels(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	return data->buses[p_bus]->channels.size();
}

String AudioServer::get_bus_send(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), String());
	return data->buses[p_bus]->send;
}

bool AudioServer::is_bus_solo(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);

	return data->buses[p_bus]->solo;
}

bool AudioServer::is_bus_mute(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);

	return data->buses[p_bus]->mute;
}

bool AudioServer::is_bus_bypassing_effects(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);

	return data->buses[p_bus]->bypass;
}

int AudioServer::get_bus_effect_count(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);

	return data->buses[p_bus]->effects.size();
}

Ref<AudioEffectInstance> AudioServer::get_bus_effect_instance(
	int p_bus, int p_effect, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), Ref<AudioEffectInstance>());
	ERR_FAIL_INDEX_V(p_effect, data->buses[p_bus]->effects.size(), Ref<AudioEffectInstance>());
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), Ref<AudioEffectInstance>());

	return data->buses[p_bus]->channels[p_channel].effect_instances[p_effect];
}

Ref<AudioEffect> AudioServer::get_bus_effect(int p_bus, int p_effect)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), Ref<AudioEffect>());
	ERR_FAIL_INDEX_V(p_effect, data->buses[p_bus]->effects.size(), Ref<AudioEffect>());

	return data->buses[p_bus]->effects[p_effect].effect;
}

bool AudioServer::is_bus_effect_enabled(int p_bus, int p_effect)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);
	ERR_FAIL_INDEX_V(p_effect, data->buses[p_bus]->effects.size(), false);
	return data->buses[p_bus]->effects[p_effect].enabled;
}

float AudioServer::get_bus_peak_volume_left_db(int p_bus, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), 0);

	return data->buses[p_bus]->channels[p_channel].peak_volume.left;
}

float AudioServer::get_bus_peak_volume_right_db(int p_bus, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), 0);

	return data->buses[p_bus]->channels[p_channel].peak_volume.right;
}

bool AudioServer::is_bus_channel_active(int p_bus, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), false);

	return data->buses[p_bus]->channels[p_channel].active;
}

void AudioServer::set_playback_speed_scale(float p_scale)
{
	ERR_FAIL_COND(p_scale <= 0);

	data->playback_speed_scale = p_scale;
}

float AudioServer::get_playback_speed_scale() { return data->playback_speed_scale; }

void AudioServer::start_playback_stream(Ref<AudioStreamPlayback> p_playback,
	const String& p_bus, Vector<AudioFrame> p_volume_db_vector, float p_start_time,
	float p_pitch_scale)
{
	ERR_FAIL_COND(p_playback.is_null());

	HashMap<String, Vector<AudioFrame>> map;
	map[p_bus] = p_volume_db_vector;

	start_playback_stream(p_playback, map, p_start_time, p_pitch_scale);
}

void AudioServer::start_playback_stream(Ref<AudioStreamPlayback> p_playback,
	const HashMap<String, Vector<AudioFrame>>& p_bus_volumes, float p_start_time,
	float p_pitch_scale, float p_highshelf_gain, float p_attenuation_cutoff_hz)
{
	ERR_FAIL_COND(p_playback.is_null());

	AudioStreamPlaybackListNode* playback_node = new AudioStreamPlaybackListNode();
	playback_node->stream_playback = p_playback;
	playback_node->stream_playback->start(p_start_time);

	AudioStreamPlaybackBusDetails* new_bus_details = new AudioStreamPlaybackBusDetails();
	int idx = 0;
	for (KeyValue<String, Vector<AudioFrame>> pair : p_bus_volumes) {
		if (pair.value.size() < data->channel_count || pair.value.size() != MAX_CHANNELS_PER_BUS) {
			delete playback_node;
			delete new_bus_details;
			ERR_FAIL();
		}

		new_bus_details->bus_active[idx] = true;
		new_bus_details->bus[idx] = pair.key;
		for (int channel_idx = 0; channel_idx < MAX_CHANNELS_PER_BUS; channel_idx++) {
			new_bus_details->volume[idx][channel_idx] = pair.value[channel_idx];
		}
		idx++;
	}
	playback_node->bus_details.store(new_bus_details);
	playback_node->prev_bus_details = new AudioStreamPlaybackBusDetails();

	playback_node->pitch_scale.set(p_pitch_scale);
	playback_node->highshelf_gain.set(p_highshelf_gain);
	playback_node->attenuation_filter_cutoff_hz.set(p_attenuation_cutoff_hz);

	memset(playback_node->prev_bus_details->volume, 0,
		sizeof(playback_node->prev_bus_details->volume));

	for (AudioFrame& frame : playback_node->lookahead) {
		frame = AudioFrame(0, 0);
	}

	playback_node->state.store(AudioStreamPlaybackListNode::PLAYING);

	data->playback_list.insert(playback_node);
}

void AudioServer::stop_playback_stream(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND(p_playback.is_null());

	// Handle sample playback.
	if (p_playback->get_is_sample()) {
		if (p_playback->get_sample_playback().is_valid()) {
			AudioServer::stop_sample_playback(p_playback->get_sample_playback());
		}
		else {
			_delete_stream_playback(p_playback);
		}
		return;
	}

	if (!p_playback->is_playing()) {
		p_playback->stop();
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	AudioStreamPlaybackListNode::PlaybackState new_state, old_state;
	do {
		old_state = playback_node->state.load();
		if (old_state == AudioStreamPlaybackListNode::AWAITING_DELETION) {
			break; // Don't fade out again.
		}
		new_state = AudioStreamPlaybackListNode::FADE_OUT_TO_DELETION;

	} while (!playback_node->state.compare_exchange_strong(old_state, new_state));
}

void AudioServer::set_playback_bus_exclusive(
	Ref<AudioStreamPlayback> p_playback, const String& p_bus, Vector<AudioFrame> p_volumes)
{
	ERR_FAIL_COND(p_volumes.size() != MAX_CHANNELS_PER_BUS);

	HashMap<String, Vector<AudioFrame>> map;
	map[p_bus] = p_volumes;

	set_playback_bus_volumes_linear(p_playback, map);
}

void AudioServer::set_playback_bus_volumes_linear(Ref<AudioStreamPlayback> p_playback,
	const HashMap<String, Vector<AudioFrame>>& p_bus_volumes)
{
	ERR_FAIL_COND(p_bus_volumes.size() > MAX_BUSES_PER_PLAYBACK);

	// Samples.
	if (p_playback->get_is_sample() && p_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = p_playback->get_sample_playback();
		AudioDriverManager::get_active_driver()->set_sample_playback_bus_volumes_linear(
			sample_playback, p_bus_volumes);
		return;
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}
	AudioStreamPlaybackBusDetails *old_bus_details,
		*new_bus_details = new AudioStreamPlaybackBusDetails();

	int idx = 0;
	for (KeyValue<String, Vector<AudioFrame>> pair : p_bus_volumes) {
		if (idx >= MAX_BUSES_PER_PLAYBACK) {
			break;
		}
		if (pair.value.size() < data->channel_count || pair.value.size() != MAX_CHANNELS_PER_BUS) {
			delete new_bus_details;
			ERR_FAIL();
		}

		new_bus_details->bus_active[idx] = true;
		new_bus_details->bus[idx] = pair.key;
		for (int channel_idx = 0; channel_idx < MAX_CHANNELS_PER_BUS; channel_idx++) {
			new_bus_details->volume[idx][channel_idx] = pair.value[channel_idx];
		}
		idx++;
	}

	do {
		old_bus_details = playback_node->bus_details.load();
	} while (!playback_node->bus_details.compare_exchange_strong(old_bus_details, new_bus_details));

	data->bus_details_graveyard.insert(old_bus_details);
}

void AudioServer::set_playback_all_bus_volumes_linear(
	Ref<AudioStreamPlayback> p_playback, Vector<AudioFrame> p_volumes)
{
	ERR_FAIL_COND(p_playback.is_null());
	ERR_FAIL_COND(p_volumes.size() != MAX_CHANNELS_PER_BUS);

	HashMap<String, Vector<AudioFrame>> map;

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}
	for (int bus_idx = 0; bus_idx < MAX_BUSES_PER_PLAYBACK; bus_idx++) {
		if (playback_node->bus_details.load()->bus_active[bus_idx]) {
			map[playback_node->bus_details.load()->bus[bus_idx]] = p_volumes;
		}
	}

	set_playback_bus_volumes_linear(p_playback, map);
}

void AudioServer::set_playback_pitch_scale(Ref<AudioStreamPlayback> p_playback, float p_pitch_scale)
{
	ERR_FAIL_COND(p_playback.is_null());

	// Samples.
	if (p_playback->get_is_sample() && p_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = p_playback->get_sample_playback();
		AudioServer::update_sample_playback_pitch_scale(
			sample_playback, p_pitch_scale);
		return;
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	playback_node->pitch_scale.set(p_pitch_scale);
}

void AudioServer::set_playback_paused(Ref<AudioStreamPlayback> p_playback, bool p_paused)
{
	ERR_FAIL_COND(p_playback.is_null());

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	AudioStreamPlaybackListNode::PlaybackState new_state, old_state;
	do {
		old_state = playback_node->state.load();
		new_state = p_paused ? AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE
							 : AudioStreamPlaybackListNode::PLAYING;
		if (!p_paused && old_state == AudioStreamPlaybackListNode::PLAYING) {
			return; // No-op.
		}
		if (p_paused && (old_state == AudioStreamPlaybackListNode::PAUSED ||
							old_state == AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE)) {
			return; // No-op.
		}

	} while (!playback_node->state.compare_exchange_strong(old_state, new_state));
}

void AudioServer::set_playback_highshelf_params(
	Ref<AudioStreamPlayback> p_playback, float p_gain, float p_attenuation_cutoff_hz)
{
	ERR_FAIL_COND(p_playback.is_null());

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	playback_node->attenuation_filter_cutoff_hz.set(p_attenuation_cutoff_hz);
	playback_node->highshelf_gain.set(p_gain);
}

bool AudioServer::is_playback_active(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND_V(p_playback.is_null(), false);

	if (p_playback->get_is_sample()) {
		if (p_playback->get_sample_playback().is_valid()) {
			return data->sample_playback_list.has(p_playback->get_sample_playback());
		}
		else {
			return false;
		}
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return false;
	}

	return playback_node->state.load() == AudioStreamPlaybackListNode::PLAYING;
}

float AudioServer::get_playback_position(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND_V(p_playback.is_null(), 0);

	// Samples.
	if (p_playback->get_is_sample() && p_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = p_playback->get_sample_playback();
		return AudioServer::get_sample_playback_position(sample_playback);
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return 0;
	}

	return playback_node->stream_playback->get_playback_position();
}

bool AudioServer::is_playback_paused(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND_V(p_playback.is_null(), false);

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return false;
	}

	return playback_node->state.load() == AudioStreamPlaybackListNode::PAUSED ||
		   playback_node->state.load() == AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE;
}

uint64_t AudioServer::get_mix_count() { return data->mix_count; }

uint64_t AudioServer::get_mixed_frames() { return data->mix_frames; }

String AudioServer::get_driver_name() { return AudioDriverManager::get_active_driver()->get_name(); }

void AudioServer::notify_listener_changed()
{
	for (CallbackItem* ci : data->listener_changed_callback_list) {
		ci->callback(ci->userdata);
	}
}

void AudioServer::init_channels_and_buffers()
{
	data->channel_count = get_channel_count();
	data->temp_buffer.resize(data->channel_count);
	data->mix_buffer.resize(data->buffer_size + LOOKAHEAD_BUFFER_SIZE);

	for (int i = 0; i < data->temp_buffer.size(); i++) {
		data->temp_buffer.write[i].resize(data->buffer_size);
	}

	for (int i = 0; i < data->buses.size(); i++) {
		data->buses[i]->channels.resize(data->channel_count);
		for (int j = 0; j < data->channel_count; j++) {
			data->buses.write[i]->channels.write[j].buffer.resize(data->buffer_size);
		}
		_update_bus_effects(i);
	}
}

void AudioServer::_cleanup_lists()
{
	data->mix_callback_list.maybe_cleanup();
	data->update_callback_list.maybe_cleanup();
	data->listener_changed_callback_list.maybe_cleanup();
	data->playback_list.maybe_cleanup();
	for (AudioStreamPlaybackBusDetails* bus_details : data->bus_details_graveyard_frame_old) {
		data->bus_details_graveyard_frame_old.erase(
			bus_details, [](AudioStreamPlaybackBusDetails* d) { delete d; });
	}
	for (AudioStreamPlaybackBusDetails* bus_details : data->bus_details_graveyard) {
		data->bus_details_graveyard_frame_old.insert(bus_details);
		data->bus_details_graveyard.erase(bus_details);
	}
	data->bus_details_graveyard.maybe_cleanup();
	data->bus_details_graveyard_frame_old.maybe_cleanup();
}

void AudioServer::finish()
{
	for (int i = 0; i < AudioDriverManager::get_driver_count(); i++) {
		AudioDriverManager::get_driver(i)->finish();
	}

	for (int i = 0; i < data->buses.size(); i++) {
		memdelete(data->buses[i]);
	}

	data->buses.clear();
}

/* MISC config */

void AudioServer::lock() { AudioDriverManager::get_active_driver()->lock(); }

void AudioServer::unlock() { AudioDriverManager::get_active_driver()->unlock(); }

AudioServer::SpeakerMode AudioServer::get_speaker_mode()
{
	return (AudioServer::SpeakerMode)AudioDriverManager::get_active_driver()->get_speaker_mode();
}

float AudioServer::get_mix_rate() { return AudioDriverManager::get_active_driver()->get_mix_rate(); }

float AudioServer::get_input_mix_rate()
{
	return AudioDriverManager::get_active_driver()->get_input_mix_rate();
}

float AudioServer::read_output_peak_db() { return 0; }

double AudioServer::get_output_latency()
{
	return AudioDriverManager::get_active_driver()->get_latency();
}

double AudioServer::get_time_to_next_mix()
{
	return AudioDriverManager::get_active_driver()->get_time_to_next_mix();
}

double AudioServer::get_time_since_last_mix()
{
	return AudioDriverManager::get_active_driver()->get_time_since_last_mix();
}

void AudioServer::add_update_callback(AudioCallback p_callback, void* p_userdata)
{
	CallbackItem* ci = new CallbackItem();
	ci->callback = p_callback;
	ci->userdata = p_userdata;
	data->update_callback_list.insert(ci);
}

void AudioServer::remove_update_callback(AudioCallback p_callback, void* p_userdata)
{
	for (CallbackItem* ci : data->update_callback_list) {
		if (ci->callback == p_callback && ci->userdata == p_userdata) {
			data->update_callback_list.erase(ci, [](CallbackItem* c) { delete c; });
		}
	}
}

void AudioServer::add_mix_callback(AudioCallback p_callback, void* p_userdata)
{
	CallbackItem* ci = new CallbackItem();
	ci->callback = p_callback;
	ci->userdata = p_userdata;
	data->mix_callback_list.insert(ci);
}

void AudioServer::remove_mix_callback(AudioCallback p_callback, void* p_userdata)
{
	for (CallbackItem* ci : data->mix_callback_list) {
		if (ci->callback == p_callback && ci->userdata == p_userdata) {
			data->mix_callback_list.erase(ci, [](CallbackItem* c) { delete c; });
		}
	}
}

void AudioServer::add_listener_changed_callback(AudioCallback p_callback, void* p_userdata)
{
	CallbackItem* ci = new CallbackItem();
	ci->callback = p_callback;
	ci->userdata = p_userdata;
	data->listener_changed_callback_list.insert(ci);
}

void AudioServer::remove_listener_changed_callback(AudioCallback p_callback, void* p_userdata)
{
	for (CallbackItem* ci : data->listener_changed_callback_list) {
		if (ci->callback == p_callback && ci->userdata == p_userdata) {
			data->listener_changed_callback_list.erase(ci, [](CallbackItem* c) { delete c; });
		}
	}
}

Ref<AudioBusLayout> AudioServer::generate_bus_layout()
{
	Ref<AudioBusLayout> state;
	state.instantiate();

	state->buses.resize(data->buses.size());

	for (int i = 0; i < data->buses.size(); i++) {
		state->buses.write[i].name = data->buses[i]->name;
		state->buses.write[i].send = data->buses[i]->send;
		state->buses.write[i].mute = data->buses[i]->mute;
		state->buses.write[i].solo = data->buses[i]->solo;
		state->buses.write[i].bypass = data->buses[i]->bypass;
		state->buses.write[i].volume_db = data->buses[i]->volume_db;
		for (int j = 0; j < data->buses[i]->effects.size(); j++) {
			AudioBusLayout::Bus::Effect fx;
			fx.effect = data->buses[i]->effects[j].effect;
			fx.enabled = data->buses[i]->effects[j].enabled;
			state->buses.write[i].effects.push_back(fx);
		}
	}

	return state;
}

PackedStringArray AudioServer::get_output_device_list()
{
	return AudioDriverManager::get_active_driver()->get_output_device_list();
}

String AudioServer::get_output_device()
{
	return AudioDriverManager::get_active_driver()->get_output_device();
}

void AudioServer::set_output_device(const String& p_name)
{
	AudioDriverManager::get_active_driver()->set_output_device(p_name);
}

PackedStringArray AudioServer::get_input_device_list()
{
	return AudioDriverManager::get_active_driver()->get_input_device_list();
}

String AudioServer::get_input_device() { return AudioDriverManager::get_active_driver()->get_input_device(); }

void AudioServer::set_input_device(const String& p_name)
{
	AudioDriverManager::get_active_driver()->set_input_device(p_name);
}

int AudioServer::get_input_frames_available()
{
	AudioDriver* ad = AudioDriverManager::get_active_driver();
	ad->lock();
	int64_t input_position = ad->get_input_position();
	if (input_position < data->input_buffer_ofs) {
		input_position += ad->get_input_buffer().size();
	}
	ad->unlock();
	return (int)((input_position - data->input_buffer_ofs) / 2); // Buffer is stereo.
}

int AudioServer::get_input_buffer_length_frames()
{
	AudioDriver* ad = AudioDriverManager::get_active_driver();
	ad->lock();
	int buffsize = ad->get_input_buffer().size();
	ad->unlock();
	return buffsize / 2;
}

PackedVector2Array AudioServer::get_input_frames(int p_frames)
{
	PackedVector2Array ret;
	AudioDriver* ad = AudioDriverManager::get_active_driver();
	ad->lock();
	int input_position = ad->get_input_position();
	Vector<int32_t> buf = ad->get_input_buffer();
	if (input_position < data->input_buffer_ofs) {
		input_position += buf.size();
	}
	if ((data->input_buffer_ofs + p_frames * 2 <= input_position) && (p_frames >= 0)) {
		ret.resize(p_frames);
		for (int i = 0; i < p_frames; i++) {
			float l = (buf[data->input_buffer_ofs++] >> 16) / 32768.f;
			if (data->input_buffer_ofs >= buf.size()) {
				data->input_buffer_ofs = 0;
			}
			float r = (buf[data->input_buffer_ofs++] >> 16) / 32768.f;
			if (data->input_buffer_ofs >= buf.size()) {
				data->input_buffer_ofs = 0;
			}
			ret.write[i] = Vector2(l, r);
		}
	}
	ad->unlock();
	return ret;
}

void AudioServer::set_enable_tagging_used_audio_streams(bool p_enable)
{
	data->tag_used_audio_streams = p_enable;
}

AudioServer::PlaybackType AudioServer::get_default_playback_type()
{
	int playback_type = GLOBAL_GET_CACHED(int, "audio/general/default_playback_type");
	ERR_FAIL_COND_V_MSG(playback_type < 0 || playback_type >= PlaybackType::PLAYBACK_TYPE_MAX,
		PlaybackType::PLAYBACK_TYPE_STREAM,
		vformat(
			R"(Project settings value (%s) for "audio/general/default_playback_type" is not supported)",
			playback_type));

	switch (playback_type) {
	case 1: {
		return PlaybackType::PLAYBACK_TYPE_SAMPLE;
	} break;

	case 0:
	default: {
		return PlaybackType::PLAYBACK_TYPE_STREAM;
	} break;
	}
}

bool AudioServer::is_stream_registered_as_sample(const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND_V_MSG(p_stream.is_null(), false, "Parameter p_stream is null.");
	return AudioDriverManager::get_active_driver()->is_stream_registered_as_sample(p_stream);
}

void AudioServer::register_stream_as_sample(const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND_MSG(p_stream.is_null(), "Parameter p_stream is null.");
	ERR_FAIL_COND_MSG(!(p_stream->can_be_sampled()), "Parameter p_stream cannot be sampled.");
	Ref<AudioSample> sample = p_stream->generate_sample();
	register_sample(sample);
}

void AudioServer::unregister_stream_as_sample(const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND_MSG(p_stream.is_null(), "Parameter p_stream is null.");
	ERR_FAIL_COND_MSG(!(p_stream->can_be_sampled()), "Parameter p_stream cannot be sampled.");
	Ref<AudioSample> sample = p_stream->generate_sample();
	unregister_sample(sample);
}

void AudioServer::register_sample(const Ref<AudioSample>& p_sample)
{
	ERR_FAIL_COND_MSG(p_sample.is_null(), "Parameter p_sample is null.");
	ERR_FAIL_COND_MSG(p_sample->stream.is_null(), "Parameter p_sample->stream is null.");
	ERR_FAIL_COND_MSG(
		!(p_sample->stream->can_be_sampled()), "Parameter p_stream cannot be sampled.");
	AudioDriverManager::get_active_driver()->register_sample(p_sample);
}

void AudioServer::unregister_sample(const Ref<AudioSample>& p_sample)
{
	ERR_FAIL_COND_MSG(p_sample.is_null(), "Parameter p_sample is null.");
	ERR_FAIL_COND_MSG(p_sample->stream.is_null(), "Parameter p_sample->stream is null.");
	AudioDriverManager::get_active_driver()->unregister_sample(p_sample);
}

void AudioServer::start_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	AudioDriverManager::get_active_driver()->start_sample_playback(p_playback);
	data->sample_playback_list.ordered_insert(p_playback);
}

void AudioServer::stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	if (!data->sample_playback_list.has(p_playback)) {
		return;
	}
	data->sample_playback_list.erase(p_playback);
	AudioDriverManager::get_active_driver()->stop_sample_playback(p_playback);
	p_playback->stream_playback->set_sample_playback(nullptr);
	stop_playback_stream(p_playback->stream_playback);
}

void AudioServer::set_sample_playback_pause(
	const Ref<AudioSamplePlayback>& p_playback, bool p_paused)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	AudioDriverManager::get_active_driver()->set_sample_playback_pause(p_playback, p_paused);
}

bool AudioServer::is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_V_MSG(p_playback.is_null(), false, "Parameter p_playback is null.");
	return data->sample_playback_list.has(p_playback);
}

double AudioServer::get_sample_playback_position(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_V_MSG(p_playback.is_null(), false, "Parameter p_playback is null.");
	return AudioDriverManager::get_active_driver()->get_sample_playback_position(p_playback);
}

void AudioServer::update_sample_playback_pitch_scale(
	const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	return AudioDriverManager::get_active_driver()->update_sample_playback_pitch_scale(
		p_playback, p_pitch_scale);
}

void AudioServer::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void AudioServer::finalize()
{
	if (!data) {
		return;
	}
	// Cleanup resources while we still have an active AudioServer state,
	// for resources that depend on AudioServer still existing.
	_cleanup_lists();

	memdelete(data);
	data = nullptr;
}

AudioBusLayout::AudioBusLayout()
{
	buses.resize(1);
	buses.write[0].name = SceneString(Master);
}

void AudioServer::_update_bus_effects(int) {}

void AudioServer::set_bus_volume_db(int, float) {}

void AudioServer::init() {}

void AudioDriverManager::initialize(int) {}

void AudioServer::update() {}

void AudioServer::load_default_bus_layout() {}

void AudioServer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options)
{
}

#ifdef MARK_EDITED
#undef MARK_EDITED
#endif

// ============================================================================
// Merged from servers/audio/audio_driver_dummy.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "core/os/os.h"

Error AudioDriverDummy::init() {
	active.clear();
	exit_thread.clear();
	samples_in = nullptr;

	if (mix_rate == -1) {
		mix_rate = _get_configured_mix_rate();
	}

	channels = get_channels();
	samples_in = memnew_arr(int32_t, size_t(buffer_frames) * channels);

	if (use_threads) {
		thread.start(AudioDriverDummy::thread_func, this);
	}

	return OK;
}

void AudioDriverDummy::thread_func(void *p_udata) {
	AudioDriverDummy *ad = static_cast<AudioDriverDummy *>(p_udata);

	uint64_t usdelay = (ad->buffer_frames / float(ad->mix_rate)) * 1000000;

	while (!ad->exit_thread.is_set()) {
		if (ad->active.is_set()) {
			ad->lock();
			ad->start_counting_ticks();

			ad->audio_server_process(ad->buffer_frames, ad->samples_in);

			ad->stop_counting_ticks();
			ad->unlock();
		}

		OS::delay_usec(usdelay);
	}
}

void AudioDriverDummy::start() {
	active.set();
}

int AudioDriverDummy::get_mix_rate() const {
	return mix_rate;
}

AudioDriver::SpeakerMode AudioDriverDummy::get_speaker_mode() const {
	return speaker_mode;
}

void AudioDriverDummy::lock() {
	mutex.lock();
}

void AudioDriverDummy::unlock() {
	mutex.unlock();
}

void AudioDriverDummy::set_use_threads(bool p_use_threads) {
	use_threads = p_use_threads;
}

void AudioDriverDummy::set_speaker_mode(SpeakerMode p_mode) {
	speaker_mode = p_mode;
}

void AudioDriverDummy::set_mix_rate(int p_rate) {
	mix_rate = p_rate;
}

uint32_t AudioDriverDummy::get_channels() const {
	static const int channels_for_mode[4] = { 2, 4, 8, 16 };
	return channels_for_mode[speaker_mode];
}

void AudioDriverDummy::mix_audio(int p_frames, int32_t *p_buffer) {
	ERR_FAIL_COND(!active.is_set()); // If not active, should not mix.
	ERR_FAIL_COND(use_threads == true); // If using threads, this will not work well.

	uint32_t todo = p_frames;
	while (todo) {
		uint32_t to_mix = MIN(buffer_frames, todo);
		lock();
		audio_server_process(to_mix, samples_in);
		unlock();

		uint32_t total_samples = to_mix * channels;

		for (uint32_t i = 0; i < total_samples; i++) {
			p_buffer[i] = samples_in[i];
		}

		todo -= to_mix;
		p_buffer += total_samples;
	}
}

void AudioDriverDummy::finish() {
	if (use_threads) {
		exit_thread.set();
		if (thread.is_started()) {
			thread.wait_to_finish();
		}
	}

	if (samples_in) {
		memdelete_arr(samples_in);
	}
}

AudioDriverDummy::AudioDriverDummy() {}

// ============================================================================
// Merged from servers/audio/audio_filter_sw.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "core/math/math_funcs.h"

void AudioFilterSW::set_mode(Mode p_mode) {
	mode = p_mode;
}

void AudioFilterSW::set_cutoff(float p_cutoff) {
	cutoff = p_cutoff;
}

void AudioFilterSW::set_resonance(float p_resonance) {
	resonance = p_resonance;
}

void AudioFilterSW::set_gain(float p_gain) {
	gain = p_gain;
}

void AudioFilterSW::set_sampling_rate(float p_srate) {
	sampling_rate = p_srate;
}

void AudioFilterSW::prepare_coefficients(Coeffs *p_coeffs) {
	int sr_limit = (sampling_rate / 2) + 512;

	double final_cutoff = (cutoff > sr_limit) ? sr_limit : cutoff;
	if (final_cutoff < 1) {
		final_cutoff = 1; //don't allow less than this
	}

	double omega = Math::TAU * final_cutoff / sampling_rate;

	double sin_v = Math::sin(omega);
	double cos_v = Math::cos(omega);

	double Q = resonance;
	if (Q <= 0.0) {
		Q = 0.0001;
	}

	if (mode == BANDPASS) {
		Q *= 2.0;
	} else if (mode == PEAK) {
		Q *= 3.0;
	}

	double tmpgain = gain;

	if (tmpgain < 0.001) {
		tmpgain = 0.001;
	}

	if (stages > 1) {
		Q = (Q > 1.0 ? Math::pow(Q, 1.0 / stages) : Q);
		tmpgain = Math::pow(tmpgain, 1.0 / (stages + 1));
	}
	double alpha = sin_v / (2 * Q);

	double a0 = 1.0 + alpha;

	switch (mode) {
		case LOWPASS: {
			p_coeffs->b0 = (1.0 - cos_v) / 2.0;
			p_coeffs->b1 = 1.0 - cos_v;
			p_coeffs->b2 = (1.0 - cos_v) / 2.0;
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;

		case HIGHPASS: {
			p_coeffs->b0 = (1.0 + cos_v) / 2.0;
			p_coeffs->b1 = -(1.0 + cos_v);
			p_coeffs->b2 = (1.0 + cos_v) / 2.0;
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;

		case BANDPASS: {
			p_coeffs->b0 = alpha * std::sqrt(Q + 1);
			p_coeffs->b1 = 0.0;
			p_coeffs->b2 = -alpha * std::sqrt(Q + 1);
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;

		case NOTCH: {
			p_coeffs->b0 = 1.0;
			p_coeffs->b1 = -2.0 * cos_v;
			p_coeffs->b2 = 1.0;
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;
		case PEAK: {
			p_coeffs->b0 = (1.0 + alpha * tmpgain);
			p_coeffs->b1 = (-2.0 * cos_v);
			p_coeffs->b2 = (1.0 - alpha * tmpgain);
			p_coeffs->a1 = -2 * cos_v;
			p_coeffs->a2 = (1 - alpha / tmpgain);
		} break;
		case BANDLIMIT: {
			//this one is extra tricky
			double hicutoff = resonance;
			double centercutoff = (cutoff + resonance) / 2.0;
			double bandwidth = (Math::log(centercutoff) - Math::log(hicutoff)) / Math::log((double)2);
			omega = Math::TAU * centercutoff / sampling_rate;
			alpha = Math::sin(omega) * Math::sinh(Math::log((double)2) / 2 * bandwidth * omega / Math::sin(omega));
			a0 = 1 + alpha;

			p_coeffs->b0 = alpha;
			p_coeffs->b1 = 0;
			p_coeffs->b2 = -alpha;
			p_coeffs->a1 = -2 * Math::cos(omega);
			p_coeffs->a2 = 1 - alpha;

		} break;
		case LOWSHELF: {
			double tmpq = Math::sqrt(Q);
			if (tmpq <= 0) {
				tmpq = 0.001;
			}
			double beta = Math::sqrt(tmpgain) / tmpq;

			a0 = (tmpgain + 1.0) + (tmpgain - 1.0) * cos_v + beta * sin_v;
			p_coeffs->b0 = tmpgain * ((tmpgain + 1.0) - (tmpgain - 1.0) * cos_v + beta * sin_v);
			p_coeffs->b1 = 2.0 * tmpgain * ((tmpgain - 1.0) - (tmpgain + 1.0) * cos_v);
			p_coeffs->b2 = tmpgain * ((tmpgain + 1.0) - (tmpgain - 1.0) * cos_v - beta * sin_v);
			p_coeffs->a1 = -2.0 * ((tmpgain - 1.0) + (tmpgain + 1.0) * cos_v);
			p_coeffs->a2 = ((tmpgain + 1.0) + (tmpgain - 1.0) * cos_v - beta * sin_v);

		} break;
		case HIGHSHELF: {
			double tmpq = Math::sqrt(Q);
			if (tmpq <= 0) {
				tmpq = 0.001;
			}
			double beta = Math::sqrt(tmpgain) / tmpq;

			a0 = (tmpgain + 1.0) - (tmpgain - 1.0) * cos_v + beta * sin_v;
			p_coeffs->b0 = tmpgain * ((tmpgain + 1.0) + (tmpgain - 1.0) * cos_v + beta * sin_v);
			p_coeffs->b1 = -2.0 * tmpgain * ((tmpgain - 1.0) + (tmpgain + 1.0) * cos_v);
			p_coeffs->b2 = tmpgain * ((tmpgain + 1.0) + (tmpgain - 1.0) * cos_v - beta * sin_v);
			p_coeffs->a1 = 2.0 * ((tmpgain - 1.0) - (tmpgain + 1.0) * cos_v);
			p_coeffs->a2 = ((tmpgain + 1.0) - (tmpgain - 1.0) * cos_v - beta * sin_v);

		} break;
	}

	p_coeffs->b0 /= a0;
	p_coeffs->b1 /= a0;
	p_coeffs->b2 /= a0;
	p_coeffs->a1 /= 0.0 - a0;
	p_coeffs->a2 /= 0.0 - a0;
}

void AudioFilterSW::set_stages(int p_stages) {
	stages = p_stages;
}

/* Fourier transform kernel to obtain response */

float AudioFilterSW::get_response(float p_freq, Coeffs *p_coeffs) {
	float freq = p_freq / sampling_rate * Math::TAU;

	float cx = p_coeffs->b0, cy = 0.0;

	cx += std::cos(freq) * p_coeffs->b1;
	cy -= std::sin(freq) * p_coeffs->b1;
	cx += std::cos(2 * freq) * p_coeffs->b2;
	cy -= std::sin(2 * freq) * p_coeffs->b2;

	float H = cx * cx + cy * cy;
	cx = 1.0;
	cy = 0.0;

	cx -= std::cos(freq) * p_coeffs->a1;
	cy += std::sin(freq) * p_coeffs->a1;
	cx -= std::cos(2 * freq) * p_coeffs->a2;
	cy += std::sin(2 * freq) * p_coeffs->a2;

	H = H / (cx * cx + cy * cy);
	return H;
}

AudioFilterSW::Processor::Processor() {
	set_filter(nullptr);
}

void AudioFilterSW::Processor::set_filter(AudioFilterSW *p_filter, bool p_clear_history) {
	if (p_clear_history) {
		ha1 = ha2 = hb1 = hb2 = 0;
	}
	filter = p_filter;
}

void AudioFilterSW::Processor::update_coeffs(int p_interp_buffer_len) {
	if (!filter) {
		return;
	}

	if (p_interp_buffer_len) { //interpolate
		Coeffs old_coeffs = coeffs;
		filter->prepare_coefficients(&coeffs);
		incr_coeffs.a1 = (coeffs.a1 - old_coeffs.a1) / p_interp_buffer_len;
		incr_coeffs.a2 = (coeffs.a2 - old_coeffs.a2) / p_interp_buffer_len;
		incr_coeffs.b0 = (coeffs.b0 - old_coeffs.b0) / p_interp_buffer_len;
		incr_coeffs.b1 = (coeffs.b1 - old_coeffs.b1) / p_interp_buffer_len;
		incr_coeffs.b2 = (coeffs.b2 - old_coeffs.b2) / p_interp_buffer_len;
		coeffs = old_coeffs;
	} else {
		filter->prepare_coefficients(&coeffs);
	}
}

void AudioFilterSW::Processor::process(float *p_samples, int p_amount, int p_stride, bool p_interpolate) {
	if (!filter) {
		return;
	}

	if (p_interpolate) {
		for (int i = 0; i < p_amount; i++) {
			process_one_interp(*p_samples);
			p_samples += p_stride;
		}
	} else {
		for (int i = 0; i < p_amount; i++) {
			process_one(*p_samples);
			p_samples += p_stride;
		}
	}
}

// ============================================================================
// Merged from servers/audio/audio_rb_resampler.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "servers/audio/audio_server.h"
#include "core/math/math_funcs_binary.h"
#include "core/os/memory.h"

int AudioRBResampler::get_channel_count() const {
	if (!rb) {
		return 0;
	}

	return channels;
}

// Linear interpolation based sample rate conversion (low quality)
// Note that AudioStreamPlaybackResampled::mix has better algorithm,
// but it wasn't obvious to integrate that with VideoStreamPlayer
template <int C>
uint32_t AudioRBResampler::_resample(AudioFrame *p_dest, int p_todo, int32_t p_increment) {
	uint32_t read = offset & MIX_FRAC_MASK;

	for (int i = 0; i < p_todo; i++) {
		offset = (offset + p_increment) & (((1 << (rb_bits + MIX_FRAC_BITS)) - 1));
		read += p_increment;
		uint32_t pos = offset >> MIX_FRAC_BITS;
		float frac = float(offset & MIX_FRAC_MASK) / float(MIX_FRAC_LEN);
		ERR_FAIL_COND_V(pos >= rb_len, 0);
		uint32_t pos_next = (pos + 1) & rb_mask;

		// since this is a template with a known compile time value (C), conditionals go away when compiling.
		if constexpr (C == 1) {
			float v0 = rb[pos];
			float v0n = rb[pos_next];
			v0 += (v0n - v0) * frac;
			p_dest[i] = AudioFrame(v0, v0);
		}

		if constexpr (C == 2) {
			float v0 = rb[(pos << 1) + 0];
			float v1 = rb[(pos << 1) + 1];
			float v0n = rb[(pos_next << 1) + 0];
			float v1n = rb[(pos_next << 1) + 1];

			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}

		// Downmix to stereo. Apply -3dB to center, and sides, -6dB to rear.

		// four channels - channel order: front left, front right, rear left, rear right
		if constexpr (C == 4) {
			float v0 = rb[(pos << 2) + 0] + rb[(pos << 2) + 2] / 2;
			float v1 = rb[(pos << 2) + 1] + rb[(pos << 2) + 3] / 2;
			float v0n = rb[(pos_next << 2) + 0] + rb[(pos_next << 2) + 2] / 2;
			float v1n = rb[(pos_next << 2) + 1] + rb[(pos_next << 2) + 3] / 2;
			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}

		// six channels - channel order: front left, center, front right, rear left, rear right, LFE
		if constexpr (C == 6) {
			float v0 = rb[(pos * 6) + 0] + rb[(pos * 6) + 1] / Math::SQRT2 + rb[(pos * 6) + 3] / 2;
			float v1 = rb[(pos * 6) + 2] + rb[(pos * 6) + 1] / Math::SQRT2 + rb[(pos * 6) + 4] / 2;
			float v0n = rb[(pos_next * 6) + 0] + rb[(pos_next * 6) + 1] / Math::SQRT2 + rb[(pos_next * 6) + 3] / 2;
			float v1n = rb[(pos_next * 6) + 2] + rb[(pos_next * 6) + 1] / Math::SQRT2 + rb[(pos_next * 6) + 4] / 2;
			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}

		// eight channels - channel order: front left, center, front right, side left, side right, rear left, rear
		// right, LFE
		if constexpr (C == 8) {
			float v0 = rb[(pos << 3) + 0] + rb[(pos << 3) + 1] / Math::SQRT2 + rb[(pos << 3) + 3] / Math::SQRT2 + rb[(pos << 3) + 5] / 2;
			float v1 = rb[(pos << 3) + 2] + rb[(pos << 3) + 1] / Math::SQRT2 + rb[(pos << 3) + 4] / Math::SQRT2 + rb[(pos << 3) + 6] / 2;
			float v0n = rb[(pos_next << 3) + 0] + rb[(pos_next << 3) + 1] / Math::SQRT2 + rb[(pos_next << 3) + 3] / Math::SQRT2 + rb[(pos_next << 3) + 5] / 2;
			float v1n = rb[(pos_next << 3) + 2] + rb[(pos_next << 3) + 1] / Math::SQRT2 + rb[(pos_next << 3) + 4] / Math::SQRT2 + rb[(pos_next << 3) + 6] / 2;
			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}
	}

	return read >> MIX_FRAC_BITS; //rb_read_pos = offset >> MIX_FRAC_BITS;
}

bool AudioRBResampler::mix(AudioFrame *p_dest, int p_frames) {
	if (!rb) {
		return false;
	}

	int32_t increment = (src_mix_rate * MIX_FRAC_LEN * playback_speed) / target_mix_rate;
	int read_space = get_reader_space();
	int target_todo = MIN(get_num_of_ready_frames(), p_frames);

	{
		int src_read = 0;
		switch (channels) {
			case 1:
				src_read = _resample<1>(p_dest, target_todo, increment);
				break;
			case 2:
				src_read = _resample<2>(p_dest, target_todo, increment);
				break;
			case 4:
				src_read = _resample<4>(p_dest, target_todo, increment);
				break;
			case 6:
				src_read = _resample<6>(p_dest, target_todo, increment);
				break;
			case 8:
				src_read = _resample<8>(p_dest, target_todo, increment);
				break;
		}

		if (src_read > read_space) {
			src_read = read_space;
		}

		rb_read_pos.set((rb_read_pos.get() + src_read) & rb_mask);

		// Create fadeout effect for the end of stream (note that it can be because of slow writer)
		if (p_frames - target_todo > 0) {
			for (int i = 0; i < target_todo; i++) {
				p_dest[i] = p_dest[i] * float(target_todo - i) / float(target_todo);
			}
		}

		// Fill zeros (silence) for the rest of frames
		for (int i = target_todo; i < p_frames; i++) {
			p_dest[i] = AudioFrame(0, 0);
		}
	}

	return true;
}

int AudioRBResampler::get_num_of_ready_frames() {
	if (!is_ready()) {
		return 0;
	}
	int32_t increment = (src_mix_rate * MIX_FRAC_LEN) / target_mix_rate;
	int read_space = get_reader_space();
	return (int64_t(read_space) << MIX_FRAC_BITS) / increment;
}

Error AudioRBResampler::setup(int p_channels, int p_src_mix_rate, int p_target_mix_rate, int p_buffer_msec, int p_minbuff_needed) {
	ERR_FAIL_COND_V(p_channels != 1 && p_channels != 2 && p_channels != 4 && p_channels != 6 && p_channels != 8, ERR_INVALID_PARAMETER);

	int desired_rb_bits = Math::nearest_shift((uint32_t)MAX((p_buffer_msec / 1000.0) * p_src_mix_rate, p_minbuff_needed));

	bool recreate = !rb;

	if (rb && (uint32_t(desired_rb_bits) != rb_bits || channels != uint32_t(p_channels))) {
		memdelete_arr(rb);
		memdelete_arr(read_buf);
		recreate = true;
	}

	if (recreate) {
		channels = p_channels;
		rb_bits = desired_rb_bits;
		rb_len = (1 << rb_bits);
		rb_mask = rb_len - 1;
		const size_t array_size = rb_len * (size_t)p_channels;
		rb = memnew_arr(float, array_size);
		read_buf = memnew_arr(float, array_size);
	}

	src_mix_rate = p_src_mix_rate;
	target_mix_rate = p_target_mix_rate;
	offset = 0;
	rb_read_pos.set(0);
	rb_write_pos.set(0);

	//avoid maybe strange noises upon load
	for (unsigned int i = 0; i < (rb_len * channels); i++) {
		rb[i] = 0;
		read_buf[i] = 0;
	}

	return OK;
}

void AudioRBResampler::clear() {
	if (!rb) {
		return;
	}

	//should be stopped at this point but just in case
	memdelete_arr(rb);
	memdelete_arr(read_buf);
	rb = nullptr;
	offset = 0;
	rb_read_pos.set(0);
	rb_write_pos.set(0);
	read_buf = nullptr;
}

void AudioRBResampler::set_playback_speed(double p_playback_speed) {
	playback_speed = p_playback_speed;
}

double AudioRBResampler::get_playback_speed() const {
	return playback_speed;
}

AudioRBResampler::AudioRBResampler() {
	rb = nullptr;
	offset = 0;
	read_buf = nullptr;
	rb_read_pos.set(0);
	rb_write_pos.set(0);

	rb_bits = 0;
	rb_len = 0;
	rb_mask = 0;
	read_buff_len = 0;
	channels = 0;
	src_mix_rate = 0;
	target_mix_rate = 0;
}

AudioRBResampler::~AudioRBResampler() {
	if (rb) {
		memdelete_arr(rb);
		memdelete_arr(read_buf);
	}
}

// ============================================================================
// Merged from servers/audio/audio_effect.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "servers/audio/audio_server.h"
// --- Inlined from servers/audio/effects/audio_effects.h ---
#include "core/os/thread.h"
#include "core/templates/local_vector.h"
#include "core/types.h"
#include "scene/resources/audio_stream_wav.h"
#include "servers/audio/audio_server.h"
// --- Folded from servers/audio/effects/eq_filter.h ---
#include "core/templates/vector.h"

class EQ {
public:
	enum Preset {
		PRESET_6_BANDS,
		PRESET_8_BANDS,
		PRESET_10_BANDS,
		PRESET_21_BANDS,
		PRESET_31_BANDS
	};

	class BandProcess {
		friend class EQ;
		float c1, c2, c3;
		struct History {
			float a1, a2, a3;
			float b1, b2, b3;

		} history;

	public:
		inline void process_one(float &p_data);

		BandProcess();
	};

private:
	struct Band {
		float freq;
		float c1, c2, c3;
	};

	Vector<Band> band;

	float mix_rate;

	void recalculate_band_coefficients();

public:
	void set_mix_rate(float p_mix_rate);

	int get_band_count() const;
	void set_preset_band_mode(Preset p_preset);
	void set_bands(const Vector<float> &p_bands);
	BandProcess get_band_processor(int p_band) const;
	float get_band_frequency(int p_band);

	EQ();
	~EQ();
};

/* Inline Function */

inline void EQ::BandProcess::process_one(float &p_data) {
	history.a1 = p_data;

	history.b1 = c1 * (history.a1 - history.a3) + c3 * history.b2 - c2 * history.b3;

	p_data = history.b1;

	history.a3 = history.a2;
	history.a2 = history.a1;
	history.b3 = history.b2;
	history.b2 = history.b1;
}

// --- End servers/audio/effects/eq_filter.h ---
// --- Folded from servers/audio/effects/reverb_filter.h ---
class Reverb {
public:
	enum {
		INPUT_BUFFER_MAX_SIZE = 1024,

	};

private:
	enum {
		MAX_COMBS = 8,
		MAX_ALLPASS = 4,
		MAX_ECHO_MS = 500

	};

	static const float comb_tunings[MAX_COMBS];
	static const float allpass_tunings[MAX_ALLPASS];

	struct Comb {
		int size = 0;
		float *buffer = nullptr;
		float feedback = 0;
		float damp = 0; //lowpass
		float damp_h = 0; //history
		int pos = 0;
		int extra_spread_frames = 0;

		Comb() {}
	};

	struct AllPass {
		int size = 0;
		float *buffer = nullptr;
		int pos = 0;
		int extra_spread_frames = 0;
		AllPass() {}
	};

	Comb comb[MAX_COMBS];
	AllPass allpass[MAX_ALLPASS];
	float *input_buffer = nullptr;
	float *echo_buffer = nullptr;
	int echo_buffer_size = 0;
	int echo_buffer_pos = 0;

	float hpf_h1 = 0.0f;
	float hpf_h2 = 0.0f;

	struct Parameters {
		float room_size;
		float damp;
		float wet;
		float dry;
		float mix_rate;
		float extra_spread_base;
		float extra_spread;
		float predelay;
		float predelay_fb;
		float hpf;
	} params;

	void configure_buffers();
	void update_parameters();
	void clear_buffers();

public:
	void set_room_size(float p_size);
	void set_damp(float p_damp);
	void set_wet(float p_wet);
	void set_dry(float p_dry);
	void set_predelay(float p_predelay); // in ms
	void set_predelay_feedback(float p_predelay_fb); // in ms
	void set_highpass(float p_frq);
	void set_mix_rate(float p_mix_rate);
	void set_extra_spread(float p_spread);
	void set_extra_spread_base(float p_sec);

	void process(float *p_src, float *p_dst, int p_frames);

	Reverb();

	~Reverb();
};

// --- End servers/audio/effects/reverb_filter.h ---

// ============================================================================
// 1. Amplify
// ============================================================================

struct AudioEffectAmplifyConfig
{
	float volume_db = 0.0f;

	void set_volume_db(float p_volume);
	float get_volume_db() const;
	void set_volume_linear(float p_volume);
	float get_volume_linear() const;
};

struct AudioEffectAmplifyState
{
	Ref<AudioEffect> base;
	float mix_volume_db = 0.0f;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectAmplifyInstance = AudioEffectAmplifyState;

// ============================================================================
// 2. Capture
// ============================================================================

struct AudioEffectCaptureConfig
{
	RingBuffer<AudioFrame> buffer;
	SafeNumeric<uint64_t> discarded_frames;
	SafeNumeric<uint64_t> pushed_frames;
	float buffer_length_seconds = 0.1f;
	bool buffer_initialized = false;

	void set_buffer_length(float p_buffer_length_seconds);
	float get_buffer_length() const;

	bool can_get_buffer(int p_frames) const;
	PackedVector2Array get_buffer(int p_frames);
	void clear_buffer();

	int get_frames_available() const;
	int64_t get_discarded_frames() const;
	int get_buffer_length_frames() const;
	int64_t get_pushed_frames() const;
};

struct AudioEffectCaptureState
{
	Ref<AudioEffect> base;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	bool process_silence() const;
};

using AudioEffectCaptureInstance = AudioEffectCaptureState;

// ============================================================================
// 3. Chorus
// ============================================================================

struct AudioEffectChorusConfig
{
	static constexpr int32_t MAX_DELAY_MS = 50;
	static constexpr int32_t MAX_DEPTH_MS = 20;
	static constexpr int32_t MAX_WIDTH_MS = 50;
	static constexpr int32_t MAX_VOICES = 4;
	static constexpr int32_t CYCLES_FRAC = 16;
	static constexpr int32_t CYCLES_MASK = (1 << CYCLES_FRAC) - 1;
	static constexpr int32_t MAX_CHANNELS = 4;
	static constexpr int32_t MS_CUTOFF_MAX = 16000;

	struct Voice
	{
		float delay = 12.0f;
		float rate = 1.0f;
		float depth = 0.0f;
		float level = 0.0f;
		float cutoff = MS_CUTOFF_MAX;
		float pan = 0.0f;
	};

	Voice voice[MAX_VOICES];
	int voice_count = 2;
	float wet = 0.5f;
	float dry = 1.0f;

	void set_voice_count(int p_voices);
	int get_voice_count() const;

	void set_voice_delay_ms(int p_voice, float p_delay_ms);
	float get_voice_delay_ms(int p_voice) const;

	void set_voice_rate_hz(int p_voice, float p_rate_hz);
	float get_voice_rate_hz(int p_voice) const;

	void set_voice_depth_ms(int p_voice, float p_depth_ms);
	float get_voice_depth_ms(int p_voice) const;

	void set_voice_level_db(int p_voice, float p_level_db);
	float get_voice_level_db(int p_voice) const;

	void set_voice_cutoff_hz(int p_voice, float p_cutoff_hz);
	float get_voice_cutoff_hz(int p_voice) const;

	void set_voice_pan(int p_voice, float p_pan);
	float get_voice_pan(int p_voice) const;

	void set_wet(float p_amount);
	float get_wet() const;

	void set_dry(float p_amount);
	float get_dry() const;

	AudioEffectChorusConfig();
};

struct AudioEffectChorusState
{
	Ref<AudioEffect> base;

	Vector<AudioFrame> audio_buffer;
	unsigned int buffer_pos = 0;
	unsigned int buffer_mask = 0;

	AudioFrame filter_h[4];
	uint64_t cycles[4] = {};

	void _process_chunk(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectChorusInstance = AudioEffectChorusState;

// ============================================================================
// 4. Compressor
// ============================================================================

struct AudioEffectCompressorConfig
{
	float threshold = 0.0f;
	float ratio = 4.0f;
	float gain = 0.0f;
	float attack_us = 20.0f;
	float release_ms = 250.0f;
	float mix = 1.0f;
	String sidechain;

	void set_threshold(float p_threshold);
	float get_threshold() const;

	void set_ratio(float p_ratio);
	float get_ratio() const;

	void set_gain(float p_gain);
	float get_gain() const;

	void set_attack_us(float p_attack_us);
	float get_attack_us() const;

	void set_release_ms(float p_release_ms);
	float get_release_ms() const;

	void set_mix(float p_mix);
	float get_mix() const;

	void set_sidechain(const String& p_sidechain);
	String get_sidechain() const;
};

struct AudioEffectCompressorState
{
	Ref<AudioEffect> base;

	float rundb = 0.0f;
	float averatio = 0.0f;
	float runratio = 0.0f;
	float runmax = 0.0f;
	float maxover = 0.0f;
	float gr_meter = 1.0f;
	int current_channel = -1;

	void set_current_channel(int p_channel) { current_channel = p_channel; }

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectCompressorInstance = AudioEffectCompressorState;

// ============================================================================
// 5. Delay
// ============================================================================

struct AudioEffectDelayConfig
{
	enum
	{
		MAX_DELAY_MS = 3000,
		MAX_TAPS = 2
	};

	float dry = 1.0f;

	bool tap_1_active = true;
	float tap_1_delay_ms = 250.0f;
	float tap_1_level = -6.0f;
	float tap_1_pan = 0.2f;

	bool tap_2_active = true;
	float tap_2_delay_ms = 500.0f;
	float tap_2_level = -12.0f;
	float tap_2_pan = -0.4f;

	bool feedback_active = false;
	float feedback_delay_ms = 340.0f;
	float feedback_level = -6.0f;
	float feedback_lowpass = 16000.0f;

	void set_dry(float p_dry);
	float get_dry() const;

	void set_tap1_active(bool p_active);
	bool is_tap1_active() const;

	void set_tap1_delay_ms(float p_delay_ms);
	float get_tap1_delay_ms() const;

	void set_tap1_level_db(float p_level_db);
	float get_tap1_level_db() const;

	void set_tap1_pan(float p_pan);
	float get_tap1_pan() const;

	void set_tap2_active(bool p_active);
	bool is_tap2_active() const;

	void set_tap2_delay_ms(float p_delay_ms);
	float get_tap2_delay_ms() const;

	void set_tap2_level_db(float p_level_db);
	float get_tap2_level_db() const;

	void set_tap2_pan(float p_pan);
	float get_tap2_pan() const;

	void set_feedback_active(bool p_active);
	bool is_feedback_active() const;

	void set_feedback_delay_ms(float p_delay_ms);
	float get_feedback_delay_ms() const;

	void set_feedback_level_db(float p_level_db);
	float get_feedback_level_db() const;

	void set_feedback_lowpass(float p_lowpass);
	float get_feedback_lowpass() const;
};

struct AudioEffectDelayState
{
	Ref<AudioEffect> base;

	Vector<AudioFrame> ring_buffer;
	unsigned int ring_buffer_pos = 0;
	unsigned int ring_buffer_mask = 0;

	Vector<AudioFrame> feedback_buffer;
	unsigned int feedback_buffer_pos = 0;

	AudioFrame h;

	void _process_chunk(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectDelayInstance = AudioEffectDelayState;

// ============================================================================
// 6. Distortion
// ============================================================================

struct AudioEffectDistortionConfig
{
	enum Mode
	{
		MODE_CLIP,
		MODE_ATAN,
		MODE_LOFI,
		MODE_OVERDRIVE,
		MODE_WAVESHAPE,
	};

	Mode mode = MODE_CLIP;
	float pre_gain = 0.0f;
	float post_gain = 0.0f;
	float keep_hf_hz = 16000.0f;
	float drive = 0.0f;

	void set_mode(Mode p_mode);
	Mode get_mode() const;

	void set_pre_gain(float p_pre_gain);
	float get_pre_gain() const;

	void set_keep_hf_hz(float p_keep_hf_hz);
	float get_keep_hf_hz() const;

	void set_drive(float p_drive);
	float get_drive() const;

	void set_post_gain(float p_post_gain);
	float get_post_gain() const;
};

struct AudioEffectDistortionState
{
	Ref<AudioEffect> base;
	float h[2] = {};

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectDistortionInstance = AudioEffectDistortionState;

// ============================================================================
// 7. EQ
// ============================================================================

struct AudioEffectEQConfig
{
	EQ eq;
	Vector<float> gain;
	HashMap<String, int> prop_band_map;
	Vector<String> band_names;

	void set_preset(EQ::Preset p_preset);
	void set_band_gain_db(int p_band, float p_volume);
	float get_band_gain_db(int p_band) const;
	int get_band_count() const;

	AudioEffectEQConfig(EQ::Preset p_preset = EQ::PRESET_6_BANDS);
};

struct AudioEffectEQState
{
	Ref<AudioEffect> base;

	Vector<EQ::BandProcess> bands[2];
	Vector<float> gains;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectEQInstance = AudioEffectEQState;

// ============================================================================
// 8. Filter
// ============================================================================

struct AudioEffectFilterConfig
{
	enum FilterDB
	{
		FILTER_6DB,
		FILTER_12DB,
		FILTER_18DB,
		FILTER_24DB,
	};

	AudioFilterSW::Mode mode = AudioFilterSW::LOWPASS;
	float cutoff = 2000.0f;
	float resonance = 0.5f;
	float gain = 1.0f;
	FilterDB db = FILTER_6DB;

	void set_filter_mode(AudioFilterSW::Mode p_mode);
	AudioFilterSW::Mode get_filter_mode() const;

	void set_cutoff(float p_freq);
	float get_cutoff() const;

	void set_resonance(float p_amount);
	float get_resonance() const;

	void set_gain(float p_amount);
	float get_gain() const;

	void set_db(FilterDB p_db);
	FilterDB get_db() const;

	AudioEffectFilterConfig(AudioFilterSW::Mode p_mode = AudioFilterSW::LOWPASS);
};

struct AudioEffectFilterState
{
	Ref<AudioEffect> base;

	AudioFilterSW filter;
	AudioFilterSW::Processor filter_process[2][4];

	template <int S>
	void _process_filter(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);

	AudioEffectFilterState();
};

using AudioEffectFilterInstance = AudioEffectFilterState;

// ============================================================================
// 9. HardLimiter
// ============================================================================

struct AudioEffectHardLimiterConfig
{
	float pre_gain = 0.0f;
	float ceiling = -0.3f;
	float sustain = 0.02f;
	float release = 0.1f;
	float attack = 0.002f;

	void set_ceiling_db(float p_ceiling);
	float get_ceiling_db() const;

	void set_release(float p_release);
	float get_release() const;

	void set_pre_gain_db(float p_pre_gain);
	float get_pre_gain_db() const;
};

struct AudioEffectHardLimiterState
{
	Ref<AudioEffect> base;

	int sample_cursor = 0;

	float release_factor = 0;
	float attack_factor = 0;
	float gain = 1;
	float gain_target = 1;

	LocalVector<float> sample_buffer_left;
	LocalVector<float> sample_buffer_right;

	int gain_samples_to_store = 0;
	int gain_bucket_cursor = 0;
	int gain_bucket_size = 0;
	LocalVector<float> gain_buckets;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectHardLimiterInstance = AudioEffectHardLimiterState;

// ============================================================================
// 10. Limiter
// ============================================================================

struct AudioEffectLimiterConfig
{
	float threshold = 0.0f;
	float ceiling = -0.1f;
	float soft_clip = 2.0f;
	float soft_clip_ratio = 10.0f;

	void set_threshold_db(float p_threshold);
	float get_threshold_db() const;

	void set_ceiling_db(float p_ceiling);
	float get_ceiling_db() const;

	void set_soft_clip_db(float p_soft_clip);
	float get_soft_clip_db() const;

	void set_soft_clip_ratio(float p_soft_clip);
	float get_soft_clip_ratio() const;
};

struct AudioEffectLimiterState
{
	Ref<AudioEffect> base;
	float mix_volume_db = 0.0f;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectLimiterInstance = AudioEffectLimiterState;

// ============================================================================
// 11. Panner
// ============================================================================

struct AudioEffectPannerConfig
{
	float pan = 0.0f;

	void set_pan(float p_cpanume);
	float get_pan() const;
};

struct AudioEffectPannerState
{
	Ref<AudioEffect> base;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectPannerInstance = AudioEffectPannerState;

// ============================================================================
// 12. Phaser
// ============================================================================

struct AudioEffectPhaserConfig
{
	float range_min = 440.0f;
	float range_max = 1600.0f;
	float rate = 0.5f;
	float feedback = 0.7f;
	float depth = 1.0f;

	void set_range_min_hz(float p_hz);
	float get_range_min_hz() const;

	void set_range_max_hz(float p_hz);
	float get_range_max_hz() const;

	void set_rate_hz(float p_hz);
	float get_rate_hz() const;

	void set_feedback(float p_fbk);
	float get_feedback() const;

	void set_depth(float p_depth);
	float get_depth() const;
};

struct AudioEffectPhaserState
{
	Ref<AudioEffect> base;

	float phase = 0.0f;
	AudioFrame h;

	class AllpassDelay
	{
		float a = 0.0f;
		float h = 0.0f;

	public:
		_ALWAYS_INLINE_ void delay(float d)
		{
			a = (1.f - d) / (1.f + d);
		}

		_ALWAYS_INLINE_ float update(float s)
		{
			float y = s * -a + h;
			h = y * a + s;
			return y;
		}
	};

	AllpassDelay allpass[2][6];

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectPhaserInstance = AudioEffectPhaserState;

// ============================================================================
// 13. PitchShift
// ============================================================================

class SMBPitchShift
{
	enum
	{
		MAX_FRAME_LENGTH = 8192
	};

	float gInFIFO[MAX_FRAME_LENGTH] = {};
	float gOutFIFO[MAX_FRAME_LENGTH] = {};
	float gFFTworksp[2 * MAX_FRAME_LENGTH] = {};
	float gLastPhase[MAX_FRAME_LENGTH / 2 + 1] = {};
	float gSumPhase[MAX_FRAME_LENGTH / 2 + 1] = {};
	float gOutputAccum[2 * MAX_FRAME_LENGTH] = {};
	float gAnaFreq[MAX_FRAME_LENGTH] = {};
	float gAnaMagn[MAX_FRAME_LENGTH] = {};
	float gSynFreq[MAX_FRAME_LENGTH] = {};
	float gSynMagn[MAX_FRAME_LENGTH] = {};
	long gRover = 0;

	void smbFft(float* fftBuffer, long fftFrameSize, long sign);

public:
	void PitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp,
		float sampleRate, float* indata, float* outdata, int stride);
};

struct AudioEffectPitchShiftConfig
{
	enum FFTSize : unsigned int
	{
		FFT_SIZE_256,
		FFT_SIZE_512,
		FFT_SIZE_1024,
		FFT_SIZE_2048,
		FFT_SIZE_4096,
		FFT_SIZE_MAX
	};

	float pitch_scale = 1.0f;
	int oversampling = 4;
	FFTSize fft_size = FFT_SIZE_2048;
	float wet = 0.0f;
	float dry = 0.0f;
	bool filter = false;

	void set_pitch_scale(float p_pitch_scale);
	float get_pitch_scale() const;

	void set_oversampling(int p_oversampling);
	int get_oversampling() const;

	void set_fft_size(FFTSize p_fft_size);
	FFTSize get_fft_size() const;
};

struct AudioEffectPitchShiftState
{
	Ref<AudioEffect> base;

	int fft_size = 0;
	SMBPitchShift shift_l;
	SMBPitchShift shift_r;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectPitchShiftInstance = AudioEffectPitchShiftState;

// ============================================================================
// 14. Record
// ============================================================================

struct AudioEffectRecordState
{
	bool is_recording = false;
	Thread io_thread;

	Vector<AudioFrame> ring_buffer;
	Vector<float> recording_data;

	unsigned int ring_buffer_pos = 0;
	unsigned int ring_buffer_mask = 0;
	unsigned int ring_buffer_read_pos = 0;

	void _io_thread_process();
	void _io_store_buffer();
	static void _thread_callback(void* _instance);
	void _update_buffer();
	static void _update(void* userdata);

	void init();
	void finish();
	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	bool process_silence() const;

	~AudioEffectRecordState() { finish(); }
};

using AudioEffectRecordInstance = AudioEffectRecordState;

struct AudioEffectRecordConfig
{
	enum
	{
		IO_BUFFER_SIZE_MS = 1500
	};

	Ref<AudioEffectInstance> current_instance;
	AudioStreamWAV::Format format = AudioStreamWAV::FORMAT_16_BITS;

	void ensure_thread_stopped();
	void set_recording_active(bool p_record);
	bool is_recording_active() const;
	void set_format(AudioStreamWAV::Format p_format);
	AudioStreamWAV::Format get_format() const;
	Ref<AudioStreamWAV> get_recording() const;

	~AudioEffectRecordConfig() { ensure_thread_stopped(); }
};

// ============================================================================
// 15. Reverb
// ============================================================================

struct AudioEffectReverbConfig
{
	float predelay = 150.0f;
	float predelay_fb = 0.4f;
	float hpf = 0.0f;
	float room_size = 0.8f;
	float damping = 0.5f;
	float spread = 1.0f;
	float dry = 1.0f;
	float wet = 0.5f;

	void set_predelay_msec(float p_msec);
	void set_predelay_feedback(float p_feedback);
	void set_room_size(float p_size);
	void set_damping(float p_damping);
	void set_spread(float p_spread);
	void set_dry(float p_dry);
	void set_wet(float p_wet);
	void set_hpf(float p_hpf);

	float get_predelay_msec() const;
	float get_predelay_feedback() const;
	float get_room_size() const;
	float get_damping() const;
	float get_spread() const;
	float get_dry() const;
	float get_wet() const;
	float get_hpf() const;
};

struct AudioEffectReverbState
{
	Ref<AudioEffect> base;

	float tmp_src[Reverb::INPUT_BUFFER_MAX_SIZE] = {};
	float tmp_dst[Reverb::INPUT_BUFFER_MAX_SIZE] = {};

	Reverb reverb[2];

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	AudioEffectReverbState();
};

using AudioEffectReverbInstance = AudioEffectReverbState;

// ============================================================================
// 16. SpectrumAnalyzer
// ============================================================================

struct AudioEffectSpectrumAnalyzerConfig
{
	enum FFTSize
	{
		FFT_SIZE_256,
		FFT_SIZE_512,
		FFT_SIZE_1024,
		FFT_SIZE_2048,
		FFT_SIZE_4096,
		FFT_SIZE_MAX
	};

	float buffer_length = 2.0f;
	FFTSize fft_size = FFT_SIZE_1024;

	void set_buffer_length(float p_seconds);
	float get_buffer_length() const;

	void set_fft_size(FFTSize p_fft_size);
	FFTSize get_fft_size() const;
};

struct AudioEffectSpectrumAnalyzerState
{
	enum MagnitudeMode
	{
		MAGNITUDE_AVERAGE,
		MAGNITUDE_MAX,
	};

	Ref<AudioEffect> base;

	Vector<Vector<AudioFrame>> fft_history;
	Vector<float> temporal_fft;
	int temporal_fft_pos = 0;
	int fft_size = 0;
	int fft_count = 0;
	int fft_pos = 0;
	float mix_rate = 0.0f;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	Vector2 get_magnitude_for_frequency_range(
		float p_begin, float p_end, MagnitudeMode p_mode = MAGNITUDE_MAX) const;
};

using AudioEffectSpectrumAnalyzerInstance = AudioEffectSpectrumAnalyzerState;

// ============================================================================
// 17. StereoEnhance
// ============================================================================

struct AudioEffectStereoEnhanceConfig
{
	float volume_db = 0.0f;
	float pan_pullout = 1.0f;
	float time_pullout = 0.0f;
	float surround = 0.0f;

	void set_pan_pullout(float p_amount);
	float get_pan_pullout() const;

	void set_time_pullout(float p_amount);
	float get_time_pullout() const;

	void set_surround(float p_amount);
	float get_surround() const;
};

struct AudioEffectStereoEnhanceState
{
	Ref<AudioEffect> base;

	enum
	{
		MAX_DELAY_MS = 50
	};

	float* delay_ringbuff = nullptr;
	unsigned int ringbuff_pos = 0;
	unsigned int ringbuff_mask = 0;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);

	~AudioEffectStereoEnhanceState();
};

using AudioEffectStereoEnhanceInstance = AudioEffectStereoEnhanceState;

// --- End servers/audio/effects/audio_effects.h ---

void AudioEffect::_init_config(Type p_type)
{
	switch (p_type) {
	case EFFECT_AMPLIFY:
		config = memnew(AudioEffectAmplifyConfig);
		break;
	case EFFECT_CAPTURE:
		config = memnew(AudioEffectCaptureConfig);
		break;
	case EFFECT_CHORUS:
		config = memnew(AudioEffectChorusConfig);
		break;
	case EFFECT_COMPRESSOR:
		config = memnew(AudioEffectCompressorConfig);
		break;
	case EFFECT_DELAY:
		config = memnew(AudioEffectDelayConfig);
		break;
	case EFFECT_DISTORTION:
		config = memnew(AudioEffectDistortionConfig);
		break;
	case EFFECT_EQ:
		config = memnew(AudioEffectEQConfig);
		break;
	case EFFECT_FILTER:
		config = memnew(AudioEffectFilterConfig);
		break;
	case EFFECT_HARD_LIMITER:
		config = memnew(AudioEffectHardLimiterConfig);
		break;
	case EFFECT_LIMITER:
		config = memnew(AudioEffectLimiterConfig);
		break;
	case EFFECT_PANNER:
		config = memnew(AudioEffectPannerConfig);
		break;
	case EFFECT_PHASER:
		config = memnew(AudioEffectPhaserConfig);
		break;
	case EFFECT_PITCH_SHIFT:
		config = memnew(AudioEffectPitchShiftConfig);
		break;
	case EFFECT_RECORD:
		config = memnew(AudioEffectRecordConfig);
		break;
	case EFFECT_REVERB:
		config = memnew(AudioEffectReverbConfig);
		break;
	case EFFECT_SPECTRUM_ANALYZER:
		config = memnew(AudioEffectSpectrumAnalyzerConfig);
		break;
	case EFFECT_STEREO_ENHANCE:
		config = memnew(AudioEffectStereoEnhanceConfig);
		break;
	}
}

void AudioEffect::_free_config()
{
	if (!config) {
		return;
	}
	switch (effect_type) {
	case EFFECT_AMPLIFY:
		memdelete(static_cast<AudioEffectAmplifyConfig*>(config));
		break;
	case EFFECT_CAPTURE:
		memdelete(static_cast<AudioEffectCaptureConfig*>(config));
		break;
	case EFFECT_CHORUS:
		memdelete(static_cast<AudioEffectChorusConfig*>(config));
		break;
	case EFFECT_COMPRESSOR:
		memdelete(static_cast<AudioEffectCompressorConfig*>(config));
		break;
	case EFFECT_DELAY:
		memdelete(static_cast<AudioEffectDelayConfig*>(config));
		break;
	case EFFECT_DISTORTION:
		memdelete(static_cast<AudioEffectDistortionConfig*>(config));
		break;
	case EFFECT_EQ:
		memdelete(static_cast<AudioEffectEQConfig*>(config));
		break;
	case EFFECT_FILTER:
		memdelete(static_cast<AudioEffectFilterConfig*>(config));
		break;
	case EFFECT_HARD_LIMITER:
		memdelete(static_cast<AudioEffectHardLimiterConfig*>(config));
		break;
	case EFFECT_LIMITER:
		memdelete(static_cast<AudioEffectLimiterConfig*>(config));
		break;
	case EFFECT_PANNER:
		memdelete(static_cast<AudioEffectPannerConfig*>(config));
		break;
	case EFFECT_PHASER:
		memdelete(static_cast<AudioEffectPhaserConfig*>(config));
		break;
	case EFFECT_PITCH_SHIFT:
		memdelete(static_cast<AudioEffectPitchShiftConfig*>(config));
		break;
	case EFFECT_RECORD:
		memdelete(static_cast<AudioEffectRecordConfig*>(config));
		break;
	case EFFECT_REVERB:
		memdelete(static_cast<AudioEffectReverbConfig*>(config));
		break;
	case EFFECT_SPECTRUM_ANALYZER:
		memdelete(static_cast<AudioEffectSpectrumAnalyzerConfig*>(config));
		break;
	case EFFECT_STEREO_ENHANCE:
		memdelete(static_cast<AudioEffectStereoEnhanceConfig*>(config));
		break;
	}
	config = nullptr;
}

AudioEffect::AudioEffect(Type p_type) : effect_type(p_type)
{
	_init_config(p_type);
}

AudioEffect::~AudioEffect()
{
	_free_config();
}

void AudioEffect::set_effect_type(Type p_type)
{
	if (effect_type == p_type && config != nullptr) {
		return;
	}
	AudioServer::lock();
	_free_config();
	effect_type = p_type;
	_init_config(p_type);
	AudioServer::unlock();
}

Ref<AudioEffectInstance> AudioEffect::instantiate()
{
	switch (effect_type) {
	case EFFECT_AMPLIFY: {
		AudioEffectAmplifyConfig* cfg = get_amplify_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectAmplifyState* state = memnew(AudioEffectAmplifyState);
		state->base = Ref<AudioEffect>(this);
		state->mix_volume_db = cfg->volume_db;
		return AudioEffectInstance::create(state);
	}
	case EFFECT_CAPTURE: {
		AudioEffectCaptureConfig* cfg = get_capture_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		if (!cfg->buffer_initialized) {
			float target_buffer_size = AudioServer::get_mix_rate() * cfg->buffer_length_seconds;
			ERR_FAIL_COND_V(
				target_buffer_size <= 0 || target_buffer_size >= (1 << 27), Ref<AudioEffectInstance>());
			cfg->buffer.resize(Math::nearest_shift((uint32_t)target_buffer_size));
			cfg->buffer_initialized = true;
		}
		cfg->clear_buffer();
		AudioEffectCaptureState* state = memnew(AudioEffectCaptureState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_CHORUS: {
		AudioEffectChorusState* state = memnew(AudioEffectChorusState);
		state->base = Ref<AudioEffect>(this);
		for (int i = 0; i < 4; i++) {
			state->filter_h[i] = AudioFrame(0, 0);
			state->cycles[i] = 0;
		}

		float ring_buffer_max_size = AudioEffectChorusConfig::MAX_DELAY_MS +
									 AudioEffectChorusConfig::MAX_DEPTH_MS +
									 AudioEffectChorusConfig::MAX_WIDTH_MS;
		ring_buffer_max_size *= 2;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->buffer_mask = ringbuff_size - 1;
		state->buffer_pos = 0;
		state->audio_buffer.resize(ringbuff_size);
		for (int i = 0; i < ringbuff_size; i++) {
			state->audio_buffer.write[i] = AudioFrame(0, 0);
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_COMPRESSOR: {
		AudioEffectCompressorState* state = memnew(AudioEffectCompressorState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_DELAY: {
		AudioEffectDelayState* state = memnew(AudioEffectDelayState);
		state->base = Ref<AudioEffect>(this);

		float ring_buffer_max_size = AudioEffectDelayConfig::MAX_DELAY_MS + 100;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->ring_buffer_mask = ringbuff_size - 1;
		state->ring_buffer_pos = 0;
		state->ring_buffer.resize(ringbuff_size);
		state->feedback_buffer.resize(ringbuff_size);
		state->feedback_buffer_pos = 0;
		state->h = AudioFrame(0, 0);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_DISTORTION: {
		AudioEffectDistortionState* state = memnew(AudioEffectDistortionState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_EQ: {
		AudioEffectEQConfig* cfg = get_eq_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectEQState* state = memnew(AudioEffectEQState);
		state->base = Ref<AudioEffect>(this);
		state->gains.resize(cfg->eq.get_band_count());
		for (int i = 0; i < 2; i++) {
			state->bands[i].resize(cfg->eq.get_band_count());
			for (int j = 0; j < state->bands[i].size(); j++) {
				state->bands[i].write[j] = cfg->eq.get_band_processor(j);
			}
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_FILTER: {
		AudioEffectFilterState* state = memnew(AudioEffectFilterState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_HARD_LIMITER: {
		AudioEffectHardLimiterConfig* cfg = get_hard_limiter_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectHardLimiterState* state = memnew(AudioEffectHardLimiterState);
		state->base = Ref<AudioEffect>(this);

		float mix_rate = AudioServer::get_mix_rate();
		for (int i = 0; i < (int)Math::ceil(mix_rate * cfg->attack) + 1; i++) {
			state->sample_buffer_left.push_back(0.0f);
			state->sample_buffer_right.push_back(0.0f);
		}
		state->gain_samples_to_store = (int)Math::ceil(mix_rate * (cfg->attack + cfg->sustain) + 1);
		state->gain_bucket_size = (int)(mix_rate * cfg->attack);
		for (int i = 0; i < state->gain_samples_to_store; i += state->gain_bucket_size) {
			state->gain_buckets.push_back(1.0f);
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_LIMITER: {
		AudioEffectLimiterState* state = memnew(AudioEffectLimiterState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_PANNER: {
		AudioEffectPannerState* state = memnew(AudioEffectPannerState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_PHASER: {
		AudioEffectPhaserState* state = memnew(AudioEffectPhaserState);
		state->base = Ref<AudioEffect>(this);
		state->phase = 0;
		state->h = AudioFrame(0, 0);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_PITCH_SHIFT: {
		AudioEffectPitchShiftConfig* cfg = get_pitch_shift_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectPitchShiftState* state = memnew(AudioEffectPitchShiftState);
		state->base = Ref<AudioEffect>(this);
		static const int fft_sizes[AudioEffectPitchShiftConfig::FFT_SIZE_MAX] = { 256, 512, 1024, 2048, 4096 };
		state->fft_size = fft_sizes[cfg->fft_size];
		return AudioEffectInstance::create(state);
	}
	case EFFECT_RECORD: {
		AudioEffectRecordConfig* cfg = get_record_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectRecordState* state = memnew(AudioEffectRecordState);
		state->is_recording = false;

		float ring_buffer_max_size = AudioEffectRecordConfig::IO_BUFFER_SIZE_MS;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->ring_buffer_mask = ringbuff_size - 1;
		state->ring_buffer_pos = 0;
		state->ring_buffer.resize(ringbuff_size);
		state->ring_buffer_read_pos = 0;

		cfg->ensure_thread_stopped();
		bool is_currently_recording = false;
		if (cfg->current_instance.is_valid()) {
			if (const AudioEffectRecordState* prev = cfg->current_instance->get_state<AudioEffectRecordState>()) {
				is_currently_recording = prev->is_recording;
			}
		}
		if (is_currently_recording) {
			state->init();
		}
		Ref<AudioEffectInstance> ins = AudioEffectInstance::create(state);
		cfg->current_instance = ins;
		return ins;
	}
	case EFFECT_REVERB: {
		AudioEffectReverbState* state = memnew(AudioEffectReverbState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_SPECTRUM_ANALYZER: {
		AudioEffectSpectrumAnalyzerConfig* cfg = get_spectrum_analyzer_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectSpectrumAnalyzerState* state = memnew(AudioEffectSpectrumAnalyzerState);
		state->base = Ref<AudioEffect>(this);
		static const int fft_sizes[AudioEffectSpectrumAnalyzerConfig::FFT_SIZE_MAX] = { 256, 512, 1024, 2048, 4096 };
		state->fft_size = fft_sizes[cfg->fft_size];
		state->mix_rate = AudioServer::get_mix_rate();
		state->fft_count = (cfg->buffer_length / (float(state->fft_size) / state->mix_rate)) + 1;
		state->fft_pos = 0;
		state->fft_history.resize(state->fft_count);
		state->temporal_fft.resize(state->fft_size * 8);
		state->temporal_fft_pos = 0;
		for (int i = 0; i < state->fft_count; i++) {
			state->fft_history.write[i].resize(state->fft_size);
			for (int j = 0; j < state->fft_size; j++) {
				state->fft_history.write[i].write[j] = AudioFrame(0, 0);
			}
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_STEREO_ENHANCE: {
		AudioEffectStereoEnhanceState* state = memnew(AudioEffectStereoEnhanceState);
		state->base = Ref<AudioEffect>(this);

		float ring_buffer_max_size = AudioEffectStereoEnhanceState::MAX_DELAY_MS + 2;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = (int)ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->ringbuff_mask = ringbuff_size - 1;
		state->ringbuff_pos = 0;
		state->delay_ringbuff = memnew_arr(float, ringbuff_size);
		return AudioEffectInstance::create(state);
	}
	}
	return Ref<AudioEffectInstance>();
}

// ============================================================================
// Merged from servers/audio/audio_stream.cpp
// ============================================================================

#include "servers/audio/audio_stream.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"

constexpr uint64_t POLYPHONIC_ID_MASK = 0xFFFFFFFF;
constexpr uint64_t POLYPHONIC_INDEX_SHIFT = 32;

PackedVector2Array AudioStreamPlayback::_mix_audio_bind(float p_rate_scale, int p_frames)
{
	Vector<AudioFrame> frames = mix_audio(p_rate_scale, p_frames);

	PackedVector2Array res;
	res.resize(frames.size());

	Vector2* res_ptrw = res.ptrw();
	for (int i = 0; i < frames.size(); i++) {
		res_ptrw[i] = Vector2(frames[i].left, frames[i].right);
	}

	return res;
}

Vector<AudioFrame> AudioStreamPlayback::mix_audio(float p_rate_scale, int p_frames)
{
	Vector<AudioFrame> res;
	res.resize(p_frames);

	int frames = mix(res.ptrw(), p_rate_scale, p_frames);
	res.resize(frames);

	return res;
}

void AudioStreamPlayback::start_playback(double p_from_pos) { start(p_from_pos); }

void AudioStreamPlayback::stop_playback() { stop(); }

void AudioStreamPlayback::seek_playback(double p_time) { seek(p_time); }

Ref<AudioSamplePlayback> AudioStreamPlayback::get_sample_playback() const { return nullptr; }

AudioStreamPlayback::AudioStreamPlayback() {}

AudioStreamPlayback::~AudioStreamPlayback()
{
	if (get_sample_playback().is_valid() && likely(AudioServer::is_initialized())) {
		AudioServer::stop_sample_playback(get_sample_playback());
	}
}

void AudioStreamPlaybackResampled::begin_resample()
{
	// clear cubic interpolation history
	internal_buffer[0] = AudioFrame(0.0, 0.0);
	internal_buffer[1] = AudioFrame(0.0, 0.0);
	internal_buffer[2] = AudioFrame(0.0, 0.0);
	internal_buffer[3] = AudioFrame(0.0, 0.0);
	// mix buffer
	_mix_internal(internal_buffer + 4, INTERNAL_BUFFER_LEN);
	mix_offset = 0;
}

int AudioStreamPlaybackResampled::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	float target_rate = AudioServer::get_mix_rate();
	float playback_speed_scale = AudioServer::get_playback_speed_scale();

	uint64_t mix_increment = uint64_t(
		((get_stream_sampling_rate() * p_rate_scale * playback_speed_scale) / double(target_rate)) *
		double(FP_LEN));

	int mixed_frames_total = -1;

	int i;
	for (i = 0; i < p_frames; i++) {
		uint32_t idx = CUBIC_INTERP_HISTORY + uint32_t(mix_offset >> FP_BITS);
		// standard cubic interpolation (great quality/performance ratio)
		// this used to be moved to a LUT for greater performance, but nowadays CPU speed is
		// generally faster than memory.
		float mu = (mix_offset & FP_MASK) / float(FP_LEN);
		AudioFrame y0 = internal_buffer[idx - 3];
		AudioFrame y1 = internal_buffer[idx - 2];
		AudioFrame y2 = internal_buffer[idx - 1];
		AudioFrame y3 = internal_buffer[idx - 0];

		if (idx >= internal_buffer_end && mixed_frames_total == -1) {
			// The internal buffer ends somewhere in this range, and we haven't yet recorded the
			// number of good frames we have.
			mixed_frames_total = i;
		}

		float mu2 = mu * mu;
		float h11 = mu2 * (mu - 1);
		float z = mu2 - h11;
		float h01 = z - h11;
		float h10 = mu - z;

		p_buffer[i] = y1 + (y2 - y1) * h01 + ((y2 - y0) * h10 + (y3 - y1) * h11) * 0.5;

		mix_offset += mix_increment;

		while ((mix_offset >> FP_BITS) >= INTERNAL_BUFFER_LEN) {
			internal_buffer[0] = internal_buffer[INTERNAL_BUFFER_LEN + 0];
			internal_buffer[1] = internal_buffer[INTERNAL_BUFFER_LEN + 1];
			internal_buffer[2] = internal_buffer[INTERNAL_BUFFER_LEN + 2];
			internal_buffer[3] = internal_buffer[INTERNAL_BUFFER_LEN + 3];
			int mixed_frames = _mix_internal(internal_buffer + 4, INTERNAL_BUFFER_LEN);
			if (mixed_frames != INTERNAL_BUFFER_LEN) {
				// internal_buffer[mixed_frames] is the first frame of silence.
				internal_buffer_end = mixed_frames;
			}
			else {
				// The internal buffer does not contain the first frame of silence.
				internal_buffer_end = -1;
			}
			mix_offset -= (INTERNAL_BUFFER_LEN << FP_BITS);
		}
	}
	if (mixed_frames_total == -1 && i == p_frames) {
		mixed_frames_total = p_frames;
	}
	return mixed_frames_total;
}

void AudioStream::tag_used(float p_offset)
{
	if (tagged_frame != AudioServer::get_mixed_frames()) {
		offset_count = 0;
		tagged_frame = AudioServer::get_mixed_frames();
	}
	if (offset_count < MAX_TAGGED_OFFSETS) {
		tagged_offsets[offset_count++] = p_offset;
	}
}

uint64_t AudioStream::get_tagged_frame() const { return tagged_frame; }

uint32_t AudioStream::get_tagged_frame_count() const { return offset_count; }

Ref<AudioSample> AudioStream::generate_sample() const
{
	Ref<AudioSample> as = memnew(AudioSample);
	return as;
}

float AudioStream::get_tagged_frame_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, MAX_TAGGED_OFFSETS, 0);
	return tagged_offsets[p_index];
}

double AudioStream::get_bpm() const { return 0.0; }

bool AudioStream::has_loop() const { return false; }

int AudioStream::get_bar_beats() const { return 4; }

int AudioStream::get_beat_count() const { return 0; }

double AudioStream::get_length() const { return 0.0; }

Ref<AudioStreamPlayback> AudioStream::instantiate_playback() { return Ref<AudioStreamPlayback>(); }

String AudioStream::get_stream_name() const { return ""; }

bool AudioStream::is_monophonic() const { return false; }

double AudioStreamPlayback::get_playback_position() const { return 0.0; }

bool AudioStreamPlayback::is_playing() const { return false; }

void AudioStreamPlayback::start(double p_from_pos) {}

void AudioStreamPlayback::stop() {}

int AudioStreamPlaybackResampled::_mix_internal(AudioFrame* p_buffer, int p_frames) { return 0; }

float AudioStreamPlaybackResampled::get_stream_sampling_rate() { return 44100.0; }

float AudioStreamPlaybackResampled::get_stream_sampling_rate() const { return 44100.0; }

int AudioStreamPlayback::get_loop_count() const { return 0; }

void AudioStreamPlayback::seek(double p_time) {}

void AudioStreamPlayback::tag_used_streams() {}

int AudioStreamPlayback::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) { return 0; }

///////////////////////////////////////
// AudioStreamGenerator / AudioStreamGeneratorPlayback

void AudioStreamGenerator::set_source_mode(SourceMode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 2);
	if (source_mode == p_mode) {
		return;
	}
	AudioServer::lock();
	source_mode = p_mode;
	for (AudioStreamGeneratorPlayback* pb : playbacks) {
		pb->stop();
		if (source_mode == SOURCE_BUFFER && pb->buffer.size() == 0) {
			uint32_t target_buffer_size = _get_target_rate() * buffer_len;
			pb->buffer.resize(Math::nearest_shift(target_buffer_size));
			pb->buffer.clear();
		}
	}
	AudioServer::unlock();
}

AudioStreamGenerator::SourceMode AudioStreamGenerator::get_source_mode() const
{
	return source_mode;
}

void AudioStreamGenerator::set_mix_rate(float p_mix_rate) { mix_rate = p_mix_rate; }

float AudioStreamGenerator::get_mix_rate() const { return mix_rate; }

void AudioStreamGenerator::set_mix_rate_mode(
	AudioStreamGenerator::AudioStreamGeneratorMixRate p_mix_rate_mode)
{
	ERR_FAIL_INDEX(p_mix_rate_mode, AudioStreamGeneratorMixRate::MIX_RATE_MAX);
	mix_rate_mode = p_mix_rate_mode;
}

AudioStreamGenerator::AudioStreamGeneratorMixRate AudioStreamGenerator::get_mix_rate_mode() const
{
	return mix_rate_mode;
}

void AudioStreamGenerator::set_buffer_length(float p_seconds) { buffer_len = p_seconds; }

float AudioStreamGenerator::get_buffer_length() const { return buffer_len; }

float AudioStreamGenerator::_get_target_rate() const
{
	switch (mix_rate_mode) {
	case AudioStreamGeneratorMixRate::MIX_RATE_OUTPUT:
		return AudioServer::get_mix_rate();
	case AudioStreamGeneratorMixRate::MIX_RATE_INPUT:
		return AudioServer::get_input_mix_rate();
	default:
		return mix_rate;
	}
}

Ref<AudioStreamPlayback> AudioStreamGenerator::instantiate_playback()
{
	Ref<AudioStreamGeneratorPlayback> playback;
	playback.instantiate();
	playback->generator = Ref<AudioStreamGenerator>(this);
	playbacks.insert(playback.ptr());
	if (source_mode == SOURCE_BUFFER) {
		uint32_t target_buffer_size = _get_target_rate() * buffer_len;
		playback->buffer.resize(Math::nearest_shift(target_buffer_size));
		playback->buffer.clear();
	}
	return playback;
}

double AudioStreamGenerator::get_length() const { return 0; }

bool AudioStreamGenerator::is_monophonic() const { return true; }

bool AudioStreamGeneratorPlayback::push_frame(const Vector2& p_frame)
{
	if (buffer.space_left() < 1) {
		return false;
	}

	AudioFrame f(p_frame);

	buffer.write(&f, 1);
	return true;
}

bool AudioStreamGeneratorPlayback::can_push_buffer(int p_frames) const
{
	return buffer.space_left() >= p_frames;
}

bool AudioStreamGeneratorPlayback::push_buffer(const PackedVector2Array& p_frames)
{
	int to_write = p_frames.size();
	if (buffer.space_left() < to_write) {
		return false;
	}

	const Vector2* r = p_frames.ptr();
	if constexpr (sizeof(real_t) == 4) {
		// write directly
		buffer.write((const AudioFrame*)r, to_write);
	}
	else {
		// convert from double
		AudioFrame buf[2048];
		int ofs = 0;
		while (to_write) {
			int w = MIN(to_write, 2048);
			for (int i = 0; i < w; i++) {
				buf[i] = AudioFrame(r[i + ofs]);
			}
			buffer.write(buf, w);
			ofs += w;
			to_write -= w;
		}
	}
	return true;
}

int AudioStreamGeneratorPlayback::get_frames_available() const { return buffer.space_left(); }

int AudioStreamGeneratorPlayback::get_skips() const { return skips; }

void AudioStreamGeneratorPlayback::clear_buffer()
{
	ERR_FAIL_COND(active);
	buffer.clear();
	mixed = 0;
}

int AudioStreamGeneratorPlayback::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	return AudioStreamPlaybackResampled::mix(p_buffer, p_rate_scale, p_frames);
}

int AudioStreamGeneratorPlayback::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	if (!active || generator.is_null()) {
		return 0;
	}

	if (generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		return 0;
	}

	int read_amount = buffer.data_left();
	if (p_frames < read_amount) {
		read_amount = p_frames;
	}

	buffer.read(p_buffer, read_amount);

	if (read_amount < p_frames) {
		// Fill with zeros as fallback in case of buffer underrun.
		for (int i = read_amount; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		skips++;
	}

	mixed += p_frames / generator->_get_target_rate();
	return p_frames;
}

float AudioStreamGeneratorPlayback::get_stream_sampling_rate()
{
	if (generator.is_null()) {
		return 44100.0f;
	}
	if (generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		return AudioDriverManager::get_active_driver()->get_input_mix_rate();
	}
	return generator->_get_target_rate();
}

void AudioStreamGeneratorPlayback::start(double p_from_pos)
{
	if (generator.is_valid() &&
		generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		if (active) {
			return;
		}
		input_ofs = 0;
		if (AudioServer::set_input_device_active(true) == OK) {
			active = true;
			using_microphone = true;
			begin_resample();
		}
		return;
	}

	if (mixed == 0.0f) {
		begin_resample();
	}
	skips = 0;
	active = true;
	mixed = 0.0f;
}

void AudioStreamGeneratorPlayback::stop()
{
	if (active) {
		if (using_microphone) {
			AudioServer::set_input_device_active(false);
			using_microphone = false;
		}
		active = false;
	}
}

bool AudioStreamGeneratorPlayback::is_playing() const { return active; }

int AudioStreamGeneratorPlayback::get_loop_count() const { return 0; }

double AudioStreamGeneratorPlayback::get_playback_position() const
{
	if (generator.is_valid() &&
		generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		return 0;
	}
	return mixed;
}

void AudioStreamGeneratorPlayback::seek(double p_time)
{
	// No seek possible for generator or microphone input.
}

void AudioStreamGeneratorPlayback::tag_used_streams()
{
	if (generator.is_valid()) {
		generator->tag_used(0);
	}
}

AudioStreamGeneratorPlayback::AudioStreamGeneratorPlayback() {}

AudioStreamGeneratorPlayback::~AudioStreamGeneratorPlayback()
{
	if (generator.is_valid()) {
		generator->playbacks.erase(this);
	}
	stop();
}

///////////////////////////////////////
// AudioStreamPolyphonic / AudioStreamPlaybackPolyphonic

AudioStreamPolyphonic::AudioStreamPolyphonic() {}

void AudioStreamPolyphonic::set_mode(Mode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 2);
	mode = p_mode;
}

AudioStreamPolyphonic::Mode AudioStreamPolyphonic::get_mode() const { return mode; }

void AudioStreamPolyphonic::set_polyphony(int p_voices)
{
	ERR_FAIL_COND(p_voices < 0 || p_voices > 128);
	polyphony = p_voices;
}

int AudioStreamPolyphonic::get_polyphony() const { return polyphony; }

void AudioStreamPolyphonic::add_stream(int p_index, Ref<AudioStream> p_stream, float p_weight)
{
	if (p_index < 0) {
		p_index = audio_stream_pool.size();
	}
	ERR_FAIL_COND(p_index > audio_stream_pool.size());
	PoolEntry entry{ p_stream, p_weight };
	audio_stream_pool.insert(p_index, entry);
}

void AudioStreamPolyphonic::move_stream(int p_index_from, int p_index_to)
{
	ERR_FAIL_INDEX(p_index_from, audio_stream_pool.size());
	if (p_index_to < 0) {
		p_index_to = audio_stream_pool.size() - 1;
	}
	ERR_FAIL_INDEX(p_index_to, audio_stream_pool.size());
	PoolEntry entry = audio_stream_pool[p_index_from];
	audio_stream_pool.remove_at(p_index_from);
	audio_stream_pool.insert(p_index_to, entry);
}

void AudioStreamPolyphonic::remove_stream(int p_index)
{
	ERR_FAIL_INDEX(p_index, audio_stream_pool.size());
	audio_stream_pool.remove_at(p_index);
}

void AudioStreamPolyphonic::set_stream(int p_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_INDEX(p_index, audio_stream_pool.size());
	audio_stream_pool.write[p_index].stream = p_stream;
}

Ref<AudioStream> AudioStreamPolyphonic::get_stream(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, audio_stream_pool.size(), nullptr);
	return audio_stream_pool[p_index].stream;
}

void AudioStreamPolyphonic::set_stream_probability_weight(int p_index, float p_weight)
{
	ERR_FAIL_INDEX(p_index, audio_stream_pool.size());
	audio_stream_pool.write[p_index].weight = p_weight;
}

float AudioStreamPolyphonic::get_stream_probability_weight(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, audio_stream_pool.size(), 0);
	return audio_stream_pool[p_index].weight;
}

void AudioStreamPolyphonic::set_streams_count(int p_count) { audio_stream_pool.resize(p_count); }

int AudioStreamPolyphonic::get_streams_count() const { return audio_stream_pool.size(); }

void AudioStreamPolyphonic::set_random_pitch(float p_pitch)
{
	if (p_pitch < 1) {
		p_pitch = 1;
	}
	random_pitch_scale = p_pitch;
}

float AudioStreamPolyphonic::get_random_pitch() const { return random_pitch_scale; }

void AudioStreamPolyphonic::set_random_pitch_semitones(float p_semitones)
{
	random_pitch_scale = powf(2, p_semitones * 0.08333333f);
}

float AudioStreamPolyphonic::get_random_pitch_semitones() const
{
	return 12.0f * log2f(MAX(1.0f, random_pitch_scale));
}

void AudioStreamPolyphonic::set_random_volume_offset_db(float p_volume_offset_db)
{
	if (p_volume_offset_db < 0) {
		p_volume_offset_db = 0;
	}
	random_volume_offset_db = p_volume_offset_db;
}

float AudioStreamPolyphonic::get_random_volume_offset_db() const
{
	return random_volume_offset_db;
}

void AudioStreamPolyphonic::set_playback_mode(PlaybackMode p_playback_mode)
{
	playback_mode = p_playback_mode;
}

AudioStreamPolyphonic::PlaybackMode AudioStreamPolyphonic::get_playback_mode() const
{
	return playback_mode;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instance_playback_random()
{
	Ref<AudioStreamPlaybackPolyphonic> playback;
	playback.instantiate();
	playback->mode = MODE_RANDOMIZER;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playbacks.insert(playback.ptr());

	double total_weight = 0;
	Vector<PoolEntry> local_pool;
	for (const PoolEntry& entry : audio_stream_pool) {
		if (entry.stream.is_valid() && entry.weight > 0) {
			local_pool.push_back(entry);
			total_weight += entry.weight;
		}
	}
	if (local_pool.is_empty()) {
		return playback;
	}
	double chosen_cumulative_weight = Math::random(0.0, total_weight);
	double cumulative_weight = 0;
	for (PoolEntry& entry : local_pool) {
		cumulative_weight += entry.weight;
		if (cumulative_weight > chosen_cumulative_weight) {
			playback->randomizer_playback = entry.stream->instantiate_playback();
			last_playback = entry.stream;
			break;
		}
	}
	if (playback->randomizer_playback.is_null()) {
		// This indicates a floating point error. Take the last element.
		last_playback = local_pool[local_pool.size() - 1].stream;
		playback->randomizer_playback =
			local_pool.write[local_pool.size() - 1].stream->instantiate_playback();
	}
	return playback;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instance_playback_no_repeats()
{
	Ref<AudioStreamPlaybackPolyphonic> playback;

	double total_weight = 0;
	Vector<PoolEntry> local_pool;
	for (const PoolEntry& entry : audio_stream_pool) {
		if (entry.stream == last_playback) {
			continue;
		}
		if (entry.stream.is_valid() && entry.weight > 0) {
			local_pool.push_back(entry);
			total_weight += entry.weight;
		}
	}
	if (local_pool.is_empty()) {
		// There is only one sound to choose from.
		// Always play a random sound while allowing repeats (which always plays the same sound).
		return instance_playback_random();
	}

	playback.instantiate();
	playback->mode = MODE_RANDOMIZER;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playbacks.insert(playback.ptr());

	double chosen_cumulative_weight = Math::random(0.0, total_weight);
	double cumulative_weight = 0;
	for (PoolEntry& entry : local_pool) {
		cumulative_weight += entry.weight;
		if (cumulative_weight > chosen_cumulative_weight) {
			last_playback = entry.stream;
			playback->randomizer_playback = entry.stream->instantiate_playback();
			break;
		}
	}
	if (playback->randomizer_playback.is_null()) {
		// This indicates a floating point error. Take the last element.
		last_playback = local_pool[local_pool.size() - 1].stream;
		playback->randomizer_playback =
			local_pool.write[local_pool.size() - 1].stream->instantiate_playback();
	}
	return playback;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instance_playback_sequential()
{
	Ref<AudioStreamPlaybackPolyphonic> playback;
	playback.instantiate();
	playback->mode = MODE_RANDOMIZER;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playbacks.insert(playback.ptr());

	Vector<Ref<AudioStream>> local_pool;
	for (const PoolEntry& entry : audio_stream_pool) {
		if (entry.stream.is_null()) {
			continue;
		}
		if (local_pool.has(entry.stream)) {
			WARN_PRINT("Duplicate stream in sequential playback pool");
			continue;
		}
		local_pool.push_back(entry.stream);
	}
	if (local_pool.is_empty()) {
		return playback;
	}
	bool found_last_stream = false;
	for (Ref<AudioStream>& entry : local_pool) {
		if (found_last_stream) {
			last_playback = entry;
			playback->randomizer_playback = entry->instantiate_playback();
			break;
		}
		if (entry == last_playback) {
			found_last_stream = true;
		}
	}
	if (playback->randomizer_playback.is_null()) {
		// Wrap around
		last_playback = local_pool[0];
		playback->randomizer_playback = local_pool.write[0]->instantiate_playback();
	}
	return playback;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instantiate_playback()
{
	if (mode == MODE_RANDOMIZER) {
		switch (playback_mode) {
		case PLAYBACK_RANDOM:
			return instance_playback_random();
		case PLAYBACK_RANDOM_NO_REPEATS:
			return instance_playback_no_repeats();
		case PLAYBACK_SEQUENTIAL:
			return instance_playback_sequential();
		default:
			ERR_FAIL_V_MSG(nullptr, "Unhandled playback mode.");
		}
	}

	Ref<AudioStreamPlaybackPolyphonic> playback;
	playback.instantiate();
	playback->mode = MODE_POLYPHONIC;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playback->streams.resize(polyphony);
	return playback;
}

double AudioStreamPolyphonic::get_length() const
{
	if (mode == MODE_RANDOMIZER) {
		if (!last_playback.is_valid()) {
			return 0;
		}
		return last_playback->get_length();
	}
	return 0.0;
}

bool AudioStreamPolyphonic::is_monophonic() const
{
	if (mode == MODE_RANDOMIZER) {
		for (const PoolEntry& entry : audio_stream_pool) {
			if (entry.stream.is_valid() && entry.stream->is_monophonic()) {
				return true;
			}
		}
		return false;
	}
	return true; // Avoids stream players instantiating more than one polyphonic playback.
}

void AudioStreamPlaybackPolyphonic::start(double p_from_pos)
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		randomizer_playing = randomizer_playback;
		if (parent_stream.is_valid()) {
			// GH-10238 : Pitch_scale is multiplicative, so picking a random number for it without
			// log conversion will bias it towards higher pitches (0.5 is down one octave, 2.0 is up
			// one octave).
			float range_from = Math::log(1.0f / parent_stream->random_pitch_scale);
			float range_to = Math::log(parent_stream->random_pitch_scale);

			pitch_scale = Math::exp(range_from + Math::randf() * (range_to - range_from));

			float vol_from = -parent_stream->random_volume_offset_db;
			float vol_to = parent_stream->random_volume_offset_db;

			float volume_offset_db = vol_from + Math::randf() * (vol_to - vol_from);
			volume_scale = Math::db_to_linear(volume_offset_db);
		}
		else {
			pitch_scale = 1.0f;
			volume_scale = 1.0f;
		}

		if (randomizer_playing.is_valid()) {
			randomizer_playing->start(p_from_pos);
		}
		return;
	}

	if (active) {
		stop();
	}

	active = true;
}

void AudioStreamPlaybackPolyphonic::stop()
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			randomizer_playing->stop();
		}
		return;
	}

	if (!active) {
		return;
	}

	bool locked = false;
	for (Stream& s : streams) {
		if (s.active.is_set()) {
			// Need locking because something may still be mixing.
			if (!locked) {
				locked = true;
				AudioServer::lock();
			}
		}
		s.active.clear();
		s.finish_request.clear();
		s.stream_playback.unref();
		s.stream.unref();
	}
	if (locked) {
		AudioServer::unlock();
	}

	active = false;
}

bool AudioStreamPlaybackPolyphonic::is_playing() const
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			return randomizer_playing->is_playing();
		}
		return false;
	}
	return active;
}

int AudioStreamPlaybackPolyphonic::get_loop_count() const
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			return randomizer_playing->get_loop_count();
		}
		return 0;
	}
	return 0;
}

double AudioStreamPlaybackPolyphonic::get_playback_position() const
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			return randomizer_playing->get_playback_position();
		}
		return 0;
	}
	return 0;
}

void AudioStreamPlaybackPolyphonic::seek(double p_time)
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			randomizer_playing->seek(p_time);
		}
		return;
	}
	// Ignored in polyphonic mode.
}

void AudioStreamPlaybackPolyphonic::tag_used_streams()
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		Ref<AudioStreamPlayback> p = randomizer_playing; // Thread safety
		if (p.is_valid()) {
			p->tag_used_streams();
		}
		if (parent_stream.is_valid()) {
			parent_stream->tag_used(0);
		}
		return;
	}

	for (Stream& s : streams) {
		if (s.active.is_set()) {
			s.stream_playback->tag_used_streams();
		}
	}
}

int AudioStreamPlaybackPolyphonic::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			int mixed_samples =
				randomizer_playing->mix(p_buffer, p_rate_scale * pitch_scale, p_frames);
			for (int samp = 0; samp < mixed_samples; samp++) {
				p_buffer[samp] *= volume_scale;
			}
			return mixed_samples;
		}
		else {
			for (int i = 0; i < p_frames; i++) {
				p_buffer[i] = AudioFrame(0, 0);
			}
			return p_frames;
		}
	}

	if (!active) {
		return 0;
	}

	// Pre-clear buffer.
	for (int i = 0; i < p_frames; i++) {
		p_buffer[i] = AudioFrame(0, 0);
	}

	for (Stream& s : streams) {
		if (!s.active.is_set()) {
			continue;
		}

		if (s.stream_playback->get_is_sample()) {
			if (s.finish_request.is_set()) {
				s.active.clear();
				AudioServer::stop_sample_playback(s.stream_playback->get_sample_playback());
			}
			continue;
		}

		float volume_db = s.volume_db; // Copy because it can be overridden at any time.
		float next_volume = Math::db_to_linear(volume_db);
		s.prev_volume_db = volume_db;

		if (s.finish_request.is_set()) {
			if (s.pending_play.is_set()) {
				// Did not get the chance to play, was finalized too soon.
				s.active.clear();
				continue;
			}
			next_volume = 0;
		}

		if (s.pending_play.is_set()) {
			s.stream_playback->start(s.play_offset);
			s.pending_play.clear();
		}
		float prev_volume = Math::db_to_linear(s.prev_volume_db);

		float volume_inc = (next_volume - prev_volume) / float(p_frames);

		int todo = p_frames;
		int offset = 0;
		float volume = prev_volume;

		bool stream_done = false;

		while (todo) {
			int to_mix = MIN(todo, int(INTERNAL_BUFFER_LEN));
			int mixed =
				s.stream_playback->mix(internal_buffer, p_rate_scale * s.pitch_scale, to_mix);

			for (int i = 0; i < to_mix; i++) {
				p_buffer[offset + i] += internal_buffer[i] * volume;
				volume += volume_inc;
			}

			if (mixed < to_mix) {
				// Stream is done.
				s.active.clear();
				stream_done = true;
				break;
			}

			todo -= to_mix;
			offset += to_mix;
		}

		if (stream_done) {
			continue;
		}

		if (s.finish_request.is_set()) {
			s.active.clear();
		}
	}

	return p_frames;
}

AudioStreamPlaybackPolyphonic::ID AudioStreamPlaybackPolyphonic::play_stream(
	const Ref<AudioStream>& p_stream, float p_from_offset, float p_volume_db, float p_pitch_scale,
	AudioServer::PlaybackType p_playback_type, const String& p_bus)
{
	ERR_FAIL_COND_V(p_stream.is_null(), INVALID_ID);

	AudioServer::PlaybackType pb_type =
		p_playback_type == AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT
			? AudioServer::get_default_playback_type()
			: p_playback_type;

	for (uint32_t i = 0; i < streams.size(); i++) {
		if (streams[i].active.is_set() && streams[i].stream_playback->get_is_sample()) {
			Ref<AudioSamplePlayback> active_sample_playback =
				streams[i].stream_playback->get_sample_playback();
			if (active_sample_playback.is_null() ||
				!AudioServer::is_sample_playback_active(active_sample_playback)) {
				streams[i].active.clear();
			}
		}

		if (!streams[i].active.is_set()) {
			// Can use this stream, as it's not active.
			streams[i].stream = p_stream;
			streams[i].stream_playback = streams[i].stream->instantiate_playback();
			streams[i].play_offset = p_from_offset;
			streams[i].volume_db = p_volume_db;
			streams[i].prev_volume_db = p_volume_db;
			streams[i].pitch_scale = p_pitch_scale;
			streams[i].id = id_counter++;
			streams[i].finish_request.clear();
			streams[i].pending_play.set();
			streams[i].active.set();

			// Sample playback.
			if (pb_type == AudioServer::PlaybackType::PLAYBACK_TYPE_SAMPLE &&
				p_stream->can_be_sampled()) {
				streams[i].stream_playback->set_is_sample(true);
				if (!AudioServer::is_stream_registered_as_sample(p_stream)) {
					AudioServer::register_stream_as_sample(p_stream);
				}
				float linear_volume = Math::db_to_linear(p_volume_db);
				Ref<AudioSamplePlayback> sp;
				sp.instantiate();
				sp->stream = streams[i].stream;
				sp->offset = p_from_offset;
				sp->volume_vector.resize(4);
				sp->volume_vector.write[0] = AudioFrame(linear_volume, linear_volume);
				sp->volume_vector.write[1] = AudioFrame(linear_volume, /* LFE= */ 1.0f);
				sp->volume_vector.write[2] = AudioFrame(linear_volume, linear_volume);
				sp->volume_vector.write[3] = AudioFrame(linear_volume, linear_volume);
				sp->bus = p_bus;

				if (streams[i].stream_playback->get_sample_playback().is_valid()) {
					AudioServer::stop_playback_stream(streams[i].stream_playback);
				}

				streams[i].stream_playback->set_sample_playback(sp);
				AudioServer::start_sample_playback(sp);
			}

			return (ID(i) << POLYPHONIC_INDEX_SHIFT) | ID(streams[i].id);
		}
	}

	return INVALID_ID;
}

AudioStreamPlaybackPolyphonic::Stream* AudioStreamPlaybackPolyphonic::_find_stream(int64_t p_id)
{
	uint32_t index = static_cast<uint64_t>(p_id) >> POLYPHONIC_INDEX_SHIFT;
	if (index >= streams.size()) {
		return nullptr;
	}
	if (!streams[index].active.is_set()) {
		return nullptr; // Not active, no longer exists.
	}
	int64_t id = static_cast<uint64_t>(p_id) & POLYPHONIC_ID_MASK;
	if (streams[index].id != id) {
		return nullptr;
	}
	return &streams[index];
}

const AudioStreamPlaybackPolyphonic::Stream* AudioStreamPlaybackPolyphonic::_find_stream(
	int64_t p_id) const
{
	uint32_t index = static_cast<uint64_t>(p_id) >> POLYPHONIC_INDEX_SHIFT;
	if (index >= streams.size()) {
		return nullptr;
	}
	if (!streams[index].active.is_set()) {
		return nullptr; // Not active, no longer exists.
	}
	int64_t id = static_cast<uint64_t>(p_id) & POLYPHONIC_ID_MASK;
	if (streams[index].id != id) {
		return nullptr;
	}
	return &streams[index];
}

void AudioStreamPlaybackPolyphonic::set_stream_volume(ID p_stream_id, float p_volume_db)
{
	Stream* s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->volume_db = p_volume_db;
}

void AudioStreamPlaybackPolyphonic::set_stream_pitch_scale(ID p_stream_id, float p_pitch_scale)
{
	Stream* s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->pitch_scale = p_pitch_scale;
}

bool AudioStreamPlaybackPolyphonic::is_stream_playing(ID p_stream_id) const
{
	return _find_stream(p_stream_id) != nullptr;
}

void AudioStreamPlaybackPolyphonic::stop_stream(ID p_stream_id)
{
	Stream* s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->finish_request.set();
}

void AudioStreamPlaybackPolyphonic::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackPolyphonic::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackPolyphonic::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackPolyphonic::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

AudioStreamPlaybackPolyphonic::AudioStreamPlaybackPolyphonic() {}

AudioStreamPlaybackPolyphonic::~AudioStreamPlaybackPolyphonic()
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER && parent_stream.is_valid()) {
		parent_stream->playbacks.erase(this);
	}
}

// ============================================================================
// Merged from servers/audio/effects/audio_effects.cpp
// ============================================================================


#include "core/io/marshalls.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"
#include "servers/audio/audio_server.h"

#include <cmath>
#include <cstring>

// ============================================================================
// 1. Amplify
// ============================================================================

void AudioEffectAmplifyState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectAmplifyConfig* cfg = base.is_valid() ? base->get_amplify_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float volume_db = cfg->volume_db;
	float vol = Math::db_to_linear(mix_volume_db);
	float vol_inc = (Math::db_to_linear(volume_db) - vol) / float(p_frame_count);

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i] * vol;
		vol += vol_inc;
	}
	mix_volume_db = volume_db;
}

void AudioEffectAmplifyConfig::set_volume_db(float p_volume) { volume_db = p_volume; }

float AudioEffectAmplifyConfig::get_volume_db() const { return volume_db; }

void AudioEffectAmplifyConfig::set_volume_linear(float p_volume)
{
	set_volume_db(Math::linear_to_db(p_volume));
}

float AudioEffectAmplifyConfig::get_volume_linear() const
{
	return Math::db_to_linear(get_volume_db());
}

// ============================================================================
// 2. Capture
// ============================================================================

bool AudioEffectCaptureConfig::can_get_buffer(int p_frames) const
{
	return buffer.data_left() >= p_frames;
}

PackedVector2Array AudioEffectCaptureConfig::get_buffer(int p_frames)
{
	ERR_FAIL_COND_V(!buffer_initialized, PackedVector2Array());
	ERR_FAIL_INDEX_V(p_frames, buffer.size(), PackedVector2Array());
	int data_left = buffer.data_left();
	if (data_left < p_frames || p_frames == 0) {
		return PackedVector2Array();
	}

	PackedVector2Array ret;
	ret.resize(p_frames);

	Vector<AudioFrame> streaming_data;
	streaming_data.resize(p_frames);
	buffer.read(streaming_data.ptrw(), p_frames);
	for (int32_t i = 0; i < p_frames; i++) {
		ret.write[i] = Vector2(streaming_data[i].left, streaming_data[i].right);
	}
	return ret;
}

void AudioEffectCaptureConfig::clear_buffer()
{
	const int32_t data_left = buffer.data_left();
	buffer.advance_read(data_left);
}

void AudioEffectCaptureConfig::set_buffer_length(float p_buffer_length_seconds)
{
	buffer_length_seconds = p_buffer_length_seconds;
}

float AudioEffectCaptureConfig::get_buffer_length() const { return buffer_length_seconds; }

int AudioEffectCaptureConfig::get_frames_available() const
{
	ERR_FAIL_COND_V(!buffer_initialized, 0);
	return buffer.data_left();
}

int64_t AudioEffectCaptureConfig::get_discarded_frames() const { return discarded_frames.get(); }

int AudioEffectCaptureConfig::get_buffer_length_frames() const
{
	ERR_FAIL_COND_V(!buffer_initialized, 0);
	return buffer.size();
}

int64_t AudioEffectCaptureConfig::get_pushed_frames() const { return pushed_frames.get(); }

void AudioEffectCaptureState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	AudioEffectCaptureConfig* cfg = base.is_valid() ? base->get_capture_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	RingBuffer<AudioFrame>& buffer = cfg->buffer;

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
	}

	if (buffer.space_left() >= p_frame_count) {
		int32_t ret = buffer.write(p_src_frames, p_frame_count);
		ERR_FAIL_COND_MSG(ret != p_frame_count,
			"Failed to add data to effect capture ring buffer despite sufficient space.");
		cfg->pushed_frames.add(p_frame_count);
	}
	else {
		cfg->discarded_frames.add(p_frame_count);
	}
}

bool AudioEffectCaptureState::process_silence() const { return true; }

// ============================================================================
// 3. Chorus
// ============================================================================

void AudioEffectChorusState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	int todo = p_frame_count;

	while (todo) {
		int to_mix = MIN(todo, 256);

		_process_chunk(p_src_frames, p_dst_frames, to_mix);

		p_src_frames += to_mix;
		p_dst_frames += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectChorusState::_process_chunk(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectChorusConfig* cfg = base.is_valid() ? base->get_chorus_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	for (int i = 0; i < p_frame_count; i++) {
		audio_buffer.write[(buffer_pos + i) & buffer_mask] = p_src_frames[i];
		p_dst_frames[i] = p_src_frames[i] * cfg->dry;
	}

	float mix_rate = AudioServer::get_mix_rate();

	for (int vc = 0; vc < cfg->voice_count; vc++) {
		const AudioEffectChorusConfig::Voice& v = cfg->voice[vc];

		double time_to_mix = (float)p_frame_count / mix_rate;
		double cycles_to_mix = time_to_mix * v.rate;

		unsigned int local_rb_pos = buffer_pos;
		AudioFrame* dst_buff = p_dst_frames;
		AudioFrame* rb_buff = audio_buffer.ptrw();

		double delay_msec = v.delay;
		unsigned int delay_frames = Math::fast_ftoi((delay_msec / 1000.0) * mix_rate);
		float max_depth_frames = (v.depth / 1000.0) * mix_rate;

		uint64_t local_cycles = cycles[vc];
		uint64_t increment = std::rint(
			cycles_to_mix / (double)p_frame_count * (double)(1 << AudioEffectChorusConfig::CYCLES_FRAC));

		if ((((unsigned int)max_depth_frames) + 10) > delay_frames) {
			delay_frames += (int)max_depth_frames - delay_frames;
			delay_frames += 10;
		}

		if (v.cutoff == 0) {
			continue;
		}
		float auxlp = std::exp(-Math::TAU * v.cutoff / mix_rate);
		float c1 = 1.0 - auxlp;
		float c2 = auxlp;
		AudioFrame h = filter_h[vc];
		if (v.cutoff >= AudioEffectChorusConfig::MS_CUTOFF_MAX) {
			c1 = 1.0;
			c2 = 0.0;
		}

		AudioFrame vol_modifier = AudioFrame(cfg->wet, cfg->wet) * Math::db_to_linear(v.level);
		vol_modifier.left *= CLAMP(1.0 - v.pan, 0, 1);
		vol_modifier.right *= CLAMP(1.0 + v.pan, 0, 1);

		for (int i = 0; i < p_frame_count; i++) {
			float phase = (float)(local_cycles & AudioEffectChorusConfig::CYCLES_MASK) /
						  (float)(1 << AudioEffectChorusConfig::CYCLES_FRAC);

			float wave_delay = std::sin(phase * Math::TAU) * max_depth_frames;

			int wave_delay_frames = std::rint(std::floor(wave_delay));
			float wave_delay_frac = wave_delay - (float)wave_delay_frames;

			unsigned int rb_source = local_rb_pos;
			rb_source -= delay_frames;
			rb_source -= wave_delay_frames;

			AudioFrame val = rb_buff[rb_source & buffer_mask];
			AudioFrame val_next = rb_buff[(rb_source - 1) & buffer_mask];

			val += (val_next - val) * wave_delay_frac;

			val = val * c1 + h * c2;
			h = val;

			dst_buff[i] += val * vol_modifier;

			local_cycles += increment;
			local_rb_pos++;
		}

		filter_h[vc] = h;
		cycles[vc] +=
			Math::fast_ftoi(cycles_to_mix * (double)(1 << AudioEffectChorusConfig::CYCLES_FRAC));
	}

	buffer_pos += p_frame_count;
}

void AudioEffectChorusConfig::set_voice_count(int p_voices)
{
	ERR_FAIL_COND(p_voices < 1 || p_voices > MAX_VOICES);
	voice_count = p_voices;
}

int AudioEffectChorusConfig::get_voice_count() const { return voice_count; }

void AudioEffectChorusConfig::set_voice_delay_ms(int p_voice, float p_delay_ms)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].delay = p_delay_ms;
}

float AudioEffectChorusConfig::get_voice_delay_ms(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].delay;
}

void AudioEffectChorusConfig::set_voice_rate_hz(int p_voice, float p_rate_hz)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].rate = p_rate_hz;
}

float AudioEffectChorusConfig::get_voice_rate_hz(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].rate;
}

void AudioEffectChorusConfig::set_voice_depth_ms(int p_voice, float p_depth_ms)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].depth = p_depth_ms;
}

float AudioEffectChorusConfig::get_voice_depth_ms(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].depth;
}

void AudioEffectChorusConfig::set_voice_level_db(int p_voice, float p_level_db)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].level = p_level_db;
}

float AudioEffectChorusConfig::get_voice_level_db(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].level;
}

void AudioEffectChorusConfig::set_voice_cutoff_hz(int p_voice, float p_cutoff_hz)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].cutoff = MAX(p_cutoff_hz, 1.0f);
}

float AudioEffectChorusConfig::get_voice_cutoff_hz(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].cutoff;
}

void AudioEffectChorusConfig::set_voice_pan(int p_voice, float p_pan)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].pan = p_pan;
}

float AudioEffectChorusConfig::get_voice_pan(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].pan;
}

void AudioEffectChorusConfig::set_wet(float p_amount) { wet = p_amount; }

float AudioEffectChorusConfig::get_wet() const { return wet; }

void AudioEffectChorusConfig::set_dry(float p_amount) { dry = p_amount; }

float AudioEffectChorusConfig::get_dry() const { return dry; }

AudioEffectChorusConfig::AudioEffectChorusConfig()
{
	voice_count = 2;
	voice[0].delay = 15.0f;
	voice[1].delay = 20.0f;
	voice[0].rate = 0.8f;
	voice[1].rate = 1.2f;
	voice[0].depth = 2.0f;
	voice[1].depth = 3.0f;
	voice[0].cutoff = 8000.0f;
	voice[1].cutoff = 8000.0f;
	voice[0].pan = -0.5f;
	voice[1].pan = 0.5f;
	wet = 0.5f;
	dry = 1.0f;
}

// ============================================================================
// 4. Compressor
// ============================================================================

void AudioEffectCompressorState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectCompressorConfig* cfg = base.is_valid() ? base->get_compressor_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float threshold = Math::db_to_linear(cfg->threshold);
	float sample_rate = AudioServer::get_mix_rate();

	float ratatcoef = std::exp(-1 / (0.00001f * sample_rate));
	float ratrelcoef = std::exp(-1 / (0.5f * sample_rate));
	float attime = cfg->attack_us / 1000000.0f;
	float reltime = cfg->release_ms / 1000.0f;
	float atcoef = std::exp(-1 / (attime * sample_rate));
	float relcoef = std::exp(-1 / (reltime * sample_rate));

	float makeup = Math::db_to_linear(cfg->gain);

	float mix = cfg->mix;
	float gr_meter_decay = std::exp(1 / (1 * sample_rate));

	const AudioFrame* src = p_src_frames;

	if (cfg->sidechain != String() && current_channel != -1) {
		int bus = AudioServer::thread_find_bus_index(cfg->sidechain);
		if (bus >= 0) {
			src = AudioServer::thread_get_channel_mix_buffer(bus, current_channel);
		}
	}

	for (int i = 0; i < p_frame_count; i++) {
		AudioFrame s = src[i];
		s.left = Math::abs(s.left);
		s.right = Math::abs(s.right);

		float peak = MAX(s.left, s.right);
		float overdb = 2.08136898f * Math::linear_to_db(peak / threshold);

		if (overdb < 0.0f) {
			overdb = 0.0f;
		}

		if (overdb - rundb > 5) {
			averatio = 4;
		}

		if (overdb > rundb) {
			rundb = overdb + atcoef * (rundb - overdb);
			runratio = averatio + ratatcoef * (runratio - averatio);
		}
		else {
			rundb = overdb + relcoef * (rundb - overdb);
			runratio = averatio + ratrelcoef * (runratio - averatio);
		}

		overdb = rundb;
		averatio = runratio;

		float cratio = cfg->ratio;
		float gr = -overdb * (cratio - 1) / cratio;
		float grv = Math::db_to_linear(gr);

		runmax = maxover + relcoef * (runmax - maxover);
		maxover = runmax;

		if (grv < gr_meter) {
			gr_meter = grv;
		}
		else {
			gr_meter *= gr_meter_decay;
			if (gr_meter > 1) {
				gr_meter = 1;
			}
		}

		p_dst_frames[i] = p_src_frames[i] * grv * makeup * mix + p_src_frames[i] * (1.0f - mix);
	}
}

void AudioEffectCompressorConfig::set_threshold(float p_threshold) { threshold = p_threshold; }

float AudioEffectCompressorConfig::get_threshold() const { return threshold; }

void AudioEffectCompressorConfig::set_ratio(float p_ratio) { ratio = p_ratio; }

float AudioEffectCompressorConfig::get_ratio() const { return ratio; }

void AudioEffectCompressorConfig::set_gain(float p_gain) { gain = p_gain; }

float AudioEffectCompressorConfig::get_gain() const { return gain; }

void AudioEffectCompressorConfig::set_attack_us(float p_attack_us) { attack_us = p_attack_us; }

float AudioEffectCompressorConfig::get_attack_us() const { return attack_us; }

void AudioEffectCompressorConfig::set_release_ms(float p_release_ms) { release_ms = p_release_ms; }

float AudioEffectCompressorConfig::get_release_ms() const { return release_ms; }

void AudioEffectCompressorConfig::set_mix(float p_mix) { mix = p_mix; }

float AudioEffectCompressorConfig::get_mix() const { return mix; }

void AudioEffectCompressorConfig::set_sidechain(const String& p_sidechain)
{
	AudioServer::lock();
	sidechain = p_sidechain;
	AudioServer::unlock();
}

String AudioEffectCompressorConfig::get_sidechain() const { return sidechain; }

// ============================================================================
// 5. Delay
// ============================================================================

void AudioEffectDelayState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	int todo = p_frame_count;

	while (todo) {
		int to_mix = MIN(todo, 256);

		_process_chunk(p_src_frames, p_dst_frames, to_mix);

		p_src_frames += to_mix;
		p_dst_frames += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectDelayState::_process_chunk(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectDelayConfig* cfg = base.is_valid() ? base->get_delay_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float main_level_f = cfg->dry;
	float mix_rate = AudioServer::get_mix_rate();

	float tap_1_level_f = cfg->tap_1_active ? Math::db_to_linear(cfg->tap_1_level) : 0.0f;
	int tap_1_delay_frames = int((cfg->tap_1_delay_ms / 1000.0f) * mix_rate);

	float tap_2_level_f = cfg->tap_2_active ? Math::db_to_linear(cfg->tap_2_level) : 0.0f;
	int tap_2_delay_frames = int((cfg->tap_2_delay_ms / 1000.0f) * mix_rate);

	float feedback_level_f = cfg->feedback_active ? Math::db_to_linear(cfg->feedback_level) : 0.0f;
	unsigned int feedback_delay_frames = int((cfg->feedback_delay_ms / 1000.0f) * mix_rate);

	AudioFrame tap1_vol = AudioFrame(tap_1_level_f, tap_1_level_f);
	tap1_vol.left *= CLAMP(1.0f - cfg->tap_1_pan, 0.0f, 1.0f);
	tap1_vol.right *= CLAMP(1.0f + cfg->tap_1_pan, 0.0f, 1.0f);

	AudioFrame tap2_vol = AudioFrame(tap_2_level_f, tap_2_level_f);
	tap2_vol.left *= CLAMP(1.0f - cfg->tap_2_pan, 0.0f, 1.0f);
	tap2_vol.right *= CLAMP(1.0f + cfg->tap_2_pan, 0.0f, 1.0f);

	float lpf_c = std::exp(-Math::TAU * cfg->feedback_lowpass / mix_rate);
	float lpf_ic = 1.0f - lpf_c;

	const AudioFrame* src = p_src_frames;
	AudioFrame* dst = p_dst_frames;
	AudioFrame* rb_buf = ring_buffer.ptrw();
	AudioFrame* fb_buf = feedback_buffer.ptrw();

	for (int i = 0; i < p_frame_count; i++) {
		rb_buf[ring_buffer_pos & ring_buffer_mask] = src[i];

		AudioFrame main_val = src[i] * main_level_f;
		AudioFrame tap_1_val =
			rb_buf[(ring_buffer_pos - tap_1_delay_frames) & ring_buffer_mask] * tap1_vol;
		AudioFrame tap_2_val =
			rb_buf[(ring_buffer_pos - tap_2_delay_frames) & ring_buffer_mask] * tap2_vol;

		AudioFrame out = main_val + tap_1_val + tap_2_val;
		out += fb_buf[feedback_buffer_pos];

		AudioFrame fb_in = out * feedback_level_f * lpf_ic + h * lpf_c;
		fb_in.undenormalize();

		h = fb_in;
		fb_buf[feedback_buffer_pos] = fb_in;

		dst[i] = out;

		ring_buffer_pos++;

		if ((++feedback_buffer_pos) >= feedback_delay_frames) {
			feedback_buffer_pos = 0;
		}
	}
}

void AudioEffectDelayConfig::set_dry(float p_dry) { dry = p_dry; }

float AudioEffectDelayConfig::get_dry() const { return dry; }

void AudioEffectDelayConfig::set_tap1_active(bool p_active) { tap_1_active = p_active; }

bool AudioEffectDelayConfig::is_tap1_active() const { return tap_1_active; }

void AudioEffectDelayConfig::set_tap1_delay_ms(float p_delay_ms) { tap_1_delay_ms = p_delay_ms; }

float AudioEffectDelayConfig::get_tap1_delay_ms() const { return tap_1_delay_ms; }

void AudioEffectDelayConfig::set_tap1_level_db(float p_level_db) { tap_1_level = p_level_db; }

float AudioEffectDelayConfig::get_tap1_level_db() const { return tap_1_level; }

void AudioEffectDelayConfig::set_tap1_pan(float p_pan) { tap_1_pan = p_pan; }

float AudioEffectDelayConfig::get_tap1_pan() const { return tap_1_pan; }

void AudioEffectDelayConfig::set_tap2_active(bool p_active) { tap_2_active = p_active; }

bool AudioEffectDelayConfig::is_tap2_active() const { return tap_2_active; }

void AudioEffectDelayConfig::set_tap2_delay_ms(float p_delay_ms) { tap_2_delay_ms = p_delay_ms; }

float AudioEffectDelayConfig::get_tap2_delay_ms() const { return tap_2_delay_ms; }

void AudioEffectDelayConfig::set_tap2_level_db(float p_level_db) { tap_2_level = p_level_db; }

float AudioEffectDelayConfig::get_tap2_level_db() const { return tap_2_level; }

void AudioEffectDelayConfig::set_tap2_pan(float p_pan) { tap_2_pan = p_pan; }

float AudioEffectDelayConfig::get_tap2_pan() const { return tap_2_pan; }

void AudioEffectDelayConfig::set_feedback_active(bool p_active) { feedback_active = p_active; }

bool AudioEffectDelayConfig::is_feedback_active() const { return feedback_active; }

void AudioEffectDelayConfig::set_feedback_delay_ms(float p_delay_ms) { feedback_delay_ms = p_delay_ms; }

float AudioEffectDelayConfig::get_feedback_delay_ms() const { return feedback_delay_ms; }

void AudioEffectDelayConfig::set_feedback_level_db(float p_level_db) { feedback_level = p_level_db; }

float AudioEffectDelayConfig::get_feedback_level_db() const { return feedback_level; }

void AudioEffectDelayConfig::set_feedback_lowpass(float p_lowpass)
{
	feedback_lowpass = MAX(p_lowpass, 1.0f);
}

float AudioEffectDelayConfig::get_feedback_lowpass() const { return feedback_lowpass; }

// ============================================================================
// 6. Distortion
// ============================================================================

void AudioEffectDistortionState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectDistortionConfig* cfg = base.is_valid() ? base->get_distortion_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	const float* src = (const float*)p_src_frames;
	float* dst = (float*)p_dst_frames;

	float lpf_c = std::exp(-Math::TAU * cfg->keep_hf_hz / AudioServer::get_mix_rate());
	float lpf_ic = 1.0f - lpf_c;

	float drive_f = cfg->drive;
	float pregain_f = Math::db_to_linear(cfg->pre_gain);
	float postgain_f = Math::db_to_linear(cfg->post_gain);

	float atan_mult = std::pow(10.0f, drive_f * drive_f * 3.0f) - 1.0f + 0.001f;
	float atan_div = 1.0f / (std::atan(atan_mult) * (1.0f + drive_f * 8.0f));

	float lofi_mult = std::pow(2.0f, 2.0f + (1.0f - drive_f) * 14.0f);

	for (int i = 0; i < p_frame_count * 2; i++) {
		float out = undenormalize(src[i] * lpf_ic + lpf_c * h[i & 1]);
		h[i & 1] = out;
		float a = out;
		float ha = src[i] - out;
		a *= pregain_f;

		switch (cfg->mode) {
		case AudioEffectDistortionConfig::MODE_CLIP: {
			float a_sign = a < 0 ? -1.0f : 1.0f;
			a = std::pow(std::abs(a), 1.0001f - drive_f) * a_sign;
			if (a > 1.0f) {
				a = 1.0f;
			}
			else if (a < -1.0f) {
				a = -1.0f;
			}
		} break;
		case AudioEffectDistortionConfig::MODE_ATAN: {
			a = std::atan(a * atan_mult) * atan_div;
		} break;
		case AudioEffectDistortionConfig::MODE_LOFI: {
			a = std::floor(a * lofi_mult + 0.5f) / lofi_mult;
		} break;
		case AudioEffectDistortionConfig::MODE_OVERDRIVE: {
			const double x = a * 0.686306;
			const double z = 1 + std::exp(std::sqrt(std::abs(x)) * -0.75);
			a = (std::exp(x) - std::exp(-x * z)) / (std::exp(x) + std::exp(-x));
		} break;
		case AudioEffectDistortionConfig::MODE_WAVESHAPE: {
			float x = a;
			float k = 2 * drive_f / (1.00001f - drive_f);
			a = (1.0f + k) * x / (1.0f + k * std::abs(x));
		} break;
		}

		dst[i] = a * postgain_f + ha;
	}
}

void AudioEffectDistortionConfig::set_mode(Mode p_mode) { mode = p_mode; }

AudioEffectDistortionConfig::Mode AudioEffectDistortionConfig::get_mode() const { return mode; }

void AudioEffectDistortionConfig::set_pre_gain(float p_pre_gain) { pre_gain = p_pre_gain; }

float AudioEffectDistortionConfig::get_pre_gain() const { return pre_gain; }

void AudioEffectDistortionConfig::set_keep_hf_hz(float p_keep_hf_hz)
{
	keep_hf_hz = MAX(p_keep_hf_hz, 1.0f);
}

float AudioEffectDistortionConfig::get_keep_hf_hz() const { return keep_hf_hz; }

void AudioEffectDistortionConfig::set_drive(float p_drive) { drive = p_drive; }

float AudioEffectDistortionConfig::get_drive() const { return drive; }

void AudioEffectDistortionConfig::set_post_gain(float p_post_gain) { post_gain = p_post_gain; }

float AudioEffectDistortionConfig::get_post_gain() const { return post_gain; }

// ============================================================================
// 7. EQ
// ============================================================================

void AudioEffectEQState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectEQConfig* cfg = base.is_valid() ? base->get_eq_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	int band_count = bands[0].size();
	EQ::BandProcess* proc_l = bands[0].ptrw();
	EQ::BandProcess* proc_r = bands[1].ptrw();
	float* bgain = gains.ptrw();
	for (int i = 0; i < band_count; i++) {
		bgain[i] = Math::db_to_linear(cfg->gain[i]);
	}

	for (int i = 0; i < p_frame_count; i++) {
		AudioFrame src = p_src_frames[i];
		AudioFrame dst = AudioFrame(0, 0);

		for (int j = 0; j < band_count; j++) {
			float l = src.left;
			float r = src.right;

			proc_l[j].process_one(l);
			proc_r[j].process_one(r);

			dst.left += l * bgain[j];
			dst.right += r * bgain[j];
		}

		p_dst_frames[i] = dst;
	}
}

void AudioEffectEQConfig::set_preset(EQ::Preset p_preset)
{
	eq = EQ();
	eq.set_mix_rate(AudioServer::get_mix_rate());
	eq.set_preset_band_mode(p_preset);
	gain.clear();
	prop_band_map.clear();
	band_names.clear();
	gain.resize(eq.get_band_count());
	for (int i = 0; i < gain.size(); i++) {
		gain.write[i] = 0.0f;
		String band_name = "band_db/" + itos(eq.get_band_frequency(i)) + "_hz";
		prop_band_map[band_name] = i;
		band_names.push_back(band_name);
	}
}

void AudioEffectEQConfig::set_band_gain_db(int p_band, float p_volume)
{
	ERR_FAIL_INDEX(p_band, gain.size());
	gain.write[p_band] = p_volume;
}

float AudioEffectEQConfig::get_band_gain_db(int p_band) const
{
	ERR_FAIL_INDEX_V(p_band, gain.size(), 0);
	return gain[p_band];
}

int AudioEffectEQConfig::get_band_count() const { return gain.size(); }

AudioEffectEQConfig::AudioEffectEQConfig(EQ::Preset p_preset)
{
	set_preset(p_preset);
}

// ============================================================================
// 8. Filter
// ============================================================================

template <int S>
void AudioEffectFilterState::_process_filter(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	for (int i = 0; i < p_frame_count; i++) {
		float f = p_src_frames[i].left;
		filter_process[0][0].process_one(f);
		if constexpr (S > 1) {
			filter_process[0][1].process_one(f);
		}
		if constexpr (S > 2) {
			filter_process[0][2].process_one(f);
		}
		if constexpr (S > 3) {
			filter_process[0][3].process_one(f);
		}
		p_dst_frames[i].left = f;
	}

	for (int i = 0; i < p_frame_count; i++) {
		float f = p_src_frames[i].right;
		filter_process[1][0].process_one(f);
		if constexpr (S > 1) {
			filter_process[1][1].process_one(f);
		}
		if constexpr (S > 2) {
			filter_process[1][2].process_one(f);
		}
		if constexpr (S > 3) {
			filter_process[1][3].process_one(f);
		}
		p_dst_frames[i].right = f;
	}
}

void AudioEffectFilterState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectFilterConfig* cfg = base.is_valid() ? base->get_filter_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	filter.set_cutoff(cfg->cutoff);
	filter.set_gain(cfg->gain);
	filter.set_resonance(cfg->resonance);
	filter.set_mode(cfg->mode);
	int stages = int(cfg->db) + 1;
	filter.set_stages(stages);
	filter.set_sampling_rate(AudioServer::get_mix_rate());

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].update_coeffs();
		}
	}

	if (stages == 1) {
		_process_filter<1>(p_src_frames, p_dst_frames, p_frame_count);
	}
	else if (stages == 2) {
		_process_filter<2>(p_src_frames, p_dst_frames, p_frame_count);
	}
	else if (stages == 3) {
		_process_filter<3>(p_src_frames, p_dst_frames, p_frame_count);
	}
	else if (stages == 4) {
		_process_filter<4>(p_src_frames, p_dst_frames, p_frame_count);
	}
}

AudioEffectFilterState::AudioEffectFilterState()
{
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].set_filter(&filter);
		}
	}
}

void AudioEffectFilterConfig::set_filter_mode(AudioFilterSW::Mode p_mode) { mode = p_mode; }

AudioFilterSW::Mode AudioEffectFilterConfig::get_filter_mode() const { return mode; }

void AudioEffectFilterConfig::set_cutoff(float p_freq) { cutoff = MAX(p_freq, 1.0f); }

float AudioEffectFilterConfig::get_cutoff() const { return cutoff; }

void AudioEffectFilterConfig::set_resonance(float p_amount) { resonance = p_amount; }

float AudioEffectFilterConfig::get_resonance() const { return resonance; }

void AudioEffectFilterConfig::set_gain(float p_amount) { gain = p_amount; }

float AudioEffectFilterConfig::get_gain() const { return gain; }

void AudioEffectFilterConfig::set_db(FilterDB p_db) { db = p_db; }

AudioEffectFilterConfig::FilterDB AudioEffectFilterConfig::get_db() const { return db; }

AudioEffectFilterConfig::AudioEffectFilterConfig(AudioFilterSW::Mode p_mode) : mode(p_mode) {}

// ============================================================================
// 9. HardLimiter
// ============================================================================

void AudioEffectHardLimiterState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectHardLimiterConfig* cfg = base.is_valid() ? base->get_hard_limiter_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float sample_rate = AudioServer::get_mix_rate();

	float ceiling = Math::db_to_linear(cfg->ceiling);
	float release = cfg->release;
	float attack = cfg->attack;
	float pre_gain = Math::db_to_linear(cfg->pre_gain);

	for (int i = 0; i < p_frame_count; i++) {
		float sample_left = p_src_frames[i].left * pre_gain;
		float sample_right = p_src_frames[i].right * pre_gain;

		float largest_sample = MAX(Math::abs(sample_left), Math::abs(sample_right));

		release_factor = MAX(0.0f, release_factor - 1.0f / sample_rate);
		release_factor = MIN(release_factor, release);

		if (release_factor > 0.0f) {
			gain = Math::lerp(gain_target, 1.0f, 1.0f - release_factor / release);
		}

		if (largest_sample * gain > ceiling) {
			gain_target = ceiling / largest_sample;
			release_factor = release;
			attack_factor = attack;
		}

		attack_factor = MAX(0.0f, attack_factor - 1.0f / sample_rate);
		if (attack_factor > 0.0f) {
			gain = Math::lerp(gain_target, gain, 1.0f - attack_factor / attack);
		}

		int bucket_id = gain_bucket_cursor / gain_bucket_size;

		if (gain_bucket_cursor % gain_bucket_size == 0) {
			gain_buckets[bucket_id] = 1.0f;
		}

		gain_buckets[bucket_id] = MIN(gain_buckets[bucket_id], gain);
		gain_bucket_cursor = (gain_bucket_cursor + 1) % gain_samples_to_store;

		for (int j = 0; j < (int)gain_buckets.size(); j++) {
			gain = MIN(gain, gain_buckets[j]);
		}

		float dst_buffer_left = sample_buffer_left[sample_cursor];
		float dst_buffer_right = sample_buffer_right[sample_cursor];

		sample_buffer_left[sample_cursor] = sample_left;
		sample_buffer_right[sample_cursor] = sample_right;

		sample_cursor = (sample_cursor + 1) % sample_buffer_left.size();

		p_dst_frames[i].left = dst_buffer_left * gain;
		p_dst_frames[i].right = dst_buffer_right * gain;
	}
}

void AudioEffectHardLimiterConfig::set_ceiling_db(float p_ceiling) { ceiling = p_ceiling; }

float AudioEffectHardLimiterConfig::get_ceiling_db() const { return ceiling; }

float AudioEffectHardLimiterConfig::get_pre_gain_db() const { return pre_gain; }

void AudioEffectHardLimiterConfig::set_pre_gain_db(float p_pre_gain) { pre_gain = p_pre_gain; }

float AudioEffectHardLimiterConfig::get_release() const { return release; }

void AudioEffectHardLimiterConfig::set_release(float p_release) { release = p_release; }

// ============================================================================
// 10. Limiter
// ============================================================================

void AudioEffectLimiterState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectLimiterConfig* cfg = base.is_valid() ? base->get_limiter_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float threshdb = cfg->threshold;
	float ceiling = Math::db_to_linear(cfg->ceiling);
	float ceildb = cfg->ceiling;
	float makeup = Math::db_to_linear(ceildb - threshdb);
	float sc = -cfg->soft_clip;
	float scv = Math::db_to_linear(sc);
	float peakdb = ceildb + 25;
	float scmult = Math::abs((ceildb - sc) / (peakdb - sc));

	for (int i = 0; i < p_frame_count; i++) {
		float spl0 = p_src_frames[i].left * makeup;
		float spl1 = p_src_frames[i].right * makeup;
		float sign0 = (spl0 < 0.0f ? -1.0f : 1.0f);
		float sign1 = (spl1 < 0.0f ? -1.0f : 1.0f);
		float abs0 = Math::abs(spl0);
		float abs1 = Math::abs(spl1);
		float overdb0 = Math::linear_to_db(abs0) - ceildb;
		float overdb1 = Math::linear_to_db(abs1) - ceildb;

		if (abs0 > scv) {
			spl0 = sign0 * (scv + Math::db_to_linear(overdb0 * scmult));
		}
		if (abs1 > scv) {
			spl1 = sign1 * (scv + Math::db_to_linear(overdb1 * scmult));
		}

		spl0 = MIN(ceiling, Math::abs(spl0)) * (spl0 < 0.0f ? -1.0f : 1.0f);
		spl1 = MIN(ceiling, Math::abs(spl1)) * (spl1 < 0.0f ? -1.0f : 1.0f);

		p_dst_frames[i].left = spl0;
		p_dst_frames[i].right = spl1;
	}
}

void AudioEffectLimiterConfig::set_threshold_db(float p_threshold) { threshold = p_threshold; }

float AudioEffectLimiterConfig::get_threshold_db() const { return threshold; }

void AudioEffectLimiterConfig::set_ceiling_db(float p_ceiling) { ceiling = p_ceiling; }

float AudioEffectLimiterConfig::get_ceiling_db() const { return ceiling; }

void AudioEffectLimiterConfig::set_soft_clip_db(float p_soft_clip) { soft_clip = p_soft_clip; }

float AudioEffectLimiterConfig::get_soft_clip_db() const { return soft_clip; }

void AudioEffectLimiterConfig::set_soft_clip_ratio(float p_soft_clip) { soft_clip_ratio = p_soft_clip; }

float AudioEffectLimiterConfig::get_soft_clip_ratio() const { return soft_clip_ratio; }

// ============================================================================
// 11. Panner
// ============================================================================

void AudioEffectPannerState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectPannerConfig* cfg = base.is_valid() ? base->get_panner_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float lvol = CLAMP(1.0f - cfg->pan, 0.0f, 1.0f);
	float rvol = CLAMP(1.0f + cfg->pan, 0.0f, 1.0f);

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i].left = p_src_frames[i].left * lvol + p_src_frames[i].right * (1.0f - rvol);
		p_dst_frames[i].right = p_src_frames[i].right * rvol + p_src_frames[i].left * (1.0f - lvol);
	}
}

void AudioEffectPannerConfig::set_pan(float p_cpanume) { pan = p_cpanume; }

float AudioEffectPannerConfig::get_pan() const { return pan; }

// ============================================================================
// 12. Phaser
// ============================================================================

void AudioEffectPhaserState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectPhaserConfig* cfg = base.is_valid() ? base->get_phaser_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float sampling_rate = AudioServer::get_mix_rate();

	float dmin = cfg->range_min / (sampling_rate / 2.0f);
	float dmax = cfg->range_max / (sampling_rate / 2.0f);
	float increment = Math::TAU * (cfg->rate / sampling_rate);

	for (int i = 0; i < p_frame_count; i++) {
		phase += increment;

		while (phase >= Math::TAU) {
			phase -= Math::TAU;
		}

		float d = dmin + (dmax - dmin) * ((std::sin(phase) + 1.f) / 2.f);

		for (int j = 0; j < 6; j++) {
			allpass[0][j].delay(d);
			allpass[1][j].delay(d);
		}

		float y = allpass[0][0].update(
			allpass[0][1].update(allpass[0][2].update(allpass[0][3].update(allpass[0][4].update(
				allpass[0][5].update(p_src_frames[i].left + h.left * cfg->feedback))))));
		h.left = y;
		p_dst_frames[i].left = p_src_frames[i].left + y * cfg->depth;

		y = allpass[1][0].update(
			allpass[1][1].update(allpass[1][2].update(allpass[1][3].update(allpass[1][4].update(
				allpass[1][5].update(p_src_frames[i].right + h.right * cfg->feedback))))));
		h.right = y;
		p_dst_frames[i].right = p_src_frames[i].right + y * cfg->depth;
	}
}

void AudioEffectPhaserConfig::set_range_min_hz(float p_hz) { range_min = p_hz; }

float AudioEffectPhaserConfig::get_range_min_hz() const { return range_min; }

void AudioEffectPhaserConfig::set_range_max_hz(float p_hz) { range_max = p_hz; }

float AudioEffectPhaserConfig::get_range_max_hz() const { return range_max; }

void AudioEffectPhaserConfig::set_rate_hz(float p_hz) { rate = p_hz; }

float AudioEffectPhaserConfig::get_rate_hz() const { return rate; }

void AudioEffectPhaserConfig::set_feedback(float p_fbk) { feedback = p_fbk; }

float AudioEffectPhaserConfig::get_feedback() const { return feedback; }

void AudioEffectPhaserConfig::set_depth(float p_depth) { depth = p_depth; }

float AudioEffectPhaserConfig::get_depth() const { return depth; }

// ============================================================================
// 13. PitchShift
// ============================================================================

/* clang-format off */
void SMBPitchShift::PitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata, int stride)
{
	double magn, phase, tmp, window, real, imag;
	double freqPerBin, expct;
	long i, k, qpd, index, inFifoLatency, stepSize, fftFrameSize2;

	fftFrameSize2 = fftFrameSize / 2;
	stepSize = fftFrameSize / osamp;
	freqPerBin = sampleRate / (double)fftFrameSize;
	expct = 2. * Math::PI * (double)stepSize / (double)fftFrameSize;
	inFifoLatency = fftFrameSize - stepSize;
	if (gRover == 0) {
		gRover = inFifoLatency;
	}

	for (i = 0; i < numSampsToProcess; i++) {
		gInFIFO[gRover] = indata[i * stride];
		outdata[i * stride] = gOutFIFO[gRover - inFifoLatency];
		gRover++;

		if (gRover >= fftFrameSize) {
			gRover = inFifoLatency;

			for (k = 0; k < fftFrameSize; k++) {
				window = -.5 * std::cos(2. * Math::PI * (double)k / (double)fftFrameSize) + .5;
				gFFTworksp[2 * k] = gInFIFO[k] * window;
				gFFTworksp[2 * k + 1] = 0.;
			}

			smbFft(gFFTworksp, fftFrameSize, -1);

			for (k = 0; k <= fftFrameSize2; k++) {
				real = gFFTworksp[2 * k];
				imag = gFFTworksp[2 * k + 1];

				magn = 2. * std::sqrt(real * real + imag * imag);
				phase = std::atan2(imag, real);

				tmp = phase - gLastPhase[k];
				gLastPhase[k] = phase;

				tmp -= (double)k * expct;

				qpd = tmp / Math::PI;
				if (qpd >= 0) {
					qpd += qpd & 1;
				}
				else {
					qpd -= qpd & 1;
				}
				tmp -= Math::PI * (double)qpd;

				tmp = osamp * tmp / (2. * Math::PI);
				tmp = (double)k * freqPerBin + tmp * freqPerBin;

				gAnaMagn[k] = magn;
				gAnaFreq[k] = tmp;
			}

			size_t fftBufferSize = static_cast<size_t>(fftFrameSize) * sizeof(float);
			if (unlikely(fftBufferSize > MAX_FRAME_LENGTH)) {
				ERR_PRINT_ONCE("Invalid FFT frame size for PitchShift. This is a bug, please report.");
				return;
			}
			memset(gSynMagn, 0, fftBufferSize);
			memset(gSynFreq, 0, fftBufferSize);
			for (k = 0; k <= fftFrameSize2; k++) {
				index = k * pitchShift;
				if (index <= fftFrameSize2) {
					gSynMagn[index] += gAnaMagn[k];
					gSynFreq[index] = gAnaFreq[k] * pitchShift;
				}
			}

			for (k = 0; k <= fftFrameSize2; k++) {
				magn = gSynMagn[k];
				tmp = gSynFreq[k];

				tmp -= (double)k * freqPerBin;
				tmp /= freqPerBin;
				tmp = 2. * Math::PI * tmp / osamp;
				tmp += (double)k * expct;

				gSumPhase[k] += tmp;
				phase = gSumPhase[k];

				gFFTworksp[2 * k] = magn * std::cos(phase);
				gFFTworksp[2 * k + 1] = magn * std::sin(phase);
			}

			for (k = fftFrameSize + 2; k < 2 * fftFrameSize; k++) {
				gFFTworksp[k] = 0.;
			}

			smbFft(gFFTworksp, fftFrameSize, 1);

			for (k = 0; k < fftFrameSize; k++) {
				window = -.5 * std::cos(2. * Math::PI * (double)k / (double)fftFrameSize) + .5;
				gOutputAccum[k] += 2. * window * gFFTworksp[2 * k] / (fftFrameSize2 * osamp);
			}
			for (k = 0; k < stepSize; k++) {
				gOutFIFO[k] = gOutputAccum[k];
			}

			memmove(gOutputAccum, gOutputAccum + stepSize, fftBufferSize);

			for (k = 0; k < inFifoLatency; k++) {
				gInFIFO[k] = gInFIFO[k + stepSize];
			}
		}
	}
}

void SMBPitchShift::smbFft(float *fftBuffer, long fftFrameSize, long sign)
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2 * fftFrameSize - 2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1) {
			if (i & bitm) {
				j++;
			}
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer + i;
			p2 = fftBuffer + j;
			temp = *p1;
			*(p1++) = *p2;
			*(p2++) = temp;
			temp = *p1;
			*p1 = *p2;
			*p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(std::log((double)fftFrameSize) / std::log(2.) + .5); k++) {
		le <<= 1;
		le2 = le >> 1;
		ur = 1.0;
		ui = 0.0;
		arg = Math::PI / (le2 >> 1);
		wr = std::cos(arg);
		wi = sign * std::sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer + j;
			p1i = p1r + 1;
			p2r = p1r + le2;
			p2i = p2r + 1;
			for (i = j; i < 2 * fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr;
				*p2i = *p1i - ti;
				*p1r += tr;
				*p1i += ti;
				p1r += le;
				p1i += le;
				p2r += le;
				p2i += le;
			}
			tr = ur * wr - ui * wi;
			ui = ur * wi + ui * wr;
			ur = tr;
		}
	}
}
/* clang-format on */

void AudioEffectPitchShiftState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectPitchShiftConfig* cfg = base.is_valid() ? base->get_pitch_shift_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	if (Math::is_equal_approx(cfg->pitch_scale, 1.0f)) {
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_frames[i] = p_src_frames[i];
		}
		return;
	}

	float sample_rate = AudioServer::get_mix_rate();

	float* in_l = (float*)p_src_frames;
	float* in_r = in_l + 1;

	float* out_l = (float*)p_dst_frames;
	float* out_r = out_l + 1;

	shift_l.PitchShift(cfg->pitch_scale, p_frame_count, fft_size, cfg->oversampling, sample_rate,
		in_l, out_l, 2);
	shift_r.PitchShift(cfg->pitch_scale, p_frame_count, fft_size, cfg->oversampling, sample_rate,
		in_r, out_r, 2);
}

void AudioEffectPitchShiftConfig::set_pitch_scale(float p_pitch_scale)
{
	ERR_FAIL_COND(!(p_pitch_scale > 0.0f));
	pitch_scale = p_pitch_scale;
}

float AudioEffectPitchShiftConfig::get_pitch_scale() const { return pitch_scale; }

void AudioEffectPitchShiftConfig::set_oversampling(int p_oversampling)
{
	ERR_FAIL_COND(p_oversampling < 4);
	oversampling = p_oversampling;
}

int AudioEffectPitchShiftConfig::get_oversampling() const { return oversampling; }

void AudioEffectPitchShiftConfig::set_fft_size(FFTSize p_fft_size)
{
	ERR_FAIL_INDEX(p_fft_size, FFT_SIZE_MAX);
	fft_size = p_fft_size;
}

AudioEffectPitchShiftConfig::FFTSize AudioEffectPitchShiftConfig::get_fft_size() const
{
	return fft_size;
}

// ============================================================================
// 14. Record
// ============================================================================

void AudioEffectRecordState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	if (!is_recording) {
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_frames[i] = p_src_frames[i];
		}
		return;
	}

	const AudioFrame* src = p_src_frames;
	AudioFrame* rb_buf = ring_buffer.ptrw();
	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
		rb_buf[ring_buffer_pos & ring_buffer_mask] = src[i];
		ring_buffer_pos++;
	}
}

void AudioEffectRecordState::_update_buffer()
{
	while (ring_buffer_read_pos < ring_buffer_pos) {
		_io_store_buffer();
	}
}

void AudioEffectRecordState::_update(void* userdata)
{
	AudioEffectRecordState* ins = static_cast<AudioEffectRecordState*>(userdata);
	ins->_update_buffer();
}

bool AudioEffectRecordState::process_silence() const { return true; }

void AudioEffectRecordState::_io_thread_process()
{
	while (is_recording) {
		_update_buffer();
		if (is_recording) {
			OS::delay_usec(500);
		}
	}
}

void AudioEffectRecordState::_io_store_buffer()
{
	int to_read = ring_buffer_pos - ring_buffer_read_pos;
	AudioFrame* rb_buf = ring_buffer.ptrw();

	while (to_read) {
		AudioFrame buffered_frame = rb_buf[ring_buffer_read_pos & ring_buffer_mask];
		recording_data.push_back(buffered_frame.left);
		recording_data.push_back(buffered_frame.right);

		ring_buffer_read_pos++;
		to_read--;
	}
}

void AudioEffectRecordState::_thread_callback(void* _instance)
{
	AudioEffectRecordState* aeri = reinterpret_cast<AudioEffectRecordState*>(_instance);
	aeri->_io_thread_process();
}

void AudioEffectRecordState::init()
{
	ring_buffer_pos = 0;
	ring_buffer_read_pos = 0;
	recording_data.clear();
	is_recording = true;
	io_thread.start(_thread_callback, this);
}

void AudioEffectRecordState::finish()
{
	is_recording = false;
	if (io_thread.is_started()) {
		io_thread.wait_to_finish();
	}
}

void AudioEffectRecordConfig::ensure_thread_stopped()
{
	if (current_instance.is_valid()) {
		if (AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>()) {
			state->finish();
		}
	}
}

void AudioEffectRecordConfig::set_recording_active(bool p_record)
{
	if (p_record) {
		if (current_instance.is_null()) {
			WARN_PRINT("Recording should not be set as active before Godot has initialized.");
			return;
		}
		ensure_thread_stopped();
		if (AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>()) {
			state->init();
		}
	}
	else {
		if (current_instance.is_valid()) {
			if (AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>()) {
				state->is_recording = false;
			}
		}
	}
}

bool AudioEffectRecordConfig::is_recording_active() const
{
	if (current_instance.is_null()) {
		return false;
	}
	const AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>();
	return state ? state->is_recording : false;
}

void AudioEffectRecordConfig::set_format(AudioStreamWAV::Format p_format) { format = p_format; }

AudioStreamWAV::Format AudioEffectRecordConfig::get_format() const { return format; }

Ref<AudioStreamWAV> AudioEffectRecordConfig::get_recording() const
{
	AudioStreamWAV::Format dst_format = format;
	bool stereo = true;

	Vector<uint8_t> dst_data;

	ERR_FAIL_COND_V(current_instance.is_null(), nullptr);
	const AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>();
	ERR_FAIL_COND_V(!state || state->recording_data.is_empty(), nullptr);

	if (dst_format == AudioStreamWAV::FORMAT_8_BITS) {
		int data_size = state->recording_data.size();
		dst_data.resize(data_size);
		uint8_t* w = dst_data.ptrw();

		for (int i = 0; i < data_size; i++) {
			int8_t v = CLAMP(state->recording_data[i] * 128, -128, 127);
			w[i] = v;
		}
	}
	else if (dst_format == AudioStreamWAV::FORMAT_16_BITS) {
		int data_size = state->recording_data.size();
		dst_data.resize(data_size * 2);
		uint8_t* w = dst_data.ptrw();

		for (int i = 0; i < data_size; i++) {
			int16_t v = CLAMP(state->recording_data[i] * 32768, -32768, 32767);
			encode_uint16(v, &w[i * 2]);
		}
	}
	else if (dst_format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		Vector<float> left;
		Vector<float> right;

		int tframes = state->recording_data.size() / 2;
		left.resize(tframes);
		right.resize(tframes);

		for (int i = 0; i < tframes; i++) {
			left.set(i, state->recording_data[i * 2 + 0]);
			right.set(i, state->recording_data[i * 2 + 1]);
		}

		Vector<uint8_t> bleft;
		Vector<uint8_t> bright;

		AudioStreamWAV::_compress_ima_adpcm(left, bleft);
		AudioStreamWAV::_compress_ima_adpcm(right, bright);

		int dl = bleft.size();
		dst_data.resize(dl * 2);

		uint8_t* w = dst_data.ptrw();
		const uint8_t* rl = bleft.ptr();
		const uint8_t* rr = bright.ptr();

		for (int i = 0; i < dl; i++) {
			w[i * 2 + 0] = rl[i];
			w[i * 2 + 1] = rr[i];
		}
	}
	else if (dst_format == AudioStreamWAV::FORMAT_QOA) {
		qoa_desc desc = {};
		desc.samples = state->recording_data.size() / 2;
		desc.samplerate = AudioServer::get_mix_rate();
		desc.channels = 2;
		AudioStreamWAV::_compress_qoa(state->recording_data, dst_data, &desc);
	}
	else {
		ERR_PRINT("Format not implemented.");
	}

	Ref<AudioStreamWAV> sample;
	sample.instantiate();
	sample->set_data(dst_data);
	sample->set_format(dst_format);
	sample->set_mix_rate(AudioServer::get_mix_rate());
	sample->set_loop_mode(AudioStreamWAV::LOOP_DISABLED);
	sample->set_loop_begin(0);
	sample->set_loop_end(0);
	sample->set_stereo(stereo);

	return sample;
}

// ============================================================================
// 15. Reverb
// ============================================================================

void AudioEffectReverbState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectReverbConfig* cfg = base.is_valid() ? base->get_reverb_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	for (int i = 0; i < 2; i++) {
		Reverb& r = reverb[i];

		r.set_predelay(cfg->predelay);
		r.set_predelay_feedback(cfg->predelay_fb);
		r.set_highpass(cfg->hpf);
		r.set_room_size(cfg->room_size);
		r.set_damp(cfg->damping);
		r.set_extra_spread(cfg->spread);
		r.set_wet(cfg->wet);
		r.set_dry(cfg->dry);
	}

	int todo = p_frame_count;
	int offset = 0;

	while (todo) {
		int to_mix = MIN(todo, Reverb::INPUT_BUFFER_MAX_SIZE);

		for (int j = 0; j < to_mix; j++) {
			tmp_src[j] = p_src_frames[offset + j].left;
		}

		reverb[0].process(tmp_src, tmp_dst, to_mix);

		for (int j = 0; j < to_mix; j++) {
			p_dst_frames[offset + j].left = tmp_dst[j];
			tmp_src[j] = p_src_frames[offset + j].right;
		}

		reverb[1].process(tmp_src, tmp_dst, to_mix);

		for (int j = 0; j < to_mix; j++) {
			p_dst_frames[offset + j].right = tmp_dst[j];
		}

		offset += to_mix;
		todo -= to_mix;
	}
}

AudioEffectReverbState::AudioEffectReverbState()
{
	reverb[0].set_mix_rate(AudioServer::get_mix_rate());
	reverb[0].set_extra_spread_base(0);
	reverb[1].set_mix_rate(AudioServer::get_mix_rate());
	reverb[1].set_extra_spread_base(0.000521f);
}

void AudioEffectReverbConfig::set_predelay_msec(float p_msec) { predelay = p_msec; }

void AudioEffectReverbConfig::set_predelay_feedback(float p_feedback)
{
	predelay_fb = CLAMP(p_feedback, 0.0f, 0.98f);
}

void AudioEffectReverbConfig::set_room_size(float p_size) { room_size = p_size; }

void AudioEffectReverbConfig::set_damping(float p_damping) { damping = p_damping; }

void AudioEffectReverbConfig::set_spread(float p_spread) { spread = p_spread; }

void AudioEffectReverbConfig::set_dry(float p_dry) { dry = p_dry; }

void AudioEffectReverbConfig::set_wet(float p_wet) { wet = p_wet; }

void AudioEffectReverbConfig::set_hpf(float p_hpf) { hpf = p_hpf; }

float AudioEffectReverbConfig::get_predelay_msec() const { return predelay; }

float AudioEffectReverbConfig::get_predelay_feedback() const { return predelay_fb; }

float AudioEffectReverbConfig::get_room_size() const { return room_size; }

float AudioEffectReverbConfig::get_damping() const { return damping; }

float AudioEffectReverbConfig::get_spread() const { return spread; }

float AudioEffectReverbConfig::get_dry() const { return dry; }

float AudioEffectReverbConfig::get_wet() const { return wet; }

float AudioEffectReverbConfig::get_hpf() const { return hpf; }

// ============================================================================
// 16. SpectrumAnalyzer
// ============================================================================

static void _spectrum_smb_fft(float* fftBuffer, long fftFrameSize, long sign)
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k, logN;
	logN = (long)(Math::log((double)fftFrameSize) / Math::log(2.) + .5);

	for (i = 2; i < 2 * fftFrameSize - 2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1) {
			if (i & bitm) {
				j++;
			}
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer + i;
			p2 = fftBuffer + j;
			temp = *p1;
			*(p1++) = *p2;
			*(p2++) = temp;
			temp = *p1;
			*p1 = *p2;
			*p2 = temp;
		}
	}
	for (k = 0, le = 2; k < logN; k++) {
		le <<= 1;
		le2 = le >> 1;
		ur = 1.0;
		ui = 0.0;
		arg = Math::PI / (le2 >> 1);
		wr = std::cos(arg);
		wi = sign * std::sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer + j;
			p1i = p1r + 1;
			p2r = p1r + le2;
			p2i = p2r + 1;
			for (i = j; i < 2 * fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr;
				*p2i = *p1i - ti;
				*p1r += tr;
				*p1i += ti;
				p1r += le;
				p1i += le;
				p2r += le;
				p2i += le;
			}
			tr = ur * wr - ui * wi;
			ui = ur * wi + ui * wr;
			ur = tr;
		}
	}
}

void AudioEffectSpectrumAnalyzerState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
	}

	while (p_frame_count) {
		int to_fill = fft_size * 2 - temporal_fft_pos;
		to_fill = MIN(to_fill, p_frame_count);
		const double to_fill_step = Math::TAU / (double)(fft_size * 2);

		float* fftw = temporal_fft.ptrw();
		for (int i = 0; i < to_fill; i++) {
			float window = -0.5 * Math::cos(to_fill_step * (double)temporal_fft_pos) + 0.5;
			fftw[temporal_fft_pos * 2] = window * p_src_frames->left;
			fftw[temporal_fft_pos * 2 + 1] = 0;
			fftw[(temporal_fft_pos + fft_size * 2) * 2] = window * p_src_frames->right;
			fftw[(temporal_fft_pos + fft_size * 2) * 2 + 1] = 0;
			++p_src_frames;
			++temporal_fft_pos;
		}

		p_frame_count -= to_fill;

		if (temporal_fft_pos == fft_size * 2) {
			_spectrum_smb_fft(fftw, fft_size * 2, -1);
			_spectrum_smb_fft(fftw + fft_size * 4, fft_size * 2, -1);
			int next = (fft_pos + 1) % fft_count;

			AudioFrame* hw = (AudioFrame*)fft_history[next].ptr();

			for (int i = 0; i < fft_size; i++) {
				hw[i].left = Vector2(fftw[i * 2], fftw[i * 2 + 1]).length() / float(fft_size);
				hw[i].right =
					Vector2(fftw[fft_size * 4 + i * 2], fftw[fft_size * 4 + i * 2 + 1]).length() /
					float(fft_size);
			}

			fft_pos = next;
			temporal_fft_pos = 0;
		}
	}
}

Vector2 AudioEffectSpectrumAnalyzerState::get_magnitude_for_frequency_range(
	float p_begin, float p_end, MagnitudeMode p_mode) const
{
	int fft_index = fft_pos;

	int begin_pos = p_begin * fft_size / (mix_rate * 0.5f);
	int end_pos = p_end * fft_size / (mix_rate * 0.5f);

	begin_pos = CLAMP(begin_pos, 0, fft_size - 1);
	end_pos = CLAMP(end_pos, 0, fft_size - 1);

	if (begin_pos > end_pos) {
		SWAP(begin_pos, end_pos);
	}
	const AudioFrame* r = fft_history[fft_index].ptr();

	if (p_mode == MAGNITUDE_AVERAGE) {
		Vector2 avg;
		for (int i = begin_pos; i <= end_pos; i++) {
			avg += Vector2(r[i]);
		}
		avg /= float(end_pos - begin_pos + 1);
		return avg;
	}
	else {
		Vector2 max;
		for (int i = begin_pos; i <= end_pos; i++) {
			max.x = MAX(max.x, r[i].left);
			max.y = MAX(max.y, r[i].right);
		}
		return max;
	}
}

void AudioEffectSpectrumAnalyzerConfig::set_buffer_length(float p_seconds)
{
	buffer_length = p_seconds;
}

float AudioEffectSpectrumAnalyzerConfig::get_buffer_length() const { return buffer_length; }

void AudioEffectSpectrumAnalyzerConfig::set_fft_size(FFTSize p_fft_size)
{
	ERR_FAIL_INDEX(p_fft_size, FFT_SIZE_MAX);
	fft_size = p_fft_size;
}

AudioEffectSpectrumAnalyzerConfig::FFTSize AudioEffectSpectrumAnalyzerConfig::get_fft_size() const
{
	return fft_size;
}

// ============================================================================
// 17. StereoEnhance
// ============================================================================

void AudioEffectStereoEnhanceState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectStereoEnhanceConfig* cfg = base.is_valid() ? base->get_stereo_enhance_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float intensity = cfg->pan_pullout;
	bool surround_mode = cfg->surround > 0;
	float surround_amount = cfg->surround;
	unsigned int delay_frames = (cfg->time_pullout / 1000.0f) * AudioServer::get_mix_rate();

	for (int i = 0; i < p_frame_count; i++) {
		float left = p_src_frames[i].left;
		float right = p_src_frames[i].right;

		float center = (left + right) / 2.0f;

		left = center + (left - center) * intensity;
		right = center + (right - center) * intensity;

		if (surround_mode) {
			float val = (left + right) / 2.0f;
			delay_ringbuff[ringbuff_pos & ringbuff_mask] = val;

			float out = delay_ringbuff[(ringbuff_pos - delay_frames) & ringbuff_mask] * surround_amount;
			left += out;
			right += -out;
		}
		else {
			float val = right;
			delay_ringbuff[ringbuff_pos & ringbuff_mask] = val;
			right = delay_ringbuff[(ringbuff_pos - delay_frames) & ringbuff_mask];
		}

		p_dst_frames[i].left = left;
		p_dst_frames[i].right = right;
		ringbuff_pos++;
	}
}

AudioEffectStereoEnhanceState::~AudioEffectStereoEnhanceState()
{
	memdelete_arr(delay_ringbuff);
}

void AudioEffectStereoEnhanceConfig::set_pan_pullout(float p_amount) { pan_pullout = p_amount; }

float AudioEffectStereoEnhanceConfig::get_pan_pullout() const { return pan_pullout; }

void AudioEffectStereoEnhanceConfig::set_time_pullout(float p_amount) { time_pullout = p_amount; }

float AudioEffectStereoEnhanceConfig::get_time_pullout() const { return time_pullout; }

void AudioEffectStereoEnhanceConfig::set_surround(float p_amount) { surround = p_amount; }

float AudioEffectStereoEnhanceConfig::get_surround() const { return surround; }

// ============================================================================
// Merged from servers/audio/effects/eq_filter.cpp
// ============================================================================


#include "core/error/error_macros.h"
#include "core/math/math_funcs.h"

#define POW2(v) ((v) * (v))

/* Helper */
static int solve_quadratic(double a, double b, double c, double *r1, double *r2) {
	//solves quadractic and returns number of roots

	double base = 2 * a;
	if (base == 0.0f) {
		return 0;
	}

	double squared = b * b - 4 * a * c;
	if (squared < 0.0) {
		return 0;
	}

	squared = Math::sqrt(squared);

	*r1 = (-b + squared) / base;
	*r2 = (-b - squared) / base;

	if (*r1 == *r2) {
		return 1;
	} else {
		return 2;
	}
}

EQ::BandProcess::BandProcess() {
	c1 = c2 = c3 = history.a1 = history.a2 = history.a3 = 0;
	history.b1 = history.b2 = history.b3 = 0;
}

void EQ::recalculate_band_coefficients() {
#define BAND_LOG(m_f) (Math::log((m_f)) / Math::log(2.))

	for (int i = 0; i < band.size(); i++) {
		double octave_size;

		double frq = band[i].freq;

		if (i == 0) {
			octave_size = BAND_LOG(band[1].freq) - BAND_LOG(frq);
		} else if (i == (band.size() - 1)) {
			octave_size = BAND_LOG(frq) - BAND_LOG(band[i - 1].freq);
		} else {
			double next = BAND_LOG(band[i + 1].freq) - BAND_LOG(frq);
			double prev = BAND_LOG(frq) - BAND_LOG(band[i - 1].freq);
			octave_size = (next + prev) / 2.0;
		}

		double frq_l = Math::round(frq / Math::pow(2.0, octave_size / 2.0));

		double side_gain2 = POW2(Math::SQRT12);
		double th = Math::TAU * frq / mix_rate;
		double th_l = Math::TAU * frq_l / mix_rate;

		double c2a = side_gain2 * POW2(Math::cos(th)) - 2.0 * side_gain2 * Math::cos(th_l) * Math::cos(th) + side_gain2 - POW2(Math::sin(th_l));

		double c2b = 2.0 * side_gain2 * POW2(Math::cos(th_l)) + side_gain2 * POW2(Math::cos(th)) - 2.0 * side_gain2 * Math::cos(th_l) * Math::cos(th) - side_gain2 + POW2(Math::sin(th_l));

		double c2c = 0.25 * side_gain2 * POW2(Math::cos(th)) - 0.5 * side_gain2 * Math::cos(th_l) * Math::cos(th) + 0.25 * side_gain2 - 0.25 * POW2(Math::sin(th_l));

		//printf("band %i, precoefs = %f,%f,%f\n",i,c2a,c2b,c2c);

		// Default initializing to silence compiler warning about potential uninitialized use.
		// Both variables are properly set in _solve_quadratic before use, or we continue if roots == 0.
		double r1 = 0, r2 = 0; //roots
		int roots = solve_quadratic(c2a, c2b, c2c, &r1, &r2);

		ERR_CONTINUE(roots == 0);

		band.write[i].c1 = 2.0 * ((0.5 - r1) / 2.0);
		band.write[i].c2 = 2.0 * r1;
		band.write[i].c3 = 2.0 * (0.5 + r1) * Math::cos(th);
		//printf("band %i, coefs = %f,%f,%f\n",i,(float)bands[i].c1,(float)bands[i].c2,(float)bands[i].c3);
	}
}

void EQ::set_preset_band_mode(Preset p_preset) {
	band.clear();

#define PUSH_BANDS(m_bands) \
	for (int i = 0; i < m_bands; i++) { \
		Band b; \
		b.freq = bands[i]; \
		b.c1 = b.c2 = b.c3 = 0; \
		band.push_back(b); \
	}

	switch (p_preset) {
		case PRESET_6_BANDS: {
			static const double bands[] = { 32, 100, 320, 1e3, 3200, 10e3 };
			PUSH_BANDS(6);

		} break;

		case PRESET_8_BANDS: {
			static const double bands[] = { 32, 72, 192, 512, 1200, 3000, 7500, 16e3 };

			PUSH_BANDS(8);
		} break;

		case PRESET_10_BANDS: {
			static const double bands[] = { 31.25, 62.5, 125, 250, 500, 1e3, 2e3, 4e3, 8e3, 16e3 };

			PUSH_BANDS(10);

		} break;

		case PRESET_21_BANDS: {
			static const double bands[] = { 22, 32, 44, 63, 90, 125, 175, 250, 350, 500, 700, 1e3, 1400, 2e3, 2800, 4e3, 5600, 8e3, 11e3, 16e3, 22e3 };
			PUSH_BANDS(21);

		} break;

		case PRESET_31_BANDS: {
			static const double bands[] = { 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1e3, 1250, 1600, 2e3, 2500, 3150, 4e3, 5e3, 6300, 8e3, 10e3, 12500, 16e3, 20e3 };
			PUSH_BANDS(31);
		} break;
	};

	recalculate_band_coefficients();
}

int EQ::get_band_count() const {
	return band.size();
}

float EQ::get_band_frequency(int p_band) {
	ERR_FAIL_INDEX_V(p_band, band.size(), 0);
	return band[p_band].freq;
}

void EQ::set_bands(const Vector<float> &p_bands) {
	band.resize(p_bands.size());
	for (int i = 0; i < p_bands.size(); i++) {
		band.write[i].freq = p_bands[i];
	}

	recalculate_band_coefficients();
}

void EQ::set_mix_rate(float p_mix_rate) {
	mix_rate = p_mix_rate;
	recalculate_band_coefficients();
}

EQ::BandProcess EQ::get_band_processor(int p_band) const {
	EQ::BandProcess band_proc;

	ERR_FAIL_INDEX_V(p_band, band.size(), band_proc);

	band_proc.c1 = band[p_band].c1;
	band_proc.c2 = band[p_band].c2;
	band_proc.c3 = band[p_band].c3;

	return band_proc;
}

EQ::EQ() {
	mix_rate = 44100;
}

EQ::~EQ() {
}

#ifdef POW2
#undef POW2
#endif
#ifdef BAND_LOG
#undef BAND_LOG
#endif
#ifdef PUSH_BANDS
#undef PUSH_BANDS
#endif

// ============================================================================
// Merged from servers/audio/effects/reverb_filter.cpp
// ============================================================================


#include "servers/audio/audio_server.h"
#include "core/os/memory.h"

const float Reverb::comb_tunings[MAX_COMBS] = {
	//freeverb comb tunings
	0.025306122448979593f,
	0.026938775510204082f,
	0.028956916099773241f,
	0.03074829931972789f,
	0.032244897959183672f,
	0.03380952380952381f,
	0.035306122448979592f,
	0.036666666666666667f
};

const float Reverb::allpass_tunings[MAX_ALLPASS] = {
	//freeverb allpass tunings
	0.0051020408163265302f,
	0.007732426303854875f,
	0.01f,
	0.012607709750566893f
};

void Reverb::process(float *p_src, float *p_dst, int p_frames) {
	if (p_frames > INPUT_BUFFER_MAX_SIZE) {
		p_frames = INPUT_BUFFER_MAX_SIZE;
	}

	int predelay_frames = std::rint((params.predelay / 1000.0) * params.mix_rate);
	if (predelay_frames < 10) {
		predelay_frames = 10;
	}
	if (predelay_frames >= echo_buffer_size) {
		predelay_frames = echo_buffer_size - 1;
	}

	for (int i = 0; i < p_frames; i++) {
		if (echo_buffer_pos >= echo_buffer_size) {
			echo_buffer_pos = 0;
		}

		int read_pos = echo_buffer_pos - predelay_frames;
		while (read_pos < 0) {
			read_pos += echo_buffer_size;
		}

		float in = undenormalize(echo_buffer[read_pos] * params.predelay_fb + p_src[i]);

		echo_buffer[echo_buffer_pos] = in;

		input_buffer[i] = in;

		p_dst[i] = 0; //take the chance and clear this

		echo_buffer_pos++;
	}

	if (params.hpf > 0) {
		float hpaux = std::exp(-Math::TAU * params.hpf * 6000 / params.mix_rate);
		float hp_a1 = (1.0 + hpaux) / 2.0;
		float hp_a2 = -(1.0 + hpaux) / 2.0;
		float hp_b1 = hpaux;

		for (int i = 0; i < p_frames; i++) {
			float in = input_buffer[i];
			input_buffer[i] = in * hp_a1 + hpf_h1 * hp_a2 + hpf_h2 * hp_b1;
			hpf_h2 = input_buffer[i];
			hpf_h1 = in;
		}
	}

	for (int i = 0; i < MAX_COMBS; i++) {
		Comb &c = comb[i];

		int size_limit = c.size - std::rint((float)c.extra_spread_frames * (1.0 - params.extra_spread));
		for (int j = 0; j < p_frames; j++) {
			if (c.pos >= size_limit) { //reset this now just in case
				c.pos = 0;
			}

			float out = undenormalize(c.buffer[c.pos] * c.feedback);
			out = out * (1.0 - c.damp) + c.damp_h * c.damp; //lowpass
			c.damp_h = out;
			c.buffer[c.pos] = input_buffer[j] + out;
			p_dst[j] += out;
			c.pos++;
		}
	}

	static const float allpass_feedback = 0.7;
	/* this one works, but the other version is just nicer....
	int ap_size_limit[MAX_ALLPASS];

	for (int i=0;i<MAX_ALLPASS;i++) {
		AllPass &a=allpass[i];
		ap_size_limit[i]=a.size-std::rint((float)a.extra_spread_frames*(1.0-params.extra_spread));
	}

	for (int i=0;i<p_frames;i++) {
		float sample=p_dst[i];
		float aux,in;
		float AllPass*ap;

#define PROCESS_ALLPASS(m_ap) 	\
	ap=&allpass[m_ap];	\
	if (ap->pos>=ap_size_limit[m_ap])	\
		ap->pos=0;	\
	aux=undenormalize(ap->buffer[ap->pos]);	\
	in=sample;	\
	sample=-in+aux;	\
	ap->pos++;


		PROCESS_ALLPASS(0);
		PROCESS_ALLPASS(1);
		PROCESS_ALLPASS(2);
		PROCESS_ALLPASS(3);

		p_dst[i]=sample;
	}
	*/

	for (int i = 0; i < MAX_ALLPASS; i++) {
		AllPass &a = allpass[i];
		int size_limit = a.size - std::rint((float)a.extra_spread_frames * (1.0 - params.extra_spread));

		for (int j = 0; j < p_frames; j++) {
			if (a.pos >= size_limit) {
				a.pos = 0;
			}

			float aux = a.buffer[a.pos];
			a.buffer[a.pos] = undenormalize(allpass_feedback * aux + p_dst[j]);
			p_dst[j] = aux - allpass_feedback * a.buffer[a.pos];
			a.pos++;
		}
	}

	static const float wet_scale = 0.6;

	for (int i = 0; i < p_frames; i++) {
		p_dst[i] = p_dst[i] * params.wet * wet_scale + p_src[i] * params.dry;
	}
}

void Reverb::set_room_size(float p_size) {
	params.room_size = p_size;
	update_parameters();
}

void Reverb::set_damp(float p_damp) {
	params.damp = p_damp;
	update_parameters();
}

void Reverb::set_wet(float p_wet) {
	params.wet = p_wet;
}

void Reverb::set_dry(float p_dry) {
	params.dry = p_dry;
}

void Reverb::set_predelay(float p_predelay) {
	params.predelay = p_predelay;
}

void Reverb::set_predelay_feedback(float p_predelay_fb) {
	params.predelay_fb = p_predelay_fb;
}

void Reverb::set_highpass(float p_frq) {
	if (p_frq > 1) {
		p_frq = 1;
	}
	if (p_frq < 0) {
		p_frq = 0;
	}
	params.hpf = p_frq;
}

void Reverb::set_extra_spread(float p_spread) {
	params.extra_spread = p_spread;
}

void Reverb::set_mix_rate(float p_mix_rate) {
	params.mix_rate = p_mix_rate;
	configure_buffers();
}

void Reverb::set_extra_spread_base(float p_sec) {
	params.extra_spread_base = p_sec;
	configure_buffers();
}

void Reverb::configure_buffers() {
	clear_buffers(); //clear if necessary

	for (int i = 0; i < MAX_COMBS; i++) {
		Comb &c = comb[i];

		c.extra_spread_frames = std::rint(params.extra_spread_base * params.mix_rate);

		int len = std::rint(comb_tunings[i] * params.mix_rate) + c.extra_spread_frames;
		if (len < 5) {
			len = 5; //may this happen?
		}

		c.buffer = memnew_arr(float, len);
		c.pos = 0;
		for (int j = 0; j < len; j++) {
			c.buffer[j] = 0;
		}
		c.size = len;
	}

	for (int i = 0; i < MAX_ALLPASS; i++) {
		AllPass &a = allpass[i];

		a.extra_spread_frames = std::rint(params.extra_spread_base * params.mix_rate);

		int len = std::rint(allpass_tunings[i] * params.mix_rate) + a.extra_spread_frames;
		if (len < 5) {
			len = 5; //may this happen?
		}

		a.buffer = memnew_arr(float, len);
		a.pos = 0;
		for (int j = 0; j < len; j++) {
			a.buffer[j] = 0;
		}
		a.size = len;
	}

	echo_buffer_size = (int)(((float)MAX_ECHO_MS / 1000.0) * params.mix_rate + 1.0);
	echo_buffer = memnew_arr(float, echo_buffer_size);
	for (int i = 0; i < echo_buffer_size; i++) {
		echo_buffer[i] = 0;
	}

	echo_buffer_pos = 0;
}

void Reverb::update_parameters() {
	//more freeverb derived constants
	static const float room_scale = 0.28f;
	static const float room_offset = 0.7f;

	for (int i = 0; i < MAX_COMBS; i++) {
		Comb &c = comb[i];
		c.feedback = room_offset + params.room_size * room_scale;
		if (c.feedback < room_offset) {
			c.feedback = room_offset;
		} else if (c.feedback > (room_offset + room_scale)) {
			c.feedback = (room_offset + room_scale);
		}

		float auxdmp = params.damp / 2.0 + 0.5; //only half the range (0.5 .. 1.0 is enough)
		auxdmp *= auxdmp;

		c.damp = std::exp(-Math::TAU * auxdmp * 10000 / params.mix_rate); // 0 .. 10khz
	}
}

void Reverb::clear_buffers() {
	if (echo_buffer) {
		memdelete_arr(echo_buffer);
	}

	for (int i = 0; i < MAX_COMBS; i++) {
		if (comb[i].buffer) {
			memdelete_arr(comb[i].buffer);
		}

		comb[i].buffer = nullptr;
	}

	for (int i = 0; i < MAX_ALLPASS; i++) {
		if (allpass[i].buffer) {
			memdelete_arr(allpass[i].buffer);
		}

		allpass[i].buffer = nullptr;
	}
}

Reverb::Reverb() {
	params.room_size = 0.8;
	params.damp = 0.5;
	params.dry = 1.0;
	params.wet = 0.0;
	params.mix_rate = 44100;
	params.extra_spread_base = 0;
	params.extra_spread = 1.0;
	params.predelay = 150;
	params.predelay_fb = 0.4;
	params.hpf = 0;

	input_buffer = memnew_arr(float, INPUT_BUFFER_MAX_SIZE);

	configure_buffers();
	update_parameters();
}

Reverb::~Reverb() {
	memdelete_arr(input_buffer);
	clear_buffers();
}

#ifdef PROCESS_ALLPASS
#undef PROCESS_ALLPASS
#endif

// ============================================================================
// Merged from servers/text/text_server.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "servers/rendering/renderer.h"
// Inlined servers/text/text_server.compat.inc
#ifndef DISABLE_DEPRECATED

#include "servers/text/text_server.h"

void TextServer::_font_draw_glyph_bind_compat_104872(const RID& p_font, const RID& p_canvas,
	int64_t p_size, const Vector2& p_pos, int64_t p_index, const Color& p_color) const
{
	font_draw_glyph(p_font, p_canvas, p_size, p_pos, p_index, p_color, 0.0);
}

void TextServer::_font_draw_glyph_outline_bind_compat_104872(const RID& p_font, const RID& p_canvas,
	int64_t p_size, int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
	const Color& p_color) const
{
	font_draw_glyph_outline(p_font, p_canvas, p_size, p_outline_size, p_pos, p_index, p_color, 0.0);
}

void TextServer::_shaped_text_draw_bind_compat_104872(const RID& p_shaped, const RID& p_canvas,
	const Vector2& p_pos, double p_clip_l, double p_clip_r, const Color& p_color) const
{
	shaped_text_draw(p_shaped, p_canvas, p_pos, p_clip_l, p_clip_r, p_color, 0.0);
}

void TextServer::_shaped_text_draw_outline_bind_compat_104872(const RID& p_shaped,
	const RID& p_canvas, const Vector2& p_pos, double p_clip_l, double p_clip_r,
	int64_t p_outline_size, const Color& p_color) const
{
	shaped_text_draw_outline(
		p_shaped, p_canvas, p_pos, p_clip_l, p_clip_r, p_outline_size, p_color, 0.0);
}

#endif // DISABLE_DEPRECATED



#include "servers/text/text_server.h"

#ifndef DISABLE_DEPRECATED
#include "core/string/translation_server.h"
#endif // DISABLE_DEPRECATED

void TextServerManager::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void TextServerManager::finalize()
{
	if (!data) {
		return;
	}
	if (data->primary_interface.is_valid()) {
		data->primary_interface.unref();
	}
	while (data->interfaces.size() > 0) {
		data->interfaces.remove_at(0);
	}
	memdelete(data);
	data = nullptr;
}

void TextServerManager::add_interface(const Ref<TextServer>& p_interface)
{
	ERR_FAIL_NULL(data);
	data->interfaces.append(p_interface);
}

void TextServerManager::remove_interface(const Ref<TextServer>& p_interface)
{
	ERR_FAIL_NULL(data);
	data->interfaces.erase(p_interface);
}

int TextServerManager::get_interface_count()
{
	return data ? data->interfaces.size() : 0;
}

Ref<TextServer> TextServerManager::get_interface(int p_index)
{
	ERR_FAIL_NULL_V(data, nullptr);
	ERR_FAIL_INDEX_V(p_index, data->interfaces.size(), nullptr);
	return data->interfaces[p_index];
}

Ref<TextServer> TextServerManager::find_interface(const String& p_name)
{
	ERR_FAIL_NULL_V(data, nullptr);
	int idx = -1;
	for (int i = 0; i < data->interfaces.size(); i++) {
		if (data->interfaces[i]->get_name() == p_name) {
			idx = i;
			break;
		}
	}

	ERR_FAIL_COND_V_MSG(idx == -1, nullptr, "Interface not found.");
	return data->interfaces[idx];
}

bool Glyph::operator==(const Glyph& p_a) const
{
	return (p_a.index == index) && (p_a.font_rid == font_rid) && (p_a.font_size == font_size) &&
		   (p_a.start == start);
}

bool Glyph::operator!=(const Glyph& p_a) const
{
	return (p_a.index != index) || (p_a.font_rid != font_rid) || (p_a.font_size != font_size) ||
		   (p_a.start != start);
}

bool Glyph::operator<(const Glyph& p_a) const
{
	if (p_a.start == start) {
		if (p_a.count == count) {
			if ((p_a.flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL) {
				return true;
			}
			else {
				return false;
			}
		}
		return p_a.count > count;
	}
	return p_a.start < start;
}

bool Glyph::operator>(const Glyph& p_a) const
{
	if (p_a.start == start) {
		if (p_a.count == count) {
			if ((p_a.flags & TextServer::GRAPHEME_IS_VIRTUAL) == TextServer::GRAPHEME_IS_VIRTUAL) {
				return false;
			}
			else {
				return true;
			}
		}
		return p_a.count < count;
	}
	return p_a.start > start;
}

double TextServer::vp_oversampling = 0.0;

_FORCE_INLINE_ int32_t ot_tag_from_string(const char* p_str, int p_len)
{
	char tag[4];
	uint32_t i;

	if (!p_str || !p_len || !*p_str) {
		return OT_TAG(0, 0, 0, 0);
	}

	if (p_len < 0 || p_len > 4) {
		p_len = 4;
	}
	for (i = 0; i < (uint32_t)p_len && p_str[i]; i++) {
		tag[i] = p_str[i];
	}

	for (; i < 4; i++) {
		tag[i] = ' ';
	}

	return OT_TAG(tag[0], tag[1], tag[2], tag[3]);
}

int64_t TextServer::name_to_tag(const String& p_name) const
{
	// No readable name, use tag string.
	return ot_tag_from_string(p_name.replace("custom_", "").ascii().get_data(), -1);
}

_FORCE_INLINE_ void ot_tag_to_string(int32_t p_tag, char* p_buf)
{
	p_buf[0] = (char)(uint8_t)(p_tag >> 24);
	p_buf[1] = (char)(uint8_t)(p_tag >> 16);
	p_buf[2] = (char)(uint8_t)(p_tag >> 8);
	p_buf[3] = (char)(uint8_t)(p_tag >> 0);
}

String TextServer::tag_to_name(int64_t p_tag) const
{
	// No readable name, use tag string.
	char name[5];
	memset(name, 0, 5);
	ot_tag_to_string(p_tag, name);
	return String("custom_") + String(name);
}

Vector2 TextServer::get_hex_code_box_size(int64_t p_size, int64_t p_index) const
{
	int w = ((p_index <= 0xFF) ? 1 : ((p_index <= 0xFFFF) ? 2 : 3));
	int sp = MAX(0, w - 1);
	int sz = MAX(1, Math::round(p_size / 15.f));

	return Vector2(4 + 3 * w + sp + 1, 15) * sz;
}

void TextServer::_draw_hex_code_box_number(const RID& p_canvas, int64_t p_size,
	const Vector2& p_pos, uint8_t p_index, const Color& p_color) const
{
	static uint8_t chars[] = {0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x77,
		0x1F, 0x4E, 0x3D, 0x4F, 0x47, 0x00};
	uint8_t x = chars[p_index];
	if (x & (1 << 6)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos, Size2(3, 1) * p_size), p_color);
	}
	if (x & (1 << 5)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos + Point2(2, 0) * p_size, Size2(1, 3) * p_size), p_color);
	}
	if (x & (1 << 4)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos + Point2(2, 2) * p_size, Size2(1, 3) * p_size), p_color);
	}
	if (x & (1 << 3)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos + Point2(0, 4) * p_size, Size2(3, 1) * p_size), p_color);
	}
	if (x & (1 << 2)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos + Point2(0, 2) * p_size, Size2(1, 3) * p_size), p_color);
	}
	if (x & (1 << 1)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos, Size2(1, 3) * p_size), p_color);
	}
	if (x & (1 << 0)) {
		Renderer::canvas_item_add_rect(
			p_canvas, Rect2(p_pos + Point2(0, 2) * p_size, Size2(3, 1) * p_size), p_color);
	}
}

void TextServer::draw_hex_code_box(const RID& p_canvas, int64_t p_size, const Vector2& p_pos,
	int64_t p_index, const Color& p_color) const
{
	if (p_index == 0) {
		return;
	}

	int w = ((p_index <= 0xFF) ? 1 : ((p_index <= 0xFFFF) ? 2 : 3));
	int sp = MAX(0, w - 1);
	int sz = MAX(1, Math::round(p_size / 15.f));

	Size2 size = Vector2(4 + 3 * w + sp, 15) * sz;
	Point2 pos = p_pos - Point2i(0, size.y * 0.85);

	// Draw frame.
	Renderer::canvas_item_add_rect(
		p_canvas, Rect2(pos + Point2(0, 0), Size2(sz, size.y)), p_color);
	Renderer::canvas_item_add_rect(
		p_canvas, Rect2(pos + Point2(size.x - sz, 0), Size2(sz, size.y)), p_color);
	Renderer::canvas_item_add_rect(
		p_canvas, Rect2(pos + Point2(0, 0), Size2(size.x, sz)), p_color);
	Renderer::canvas_item_add_rect(
		p_canvas, Rect2(pos + Point2(0, size.y - sz), Size2(size.x, sz)), p_color);

	uint8_t a = p_index & 0x0F;
	uint8_t b = (p_index >> 4) & 0x0F;
	uint8_t c = (p_index >> 8) & 0x0F;
	uint8_t d = (p_index >> 12) & 0x0F;
	uint8_t e = (p_index >> 16) & 0x0F;
	uint8_t f = (p_index >> 20) & 0x0F;

	// Draw hex code.
	if (p_index <= 0xFF) {
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(2, 2) * sz, b, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(2, 8) * sz, a, p_color);
	}
	else if (p_index <= 0xFFFF) {
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(2, 2) * sz, d, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(6, 2) * sz, c, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(2, 8) * sz, b, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(6, 8) * sz, a, p_color);
	}
	else {
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(2, 2) * sz, f, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(6, 2) * sz, e, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(10, 2) * sz, d, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(2, 8) * sz, c, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(6, 8) * sz, b, p_color);
		_draw_hex_code_box_number(p_canvas, sz, pos + Point2(10, 8) * sz, a, p_color);
	}
}

PackedInt32Array TextServer::shaped_text_get_line_breaks_adv(const RID& p_shaped,
	const PackedFloat32Array& p_width, int64_t p_start, bool p_once, uint32_t p_break_flags) const
{
	return PackedInt32Array();
}

PackedInt32Array TextServer::shaped_text_get_line_breaks(
	const RID& p_shaped, double p_width, int64_t p_start, uint32_t p_break_flags) const
{
	return PackedInt32Array();
}

bool TextServer::shaped_text_has_visible_chars(const RID& p_shaped) const
{
	int v_size = shaped_text_get_glyph_count(p_shaped);
	if (v_size == 0) {
		return false;
	}

	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);
	for (int i = 0; i < v_size; i++) {
		if (glyphs[i].index != 0 &&
			(glyphs[i].flags & GRAPHEME_IS_VIRTUAL) != GRAPHEME_IS_VIRTUAL) {
			return true;
		}
	}
	return false;
}

PackedInt32Array TextServer::shaped_text_get_word_breaks(
	const RID& p_shaped, uint32_t p_grapheme_flags, uint32_t p_skip_grapheme_flags) const
{
	PackedInt32Array words;

	const_cast<TextServer*>(this)->shaped_text_update_justification_ops(p_shaped);
	const Vector2i& range = shaped_text_get_range(p_shaped);

	int word_start = range.x;

	const int l_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* l_gl = const_cast<TextServer*>(this)->shaped_text_sort_logical(p_shaped);

	for (int i = 0; i < l_size; i++) {
		if (l_gl[i].count > 0) {
			if ((l_gl[i].flags & p_grapheme_flags) != 0 &&
				(l_gl[i].flags & p_skip_grapheme_flags) == 0) {
				int next = (i == 0) ? l_gl[i].start : l_gl[i - 1].end;
				if (word_start < next) {
					words.push_back(word_start);
					words.push_back(next);
				}
				word_start = l_gl[i].end;
			}
		}
	}
	if (l_size > 0) {
		if (word_start != range.y) {
			words.push_back(word_start);
			words.push_back(range.y);
		}
	}

	return words;
}

CaretInfo TextServer::shaped_text_get_carets(const RID& p_shaped, int64_t p_position) const
{
	TextServer::Orientation orientation = shaped_text_get_orientation(p_shaped);
	const Vector2& range = shaped_text_get_range(p_shaped);
	real_t ascent = shaped_text_get_ascent(p_shaped);
	real_t descent = shaped_text_get_descent(p_shaped);
	real_t height = (ascent + descent) / 2;

	real_t off = 0.0f;
	real_t obj_off = -1.0f;
	CaretInfo caret;
	caret.l_dir = DIRECTION_AUTO;
	caret.t_dir = DIRECTION_AUTO;

	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	for (int i = 0; i < v_size; i++) {
		if (glyphs[i].count > 0) {
			// Skip inline objects.
			if ((glyphs[i].flags & GRAPHEME_IS_EMBEDDED_OBJECT) == GRAPHEME_IS_EMBEDDED_OBJECT &&
				glyphs[i].start == glyphs[i].end) {
				obj_off = glyphs[i].advance;
				continue;
			}
			// Caret before grapheme (top / left).
			if (p_position == glyphs[i].start &&
				((glyphs[i].flags & GRAPHEME_IS_VIRTUAL) != GRAPHEME_IS_VIRTUAL)) {
				real_t advance = 0.f;
				for (int j = 0; j < glyphs[i].count; j++) {
					advance += glyphs[i + j].advance * glyphs[i + j].repeat;
				}
				real_t char_adv = advance / (real_t)(glyphs[i].end - glyphs[i].start);
				Rect2 cr;
				if (orientation == ORIENTATION_HORIZONTAL) {
					if (glyphs[i].start == range.x) {
						cr.size.y = height * 2;
					}
					else {
						cr.size.y = height;
					}
					cr.position.y = -ascent;
					cr.position.x = off;
					if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
						caret.t_dir = DIRECTION_RTL;
						cr.position.x += advance;
						cr.size.x = -char_adv;
					}
					else {
						caret.t_dir = DIRECTION_LTR;
						cr.size.x = char_adv;
					}
				}
				else {
					if (glyphs[i].start == range.x) {
						cr.size.x = height * 2;
					}
					else {
						cr.size.x = height;
					}
					cr.position.x = -ascent;
					cr.position.y = off;
					if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
						caret.t_dir = DIRECTION_RTL;
						cr.position.y += advance;
						cr.size.y = -char_adv;
					}
					else {
						caret.t_dir = DIRECTION_LTR;
						cr.size.y = char_adv;
					}
				}
				caret.t_caret = cr;
			}
			// Caret after grapheme (bottom / right).
			if (p_position == glyphs[i].end &&
				((glyphs[i].flags & GRAPHEME_IS_VIRTUAL) != GRAPHEME_IS_VIRTUAL)) {
				real_t advance = 0.f;
				for (int j = 0; j < glyphs[i].count; j++) {
					advance += glyphs[i + j].advance * glyphs[i + j].repeat;
				}
				real_t char_adv = advance / (real_t)(glyphs[i].end - glyphs[i].start);
				Rect2 cr;
				if (orientation == ORIENTATION_HORIZONTAL) {
					if (glyphs[i].end == range.y) {
						cr.size.y = height * 2;
						cr.position.y = -ascent;
					}
					else {
						cr.size.y = height;
						cr.position.y = -ascent + height;
					}
					cr.position.x = off;
					if ((glyphs[i].flags & GRAPHEME_IS_RTL) != GRAPHEME_IS_RTL) {
						caret.l_dir = DIRECTION_LTR;
						cr.position.x += advance;
						cr.size.x = -char_adv;
					}
					else {
						caret.l_dir = DIRECTION_RTL;
						cr.size.x = char_adv;
					}
				}
				else {
					cr.size.y = 1.0f;
					if (glyphs[i].end == range.y) {
						cr.size.x = height * 2;
						cr.position.x = -ascent;
					}
					else {
						cr.size.x = height;
						cr.position.x = -ascent + height;
					}
					cr.position.y = off;
					if ((glyphs[i].flags & GRAPHEME_IS_RTL) != GRAPHEME_IS_RTL) {
						caret.l_dir = DIRECTION_LTR;
						cr.position.y += advance;
						cr.size.y = -char_adv;
					}
					else {
						caret.l_dir = DIRECTION_RTL;
						cr.position.x += advance;
						cr.size.y = char_adv;
					}
				}
				cr.position.x += MAX(0.0, obj_off); // Prevent split caret when on an inline object.
				caret.l_caret = cr;
			}
			// Caret inside grapheme (middle).
			if (p_position > glyphs[i].start && p_position < glyphs[i].end &&
				(glyphs[i].flags & GRAPHEME_IS_VIRTUAL) != GRAPHEME_IS_VIRTUAL) {
				real_t advance = 0.f;
				for (int j = 0; j < glyphs[i].count; j++) {
					advance += glyphs[i + j].advance * glyphs[i + j].repeat;
				}
				real_t char_adv = advance / (real_t)(glyphs[i].end - glyphs[i].start);
				Rect2 cr;
				if (orientation == ORIENTATION_HORIZONTAL) {
					cr.size.y = height * 2;
					cr.position.y = -ascent;
					if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
						cr.position.x = off + char_adv * (glyphs[i].end - p_position);
						cr.size.x = -char_adv;
					}
					else {
						cr.position.x = off + char_adv * (p_position - glyphs[i].start);
						cr.size.x = char_adv;
					}
				}
				else {
					cr.size.x = height * 2;
					cr.position.x = -ascent;
					if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
						cr.position.y = off + char_adv * (glyphs[i].end - p_position);
						cr.size.y = -char_adv;
					}
					else {
						cr.position.y = off + char_adv * (p_position - glyphs[i].start);
						cr.size.y = char_adv;
					}
				}
				caret.t_caret = cr;
				caret.l_caret = cr;
			}
		}
		off += glyphs[i].advance * glyphs[i].repeat;
		if (obj_off >= 0.0) {
			off += obj_off;
			obj_off = -1.0;
		}
	}
	return caret;
}

TextServer::Direction TextServer::shaped_text_get_dominant_direction_in_range(
	const RID& p_shaped, int64_t p_start, int64_t p_end) const
{
	if (p_start == p_end) {
		return DIRECTION_AUTO;
	}

	int start = MIN(p_start, p_end);
	int end = MAX(p_start, p_end);

	int rtl = 0;
	int ltr = 0;

	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	for (int i = 0; i < v_size; i++) {
		if ((glyphs[i].end > start) && (glyphs[i].start < end)) {
			if (glyphs[i].count > 0) {
				if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
					rtl++;
				}
				else {
					ltr++;
				}
			}
		}
	}
	if (ltr == rtl) {
		return DIRECTION_AUTO;
	}
	else if (ltr > rtl) {
		return DIRECTION_LTR;
	}
	else {
		return DIRECTION_RTL;
	}
}

_FORCE_INLINE_ void _push_range(Vector<Vector2>& r_vector, real_t p_start, real_t p_end)
{
	if (!r_vector.is_empty() &&
		Math::is_equal_approx(r_vector[r_vector.size() - 1].y, p_start, (real_t)UNIT_EPSILON)) {
		r_vector.write[r_vector.size() - 1].y = p_end;
	}
	else {
		r_vector.push_back(Vector2(p_start, p_end));
	}
}

Vector<Vector2> TextServer::shaped_text_get_selection(
	const RID& p_shaped, int64_t p_start, int64_t p_end) const
{
	Vector<Vector2> ranges;

	if (p_start == p_end) {
		return ranges;
	}

	int start = MIN(p_start, p_end);
	int end = MAX(p_start, p_end);

	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	real_t off = 0.0f;
	for (int i = 0; i < v_size; i++) {
		for (int k = 0; k < glyphs[i].repeat; k++) {
			if ((glyphs[i].count > 0) &&
				((glyphs[i].index != 0) ||
					((glyphs[i].flags & GRAPHEME_IS_SPACE) == GRAPHEME_IS_SPACE))) {
				if (glyphs[i].start < end && glyphs[i].end > start) {
					// Grapheme fully in selection range.
					if (glyphs[i].start >= start && glyphs[i].end <= end) {
						real_t advance = 0.f;
						for (int j = 0; j < glyphs[i].count; j++) {
							advance += glyphs[i + j].advance;
						}
						_push_range(ranges, off, off + advance);
					}
					// Only start of grapheme is in selection range.
					if (glyphs[i].start >= start && glyphs[i].end > end) {
						real_t advance = 0.f;
						for (int j = 0; j < glyphs[i].count; j++) {
							advance += glyphs[i + j].advance;
						}
						real_t char_adv = advance / (real_t)(glyphs[i].end - glyphs[i].start);
						if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
							_push_range(
								ranges, off + char_adv * (glyphs[i].end - end), off + advance);
						}
						else {
							_push_range(ranges, off, off + char_adv * (end - glyphs[i].start));
						}
					}
					// Only end of grapheme is in selection range.
					if (glyphs[i].start < start && glyphs[i].end <= end) {
						real_t advance = 0.f;
						for (int j = 0; j < glyphs[i].count; j++) {
							advance += glyphs[i + j].advance;
						}
						real_t char_adv = advance / (real_t)(glyphs[i].end - glyphs[i].start);
						if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
							_push_range(ranges, off, off + char_adv * (glyphs[i].end - start));
						}
						else {
							_push_range(
								ranges, off + char_adv * (start - glyphs[i].start), off + advance);
						}
					}
					// Selection range is within grapheme.
					if (glyphs[i].start < start && glyphs[i].end > end) {
						real_t advance = 0.f;
						for (int j = 0; j < glyphs[i].count; j++) {
							advance += glyphs[i + j].advance;
						}
						real_t char_adv = advance / (real_t)(glyphs[i].end - glyphs[i].start);
						if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
							_push_range(ranges, off + char_adv * (glyphs[i].end - end),
								off + char_adv * (glyphs[i].end - start));
						}
						else {
							_push_range(ranges, off + char_adv * (start - glyphs[i].start),
								off + char_adv * (end - glyphs[i].start));
						}
					}
				}
			}
			off += glyphs[i].advance;
		}
	}

	return ranges;
}

int64_t TextServer::shaped_text_hit_test_grapheme(const RID& p_shaped, double p_coords) const
{
	// Exact grapheme hit test, return -1 if missed.
	double off = 0.0f;

	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	for (int i = 0; i < v_size; i++) {
		for (int j = 0; j < glyphs[i].repeat; j++) {
			if (p_coords >= off && p_coords < off + glyphs[i].advance) {
				return i;
			}
			off += glyphs[i].advance;
		}
	}
	return -1;
}

int64_t TextServer::shaped_text_hit_test_position(const RID& p_shaped, double p_coords) const
{
	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	// Cursor placement hit test.

	// Place caret to the left of the leftmost grapheme, or to position 0 if string is empty.
	if (Math::floor(p_coords) <= 0) {
		if (v_size > 0) {
			if ((glyphs[0].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
				return glyphs[0].end;
			}
			else {
				return glyphs[0].start;
			}
		}
		else {
			return 0;
		}
	}

	// Place caret to the right of the rightmost grapheme, or to position 0 if string is empty.
	if (Math::ceil(p_coords) >= shaped_text_get_width(p_shaped)) {
		if (v_size > 0) {
			if ((glyphs[v_size - 1].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
				return glyphs[v_size - 1].start;
			}
			else {
				return glyphs[v_size - 1].end;
			}
		}
		else {
			return 0;
		}
	}

	real_t off = 0.0f;
	for (int i = 0; i < v_size; i++) {
		if (glyphs[i].count > 0) {
			real_t advance = 0.f;
			for (int j = 0; j < glyphs[i].count; j++) {
				advance += glyphs[i + j].advance * glyphs[i + j].repeat;
			}
			if (((glyphs[i].flags & GRAPHEME_IS_VIRTUAL) == GRAPHEME_IS_VIRTUAL) &&
				(p_coords >= off && p_coords < off + advance)) {
				if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
					return glyphs[i].end;
				}
				else {
					return glyphs[i].start;
				}
			}
			// Ligature, handle mid-grapheme hit.
			if (p_coords >= off && p_coords < off + advance &&
				glyphs[i].end > glyphs[i].start + 1) {
				int cnt = glyphs[i].end - glyphs[i].start;
				real_t char_adv = advance / (real_t)(cnt);
				real_t sub_off = off;
				for (int j = 0; j < cnt; j++) {
					// Place caret to the left of clicked sub-grapheme.
					if (p_coords >= sub_off && p_coords < sub_off + char_adv / 2) {
						if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
							return glyphs[i].end - j;
						}
						else {
							return glyphs[i].start + j;
						}
					}
					// Place caret to the right of clicked sub-grapheme.
					if (p_coords >= sub_off + char_adv / 2 && p_coords < sub_off + char_adv) {
						if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
							return glyphs[i].start + (cnt - 1) - j;
						}
						else {
							return glyphs[i].end - (cnt - 1) + j;
						}
					}
					sub_off += char_adv;
				}
			}
			// Place caret to the left of clicked grapheme.
			if (p_coords >= off && p_coords < off + advance / 2) {
				if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
					return glyphs[i].end;
				}
				else {
					return glyphs[i].start;
				}
			}
			// Place caret to the right of clicked grapheme.
			if (p_coords >= off + advance / 2 && p_coords < off + advance) {
				if ((glyphs[i].flags & GRAPHEME_IS_RTL) == GRAPHEME_IS_RTL) {
					return glyphs[i].start;
				}
				else {
					return glyphs[i].end;
				}
			}
		}
		off += glyphs[i].advance * glyphs[i].repeat;
	}

	return -1;
}

Vector2 TextServer::shaped_text_get_grapheme_bounds(const RID& p_shaped, int64_t p_pos) const
{
	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	real_t off = 0.0f;
	for (int i = 0; i < v_size; i++) {
		if ((glyphs[i].count > 0) &&
			((glyphs[i].index != 0) ||
				((glyphs[i].flags & GRAPHEME_IS_SPACE) == GRAPHEME_IS_SPACE))) {
			if (glyphs[i].start <= p_pos && glyphs[i].end >= p_pos) {
				real_t advance = 0.f;
				for (int j = 0; j < glyphs[i].count; j++) {
					advance += glyphs[i + j].advance;
				}
				return Vector2(off, off + advance);
			}
		}
		off += glyphs[i].advance * glyphs[i].repeat;
	}

	return Vector2();
}

int64_t TextServer::shaped_text_next_grapheme_pos(const RID& p_shaped, int64_t p_pos) const
{
	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);
	for (int i = 0; i < v_size; i++) {
		if (p_pos >= glyphs[i].start && p_pos < glyphs[i].end) {
			return glyphs[i].end;
		}
	}
	return p_pos;
}

int64_t TextServer::shaped_text_prev_grapheme_pos(const RID& p_shaped, int64_t p_pos) const
{
	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);
	for (int i = 0; i < v_size; i++) {
		if (p_pos > glyphs[i].start && p_pos <= glyphs[i].end) {
			return glyphs[i].start;
		}
	}

	return p_pos;
}

int64_t TextServer::shaped_text_prev_character_pos(const RID& p_shaped, int64_t p_pos) const
{
	const PackedInt32Array& chars = shaped_text_get_character_breaks(p_shaped);
	int64_t prev = shaped_text_get_range(p_shaped).x;
	for (const int32_t& E : chars) {
		if (E >= p_pos) {
			return prev;
		}
		prev = E;
	}
	return prev;
}

int64_t TextServer::shaped_text_next_character_pos(const RID& p_shaped, int64_t p_pos) const
{
	const PackedInt32Array& chars = shaped_text_get_character_breaks(p_shaped);
	int64_t prev = shaped_text_get_range(p_shaped).x;
	for (const int32_t& E : chars) {
		if (E > p_pos) {
			return E;
		}
		prev = E;
	}
	return prev;
}

int64_t TextServer::shaped_text_closest_character_pos(const RID& p_shaped, int64_t p_pos) const
{
	const PackedInt32Array& chars = shaped_text_get_character_breaks(p_shaped);
	int64_t prev = shaped_text_get_range(p_shaped).x;
	for (const int32_t& E : chars) {
		if (E == p_pos) {
			return E;
		}
		else if (E > p_pos) {
			if ((E - p_pos) < (p_pos - prev)) {
				return E;
			}
			else {
				return prev;
			}
		}
		prev = E;
	}
	return prev;
}

PackedInt32Array TextServer::string_get_character_breaks(
	const String& p_string, const String& p_language) const
{
	PackedInt32Array ret;
	if (!p_string.is_empty()) {
		ret.resize(p_string.size() - 1);
		for (int i = 0; i < p_string.size() - 1; i++) {
			ret.write[i] = i + 1;
		}
	}
	return ret;
}

void TextServer::shaped_text_draw(const RID& p_shaped, const RID& p_canvas, const Vector2& p_pos,
	double p_clip_l, double p_clip_r, const Color& p_color, float p_oversampling) const
{
	TextServer::Orientation orientation = shaped_text_get_orientation(p_shaped);
	bool hex_codes =
		shaped_text_get_preserve_control(p_shaped) || shaped_text_get_preserve_invalid(p_shaped);

	bool rtl = shaped_text_get_direction(p_shaped) == DIRECTION_RTL;

	int ellipsis_pos = shaped_text_get_ellipsis_pos(p_shaped);
	int trim_pos = shaped_text_get_trim_pos(p_shaped);

	const Glyph* ellipsis_glyphs = shaped_text_get_ellipsis_glyphs(p_shaped);
	int ellipsis_gl_size = shaped_text_get_ellipsis_glyph_count(p_shaped);

	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	Vector2 ofs;
	// Draw RTL ellipsis string when needed.
	if (rtl && ellipsis_pos >= 0) {
		for (int i = ellipsis_gl_size - 1; i >= 0; i--) {
			for (int j = 0; j < ellipsis_glyphs[i].repeat; j++) {
				font_draw_glyph(ellipsis_glyphs[i].font_rid, p_canvas, ellipsis_glyphs[i].font_size,
					ofs + p_pos + Vector2(ellipsis_glyphs[i].x_off, ellipsis_glyphs[i].y_off),
					ellipsis_glyphs[i].index, p_color, p_oversampling);
				if (orientation == ORIENTATION_HORIZONTAL) {
					ofs.x += ellipsis_glyphs[i].advance;
				}
				else {
					ofs.y += ellipsis_glyphs[i].advance;
				}
			}
		}
	}
	// Draw at the baseline.
	for (int i = 0; i < v_size; i++) {
		if (trim_pos >= 0) {
			if (rtl) {
				if (i < trim_pos) {
					continue;
				}
			}
			else {
				if (i >= trim_pos) {
					break;
				}
			}
		}
		for (int j = 0; j < glyphs[i].repeat; j++) {
			if (p_clip_r > 0) {
				// Clip right / bottom.
				if (orientation == ORIENTATION_HORIZONTAL) {
					if (ofs.x + glyphs[i].advance > p_clip_r) {
						return;
					}
				}
				else {
					if (ofs.y + glyphs[i].advance > p_clip_r) {
						return;
					}
				}
			}
			if (p_clip_l > 0) {
				// Clip left / top.
				if (orientation == ORIENTATION_HORIZONTAL) {
					if (ofs.x < p_clip_l) {
						ofs.x += glyphs[i].advance;
						continue;
					}
				}
				else {
					if (ofs.y < p_clip_l) {
						ofs.y += glyphs[i].advance;
						continue;
					}
				}
			}

			if (glyphs[i].font_rid != RID()) {
				font_draw_glyph(glyphs[i].font_rid, p_canvas, glyphs[i].font_size,
					ofs + p_pos + Vector2(glyphs[i].x_off, glyphs[i].y_off), glyphs[i].index,
					p_color, p_oversampling);
			}
			else if (hex_codes &&
					   ((glyphs[i].flags & GRAPHEME_IS_VIRTUAL) != GRAPHEME_IS_VIRTUAL) &&
					   ((glyphs[i].flags & GRAPHEME_IS_EMBEDDED_OBJECT) !=
						   GRAPHEME_IS_EMBEDDED_OBJECT)) {
				TextServer::draw_hex_code_box(p_canvas, glyphs[i].font_size,
					ofs + p_pos + Vector2(glyphs[i].x_off, glyphs[i].y_off), glyphs[i].index,
					p_color);
			}
			if (orientation == ORIENTATION_HORIZONTAL) {
				ofs.x += glyphs[i].advance;
			}
			else {
				ofs.y += glyphs[i].advance;
			}
		}
	}
	// Draw LTR ellipsis string when needed.
	if (!rtl && ellipsis_pos >= 0) {
		for (int i = 0; i < ellipsis_gl_size; i++) {
			for (int j = 0; j < ellipsis_glyphs[i].repeat; j++) {
				font_draw_glyph(ellipsis_glyphs[i].font_rid, p_canvas, ellipsis_glyphs[i].font_size,
					ofs + p_pos + Vector2(ellipsis_glyphs[i].x_off, ellipsis_glyphs[i].y_off),
					ellipsis_glyphs[i].index, p_color, p_oversampling);
				if (orientation == ORIENTATION_HORIZONTAL) {
					ofs.x += ellipsis_glyphs[i].advance;
				}
				else {
					ofs.y += ellipsis_glyphs[i].advance;
				}
			}
		}
	}
}

void TextServer::shaped_text_draw_outline(const RID& p_shaped, const RID& p_canvas,
	const Vector2& p_pos, double p_clip_l, double p_clip_r, int64_t p_outline_size,
	const Color& p_color, float p_oversampling) const
{
	TextServer::Orientation orientation = shaped_text_get_orientation(p_shaped);

	bool rtl = (shaped_text_get_inferred_direction(p_shaped) == DIRECTION_RTL);

	int ellipsis_pos = shaped_text_get_ellipsis_pos(p_shaped);
	int trim_pos = shaped_text_get_trim_pos(p_shaped);

	const Glyph* ellipsis_glyphs = shaped_text_get_ellipsis_glyphs(p_shaped);
	int ellipsis_gl_size = shaped_text_get_ellipsis_glyph_count(p_shaped);

	int v_size = shaped_text_get_glyph_count(p_shaped);
	const Glyph* glyphs = shaped_text_get_glyphs(p_shaped);

	Vector2 ofs;
	// Draw RTL ellipsis string when needed.
	if (rtl && ellipsis_pos >= 0) {
		for (int i = ellipsis_gl_size - 1; i >= 0; i--) {
			for (int j = 0; j < ellipsis_glyphs[i].repeat; j++) {
				font_draw_glyph_outline(ellipsis_glyphs[i].font_rid, p_canvas,
					ellipsis_glyphs[i].font_size, p_outline_size,
					ofs + p_pos + Vector2(ellipsis_glyphs[i].x_off, ellipsis_glyphs[i].y_off),
					ellipsis_glyphs[i].index, p_color, p_oversampling);
				if (orientation == ORIENTATION_HORIZONTAL) {
					ofs.x += ellipsis_glyphs[i].advance;
				}
				else {
					ofs.y += ellipsis_glyphs[i].advance;
				}
			}
		}
	}
	// Draw at the baseline.
	for (int i = 0; i < v_size; i++) {
		if (trim_pos >= 0) {
			if (rtl) {
				if (i < trim_pos) {
					continue;
				}
			}
			else {
				if (i >= trim_pos) {
					break;
				}
			}
		}
		for (int j = 0; j < glyphs[i].repeat; j++) {
			if (p_clip_r > 0) {
				// Clip right / bottom.
				if (orientation == ORIENTATION_HORIZONTAL) {
					if (ofs.x + glyphs[i].advance > p_clip_r) {
						return;
					}
				}
				else {
					if (ofs.y + glyphs[i].advance > p_clip_r) {
						return;
					}
				}
			}
			if (p_clip_l > 0) {
				// Clip left / top.
				if (orientation == ORIENTATION_HORIZONTAL) {
					if (ofs.x < p_clip_l) {
						ofs.x += glyphs[i].advance;
						continue;
					}
				}
				else {
					if (ofs.y < p_clip_l) {
						ofs.y += glyphs[i].advance;
						continue;
					}
				}
			}
			if (glyphs[i].font_rid != RID()) {
				font_draw_glyph_outline(glyphs[i].font_rid, p_canvas, glyphs[i].font_size,
					p_outline_size, ofs + p_pos + Vector2(glyphs[i].x_off, glyphs[i].y_off),
					glyphs[i].index, p_color, p_oversampling);
			}
			if (orientation == ORIENTATION_HORIZONTAL) {
				ofs.x += glyphs[i].advance;
			}
			else {
				ofs.y += glyphs[i].advance;
			}
		}
	}
	// Draw LTR ellipsis string when needed.
	if (!rtl && ellipsis_pos >= 0) {
		for (int i = 0; i < ellipsis_gl_size; i++) {
			for (int j = 0; j < ellipsis_glyphs[i].repeat; j++) {
				font_draw_glyph_outline(ellipsis_glyphs[i].font_rid, p_canvas,
					ellipsis_glyphs[i].font_size, p_outline_size,
					ofs + p_pos + Vector2(ellipsis_glyphs[i].x_off, ellipsis_glyphs[i].y_off),
					ellipsis_glyphs[i].index, p_color, p_oversampling);
				if (orientation == ORIENTATION_HORIZONTAL) {
					ofs.x += ellipsis_glyphs[i].advance;
				}
				else {
					ofs.y += ellipsis_glyphs[i].advance;
				}
			}
		}
	}
}

void TextServer::_diacritics_map_add(const String& p_from, char32_t p_to)
{
	for (int i = 0; i < p_from.size(); i++) {
		diacritics_map[p_from[i]] = p_to;
	}
}

void TextServer::_init_diacritics_map()
{
	diacritics_map.clear();

	// Latin.
	_diacritics_map_add(U"ÀÁÂÃÄÅĀĂĄǍǞǠǺȀȂȦḀẠẢẤẦẨẪẬẮẰẲẴẶ", U'A');
	_diacritics_map_add(U"àáâãäåāăąǎǟǡǻȁȃȧḁẚạảấầẩẫậắằẳẵặ", U'a');
	_diacritics_map_add(U"ǢǼ", U'Æ');
	_diacritics_map_add(U"ǣǽ", U'æ');
	_diacritics_map_add(U"ḂḄḆ", U'B');
	_diacritics_map_add(U"ḃḅḇ", U'b');
	_diacritics_map_add(U"ÇĆĈĊČḈ", U'C');
	_diacritics_map_add(U"çćĉċčḉ", U'c');
	_diacritics_map_add(U"ĎḊḌḎḐḒ", U'D');
	_diacritics_map_add(U"ďḋḍḏḑḓ", U'd');
	_diacritics_map_add(U"ÈÉÊËĒĔĖĘĚȆȨḔḖḘḚḜẸẺẼẾỀỂỄỆ", U'E');
	_diacritics_map_add(U"èéêëēĕėęěȇȩḕḗḙḛḝẹẻẽếềểễệ", U'e');
	_diacritics_map_add(U"Ḟ", U'F');
	_diacritics_map_add(U"ḟ", U'f');
	_diacritics_map_add(U"ĜĞĠĢǦǴḠ", U'G');
	_diacritics_map_add(U"ĝğġģǧǵḡ", U'g');
	_diacritics_map_add(U"ĤȞḢḤḦḨḪ", U'H');
	_diacritics_map_add(U"ĥȟḣḥḧḩḫẖ", U'h');
	_diacritics_map_add(U"ÌÍÎÏĨĪĬĮİǏȈȊḬḮỈỊ", U'I');
	_diacritics_map_add(U"ìíîïĩīĭįıǐȉȋḭḯỉị", U'i');
	_diacritics_map_add(U"Ĵ", U'J');
	_diacritics_map_add(U"ĵ", U'j');
	_diacritics_map_add(U"ĶǨḰḲḴ", U'K');
	_diacritics_map_add(U"ķĸǩḱḳḵ", U'k');
	_diacritics_map_add(U"ĹĻĽĿḶḸḺḼ", U'L');
	_diacritics_map_add(U"ĺļľŀḷḹḻḽ", U'l');
	_diacritics_map_add(U"ḾṀṂ", U'M');
	_diacritics_map_add(U"ḿṁṃ", U'm');
	_diacritics_map_add(U"ÑŃŅŇǸṄṆṈṊ", U'N');
	_diacritics_map_add(U"ñńņňŉǹṅṇṉṋ", U'n');
	_diacritics_map_add(U"ÒÓÔÕÖŌŎŐƠǑǪǬȌȎȪȬȮȰṌṎṐṒỌỎỐỒỔỖỘỚỜỞỠỢ", U'O');
	_diacritics_map_add(U"òóôõöōŏőơǒǫǭȍȏȫȭȯȱṍṏṑṓọỏốồổỗộớờởỡợ", U'o');
	_diacritics_map_add(U"ṔṖ", U'P');
	_diacritics_map_add(U"ṗṕ", U'p');
	_diacritics_map_add(U"ŔŖŘȐȒṘṚṜṞ", U'R');
	_diacritics_map_add(U"ŕŗřȑȓṙṛṝṟ", U'r');
	_diacritics_map_add(U"ŚŜŞŠȘṠṢṤṦṨ", U'S');
	_diacritics_map_add(U"śŝşšſșṡṣṥṧṩẛẜẝ", U's');
	_diacritics_map_add(U"ŢŤȚṪṬṮṰ", U'T');
	_diacritics_map_add(U"ţťțṫṭṯṱẗ", U't');
	_diacritics_map_add(U"ÙÚÛÜŨŪŬŮŰŲƯǓǕǗǙǛȔȖṲṴṶṸṺỤỦỨỪỬỮỰ", U'U');
	_diacritics_map_add(U"ùúûüũūŭůűųưǔǖǘǚǜȕȗṳṵṷṹṻụủứừửữự", U'u');
	_diacritics_map_add(U"ṼṾ", U'V');
	_diacritics_map_add(U"ṽṿ", U'v');
	_diacritics_map_add(U"ŴẀẂẄẆẈ", U'W');
	_diacritics_map_add(U"ŵẁẃẅẇẉẘ", U'w');
	_diacritics_map_add(U"ẊẌ", U'X');
	_diacritics_map_add(U"ẋẍ", U'x');
	_diacritics_map_add(U"ÝŶẎỲỴỶỸỾ", U'Y');
	_diacritics_map_add(U"ýÿŷẏẙỳỵỷỹỿ", U'y');
	_diacritics_map_add(U"ŹŻŽẐẒẔ", U'Z');
	_diacritics_map_add(U"źżžẑẓẕ", U'z');

	// Greek.
	_diacritics_map_add(U"ΆἈἉἊἋἌἍἎἏᾈᾉᾊᾋᾌᾍᾎᾏᾸᾹᾺΆᾼ", U'Α');
	_diacritics_map_add(U"άἀἁἂἃἄἅἆἇὰάᾀᾁᾂᾃᾄᾅᾆᾇᾰᾱᾲᾳᾴᾶᾷ", U'α');
	_diacritics_map_add(U"ΈἘἙἚἛἜἝῈΈ", U'Ε');
	_diacritics_map_add(U"έἐἑἒἓἔἕὲέ", U'ε');
	_diacritics_map_add(U"ΉἨἩἪἫἬἭἮἯᾘᾙᾚᾛᾜᾝᾞᾟῊΉῌ", U'Η');
	_diacritics_map_add(U"ήἠἡἢἣἤἥἦἧὴήᾐᾑᾒᾓᾔᾕᾖᾗῂῃῄῆῇ", U'η');
	_diacritics_map_add(U"ΊΪἸἹἺἻἼἽἾἿῘῙῚΊ", U'Ι');
	_diacritics_map_add(U"ίΐϊἰἱἲἳἴἵἶἷὶίῐῑῒΐῖῗ", U'ι');
	_diacritics_map_add(U"ΌὈὉὊὋὌὍῸΌ", U'Ο');
	_diacritics_map_add(U"όὀὁὂὃὄὅὸό", U'ο');
	_diacritics_map_add(U"Ῥ", U'Ρ');
	_diacritics_map_add(U"ῤῥ", U'ρ');
	_diacritics_map_add(U"ΎΫϓϔὙὛὝὟῨῩῪΎ", U'Υ');
	_diacritics_map_add(U"ΰϋύὐὑὒὓὔὕὖὗὺύῠῡῢΰῦῧ", U'υ');
	_diacritics_map_add(U"ΏὨὩὪὫὬὭὮὯᾨᾩᾪᾫᾬᾭᾮᾯῺΏῼ", U'Ω');
	_diacritics_map_add(U"ώὠὡὢὣὤὥὦὧὼώᾠᾡᾢᾣᾤᾥᾦᾧῲῳῴῶῷ", U'ω');

	// Cyrillic.
	_diacritics_map_add(U"ӐӒ", U'А');
	_diacritics_map_add(U"ӑӓ", U'а');
	_diacritics_map_add(U"ЀЁӖ", U'Е');
	_diacritics_map_add(U"ѐёӗ", U'е');
	_diacritics_map_add(U"Ӛ", U'Ә');
	_diacritics_map_add(U"ӛ", U'ә');
	_diacritics_map_add(U"Ӝ", U'Ж');
	_diacritics_map_add(U"ӝ", U'ж');
	_diacritics_map_add(U"Ӟ", U'З');
	_diacritics_map_add(U"ӟ", U'з');
	_diacritics_map_add(U"Ѓ", U'Г');
	_diacritics_map_add(U"ѓ", U'г');
	_diacritics_map_add(U"Ї", U'І');
	_diacritics_map_add(U"ї", U'і');
	_diacritics_map_add(U"ЍӢӤЙ", U'И');
	_diacritics_map_add(U"ѝӣӥй", U'и');
	_diacritics_map_add(U"Ќ", U'К');
	_diacritics_map_add(U"ќ", U'к');
	_diacritics_map_add(U"Ӧ", U'О');
	_diacritics_map_add(U"ӧ", U'о');
	_diacritics_map_add(U"Ӫ", U'Ө');
	_diacritics_map_add(U"ӫ", U'ө');
	_diacritics_map_add(U"Ӭ", U'Э');
	_diacritics_map_add(U"ӭ", U'э');
	_diacritics_map_add(U"ЎӮӰӲ", U'У');
	_diacritics_map_add(U"ўӯӱӳ", U'у');
	_diacritics_map_add(U"Ӵ", U'Ч');
	_diacritics_map_add(U"ӵ", U'ч');
	_diacritics_map_add(U"Ӹ", U'Ы');
	_diacritics_map_add(U"ӹ", U'ы');
}

String TextServer::strip_diacritics(const String& p_string) const
{
	String result;
	for (int i = 0; i < p_string.length(); i++) {
		if (p_string[i] < 0x02B0 || p_string[i] > 0x036F) { // Skip combining diacritics.
			if (diacritics_map.has(p_string[i])) {
				result += diacritics_map[p_string[i]];
			}
			else {
				result += p_string[i];
			}
		}
	}
	return result;
}

#ifndef DISABLE_DEPRECATED
String TextServer::format_number(const String& p_string, const String& p_language) const
{
	const String lang =
		p_language.is_empty() ? TranslationServer::get_tool_locale() : p_language;
	return TranslationServer::format_number(p_string, lang);
}

String TextServer::parse_number(const String& p_string, const String& p_language) const
{
	const String lang =
		p_language.is_empty() ? TranslationServer::get_tool_locale() : p_language;
	return TranslationServer::parse_number(p_string, lang);
}

String TextServer::percent_sign(const String& p_language) const
{
	const String lang =
		p_language.is_empty() ? TranslationServer::get_tool_locale() : p_language;
	return TranslationServer::get_percent_sign(lang);
}
#endif // DISABLE_DEPRECATED

bool TextServer::is_valid_identifier(const String& p_string) const
{
	return p_string.is_valid_unicode_identifier();
}

bool TextServer::is_valid_letter(uint64_t p_unicode) const { return is_unicode_letter(p_unicode); }

void TextServerManager::set_primary_interface(const Ref<TextServer>& p_interface)
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_COND(p_interface.is_null());
	data->primary_interface = p_interface;
}



// ============================================================================
// Merged from servers/display/display_server.cpp
// ============================================================================

// Inlined servers/display/display_server.compat.inc
#ifndef DISABLE_DEPRECATED

#include "servers/display/display_server.h"

RID DisplayServer::_accessibility_create_sub_text_edit_elements_bind_compat_113459(
	const RID& p_parent_rid, const RID& p_shaped_text, float p_min_height, int p_insert_pos)
{
	return accessibility_create_sub_text_edit_elements(
		p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, false);
}

#endif // DISABLE_DEPRECATED



#include "servers/display/display_server.h"
#include "core/input/input.h"
#include "core/os/os.h"
#include "scene/resources/texture.h"
#include "servers/display/accessibility_server.h"
#include "servers/display/display_server.h"
#include "servers/display/native_menu.h"
#include "servers/rendering/renderer.h"

#if defined(RD_ENABLED)
#include "servers/rendering/rendering_device.h"
#endif

#if defined(VULKAN_ENABLED)
#define Window VulkanX11Window
#define Font VulkanX11Font
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#undef Font
#undef Window
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef Always
#undef Always
#endif
#endif
#if defined(D3D12_ENABLED)
#include "drivers/d3d12/rendering_context_driver_d3d12.h"
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif


bool DisplayServer::window_early_clear_override_enabled = false;
Color DisplayServer::window_early_clear_override_color = Color(0, 0, 0, 0);

DisplayServer::DisplayServerCreate
	DisplayServer::server_create_functions[DisplayServer::MAX_SERVERS] = {{"headless",
		&DisplayServerHeadless::create_func, &DisplayServerHeadless::get_rendering_drivers_func}};

int DisplayServer::server_create_count = 1;

DisplayServerDriver::DisplayServerDriver()
{
	DisplayServer::_bind_driver(this);
}

DisplayServerDriver::~DisplayServerDriver()
{
	DisplayServer::_unbind_driver(this);
}

bool DisplayServerDriver::tts_is_speaking() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return false;
}

bool DisplayServerDriver::tts_is_paused() const
{
	WARN_PRINT("TTS is not supported by this display server.");
	return false;
}

void DisplayServerDriver::tts_pause() { WARN_PRINT("TTS is not supported by this display server."); }

void DisplayServerDriver::tts_resume() { WARN_PRINT("TTS is not supported by this display server."); }

void DisplayServerDriver::tts_speak(const String& p_text, const String& p_voice, int p_volume,
	float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	WARN_PRINT("TTS is not supported by this display server.");
}

PackedStringArray DisplayServerDriver::tts_get_voices_for_language(const String& p_language) const
{
	return PackedStringArray();
}

void DisplayServerDriver::tts_post_utterance_event(
	DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos)
{
}

void DisplayServerDriver::tts_stop() { WARN_PRINT("TTS is not supported by this display server."); }

bool DisplayServerDriver::_get_window_early_clear_override(Color& r_color)
{
	if (DisplayServer::window_early_clear_override_enabled) {
		r_color = DisplayServer::window_early_clear_override_color;
		return true;
	}
	else if (Renderer::data) {
		r_color = Renderer::get_default_clear_color();
		return true;
	}
	else {
		return false;
	}
}

void DisplayServer::set_early_window_clear_color_override(bool p_enabled, Color p_color)
{
	window_early_clear_override_enabled = p_enabled;
	window_early_clear_override_color = p_color;
}

void DisplayServerDriver::mouse_set_mode(DisplayServerEnums::MouseMode p_mode)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

DisplayServerEnums::MouseMode DisplayServerDriver::mouse_get_mode() const
{
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServerDriver::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

DisplayServerEnums::MouseMode DisplayServerDriver::mouse_get_mode_override() const
{
	return DisplayServerEnums::MOUSE_MODE_VISIBLE;
}

void DisplayServerDriver::mouse_set_mode_override_enabled(bool p_override_enabled)
{
	WARN_PRINT("Mouse is not supported by this display server.");
}

bool DisplayServerDriver::mouse_is_mode_override_enabled() const { return false; }

void DisplayServerDriver::warp_mouse(const Point2i& p_position) {}

Point2i DisplayServerDriver::mouse_get_position() const
{
	ERR_FAIL_V_MSG(Point2i(), "Mouse is not supported by this display server.");
}

void DisplayServerDriver::clipboard_set(const String& p_text)
{
	WARN_PRINT("Clipboard is not supported by this display server.");
}

String DisplayServerDriver::clipboard_get() const
{
	ERR_FAIL_V_MSG(String(), "Clipboard is not supported by this display server.");
}

Ref<Image> DisplayServerDriver::clipboard_get_image() const
{
	ERR_FAIL_V_MSG(Ref<Image>(), "Clipboard is not supported by this display server.");
}

bool DisplayServerDriver::clipboard_has() const { return !clipboard_get().is_empty(); }

bool DisplayServerDriver::clipboard_has_image() const { return clipboard_get_image().is_valid(); }

void DisplayServerDriver::clipboard_set_primary(const String& p_text)
{
	WARN_PRINT("Primary clipboard is not supported by this display server.");
}

String DisplayServerDriver::clipboard_get_primary() const
{
	ERR_FAIL_V_MSG(String(), "Primary clipboard is not supported by this display server.");
}

void DisplayServerDriver::screen_set_orientation(
	DisplayServerEnums::ScreenOrientation p_orientation, int p_screen)
{
	WARN_PRINT("Orientation not supported by this display server.");
}

DisplayServerEnums::ScreenOrientation DisplayServerDriver::screen_get_orientation(int p_screen) const
{
	return DisplayServerEnums::SCREEN_LANDSCAPE;
}

float DisplayServerDriver::screen_get_scale(int p_screen) const { return 1.0f; }

bool DisplayServerDriver::is_touchscreen_available() const
{
	return Input::is_initialized() && Input::is_emulating_touch_from_mouse();
}

void DisplayServerDriver::screen_set_keep_on(bool p_enable)
{
	WARN_PRINT("Keeping screen on not supported by this display server.");
}

bool DisplayServerDriver::screen_is_kept_on() const { return false; }

int DisplayServerDriver::get_screen_from_rect(const Rect2& p_rect) const
{
	int nearest_area = 0;
	int pos_screen = DisplayServerEnums::INVALID_SCREEN;
	for (int i = 0; i < get_screen_count(); i++) {
		Rect2i r;
		r.position = screen_get_position(i);
		r.size = screen_get_size(i);
		Rect2 inters = r.intersection(p_rect);
		int area = inters.size.width * inters.size.height;
		if (area > nearest_area) {
			pos_screen = i;
			nearest_area = area;
		}
	}
	return pos_screen;
}

DisplayServerEnums::WindowID DisplayServerDriver::create_sub_window(DisplayServerEnums::WindowMode p_mode,
	DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i& p_rect,
	bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent)
{
	ERR_FAIL_V_MSG(
		DisplayServerEnums::INVALID_WINDOW_ID, "Sub-windows not supported by this display server.");
}

void DisplayServerDriver::show_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_MSG("Sub-windows not supported by this display server.");
}

void DisplayServerDriver::delete_sub_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_MSG("Sub-windows not supported by this display server.");
}

void DisplayServerDriver::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive)
{
	// Do nothing, if not supported.
}

void DisplayServerDriver::window_set_mouse_passthrough(
	const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_MSG("Mouse passthrough not supported by this display server.");
}

void DisplayServerDriver::gl_window_make_current(DisplayServerEnums::WindowID p_window_id)
{
	// noop except in gles
}

void DisplayServerDriver::window_set_ime_active(
	const bool p_active, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("IME not supported by this display server.");
}

void DisplayServerDriver::window_set_ime_position(
	const Point2i& p_pos, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("IME not supported by this display server.");
}

#ifndef DISABLE_DEPRECATED

RID DisplayServerDriver::accessibility_create_element(
	DisplayServerEnums::WindowID p_window, DisplayServerEnums::AccessibilityRole p_role)
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::create_element(
			p_window, (AccessibilityServerEnums::AccessibilityRole)p_role);
	}
	else {
		return RID();
	}
}

RID DisplayServerDriver::accessibility_create_sub_element(
	const RID& p_parent_rid, DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::create_sub_element(
			p_parent_rid, (AccessibilityServerEnums::AccessibilityRole)p_role, p_insert_pos);
	}
	else {
		return RID();
	}
}

RID DisplayServerDriver::accessibility_create_sub_text_edit_elements(const RID& p_parent_rid,
	const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::create_sub_text_edit_elements(
			p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, p_is_last_line);
	}
	else {
		return RID();
	}
}

bool DisplayServerDriver::accessibility_has_element(const RID& p_id) const
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::has_element(p_id);
	}
	else {
		return false;
	}
}

void DisplayServerDriver::accessibility_free_element(const RID& p_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::free_element(p_id);
	}
}

void DisplayServerDriver::accessibility_update_set_focus(const RID& p_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_focus(p_id);
	}
}

RID DisplayServerDriver::accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id) const
{
	if (AccessibilityServer::is_initialized()) {
		return AccessibilityServer::get_window_root(p_window_id);
	}
	else {
		return RID();
	}
}

void DisplayServerDriver::accessibility_set_window_rect(
	DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::set_window_rect(p_window_id, p_rect_out, p_rect_in);
	}
}

void DisplayServerDriver::accessibility_set_window_focused(
	DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::set_window_focused(p_window_id, p_focused);
	}
}

void DisplayServerDriver::accessibility_window_activation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::window_activation_completed(p_window_id);
	}
}

void DisplayServerDriver::accessibility_window_deactivation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::window_deactivation_completed(p_window_id);
	}
}

void DisplayServerDriver::accessibility_update_set_role(
	const RID& p_id, DisplayServerEnums::AccessibilityRole p_role)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_role(
			p_id, (AccessibilityServerEnums::AccessibilityRole)p_role);
	}
}

void DisplayServerDriver::accessibility_update_set_name(const RID& p_id, const String& p_name)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_name(p_id, p_name);
	}
}

void DisplayServerDriver::accessibility_update_set_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_description(p_id, p_description);
	}
}

void DisplayServerDriver::accessibility_update_set_extra_info(
	const RID& p_id, const String& p_name_extra_info)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_extra_info(p_id, p_name_extra_info);
	}
}

void DisplayServerDriver::accessibility_update_set_value(const RID& p_id, const String& p_value)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_value(p_id, p_value);
	}
}

void DisplayServerDriver::accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_tooltip(p_id, p_tooltip);
	}
}

void DisplayServerDriver::accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_bounds(p_id, p_rect);
	}
}

void DisplayServerDriver::accessibility_update_set_transform(
	const RID& p_id, const Transform2D& p_transform)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_transform(p_id, p_transform);
	}
}

void DisplayServerDriver::accessibility_update_add_child(const RID& p_id, const RID& p_child_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_child(p_id, p_child_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_controls(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_controls(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_details(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_details(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_described_by(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_described_by(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_flow_to(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_flow_to(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_labeled_by(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_labeled_by(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_add_related_radio_group(
	const RID& p_id, const RID& p_related_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_related_radio_group(p_id, p_related_id);
	}
}

void DisplayServerDriver::accessibility_update_set_active_descendant(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_active_descendant(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_next_on_line(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_previous_on_line(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_previous_on_line(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_member_of(p_id, p_group_id);
	}
}

void DisplayServerDriver::accessibility_update_set_in_page_link_target(
	const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_in_page_link_target(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_error_message(p_id, p_other_id);
	}
}

void DisplayServerDriver::accessibility_update_set_live(
	const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_live(
			p_id, (AccessibilityServerEnums::AccessibilityLiveMode)p_live);
	}
}

void DisplayServerDriver::accessibility_update_add_custom_action(
	const RID& p_id, int p_action_id, const String& p_action_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_add_custom_action(
			p_id, p_action_id, p_action_description);
	}
}

void DisplayServerDriver::accessibility_update_set_table_row_count(const RID& p_id, int p_count)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_row_count(p_id, p_count);
	}
}

void DisplayServerDriver::accessibility_update_set_table_column_count(const RID& p_id, int p_count)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_column_count(p_id, p_count);
	}
}

void DisplayServerDriver::accessibility_update_set_table_row_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_row_index(p_id, p_index);
	}
}

void DisplayServerDriver::accessibility_update_set_table_column_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_column_index(p_id, p_index);
	}
}

void DisplayServerDriver::accessibility_update_set_table_cell_position(
	const RID& p_id, int p_row_index, int p_column_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_cell_position(
			p_id, p_row_index, p_column_index);
	}
}

void DisplayServerDriver::accessibility_update_set_table_cell_span(
	const RID& p_id, int p_row_span, int p_column_span)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_table_cell_span(
			p_id, p_row_span, p_column_span);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_count(const RID& p_id, int p_size)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_count(p_id, p_size);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_index(const RID& p_id, int p_index)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_index(p_id, p_index);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_level(const RID& p_id, int p_level)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_level(p_id, p_level);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_selected(p_id, p_selected);
	}
}

void DisplayServerDriver::accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_item_expanded(p_id, p_expanded);
	}
}

void DisplayServerDriver::accessibility_update_set_popup_type(
	const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_popup_type(
			p_id, (AccessibilityServerEnums::AccessibilityPopupType)p_popup);
	}
}

void DisplayServerDriver::accessibility_update_set_checked(const RID& p_id, bool p_checekd)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_checked(p_id, p_checekd);
	}
}

void DisplayServerDriver::accessibility_update_set_num_value(const RID& p_id, double p_position)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_value(p_id, p_position);
	}
}

void DisplayServerDriver::accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_range(p_id, p_min, p_max);
	}
}

void DisplayServerDriver::accessibility_update_set_num_step(const RID& p_id, double p_step)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_step(p_id, p_step);
	}
}

void DisplayServerDriver::accessibility_update_set_num_jump(const RID& p_id, double p_jump)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_num_jump(p_id, p_jump);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_x(const RID& p_id, double p_position)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_x(p_id, p_position);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_x_range(
	const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_x_range(p_id, p_min, p_max);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_y(const RID& p_id, double p_position)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_y(p_id, p_position);
	}
}

void DisplayServerDriver::accessibility_update_set_scroll_y_range(
	const RID& p_id, double p_min, double p_max)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_scroll_y_range(p_id, p_min, p_max);
	}
}

void DisplayServerDriver::accessibility_update_set_text_decorations(
	const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_decorations(
			p_id, p_underline, p_strikethrough, p_overline, Color(0, 0, 0, 1));
	}
}

void DisplayServerDriver::accessibility_update_set_text_align(
	const RID& p_id, HorizontalAlignment p_align)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_align(p_id, p_align);
	}
}

void DisplayServerDriver::accessibility_update_set_text_selection(const RID& p_id,
	const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_selection(
			p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
	}
}

void DisplayServerDriver::accessibility_update_set_flag(
	const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_flag(
			p_id, (AccessibilityServerEnums::AccessibilityFlags)p_flag, p_value);
	}
}

void DisplayServerDriver::accessibility_update_set_classname(const RID& p_id, const String& p_classname)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_classname(p_id, p_classname);
	}
}

void DisplayServerDriver::accessibility_update_set_placeholder(
	const RID& p_id, const String& p_placeholder)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_placeholder(p_id, p_placeholder);
	}
}

void DisplayServerDriver::accessibility_update_set_language(const RID& p_id, const String& p_language)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_language(p_id, p_language);
	}
}

void DisplayServerDriver::accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_text_orientation(p_id, p_vertical);
	}
}

void DisplayServerDriver::accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_list_orientation(p_id, p_vertical);
	}
}

void DisplayServerDriver::accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_shortcut(p_id, p_shortcut);
	}
}

void DisplayServerDriver::accessibility_update_set_url(const RID& p_id, const String& p_url)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_url(p_id, p_url);
	}
}

void DisplayServerDriver::accessibility_update_set_role_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_role_description(p_id, p_description);
	}
}

void DisplayServerDriver::accessibility_update_set_state_description(
	const RID& p_id, const String& p_description)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_state_description(p_id, p_description);
	}
}

void DisplayServerDriver::accessibility_update_set_color_value(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_color_value(p_id, p_color);
	}
}

void DisplayServerDriver::accessibility_update_set_background_color(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_background_color(p_id, p_color);
	}
}

void DisplayServerDriver::accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	if (AccessibilityServer::is_initialized()) {
		AccessibilityServer::update_set_foreground_color(p_id, p_color);
	}
}

#endif // DISABLE_DEPRECATED

Point2i DisplayServerDriver::ime_get_selection() const
{
	ERR_FAIL_V_MSG(
		Point2i(), "IME or NOTIFICATION_WM_IME_UPDATE not supported by this display server.");
}

String DisplayServerDriver::ime_get_text() const
{
	ERR_FAIL_V_MSG(
		String(), "IME or NOTIFICATION_WM_IME_UPDATE not supported by this display server.");
}

void DisplayServerDriver::virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect,
	DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start,
	int p_cursor_end)
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
}

void DisplayServerDriver::virtual_keyboard_hide()
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
}

// returns height of the currently shown keyboard (0 if keyboard is hidden)
int DisplayServerDriver::virtual_keyboard_get_height() const
{
	WARN_PRINT("Virtual keyboard not supported by this display server.");
	return 0;
}

bool DisplayServerDriver::has_hardware_keyboard() const { return true; }

void DisplayServerDriver::cursor_set_shape(DisplayServerEnums::CursorShape p_shape)
{
	WARN_PRINT("Cursor shape not supported by this display server.");
}

DisplayServerEnums::CursorShape DisplayServerDriver::cursor_get_shape() const
{
	return DisplayServerEnums::CURSOR_ARROW;
}

void DisplayServerDriver::cursor_set_custom_image(const Ref<Resource>& p_cursor,
	DisplayServerEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	WARN_PRINT("Custom cursor shape not supported by this display server.");
}

bool DisplayServerDriver::get_swap_cancel_ok() { return false; }

void DisplayServerDriver::enable_for_stealing_focus(ProcessID pid) {}

Error DisplayServerDriver::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid,
	const Rect2i& p_rect, bool p_visible, bool p_grab_focus)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServerDriver::request_close_embedded_process(ProcessID p_pid)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

Error DisplayServerDriver::remove_embedded_process(ProcessID p_pid)
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return ERR_UNAVAILABLE;
}

ProcessID DisplayServerDriver::get_focused_process_id()
{
	WARN_PRINT("Embedded process not supported by this display server.");
	return 0;
}

void DisplayServerDriver::beep() const {}

int DisplayServerDriver::keyboard_get_layout_count() const { return 0; }

int DisplayServerDriver::keyboard_get_current_layout() const { return -1; }

void DisplayServerDriver::keyboard_set_current_layout(int p_index) {}

String DisplayServerDriver::keyboard_get_layout_language(int p_index) const { return ""; }

String DisplayServerDriver::keyboard_get_layout_name(int p_index) const { return "Not supported"; }

Key DisplayServerDriver::keyboard_get_keycode_from_physical(Key p_keycode) const
{
	ERR_FAIL_V_MSG(p_keycode, "Not supported by this display server.");
}

Key DisplayServerDriver::keyboard_get_label_from_physical(Key p_keycode) const
{
	ERR_FAIL_V_MSG(p_keycode, "Not supported by this display server.");
}

void DisplayServerDriver::show_emoji_and_symbol_picker() const {}

void DisplayServerDriver::force_process_and_drop_events() {}

void DisplayServerDriver::release_rendering_thread()
{
	WARN_PRINT("Rendering thread not supported by this display server.");
}

void DisplayServerDriver::swap_buffers()
{
	WARN_PRINT("Swap buffers not supported by this display server.");
}

void DisplayServerDriver::set_native_icon(const String& p_filename)
{
	WARN_PRINT("Native icon not supported by this display server.");
}

void DisplayServerDriver::set_icon(const Ref<Image>& p_icon)
{
	WARN_PRINT("Icon not supported by this display server.");
}

void DisplayServerDriver::status_indicator_set_icon(
	DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServerDriver::status_indicator_set_tooltip(
	DisplayServerEnums::IndicatorID p_id, const String& p_tooltip)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

void DisplayServerDriver::status_indicator_set_menu(
	DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

Rect2 DisplayServerDriver::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id) const
{
	WARN_PRINT("Status indicator not supported by this display server.");
	return Rect2();
}

void DisplayServerDriver::delete_status_indicator(DisplayServerEnums::IndicatorID p_id)
{
	WARN_PRINT("Status indicator not supported by this display server.");
}

int64_t DisplayServerDriver::window_get_native_handle(
	DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window) const
{
	WARN_PRINT("Native handle not supported by this display server.");
	return 0;
}

void DisplayServerDriver::window_set_vsync_mode(
	DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("Changing the V-Sync mode is not supported by this display server.");
}

DisplayServerEnums::VSyncMode DisplayServerDriver::window_get_vsync_mode(
	DisplayServerEnums::WindowID p_window) const
{
	WARN_PRINT("Changing the V-Sync mode is not supported by this display server.");
	return DisplayServerEnums::VSyncMode::VSYNC_ENABLED;
}

bool DisplayServerDriver::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

void DisplayServerDriver::window_request_hdr_output(
	const bool p_enable, DisplayServerEnums::WindowID p_window)
{
	if (p_enable) {
		WARN_PRINT("HDR output requested, but it is not supported by this display server.");
	}
}

bool DisplayServerDriver::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

bool DisplayServerDriver::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const
{
	return false;
}

void DisplayServerDriver::window_set_hdr_output_reference_luminance(
	const float p_reference_luminance, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT("Attempting to set reference luminance, but HDR output is not supported by this "
			   "display server.");
}

float DisplayServerDriver::window_get_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return -1.0f;
}

float DisplayServerDriver::window_get_hdr_output_current_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return 0.0f;
}

void DisplayServerDriver::window_set_hdr_output_max_luminance(
	const float p_max_luminance, DisplayServerEnums::WindowID p_window)
{
	WARN_PRINT(
		"Attempting to set max luminance, but HDR output is not supported by this display server.");
}

float DisplayServerDriver::window_get_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return -1.0f;
}

float DisplayServerDriver::window_get_hdr_output_current_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	return 0.0f;
}

float DisplayServerDriver::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const
{
	return 1.0f;
}

DisplayServerEnums::WindowID DisplayServerDriver::get_focused_window() const
{
	return DisplayServerEnums::MAIN_WINDOW_ID; // Proper value for single windows.
}

void DisplayServerDriver::set_context(DisplayServerEnums::Context p_context) {}

Ref<Image> DisplayServerDriver::_get_cursor_image_from_resource(
	const Ref<Resource>& p_cursor, const Vector2& p_hotspot)
{
	Ref<Image> image;
	ERR_FAIL_COND_V_MSG(p_hotspot.x < 0 || p_hotspot.y < 0, image, "Hotspot outside cursor image.");

	Ref<Texture2D> texture = p_cursor;
	if (texture.is_valid()) {
		image = texture->get_image();
	}
	else {
		image = p_cursor;
	}
	ERR_FAIL_COND_V(image.is_null(), image);

	Size2 image_size = image->get_size();
	ERR_FAIL_COND_V_MSG(p_hotspot.x > image_size.width || p_hotspot.y > image_size.height, image,
		"Hotspot outside cursor image.");
	ERR_FAIL_COND_V_MSG(image_size.width > 256 || image_size.height > 256, image,
		"Cursor image too big. Max supported size is 256x256.");
	return image;
}


uint32_t DisplayServerDriver::mouse_get_button_state() const { return 0; }

void DisplayServer::_bind_driver(DisplayServerDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
	Input::set_mouse_mode_func = _input_set_mouse_mode;
	Input::get_mouse_mode_func = _input_get_mouse_mode;
	Input::set_mouse_mode_override_func = _input_set_mouse_mode_override;
	Input::get_mouse_mode_override_func = _input_get_mouse_mode_override;
	Input::set_mouse_mode_override_enabled_func = _input_set_mouse_mode_override_enabled;
	Input::is_mouse_mode_override_enabled_func = _input_is_mouse_mode_override_enabled;
	Input::warp_mouse_func = _input_warp;
	Input::get_current_cursor_shape_func = _input_get_current_cursor_shape;
	Input::set_custom_mouse_cursor_func = _input_set_custom_mouse_cursor_func;
}

void DisplayServer::_unbind_driver(DisplayServerDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

void DisplayServer::register_create_function(
	const char* p_name, CreateFunction p_function, GetRenderingDriversFunction p_get_drivers)
{
	ERR_FAIL_COND(server_create_count == MAX_SERVERS);
	// Headless display server is always last
	server_create_functions[server_create_count] = server_create_functions[server_create_count - 1];
	server_create_functions[server_create_count - 1].name = p_name;
	server_create_functions[server_create_count - 1].create_function = p_function;
	server_create_functions[server_create_count - 1].get_rendering_drivers_function = p_get_drivers;
	server_create_count++;
}

int DisplayServer::get_create_function_count() { return server_create_count; }

const char* DisplayServer::get_create_function_name(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	return server_create_functions[p_index].name;
}

Vector<String> DisplayServer::get_create_function_rendering_drivers(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, Vector<String>());
	return server_create_functions[p_index].get_rendering_drivers_function();
}

DisplayServerDriver* DisplayServer::create(int p_index, const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	if (data && data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	DisplayServerDriver* driver = server_create_functions[p_index].create_function(
		p_rendering_driver, p_mode, p_vsync_mode, p_flags, p_position, p_resolution, p_screen,
		p_context, p_parent_window, r_error);
	if (r_error != OK) {
		if (driver) {
			memdelete(driver);
		}
		return nullptr;
	}
	if (driver) {
		_bind_driver(driver);
	}
	return driver;
}

void DisplayServer::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

void DisplayServer::_input_set_mouse_mode(InputClassEnums::MouseMode p_mode)
{
	if (data && data->driver) {
		data->driver->mouse_set_mode(DisplayServerEnums::MouseMode(p_mode));
	}
}

InputClassEnums::MouseMode DisplayServer::_input_get_mouse_mode()
{
	if (data && data->driver) {
		return InputClassEnums::MouseMode(data->driver->mouse_get_mode());
	}
	return InputClassEnums::MouseMode(DisplayServerEnums::MOUSE_MODE_VISIBLE);
}

void DisplayServer::_input_set_mouse_mode_override(InputClassEnums::MouseMode p_mode)
{
	if (data && data->driver) {
		data->driver->mouse_set_mode_override(DisplayServerEnums::MouseMode(p_mode));
	}
}

InputClassEnums::MouseMode DisplayServer::_input_get_mouse_mode_override()
{
	if (data && data->driver) {
		return InputClassEnums::MouseMode(data->driver->mouse_get_mode_override());
	}
	return InputClassEnums::MouseMode(DisplayServerEnums::MOUSE_MODE_VISIBLE);
}

void DisplayServer::_input_set_mouse_mode_override_enabled(bool p_enabled)
{
	if (data && data->driver) {
		data->driver->mouse_set_mode_override_enabled(p_enabled);
	}
}

bool DisplayServer::_input_is_mouse_mode_override_enabled()
{
	return (data && data->driver) ? data->driver->mouse_is_mode_override_enabled() : false;
}

void DisplayServer::_input_warp(const Vector2& p_to_pos)
{
	if (data && data->driver) {
		data->driver->warp_mouse(p_to_pos);
	}
}

InputClassEnums::CursorShape DisplayServer::_input_get_current_cursor_shape()
{
	if (data && data->driver) {
		return (InputClassEnums::CursorShape)data->driver->cursor_get_shape();
	}
	return (InputClassEnums::CursorShape)DisplayServerEnums::CURSOR_ARROW;
}

void DisplayServer::_input_set_custom_mouse_cursor_func(
	const Ref<Resource>& p_image, InputClassEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	if (data && data->driver) {
		data->driver->cursor_set_custom_image(
			p_image, (DisplayServerEnums::CursorShape)p_shape, p_hotspot);
	}
}

bool DisplayServer::is_rendering_device_supported()
{
#if defined(RD_ENABLED)
	if (RD::data) {
		return true;
	}

	if (supported_rendering_device == DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS) {
		return true;
	}
	else if (supported_rendering_device ==
			   DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE) {
		return false;
	}

	Error err;

#if defined(WINDOWS_ENABLED) || defined(LINUXBSD_ENABLED)
	// On some drivers combining OpenGL and RenderingDevice can result in crash, offload the check
	// to the subprocess.
	List<String> arguments;
	arguments.push_back("--test-rd-support");
	if (is_initialized()) {
		arguments.push_back("--display-driver");
		arguments.push_back(get_name().to_lower());
	}

	String pipe;
	int exitcode = 0;
	err = OS::execute(
		OS::get_executable_path(), arguments, &pipe, &exitcode);
	if (err == OK && exitcode == 0) {
		supported_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
		return true;
	}
	else {
		supported_rendering_device = DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
	}
#else // WINDOWS_ENABLED

	RenderingContextDriver* rcd = nullptr;

#if defined(VULKAN_ENABLED)
	rcd = memnew(RenderingContextDriverVulkan);
#endif
#ifdef D3D12_ENABLED
	if (rcd == nullptr) {
		rcd = memnew(RenderingContextDriverD3D12);
	}
#endif
#ifdef METAL_ENABLED
	if (rcd == nullptr) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		// Eliminate "RenderingContextDriverMetal is only available on iOS 14.0 or newer".
		rcd = memnew(RenderingContextDriverMetal);
		GODOT_CLANG_WARNING_POP
	}
#endif

	if (rcd != nullptr) {
		err = rcd->initialize();
		if (err == OK) {
			err = RenderingDevice::initialize(rcd, rcd->device_get(0));
			RenderingDevice::finalize();
			if (err == OK) {
				// Creating a RenderingDevice is quite slow.
				// Cache the result for future usage, so that it's much faster on subsequent calls.
				supported_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::SUCCESS;
				memdelete(rcd);
				rcd = nullptr;
				return true;
			}
			else {
				supported_rendering_device =
					DisplayServerEnums::RenderingDeviceCreationStatus::FAILURE;
			}
		}

		memdelete(rcd);
		rcd = nullptr;
	}

#endif // WINDOWS_ENABLED
#endif // RD_ENABLED
	return false;
}

bool DisplayServer::can_create_rendering_device() { return true; }

String DisplayServer::get_name()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->get_name();
}

void DisplayServer::set_context(DisplayServerEnums::Context p_context)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_context(p_context);
}

void DisplayServer::set_native_icon(const String& p_filename)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_native_icon(p_filename);
}

void DisplayServer::set_icon(const Ref<Image>& p_icon)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->set_icon(p_icon);
}

bool DisplayServer::has_feature(DisplayServerEnums::Feature p_feature)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->has_feature(p_feature);
}

void DisplayServer::process_events()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->process_events();
}

void DisplayServer::force_process_and_drop_events()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->force_process_and_drop_events();
}

void DisplayServer::release_rendering_thread()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->release_rendering_thread();
}

void DisplayServer::swap_buffers()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->swap_buffers();
}

void DisplayServer::beep()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->beep();
}

bool DisplayServer::tts_is_speaking()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->tts_is_speaking();
}

bool DisplayServer::tts_is_paused()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->tts_is_paused();
}

PackedStringArray DisplayServer::tts_get_voices_for_language(const String& p_language)
{
	ERR_FAIL_COND_V(!is_initialized(), PackedStringArray());
	return data->driver->tts_get_voices_for_language(p_language);
}

void DisplayServer::tts_speak(const String& p_text, const String& p_voice, int p_volume, float p_pitch, float p_rate, int64_t p_utterance_id, bool p_interrupt)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_speak(p_text, p_voice, p_volume, p_pitch, p_rate, p_utterance_id, p_interrupt);
}

void DisplayServer::tts_pause()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_pause();
}

void DisplayServer::tts_resume()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_resume();
}

void DisplayServer::tts_stop()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_stop();
}

void DisplayServer::tts_post_utterance_event(DisplayServerEnums::TTSUtteranceEvent p_event, int64_t p_id, int p_pos)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tts_post_utterance_event(p_event, p_id, p_pos);
}

bool DisplayServer::is_dark_mode_supported()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_dark_mode_supported();
}

bool DisplayServer::is_dark_mode()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_dark_mode();
}

Color DisplayServer::get_accent_color()
{
	ERR_FAIL_COND_V(!is_initialized(), Color());
	return data->driver->get_accent_color();
}

Color DisplayServer::get_base_color()
{
	ERR_FAIL_COND_V(!is_initialized(), Color());
	return data->driver->get_base_color();
}

void DisplayServer::mouse_set_mode(DisplayServerEnums::MouseMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->mouse_set_mode(p_mode);
}

DisplayServerEnums::MouseMode DisplayServer::mouse_get_mode()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::MOUSE_MODE_VISIBLE);
	return data->driver->mouse_get_mode();
}

void DisplayServer::mouse_set_mode_override(DisplayServerEnums::MouseMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->mouse_set_mode_override(p_mode);
}

DisplayServerEnums::MouseMode DisplayServer::mouse_get_mode_override()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::MOUSE_MODE_VISIBLE);
	return data->driver->mouse_get_mode_override();
}

void DisplayServer::mouse_set_mode_override_enabled(bool p_override_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->mouse_set_mode_override_enabled(p_override_enabled);
}

bool DisplayServer::mouse_is_mode_override_enabled()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->mouse_is_mode_override_enabled();
}

void DisplayServer::warp_mouse(const Point2i& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->warp_mouse(p_position);
}

Point2i DisplayServer::mouse_get_position()
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->mouse_get_position();
}

uint32_t DisplayServer::mouse_get_button_state()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->mouse_get_button_state();
}

void DisplayServer::cursor_set_shape(DisplayServerEnums::CursorShape p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->cursor_set_shape(p_shape);
}

DisplayServerEnums::CursorShape DisplayServer::cursor_get_shape()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::CURSOR_ARROW);
	return data->driver->cursor_get_shape();
}

void DisplayServer::cursor_set_custom_image(const Ref<Resource>& p_cursor, DisplayServerEnums::CursorShape p_shape, const Vector2& p_hotspot)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->cursor_set_custom_image(p_cursor, p_shape, p_hotspot);
}

Point2i DisplayServer::ime_get_selection()
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->ime_get_selection();
}

String DisplayServer::ime_get_text()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->ime_get_text();
}

void DisplayServer::virtual_keyboard_show(const String& p_existing_text, const Rect2& p_screen_rect, DisplayServerEnums::VirtualKeyboardType p_type, int p_max_length, int p_cursor_start, int p_cursor_end)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->virtual_keyboard_show(p_existing_text, p_screen_rect, p_type, p_max_length, p_cursor_start, p_cursor_end);
}

void DisplayServer::virtual_keyboard_hide()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->virtual_keyboard_hide();
}

int DisplayServer::virtual_keyboard_get_height()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->virtual_keyboard_get_height();
}

bool DisplayServer::has_hardware_keyboard()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->has_hardware_keyboard();
}

int DisplayServer::keyboard_get_layout_count()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->keyboard_get_layout_count();
}

int DisplayServer::keyboard_get_current_layout()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->keyboard_get_current_layout();
}

void DisplayServer::keyboard_set_current_layout(int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->keyboard_set_current_layout(p_index);
}

String DisplayServer::keyboard_get_layout_language(int p_index)
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->keyboard_get_layout_language(p_index);
}

String DisplayServer::keyboard_get_layout_name(int p_index)
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->keyboard_get_layout_name(p_index);
}

Key DisplayServer::keyboard_get_keycode_from_physical(Key p_keycode)
{
	ERR_FAIL_COND_V(!is_initialized(), p_keycode);
	return data->driver->keyboard_get_keycode_from_physical(p_keycode);
}

Key DisplayServer::keyboard_get_label_from_physical(Key p_keycode)
{
	ERR_FAIL_COND_V(!is_initialized(), p_keycode);
	return data->driver->keyboard_get_label_from_physical(p_keycode);
}

int DisplayServer::tablet_get_driver_count()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->tablet_get_driver_count();
}

String DisplayServer::tablet_get_driver_name(int p_driver)
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->tablet_get_driver_name(p_driver);
}

String DisplayServer::tablet_get_current_driver()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->tablet_get_current_driver();
}

void DisplayServer::tablet_set_current_driver(const String& p_driver)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->tablet_set_current_driver(p_driver);
}

void DisplayServer::clipboard_set(const String& p_text)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->clipboard_set(p_text);
}

String DisplayServer::clipboard_get()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->clipboard_get();
}

Ref<Image> DisplayServer::clipboard_get_image()
{
	ERR_FAIL_COND_V(!is_initialized(), Ref<Image>());
	return data->driver->clipboard_get_image();
}

bool DisplayServer::clipboard_has()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->clipboard_has();
}

bool DisplayServer::clipboard_has_image()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->clipboard_has_image();
}

void DisplayServer::clipboard_set_primary(const String& p_text)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->clipboard_set_primary(p_text);
}

String DisplayServer::clipboard_get_primary()
{
	ERR_FAIL_COND_V(!is_initialized(), String());
	return data->driver->clipboard_get_primary();
}

Rect2i DisplayServer::get_display_safe_area(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2i());
	return data->driver->get_display_safe_area(p_screen);
}

int DisplayServer::get_screen_count()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_screen_count();
}

int DisplayServer::get_primary_screen()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_primary_screen();
}

int DisplayServer::get_keyboard_focus_screen()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_keyboard_focus_screen();
}

int DisplayServer::get_screen_from_rect(const Rect2& p_rect)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_screen_from_rect(p_rect);
}

Point2i DisplayServer::screen_get_position(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->screen_get_position(p_screen);
}

Size2i DisplayServer::screen_get_size(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->screen_get_size(p_screen);
}

Rect2i DisplayServer::screen_get_usable_rect(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2i());
	return data->driver->screen_get_usable_rect(p_screen);
}

int DisplayServer::screen_get_dpi(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->screen_get_dpi(p_screen);
}

float DisplayServer::screen_get_scale(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->screen_get_scale(p_screen);
}

float DisplayServer::screen_get_max_scale()
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->screen_get_max_scale();
}

float DisplayServer::screen_get_refresh_rate(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), SCREEN_REFRESH_RATE_FALLBACK);
	return data->driver->screen_get_refresh_rate(p_screen);
}

Color DisplayServer::screen_get_pixel(const Point2i& p_position)
{
	ERR_FAIL_COND_V(!is_initialized(), Color());
	return data->driver->screen_get_pixel(p_position);
}

Ref<Image> DisplayServer::screen_get_image(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), Ref<Image>());
	return data->driver->screen_get_image(p_screen);
}

Ref<Image> DisplayServer::screen_get_image_rect(const Rect2i& p_rect)
{
	ERR_FAIL_COND_V(!is_initialized(), Ref<Image>());
	return data->driver->screen_get_image_rect(p_rect);
}

bool DisplayServer::is_touchscreen_available()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_touchscreen_available();
}

void DisplayServer::screen_set_orientation(DisplayServerEnums::ScreenOrientation p_orientation, int p_screen)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->screen_set_orientation(p_orientation, p_screen);
}

DisplayServerEnums::ScreenOrientation DisplayServer::screen_get_orientation(int p_screen)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::SCREEN_LANDSCAPE);
	return data->driver->screen_get_orientation(p_screen);
}

void DisplayServer::screen_set_keep_on(bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->screen_set_keep_on(p_enable);
}

bool DisplayServer::screen_is_kept_on()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->screen_is_kept_on();
}

Vector<DisplayServerEnums::WindowID> DisplayServer::get_window_list()
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<DisplayServerEnums::WindowID>());
	return data->driver->get_window_list();
}

DisplayServerEnums::WindowID DisplayServer::create_sub_window(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Rect2i& p_rect, bool p_exclusive, DisplayServerEnums::WindowID p_transient_parent)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->create_sub_window(p_mode, p_vsync_mode, p_flags, p_rect, p_exclusive, p_transient_parent);
}

void DisplayServer::show_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->show_window(p_id);
}

void DisplayServer::delete_sub_window(DisplayServerEnums::WindowID p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->delete_sub_window(p_id);
}

DisplayServerEnums::WindowID DisplayServer::window_get_active_popup()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->window_get_active_popup();
}

void DisplayServer::window_set_popup_safe_rect(DisplayServerEnums::WindowID p_window, const Rect2i& p_rect)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_popup_safe_rect(p_window, p_rect);
}

Rect2i DisplayServer::window_get_popup_safe_rect(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2i());
	return data->driver->window_get_popup_safe_rect(p_window);
}

int64_t DisplayServer::window_get_native_handle(DisplayServerEnums::HandleType p_handle_type, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->window_get_native_handle(p_handle_type, p_window);
}

DisplayServerEnums::WindowID DisplayServer::get_window_at_screen_position(const Point2i& p_position)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->get_window_at_screen_position(p_position);
}

void DisplayServer::window_set_title(const String& p_title, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_title(p_title, p_window);
}

Size2i DisplayServer::window_get_title_size(const String& p_title, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_title_size(p_title, p_window);
}

void DisplayServer::window_set_mouse_passthrough(const Vector<Vector2>& p_region, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_mouse_passthrough(p_region, p_window);
}

int DisplayServer::window_get_current_screen(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->window_get_current_screen(p_window);
}

void DisplayServer::window_set_current_screen(int p_screen, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_current_screen(p_screen, p_window);
}

Point2i DisplayServer::window_get_position(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->window_get_position(p_window);
}

Point2i DisplayServer::window_get_position_with_decorations(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Point2i());
	return data->driver->window_get_position_with_decorations(p_window);
}

void DisplayServer::window_set_position(const Point2i& p_position, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_position(p_position, p_window);
}

void DisplayServer::window_set_transient(DisplayServerEnums::WindowID p_window, DisplayServerEnums::WindowID p_parent)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_transient(p_window, p_parent);
}

void DisplayServer::window_set_exclusive(DisplayServerEnums::WindowID p_window, bool p_exclusive)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_exclusive(p_window, p_exclusive);
}

void DisplayServer::window_set_max_size(const Size2i p_size, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_max_size(p_size, p_window);
}

Size2i DisplayServer::window_get_max_size(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_max_size(p_window);
}

void DisplayServer::window_set_min_size(const Size2i p_size, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_min_size(p_size, p_window);
}

Size2i DisplayServer::window_get_min_size(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_min_size(p_window);
}

void DisplayServer::window_set_size(const Size2i p_size, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_size(p_size, p_window);
}

Size2i DisplayServer::window_get_size(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_size(p_window);
}

Size2i DisplayServer::window_get_size_with_decorations(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Size2i());
	return data->driver->window_get_size_with_decorations(p_window);
}

float DisplayServer::window_get_scale(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->window_get_scale(p_window);
}

void DisplayServer::window_set_mode(DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_mode(p_mode, p_window);
}

DisplayServerEnums::WindowMode DisplayServer::window_get_mode(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::WINDOW_MODE_WINDOWED);
	return data->driver->window_get_mode(p_window);
}

void DisplayServer::window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_vsync_mode(p_vsync_mode, p_window);
}

DisplayServerEnums::VSyncMode DisplayServer::window_get_vsync_mode(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::VSyncMode::VSYNC_ENABLED);
	return data->driver->window_get_vsync_mode(p_window);
}

bool DisplayServer::window_is_hdr_output_supported(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_hdr_output_supported(p_window);
}

void DisplayServer::window_request_hdr_output(const bool p_enable, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_request_hdr_output(p_enable, p_window);
}

bool DisplayServer::window_is_hdr_output_requested(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_hdr_output_requested(p_window);
}

bool DisplayServer::window_is_hdr_output_enabled(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_hdr_output_enabled(p_window);
}

void DisplayServer::window_set_hdr_output_reference_luminance(const float p_reference_luminance, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_hdr_output_reference_luminance(p_reference_luminance, p_window);
}

float DisplayServer::window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), -1.0f);
	return data->driver->window_get_hdr_output_reference_luminance(p_window);
}

float DisplayServer::window_get_hdr_output_current_reference_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0.0f);
	return data->driver->window_get_hdr_output_current_reference_luminance(p_window);
}

void DisplayServer::window_set_hdr_output_max_luminance(const float p_max_luminance, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_hdr_output_max_luminance(p_max_luminance, p_window);
}

float DisplayServer::window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), -1.0f);
	return data->driver->window_get_hdr_output_max_luminance(p_window);
}

float DisplayServer::window_get_hdr_output_current_max_luminance(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 0.0f);
	return data->driver->window_get_hdr_output_current_max_luminance(p_window);
}

void DisplayServer::window_set_icon(const Ref<Image>& p_icon, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_icon(p_icon, p_window);
}

float DisplayServer::window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), 1.0f);
	return data->driver->window_get_output_max_linear_value(p_window);
}

bool DisplayServer::window_is_maximize_allowed(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_maximize_allowed(p_window);
}

void DisplayServer::window_set_flag(DisplayServerEnums::WindowFlags p_flag, bool p_enabled, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_flag(p_flag, p_enabled, p_window);
}

bool DisplayServer::window_get_flag(DisplayServerEnums::WindowFlags p_flag, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_get_flag(p_flag, p_window);
}

void DisplayServer::window_request_attention(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_request_attention(p_window);
}

void DisplayServer::window_set_taskbar_progress_value(float p_value, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_taskbar_progress_value(p_value, p_window);
}

void DisplayServer::window_set_taskbar_progress_state(DisplayServerEnums::ProgressState p_state, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_taskbar_progress_state(p_state, p_window);
}

void DisplayServer::window_move_to_foreground(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_move_to_foreground(p_window);
}

bool DisplayServer::window_is_focused(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_is_focused(p_window);
}

DisplayServerEnums::WindowID DisplayServer::get_focused_window()
{
	ERR_FAIL_COND_V(!is_initialized(), DisplayServerEnums::INVALID_WINDOW_ID);
	return data->driver->get_focused_window();
}

void DisplayServer::window_set_window_buttons_offset(const Vector2i& p_offset, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_window_buttons_offset(p_offset, p_window);
}

Vector3i DisplayServer::window_get_safe_title_margins(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3i());
	return data->driver->window_get_safe_title_margins(p_window);
}

bool DisplayServer::window_can_draw(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_can_draw(p_window);
}

bool DisplayServer::can_any_window_draw()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->can_any_window_draw();
}

void DisplayServer::window_set_ime_active(const bool p_active, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_ime_active(p_active, p_window);
}

void DisplayServer::window_set_ime_position(const Point2i& p_pos, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_ime_position(p_pos, p_window);
}

bool DisplayServer::window_maximize_on_title_dbl_click()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_maximize_on_title_dbl_click();
}

bool DisplayServer::window_minimize_on_title_dbl_click()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->window_minimize_on_title_dbl_click();
}

void DisplayServer::window_start_drag(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_start_drag(p_window);
}

void DisplayServer::window_set_color(const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_set_color(p_color);
}

void DisplayServer::window_start_resize(DisplayServerEnums::WindowResizeEdge p_edge, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->window_start_resize(p_edge, p_window);
}

void DisplayServer::gl_window_make_current(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->gl_window_make_current(p_window_id);
}

bool DisplayServer::is_window_transparency_available()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_window_transparency_available();
}

void DisplayServer::enable_for_stealing_focus(ProcessID pid)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->enable_for_stealing_focus(pid);
}

Error DisplayServer::embed_process(DisplayServerEnums::WindowID p_window, ProcessID p_pid, const Rect2i& p_rect, bool p_visible, bool p_grab_focus)
{
	ERR_FAIL_COND_V(!is_initialized(), ERR_UNAVAILABLE);
	return data->driver->embed_process(p_window, p_pid, p_rect, p_visible, p_grab_focus);
}

Error DisplayServer::request_close_embedded_process(ProcessID p_pid)
{
	ERR_FAIL_COND_V(!is_initialized(), ERR_UNAVAILABLE);
	return data->driver->request_close_embedded_process(p_pid);
}

Error DisplayServer::remove_embedded_process(ProcessID p_pid)
{
	ERR_FAIL_COND_V(!is_initialized(), ERR_UNAVAILABLE);
	return data->driver->remove_embedded_process(p_pid);
}

ProcessID DisplayServer::get_focused_process_id()
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->driver->get_focused_process_id();
}

bool DisplayServer::get_swap_cancel_ok()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->get_swap_cancel_ok();
}

void DisplayServer::show_emoji_and_symbol_picker()
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->show_emoji_and_symbol_picker();
}

void DisplayServer::status_indicator_set_icon(DisplayServerEnums::IndicatorID p_id, const Ref<Texture2D>& p_icon)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->status_indicator_set_icon(p_id, p_icon);
}

void DisplayServer::status_indicator_set_tooltip(DisplayServerEnums::IndicatorID p_id, const String& p_tooltip)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->status_indicator_set_tooltip(p_id, p_tooltip);
}

void DisplayServer::status_indicator_set_menu(DisplayServerEnums::IndicatorID p_id, const RID& p_menu_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->status_indicator_set_menu(p_id, p_menu_rid);
}

Rect2 DisplayServer::status_indicator_get_rect(DisplayServerEnums::IndicatorID p_id)
{
	ERR_FAIL_COND_V(!is_initialized(), Rect2());
	return data->driver->status_indicator_get_rect(p_id);
}

void DisplayServer::delete_status_indicator(DisplayServerEnums::IndicatorID p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->delete_status_indicator(p_id);
}

bool DisplayServer::is_in_pip_mode(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->is_in_pip_mode(p_window);
}

void DisplayServer::pip_mode_enter(DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pip_mode_enter(p_window);
}

void DisplayServer::pip_mode_set_aspect_ratio(int p_numerator, int p_denominator, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pip_mode_set_aspect_ratio(p_numerator, p_denominator, p_window);
}

void DisplayServer::pip_mode_set_auto_enter_on_background(bool p_auto_enter_on_background, DisplayServerEnums::WindowID p_window)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->pip_mode_set_auto_enter_on_background(p_auto_enter_on_background, p_window);
}

int DisplayServer::accessibility_should_increase_contrast()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_should_increase_contrast();
}

int DisplayServer::accessibility_should_reduce_animation()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_should_reduce_animation();
}

int DisplayServer::accessibility_should_reduce_transparency()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_should_reduce_transparency();
}

int DisplayServer::accessibility_screen_reader_active()
{
	ERR_FAIL_COND_V(!is_initialized(), -1);
	return data->driver->accessibility_screen_reader_active();
}

#ifndef DISABLE_DEPRECATED
RID DisplayServer::accessibility_create_element(DisplayServerEnums::WindowID p_window_id, DisplayServerEnums::AccessibilityRole p_role)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_create_element(p_window_id, p_role);
}

RID DisplayServer::accessibility_create_sub_element(const RID& p_parent_rid, DisplayServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_create_sub_element(p_parent_rid, p_role, p_insert_pos);
}

RID DisplayServer::accessibility_create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_create_sub_text_edit_elements(p_parent_rid, p_shaped_text, p_min_height, p_insert_pos, p_is_last_line);
}

bool DisplayServer::accessibility_has_element(const RID& p_id)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->driver->accessibility_has_element(p_id);
}

void DisplayServer::accessibility_free_element(const RID& p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_free_element(p_id);
}

void DisplayServer::accessibility_update_set_focus(const RID& p_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_focus(p_id);
}

RID DisplayServer::accessibility_get_window_root(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->driver->accessibility_get_window_root(p_window_id);
}

void DisplayServer::accessibility_set_window_rect(DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_set_window_rect(p_window_id, p_rect_out, p_rect_in);
}

void DisplayServer::accessibility_set_window_focused(DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_set_window_focused(p_window_id, p_focused);
}

void DisplayServer::accessibility_window_activation_completed(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_window_activation_completed(p_window_id);
}

void DisplayServer::accessibility_window_deactivation_completed(DisplayServerEnums::WindowID p_window_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_window_deactivation_completed(p_window_id);
}

void DisplayServer::accessibility_update_set_role(const RID& p_id, DisplayServerEnums::AccessibilityRole p_role)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_role(p_id, p_role);
}

void DisplayServer::accessibility_update_set_name(const RID& p_id, const String& p_name)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_name(p_id, p_name);
}

void DisplayServer::accessibility_update_set_extra_info(const RID& p_id, const String& p_name_extra_info)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_extra_info(p_id, p_name_extra_info);
}

void DisplayServer::accessibility_update_set_description(const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_description(p_id, p_description);
}

void DisplayServer::accessibility_update_set_value(const RID& p_id, const String& p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_value(p_id, p_value);
}

void DisplayServer::accessibility_update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_tooltip(p_id, p_tooltip);
}

void DisplayServer::accessibility_update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_bounds(p_id, p_rect);
}

void DisplayServer::accessibility_update_set_transform(const RID& p_id, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_transform(p_id, p_transform);
}

void DisplayServer::accessibility_update_add_child(const RID& p_id, const RID& p_child_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_child(p_id, p_child_id);
}

void DisplayServer::accessibility_update_add_related_controls(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_controls(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_details(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_details(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_described_by(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_described_by(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_flow_to(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_flow_to(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_labeled_by(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_labeled_by(p_id, p_related_id);
}

void DisplayServer::accessibility_update_add_related_radio_group(const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_related_radio_group(p_id, p_related_id);
}

void DisplayServer::accessibility_update_set_active_descendant(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_active_descendant(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_next_on_line(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_previous_on_line(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_previous_on_line(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_member_of(p_id, p_group_id);
}

void DisplayServer::accessibility_update_set_in_page_link_target(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_in_page_link_target(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_error_message(p_id, p_other_id);
}

void DisplayServer::accessibility_update_set_live(const RID& p_id, DisplayServerEnums::AccessibilityLiveMode p_live)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_live(p_id, p_live);
}

void DisplayServer::accessibility_update_add_custom_action(const RID& p_id, int p_action_id, const String& p_action_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_add_custom_action(p_id, p_action_id, p_action_description);
}

void DisplayServer::accessibility_update_set_table_row_count(const RID& p_id, int p_count)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_row_count(p_id, p_count);
}

void DisplayServer::accessibility_update_set_table_column_count(const RID& p_id, int p_count)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_column_count(p_id, p_count);
}

void DisplayServer::accessibility_update_set_table_row_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_row_index(p_id, p_index);
}

void DisplayServer::accessibility_update_set_table_column_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_column_index(p_id, p_index);
}

void DisplayServer::accessibility_update_set_table_cell_position(const RID& p_id, int p_row_index, int p_column_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_cell_position(p_id, p_row_index, p_column_index);
}

void DisplayServer::accessibility_update_set_table_cell_span(const RID& p_id, int p_row_span, int p_column_span)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_table_cell_span(p_id, p_row_span, p_column_span);
}

void DisplayServer::accessibility_update_set_list_item_count(const RID& p_id, int p_size)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_count(p_id, p_size);
}

void DisplayServer::accessibility_update_set_list_item_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_index(p_id, p_index);
}

void DisplayServer::accessibility_update_set_list_item_level(const RID& p_id, int p_level)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_level(p_id, p_level);
}

void DisplayServer::accessibility_update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_selected(p_id, p_selected);
}

void DisplayServer::accessibility_update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_item_expanded(p_id, p_expanded);
}

void DisplayServer::accessibility_update_set_popup_type(const RID& p_id, DisplayServerEnums::AccessibilityPopupType p_popup)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_popup_type(p_id, p_popup);
}

void DisplayServer::accessibility_update_set_checked(const RID& p_id, bool p_checekd)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_checked(p_id, p_checekd);
}

void DisplayServer::accessibility_update_set_num_value(const RID& p_id, double p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_value(p_id, p_position);
}

void DisplayServer::accessibility_update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_range(p_id, p_min, p_max);
}

void DisplayServer::accessibility_update_set_num_step(const RID& p_id, double p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_step(p_id, p_step);
}

void DisplayServer::accessibility_update_set_num_jump(const RID& p_id, double p_jump)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_num_jump(p_id, p_jump);
}

void DisplayServer::accessibility_update_set_scroll_x(const RID& p_id, double p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_x(p_id, p_position);
}

void DisplayServer::accessibility_update_set_scroll_x_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_x_range(p_id, p_min, p_max);
}

void DisplayServer::accessibility_update_set_scroll_y(const RID& p_id, double p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_y(p_id, p_position);
}

void DisplayServer::accessibility_update_set_scroll_y_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_scroll_y_range(p_id, p_min, p_max);
}

void DisplayServer::accessibility_update_set_text_decorations(const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_decorations(p_id, p_underline, p_strikethrough, p_overline);
}

void DisplayServer::accessibility_update_set_text_align(const RID& p_id, HorizontalAlignment p_align)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_align(p_id, p_align);
}

void DisplayServer::accessibility_update_set_text_selection(const RID& p_id, const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_selection(p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
}

void DisplayServer::accessibility_update_set_flag(const RID& p_id, DisplayServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_flag(p_id, p_flag, p_value);
}

void DisplayServer::accessibility_update_set_classname(const RID& p_id, const String& p_classname)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_classname(p_id, p_classname);
}

void DisplayServer::accessibility_update_set_placeholder(const RID& p_id, const String& p_placeholder)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_placeholder(p_id, p_placeholder);
}

void DisplayServer::accessibility_update_set_language(const RID& p_id, const String& p_language)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_language(p_id, p_language);
}

void DisplayServer::accessibility_update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_text_orientation(p_id, p_vertical);
}

void DisplayServer::accessibility_update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_list_orientation(p_id, p_vertical);
}

void DisplayServer::accessibility_update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_shortcut(p_id, p_shortcut);
}

void DisplayServer::accessibility_update_set_url(const RID& p_id, const String& p_url)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_url(p_id, p_url);
}

void DisplayServer::accessibility_update_set_role_description(const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_role_description(p_id, p_description);
}

void DisplayServer::accessibility_update_set_state_description(const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_state_description(p_id, p_description);
}

void DisplayServer::accessibility_update_set_color_value(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_color_value(p_id, p_color);
}

void DisplayServer::accessibility_update_set_background_color(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_background_color(p_id, p_color);
}

void DisplayServer::accessibility_update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND(!is_initialized());
	data->driver->accessibility_update_set_foreground_color(p_id, p_color);
}
#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from servers/display/accessibility_server.cpp
// ============================================================================

#include "servers/display/accessibility_server.h"
// --- Inlined from servers/display/accessibility_server_dummy.h ---
#include "servers/display/accessibility_server.h"

class AccessibilityServerDummy : public AccessibilityServerDriver
{
	friend class AccessibilityServer;

protected:
	static AccessibilityServerDriver* create_func(Error& r_error)
	{
		r_error = OK;
		return memnew(AccessibilityServerDummy);
	}

public:
	virtual bool window_create(DisplayServerEnums::WindowID p_window_id, void* p_handle) override
	{
		return true;
	}

	virtual void window_destroy(DisplayServerEnums::WindowID p_window_id) override {}

	virtual RID create_element(DisplayServerEnums::WindowID p_window_id,
		AccessibilityServerEnums::AccessibilityRole p_role) override
	{
		return RID();
	}

	virtual RID create_sub_element(const RID& p_parent_rid,
		AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos = -1) override
	{
		return RID();
	}

	virtual RID create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text,
		float p_min_height, int p_insert_pos = -1, bool p_is_last_line = false) override
	{
		return RID();
	}

	virtual bool has_element(const RID& p_id) const override { return false; }

	virtual void free_element(const RID& p_id) override {}

	virtual RID get_window_root(DisplayServerEnums::WindowID p_window_id) const override
	{
		return RID();
	}

	virtual void update_set_focus(const RID& p_id) override {}

	virtual void set_window_rect(DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out,
		const Rect2& p_rect_in) override
	{
	}

	virtual void set_window_focused(
		DisplayServerEnums::WindowID p_window_id, bool p_focused) override
	{
	}

	virtual void window_activation_completed(DisplayServerEnums::WindowID p_window_id) override {}

	virtual void window_deactivation_completed(DisplayServerEnums::WindowID p_window_id) override {}

	virtual void update_set_role(
		const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role) override
	{
	}

	virtual void update_set_name(const RID& p_id, const String& p_name) override {}

	virtual void update_set_braille_label(const RID& p_id, const String& p_name) override {}

	virtual void update_set_braille_role_description(
		const RID& p_id, const String& p_description) override
	{
	}

	virtual void update_set_extra_info(const RID& p_id, const String& p_name_extra_info) override {}

	virtual void update_set_description(const RID& p_id, const String& p_description) override {}

	virtual void update_set_value(const RID& p_id, const String& p_value) override {}

	virtual void update_set_tooltip(const RID& p_id, const String& p_tooltip) override {}

	virtual void update_set_bounds(const RID& p_id, const Rect2& p_rect) override {}

	virtual void update_set_transform(const RID& p_id, const Transform2D& p_transform) override {}

	virtual void update_add_child(const RID& p_id, const RID& p_child_id) override {}

	virtual void update_add_related_controls(const RID& p_id, const RID& p_related_id) override {}

	virtual void update_add_related_details(const RID& p_id, const RID& p_related_id) override {}

	virtual void update_add_related_described_by(const RID& p_id, const RID& p_related_id) override
	{
	}

	virtual void update_add_related_flow_to(const RID& p_id, const RID& p_related_id) override {}

	virtual void update_add_related_labeled_by(const RID& p_id, const RID& p_related_id) override {}

	virtual void update_add_related_radio_group(const RID& p_id, const RID& p_related_id) override
	{
	}

	virtual void update_set_active_descendant(const RID& p_id, const RID& p_other_id) override {}

	virtual void update_set_next_on_line(const RID& p_id, const RID& p_other_id) override {}

	virtual void update_set_previous_on_line(const RID& p_id, const RID& p_other_id) override {}

	virtual void update_set_member_of(const RID& p_id, const RID& p_group_id) override {}

	virtual void update_set_in_page_link_target(const RID& p_id, const RID& p_other_id) override {}

	virtual void update_set_error_message(const RID& p_id, const RID& p_other_id) override {}

	virtual void update_set_live(
		const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live) override
	{
	}

	virtual void update_add_custom_action(
		const RID& p_id, int p_action_id, const String& p_action_description) override
	{
	}

	virtual void update_set_table_row_count(const RID& p_id, int p_count) override {}

	virtual void update_set_table_column_count(const RID& p_id, int p_count) override {}

	virtual void update_set_table_row_index(const RID& p_id, int p_index) override {}

	virtual void update_set_table_column_index(const RID& p_id, int p_index) override {}

	virtual void update_set_table_cell_position(
		const RID& p_id, int p_row_index, int p_column_index) override
	{
	}

	virtual void update_set_table_cell_span(
		const RID& p_id, int p_row_span, int p_column_span) override
	{
	}

	virtual void update_set_list_item_count(const RID& p_id, int p_size) override {}

	virtual void update_set_list_item_index(const RID& p_id, int p_index) override {}

	virtual void update_set_list_item_level(const RID& p_id, int p_level) override {}

	virtual void update_set_list_item_selected(const RID& p_id, bool p_selected) override {}

	virtual void update_set_list_item_expanded(const RID& p_id, bool p_expanded) override {}

	virtual void update_set_popup_type(
		const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup) override
	{
	}

	virtual void update_set_checked(const RID& p_id, bool p_checekd) override {}

	virtual void update_set_num_value(const RID& p_id, double p_position) override {}

	virtual void update_set_num_range(const RID& p_id, double p_min, double p_max) override {}

	virtual void update_set_num_step(const RID& p_id, double p_step) override {}

	virtual void update_set_num_jump(const RID& p_id, double p_jump) override {}

	virtual void update_set_scroll_x(const RID& p_id, double p_position) override {}

	virtual void update_set_scroll_x_range(const RID& p_id, double p_min, double p_max) override {}

	virtual void update_set_scroll_y(const RID& p_id, double p_position) override {}

	virtual void update_set_scroll_y_range(const RID& p_id, double p_min, double p_max) override {}

	virtual void update_set_text_decorations(const RID& p_id, bool p_underline,
		bool p_strikethrough, bool p_overline, const Color& p_color) override
	{
	}

	virtual void update_set_text_align(const RID& p_id, HorizontalAlignment p_align) override {}

	virtual void update_set_text_selection(const RID& p_id, const RID& p_text_start_id,
		int p_start_char, const RID& p_text_end_id, int p_end_char) override
	{
	}

	virtual void update_set_flag(
		const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag, bool p_value) override
	{
	}

	virtual void update_set_classname(const RID& p_id, const String& p_classname) override {}

	virtual void update_set_placeholder(const RID& p_id, const String& p_placeholder) override {}

	virtual void update_set_language(const RID& p_id, const String& p_language) override {}

	virtual void update_set_text_orientation(const RID& p_id, bool p_vertical) override {}

	virtual void update_set_list_orientation(const RID& p_id, bool p_vertical) override {}

	virtual void update_set_shortcut(const RID& p_id, const String& p_shortcut) override {}

	virtual void update_set_url(const RID& p_id, const String& p_url) override {}

	virtual void update_set_role_description(const RID& p_id, const String& p_description) override
	{
	}

	virtual void update_set_state_description(const RID& p_id, const String& p_description) override
	{
	}

	virtual void update_set_color_value(const RID& p_id, const Color& p_color) override {}

	virtual void update_set_background_color(const RID& p_id, const Color& p_color) override {}

	virtual void update_set_foreground_color(const RID& p_id, const Color& p_color) override {}

	AccessibilityServerDummy() {}

	virtual ~AccessibilityServerDummy() {}
};



// --- End servers/display/accessibility_server_dummy.h ---

AccessibilityServerDriver::AccessibilityServerDriver()
{
	AccessibilityServer::_bind_driver(this);
}

AccessibilityServerDriver::~AccessibilityServerDriver()
{
	AccessibilityServer::_unbind_driver(this);
}

AccessibilityServer::AccessibilityServerCreate
	AccessibilityServer::server_create_functions[AccessibilityServer::MAX_SERVERS] = {
		{"dummy", &AccessibilityServerDummy::create_func}};

int AccessibilityServer::server_create_count = 1;

void AccessibilityServer::_bind_driver(AccessibilityServerDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void AccessibilityServer::_unbind_driver(AccessibilityServerDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

AccessibilityServerDriver* AccessibilityServer::create(int p_index, Error& r_error)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	if (data && data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	AccessibilityServerDriver* driver = server_create_functions[p_index].create_function(r_error);
	if (r_error != OK) {
		if (driver) {
			memdelete(driver);
		}
		return nullptr;
	}
	if (driver) {
		_bind_driver(driver);
	}
	return driver;
}

void AccessibilityServer::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		memdelete(data->driver);
		data->driver = nullptr;
	}
	memdelete(data);
	data = nullptr;
}

void AccessibilityServer::register_create_function(const char* p_name, CreateFunction p_function)
{
	ERR_FAIL_COND(server_create_count == MAX_SERVERS);
	// Dummy server is always last
	server_create_functions[server_create_count] = server_create_functions[server_create_count - 1];
	server_create_functions[server_create_count - 1].name = p_name;
	server_create_functions[server_create_count - 1].create_function = p_function;
	server_create_count++;
}

int AccessibilityServer::get_create_function_count() { return server_create_count; }

const char* AccessibilityServer::get_create_function_name(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, server_create_count, nullptr);
	return server_create_functions[p_index].name;
}

bool AccessibilityServer::is_supported()
{
	return is_initialized() ? data->driver->is_supported() : false;
}

bool AccessibilityServer::window_create(DisplayServerEnums::WindowID p_window_id, void* p_handle)
{
	return is_initialized() ? data->driver->window_create(p_window_id, p_handle) : true;
}

void AccessibilityServer::window_destroy(DisplayServerEnums::WindowID p_window_id)
{
	if (is_initialized()) {
		data->driver->window_destroy(p_window_id);
	}
}

RID AccessibilityServer::create_element(
	DisplayServerEnums::WindowID p_window_id, AccessibilityServerEnums::AccessibilityRole p_role)
{
	return is_initialized() ? data->driver->create_element(p_window_id, p_role) : RID();
}

RID AccessibilityServer::create_sub_element(const RID& p_parent_rid,
	AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	return is_initialized() ? data->driver->create_sub_element(p_parent_rid, p_role, p_insert_pos)
							: RID();
}

RID AccessibilityServer::create_sub_text_edit_elements(const RID& p_parent_rid,
	const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	return is_initialized() ? data->driver->create_sub_text_edit_elements(
								  p_parent_rid, p_shaped_text, p_min_height, p_insert_pos,
								  p_is_last_line)
							: RID();
}

bool AccessibilityServer::has_element(const RID& p_id)
{
	return is_initialized() ? data->driver->has_element(p_id) : false;
}

void AccessibilityServer::free_element(const RID& p_id)
{
	if (is_initialized()) {
		data->driver->free_element(p_id);
	}
}

RID AccessibilityServer::get_window_root(DisplayServerEnums::WindowID p_window_id)
{
	return is_initialized() ? data->driver->get_window_root(p_window_id) : RID();
}

void AccessibilityServer::update_set_focus(const RID& p_id)
{
	if (is_initialized()) {
		data->driver->update_set_focus(p_id);
	}
}

void AccessibilityServer::set_window_rect(
	DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
	if (is_initialized()) {
		data->driver->set_window_rect(p_window_id, p_rect_out, p_rect_in);
	}
}

void AccessibilityServer::set_window_focused(
	DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	if (is_initialized()) {
		data->driver->set_window_focused(p_window_id, p_focused);
	}
}

void AccessibilityServer::window_activation_completed(DisplayServerEnums::WindowID p_window_id)
{
	if (is_initialized()) {
		data->driver->window_activation_completed(p_window_id);
	}
}

void AccessibilityServer::window_deactivation_completed(DisplayServerEnums::WindowID p_window_id)
{
	if (is_initialized()) {
		data->driver->window_deactivation_completed(p_window_id);
	}
}

void AccessibilityServer::update_set_role(
	const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role)
{
	if (is_initialized()) {
		data->driver->update_set_role(p_id, p_role);
	}
}

void AccessibilityServer::update_set_name(const RID& p_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->update_set_name(p_id, p_name);
	}
}

void AccessibilityServer::update_set_braille_label(const RID& p_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->update_set_braille_label(p_id, p_name);
	}
}

void AccessibilityServer::update_set_braille_role_description(
	const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_braille_role_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_extra_info(
	const RID& p_id, const String& p_name_extra_info)
{
	if (is_initialized()) {
		data->driver->update_set_extra_info(p_id, p_name_extra_info);
	}
}

void AccessibilityServer::update_set_description(const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_value(const RID& p_id, const String& p_value)
{
	if (is_initialized()) {
		data->driver->update_set_value(p_id, p_value);
	}
}

void AccessibilityServer::update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	if (is_initialized()) {
		data->driver->update_set_tooltip(p_id, p_tooltip);
	}
}

void AccessibilityServer::update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	if (is_initialized()) {
		data->driver->update_set_bounds(p_id, p_rect);
	}
}

void AccessibilityServer::update_set_transform(const RID& p_id, const Transform2D& p_transform)
{
	if (is_initialized()) {
		data->driver->update_set_transform(p_id, p_transform);
	}
}

void AccessibilityServer::update_add_child(const RID& p_id, const RID& p_child_id)
{
	if (is_initialized()) {
		data->driver->update_add_child(p_id, p_child_id);
	}
}

void AccessibilityServer::update_add_related_controls(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_controls(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_details(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_details(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_described_by(
	const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_described_by(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_flow_to(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_flow_to(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_labeled_by(const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_labeled_by(p_id, p_related_id);
	}
}

void AccessibilityServer::update_add_related_radio_group(
	const RID& p_id, const RID& p_related_id)
{
	if (is_initialized()) {
		data->driver->update_add_related_radio_group(p_id, p_related_id);
	}
}

void AccessibilityServer::update_set_active_descendant(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_active_descendant(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_next_on_line(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_previous_on_line(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_previous_on_line(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	if (is_initialized()) {
		data->driver->update_set_member_of(p_id, p_group_id);
	}
}

void AccessibilityServer::update_set_in_page_link_target(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_in_page_link_target(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	if (is_initialized()) {
		data->driver->update_set_error_message(p_id, p_other_id);
	}
}

void AccessibilityServer::update_set_live(
	const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live)
{
	if (is_initialized()) {
		data->driver->update_set_live(p_id, p_live);
	}
}

void AccessibilityServer::update_add_custom_action(
	const RID& p_id, int p_action_id, const String& p_action_description)
{
	if (is_initialized()) {
		data->driver->update_add_custom_action(p_id, p_action_id, p_action_description);
	}
}

void AccessibilityServer::update_set_table_row_count(const RID& p_id, int p_count)
{
	if (is_initialized()) {
		data->driver->update_set_table_row_count(p_id, p_count);
	}
}

void AccessibilityServer::update_set_table_column_count(const RID& p_id, int p_count)
{
	if (is_initialized()) {
		data->driver->update_set_table_column_count(p_id, p_count);
	}
}

void AccessibilityServer::update_set_table_row_index(const RID& p_id, int p_index)
{
	if (is_initialized()) {
		data->driver->update_set_table_row_index(p_id, p_index);
	}
}

void AccessibilityServer::update_set_table_column_index(const RID& p_id, int p_index)
{
	if (is_initialized()) {
		data->driver->update_set_table_column_index(p_id, p_index);
	}
}

void AccessibilityServer::update_set_table_cell_position(
	const RID& p_id, int p_row_index, int p_column_index)
{
	if (is_initialized()) {
		data->driver->update_set_table_cell_position(p_id, p_row_index, p_column_index);
	}
}

void AccessibilityServer::update_set_table_cell_span(
	const RID& p_id, int p_row_span, int p_column_span)
{
	if (is_initialized()) {
		data->driver->update_set_table_cell_span(p_id, p_row_span, p_column_span);
	}
}

void AccessibilityServer::update_set_list_item_count(const RID& p_id, int p_size)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_count(p_id, p_size);
	}
}

void AccessibilityServer::update_set_list_item_index(const RID& p_id, int p_index)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_index(p_id, p_index);
	}
}

void AccessibilityServer::update_set_list_item_level(const RID& p_id, int p_level)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_level(p_id, p_level);
	}
}

void AccessibilityServer::update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_selected(p_id, p_selected);
	}
}

void AccessibilityServer::update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	if (is_initialized()) {
		data->driver->update_set_list_item_expanded(p_id, p_expanded);
	}
}

void AccessibilityServer::update_set_popup_type(
	const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup)
{
	if (is_initialized()) {
		data->driver->update_set_popup_type(p_id, p_popup);
	}
}

void AccessibilityServer::update_set_checked(const RID& p_id, bool p_checekd)
{
	if (is_initialized()) {
		data->driver->update_set_checked(p_id, p_checekd);
	}
}

void AccessibilityServer::update_set_num_value(const RID& p_id, double p_position)
{
	if (is_initialized()) {
		data->driver->update_set_num_value(p_id, p_position);
	}
}

void AccessibilityServer::update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	if (is_initialized()) {
		data->driver->update_set_num_range(p_id, p_min, p_max);
	}
}

void AccessibilityServer::update_set_num_step(const RID& p_id, double p_step)
{
	if (is_initialized()) {
		data->driver->update_set_num_step(p_id, p_step);
	}
}

void AccessibilityServer::update_set_num_jump(const RID& p_id, double p_jump)
{
	if (is_initialized()) {
		data->driver->update_set_num_jump(p_id, p_jump);
	}
}

void AccessibilityServer::update_set_scroll_x(const RID& p_id, double p_position)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_x(p_id, p_position);
	}
}

void AccessibilityServer::update_set_scroll_x_range(const RID& p_id, double p_min, double p_max)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_x_range(p_id, p_min, p_max);
	}
}

void AccessibilityServer::update_set_scroll_y(const RID& p_id, double p_position)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_y(p_id, p_position);
	}
}

void AccessibilityServer::update_set_scroll_y_range(const RID& p_id, double p_min, double p_max)
{
	if (is_initialized()) {
		data->driver->update_set_scroll_y_range(p_id, p_min, p_max);
	}
}

void AccessibilityServer::update_set_text_decorations(const RID& p_id, bool p_underline,
	bool p_strikethrough, bool p_overline, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_text_decorations(
			p_id, p_underline, p_strikethrough, p_overline, p_color);
	}
}

void AccessibilityServer::update_set_text_align(const RID& p_id, HorizontalAlignment p_align)
{
	if (is_initialized()) {
		data->driver->update_set_text_align(p_id, p_align);
	}
}

void AccessibilityServer::update_set_text_selection(const RID& p_id, const RID& p_text_start_id,
	int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	if (is_initialized()) {
		data->driver->update_set_text_selection(
			p_id, p_text_start_id, p_start_char, p_text_end_id, p_end_char);
	}
}

void AccessibilityServer::update_set_flag(
	const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	if (is_initialized()) {
		data->driver->update_set_flag(p_id, p_flag, p_value);
	}
}

void AccessibilityServer::update_set_classname(const RID& p_id, const String& p_classname)
{
	if (is_initialized()) {
		data->driver->update_set_classname(p_id, p_classname);
	}
}

void AccessibilityServer::update_set_placeholder(const RID& p_id, const String& p_placeholder)
{
	if (is_initialized()) {
		data->driver->update_set_placeholder(p_id, p_placeholder);
	}
}

void AccessibilityServer::update_set_language(const RID& p_id, const String& p_language)
{
	if (is_initialized()) {
		data->driver->update_set_language(p_id, p_language);
	}
}

void AccessibilityServer::update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	if (is_initialized()) {
		data->driver->update_set_text_orientation(p_id, p_vertical);
	}
}

void AccessibilityServer::update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	if (is_initialized()) {
		data->driver->update_set_list_orientation(p_id, p_vertical);
	}
}

void AccessibilityServer::update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	if (is_initialized()) {
		data->driver->update_set_shortcut(p_id, p_shortcut);
	}
}

void AccessibilityServer::update_set_url(const RID& p_id, const String& p_url)
{
	if (is_initialized()) {
		data->driver->update_set_url(p_id, p_url);
	}
}

void AccessibilityServer::update_set_role_description(const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_role_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_state_description(
	const RID& p_id, const String& p_description)
{
	if (is_initialized()) {
		data->driver->update_set_state_description(p_id, p_description);
	}
}

void AccessibilityServer::update_set_color_value(const RID& p_id, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_color_value(p_id, p_color);
	}
}

void AccessibilityServer::update_set_background_color(const RID& p_id, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_background_color(p_id, p_color);
	}
}

void AccessibilityServer::update_set_foreground_color(const RID& p_id, const Color& p_color)
{
	if (is_initialized()) {
		data->driver->update_set_foreground_color(p_id, p_color);
	}
}

// ============================================================================
// Merged from servers/display/display_server_headless.cpp
// ============================================================================

#include "servers/display/display_server.h"

#include "core/input/input.h"
#include "core/input/input_event.h"
#include "servers/display/native_menu.h"
#include "servers/rendering/dummy/rasterizer_dummy.h"

DisplayServerDriver* DisplayServerHeadless::create_func(const String& p_rendering_driver,
	DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode,
	uint32_t p_flags, const Vector2i* p_position, const Vector2i& p_resolution, int p_screen,
	DisplayServerEnums::Context p_context, int64_t p_parent_window, Error& r_error)
{
	r_error = OK;
	RasterizerDummy::make_current();
	return memnew(DisplayServerHeadless());
}

void DisplayServerHeadless::_dispatch_input_events(const InputEvent& p_event)
{
	if (DisplayServerHeadless* ds = DisplayServer::driver_as<DisplayServerHeadless>()) {
		ds->_dispatch_input_event(p_event);
	}
}

void DisplayServerHeadless::process_events() { Input::flush_buffered_events(); }

DisplayServerHeadless::DisplayServerHeadless()
{
	NativeMenu::initialize();
	Input::set_event_dispatch_function(_dispatch_input_events);
}

DisplayServerHeadless::~DisplayServerHeadless()
{
	NativeMenu::finalize();
}

void DisplayServerHeadless::_dispatch_input_event(const InputEvent&) {}

// ============================================================================
// Merged from servers/display/native_menu.cpp
// ============================================================================

#include "servers/display/native_menu.h"
#include "scene/resources/texture.h"

NativeMenuDriver::NativeMenuDriver() { NativeMenu::_bind_driver(this); }

NativeMenuDriver::~NativeMenuDriver() { NativeMenu::_unbind_driver(this); }

bool NativeMenuDriver::has_feature(Feature p_feature) const
{
	(void)p_feature;
	return false;
}

bool NativeMenuDriver::has_system_menu(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	return false;
}

RID NativeMenuDriver::get_system_menu(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

String NativeMenuDriver::get_system_menu_name(SystemMenus p_menu_id) const
{
	switch (p_menu_id) {
	case MAIN_MENU_ID:
		return "Main menu";
	case APPLICATION_MENU_ID:
		return "Application menu";
	case WINDOW_MENU_ID:
		return "Window menu";
	case HELP_MENU_ID:
		return "Help menu";
	case DOCK_MENU_ID:
		return "Dock menu";
	default:
		return "Invalid";
	}
}

String NativeMenuDriver::get_system_menu_text(SystemMenus p_menu_id) const
{
	(void)p_menu_id;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

void NativeMenuDriver::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	(void)p_menu_id;
	(void)p_name;
	WARN_PRINT("Global menus are not supported on this platform.");
}

RID NativeMenuDriver::create_menu()
{
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

bool NativeMenuDriver::has_menu(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::free_menu(const RID& p_rid)
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

Size2 NativeMenuDriver::get_size(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Size2();
}

void NativeMenuDriver::popup(const RID& p_rid, const Vector2i& p_position)
{
	(void)p_rid;
	(void)p_position;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	(void)p_rid;
	(void)p_is_rtl;
	WARN_PRINT("Global menus are not supported on this platform.");
}

bool NativeMenuDriver::is_opened(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::set_minimum_width(const RID& p_rid, float p_width)
{
	(void)p_rid;
	(void)p_width;
	WARN_PRINT("Global menus are not supported on this platform.");
}

float NativeMenuDriver::get_minimum_width(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0.f;
}

int NativeMenuDriver::add_separator(const RID& p_rid, int p_index)
{
	(void)p_rid;
	(void)p_index;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::find_item_index_with_text(const RID& p_rid, const String& p_text) const
{
	(void)p_rid;
	(void)p_text;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::find_item_index_with_submenu(
	const RID& p_rid, const RID& p_submenu_rid) const
{
	if (!has_menu(p_rid) || !has_menu(p_submenu_rid)) {
		return -1;
	}
	int count = get_item_count(p_rid);
	for (int i = 0; i < count; i++) {
		if (p_submenu_rid == get_item_submenu(p_rid, i)) {
			return i;
		}
	}
	return -1;
}

bool NativeMenuDriver::is_item_checked(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_indeterminate(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_checkable(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_radio_checkable(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenuDriver::get_item_text(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

RID NativeMenuDriver::get_item_submenu(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return RID();
}

Key NativeMenuDriver::get_item_accelerator(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Key::NONE;
}

bool NativeMenuDriver::is_item_disabled(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

bool NativeMenuDriver::is_item_hidden(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

String NativeMenuDriver::get_item_tooltip(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return String();
}

int NativeMenuDriver::get_item_state(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::get_item_max_states(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

Ref<Texture2D> NativeMenuDriver::get_item_icon(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return Ref<Texture2D>();
}

int NativeMenuDriver::get_item_indentation_level(const RID& p_rid, int p_idx) const
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

void NativeMenuDriver::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checked;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_indeterminate;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checkable;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_checkable;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_text;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_submenu_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_keycode;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_disabled;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_hidden;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_tooltip;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_state;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_max_states;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_icon;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_level;
	WARN_PRINT("Global menus are not supported on this platform.");
}

int NativeMenuDriver::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	(void)p_rid;
	(void)p_idx;
	(void)p_target_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
	return -1;
}

int NativeMenuDriver::get_item_count(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return 0;
}

bool NativeMenuDriver::is_system_menu(const RID& p_rid) const
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
	return false;
}

void NativeMenuDriver::remove_item(const RID& p_rid, int p_idx)
{
	(void)p_rid;
	(void)p_idx;
	WARN_PRINT("Global menus are not supported on this platform.");
}

void NativeMenuDriver::clear(const RID& p_rid)
{
	(void)p_rid;
	WARN_PRINT("Global menus are not supported on this platform.");
}

/* NativeMenu Static Dispatch */

void NativeMenu::_bind_driver(NativeMenuDriver* p_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->driver = p_driver;
}

void NativeMenu::_unbind_driver(NativeMenuDriver* p_driver)
{
	if (data && data->driver == p_driver) {
		data->driver = nullptr;
	}
}

void NativeMenu::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
	if (!data->driver) {
		memnew(NativeMenuDriver);
	}
}

void NativeMenu::finalize()
{
	if (!data) {
		return;
	}
	if (data->driver) {
		NativeMenuDriver* drv = data->driver;
		data->driver = nullptr;
		memdelete(drv);
	}
	memdelete(data);
	data = nullptr;
}

bool NativeMenu::has_feature(Feature p_feature)
{
	return is_initialized() ? data->driver->has_feature(p_feature) : false;
}

bool NativeMenu::has_system_menu(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->has_system_menu(p_menu_id) : false;
}

RID NativeMenu::get_system_menu(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu(p_menu_id) : RID();
}

String NativeMenu::get_system_menu_name(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu_name(p_menu_id) : String();
}

String NativeMenu::get_system_menu_text(SystemMenus p_menu_id)
{
	return is_initialized() ? data->driver->get_system_menu_text(p_menu_id) : String();
}

void NativeMenu::set_system_menu_text(SystemMenus p_menu_id, const String& p_name)
{
	if (is_initialized()) {
		data->driver->set_system_menu_text(p_menu_id, p_name);
	}
}

RID NativeMenu::create_menu() { return is_initialized() ? data->driver->create_menu() : RID(); }

bool NativeMenu::has_menu(const RID& p_rid)
{
	return is_initialized() ? data->driver->has_menu(p_rid) : false;
}

void NativeMenu::free_menu(const RID& p_rid)
{
	if (is_initialized()) {
		data->driver->free_menu(p_rid);
	}
}

Size2 NativeMenu::get_size(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_size(p_rid) : Size2();
}

void NativeMenu::popup(const RID& p_rid, const Vector2i& p_position)
{
	if (is_initialized()) {
		data->driver->popup(p_rid, p_position);
	}
}

void NativeMenu::set_interface_direction(const RID& p_rid, bool p_is_rtl)
{
	if (is_initialized()) {
		data->driver->set_interface_direction(p_rid, p_is_rtl);
	}
}

void NativeMenu::set_minimum_width(const RID& p_rid, float p_width)
{
	if (is_initialized()) {
		data->driver->set_minimum_width(p_rid, p_width);
	}
}

float NativeMenu::get_minimum_width(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_minimum_width(p_rid) : 0.f;
}

bool NativeMenu::is_opened(const RID& p_rid)
{
	return is_initialized() ? data->driver->is_opened(p_rid) : false;
}

int NativeMenu::add_separator(const RID& p_rid, int p_index)
{
	return is_initialized() ? data->driver->add_separator(p_rid, p_index) : -1;
}

int NativeMenu::find_item_index_with_text(const RID& p_rid, const String& p_text)
{
	return is_initialized() ? data->driver->find_item_index_with_text(p_rid, p_text) : -1;
}

int NativeMenu::find_item_index_with_submenu(const RID& p_rid, const RID& p_submenu_rid)
{
	return is_initialized() ? data->driver->find_item_index_with_submenu(p_rid, p_submenu_rid) : -1;
}

bool NativeMenu::is_item_checked(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_checked(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_indeterminate(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_indeterminate(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_checkable(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_checkable(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_radio_checkable(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_radio_checkable(p_rid, p_idx) : false;
}

String NativeMenu::get_item_text(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_text(p_rid, p_idx) : String();
}

RID NativeMenu::get_item_submenu(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_submenu(p_rid, p_idx) : RID();
}

Key NativeMenu::get_item_accelerator(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_accelerator(p_rid, p_idx) : Key::NONE;
}

bool NativeMenu::is_item_disabled(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_disabled(p_rid, p_idx) : false;
}

bool NativeMenu::is_item_hidden(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->is_item_hidden(p_rid, p_idx) : false;
}

String NativeMenu::get_item_tooltip(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_tooltip(p_rid, p_idx) : String();
}

int NativeMenu::get_item_state(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_state(p_rid, p_idx) : -1;
}

int NativeMenu::get_item_max_states(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_max_states(p_rid, p_idx) : -1;
}

Ref<Texture2D> NativeMenu::get_item_icon(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_icon(p_rid, p_idx) : Ref<Texture2D>();
}

int NativeMenu::get_item_indentation_level(const RID& p_rid, int p_idx)
{
	return is_initialized() ? data->driver->get_item_indentation_level(p_rid, p_idx) : 0;
}

void NativeMenu::set_item_checked(const RID& p_rid, int p_idx, bool p_checked)
{
	if (is_initialized()) {
		data->driver->set_item_checked(p_rid, p_idx, p_checked);
	}
}

void NativeMenu::set_item_indeterminate(const RID& p_rid, int p_idx, bool p_indeterminate)
{
	if (is_initialized()) {
		data->driver->set_item_indeterminate(p_rid, p_idx, p_indeterminate);
	}
}

void NativeMenu::set_item_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	if (is_initialized()) {
		data->driver->set_item_checkable(p_rid, p_idx, p_checkable);
	}
}

void NativeMenu::set_item_radio_checkable(const RID& p_rid, int p_idx, bool p_checkable)
{
	if (is_initialized()) {
		data->driver->set_item_radio_checkable(p_rid, p_idx, p_checkable);
	}
}

void NativeMenu::set_item_text(const RID& p_rid, int p_idx, const String& p_text)
{
	if (is_initialized()) {
		data->driver->set_item_text(p_rid, p_idx, p_text);
	}
}

void NativeMenu::set_item_submenu(const RID& p_rid, int p_idx, const RID& p_submenu_rid)
{
	if (is_initialized()) {
		data->driver->set_item_submenu(p_rid, p_idx, p_submenu_rid);
	}
}

void NativeMenu::set_item_accelerator(const RID& p_rid, int p_idx, Key p_keycode)
{
	if (is_initialized()) {
		data->driver->set_item_accelerator(p_rid, p_idx, p_keycode);
	}
}

void NativeMenu::set_item_disabled(const RID& p_rid, int p_idx, bool p_disabled)
{
	if (is_initialized()) {
		data->driver->set_item_disabled(p_rid, p_idx, p_disabled);
	}
}

void NativeMenu::set_item_hidden(const RID& p_rid, int p_idx, bool p_hidden)
{
	if (is_initialized()) {
		data->driver->set_item_hidden(p_rid, p_idx, p_hidden);
	}
}

void NativeMenu::set_item_tooltip(const RID& p_rid, int p_idx, const String& p_tooltip)
{
	if (is_initialized()) {
		data->driver->set_item_tooltip(p_rid, p_idx, p_tooltip);
	}
}

void NativeMenu::set_item_state(const RID& p_rid, int p_idx, int p_state)
{
	if (is_initialized()) {
		data->driver->set_item_state(p_rid, p_idx, p_state);
	}
}

void NativeMenu::set_item_max_states(const RID& p_rid, int p_idx, int p_max_states)
{
	if (is_initialized()) {
		data->driver->set_item_max_states(p_rid, p_idx, p_max_states);
	}
}

void NativeMenu::set_item_icon(const RID& p_rid, int p_idx, const Ref<Texture2D>& p_icon)
{
	if (is_initialized()) {
		data->driver->set_item_icon(p_rid, p_idx, p_icon);
	}
}

void NativeMenu::set_item_indentation_level(const RID& p_rid, int p_idx, int p_level)
{
	if (is_initialized()) {
		data->driver->set_item_indentation_level(p_rid, p_idx, p_level);
	}
}

int NativeMenu::set_item_index(const RID& p_rid, int p_idx, int p_target_idx)
{
	return is_initialized() ? data->driver->set_item_index(p_rid, p_idx, p_target_idx) : -1;
}

int NativeMenu::get_item_count(const RID& p_rid)
{
	return is_initialized() ? data->driver->get_item_count(p_rid) : 0;
}

bool NativeMenu::is_system_menu(const RID& p_rid)
{
	return is_initialized() ? data->driver->is_system_menu(p_rid) : false;
}

void NativeMenu::remove_item(const RID& p_rid, int p_idx)
{
	if (is_initialized()) {
		data->driver->remove_item(p_rid, p_idx);
	}
}

void NativeMenu::clear(const RID& p_rid)
{
	if (is_initialized()) {
		data->driver->clear(p_rid);
	}
}


// --- Merged from servers/physics_2d/physics_server_2d.cpp ---
#include "servers/physics_2d/physics_server_2d.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED
#include "modules/godot_physics_2d/godot_physics_server_2d.h"
#endif

void PhysicsServer2D::initialize(bool p_using_threads)
{
#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED
	if (is_initialized()) {
		return;
	}
	if (!data) {
		data = memnew(Data);
	}
	data->server = memnew(GodotPhysicsServer2D(p_using_threads));
	data->server->init();
#endif
}

void PhysicsServer2D::finalize()
{
	if (!data) {
		return;
	}
#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED
	if (data->server) {
		data->server->finish();
		memdelete(data->server);
		data->server = nullptr;
	}
#endif
	memdelete(data);
	data = nullptr;
}

#ifdef MODULE_GODOT_PHYSICS_2D_ENABLED

RID PhysicsServer2D::world_boundary_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->world_boundary_shape_create();
}

RID PhysicsServer2D::separation_ray_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->separation_ray_shape_create();
}

RID PhysicsServer2D::segment_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->segment_shape_create();
}

RID PhysicsServer2D::circle_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->circle_shape_create();
}

RID PhysicsServer2D::rectangle_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->rectangle_shape_create();
}

RID PhysicsServer2D::capsule_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->capsule_shape_create();
}

RID PhysicsServer2D::convex_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->convex_polygon_shape_create();
}

RID PhysicsServer2D::concave_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->concave_polygon_shape_create();
}

void PhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->shape_set_custom_solver_bias(p_shape, p_bias);
}

PS2DE::ShapeType PhysicsServer2D::shape_get_type(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::ShapeType());
	return data->server->shape_get_type(p_shape);
}

real_t PhysicsServer2D::shape_get_custom_solver_bias(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->shape_get_custom_solver_bias(p_shape);
}

bool PhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A, const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B, const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->shape_collide(p_shape_A, p_xform_A, p_motion_A, p_shape_B, p_xform_B, p_motion_B, r_results, p_result_max, r_result_count);
}

RID PhysicsServer2D::space_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->space_create();
}

void PhysicsServer2D::space_set_active(RID p_space, bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_active(p_space, p_active);
}

bool PhysicsServer2D::space_is_active(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->space_is_active(p_space);
}

void PhysicsServer2D::space_set_param(RID p_space, PS2DE::SpaceParameter p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_param(p_space, p_param, p_value);
}

real_t PhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_param(p_space, p_param);
}

PhysicsDirectSpaceState2D* PhysicsServer2D::space_get_direct_state(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->space_get_direct_state(p_space);
}

void PhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_debug_contacts(p_space, p_max_contacts);
}

Vector<Vector2> PhysicsServer2D::space_get_contacts(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<Vector2>());
	return data->server->space_get_contacts(p_space);
}

int PhysicsServer2D::space_get_contact_count(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_contact_count(p_space);
}

RID PhysicsServer2D::area_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_create();
}

void PhysicsServer2D::area_set_space(RID p_area, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_space(p_area, p_space);
}

RID PhysicsServer2D::area_get_space(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_space(p_area);
}

void PhysicsServer2D::area_add_shape(RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_add_shape(p_area, p_shape, p_transform, p_disabled);
}

void PhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape(p_area, p_shape_idx, p_shape);
}

void PhysicsServer2D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_transform(p_area, p_shape_idx, p_transform);
}

int PhysicsServer2D::area_get_shape_count(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_shape_count(p_area);
}

RID PhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_shape(p_area, p_shape_idx);
}

Transform2D PhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->server->area_get_shape_transform(p_area, p_shape_idx);
}

void PhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_remove_shape(p_area, p_shape_idx);
}

void PhysicsServer2D::area_clear_shapes(RID p_area)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_clear_shapes(p_area);
}

void PhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_disabled(p_area, p_shape, p_disabled);
}

void PhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_transform(p_area, p_transform);
}

Transform2D PhysicsServer2D::area_get_transform(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->server->area_get_transform(p_area);
}

void PhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_layer(p_area, p_layer);
}

uint32_t PhysicsServer2D::area_get_collision_layer(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_layer(p_area);
}

void PhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_mask(p_area, p_mask);
}

uint32_t PhysicsServer2D::area_get_collision_mask(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_mask(p_area);
}

void PhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_monitorable(p_area, p_monitorable);
}

void PhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_pickable(p_area, p_pickable);
}

RID PhysicsServer2D::body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_create();
}

void PhysicsServer2D::body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_space(p_body, p_space);
}

RID PhysicsServer2D::body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_space(p_body);
}

void PhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_mode(p_body, p_mode);
}

PS2DE::BodyMode PhysicsServer2D::body_get_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::BodyMode());
	return data->server->body_get_mode(p_body);
}

void PhysicsServer2D::body_add_shape(RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_shape(p_body, p_shape, p_transform, p_disabled);
}

void PhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape(p_body, p_shape_idx, p_shape);
}

void PhysicsServer2D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform2D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_transform(p_body, p_shape_idx, p_transform);
}

int PhysicsServer2D::body_get_shape_count(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_shape_count(p_body);
}

RID PhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_shape(p_body, p_shape_idx);
}

Transform2D PhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform2D());
	return data->server->body_get_shape_transform(p_body, p_shape_idx);
}

void PhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_disabled(p_body, p_shape, p_disabled);
}

void PhysicsServer2D::body_set_shape_as_one_way_collision(RID p_body, int p_shape, bool p_enabled, real_t p_margin, const Vector2& p_direction)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_as_one_way_collision(p_body, p_shape, p_enabled, p_margin, p_direction);
}

void PhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_shape(p_body, p_shape_idx);
}

void PhysicsServer2D::body_clear_shapes(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_clear_shapes(p_body);
}

void PhysicsServer2D::body_set_continuous_collision_detection_mode(RID p_body, PS2DE::CCDMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_continuous_collision_detection_mode(p_body, p_mode);
}

PS2DE::CCDMode PhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::CCDMode());
	return data->server->body_get_continuous_collision_detection_mode(p_body);
}

void PhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer2D::body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_layer(p_body);
}

void PhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer2D::body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_mask(p_body);
}

void PhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_priority(p_body, p_priority);
}

real_t PhysicsServer2D::body_get_collision_priority(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_priority(p_body);
}

void PhysicsServer2D::body_reset_mass_properties(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_reset_mass_properties(p_body);
}

void PhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque_impulse(p_body, p_torque);
}

void PhysicsServer2D::body_apply_impulse(RID p_body, const Vector2& p_impulse, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_impulse(p_body, p_impulse, p_position);
}

void PhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_force(p_body, p_force);
}

void PhysicsServer2D::body_apply_force(RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_force(p_body, p_force, p_position);
}

void PhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque(p_body, p_torque);
}

void PhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_central_force(p_body, p_force);
}

void PhysicsServer2D::body_add_constant_force(RID p_body, const Vector2& p_force, const Vector2& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_force(p_body, p_force, p_position);
}

void PhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_torque(p_body, p_torque);
}

void PhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_force(p_body, p_force);
}

Vector2 PhysicsServer2D::body_get_constant_force(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector2());
	return data->server->body_get_constant_force(p_body);
}

void PhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_torque(p_body, p_torque);
}

real_t PhysicsServer2D::body_get_constant_torque(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_constant_torque(p_body);
}

void PhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_axis_velocity(p_body, p_axis_velocity);
}

void PhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_max_contacts_reported(p_body, p_contacts);
}

int PhysicsServer2D::body_get_max_contacts_reported(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_max_contacts_reported(p_body);
}

void PhysicsServer2D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_contacts_reported_depth_threshold(p_body, p_threshold);
}

real_t PhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_contacts_reported_depth_threshold(p_body);
}

void PhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_omit_force_integration(p_body, p_omit);
}

bool PhysicsServer2D::body_is_omitting_force_integration(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_omitting_force_integration(p_body);
}

bool PhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape, const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max, int& r_result_count)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_collide_shape(p_body, p_body_shape, p_shape, p_shape_xform, p_motion, r_results, p_result_max, r_result_count);
}

void PhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_pickable(p_body, p_pickable);
}

PhysicsDirectBodyState2D* PhysicsServer2D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->body_get_direct_state(p_body);
}

bool PhysicsServer2D::body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_test_motion(p_body, p_parameters, r_result);
}

RID PhysicsServer2D::joint_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->joint_create();
}

void PhysicsServer2D::joint_clear(RID p_joint)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_clear(p_joint);
}

void PhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->joint_get_param(p_joint, p_param);
}

void PhysicsServer2D::joint_disable_collisions_between_bodies(RID p_joint, const bool p_disable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_disable_collisions_between_bodies(p_joint, p_disable);
}

bool PhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->joint_is_disabled_collisions_between_bodies(p_joint);
}

void PhysicsServer2D::joint_make_pin(RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_pin(p_joint, p_anchor, p_body_a, p_body_b);
}

void PhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1, const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_groove(p_joint, p_a_groove1, p_a_groove2, p_b_anchor, p_body_a, p_body_b);
}

void PhysicsServer2D::joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_damped_spring(p_joint, p_anchor_a, p_anchor_b, p_body_a, p_body_b);
}

void PhysicsServer2D::pin_joint_set_param(RID p_joint, PS2DE::PinJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->pin_joint_get_param(p_joint, p_param);
}

void PhysicsServer2D::pin_joint_set_flag(RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_flag(p_joint, p_flag, p_enabled);
}

bool PhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->pin_joint_get_flag(p_joint, p_flag);
}

void PhysicsServer2D::damped_spring_joint_set_param(RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->damped_spring_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer2D::damped_spring_joint_get_param(RID p_joint, PS2DE::DampedSpringParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->damped_spring_joint_get_param(p_joint, p_param);
}

PS2DE::JointType PhysicsServer2D::joint_get_type(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), PS2DE::JointType());
	return data->server->joint_get_type(p_joint);
}

void PhysicsServer2D::free_rid(RID p_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->free_rid(p_rid);
}

void PhysicsServer2D::set_active(bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->set_active(p_active);
}

void PhysicsServer2D::init()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->init();
}

void PhysicsServer2D::step(real_t p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->step(p_step);
}

void PhysicsServer2D::sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->sync();
}

void PhysicsServer2D::flush_queries()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->flush_queries();
}

void PhysicsServer2D::end_sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->end_sync();
}

void PhysicsServer2D::finish()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->finish();
}

bool PhysicsServer2D::is_flushing_queries()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->is_flushing_queries();
}

int PhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->get_process_info(p_info);
}

#else // !MODULE_GODOT_PHYSICS_2D_ENABLED

RID PhysicsServer2D::world_boundary_shape_create() { return RID(); }
RID PhysicsServer2D::separation_ray_shape_create() { return RID(); }
RID PhysicsServer2D::segment_shape_create() { return RID(); }
RID PhysicsServer2D::circle_shape_create() { return RID(); }
RID PhysicsServer2D::rectangle_shape_create() { return RID(); }
RID PhysicsServer2D::capsule_shape_create() { return RID(); }
RID PhysicsServer2D::convex_polygon_shape_create() { return RID(); }
RID PhysicsServer2D::concave_polygon_shape_create() { return RID(); }
void PhysicsServer2D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias) {}
PS2DE::ShapeType PhysicsServer2D::shape_get_type(RID p_shape) { return PS2DE::SHAPE_CUSTOM; }
real_t PhysicsServer2D::shape_get_custom_solver_bias(RID p_shape) { return 0; }
bool PhysicsServer2D::shape_collide(RID p_shape_A, const Transform2D& p_xform_A, const Vector2& p_motion_A, RID p_shape_B, const Transform2D& p_xform_B, const Vector2& p_motion_B, Vector2* r_results, int p_result_max, int& r_result_count) { r_result_count = 0; return false; }
RID PhysicsServer2D::space_create() { return RID(); }
void PhysicsServer2D::space_set_active(RID p_space, bool p_active) {}
bool PhysicsServer2D::space_is_active(RID p_space) { return false; }
void PhysicsServer2D::space_set_param(RID p_space, PS2DE::SpaceParameter p_param, real_t p_value) {}
real_t PhysicsServer2D::space_get_param(RID p_space, PS2DE::SpaceParameter p_param) { return 0; }
PhysicsDirectSpaceState2D* PhysicsServer2D::space_get_direct_state(RID p_space) { return nullptr; }
void PhysicsServer2D::space_set_debug_contacts(RID p_space, int p_max_contacts) {}
Vector<Vector2> PhysicsServer2D::space_get_contacts(RID p_space) { return Vector<Vector2>(); }
int PhysicsServer2D::space_get_contact_count(RID p_space) { return 0; }
RID PhysicsServer2D::area_create() { return RID(); }
void PhysicsServer2D::area_set_space(RID p_area, RID p_space) {}
RID PhysicsServer2D::area_get_space(RID p_area) { return RID(); }
void PhysicsServer2D::area_add_shape(RID p_area, RID p_shape, const Transform2D& p_transform, bool p_disabled) {}
void PhysicsServer2D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape) {}
void PhysicsServer2D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform2D& p_transform) {}
int PhysicsServer2D::area_get_shape_count(RID p_area) { return 0; }
RID PhysicsServer2D::area_get_shape(RID p_area, int p_shape_idx) { return RID(); }
Transform2D PhysicsServer2D::area_get_shape_transform(RID p_area, int p_shape_idx) { return Transform2D(); }
void PhysicsServer2D::area_remove_shape(RID p_area, int p_shape_idx) {}
void PhysicsServer2D::area_clear_shapes(RID p_area) {}
void PhysicsServer2D::area_set_shape_disabled(RID p_area, int p_shape, bool p_disabled) {}
void PhysicsServer2D::area_set_transform(RID p_area, const Transform2D& p_transform) {}
Transform2D PhysicsServer2D::area_get_transform(RID p_area) { return Transform2D(); }
void PhysicsServer2D::area_set_collision_layer(RID p_area, uint32_t p_layer) {}
uint32_t PhysicsServer2D::area_get_collision_layer(RID p_area) { return 0; }
void PhysicsServer2D::area_set_collision_mask(RID p_area, uint32_t p_mask) {}
uint32_t PhysicsServer2D::area_get_collision_mask(RID p_area) { return 0; }
void PhysicsServer2D::area_set_monitorable(RID p_area, bool p_monitorable) {}
void PhysicsServer2D::area_set_pickable(RID p_area, bool p_pickable) {}
RID PhysicsServer2D::body_create() { return RID(); }
void PhysicsServer2D::body_set_space(RID p_body, RID p_space) {}
RID PhysicsServer2D::body_get_space(RID p_body) { return RID(); }
void PhysicsServer2D::body_set_mode(RID p_body, PS2DE::BodyMode p_mode) {}
PS2DE::BodyMode PhysicsServer2D::body_get_mode(RID p_body) { return PS2DE::BODY_MODE_STATIC; }
void PhysicsServer2D::body_add_shape(RID p_body, RID p_shape, const Transform2D& p_transform, bool p_disabled) {}
void PhysicsServer2D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape) {}
void PhysicsServer2D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform2D& p_transform) {}
int PhysicsServer2D::body_get_shape_count(RID p_body) { return 0; }
RID PhysicsServer2D::body_get_shape(RID p_body, int p_shape_idx) { return RID(); }
Transform2D PhysicsServer2D::body_get_shape_transform(RID p_body, int p_shape_idx) { return Transform2D(); }
void PhysicsServer2D::body_set_shape_disabled(RID p_body, int p_shape, bool p_disabled) {}
void PhysicsServer2D::body_set_shape_as_one_way_collision(RID p_body, int p_shape, bool p_enabled, real_t p_margin, const Vector2& p_direction) {}
void PhysicsServer2D::body_remove_shape(RID p_body, int p_shape_idx) {}
void PhysicsServer2D::body_clear_shapes(RID p_body) {}
void PhysicsServer2D::body_set_continuous_collision_detection_mode(RID p_body, PS2DE::CCDMode p_mode) {}
PS2DE::CCDMode PhysicsServer2D::body_get_continuous_collision_detection_mode(RID p_body) { return PS2DE::CCD_MODE_DISABLED; }
void PhysicsServer2D::body_set_collision_layer(RID p_body, uint32_t p_layer) {}
uint32_t PhysicsServer2D::body_get_collision_layer(RID p_body) { return 0; }
void PhysicsServer2D::body_set_collision_mask(RID p_body, uint32_t p_mask) {}
uint32_t PhysicsServer2D::body_get_collision_mask(RID p_body) { return 0; }
void PhysicsServer2D::body_set_collision_priority(RID p_body, real_t p_priority) {}
real_t PhysicsServer2D::body_get_collision_priority(RID p_body) { return 0; }
void PhysicsServer2D::body_reset_mass_properties(RID p_body) {}
void PhysicsServer2D::body_apply_central_impulse(RID p_body, const Vector2& p_impulse) {}
void PhysicsServer2D::body_apply_torque_impulse(RID p_body, real_t p_torque) {}
void PhysicsServer2D::body_apply_impulse(RID p_body, const Vector2& p_impulse, const Vector2& p_position) {}
void PhysicsServer2D::body_apply_central_force(RID p_body, const Vector2& p_force) {}
void PhysicsServer2D::body_apply_force(RID p_body, const Vector2& p_force, const Vector2& p_position) {}
void PhysicsServer2D::body_apply_torque(RID p_body, real_t p_torque) {}
void PhysicsServer2D::body_add_constant_central_force(RID p_body, const Vector2& p_force) {}
void PhysicsServer2D::body_add_constant_force(RID p_body, const Vector2& p_force, const Vector2& p_position) {}
void PhysicsServer2D::body_add_constant_torque(RID p_body, real_t p_torque) {}
void PhysicsServer2D::body_set_constant_force(RID p_body, const Vector2& p_force) {}
Vector2 PhysicsServer2D::body_get_constant_force(RID p_body) { return Vector2(); }
void PhysicsServer2D::body_set_constant_torque(RID p_body, real_t p_torque) {}
real_t PhysicsServer2D::body_get_constant_torque(RID p_body) { return 0; }
void PhysicsServer2D::body_set_axis_velocity(RID p_body, const Vector2& p_axis_velocity) {}
void PhysicsServer2D::body_add_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer2D::body_remove_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer2D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}
void PhysicsServer2D::body_set_max_contacts_reported(RID p_body, int p_contacts) {}
int PhysicsServer2D::body_get_max_contacts_reported(RID p_body) { return 0; }
void PhysicsServer2D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold) {}
real_t PhysicsServer2D::body_get_contacts_reported_depth_threshold(RID p_body) { return 0; }
void PhysicsServer2D::body_set_omit_force_integration(RID p_body, bool p_omit) {}
bool PhysicsServer2D::body_is_omitting_force_integration(RID p_body) { return false; }
bool PhysicsServer2D::body_collide_shape(RID p_body, int p_body_shape, RID p_shape, const Transform2D& p_shape_xform, const Vector2& p_motion, Vector2* r_results, int p_result_max, int& r_result_count) { r_result_count = 0; return false; }
void PhysicsServer2D::body_set_pickable(RID p_body, bool p_pickable) {}
PhysicsDirectBodyState2D* PhysicsServer2D::body_get_direct_state(RID p_body) { return nullptr; }
bool PhysicsServer2D::body_test_motion(RID p_body, const PS2DT::MotionParameters& p_parameters, PS2DT::MotionResult* r_result) { return false; }
RID PhysicsServer2D::joint_create() { return RID(); }
void PhysicsServer2D::joint_clear(RID p_joint) {}
void PhysicsServer2D::joint_set_param(RID p_joint, PS2DE::JointParam p_param, real_t p_value) {}
real_t PhysicsServer2D::joint_get_param(RID p_joint, PS2DE::JointParam p_param) { return 0; }
void PhysicsServer2D::joint_disable_collisions_between_bodies(RID p_joint, const bool p_disable) {}
bool PhysicsServer2D::joint_is_disabled_collisions_between_bodies(RID p_joint) { return false; }
void PhysicsServer2D::joint_make_pin(RID p_joint, const Vector2& p_anchor, RID p_body_a, RID p_body_b) {}
void PhysicsServer2D::joint_make_groove(RID p_joint, const Vector2& p_a_groove1, const Vector2& p_a_groove2, const Vector2& p_b_anchor, RID p_body_a, RID p_body_b) {}
void PhysicsServer2D::joint_make_damped_spring(RID p_joint, const Vector2& p_anchor_a, const Vector2& p_anchor_b, RID p_body_a, RID p_body_b) {}
void PhysicsServer2D::pin_joint_set_param(RID p_joint, PS2DE::PinJointParam p_param, real_t p_value) {}
real_t PhysicsServer2D::pin_joint_get_param(RID p_joint, PS2DE::PinJointParam p_param) { return 0; }
void PhysicsServer2D::pin_joint_set_flag(RID p_joint, PS2DE::PinJointFlag p_flag, bool p_enabled) {}
bool PhysicsServer2D::pin_joint_get_flag(RID p_joint, PS2DE::PinJointFlag p_flag) { return false; }
void PhysicsServer2D::damped_spring_joint_set_param(RID p_joint, PS2DE::DampedSpringParam p_param, real_t p_value) {}
real_t PhysicsServer2D::damped_spring_joint_get_param(RID p_joint, PS2DE::DampedSpringParam p_param) { return 0; }
PS2DE::JointType PhysicsServer2D::joint_get_type(RID p_joint) { return PS2DE::JOINT_TYPE_PIN; }
void PhysicsServer2D::free_rid(RID p_rid) {}
void PhysicsServer2D::set_active(bool p_active) {}
void PhysicsServer2D::init() {}
void PhysicsServer2D::step(real_t p_step) {}
void PhysicsServer2D::sync() {}
void PhysicsServer2D::flush_queries() {}
void PhysicsServer2D::end_sync() {}
void PhysicsServer2D::finish() {}
bool PhysicsServer2D::is_flushing_queries() { return false; }
int PhysicsServer2D::get_process_info(PS2DE::ProcessInfo p_info) { return 0; }

#endif // MODULE_GODOT_PHYSICS_2D_ENABLED

// ============================================================================
// Merged from servers/physics_2d/direct_states/physics_direct_body_state_2d.cpp
// ============================================================================

#include "servers/physics_2d/physics_server_2d.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_GODOT_PHYSICS_2D_ENABLED

PhysicsDirectBodyState2D::PhysicsDirectBodyState2D(GodotBody2D* p_body) : body(p_body) {}

Vector2 PhysicsDirectBodyState2D::get_total_gravity() const { return Vector2(); }
real_t PhysicsDirectBodyState2D::get_total_linear_damp() const { return 0.0; }
real_t PhysicsDirectBodyState2D::get_total_angular_damp() const { return 0.0; }

Vector2 PhysicsDirectBodyState2D::get_center_of_mass() const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_center_of_mass_local() const { return Vector2(); }
real_t PhysicsDirectBodyState2D::get_inverse_mass() const { return 0.0; }
real_t PhysicsDirectBodyState2D::get_inverse_inertia() const { return 0.0; }

void PhysicsDirectBodyState2D::set_linear_velocity(const Vector2& p_velocity) {}
Vector2 PhysicsDirectBodyState2D::get_linear_velocity() const { return Vector2(); }

void PhysicsDirectBodyState2D::set_angular_velocity(real_t p_velocity) {}
real_t PhysicsDirectBodyState2D::get_angular_velocity() const { return 0.0; }

void PhysicsDirectBodyState2D::set_transform(const Transform2D& p_transform) {}
Transform2D PhysicsDirectBodyState2D::get_transform() const { return Transform2D(); }

Vector2 PhysicsDirectBodyState2D::get_velocity_at_local_position(const Vector2& p_position) const { return Vector2(); }

void PhysicsDirectBodyState2D::apply_central_impulse(const Vector2& p_impulse) {}
void PhysicsDirectBodyState2D::apply_torque_impulse(real_t p_torque) {}
void PhysicsDirectBodyState2D::apply_impulse(const Vector2& p_impulse, const Vector2& p_position) {}

void PhysicsDirectBodyState2D::apply_central_force(const Vector2& p_force) {}
void PhysicsDirectBodyState2D::apply_force(const Vector2& p_force, const Vector2& p_position) {}
void PhysicsDirectBodyState2D::apply_torque(real_t p_torque) {}

void PhysicsDirectBodyState2D::add_constant_central_force(const Vector2& p_force) {}
void PhysicsDirectBodyState2D::add_constant_force(const Vector2& p_force, const Vector2& p_position) {}
void PhysicsDirectBodyState2D::add_constant_torque(real_t p_torque) {}

void PhysicsDirectBodyState2D::set_constant_force(const Vector2& p_force) {}
Vector2 PhysicsDirectBodyState2D::get_constant_force() const { return Vector2(); }

void PhysicsDirectBodyState2D::set_constant_torque(real_t p_torque) {}
real_t PhysicsDirectBodyState2D::get_constant_torque() const { return 0.0; }

void PhysicsDirectBodyState2D::set_sleep_state(bool p_enable) {}
bool PhysicsDirectBodyState2D::is_sleeping() const { return false; }

void PhysicsDirectBodyState2D::set_collision_layer(uint32_t p_layer) {}
uint32_t PhysicsDirectBodyState2D::get_collision_layer() const { return 0; }

void PhysicsDirectBodyState2D::set_collision_mask(uint32_t p_mask) {}
uint32_t PhysicsDirectBodyState2D::get_collision_mask() const { return 0; }

int PhysicsDirectBodyState2D::get_contact_count() const { return 0; }

Vector2 PhysicsDirectBodyState2D::get_contact_local_position(int p_contact_idx) const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_contact_local_normal(int p_contact_idx) const { return Vector2(); }
int PhysicsDirectBodyState2D::get_contact_local_shape(int p_contact_idx) const { return 0; }
Vector2 PhysicsDirectBodyState2D::get_contact_local_velocity_at_position(int p_contact_idx) const { return Vector2(); }

RID PhysicsDirectBodyState2D::get_contact_collider(int p_contact_idx) const { return RID(); }
Vector2 PhysicsDirectBodyState2D::get_contact_collider_position(int p_contact_idx) const { return Vector2(); }
int PhysicsDirectBodyState2D::get_contact_collider_shape(int p_contact_idx) const { return 0; }
Vector2 PhysicsDirectBodyState2D::get_contact_collider_velocity_at_position(int p_contact_idx) const { return Vector2(); }
Vector2 PhysicsDirectBodyState2D::get_contact_impulse(int p_contact_idx) const { return Vector2(); }

real_t PhysicsDirectBodyState2D::get_step() const { return 0.0; }

void PhysicsDirectBodyState2D::integrate_forces() {}

PhysicsDirectSpaceState2D* PhysicsDirectBodyState2D::get_space_state() { return nullptr; }

#endif // !MODULE_GODOT_PHYSICS_2D_ENABLED

// ============================================================================
// Merged from servers/physics_2d/direct_states/physics_direct_space_state_2d.cpp
// ============================================================================

#include "servers/physics_2d/physics_server_2d.h"

#include "modules/modules_enabled.gen.h"

Vector<real_t> PhysicsDirectSpaceState2D::_cast_motion(
	PhysicsShapeQueryParameters2D* rp_shape_query)
{
	real_t closest_safe = 1.0f, closest_unsafe = 1.0f;
	bool res = cast_motion(rp_shape_query->get_parameters(), closest_safe, closest_unsafe);
	if (!res) {
		return Vector<real_t>();
	}
	Vector<real_t> ret;
	ret.resize(2);
	ret.write[0] = closest_safe;
	ret.write[1] = closest_unsafe;
	return ret;
}

#ifndef MODULE_GODOT_PHYSICS_2D_ENABLED

PhysicsDirectSpaceState2D::PhysicsDirectSpaceState2D(GodotSpace2D* p_space) : space(p_space) {}

bool PhysicsDirectSpaceState2D::intersect_ray(
	const PS2DT::RayParameters& p_parameters, PS2DT::RayResult& r_result)
{
	return false;
}

int PhysicsDirectSpaceState2D::intersect_point(
	const PS2DT::PointParameters& p_parameters,
	PS2DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

int PhysicsDirectSpaceState2D::intersect_shape(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState2D::cast_motion(
	const PS2DT::ShapeParameters& p_parameters, real_t& p_closest_safe,
	real_t& p_closest_unsafe)
{
	return false;
}

bool PhysicsDirectSpaceState2D::collide_shape(
	const PS2DT::ShapeParameters& p_parameters, Vector2* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState2D::rest_info(
	const PS2DT::ShapeParameters& p_parameters,
	PS2DT::ShapeRestInfo* r_info)
{
	return false;
}

#endif // !MODULE_GODOT_PHYSICS_2D_ENABLED

// ============================================================================
// Merged from servers/physics_2d/queries/physics_ray_query_parameters_2d.cpp
// ============================================================================

#include "servers/physics_2d/physics_server_2d.h"

Ref<PhysicsRayQueryParameters2D> PhysicsRayQueryParameters2D::create(
	Vector2 p_from, Vector2 p_to, uint32_t p_mask, const TypedArray<RID>& p_exclude)
{
	Ref<PhysicsRayQueryParameters2D> params;
	params.instantiate();
	params->set_from(p_from);
	params->set_to(p_to);
	params->set_collision_mask(p_mask);
	params->set_exclude(p_exclude);
	return params;
}



void PhysicsRayQueryParameters2D::set_exclude(TypedArray<RID> const&) {}

// ============================================================================
// Merged from servers/physics_2d/queries/physics_shape_query_parameters_2d.cpp
// ============================================================================

#include "core/io/resource.h"
#include "servers/physics_2d/physics_server_2d.h"

Ref<Resource> PhysicsShapeQueryParameters2D::get_shape() const { return shape_ref; }

void PhysicsShapeQueryParameters2D::set_shape(const Ref<Resource>& p_shape_ref)
{
	ERR_FAIL_COND(p_shape_ref.is_null());
	shape_ref = p_shape_ref;
	parameters.shape_rid = p_shape_ref->get_rid();
}

void PhysicsShapeQueryParameters2D::set_shape_rid(const RID& p_shape)
{
	if (parameters.shape_rid != p_shape) {
		shape_ref = Ref<Resource>();
		parameters.shape_rid = p_shape;
	}
}



// ============================================================================
// Merged from servers/physics_2d/queries/physics_testmotion_query_result_2d.cpp
// ============================================================================

#include "servers/physics_2d/physics_server_2d.h"

Vector2 PhysicsTestMotionResult2D::get_travel() const { return result.travel; }

Vector2 PhysicsTestMotionResult2D::get_remainder() const { return result.remainder; }

Vector2 PhysicsTestMotionResult2D::get_collision_point() const { return result.collision_point; }

Vector2 PhysicsTestMotionResult2D::get_collision_normal() const { return result.collision_normal; }

Vector2 PhysicsTestMotionResult2D::get_collider_velocity() const
{
	return result.collider_velocity;
}

RID PhysicsTestMotionResult2D::get_collider_rid() const { return result.collider; }

int PhysicsTestMotionResult2D::get_collider_shape() const { return result.collider_shape; }

int PhysicsTestMotionResult2D::get_collision_local_shape() const
{
	return result.collision_local_shape;
}

real_t PhysicsTestMotionResult2D::get_collision_depth() const { return result.collision_depth; }

real_t PhysicsTestMotionResult2D::get_collision_safe_fraction() const
{
	return result.collision_safe_fraction;
}

real_t PhysicsTestMotionResult2D::get_collision_unsafe_fraction() const
{
	return result.collision_unsafe_fraction;
}


// --- Merged from servers/physics_3d/physics_server_3d.cpp ---
#include "servers/physics_3d/physics_server_3d.h"

#include "modules/modules_enabled.gen.h"

#ifdef MODULE_JOLT_PHYSICS_ENABLED
#include "modules/jolt_physics/jolt_physics_server_3d.h"
#include "modules/jolt_physics/jolt_physics_server_3d.h"
#endif

void PhysicsServer3D::initialize(bool p_on_separate_thread)
{
#ifdef MODULE_JOLT_PHYSICS_ENABLED
	if (is_initialized()) {
		return;
	}
	jolt_initialize();
	if (!data) {
		data = memnew(Data);
	}
	data->server = memnew(JoltPhysicsServer3D(p_on_separate_thread));
	data->server->init();
#endif
}

void PhysicsServer3D::finalize()
{
	if (!data) {
		return;
	}
#ifdef MODULE_JOLT_PHYSICS_ENABLED
	if (data->server) {
		data->server->finish();
		memdelete(data->server);
		data->server = nullptr;
	}
	jolt_deinitialize();
#endif
	memdelete(data);
	data = nullptr;
}

#ifdef MODULE_JOLT_PHYSICS_ENABLED

void PhysicsServer3D::soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_update_rendering_server(p_body, rp_rendering_server_handler);
}

RID PhysicsServer3D::shape_create(PS3DE::ShapeType p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->shape_create(p_shape);
}

RID PhysicsServer3D::world_boundary_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->world_boundary_shape_create();
}

RID PhysicsServer3D::separation_ray_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->separation_ray_shape_create();
}

RID PhysicsServer3D::sphere_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->sphere_shape_create();
}

RID PhysicsServer3D::box_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->box_shape_create();
}

RID PhysicsServer3D::capsule_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->capsule_shape_create();
}

RID PhysicsServer3D::cylinder_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->cylinder_shape_create();
}

RID PhysicsServer3D::convex_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->convex_polygon_shape_create();
}

RID PhysicsServer3D::concave_polygon_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->concave_polygon_shape_create();
}

RID PhysicsServer3D::heightmap_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->heightmap_shape_create();
}

RID PhysicsServer3D::custom_shape_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->custom_shape_create();
}

void PhysicsServer3D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->shape_set_custom_solver_bias(p_shape, p_bias);
}

PS3DE::ShapeType PhysicsServer3D::shape_get_type(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::ShapeType());
	return data->server->shape_get_type(p_shape);
}

void PhysicsServer3D::shape_set_margin(RID p_shape, real_t p_margin)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->shape_set_margin(p_shape, p_margin);
}

real_t PhysicsServer3D::shape_get_margin(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->shape_get_margin(p_shape);
}

real_t PhysicsServer3D::shape_get_custom_solver_bias(RID p_shape)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->shape_get_custom_solver_bias(p_shape);
}

RID PhysicsServer3D::space_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->space_create();
}

void PhysicsServer3D::space_set_active(RID p_space, bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_active(p_space, p_active);
}

bool PhysicsServer3D::space_is_active(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->space_is_active(p_space);
}

void PhysicsServer3D::space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_param(p_space, p_param, p_value);
}

real_t PhysicsServer3D::space_get_param(RID p_space, PS3DE::SpaceParameter p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_param(p_space, p_param);
}

PhysicsDirectSpaceState3D* PhysicsServer3D::space_get_direct_state(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->space_get_direct_state(p_space);
}

void PhysicsServer3D::space_set_debug_contacts(RID p_space, int p_max_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->space_set_debug_contacts(p_space, p_max_contacts);
}

Vector<Vector3> PhysicsServer3D::space_get_contacts(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector<Vector3>());
	return data->server->space_get_contacts(p_space);
}

int PhysicsServer3D::space_get_contact_count(RID p_space)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->space_get_contact_count(p_space);
}

RID PhysicsServer3D::area_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_create();
}

void PhysicsServer3D::area_set_space(RID p_area, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_space(p_area, p_space);
}

RID PhysicsServer3D::area_get_space(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_space(p_area);
}

void PhysicsServer3D::area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_add_shape(p_area, p_shape, p_transform, p_disabled);
}

void PhysicsServer3D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape(p_area, p_shape_idx, p_shape);
}

void PhysicsServer3D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_transform(p_area, p_shape_idx, p_transform);
}

int PhysicsServer3D::area_get_shape_count(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_shape_count(p_area);
}

RID PhysicsServer3D::area_get_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->area_get_shape(p_area, p_shape_idx);
}

Transform3D PhysicsServer3D::area_get_shape_transform(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->server->area_get_shape_transform(p_area, p_shape_idx);
}

void PhysicsServer3D::area_remove_shape(RID p_area, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_remove_shape(p_area, p_shape_idx);
}

void PhysicsServer3D::area_clear_shapes(RID p_area)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_clear_shapes(p_area);
}

void PhysicsServer3D::area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_shape_disabled(p_area, p_shape_idx, p_disabled);
}

void PhysicsServer3D::area_set_transform(RID p_area, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_transform(p_area, p_transform);
}

Transform3D PhysicsServer3D::area_get_transform(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->server->area_get_transform(p_area);
}

void PhysicsServer3D::area_set_collision_layer(RID p_area, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_layer(p_area, p_layer);
}

uint32_t PhysicsServer3D::area_get_collision_layer(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_layer(p_area);
}

void PhysicsServer3D::area_set_collision_mask(RID p_area, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_collision_mask(p_area, p_mask);
}

uint32_t PhysicsServer3D::area_get_collision_mask(RID p_area)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->area_get_collision_mask(p_area);
}

void PhysicsServer3D::area_set_monitorable(RID p_area, bool p_monitorable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_monitorable(p_area, p_monitorable);
}

void PhysicsServer3D::area_set_ray_pickable(RID p_area, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->area_set_ray_pickable(p_area, p_enable);
}

RID PhysicsServer3D::body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_create();
}

void PhysicsServer3D::body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_space(p_body, p_space);
}

RID PhysicsServer3D::body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_space(p_body);
}

void PhysicsServer3D::body_set_mode(RID p_body, PS3DE::BodyMode p_mode)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_mode(p_body, p_mode);
}

PS3DE::BodyMode PhysicsServer3D::body_get_mode(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::BodyMode());
	return data->server->body_get_mode(p_body);
}

void PhysicsServer3D::body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_shape(p_body, p_shape, p_transform, p_disabled);
}

void PhysicsServer3D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape(p_body, p_shape_idx, p_shape);
}

void PhysicsServer3D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_transform(p_body, p_shape_idx, p_transform);
}

int PhysicsServer3D::body_get_shape_count(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_shape_count(p_body);
}

RID PhysicsServer3D::body_get_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->body_get_shape(p_body, p_shape_idx);
}

Transform3D PhysicsServer3D::body_get_shape_transform(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND_V(!is_initialized(), Transform3D());
	return data->server->body_get_shape_transform(p_body, p_shape_idx);
}

void PhysicsServer3D::body_remove_shape(RID p_body, int p_shape_idx)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_shape(p_body, p_shape_idx);
}

void PhysicsServer3D::body_clear_shapes(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_clear_shapes(p_body);
}

void PhysicsServer3D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_shape_disabled(p_body, p_shape_idx, p_disabled);
}

void PhysicsServer3D::body_set_enable_continuous_collision_detection(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_enable_continuous_collision_detection(p_body, p_enable);
}

bool PhysicsServer3D::body_is_continuous_collision_detection_enabled(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_continuous_collision_detection_enabled(p_body);
}

void PhysicsServer3D::body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer3D::body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_layer(p_body);
}

void PhysicsServer3D::body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer3D::body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_mask(p_body);
}

void PhysicsServer3D::body_set_collision_priority(RID p_body, real_t p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_collision_priority(p_body, p_priority);
}

real_t PhysicsServer3D::body_get_collision_priority(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_collision_priority(p_body);
}

void PhysicsServer3D::body_set_user_flags(RID p_body, uint32_t p_flags)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_user_flags(p_body, p_flags);
}

uint32_t PhysicsServer3D::body_get_user_flags(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_user_flags(p_body);
}

void PhysicsServer3D::body_reset_mass_properties(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_reset_mass_properties(p_body);
}

void PhysicsServer3D::body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer3D::body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_impulse(p_body, p_impulse, p_position);
}

void PhysicsServer3D::body_apply_torque_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque_impulse(p_body, p_impulse);
}

void PhysicsServer3D::body_apply_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_central_force(p_body, p_force);
}

void PhysicsServer3D::body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_force(p_body, p_force, p_position);
}

void PhysicsServer3D::body_apply_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_apply_torque(p_body, p_torque);
}

void PhysicsServer3D::body_add_constant_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_central_force(p_body, p_force);
}

void PhysicsServer3D::body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_force(p_body, p_force, p_position);
}

void PhysicsServer3D::body_add_constant_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_constant_torque(p_body, p_torque);
}

void PhysicsServer3D::body_set_constant_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_force(p_body, p_force);
}

Vector3 PhysicsServer3D::body_get_constant_force(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->body_get_constant_force(p_body);
}

void PhysicsServer3D::body_set_constant_torque(RID p_body, const Vector3& p_torque)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_constant_torque(p_body, p_torque);
}

Vector3 PhysicsServer3D::body_get_constant_torque(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->body_get_constant_torque(p_body);
}

void PhysicsServer3D::body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_axis_velocity(p_body, p_axis_velocity);
}

void PhysicsServer3D::body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_axis_lock(p_body, p_axis, p_lock);
}

bool PhysicsServer3D::body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_axis_locked(p_body, p_axis);
}

void PhysicsServer3D::body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer3D::body_set_max_contacts_reported(RID p_body, int p_contacts)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_max_contacts_reported(p_body, p_contacts);
}

int PhysicsServer3D::body_get_max_contacts_reported(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_max_contacts_reported(p_body);
}

void PhysicsServer3D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_contacts_reported_depth_threshold(p_body, p_threshold);
}

real_t PhysicsServer3D::body_get_contacts_reported_depth_threshold(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->body_get_contacts_reported_depth_threshold(p_body);
}

void PhysicsServer3D::body_set_omit_force_integration(RID p_body, bool p_omit)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_omit_force_integration(p_body, p_omit);
}

bool PhysicsServer3D::body_is_omitting_force_integration(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_is_omitting_force_integration(p_body);
}

void PhysicsServer3D::body_set_ray_pickable(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->body_set_ray_pickable(p_body, p_enable);
}

PhysicsDirectBodyState3D* PhysicsServer3D::body_get_direct_state(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), nullptr);
	return data->server->body_get_direct_state(p_body);
}

bool PhysicsServer3D::body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->body_test_motion(p_body, p_parameters, r_result);
}

RID PhysicsServer3D::soft_body_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->soft_body_create();
}

void PhysicsServer3D::soft_body_set_space(RID p_body, RID p_space)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_space(p_body, p_space);
}

RID PhysicsServer3D::soft_body_get_space(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->soft_body_get_space(p_body);
}

void PhysicsServer3D::soft_body_set_mesh(RID p_body, RID p_mesh)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_mesh(p_body, p_mesh);
}

AABB PhysicsServer3D::soft_body_get_bounds(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), AABB());
	return data->server->soft_body_get_bounds(p_body);
}

void PhysicsServer3D::soft_body_set_collision_layer(RID p_body, uint32_t p_layer)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_collision_layer(p_body, p_layer);
}

uint32_t PhysicsServer3D::soft_body_get_collision_layer(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_collision_layer(p_body);
}

void PhysicsServer3D::soft_body_set_collision_mask(RID p_body, uint32_t p_mask)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_collision_mask(p_body, p_mask);
}

uint32_t PhysicsServer3D::soft_body_get_collision_mask(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_collision_mask(p_body);
}

void PhysicsServer3D::soft_body_add_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_add_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::soft_body_remove_collision_exception(RID p_body, RID p_body_b)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_remove_collision_exception(p_body, p_body_b);
}

void PhysicsServer3D::soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_get_collision_exceptions(p_body, p_exceptions);
}

void PhysicsServer3D::soft_body_set_transform(RID p_body, const Transform3D& p_transform)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_transform(p_body, p_transform);
}

void PhysicsServer3D::soft_body_set_ray_pickable(RID p_body, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_ray_pickable(p_body, p_enable);
}

void PhysicsServer3D::soft_body_set_simulation_precision(RID p_body, int p_simulation_precision)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_simulation_precision(p_body, p_simulation_precision);
}

int PhysicsServer3D::soft_body_get_simulation_precision(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_simulation_precision(p_body);
}

void PhysicsServer3D::soft_body_set_total_mass(RID p_body, real_t p_total_mass)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_total_mass(p_body, p_total_mass);
}

real_t PhysicsServer3D::soft_body_get_total_mass(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_total_mass(p_body);
}

void PhysicsServer3D::soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_linear_stiffness(p_body, p_stiffness);
}

real_t PhysicsServer3D::soft_body_get_linear_stiffness(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_linear_stiffness(p_body);
}

void PhysicsServer3D::soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_shrinking_factor(p_body, p_shrinking_factor);
}

real_t PhysicsServer3D::soft_body_get_shrinking_factor(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_shrinking_factor(p_body);
}

void PhysicsServer3D::soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_pressure_coefficient(p_body, p_pressure_coefficient);
}

real_t PhysicsServer3D::soft_body_get_pressure_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_pressure_coefficient(p_body);
}

void PhysicsServer3D::soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_damping_coefficient(p_body, p_damping_coefficient);
}

real_t PhysicsServer3D::soft_body_get_damping_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_damping_coefficient(p_body);
}

void PhysicsServer3D::soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_set_drag_coefficient(p_body, p_drag_coefficient);
}

real_t PhysicsServer3D::soft_body_get_drag_coefficient(RID p_body)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->soft_body_get_drag_coefficient(p_body);
}

void PhysicsServer3D::soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_move_point(p_body, p_point_index, p_global_position);
}

Vector3 PhysicsServer3D::soft_body_get_point_global_position(RID p_body, int p_point_index)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->soft_body_get_point_global_position(p_body, p_point_index);
}

void PhysicsServer3D::soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_point_impulse(p_body, p_point_index, p_impulse);
}

void PhysicsServer3D::soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_point_force(p_body, p_point_index, p_force);
}

void PhysicsServer3D::soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_central_impulse(p_body, p_impulse);
}

void PhysicsServer3D::soft_body_apply_central_force(RID p_body, const Vector3& p_force)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_apply_central_force(p_body, p_force);
}

void PhysicsServer3D::soft_body_remove_all_pinned_points(RID p_body)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_remove_all_pinned_points(p_body);
}

void PhysicsServer3D::soft_body_pin_point(RID p_body, int p_point_index, bool p_pin)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->soft_body_pin_point(p_body, p_point_index, p_pin);
}

bool PhysicsServer3D::soft_body_is_point_pinned(RID p_body, int p_point_index)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->soft_body_is_point_pinned(p_body, p_point_index);
}

RID PhysicsServer3D::joint_create()
{
	ERR_FAIL_COND_V(!is_initialized(), RID());
	return data->server->joint_create();
}

void PhysicsServer3D::joint_clear(RID p_joint)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_clear(p_joint);
}

PS3DE::JointType PhysicsServer3D::joint_get_type(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), PS3DE::JointType());
	return data->server->joint_get_type(p_joint);
}

void PhysicsServer3D::joint_set_solver_priority(RID p_joint, int p_priority)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_set_solver_priority(p_joint, p_priority);
}

int PhysicsServer3D::joint_get_solver_priority(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->joint_get_solver_priority(p_joint);
}

void PhysicsServer3D::joint_disable_collisions_between_bodies(RID p_joint, bool p_disable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_disable_collisions_between_bodies(p_joint, p_disable);
}

bool PhysicsServer3D::joint_is_disabled_collisions_between_bodies(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->joint_is_disabled_collisions_between_bodies(p_joint);
}

void PhysicsServer3D::joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_pin(p_joint, p_body_A, p_local_A, p_body_B, p_local_B);
}

void PhysicsServer3D::pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->pin_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::pin_joint_set_local_a(RID p_joint, const Vector3& p_A)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_local_a(p_joint, p_A);
}

Vector3 PhysicsServer3D::pin_joint_get_local_a(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->pin_joint_get_local_a(p_joint);
}

void PhysicsServer3D::pin_joint_set_local_b(RID p_joint, const Vector3& p_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->pin_joint_set_local_b(p_joint, p_B);
}

Vector3 PhysicsServer3D::pin_joint_get_local_b(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Vector3());
	return data->server->pin_joint_get_local_b(p_joint);
}

void PhysicsServer3D::joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_hinge(p_joint, p_body_A, p_hinge_A, p_body_B, p_hinge_B);
}

void PhysicsServer3D::joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_hinge_simple(p_joint, p_body_A, p_pivot_A, p_axis_A, p_body_B, p_pivot_B, p_axis_B);
}

void PhysicsServer3D::hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->hinge_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->hinge_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->hinge_joint_set_flag(p_joint, p_flag, p_enabled);
}

bool PhysicsServer3D::hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->hinge_joint_get_flag(p_joint, p_flag);
}

void PhysicsServer3D::joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_slider(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->slider_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->slider_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_cone_twist(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->cone_twist_joint_set_param(p_joint, p_param, p_value);
}

real_t PhysicsServer3D::cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->cone_twist_joint_get_param(p_joint, p_param);
}

void PhysicsServer3D::joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->joint_make_generic_6dof(p_joint, p_body_A, p_local_frame_A, p_body_B, p_local_frame_B);
}

void PhysicsServer3D::generic_6dof_joint_set_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param, real_t p_value)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->generic_6dof_joint_set_param(p_joint, p_arg1, p_param, p_value);
}

real_t PhysicsServer3D::generic_6dof_joint_get_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->generic_6dof_joint_get_param(p_joint, p_arg1, p_param);
}

void PhysicsServer3D::generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->generic_6dof_joint_set_flag(p_joint, p_arg1, p_flag, p_enable);
}

bool PhysicsServer3D::generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag)
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->generic_6dof_joint_get_flag(p_joint, p_arg1, p_flag);
}

void PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->generic_6dof_joint_set_angular_target_rotation(p_joint, p_target_rotation);
}

Quaternion PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(RID p_joint)
{
	ERR_FAIL_COND_V(!is_initialized(), Quaternion());
	return data->server->generic_6dof_joint_get_angular_target_rotation(p_joint);
}

void PhysicsServer3D::free_rid(RID p_rid)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->free_rid(p_rid);
}

void PhysicsServer3D::set_active(bool p_active)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->set_active(p_active);
}

void PhysicsServer3D::init()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->init();
}

void PhysicsServer3D::step(real_t p_step)
{
	ERR_FAIL_COND(!is_initialized());
	data->server->step(p_step);
}

void PhysicsServer3D::sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->sync();
}

void PhysicsServer3D::flush_queries()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->flush_queries();
}

void PhysicsServer3D::end_sync()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->end_sync();
}

void PhysicsServer3D::finish()
{
	ERR_FAIL_COND(!is_initialized());
	data->server->finish();
}

bool PhysicsServer3D::is_flushing_queries()
{
	ERR_FAIL_COND_V(!is_initialized(), false);
	return data->server->is_flushing_queries();
}

int PhysicsServer3D::get_process_info(PS3DE::ProcessInfo p_info)
{
	ERR_FAIL_COND_V(!is_initialized(), 0);
	return data->server->get_process_info(p_info);
}

#else // !MODULE_JOLT_PHYSICS_ENABLED

void PhysicsServer3D::soft_body_update_rendering_server(RID p_body, PhysicsServer3DRenderingServerHandler* rp_rendering_server_handler) {}
RID PhysicsServer3D::shape_create(PS3DE::ShapeType p_shape) { return RID(); }
RID PhysicsServer3D::world_boundary_shape_create() { return RID(); }
RID PhysicsServer3D::separation_ray_shape_create() { return RID(); }
RID PhysicsServer3D::sphere_shape_create() { return RID(); }
RID PhysicsServer3D::box_shape_create() { return RID(); }
RID PhysicsServer3D::capsule_shape_create() { return RID(); }
RID PhysicsServer3D::cylinder_shape_create() { return RID(); }
RID PhysicsServer3D::convex_polygon_shape_create() { return RID(); }
RID PhysicsServer3D::concave_polygon_shape_create() { return RID(); }
RID PhysicsServer3D::heightmap_shape_create() { return RID(); }
RID PhysicsServer3D::custom_shape_create() { return RID(); }
void PhysicsServer3D::shape_set_custom_solver_bias(RID p_shape, real_t p_bias) {}
PS3DE::ShapeType PhysicsServer3D::shape_get_type(RID p_shape) { return PS3DE::SHAPE_CUSTOM; }
void PhysicsServer3D::shape_set_margin(RID p_shape, real_t p_margin) {}
real_t PhysicsServer3D::shape_get_margin(RID p_shape) { return 0; }
real_t PhysicsServer3D::shape_get_custom_solver_bias(RID p_shape) { return 0; }
RID PhysicsServer3D::space_create() { return RID(); }
void PhysicsServer3D::space_set_active(RID p_space, bool p_active) {}
bool PhysicsServer3D::space_is_active(RID p_space) { return false; }
void PhysicsServer3D::space_set_param(RID p_space, PS3DE::SpaceParameter p_param, real_t p_value) {}
real_t PhysicsServer3D::space_get_param(RID p_space, PS3DE::SpaceParameter p_param) { return 0; }
PhysicsDirectSpaceState3D* PhysicsServer3D::space_get_direct_state(RID p_space) { return nullptr; }
void PhysicsServer3D::space_set_debug_contacts(RID p_space, int p_max_contacts) {}
Vector<Vector3> PhysicsServer3D::space_get_contacts(RID p_space) { return Vector<Vector3>(); }
int PhysicsServer3D::space_get_contact_count(RID p_space) { return 0; }
RID PhysicsServer3D::area_create() { return RID(); }
void PhysicsServer3D::area_set_space(RID p_area, RID p_space) {}
RID PhysicsServer3D::area_get_space(RID p_area) { return RID(); }
void PhysicsServer3D::area_add_shape(RID p_area, RID p_shape, const Transform3D& p_transform, bool p_disabled) {}
void PhysicsServer3D::area_set_shape(RID p_area, int p_shape_idx, RID p_shape) {}
void PhysicsServer3D::area_set_shape_transform(RID p_area, int p_shape_idx, const Transform3D& p_transform) {}
int PhysicsServer3D::area_get_shape_count(RID p_area) { return 0; }
RID PhysicsServer3D::area_get_shape(RID p_area, int p_shape_idx) { return RID(); }
Transform3D PhysicsServer3D::area_get_shape_transform(RID p_area, int p_shape_idx) { return Transform3D(); }
void PhysicsServer3D::area_remove_shape(RID p_area, int p_shape_idx) {}
void PhysicsServer3D::area_clear_shapes(RID p_area) {}
void PhysicsServer3D::area_set_shape_disabled(RID p_area, int p_shape_idx, bool p_disabled) {}
void PhysicsServer3D::area_set_transform(RID p_area, const Transform3D& p_transform) {}
Transform3D PhysicsServer3D::area_get_transform(RID p_area) { return Transform3D(); }
void PhysicsServer3D::area_set_collision_layer(RID p_area, uint32_t p_layer) {}
uint32_t PhysicsServer3D::area_get_collision_layer(RID p_area) { return 0; }
void PhysicsServer3D::area_set_collision_mask(RID p_area, uint32_t p_mask) {}
uint32_t PhysicsServer3D::area_get_collision_mask(RID p_area) { return 0; }
void PhysicsServer3D::area_set_monitorable(RID p_area, bool p_monitorable) {}
void PhysicsServer3D::area_set_ray_pickable(RID p_area, bool p_enable) {}
RID PhysicsServer3D::body_create() { return RID(); }
void PhysicsServer3D::body_set_space(RID p_body, RID p_space) {}
RID PhysicsServer3D::body_get_space(RID p_body) { return RID(); }
void PhysicsServer3D::body_set_mode(RID p_body, PS3DE::BodyMode p_mode) {}
PS3DE::BodyMode PhysicsServer3D::body_get_mode(RID p_body) { return PS3DE::BODY_MODE_STATIC; }
void PhysicsServer3D::body_add_shape(RID p_body, RID p_shape, const Transform3D& p_transform, bool p_disabled) {}
void PhysicsServer3D::body_set_shape(RID p_body, int p_shape_idx, RID p_shape) {}
void PhysicsServer3D::body_set_shape_transform(RID p_body, int p_shape_idx, const Transform3D& p_transform) {}
int PhysicsServer3D::body_get_shape_count(RID p_body) { return 0; }
RID PhysicsServer3D::body_get_shape(RID p_body, int p_shape_idx) { return RID(); }
Transform3D PhysicsServer3D::body_get_shape_transform(RID p_body, int p_shape_idx) { return Transform3D(); }
void PhysicsServer3D::body_remove_shape(RID p_body, int p_shape_idx) {}
void PhysicsServer3D::body_clear_shapes(RID p_body) {}
void PhysicsServer3D::body_set_shape_disabled(RID p_body, int p_shape_idx, bool p_disabled) {}
void PhysicsServer3D::body_set_enable_continuous_collision_detection(RID p_body, bool p_enable) {}
bool PhysicsServer3D::body_is_continuous_collision_detection_enabled(RID p_body) { return false; }
void PhysicsServer3D::body_set_collision_layer(RID p_body, uint32_t p_layer) {}
uint32_t PhysicsServer3D::body_get_collision_layer(RID p_body) { return 0; }
void PhysicsServer3D::body_set_collision_mask(RID p_body, uint32_t p_mask) {}
uint32_t PhysicsServer3D::body_get_collision_mask(RID p_body) { return 0; }
void PhysicsServer3D::body_set_collision_priority(RID p_body, real_t p_priority) {}
real_t PhysicsServer3D::body_get_collision_priority(RID p_body) { return 0; }
void PhysicsServer3D::body_set_user_flags(RID p_body, uint32_t p_flags) {}
uint32_t PhysicsServer3D::body_get_user_flags(RID p_body) { return 0; }
void PhysicsServer3D::body_reset_mass_properties(RID p_body) {}
void PhysicsServer3D::body_apply_central_impulse(RID p_body, const Vector3& p_impulse) {}
void PhysicsServer3D::body_apply_impulse(RID p_body, const Vector3& p_impulse, const Vector3& p_position) {}
void PhysicsServer3D::body_apply_torque_impulse(RID p_body, const Vector3& p_impulse) {}
void PhysicsServer3D::body_apply_central_force(RID p_body, const Vector3& p_force) {}
void PhysicsServer3D::body_apply_force(RID p_body, const Vector3& p_force, const Vector3& p_position) {}
void PhysicsServer3D::body_apply_torque(RID p_body, const Vector3& p_torque) {}
void PhysicsServer3D::body_add_constant_central_force(RID p_body, const Vector3& p_force) {}
void PhysicsServer3D::body_add_constant_force(RID p_body, const Vector3& p_force, const Vector3& p_position) {}
void PhysicsServer3D::body_add_constant_torque(RID p_body, const Vector3& p_torque) {}
void PhysicsServer3D::body_set_constant_force(RID p_body, const Vector3& p_force) {}
Vector3 PhysicsServer3D::body_get_constant_force(RID p_body) { return Vector3(); }
void PhysicsServer3D::body_set_constant_torque(RID p_body, const Vector3& p_torque) {}
Vector3 PhysicsServer3D::body_get_constant_torque(RID p_body) { return Vector3(); }
void PhysicsServer3D::body_set_axis_velocity(RID p_body, const Vector3& p_axis_velocity) {}
void PhysicsServer3D::body_set_axis_lock(RID p_body, PS3DE::BodyAxis p_axis, bool p_lock) {}
bool PhysicsServer3D::body_is_axis_locked(RID p_body, PS3DE::BodyAxis p_axis) { return false; }
void PhysicsServer3D::body_add_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::body_remove_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}
void PhysicsServer3D::body_set_max_contacts_reported(RID p_body, int p_contacts) {}
int PhysicsServer3D::body_get_max_contacts_reported(RID p_body) { return 0; }
void PhysicsServer3D::body_set_contacts_reported_depth_threshold(RID p_body, real_t p_threshold) {}
real_t PhysicsServer3D::body_get_contacts_reported_depth_threshold(RID p_body) { return 0; }
void PhysicsServer3D::body_set_omit_force_integration(RID p_body, bool p_omit) {}
bool PhysicsServer3D::body_is_omitting_force_integration(RID p_body) { return false; }
void PhysicsServer3D::body_set_ray_pickable(RID p_body, bool p_enable) {}
PhysicsDirectBodyState3D* PhysicsServer3D::body_get_direct_state(RID p_body) { return nullptr; }
bool PhysicsServer3D::body_test_motion(RID p_body, const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result) { return false; }
RID PhysicsServer3D::soft_body_create() { return RID(); }
void PhysicsServer3D::soft_body_set_space(RID p_body, RID p_space) {}
RID PhysicsServer3D::soft_body_get_space(RID p_body) { return RID(); }
void PhysicsServer3D::soft_body_set_mesh(RID p_body, RID p_mesh) {}
AABB PhysicsServer3D::soft_body_get_bounds(RID p_body) { return AABB(); }
void PhysicsServer3D::soft_body_set_collision_layer(RID p_body, uint32_t p_layer) {}
uint32_t PhysicsServer3D::soft_body_get_collision_layer(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_collision_mask(RID p_body, uint32_t p_mask) {}
uint32_t PhysicsServer3D::soft_body_get_collision_mask(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_add_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::soft_body_remove_collision_exception(RID p_body, RID p_body_b) {}
void PhysicsServer3D::soft_body_get_collision_exceptions(RID p_body, List<RID>* p_exceptions) {}
void PhysicsServer3D::soft_body_set_transform(RID p_body, const Transform3D& p_transform) {}
void PhysicsServer3D::soft_body_set_ray_pickable(RID p_body, bool p_enable) {}
void PhysicsServer3D::soft_body_set_simulation_precision(RID p_body, int p_simulation_precision) {}
int PhysicsServer3D::soft_body_get_simulation_precision(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_total_mass(RID p_body, real_t p_total_mass) {}
real_t PhysicsServer3D::soft_body_get_total_mass(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_linear_stiffness(RID p_body, real_t p_stiffness) {}
real_t PhysicsServer3D::soft_body_get_linear_stiffness(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_shrinking_factor(RID p_body, real_t p_shrinking_factor) {}
real_t PhysicsServer3D::soft_body_get_shrinking_factor(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_pressure_coefficient(RID p_body, real_t p_pressure_coefficient) {}
real_t PhysicsServer3D::soft_body_get_pressure_coefficient(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_damping_coefficient(RID p_body, real_t p_damping_coefficient) {}
real_t PhysicsServer3D::soft_body_get_damping_coefficient(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_set_drag_coefficient(RID p_body, real_t p_drag_coefficient) {}
real_t PhysicsServer3D::soft_body_get_drag_coefficient(RID p_body) { return 0; }
void PhysicsServer3D::soft_body_move_point(RID p_body, int p_point_index, const Vector3& p_global_position) {}
Vector3 PhysicsServer3D::soft_body_get_point_global_position(RID p_body, int p_point_index) { return Vector3(); }
void PhysicsServer3D::soft_body_apply_point_impulse(RID p_body, int p_point_index, const Vector3& p_impulse) {}
void PhysicsServer3D::soft_body_apply_point_force(RID p_body, int p_point_index, const Vector3& p_force) {}
void PhysicsServer3D::soft_body_apply_central_impulse(RID p_body, const Vector3& p_impulse) {}
void PhysicsServer3D::soft_body_apply_central_force(RID p_body, const Vector3& p_force) {}
void PhysicsServer3D::soft_body_remove_all_pinned_points(RID p_body) {}
void PhysicsServer3D::soft_body_pin_point(RID p_body, int p_point_index, bool p_pin) {}
bool PhysicsServer3D::soft_body_is_point_pinned(RID p_body, int p_point_index) { return false; }
RID PhysicsServer3D::joint_create() { return RID(); }
void PhysicsServer3D::joint_clear(RID p_joint) {}
PS3DE::JointType PhysicsServer3D::joint_get_type(RID p_joint) { return PS3DE::JOINT_TYPE_PIN; }
void PhysicsServer3D::joint_set_solver_priority(RID p_joint, int p_priority) {}
int PhysicsServer3D::joint_get_solver_priority(RID p_joint) { return 0; }
void PhysicsServer3D::joint_disable_collisions_between_bodies(RID p_joint, bool p_disable) {}
bool PhysicsServer3D::joint_is_disabled_collisions_between_bodies(RID p_joint) { return false; }
void PhysicsServer3D::joint_make_pin(RID p_joint, RID p_body_A, const Vector3& p_local_A, RID p_body_B, const Vector3& p_local_B) {}
void PhysicsServer3D::pin_joint_set_param(RID p_joint, PS3DE::PinJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::pin_joint_get_param(RID p_joint, PS3DE::PinJointParam p_param) { return 0; }
void PhysicsServer3D::pin_joint_set_local_a(RID p_joint, const Vector3& p_A) {}
Vector3 PhysicsServer3D::pin_joint_get_local_a(RID p_joint) { return Vector3(); }
void PhysicsServer3D::pin_joint_set_local_b(RID p_joint, const Vector3& p_B) {}
Vector3 PhysicsServer3D::pin_joint_get_local_b(RID p_joint) { return Vector3(); }
void PhysicsServer3D::joint_make_hinge(RID p_joint, RID p_body_A, const Transform3D& p_hinge_A, RID p_body_B, const Transform3D& p_hinge_B) {}
void PhysicsServer3D::joint_make_hinge_simple(RID p_joint, RID p_body_A, const Vector3& p_pivot_A, const Vector3& p_axis_A, RID p_body_B, const Vector3& p_pivot_B, const Vector3& p_axis_B) {}
void PhysicsServer3D::hinge_joint_set_param(RID p_joint, PS3DE::HingeJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::hinge_joint_get_param(RID p_joint, PS3DE::HingeJointParam p_param) { return 0; }
void PhysicsServer3D::hinge_joint_set_flag(RID p_joint, PS3DE::HingeJointFlag p_flag, bool p_enabled) {}
bool PhysicsServer3D::hinge_joint_get_flag(RID p_joint, PS3DE::HingeJointFlag p_flag) { return false; }
void PhysicsServer3D::joint_make_slider(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B) {}
void PhysicsServer3D::slider_joint_set_param(RID p_joint, PS3DE::SliderJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::slider_joint_get_param(RID p_joint, PS3DE::SliderJointParam p_param) { return 0; }
void PhysicsServer3D::joint_make_cone_twist(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B) {}
void PhysicsServer3D::cone_twist_joint_set_param(RID p_joint, PS3DE::ConeTwistJointParam p_param, real_t p_value) {}
real_t PhysicsServer3D::cone_twist_joint_get_param(RID p_joint, PS3DE::ConeTwistJointParam p_param) { return 0; }
void PhysicsServer3D::joint_make_generic_6dof(RID p_joint, RID p_body_A, const Transform3D& p_local_frame_A, RID p_body_B, const Transform3D& p_local_frame_B) {}
void PhysicsServer3D::generic_6dof_joint_set_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param, real_t p_value) {}
real_t PhysicsServer3D::generic_6dof_joint_get_param(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisParam p_param) { return 0; }
void PhysicsServer3D::generic_6dof_joint_set_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag, bool p_enable) {}
bool PhysicsServer3D::generic_6dof_joint_get_flag(RID p_joint, Vector3::Axis p_arg1, PS3DE::G6DOFJointAxisFlag p_flag) { return false; }
void PhysicsServer3D::generic_6dof_joint_set_angular_target_rotation(RID p_joint, const Quaternion& p_target_rotation) {}
Quaternion PhysicsServer3D::generic_6dof_joint_get_angular_target_rotation(RID p_joint) { return Quaternion(); }
void PhysicsServer3D::free_rid(RID p_rid) {}
void PhysicsServer3D::set_active(bool p_active) {}
void PhysicsServer3D::init() {}
void PhysicsServer3D::step(real_t p_step) {}
void PhysicsServer3D::sync() {}
void PhysicsServer3D::flush_queries() {}
void PhysicsServer3D::end_sync() {}
void PhysicsServer3D::finish() {}
bool PhysicsServer3D::is_flushing_queries() { return false; }
int PhysicsServer3D::get_process_info(PS3DE::ProcessInfo p_info) { return 0; }

#endif // MODULE_JOLT_PHYSICS_ENABLED

// ============================================================================
// Merged from servers/physics_3d/physics_server_3d_rendering_server_handler.cpp
// ============================================================================

#include "servers/physics_3d/physics_server_3d.h"

#include "servers/rendering/renderer.h"

void PhysicsServer3DRenderingServerHandler::prepare(RID p_mesh_rid, int p_surface)
{
	clear();

	ERR_FAIL_COND(!p_mesh_rid.is_valid());

	mesh = p_mesh_rid;
	surface = p_surface;

	RenderingServerTypes::SurfaceData surface_data = Renderer::mesh_get_surface(mesh, surface);

	uint32_t surface_offsets[RSE::ARRAY_MAX];
	uint32_t vertex_stride;
	uint32_t normal_tangent_stride;
	uint32_t attrib_stride;
	uint32_t skin_stride;
	Renderer::mesh_surface_make_offsets_from_format(surface_data.format,
		surface_data.vertex_count, surface_data.index_count, surface_offsets, vertex_stride,
		normal_tangent_stride, attrib_stride, skin_stride);

	buffer = surface_data.vertex_data;
	stride = vertex_stride;
	normal_stride = normal_tangent_stride;
	offset_vertices = surface_offsets[RSE::ARRAY_VERTEX];
	offset_normal = surface_offsets[RSE::ARRAY_NORMAL];
}

void PhysicsServer3DRenderingServerHandler::clear()
{
	buffer.resize(0);
	stride = 0;
	normal_stride = 0;
	offset_vertices = 0;
	offset_normal = 0;

	surface = 0;
	mesh = RID();
}

void PhysicsServer3DRenderingServerHandler::open() { write_buffer = buffer.ptrw(); }

void PhysicsServer3DRenderingServerHandler::close() { write_buffer = nullptr; }

void PhysicsServer3DRenderingServerHandler::commit_changes()
{
	Renderer::mesh_surface_update_vertex_region(mesh, surface, 0, buffer);
}

void PhysicsServer3DRenderingServerHandler::set_vertex(int p_vertex_id, const Vector3& p_vertex)
{
	ERR_FAIL_NULL(write_buffer);
	float* vertex_buffer =
		reinterpret_cast<float*>(write_buffer + p_vertex_id * stride + offset_vertices);
	*vertex_buffer++ = (float)p_vertex.x;
	*vertex_buffer++ = (float)p_vertex.y;
	*vertex_buffer++ = (float)p_vertex.z;
}

void PhysicsServer3DRenderingServerHandler::set_normal(int p_vertex_id, const Vector3& p_normal)
{
	ERR_FAIL_NULL(write_buffer);
	Vector2 res = p_normal.octahedron_encode();
	uint32_t value = 0;
	value |= (uint16_t)CLAMP(res.x * 65535, 0, 65535);
	value |= (uint16_t)CLAMP(res.y * 65535, 0, 65535) << 16;
	memcpy(&write_buffer[p_vertex_id * normal_stride + offset_normal], &value, sizeof(uint32_t));
}

void PhysicsServer3DRenderingServerHandler::set_aabb(const AABB& p_aabb)
{
	Renderer::mesh_set_custom_aabb(mesh, p_aabb);
}

// ============================================================================
// Merged from servers/physics_3d/direct_states/physics_direct_body_state_3d.cpp
// ============================================================================

#include "servers/physics_3d/physics_server_3d.h"

#include "modules/modules_enabled.gen.h"

#ifndef MODULE_JOLT_PHYSICS_ENABLED

PhysicsDirectBodyState3D::PhysicsDirectBodyState3D(JoltBody3D* p_body) : body(p_body) {}

Vector3 PhysicsDirectBodyState3D::get_total_gravity() const { return Vector3(); }
real_t PhysicsDirectBodyState3D::get_total_angular_damp() const { return 0.0; }
real_t PhysicsDirectBodyState3D::get_total_linear_damp() const { return 0.0; }

Vector3 PhysicsDirectBodyState3D::get_center_of_mass() const { return Vector3(); }
Vector3 PhysicsDirectBodyState3D::get_center_of_mass_local() const { return Vector3(); }
Basis PhysicsDirectBodyState3D::get_principal_inertia_axes() const { return Basis(); }
real_t PhysicsDirectBodyState3D::get_inverse_mass() const { return 0.0; }
Vector3 PhysicsDirectBodyState3D::get_inverse_inertia() const { return Vector3(); }
Basis PhysicsDirectBodyState3D::get_inverse_inertia_tensor() const { return Basis(); }

void PhysicsDirectBodyState3D::set_linear_velocity(const Vector3& p_velocity) {}
Vector3 PhysicsDirectBodyState3D::get_linear_velocity() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_angular_velocity(const Vector3& p_velocity) {}
Vector3 PhysicsDirectBodyState3D::get_angular_velocity() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_transform(const Transform3D& p_transform) {}
Transform3D PhysicsDirectBodyState3D::get_transform() const { return Transform3D(); }

Vector3 PhysicsDirectBodyState3D::get_velocity_at_local_position(const Vector3& p_position) const { return Vector3(); }

void PhysicsDirectBodyState3D::apply_central_impulse(const Vector3& p_impulse) {}
void PhysicsDirectBodyState3D::apply_impulse(const Vector3& p_impulse, const Vector3& p_position) {}
void PhysicsDirectBodyState3D::apply_torque_impulse(const Vector3& p_impulse) {}

void PhysicsDirectBodyState3D::apply_central_force(const Vector3& p_force) {}
void PhysicsDirectBodyState3D::apply_force(const Vector3& p_force, const Vector3& p_position) {}
void PhysicsDirectBodyState3D::apply_torque(const Vector3& p_torque) {}

void PhysicsDirectBodyState3D::add_constant_central_force(const Vector3& p_force) {}
void PhysicsDirectBodyState3D::add_constant_force(const Vector3& p_force, const Vector3& p_position) {}
void PhysicsDirectBodyState3D::add_constant_torque(const Vector3& p_torque) {}

void PhysicsDirectBodyState3D::set_constant_force(const Vector3& p_force) {}
Vector3 PhysicsDirectBodyState3D::get_constant_force() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_constant_torque(const Vector3& p_torque) {}
Vector3 PhysicsDirectBodyState3D::get_constant_torque() const { return Vector3(); }

void PhysicsDirectBodyState3D::set_sleep_state(bool p_sleep) {}
bool PhysicsDirectBodyState3D::is_sleeping() const { return false; }

void PhysicsDirectBodyState3D::set_collision_layer(uint32_t p_layer) {}
uint32_t PhysicsDirectBodyState3D::get_collision_layer() const { return 0; }

void PhysicsDirectBodyState3D::set_collision_mask(uint32_t p_mask) {}
uint32_t PhysicsDirectBodyState3D::get_collision_mask() const { return 0; }

int PhysicsDirectBodyState3D::get_contact_count() const { return 0; }

Vector3 PhysicsDirectBodyState3D::get_contact_local_position(int p_contact_idx) const { return Vector3(); }
Vector3 PhysicsDirectBodyState3D::get_contact_local_normal(int p_contact_idx) const { return Vector3(); }
Vector3 PhysicsDirectBodyState3D::get_contact_impulse(int p_contact_idx) const { return Vector3(); }
int PhysicsDirectBodyState3D::get_contact_local_shape(int p_contact_idx) const { return 0; }
Vector3 PhysicsDirectBodyState3D::get_contact_local_velocity_at_position(int p_contact_idx) const { return Vector3(); }

RID PhysicsDirectBodyState3D::get_contact_collider(int p_contact_idx) const { return RID(); }
Vector3 PhysicsDirectBodyState3D::get_contact_collider_position(int p_contact_idx) const { return Vector3(); }
int PhysicsDirectBodyState3D::get_contact_collider_shape(int p_contact_idx) const { return 0; }
Vector3 PhysicsDirectBodyState3D::get_contact_collider_velocity_at_position(int p_contact_idx) const { return Vector3(); }

real_t PhysicsDirectBodyState3D::get_step() const { return 0.0; }

void PhysicsDirectBodyState3D::integrate_forces() {}

PhysicsDirectSpaceState3D* PhysicsDirectBodyState3D::get_space_state() { return nullptr; }

#endif // !MODULE_JOLT_PHYSICS_ENABLED

// ============================================================================
// Merged from servers/physics_3d/direct_states/physics_direct_space_state_3d.cpp
// ============================================================================

#include "servers/physics_3d/physics_server_3d.h"

#include "modules/modules_enabled.gen.h"

Vector<real_t> PhysicsDirectSpaceState3D::_cast_motion(
	PhysicsShapeQueryParameters3D* rp_shape_query)
{
	real_t closest_safe = 1.0f, closest_unsafe = 1.0f;
	bool res = cast_motion(rp_shape_query->get_parameters(), closest_safe, closest_unsafe);
	if (!res) {
		return Vector<real_t>();
	}
	Vector<real_t> ret;
	ret.resize(2);
	ret.write[0] = closest_safe;
	ret.write[1] = closest_unsafe;
	return ret;
}

#ifndef MODULE_JOLT_PHYSICS_ENABLED

PhysicsDirectSpaceState3D::PhysicsDirectSpaceState3D(JoltSpace3D* p_space) : space(p_space) {}

bool PhysicsDirectSpaceState3D::intersect_ray(
	const PS3DT::RayParameters& p_parameters, PS3DT::RayResult& r_result)
{
	return false;
}

int PhysicsDirectSpaceState3D::intersect_point(
	const PS3DT::PointParameters& p_point_params,
	PS3DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

int PhysicsDirectSpaceState3D::intersect_shape(
	const PS3DT::ShapeParameters& p_shape_params,
	PS3DT::ShapeResult* r_results, int p_result_max)
{
	return 0;
}

bool PhysicsDirectSpaceState3D::cast_motion(
	const PS3DT::ShapeParameters& p_shape_params, real_t& r_closest_safe,
	real_t& r_closest_unsafe, PS3DT::ShapeRestInfo* r_info)
{
	return false;
}

bool PhysicsDirectSpaceState3D::collide_shape(
	const PS3DT::ShapeParameters& p_shape_params, Vector3* r_results,
	int p_result_max, int& r_result_count)
{
	r_result_count = 0;
	return false;
}

bool PhysicsDirectSpaceState3D::rest_info(
	const PS3DT::ShapeParameters& p_shape_params,
	PS3DT::ShapeRestInfo* r_info)
{
	return false;
}

Vector3 PhysicsDirectSpaceState3D::get_closest_point_to_object_volume(
	RID p_object, Vector3 p_point) const
{
	return Vector3();
}

bool PhysicsDirectSpaceState3D::body_test_motion(const JoltBody3D& p_body,
	const PS3DT::MotionParameters& p_parameters, PS3DT::MotionResult* r_result) const
{
	return false;
}

#endif // !MODULE_JOLT_PHYSICS_ENABLED

// ============================================================================
// Merged from servers/physics_3d/queries/physics_shape_query_parameters_3d.cpp
// ============================================================================

#include "core/io/resource.h"
#include "servers/physics_3d/physics_server_3d.h"

Ref<Resource> PhysicsShapeQueryParameters3D::get_shape() const { return shape_ref; }

void PhysicsShapeQueryParameters3D::set_shape(const Ref<Resource>& p_shape_ref)
{
	ERR_FAIL_COND(p_shape_ref.is_null());
	shape_ref = p_shape_ref;
	parameters.shape_rid = p_shape_ref->get_rid();
}

void PhysicsShapeQueryParameters3D::set_shape_rid(const RID& p_shape)
{
	if (parameters.shape_rid != p_shape) {
		shape_ref = Ref<Resource>();
		parameters.shape_rid = p_shape;
	}
}



// ============================================================================
// Merged from servers/physics_3d/queries/physics_testmotion_query_result_3d.cpp
// ============================================================================

#include "servers/physics_3d/physics_server_3d.h"

Vector3 PhysicsTestMotionResult3D::get_travel() const { return result.travel; }

Vector3 PhysicsTestMotionResult3D::get_remainder() const { return result.remainder; }

real_t PhysicsTestMotionResult3D::get_collision_safe_fraction() const
{
	return result.collision_safe_fraction;
}

real_t PhysicsTestMotionResult3D::get_collision_unsafe_fraction() const
{
	return result.collision_unsafe_fraction;
}

int PhysicsTestMotionResult3D::get_collision_count() const { return result.collision_count; }

Vector3 PhysicsTestMotionResult3D::get_collision_point(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].position;
}

Vector3 PhysicsTestMotionResult3D::get_collision_normal(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].normal;
}

Vector3 PhysicsTestMotionResult3D::get_collider_velocity(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].collider_velocity;
}

RID PhysicsTestMotionResult3D::get_collider_rid(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, RID());
	return result.collisions[p_collision_index].collider;
}

int PhysicsTestMotionResult3D::get_collider_shape(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].collider_shape;
}

int PhysicsTestMotionResult3D::get_collision_local_shape(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].local_shape;
}

real_t PhysicsTestMotionResult3D::get_collision_depth(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0.0);
	return result.collisions[p_collision_index].depth;
}


// --- Merged from servers/navigation_2d/navigation_server_2d.cpp ---
#if !defined(NAVIGATION_2D_DISABLED)
#include "servers/navigation_2d/navigation_server_2d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.

////////////////////////////////////////////////////////////////////////////
// NavigationPathQueryParameters2D

void NavigationPathQueryParameters2D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters2D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	pathfinding_algorithm = p_pathfinding_algorithm;
}

NavigationPathQueryParameters2D::PathfindingAlgorithm
NavigationPathQueryParameters2D::get_pathfinding_algorithm() const
{
	return pathfinding_algorithm;
}

void NavigationPathQueryParameters2D::set_path_postprocessing(
	const NavigationPathQueryParameters2D::PathPostProcessing p_path_postprocessing)
{
	path_postprocessing = p_path_postprocessing;
}

NavigationPathQueryParameters2D::PathPostProcessing
NavigationPathQueryParameters2D::get_path_postprocessing() const
{
	return path_postprocessing;
}

void NavigationPathQueryParameters2D::set_map(RID p_map) { map = p_map; }

RID NavigationPathQueryParameters2D::get_map() const { return map; }

void NavigationPathQueryParameters2D::set_start_position(Vector2 p_start_position)
{
	start_position = p_start_position;
}

Vector2 NavigationPathQueryParameters2D::get_start_position() const { return start_position; }

void NavigationPathQueryParameters2D::set_target_position(Vector2 p_target_position)
{
	target_position = p_target_position;
}

Vector2 NavigationPathQueryParameters2D::get_target_position() const { return target_position; }

void NavigationPathQueryParameters2D::set_navigation_layers(uint32_t p_navigation_layers)
{
	navigation_layers = p_navigation_layers;
}

uint32_t NavigationPathQueryParameters2D::get_navigation_layers() const
{
	return navigation_layers;
}

void NavigationPathQueryParameters2D::set_metadata_flags(uint32_t p_flags)
{
	metadata_flags = (int64_t)p_flags;
}

uint32_t NavigationPathQueryParameters2D::get_metadata_flags() const
{
	return (int64_t)metadata_flags;
}

void NavigationPathQueryParameters2D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
}

bool NavigationPathQueryParameters2D::get_simplify_path() const { return simplify_path; }

void NavigationPathQueryParameters2D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
}

real_t NavigationPathQueryParameters2D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationPathQueryParameters2D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
}

float NavigationPathQueryParameters2D::get_path_return_max_length() const
{
	return path_return_max_length;
}

void NavigationPathQueryParameters2D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
}

float NavigationPathQueryParameters2D::get_path_return_max_radius() const
{
	return path_return_max_radius;
}

void NavigationPathQueryParameters2D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
}

int NavigationPathQueryParameters2D::get_path_search_max_polygons() const
{
	return path_search_max_polygons;
}

void NavigationPathQueryParameters2D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
}

float NavigationPathQueryParameters2D::get_path_search_max_distance() const
{
	return path_search_max_distance;
}

////////////////////////////////////////////////////////////////////////////
// NavigationPathQueryResult2D

void NavigationPathQueryResult2D::set_path(const Vector<Vector2>& p_path) { path = p_path; }

const Vector<Vector2>& NavigationPathQueryResult2D::get_path() const { return path; }

void NavigationPathQueryResult2D::set_path_types(const Vector<int32_t>& p_path_types)
{
	path_types = p_path_types;
}

const Vector<int32_t>& NavigationPathQueryResult2D::get_path_types() const { return path_types; }

void NavigationPathQueryResult2D::set_path_owner_ids(const Vector<int64_t>& p_path_owner_ids)
{
	path_owner_ids = p_path_owner_ids;
}

const Vector<int64_t>& NavigationPathQueryResult2D::get_path_owner_ids() const
{
	return path_owner_ids;
}

void NavigationPathQueryResult2D::set_path_length(float p_length) { path_length = p_length; }

float NavigationPathQueryResult2D::get_path_length() const { return path_length; }

void NavigationPathQueryResult2D::reset() {}

////////////////////////////////////////////////////////////////////////////
// NavigationServer2D

bool NavigationServer2D::get_debug_enabled() { return data->debug_enabled; }

#ifdef DEBUG_ENABLED

bool NavigationServer2D::get_debug_navigation_enabled() { return data->debug_navigation_enabled; }

bool NavigationServer2D::get_debug_avoidance_enabled() { return data->debug_avoidance_enabled; }

void NavigationServer2D::set_debug_navigation_link_connection_color(const Color& p_color)
{
	data->debug_navigation_link_connection_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_link_connection_color()
{
	return data->debug_navigation_link_connection_color;
}

void NavigationServer2D::set_debug_navigation_link_connection_disabled_color(const Color& p_color)
{
	data->debug_navigation_link_connection_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_link_connection_disabled_color()
{
	return data->debug_navigation_link_connection_disabled_color;
}

void NavigationServer2D::set_debug_navigation_enable_edge_lines(const bool p_value)
{
	data->debug_navigation_enable_edge_lines = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_edge_lines()
{
	return data->debug_navigation_enable_edge_lines;
}

void NavigationServer2D::set_debug_navigation_agent_path_color(const Color& p_color)
{
	data->debug_navigation_agent_path_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_agent_path_color()
{
	return data->debug_navigation_agent_path_color;
}

void NavigationServer2D::set_debug_navigation_enable_agent_paths(const bool p_value)
{
	data->debug_navigation_enable_agent_paths = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_agent_paths()
{
	return data->debug_navigation_enable_agent_paths;
}

void NavigationServer2D::set_debug_navigation_agent_path_point_size(real_t p_point_size)
{
	data->debug_navigation_agent_path_point_size = p_point_size;
}

real_t NavigationServer2D::get_debug_navigation_agent_path_point_size()
{
	return data->debug_navigation_agent_path_point_size;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_agents_radius(const bool p_value)
{
	data->debug_navigation_avoidance_enable_agents_radius = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_agents_radius()
{
	return data->debug_navigation_avoidance_enable_agents_radius;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_obstacles_radius(const bool p_value)
{
	data->debug_navigation_avoidance_enable_obstacles_radius = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_radius()
{
	return data->debug_navigation_avoidance_enable_obstacles_radius;
}

void NavigationServer2D::set_debug_navigation_avoidance_agents_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_agents_radius_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_agents_radius_color()
{
	return data->debug_navigation_avoidance_agents_radius_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_obstacles_radius_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_obstacles_radius_color()
{
	return data->debug_navigation_avoidance_obstacles_radius_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushin_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_face_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushout_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_face_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_edge_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_edge_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_obstacles_static(const bool p_value)
{
	data->debug_navigation_avoidance_enable_obstacles_static = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_static()
{
	return data->debug_navigation_avoidance_enable_obstacles_static;
}

void NavigationServer2D::set_debug_navigation_geometry_face_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_face_color()
{
	if (!data) {
		return Color(0.0, 1.0, 1.0, 0.4);
	}
	return data->debug_navigation_geometry_face_color;
}

void NavigationServer2D::set_debug_navigation_geometry_edge_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_edge_color()
{
	if (!data) {
		return Color(0.0, 1.0, 1.0, 0.8);
	}
	return data->debug_navigation_geometry_edge_color;
}

void NavigationServer2D::set_debug_navigation_geometry_face_disabled_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_face_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_face_disabled_color()
{
	if (!data) {
		return Color(1.0, 0.0, 0.0, 0.4);
	}
	return data->debug_navigation_geometry_face_disabled_color;
}

void NavigationServer2D::set_debug_navigation_geometry_edge_disabled_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_geometry_edge_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_edge_disabled_color()
{
	if (!data) {
		return Color(1.0, 0.0, 0.0, 0.8);
	}
	return data->debug_navigation_geometry_edge_disabled_color;
}

void NavigationServer2D::set_debug_navigation_edge_connection_color(const Color& p_color)
{
	if (!data) {
		return;
	}
	data->debug_navigation_edge_connection_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_edge_connection_color()
{
	if (!data) {
		return Color(1.0, 0.0, 1.0, 0.4);
	}
	return data->debug_navigation_edge_connection_color;
}

void NavigationServer2D::set_debug_navigation_enable_edge_connections(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_edge_connections = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_edge_connections()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_edge_connections;
}

void NavigationServer2D::set_debug_navigation_enable_geometry_face_random_color(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_geometry_face_random_color = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_geometry_face_random_color()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_geometry_face_random_color;
}

void NavigationServer2D::set_debug_navigation_enable_agent_radius(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_agent_radius = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_agent_radius()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_agent_radius;
}

void NavigationServer2D::set_debug_navigation_enable_obstacles(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_obstacles = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_obstacles()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_obstacles;
}

void NavigationServer2D::set_debug_navigation_enable_link_connections(bool p_enable)
{
	if (!data) {
		return;
	}
	data->debug_navigation_enable_link_connections = p_enable;
}

bool NavigationServer2D::get_debug_navigation_enable_link_connections()
{
	if (!data) {
		return false;
	}
	return data->debug_navigation_enable_link_connections;
}
#endif // DEBUG_ENABLED

struct NavigationServer2D::NavMap
{
};
struct NavigationServer2D::NavRegion
{
};
struct NavigationServer2D::NavLink
{
};
struct NavigationServer2D::NavAgent
{
};
struct NavigationServer2D::NavObstacle
{
};

void NavigationServer2D::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void NavigationServer2D::finalize()
{
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

void NavigationServer2D::process(double p_delta_time) {}

void NavigationServer2D::free_rid(RID p_rid) {}

int NavigationServer2D::get_process_info(ProcessInfo p_info) { return 0; }

RID NavigationServer2D::map_create() { return RID(); }

void NavigationServer2D::map_set_active(RID p_map, bool p_active) {}

void NavigationServer2D::map_set_cell_size(RID p_map, real_t p_cell_size) {}

real_t NavigationServer2D::map_get_link_connection_radius(RID p_map) { return 0.0; }

bool NavigationServer2D::map_get_use_edge_connections(RID p_map) { return false; }

real_t NavigationServer2D::map_get_edge_connection_margin(RID p_map) { return 0.0; }

void NavigationServer2D::region_set_map(RID p_region, RID p_map) {}

void NavigationServer2D::region_set_transform(RID p_region, Transform2D p_transform) {}

void NavigationServer2D::region_set_use_edge_connections(RID p_region, bool p_enabled) {}

void NavigationServer2D::region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers) {}

void NavigationServer2D::region_set_enter_cost(RID p_region, real_t p_enter_cost) {}

void NavigationServer2D::region_set_travel_cost(RID p_region, real_t p_travel_cost) {}

int NavigationServer2D::region_get_connections_count(RID p_region) { return 0; }

Vector2 NavigationServer2D::region_get_connection_pathway_start(RID p_region, int p_connection_id)
{
	return Vector2();
}

Vector2 NavigationServer2D::region_get_connection_pathway_end(RID p_region, int p_connection_id)
{
	return Vector2();
}

RID NavigationServer2D::link_create() { return RID(); }

void NavigationServer2D::link_set_map(RID p_link, RID p_map) {}

void NavigationServer2D::link_set_enabled(RID p_link, bool p_enabled) {}

void NavigationServer2D::link_set_bidirectional(RID p_link, bool p_bidirectional) {}

void NavigationServer2D::link_set_navigation_layers(RID p_link, uint32_t p_navigation_layers) {}

void NavigationServer2D::link_set_enter_cost(RID p_link, real_t p_enter_cost) {}

void NavigationServer2D::link_set_travel_cost(RID p_link, real_t p_travel_cost) {}

RID NavigationServer2D::obstacle_create() { return RID(); }

void NavigationServer2D::obstacle_set_map(RID p_obstacle, RID p_map) {}

void NavigationServer2D::obstacle_set_radius(RID p_obstacle, real_t p_radius) {}

void NavigationServer2D::obstacle_set_velocity(RID p_obstacle, Vector2 p_velocity) {}

void NavigationServer2D::obstacle_set_vertices(RID p_obstacle, const Vector<Vector2>& p_vertices) {}

void NavigationServer2D::obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers) {}

void NavigationServer2D::obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled) {}

void NavigationServer2D::obstacle_set_paused(RID p_obstacle, bool p_paused) {}

void NavigationServer2D::agent_set_map(RID p_agent, RID p_map) {}

void NavigationServer2D::agent_set_position(RID p_agent, Vector2 p_position) {}

void NavigationServer2D::agent_set_velocity(RID p_agent, Vector2 p_velocity) {}

void NavigationServer2D::agent_set_velocity_forced(RID p_agent, Vector2 p_velocity) {}

void NavigationServer2D::agent_set_radius(RID p_agent, real_t p_radius) {}

void NavigationServer2D::agent_set_neighbor_distance(RID p_agent, real_t p_distance) {}

void NavigationServer2D::agent_set_max_neighbors(RID p_agent, int p_count) {}

void NavigationServer2D::agent_set_time_horizon_agents(RID p_agent, real_t p_time_horizon) {}

void NavigationServer2D::agent_set_time_horizon_obstacles(RID p_agent, real_t p_time_horizon) {}

void NavigationServer2D::agent_set_max_speed(RID p_agent, real_t p_max_speed) {}

void NavigationServer2D::agent_set_avoidance_layers(RID p_agent, uint32_t p_layers) {}

void NavigationServer2D::agent_set_avoidance_mask(RID p_agent, uint32_t p_mask) {}

void NavigationServer2D::agent_set_avoidance_priority(RID p_agent, real_t p_priority) {}

void NavigationServer2D::agent_set_paused(RID p_agent, bool p_paused) {}
#endif // !defined(NAVIGATION_2D_DISABLED)


// --- Merged from servers/navigation_3d/navigation_server_3d.cpp ---
#if !defined(NAVIGATION_3D_DISABLED)
#include "servers/navigation_3d/navigation_server_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.

////////////////////////////////////////////////////////////////////////////
// NavigationPathQueryParameters3D

void NavigationPathQueryParameters3D::set_pathfinding_algorithm(
	const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm)
{
	pathfinding_algorithm = p_pathfinding_algorithm;
}

NavigationPathQueryParameters3D::PathfindingAlgorithm
NavigationPathQueryParameters3D::get_pathfinding_algorithm() const
{
	return pathfinding_algorithm;
}

void NavigationPathQueryParameters3D::set_path_postprocessing(
	const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing)
{
	path_postprocessing = p_path_postprocessing;
}

NavigationPathQueryParameters3D::PathPostProcessing
NavigationPathQueryParameters3D::get_path_postprocessing() const
{
	return path_postprocessing;
}

void NavigationPathQueryParameters3D::set_map(RID p_map) { map = p_map; }

RID NavigationPathQueryParameters3D::get_map() const { return map; }

void NavigationPathQueryParameters3D::set_start_position(Vector3 p_start_position)
{
	start_position = p_start_position;
}

Vector3 NavigationPathQueryParameters3D::get_start_position() const { return start_position; }

void NavigationPathQueryParameters3D::set_target_position(Vector3 p_target_position)
{
	target_position = p_target_position;
}

Vector3 NavigationPathQueryParameters3D::get_target_position() const { return target_position; }

void NavigationPathQueryParameters3D::set_navigation_layers(uint32_t p_navigation_layers)
{
	navigation_layers = p_navigation_layers;
}

uint32_t NavigationPathQueryParameters3D::get_navigation_layers() const
{
	return navigation_layers;
}

void NavigationPathQueryParameters3D::set_metadata_flags(uint32_t p_flags)
{
	metadata_flags = (int64_t)p_flags;
}

uint32_t NavigationPathQueryParameters3D::get_metadata_flags() const
{
	return (int64_t)metadata_flags;
}

void NavigationPathQueryParameters3D::set_simplify_path(bool p_enabled)
{
	simplify_path = p_enabled;
}

bool NavigationPathQueryParameters3D::get_simplify_path() const { return simplify_path; }

void NavigationPathQueryParameters3D::set_simplify_epsilon(real_t p_epsilon)
{
	simplify_epsilon = MAX(0.0, p_epsilon);
}

real_t NavigationPathQueryParameters3D::get_simplify_epsilon() const { return simplify_epsilon; }

void NavigationPathQueryParameters3D::set_path_return_max_length(float p_length)
{
	path_return_max_length = MAX(0.0, p_length);
}

float NavigationPathQueryParameters3D::get_path_return_max_length() const
{
	return path_return_max_length;
}

void NavigationPathQueryParameters3D::set_path_return_max_radius(float p_radius)
{
	path_return_max_radius = MAX(0.0, p_radius);
}

float NavigationPathQueryParameters3D::get_path_return_max_radius() const
{
	return path_return_max_radius;
}

void NavigationPathQueryParameters3D::set_path_search_max_polygons(int p_max_polygons)
{
	path_search_max_polygons = p_max_polygons;
}

int NavigationPathQueryParameters3D::get_path_search_max_polygons() const
{
	return path_search_max_polygons;
}

void NavigationPathQueryParameters3D::set_path_search_max_distance(float p_distance)
{
	path_search_max_distance = MAX(0.0, p_distance);
}

float NavigationPathQueryParameters3D::get_path_search_max_distance() const
{
	return path_search_max_distance;
}

////////////////////////////////////////////////////////////////////////////
// NavigationPathQueryResult3D

void NavigationPathQueryResult3D::set_path(const Vector<Vector3>& p_path) { path = p_path; }

const Vector<Vector3>& NavigationPathQueryResult3D::get_path() const { return path; }

void NavigationPathQueryResult3D::set_path_types(const Vector<int32_t>& p_path_types)
{
	path_types = p_path_types;
}

const Vector<int32_t>& NavigationPathQueryResult3D::get_path_types() const { return path_types; }

void NavigationPathQueryResult3D::set_path_owner_ids(const Vector<int64_t>& p_path_owner_ids)
{
	path_owner_ids = p_path_owner_ids;
}

const Vector<int64_t>& NavigationPathQueryResult3D::get_path_owner_ids() const
{
	return path_owner_ids;
}

void NavigationPathQueryResult3D::set_path_length(float p_length) { path_length = p_length; }

float NavigationPathQueryResult3D::get_path_length() const { return path_length; }

void NavigationPathQueryResult3D::reset() {}

////////////////////////////////////////////////////////////////////////////
// NavigationServer3D

bool NavigationServer3D::get_debug_enabled() { return data->debug_enabled; }

#ifdef DEBUG_ENABLED
Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_face_material()
{
	if (data->debug_navigation_geometry_face_material.is_valid()) {
		return data->debug_navigation_geometry_face_material;
	}

	Ref<StandardMaterial3D> face_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	face_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	face_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	face_material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	data->debug_navigation_geometry_face_material = face_material;

	return data->debug_navigation_geometry_face_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_edge_material()
{
	if (data->debug_navigation_geometry_edge_material.is_valid()) {
		return data->debug_navigation_geometry_edge_material;
	}

	Ref<StandardMaterial3D> line_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	line_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	data->debug_navigation_geometry_edge_material = line_material;

	return data->debug_navigation_geometry_edge_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_face_disabled_material()
{
	if (data->debug_navigation_geometry_face_disabled_material.is_valid()) {
		return data->debug_navigation_geometry_face_disabled_material;
	}

	Ref<StandardMaterial3D> face_disabled_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	face_disabled_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	face_disabled_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

	data->debug_navigation_geometry_face_disabled_material = face_disabled_material;

	return data->debug_navigation_geometry_face_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_edge_disabled_material()
{
	if (data->debug_navigation_geometry_edge_disabled_material.is_valid()) {
		return data->debug_navigation_geometry_edge_disabled_material;
	}

	Ref<StandardMaterial3D> line_disabled_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	line_disabled_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	data->debug_navigation_geometry_edge_disabled_material = line_disabled_material;

	return data->debug_navigation_geometry_edge_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_edge_connections_material()
{
	if (data->debug_navigation_edge_connections_material.is_valid()) {
		return data->debug_navigation_edge_connections_material;
	}

	Ref<StandardMaterial3D> edge_connections_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	edge_connections_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	edge_connections_material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_edge_connections_material = edge_connections_material;

	return data->debug_navigation_edge_connections_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_link_connections_material()
{
	if (data->debug_navigation_link_connections_material.is_valid()) {
		return data->debug_navigation_link_connections_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_link_connections_material = material;
	return data->debug_navigation_link_connections_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_link_connections_disabled_material()
{
	if (data->debug_navigation_link_connections_disabled_material.is_valid()) {
		return data->debug_navigation_link_connections_disabled_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_link_connections_disabled_material = material;
	return data->debug_navigation_link_connections_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_agent_path_line_material()
{
	if (data->debug_navigation_agent_path_line_material.is_valid()) {
		return data->debug_navigation_agent_path_line_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_agent_path_line_material = material;
	return data->debug_navigation_agent_path_line_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_agent_path_point_material()
{
	if (data->debug_navigation_agent_path_point_material.is_valid()) {
		return data->debug_navigation_agent_path_point_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_point_size(data->debug_navigation_agent_path_point_size);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_agent_path_point_material = material;
	return data->debug_navigation_agent_path_point_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_avoidance_agents_radius_material()
{
	if (data->debug_navigation_avoidance_agents_radius_material.is_valid()) {
		return data->debug_navigation_avoidance_agents_radius_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_agents_radius_material = material;
	return data->debug_navigation_avoidance_agents_radius_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_obstacles_radius_material()
{
	if (data->debug_navigation_avoidance_obstacles_radius_material.is_valid()) {
		return data->debug_navigation_avoidance_obstacles_radius_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_obstacles_radius_material = material;
	return data->debug_navigation_avoidance_obstacles_radius_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_face_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushin_face_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushin_face_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_static_obstacle_pushin_face_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushin_face_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_face_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushout_face_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushout_face_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_static_obstacle_pushout_face_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushout_face_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushin_edge_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushin_edge_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	data->debug_navigation_avoidance_static_obstacle_pushin_edge_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushin_edge_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushout_edge_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushout_edge_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	data->debug_navigation_avoidance_static_obstacle_pushout_edge_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushout_edge_material;
}

void NavigationServer3D::set_debug_navigation_edge_connection_color(const Color& p_color)
{
	data->debug_navigation_edge_connection_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_edge_connection_color()
{
	return data->debug_navigation_edge_connection_color;
}

void NavigationServer3D::set_debug_navigation_geometry_edge_color(const Color& p_color)
{
	data->debug_navigation_geometry_edge_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_edge_color()
{
	return data->debug_navigation_geometry_edge_color;
}

void NavigationServer3D::set_debug_navigation_geometry_face_color(const Color& p_color)
{
	data->debug_navigation_geometry_face_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_face_color()
{
	return data->debug_navigation_geometry_face_color;
}

void NavigationServer3D::set_debug_navigation_geometry_edge_disabled_color(const Color& p_color)
{
	data->debug_navigation_geometry_edge_disabled_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_edge_disabled_color()
{
	return data->debug_navigation_geometry_edge_disabled_color;
}

void NavigationServer3D::set_debug_navigation_geometry_face_disabled_color(const Color& p_color)
{
	data->debug_navigation_geometry_face_disabled_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_face_disabled_color()
{
	return data->debug_navigation_geometry_face_disabled_color;
}

void NavigationServer3D::set_debug_navigation_link_connection_color(const Color& p_color)
{
	data->debug_navigation_link_connection_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_link_connection_color()
{
	return data->debug_navigation_link_connection_color;
}

void NavigationServer3D::set_debug_navigation_link_connection_disabled_color(const Color& p_color)
{
	data->debug_navigation_link_connection_disabled_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_link_connection_disabled_color()
{
	return data->debug_navigation_link_connection_disabled_color;
}

void NavigationServer3D::set_debug_navigation_agent_path_point_size(real_t p_point_size)
{
	data->debug_navigation_agent_path_point_size = MAX(0.1, p_point_size);
	if (data->debug_navigation_agent_path_point_material.is_valid()) {
		data->debug_navigation_agent_path_point_material->set_point_size(
			data->debug_navigation_agent_path_point_size);
	}
}

real_t NavigationServer3D::get_debug_navigation_agent_path_point_size()
{
	return data->debug_navigation_agent_path_point_size;
}

void NavigationServer3D::set_debug_navigation_agent_path_color(const Color& p_color)
{
	data->debug_navigation_agent_path_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_agent_path_color()
{
	return data->debug_navigation_agent_path_color;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_connections()
{
	return data->debug_navigation_enable_edge_connections;
}

void NavigationServer3D::set_debug_navigation_enable_edge_connections_xray(const bool p_value)
{
	data->debug_navigation_enable_edge_connections_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_connections_xray()
{
	return data->debug_navigation_enable_edge_connections_xray;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_lines()
{
	return data->debug_navigation_enable_edge_lines;
}

void NavigationServer3D::set_debug_navigation_enable_edge_lines_xray(const bool p_value)
{
	data->debug_navigation_enable_edge_lines_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_lines_xray()
{
	return data->debug_navigation_enable_edge_lines_xray;
}

bool NavigationServer3D::get_debug_navigation_enable_geometry_face_random_color()
{
	return data->debug_navigation_enable_geometry_face_random_color;
}

bool NavigationServer3D::get_debug_navigation_enable_link_connections()
{
	return data->debug_navigation_enable_link_connections;
}

void NavigationServer3D::set_debug_navigation_enable_link_connections_xray(const bool p_value)
{
	data->debug_navigation_enable_link_connections_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_link_connections_xray()
{
	return data->debug_navigation_enable_link_connections_xray;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_agents_radius()
{
	return data->debug_navigation_avoidance_enable_agents_radius;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_radius()
{
	return data->debug_navigation_avoidance_enable_obstacles_radius;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_static()
{
	return data->debug_navigation_avoidance_enable_obstacles_static;
}

void NavigationServer3D::set_debug_navigation_avoidance_agents_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_agents_radius_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_agents_radius_color()
{
	return data->debug_navigation_avoidance_agents_radius_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_obstacles_radius_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_obstacles_radius_color()
{
	return data->debug_navigation_avoidance_obstacles_radius_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_face_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_face_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_face_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_face_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_edge_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_edge_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_edge_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_edge_color;
}

bool NavigationServer3D::get_debug_navigation_enable_agent_paths()
{
	return data->debug_navigation_enable_agent_paths;
}

void NavigationServer3D::set_debug_navigation_enable_agent_paths_xray(const bool p_value)
{
	data->debug_navigation_enable_agent_paths_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_agent_paths_xray()
{
	return data->debug_navigation_enable_agent_paths_xray;
}

bool NavigationServer3D::get_debug_navigation_enabled() { return data->debug_navigation_enabled; }

bool NavigationServer3D::get_debug_avoidance_enabled() { return data->debug_avoidance_enabled; }

#endif // DEBUG_ENABLED

struct NavigationServer3D::NavMap
{
};
struct NavigationServer3D::NavRegion
{
};
struct NavigationServer3D::NavLink
{
};
struct NavigationServer3D::NavAgent
{
};
struct NavigationServer3D::NavObstacle
{
};

void NavigationServer3D::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void NavigationServer3D::finalize()
{
	if (data) {
		memdelete(data);
		data = nullptr;
	}
}

void NavigationServer3D::set_active(bool p_active) {}

void NavigationServer3D::sync() {}

void NavigationServer3D::process(double p_delta_time) {}

void NavigationServer3D::physics_process(double p_delta_time) {}

void NavigationServer3D::free_rid(RID p_rid) {}

int NavigationServer3D::get_process_info(ProcessInfo p_info) { return 0; }

RID NavigationServer3D::map_create() { return RID(); }

RID NavigationServer3D::region_create() { return RID(); }

void NavigationServer3D::region_set_map(RID p_region, RID p_map) {}

void NavigationServer3D::region_set_transform(RID p_region, Transform3D p_transform) {}

void NavigationServer3D::region_set_enabled(RID p_region, bool p_enabled) {}

void NavigationServer3D::region_set_use_edge_connections(RID p_region, bool p_enabled) {}

void NavigationServer3D::region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers) {}

void NavigationServer3D::region_set_enter_cost(RID p_region, real_t p_enter_cost) {}

void NavigationServer3D::region_set_travel_cost(RID p_region, real_t p_travel_cost) {}

RID NavigationServer3D::link_create() { return RID(); }

void NavigationServer3D::link_set_map(RID p_link, RID p_map) {}

void NavigationServer3D::link_set_start_position(RID p_link, Vector3 p_position) {}

void NavigationServer3D::link_set_end_position(RID p_link, Vector3 p_position) {}

void NavigationServer3D::link_set_enabled(RID p_link, bool p_enabled) {}

void NavigationServer3D::link_set_navigation_layers(RID p_link, uint32_t p_navigation_layers) {}

void NavigationServer3D::link_set_enter_cost(RID p_link, real_t p_enter_cost) {}

void NavigationServer3D::link_set_travel_cost(RID p_link, real_t p_travel_cost) {}

RID NavigationServer3D::obstacle_create() { return RID(); }

void NavigationServer3D::obstacle_set_map(RID p_obstacle, RID p_map) {}

void NavigationServer3D::obstacle_set_use_3d_avoidance(RID p_obstacle, bool p_enabled) {}

void NavigationServer3D::obstacle_set_position(RID p_obstacle, Vector3 p_position) {}

void NavigationServer3D::obstacle_set_radius(RID p_obstacle, real_t p_radius) {}

void NavigationServer3D::obstacle_set_vertices(RID p_obstacle, const Vector<Vector3>& p_vertices) {}

void NavigationServer3D::obstacle_set_height(RID p_obstacle, real_t p_height) {}

void NavigationServer3D::obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled) {}

void NavigationServer3D::obstacle_set_velocity(RID p_obstacle, Vector3 p_velocity) {}

void NavigationServer3D::obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers) {}

void NavigationServer3D::obstacle_set_paused(RID p_obstacle, bool p_paused) {}

RID NavigationServer3D::agent_create() { return RID(); }

void NavigationServer3D::agent_set_map(RID p_agent, RID p_map) {}

void NavigationServer3D::agent_set_position(RID p_agent, Vector3 p_position) {}

void NavigationServer3D::agent_set_velocity(RID p_agent, Vector3 p_velocity) {}

void NavigationServer3D::agent_set_velocity_forced(RID p_agent, Vector3 p_velocity) {}

void NavigationServer3D::agent_set_radius(RID p_agent, real_t p_radius) {}

void NavigationServer3D::agent_set_height(RID p_agent, real_t p_height) {}

void NavigationServer3D::agent_set_neighbor_distance(RID p_agent, real_t p_distance) {}

void NavigationServer3D::agent_set_max_neighbors(RID p_agent, int p_count) {}

void NavigationServer3D::agent_set_time_horizon_agents(RID p_agent, real_t p_time_horizon) {}

void NavigationServer3D::agent_set_time_horizon_obstacles(RID p_agent, real_t p_time_horizon) {}

void NavigationServer3D::agent_set_max_speed(RID p_agent, real_t p_max_speed) {}

void NavigationServer3D::agent_set_avoidance_layers(RID p_agent, uint32_t p_layers) {}

void NavigationServer3D::agent_set_avoidance_mask(RID p_agent, uint32_t p_mask) {}

void NavigationServer3D::agent_set_avoidance_priority(RID p_agent, real_t p_priority) {}

void NavigationServer3D::agent_set_paused(RID p_agent, bool p_paused) {}
#endif // !defined(NAVIGATION_3D_DISABLED)


// --- Merged from servers/xr/xr_server.cpp ---
#if !defined(XR_DISABLED)
#include "core/config/project_settings.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
// Inlined servers/xr/xr_server.compat.inc
#ifndef DISABLE_DEPRECATED

#include "servers/xr/xr_server.h"

void XRServer::_add_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker)
{
	add_tracker(p_tracker);
}

void XRServer::_remove_tracker_bind_compat_90645(const Ref<XRTracker>& p_tracker)
{
	remove_tracker(p_tracker);
}

Ref<XRTracker> XRServer::_get_tracker_bind_compat_90645(const String& p_name)
{
	return get_tracker(p_name);
}

void XRServer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED



#include "servers/xr/xr_server.h"

XRServer::XRMode XRServer::xr_mode = XRMODE_DEFAULT;

void XRServer::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void XRServer::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

XRServer::XRMode XRServer::get_xr_mode() { return xr_mode; }

void XRServer::set_xr_mode(XRServer::XRMode p_mode) { xr_mode = p_mode; }

double XRServer::get_world_scale()
{
	ERR_FAIL_NULL_V(data, 1.0);
	if (Renderer::data && Renderer::is_on_render_thread()) {
		// Return the value with which we're currently rendering,
		// if we're on the render thread
		return data->render_state.world_scale;
	}
	else {
		// Return our current value
		return data->world_scale;
	}
}

void XRServer::set_world_scale(double p_world_scale)
{
	ERR_FAIL_NULL(data);
	if (p_world_scale < 0.01) {
		p_world_scale = 0.01;
	}
	else if (p_world_scale > 1000.0) {
		p_world_scale = 1000.0;
	}

	data->world_scale = p_world_scale;
	set_render_world_scale(data->world_scale);
}

void XRServer::_set_render_world_scale(double p_world_scale)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.world_scale = p_world_scale;
}

void XRServer::_set_render_world_origin(const Transform3D& p_world_origin)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.world_origin = p_world_origin;
}

void XRServer::_set_render_reference_frame(const Transform3D& p_reference_frame)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_NULL(data);
	data->render_state.reference_frame = p_reference_frame;
}

Transform3D XRServer::get_reference_frame() { return Transform3D(); }

void XRServer::set_camera_locked_to_origin(bool p_enable)
{
	ERR_FAIL_NULL(data);
	data->camera_locked_to_origin = p_enable;
}

Ref<XRInterface> XRServer::get_primary_interface()
{
	Ref<XRInterface> xri = memnew(XRInterface);
	return xri;
}

void XRServer::remove_tracker(const Ref<XRTracker>& p_tracker) {}

void XRServer::set_primary_interface(const Ref<XRInterface>& p_primary_interface) {}

void XRServer::add_tracker(const Ref<XRTracker>& p_tracker) {}

Ref<XRTracker> XRServer::get_tracker(const String& p_name)
{
	Ref<XRTracker> xpt = memnew(XRTracker);
	return xpt;
}

void XRServer::set_render_world_scale(double p_scale) {}

void XRServer::remove_interface(const Ref<XRInterface>& p_interface) {}

Transform3D XRServer::get_world_origin() { return Transform3D(); }

void XRServer::set_world_origin(const Transform3D& p_world_origin) {}

void XRServer::_process() {}

void XRServer::clear_reference_frame() {}

void XRServer::add_interface(const Ref<XRInterface>& p_interface) {}

// ============================================================================
// Merged from servers/xr/xr_interface.cpp
// ============================================================================

#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/xr/xr_server.h"
#include "servers/xr/xr_interface.h"

bool XRInterface::is_primary()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), false);

	return XRServer::get_primary_interface() == this;
}

void XRInterface::set_primary(bool p_primary)
{
	ERR_FAIL_COND(!XRServer::is_initialized());

	if (p_primary) {
		ERR_FAIL_COND(!is_initialized());

		XRServer::set_primary_interface(this);
	}
	else if (XRServer::get_primary_interface() == this) {
		XRServer::set_primary_interface(nullptr);
	}
}

// query if this interface supports this play area mode
bool XRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_UNKNOWN;
}

// get the current play area mode
XRInterface::PlayAreaMode XRInterface::get_play_area_mode() const { return XR_PLAY_AREA_UNKNOWN; }

// change the play area mode, note that this should return false if the mode is not available
bool XRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	return p_mode == XR_PLAY_AREA_UNKNOWN;
}

// if available, returns an array of vectors denoting the play area the player can move around in
PackedVector3Array XRInterface::get_play_area() const
{
	// Return an empty array by default.
	// Note implementation is responsible for applying our reference frame and world scale to the
	// raw data. `play_area_changed` should be emitted if play area data is available and either the
	// reference frame or world scale changes.
	return PackedVector3Array();
}

/** these will only be implemented on AR interfaces, so we want dummies for VR **/
bool XRInterface::get_anchor_detection_is_enabled() const { return false; }

void XRInterface::set_anchor_detection_is_enabled(bool p_enable) {}

int XRInterface::get_camera_feed_id() { return 0; }

RID XRInterface::get_vrs_texture() { return RID(); }

/** these are optional, so we want dummies **/

RID XRInterface::get_color_texture() { return RID(); }

RID XRInterface::get_depth_texture() { return RID(); }

RID XRInterface::get_velocity_texture() { return RID(); }

RID XRInterface::get_velocity_depth_texture() { return RID(); }

Size2i XRInterface::get_velocity_target_size() { return Size2i(); }

Rect2i XRInterface::get_render_region() { return Rect2i(); }

PackedStringArray XRInterface::get_suggested_tracker_names() const
{
	PackedStringArray arr;

	return arr;
}

PackedStringArray XRInterface::get_suggested_pose_names(const String& p_tracker_name) const
{
	PackedStringArray arr;

	return arr;
}

XRInterface::TrackingStatus XRInterface::get_tracking_status() const { return XR_UNKNOWN_TRACKING; }

void XRInterface::trigger_haptic_pulse(const String& p_action_name,
	const String& p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec,
	double p_delay_sec)
{
}

String XRInterface::get_name() const { return String(); }

uint32_t XRInterface::get_capabilities() const { return 0; }

bool XRInterface::is_initialized() const { return true; }

bool XRInterface::initialize() { return true; }

void XRInterface::uninitialize() {}

Transform3D XRInterface::get_camera_transform() { return Transform3D(); }

void XRInterface::process() {}

Size2 XRInterface::get_render_target_size() { return Size2(); }

uint32_t XRInterface::get_view_count() { return 0; }

Transform3D XRInterface::get_transform_for_view(uint32_t p_view, const Transform3D& p_cam_transform)
{
	return Transform3D();
}

Projection XRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	return Projection();
}

Vector<RenderingServerTypes::BlitToScreen> XRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	return Vector<RenderingServerTypes::BlitToScreen>();
}

// XRVRS

XRVRS::~XRVRS()
{
	if (vrs_texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(vrs_texture);
		vrs_texture = RID();
	}
}

float XRVRS::get_vrs_min_radius() const { return vrs_min_radius; }

void XRVRS::set_vrs_min_radius(float p_vrs_min_radius)
{
	if (p_vrs_min_radius < 1.0) {
		WARN_PRINT_ONCE("VRS minimum radius can not be set below 1.0");
		vrs_min_radius = 1.0;
	}
	else if (p_vrs_min_radius > 100.0) {
		WARN_PRINT_ONCE("VRS minimum radius can not be set above 100.0");
		vrs_min_radius = 100.0;
	}
	else {
		vrs_min_radius = p_vrs_min_radius;
		vrs_dirty = true;
	}
}

float XRVRS::get_vrs_strength() const { return vrs_strength; }

void XRVRS::set_vrs_strength(float p_vrs_strength)
{
	if (p_vrs_strength < 0.1) {
		WARN_PRINT_ONCE("VRS strength can not be set below 0.1");
		vrs_strength = 0.1;
	}
	else if (p_vrs_strength > 10.0) {
		WARN_PRINT_ONCE("VRS strength can not be set above 10.0");
		vrs_strength = 10.0;
	}
	else {
		vrs_strength = p_vrs_strength;
		vrs_dirty = true;
	}
}

Rect2i XRVRS::get_vrs_render_region() const { return vrs_render_region; }

void XRVRS::set_vrs_render_region(const Rect2i& p_vrs_render_region)
{
	vrs_render_region = p_vrs_render_region;
	vrs_dirty = true;
}

RID XRVRS::make_vrs_texture(const Size2& p_target_size, const Vector<Vector2>& p_eye_foci)
{
	ERR_FAIL_COND_V(p_eye_foci.is_empty(), RID());

	Size2i texel_size = RD::vrs_get_texel_size();

	// Should return sensible data or graphics API does not support VRS.
	ERR_FAIL_COND_V(texel_size.x < 1 || texel_size.y < 1, RID());

	Size2 vrs_size =
		Size2(0.5 + p_target_size.x / texel_size.x, 0.5 + p_target_size.y / texel_size.y).floor();

	// Make sure we have at least one pixel.
	vrs_size = vrs_size.maxf(1.0);

	float max_radius =
		0.5 * MIN(vrs_size.x, vrs_size.y); // Maximum radius that fits inside of our image
	float min_radius =
		vrs_min_radius * max_radius / 100.0; // Minimum radius as a percentage of our size
	real_t outer_radius = MAX(1.0, (max_radius - min_radius) / vrs_strength);
	Size2 vrs_sizei = vrs_size;

	// Our density map is now unified, with a value of (0.0, 0.0) meaning a 1x1 texel size and
	// (1.0, 1.0) an max texel size. For our standard VRS extension on Vulkan this means a maximum
	// of 8x8. For the density map extension this scales depending on the max texel size.

	if (target_size != vrs_sizei || eye_foci != p_eye_foci || vrs_dirty) {
		// Out with the old.
		if (vrs_texture.is_valid()) {
			Renderer::free_rid(vrs_texture);
			vrs_texture = RID();
		}

		// In with the new.
		Vector<Ref<Image>> images;
		target_size = vrs_sizei;
		eye_foci = p_eye_foci;

		Size2 region_ratio = Size2(1.0, 1.0);
		Point2i region_offset;
		if (vrs_render_region != Rect2i()) {
			region_ratio = (Size2)vrs_render_region.size / p_target_size;
			region_offset = (Point2)vrs_render_region.position / p_target_size * vrs_sizei;
		}

		for (int i = 0; i < eye_foci.size() && i < RendererSceneRender::MAX_RENDER_VIEWS; i++) {
			PackedByteArray data;
			data.resize(vrs_sizei.x * vrs_sizei.y * 2);
			uint8_t* data_ptr = data.ptrw();

			Vector2i view_center;
			view_center.x =
				int(vrs_size.x * (eye_foci[i].x + 1.0) * region_ratio.x * 0.5) + region_offset.x;
			view_center.y =
				int(vrs_size.y * (-eye_foci[i].y + 1.0) * region_ratio.y * 0.5) + region_offset.y;

			int d = 0;
			for (int y = 0; y < vrs_sizei.y; y++) {
				for (int x = 0; x < vrs_sizei.x; x++) {
					Vector2 offset = Vector2(x - view_center.x, y - view_center.y) / region_ratio;
					real_t density = MAX(offset.length() - min_radius, 0.0) / outer_radius;
					data_ptr[d++] = CLAMP(255.0 * density, 0, 255);
					data_ptr[d++] = CLAMP(255.0 * density, 0, 255);
				}
			}
			images.push_back(
				Image::create_from_data(vrs_sizei.x, vrs_sizei.y, false, Image::FORMAT_RG8, data));
		}

		if (images.size() == 1) {
			vrs_texture = Renderer::texture_2d_create(images[0]);
		}
		else {
			vrs_texture = Renderer::texture_2d_layered_create(
				images, RSE::TEXTURE_LAYERED_2D_ARRAY);
		}

		vrs_dirty = false;
	}

	return vrs_texture;
}



// ============================================================================
// Merged from servers/xr/xr_tracker.cpp
// ============================================================================

#include "servers/xr/xr_server.h"

// XRPose

void XRPose::set_has_tracking_data(const bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRPose::get_has_tracking_data() const { return has_tracking_data; }

void XRPose::set_name(const String& p_name) { name = p_name; }

String XRPose::get_name() const { return name; }

void XRPose::set_transform(const Transform3D p_transform) { transform = p_transform; }

Transform3D XRPose::get_transform() const { return transform; }

Transform3D XRPose::get_adjusted_transform() const
{
	Transform3D adjusted_transform = transform;

	ERR_FAIL_COND_V(!XRServer::is_initialized(), transform);

	// apply world scale
	adjusted_transform.origin *= XRServer::get_world_scale();

	// apply reference frame
	adjusted_transform = XRServer::get_reference_frame() * adjusted_transform;

	return adjusted_transform;
}

void XRPose::set_linear_velocity(const Vector3 p_velocity) { linear_velocity = p_velocity; }

Vector3 XRPose::get_linear_velocity() const { return linear_velocity; }

void XRPose::set_angular_velocity(const Vector3 p_velocity) { angular_velocity = p_velocity; }

Vector3 XRPose::get_angular_velocity() const { return angular_velocity; }

void XRPose::set_tracking_confidence(const XRPose::TrackingConfidence p_tracking_confidence)
{
	tracking_confidence = p_tracking_confidence;
}

XRPose::TrackingConfidence XRPose::get_tracking_confidence() const { return tracking_confidence; }

// XRTracker

XRTracker::BodyData* XRTracker::_ensure_body_data()
{
	if (!body_data) {
		body_data = memnew(BodyData);
	}
	return body_data;
}

XRTracker::HandData* XRTracker::_ensure_hand_data()
{
	if (!hand_data) {
		hand_data = memnew(HandData);
	}
	return hand_data;
}

void XRTracker::set_tracker_type(XRServer::TrackerType p_type)
{
	type = p_type;
	if (type == XRServer::TRACKER_BODY) {
		_ensure_body_data();
	}
	else if (type == XRServer::TRACKER_HAND) {
		_ensure_hand_data();
		if (tracker_hand == TRACKER_HAND_UNKNOWN) {
			tracker_hand = TRACKER_HAND_LEFT;
		}
	}
}

XRServer::TrackerType XRTracker::get_tracker_type() const { return type; }

void XRTracker::set_tracker_name(const String& p_name)
{
	// Note: this should not be changed after the tracker is registered with the XRServer!
	name = p_name;
}

String XRTracker::get_tracker_name() const { return name; }

void XRTracker::set_tracker_desc(const String& p_desc) { description = p_desc; }

String XRTracker::get_tracker_desc() const { return description; }

void XRTracker::set_tracker_profile(const String& p_profile) { profile = p_profile; }

String XRTracker::get_tracker_profile() const { return profile; }

XRTracker::TrackerHand XRTracker::get_tracker_hand() const { return tracker_hand; }

void XRTracker::set_tracker_hand(const XRTracker::TrackerHand p_hand)
{
	ERR_FAIL_INDEX(p_hand, TRACKER_HAND_MAX);
	if (type == XRServer::TRACKER_BODY) {
		ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_UNKNOWN, "Body tracker cannot specify hand.");
	}
	else if (type == XRServer::TRACKER_HAND) {
		ERR_FAIL_COND_MSG(p_hand != TRACKER_HAND_LEFT && p_hand != TRACKER_HAND_RIGHT,
			"Hand tracker must specify hand.");
	}
	tracker_hand = p_hand;
}

void XRTracker::set_has_tracking_data(bool p_has_tracking_data)
{
	has_tracking_data = p_has_tracking_data;
}

bool XRTracker::get_has_tracking_data() const { return has_tracking_data; }

bool XRTracker::has_pose(const String& p_action_name) const
{
	return poses.has(p_action_name);
}

Ref<XRPose> XRTracker::get_pose(const String& p_action_name) const
{
	Ref<XRPose> pose;
	if (poses.has(p_action_name)) {
		pose = poses[p_action_name];
	}
	return pose;
}

void XRTracker::invalidate_pose(const String& p_action_name)
{
	if (poses.has(p_action_name)) {
		poses[p_action_name]->set_has_tracking_data(false);
	}
}

void XRTracker::set_pose(const String& p_action_name, const Transform3D& p_transform,
	const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
	const XRPose::TrackingConfidence p_tracking_confidence)
{
	Ref<XRPose> new_pose;
	if (poses.has(p_action_name)) {
		new_pose = poses[p_action_name];
	}
	else {
		new_pose.instantiate();
		new_pose->set_name(p_action_name);
		poses[p_action_name] = new_pose;
	}

	new_pose->set_has_tracking_data(p_tracking_confidence != XRPose::XR_TRACKING_CONFIDENCE_NONE);
	new_pose->set_transform(p_transform);
	new_pose->set_linear_velocity(p_linear_velocity);
	new_pose->set_angular_velocity(p_angular_velocity);
	new_pose->set_tracking_confidence(p_tracking_confidence);
}

void XRTracker::set_input(const String& p_action_name, bool p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_BOOL;
	val.bool_val = p_value;
	inputs[p_action_name] = val;
}

void XRTracker::set_input(const String& p_action_name, float p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_FLOAT;
	val.float_val = p_value;
	inputs[p_action_name] = val;
}

void XRTracker::set_input(const String& p_action_name, const Vector2& p_value)
{
	InputValue val;
	val.type = InputValue::INPUT_VECTOR2;
	val.vec2_val.x = p_value.x;
	val.vec2_val.y = p_value.y;
	inputs[p_action_name] = val;
}

XRTracker::InputValue XRTracker::get_input(const String& p_action_name) const
{
	if (inputs.has(p_action_name)) {
		return inputs[p_action_name];
	}
	return InputValue();
}

bool XRTracker::is_button_pressed(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (!val) {
		return false;
	}
	if (val->type == InputValue::INPUT_BOOL) {
		return val->bool_val;
	}
	if (val->type == InputValue::INPUT_FLOAT) {
		return val->float_val > 0.0f;
	}
	return false;
}

float XRTracker::get_input_float(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (!val) {
		return 0.0f;
	}
	if (val->type == InputValue::INPUT_FLOAT) {
		return val->float_val;
	}
	if (val->type == InputValue::INPUT_BOOL) {
		return val->bool_val ? 1.0f : 0.0f;
	}
	return 0.0f;
}

Vector2 XRTracker::get_input_vector2(const String& p_action_name) const
{
	const InputValue* val = inputs.getptr(p_action_name);
	if (val && val->type == InputValue::INPUT_VECTOR2) {
		return Vector2(val->vec2_val.x, val->vec2_val.y);
	}
	return Vector2();
}

void XRTracker::set_body_flags(uint32_t p_body_flags)
{
	_ensure_body_data()->body_flags = p_body_flags;
}

uint32_t XRTracker::get_body_flags() const
{
	return body_data ? body_data->body_flags : 0;
}

void XRTracker::set_joint_flags(Joint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	_ensure_body_data()->joint_flags[p_joint] = p_flags;
}

uint32_t XRTracker::get_joint_flags(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, uint32_t());
	return body_data ? body_data->joint_flags[p_joint] : 0;
}

void XRTracker::set_joint_transform(Joint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, JOINT_MAX);
	_ensure_body_data()->joint_transforms[p_joint] = p_transform;
}

Transform3D XRTracker::get_joint_transform(Joint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, JOINT_MAX, Transform3D());
	return body_data ? body_data->joint_transforms[p_joint] : Transform3D();
}

void XRTracker::set_hand_tracking_source(HandTrackingSource p_source)
{
	_ensure_hand_data()->hand_tracking_source = p_source;
}

XRTracker::HandTrackingSource XRTracker::get_hand_tracking_source() const
{
	return hand_data ? hand_data->hand_tracking_source : HAND_TRACKING_SOURCE_UNKNOWN;
}

void XRTracker::set_hand_joint_flags(HandJoint p_joint, uint32_t p_flags)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_flags[p_joint] = p_flags;
}

uint32_t XRTracker::get_hand_joint_flags(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, uint32_t());
	return hand_data ? hand_data->hand_joint_flags[p_joint] : 0;
}

void XRTracker::set_hand_joint_transform(HandJoint p_joint, const Transform3D& p_transform)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_transforms[p_joint] = p_transform;
}

Transform3D XRTracker::get_hand_joint_transform(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Transform3D());
	return hand_data ? hand_data->hand_joint_transforms[p_joint] : Transform3D();
}

void XRTracker::set_hand_joint_radius(HandJoint p_joint, float p_radius)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_radii[p_joint] = p_radius;
}

float XRTracker::get_hand_joint_radius(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, 0.0f);
	return hand_data ? hand_data->hand_joint_radii[p_joint] : 0.0f;
}

void XRTracker::set_hand_joint_linear_velocity(HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_linear_velocities[p_joint] = p_velocity;
}

Vector3 XRTracker::get_hand_joint_linear_velocity(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_data ? hand_data->hand_joint_linear_velocities[p_joint] : Vector3();
}

void XRTracker::set_hand_joint_angular_velocity(HandJoint p_joint, const Vector3& p_velocity)
{
	ERR_FAIL_INDEX(p_joint, HAND_JOINT_MAX);
	_ensure_hand_data()->hand_joint_angular_velocities[p_joint] = p_velocity;
}

Vector3 XRTracker::get_hand_joint_angular_velocity(HandJoint p_joint) const
{
	ERR_FAIL_INDEX_V(p_joint, HAND_JOINT_MAX, Vector3());
	return hand_data ? hand_data->hand_joint_angular_velocities[p_joint] : Vector3();
}

float XRTracker::get_blend_shape(BlendShapeEntry p_blend_shape) const
{
	ERR_FAIL_INDEX_V(p_blend_shape, FT_MAX, 0.0f);
	if (blend_shapes.size() != FT_MAX) {
		return 0.0f;
	}
	return blend_shapes[p_blend_shape];
}

void XRTracker::set_blend_shape(BlendShapeEntry p_blend_shape, float p_value)
{
	ERR_FAIL_INDEX(p_blend_shape, FT_MAX);
	if (blend_shapes.size() != FT_MAX) {
		blend_shapes.resize_initialized(FT_MAX);
	}
	blend_shapes.set(p_blend_shape, p_value);
}

PackedFloat32Array XRTracker::get_blend_shapes() const
{
	if (blend_shapes.size() == FT_MAX) {
		return blend_shapes;
	}
	PackedFloat32Array data;
	data.resize_initialized(FT_MAX);
	return data;
}

void XRTracker::set_blend_shapes(const PackedFloat32Array& p_blend_shapes)
{
	ERR_FAIL_COND(p_blend_shapes.size() != FT_MAX);
	blend_shapes = p_blend_shapes;
}

XRTracker::XRTracker(XRServer::TrackerType p_type)
{
	if (p_type != XRServer::TRACKER_UNKNOWN) {
		set_tracker_type(p_type);
	}
}

XRTracker::~XRTracker()
{
	if (body_data) {
		memdelete(body_data);
		body_data = nullptr;
	}
	if (hand_data) {
		memdelete(hand_data);
		hand_data = nullptr;
	}
}
#endif // !defined(XR_DISABLED)
