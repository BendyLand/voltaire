/**************************************************************************/
/*  camera_linux.h                                                        */
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

#include "core/os/mutex.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "servers/camera/camera_server.h"


#include "core/io/image.h"
#include "core/templates/vector.h"

class CameraFeed;

struct StreamingBuffer
{
	void* start = nullptr;
	size_t length = 0;
};

class BufferDecoder
{
protected:
	CameraFeed* camera_feed = nullptr;
	Ref<Image> image;
	int width = 0;
	int height = 0;

public:
	virtual void decode(StreamingBuffer p_buffer);

	BufferDecoder() = default;
	BufferDecoder(CameraFeed* p_camera_feed);

	virtual ~BufferDecoder();
};

class AbstractYuyvBufferDecoder : public BufferDecoder
{
protected:
	int* component_indexes = nullptr;

public:
	AbstractYuyvBufferDecoder(CameraFeed* p_camera_feed);
	~AbstractYuyvBufferDecoder();
};

class SeparateYuyvBufferDecoder final : public AbstractYuyvBufferDecoder
{
private:
	Vector<uint8_t> y_image_data;
	Vector<uint8_t> cbcr_image_data;
	Ref<Image> y_image;
	Ref<Image> cbcr_image;

public:
	SeparateYuyvBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class YuyvToGrayscaleBufferDecoder final : public AbstractYuyvBufferDecoder
{
private:
	Vector<uint8_t> image_data;

public:
	YuyvToGrayscaleBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class YuyvToRgbBufferDecoder final : public AbstractYuyvBufferDecoder
{
private:
	Vector<uint8_t> image_data;

public:
	YuyvToRgbBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class CopyBufferDecoder final : public BufferDecoder
{
private:
	Vector<uint8_t> image_data;
	bool rgba = false;

public:
	CopyBufferDecoder(CameraFeed* p_camera_feed, bool p_rgba);
	virtual void decode(StreamingBuffer p_buffer) override;
};

class JpegBufferDecoder final : public BufferDecoder
{
private:
	Vector<uint8_t> image_data;

public:
	JpegBufferDecoder(CameraFeed* p_camera_feed);
	virtual void decode(StreamingBuffer p_buffer) override;
};

#include <linux/videodev2.h>
#include "servers/camera/camera_feed.h"

class CameraFeedLinux final : public CameraFeed
{
private:
	SafeFlag exit_flag;
	Thread* thread = nullptr;
	String device_name;
	int file_descriptor = -1;
	StreamingBuffer* buffers = nullptr;
	unsigned int buffer_count = 0;
	BufferDecoder* buffer_decoder = nullptr;

	static void update_buffer_thread_func(void* p_func);

	void _update_buffer();
	void _query_device(const String& p_device_name);
	void _add_format(v4l2_fmtdesc description, v4l2_frmsize_discrete size, int frame_numerator,
		int frame_denominator);
	bool _request_buffers();
	bool _start_capturing();
	void _read_frame();
	void _stop_capturing();
	void _unmap_buffers(unsigned int p_count);
	BufferDecoder* _create_buffer_decoder();
	void _start_thread();

public:
	String get_device_name() const;
	bool activate_feed() override;
	void deactivate_feed() override;
	FeedFormat get_format() const override;

	CameraFeedLinux(const String& p_device_name);
	~CameraFeedLinux() override;
};
class CameraLinux final
{
private:
	struct Data
	{
		SafeFlag exit_flag;
		Thread camera_thread;
		Mutex camera_mutex;
	};

	static inline Data* data = nullptr;

	static void camera_thread_func(void* p_userdata);

	static void _update_devices();
	static bool _has_device(const String& p_device_name);
	static void _add_device(const String& p_device_name);
	static int _open_device(const String& p_device_name);
	static bool _is_active(const String& p_device_name);
	static bool _is_video_capture_device(int p_file_descriptor);
	static bool _can_query_format(int p_file_descriptor, int p_type);

public:
	static void initialize();
	static void finalize();

	static void set_monitoring_feeds(bool p_monitoring_feeds);
	static void handle_application_pause() {}
	static void handle_application_resume() {}
	static void handle_display_rotation_change(int p_orientation) { (void)p_orientation; }

	CameraLinux() = delete;
	CameraLinux(const CameraLinux&) = delete;
	CameraLinux& operator=(const CameraLinux&) = delete;
	~CameraLinux() = delete;
};
