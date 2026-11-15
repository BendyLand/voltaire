/**************************************************************************/
/*  camera_server.h                                                       */
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

#pragma once

#include "core/os/thread_safe.h"
#include "core/templates/rid.h"
#include "core/types.h"

/**
	The camera server is a singleton object that gives access to the various
	camera feeds that can be used as the background for our environment.
**/

class CameraFeed;
template <typename T> class TypedArray;

class CameraServer final
{
	static inline Mutex _thread_safe_;

public:
	enum FeedImage
	{
		FEED_RGBA_IMAGE = 0,
		FEED_YCBCR_IMAGE = 0,
		FEED_Y_IMAGE = 0,
		FEED_CBCR_IMAGE = 1,
		FEED_IMAGES = 2
	};

	static inline constexpr const char feeds_updated_signal_name[] = "camera_feeds_updated";

	struct Data
	{
		bool monitoring_feeds = false;
		Vector<Ref<CameraFeed>> feeds;
	};

	static inline Data* data = nullptr;

private:
	static void _default_initialize_backend() {}
	static void _default_finalize_backend() {}
	static void _default_set_monitoring_feeds(bool p_monitoring_feeds);
	static void _default_handle_application_pause() {}
	static void _default_handle_application_resume() {}
	static void _default_handle_display_rotation_change(int p_orientation) { (void)p_orientation; }

	inline static void (*_initialize_backend)() = &_default_initialize_backend;
	inline static void (*_finalize_backend)() = &_default_finalize_backend;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void set_monitoring_feeds_state(bool p_monitoring_feeds);
	_FORCE_INLINE_ static bool is_monitoring_feeds()
	{
		return data ? data->monitoring_feeds : false;
	}

	inline static void (*set_monitoring_feeds)(
		bool p_monitoring_feeds) = &_default_set_monitoring_feeds;
	inline static void (*handle_application_pause)() = &_default_handle_application_pause;
	inline static void (*handle_application_resume)() = &_default_handle_application_resume;
	inline static void (*handle_display_rotation_change)(
		int p_orientation) = &_default_handle_display_rotation_change;

	template <typename Impl> static void bind()
	{
		_initialize_backend = &Impl::initialize;
		_finalize_backend = &Impl::finalize;
		set_monitoring_feeds = &Impl::set_monitoring_feeds;
		handle_application_pause = &Impl::handle_application_pause;
		handle_application_resume = &Impl::handle_application_resume;
		handle_display_rotation_change = &Impl::handle_display_rotation_change;
	}

	static void reset_backend()
	{
		_initialize_backend = &_default_initialize_backend;
		_finalize_backend = &_default_finalize_backend;
		set_monitoring_feeds = &_default_set_monitoring_feeds;
		handle_application_pause = &_default_handle_application_pause;
		handle_application_resume = &_default_handle_application_resume;
		handle_display_rotation_change = &_default_handle_display_rotation_change;
	}

	// Right now we identify our feed by it's ID when it's used in the background.
	// May see if we can change this to purely relying on CameraFeed objects or by name.
	static int get_free_id();
	static int get_feed_index(int p_id);
	static Ref<CameraFeed> get_feed_by_id(int p_id);

	// Add and remove feeds.
	static void add_feed(const Ref<CameraFeed>& p_feed);
	static void remove_feed(const Ref<CameraFeed>& p_feed);

	// Get our feeds.
	static Ref<CameraFeed> get_feed(int p_index);
	static int get_feed_count();

	// Intended for use with custom CameraServer implementation.
	static RID feed_texture(int p_id, FeedImage p_texture);

	CameraServer() = delete;
	CameraServer(const CameraServer&) = delete;
	CameraServer& operator=(const CameraServer&) = delete;
	~CameraServer() = delete;
};


