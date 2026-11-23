/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#if defined(LINUXBSD_ENABLED)
// --- Inlined/Folded from modules/camera/camera_linux.h ---
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
#endif
#if defined(WINDOWS_ENABLED)
// --- Inlined/Folded from modules/camera/camera_win.h ---
#include "servers/camera/camera_server.h"

class CameraWindows final
{
private:
	static void add_active_cameras();

public:
	static void initialize();
	static void finalize() {}

	static void set_monitoring_feeds(bool p_monitoring_feeds)
	{
		CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	}

	static void handle_application_pause() {}
	static void handle_application_resume() {}
	static void handle_display_rotation_change(int p_orientation) { (void)p_orientation; }

	CameraWindows() = delete;
	CameraWindows(const CameraWindows&) = delete;
	CameraWindows& operator=(const CameraWindows&) = delete;
	~CameraWindows() = delete;
};
#endif
#if defined(MACOS_ENABLED)
#include "camera_apple.h"
#endif
#if defined(ANDROID_ENABLED)
// --- Inlined/Folded from modules/camera/camera_android.h ---
#include "core/os/semaphore.h"
#include "core/templates/safe_refcount.h"
#include "servers/camera/camera_feed.h"
#include "servers/camera/camera_server.h"

#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadataTags.h>
#include <media/NdkImageReader.h>

#include <optional>

enum class CameraFacing {
	BACK = 0,
	FRONT = 1,
};

struct CameraRotationParams {
	int sensor_orientation;
	CameraFacing camera_facing;
	int display_rotation;
};

class CameraFeedAndroid : public CameraFeed {

private:
	String camera_id;
	int32_t orientation;
	Ref<Image> image_y;
	Ref<Image> image_uv;
	Vector<uint8_t> data_y;
	Vector<uint8_t> data_uv;
	// Scratch buffers to avoid reallocations when compacting stride-aligned planes.
	Vector<uint8_t> scratch_y;
	Vector<uint8_t> scratch_uv;

	ACameraManager *manager = nullptr;
	ACameraMetadata *metadata = nullptr;
	ACameraDevice *device = nullptr;
	AImageReader *reader = nullptr;
	ACameraCaptureSession *session = nullptr;
	ACaptureRequest *request = nullptr;
	ACaptureSessionOutput *session_output = nullptr;
	ACaptureSessionOutputContainer *session_output_container = nullptr;
	ACameraOutputTarget *camera_output_target = nullptr;
	Mutex callback_mutex;
	Semaphore session_closed_semaphore;
	SafeFlag is_deactivating;
	SafeFlag session_close_pending;
	SafeFlag device_error_occurred;
	bool was_active_before_pause = false;

	// Callback structures - must be instance members, not static, to ensure
	// correct 'this' pointer is captured for each camera feed instance.
	ACameraDevice_stateCallbacks device_callbacks = {};
	AImageReader_ImageListener image_listener = {};
	ACameraCaptureSession_stateCallbacks session_callbacks = {};

	void _add_formats();
	void _set_rotation();
	void refresh_camera_metadata();
	static std::optional<int> calculate_rotation(const CameraRotationParams &p_params);
	static int normalize_angle(int p_angle);
	static int get_display_rotation();
	static int get_app_orientation();
	static void compact_stride_inplace(uint8_t *data, size_t width, int height, size_t stride);

	static void onError(void *context, ACameraDevice *p_device, int error);
	static void onDisconnected(void *context, ACameraDevice *p_device);
	static void onImage(void *context, AImageReader *p_reader);
	static void onSessionReady(void *context, ACameraCaptureSession *session);
	static void onSessionActive(void *context, ACameraCaptureSession *session);
	static void onSessionClosed(void *context, ACameraCaptureSession *session);

protected:
public:
	bool activate_feed() override;
	void deactivate_feed() override;
	bool set_format(int p_index, const Dictionary &p_parameters) override;
	Array get_formats() const override;
	FeedFormat get_format() const override;
	void handle_pause();
	void handle_resume();
	void handle_rotation_change();

	CameraFeedAndroid(ACameraManager *manager, ACameraMetadata *metadata, const char *id,
			CameraFeed::FeedPosition position, int32_t orientation);
	~CameraFeedAndroid() override;
};

class CameraAndroid final
{
private:
	struct Data
	{
		ACameraManager* camera_manager = nullptr;
	};

	static inline Data* data = nullptr;

	static void update_feeds();
	static void remove_all_feeds();

public:
	static void initialize();
	static void finalize();

	static void set_monitoring_feeds(bool p_monitoring_feeds);
	static void handle_application_pause();
	static void handle_application_resume();
	static void handle_display_rotation_change(int p_orientation);

	CameraAndroid() = delete;
	CameraAndroid(const CameraAndroid&) = delete;
	CameraAndroid& operator=(const CameraAndroid&) = delete;
	~CameraAndroid() = delete;
};
#endif

void initialize_camera_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#if defined(LINUXBSD_ENABLED)
	CameraServer::bind<CameraLinux>();
#endif
#if defined(WINDOWS_ENABLED)
	CameraServer::bind<CameraWindows>();
#endif
#if defined(MACOS_ENABLED)
	CameraServer::bind<CameraApple>();
#endif
#if defined(ANDROID_ENABLED)
	CameraServer::bind<CameraAndroid>();
#endif
}

void uninitialize_camera_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}


// --- Merged from modules/camera/camera_linux.cpp ---
#if defined(LINUXBSD_ENABLED)
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>


void CameraLinux::camera_thread_func(void* p_userdata)
{
	(void)p_userdata;
	_update_devices();
}

void CameraLinux::_update_devices()
{
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(CameraServer::data);

	while (!data->exit_flag.is_set()) {
		{
			MutexLock lock(data->camera_mutex);

			for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
				Ref<CameraFeedLinux> feed = (Ref<CameraFeedLinux>)CameraServer::data->feeds[i];
				if (feed.is_null()) {
					continue;
				}
				String device_name = feed->get_device_name();
				if (!_is_active(device_name)) {
					CameraServer::remove_feed(feed);
				}
			}

			struct dirent** devices;
			int count = scandir("/dev", &devices, nullptr, alphasort);

			if (count != -1) {
				for (int i = 0; i < count; i++) {
					struct dirent* device = devices[i];
					if (strncmp(device->d_name, "video", 5) == 0) {
						String device_name = String("/dev/") + String(device->d_name);
						if (!_has_device(device_name)) {
							_add_device(device_name);
						}
					}
					free(device);
				}
			}

			free(devices);
		}
		usleep(1000000);
	}
}

bool CameraLinux::_has_device(const String& p_device_name)
{
	ERR_FAIL_NULL_V(CameraServer::data, false);

	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedLinux> feed = (Ref<CameraFeedLinux>)CameraServer::data->feeds[i];
		if (feed.is_null()) {
			continue;
		}
		if (feed->get_device_name() == p_device_name) {
			return true;
		}
	}
	return false;
}

void CameraLinux::_add_device(const String& p_device_name)
{
	int file_descriptor = _open_device(p_device_name);

	if (file_descriptor != -1) {
		if (_is_video_capture_device(file_descriptor)) {
			Ref<CameraFeedLinux> feed = memnew(CameraFeedLinux(p_device_name));
			CameraServer::add_feed(feed);
		}
	}

	close(file_descriptor);
}

int CameraLinux::_open_device(const String& p_device_name)
{
	struct stat s;

	if (stat(p_device_name.ascii().get_data(), &s) == -1) {
		return -1;
	}

	if (!S_ISCHR(s.st_mode)) {
		return -1;
	}

	return open(p_device_name.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
}

// TODO any cheaper/cleaner way to check if file descriptor is invalid?
bool CameraLinux::_is_active(const String& p_device_name)
{
	struct v4l2_capability capability;
	bool result = false;
	int file_descriptor = _open_device(p_device_name);
	if (file_descriptor != -1 && ioctl(file_descriptor, VIDIOC_QUERYCAP, &capability) != -1) {
		result = true;
	}
	close(file_descriptor);
	return result;
}

bool CameraLinux::_is_video_capture_device(int p_file_descriptor)
{
	struct v4l2_capability capability;

	if (ioctl(p_file_descriptor, VIDIOC_QUERYCAP, &capability) == -1) {
		return false;
	}

	if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		return false;
	}

	if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
		return false;
	}

	return _can_query_format(p_file_descriptor, V4L2_BUF_TYPE_VIDEO_CAPTURE);
}

bool CameraLinux::_can_query_format(int p_file_descriptor, int p_type)
{
	struct v4l2_format format;
	memset(&format, 0, sizeof(format));
	format.type = p_type;

	return ioctl(p_file_descriptor, VIDIOC_G_FMT, &format) != -1;
}

void CameraLinux::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void CameraLinux::finalize()
{
	if (!data) {
		return;
	}
	data->exit_flag.set();
	if (data->camera_thread.is_started()) {
		data->camera_thread.wait_to_finish();
	}
	memdelete(data);
	data = nullptr;
}

void CameraLinux::set_monitoring_feeds(bool p_monitoring_feeds)
{
	ERR_FAIL_NULL(data);
	if (p_monitoring_feeds == CameraServer::is_monitoring_feeds()) {
		return;
	}

	CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	if (p_monitoring_feeds) {
		data->exit_flag.clear();
		data->camera_thread.start(CameraLinux::camera_thread_func, nullptr);
	}
	else {
		data->exit_flag.set();
		if (data->camera_thread.is_started()) {
			data->camera_thread.wait_to_finish();
		}
	}
}

void BufferDecoder::decode(StreamingBuffer p_buffer) {}

BufferDecoder::~BufferDecoder() {}

BufferDecoder::BufferDecoder(CameraFeed* p_camera_feed)
{
	camera_feed = p_camera_feed;
	width = camera_feed->get_format().width;
	height = camera_feed->get_format().height;
	image.instantiate();
}

AbstractYuyvBufferDecoder::AbstractYuyvBufferDecoder(CameraFeed* p_camera_feed)
	: BufferDecoder(p_camera_feed)
{
	switch (camera_feed->get_format().pixel_format) {
	case V4L2_PIX_FMT_YYUV:
		component_indexes = new int[4]{0, 1, 2, 3};
		break;
	case V4L2_PIX_FMT_YVYU:
		component_indexes = new int[4]{0, 2, 3, 1};
		break;
	case V4L2_PIX_FMT_UYVY:
		component_indexes = new int[4]{1, 3, 0, 2};
		break;
	case V4L2_PIX_FMT_VYUY:
		component_indexes = new int[4]{1, 3, 2, 0};
		break;
	default:
		component_indexes = new int[4]{0, 2, 1, 3};
	}
}

AbstractYuyvBufferDecoder::~AbstractYuyvBufferDecoder() { delete[] component_indexes; }

SeparateYuyvBufferDecoder::SeparateYuyvBufferDecoder(CameraFeed* p_camera_feed)
	: AbstractYuyvBufferDecoder(p_camera_feed)
{
	y_image_data.resize(width * height);
	cbcr_image_data.resize(width * height);
	y_image.instantiate();
	cbcr_image.instantiate();
}

void SeparateYuyvBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* y_dst = (uint8_t*)y_image_data.ptrw();
	uint8_t* uv_dst = (uint8_t*)cbcr_image_data.ptrw();
	uint8_t* src = (uint8_t*)p_buffer.start;
	uint8_t* y0_src = src + component_indexes[0];
	uint8_t* y1_src = src + component_indexes[1];
	uint8_t* u_src = src + component_indexes[2];
	uint8_t* v_src = src + component_indexes[3];

	for (int i = 0; i < width * height; i += 2) {
		*y_dst++ = *y0_src;
		*y_dst++ = *y1_src;
		*uv_dst++ = *u_src;
		*uv_dst++ = *v_src;

		y0_src += 4;
		y1_src += 4;
		u_src += 4;
		v_src += 4;
	}

	if (y_image.is_valid()) {
		y_image->set_data(width, height, false, Image::FORMAT_L8, y_image_data);
	}
	else {
		y_image.instantiate(width, height, false, Image::FORMAT_RGB8, y_image_data);
	}
	if (cbcr_image.is_valid()) {
		cbcr_image->set_data(width, height, false, Image::FORMAT_L8, cbcr_image_data);
	}
	else {
		cbcr_image.instantiate(width, height, false, Image::FORMAT_RGB8, cbcr_image_data);
	}

	camera_feed->set_ycbcr_images(y_image, cbcr_image);
}

YuyvToGrayscaleBufferDecoder::YuyvToGrayscaleBufferDecoder(CameraFeed* p_camera_feed)
	: AbstractYuyvBufferDecoder(p_camera_feed)
{
	image_data.resize(width * height);
}

void YuyvToGrayscaleBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* dst = (uint8_t*)image_data.ptrw();
	uint8_t* src = (uint8_t*)p_buffer.start;
	uint8_t* y0_src = src + component_indexes[0];
	uint8_t* y1_src = src + component_indexes[1];

	for (int i = 0; i < width * height; i += 2) {
		*dst++ = *y0_src;
		*dst++ = *y1_src;

		y0_src += 4;
		y1_src += 4;
	}

	if (image.is_valid()) {
		image->set_data(width, height, false, Image::FORMAT_L8, image_data);
	}
	else {
		image.instantiate(width, height, false, Image::FORMAT_RGB8, image_data);
	}

	camera_feed->set_rgb_image(image);
}

YuyvToRgbBufferDecoder::YuyvToRgbBufferDecoder(CameraFeed* p_camera_feed)
	: AbstractYuyvBufferDecoder(p_camera_feed)
{
	image_data.resize(width * height * 3);
}

void YuyvToRgbBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* src = (uint8_t*)p_buffer.start;
	uint8_t* y0_src = src + component_indexes[0];
	uint8_t* y1_src = src + component_indexes[1];
	uint8_t* u_src = src + component_indexes[2];
	uint8_t* v_src = src + component_indexes[3];
	uint8_t* dst = (uint8_t*)image_data.ptrw();

	for (int i = 0; i < width * height; i += 2) {
		int u = *u_src;
		int v = *v_src;
		int u1 = (((u - 128) << 7) + (u - 128)) >> 6;
		int rg = (((u - 128) << 1) + (u - 128) + ((v - 128) << 2) + ((v - 128) << 1)) >> 3;
		int v1 = (((v - 128) << 1) + (v - 128)) >> 1;

		*dst++ = CLAMP(*y0_src + v1, 0, 255);
		*dst++ = CLAMP(*y0_src - rg, 0, 255);
		*dst++ = CLAMP(*y0_src + u1, 0, 255);

		*dst++ = CLAMP(*y1_src + v1, 0, 255);
		*dst++ = CLAMP(*y1_src - rg, 0, 255);
		*dst++ = CLAMP(*y1_src + u1, 0, 255);

		y0_src += 4;
		y1_src += 4;
		u_src += 4;
		v_src += 4;
	}

	if (image.is_valid()) {
		image->set_data(width, height, false, Image::FORMAT_RGB8, image_data);
	}
	else {
		image.instantiate(width, height, false, Image::FORMAT_RGB8, image_data);
	}

	camera_feed->set_rgb_image(image);
}

CopyBufferDecoder::CopyBufferDecoder(CameraFeed* p_camera_feed, bool p_rgba)
	: BufferDecoder(p_camera_feed)
{
	rgba = p_rgba;
	image_data.resize(width * height * (rgba ? 4 : 2));
}

void CopyBufferDecoder::decode(StreamingBuffer p_buffer)
{
	uint8_t* dst = (uint8_t*)image_data.ptrw();
	memcpy(dst, p_buffer.start, p_buffer.length);

	if (image.is_valid()) {
		image->set_data(
			width, height, false, rgba ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8, image_data);
	}
	else {
		image.instantiate(
			width, height, false, rgba ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8, image_data);
	}

	camera_feed->set_rgb_image(image);
}

JpegBufferDecoder::JpegBufferDecoder(CameraFeed* p_camera_feed) : BufferDecoder(p_camera_feed) {}

void JpegBufferDecoder::decode(StreamingBuffer p_buffer)
{
	image_data.resize(p_buffer.length);
	uint8_t* dst = (uint8_t*)image_data.ptrw();
	memcpy(dst, p_buffer.start, p_buffer.length);
	if (image->load_jpg_from_buffer(image_data) == OK) {
		camera_feed->set_rgb_image(image);
	}
}

#include <sys/mman.h>
#include "servers/rendering/renderer.h"

void CameraFeedLinux::update_buffer_thread_func(void* p_func)
{
	if (p_func) {
		CameraFeedLinux* camera_feed_linux = (CameraFeedLinux*)p_func;
		camera_feed_linux->_update_buffer();
	}
}

void CameraFeedLinux::_update_buffer()
{
	while (!exit_flag.is_set()) {
		_read_frame();
		usleep(10000);
	}
}

void CameraFeedLinux::_query_device(const String& p_device_name)
{
	file_descriptor = open(p_device_name.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
	ERR_FAIL_COND_MSG(file_descriptor == -1,
		vformat("Cannot open file descriptor for %s. Error: %d.", p_device_name, errno));

	struct v4l2_capability capability;
	if (ioctl(file_descriptor, VIDIOC_QUERYCAP, &capability) == -1) {
		ERR_FAIL_MSG(vformat("Cannot query device. Error: %d.", errno));
	}
	name = String((char*)capability.card);

	for (int index = 0;; index++) {
		struct v4l2_fmtdesc fmtdesc;
		memset(&fmtdesc, 0, sizeof(fmtdesc));
		fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmtdesc.index = index;

		if (ioctl(file_descriptor, VIDIOC_ENUM_FMT, &fmtdesc) == -1) {
			break;
		}

		for (int res_index = 0;; res_index++) {
			struct v4l2_frmsizeenum frmsizeenum;
			memset(&frmsizeenum, 0, sizeof(frmsizeenum));
			frmsizeenum.pixel_format = fmtdesc.pixelformat;
			frmsizeenum.index = res_index;

			if (ioctl(file_descriptor, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum) == -1) {
				break;
			}

			for (int framerate_index = 0;; framerate_index++) {
				struct v4l2_frmivalenum frmivalenum;
				memset(&frmivalenum, 0, sizeof(frmivalenum));
				frmivalenum.pixel_format = fmtdesc.pixelformat;
				frmivalenum.width = frmsizeenum.discrete.width;
				frmivalenum.height = frmsizeenum.discrete.height;
				frmivalenum.index = framerate_index;

				if (ioctl(file_descriptor, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum) == -1) {
					if (framerate_index == 0) {
						_add_format(fmtdesc, frmsizeenum.discrete, -1, 1);
					}
					break;
				}

				_add_format(fmtdesc, frmsizeenum.discrete, frmivalenum.discrete.numerator,
					frmivalenum.discrete.denominator);
			}
		}
	}

	close(file_descriptor);
}

void CameraFeedLinux::_add_format(v4l2_fmtdesc p_description, v4l2_frmsize_discrete p_size,
	int p_frame_numerator, int p_frame_denominator)
{
	FeedFormat feed_format;
	feed_format.width = p_size.width;
	feed_format.height = p_size.height;
	feed_format.format = String((char*)p_description.description);
	feed_format.frame_numerator = p_frame_numerator;
	feed_format.frame_denominator = p_frame_denominator;
	feed_format.pixel_format = p_description.pixelformat;
	print_verbose(vformat("%s %dx%d@%d/%dfps", (char*)p_description.description, p_size.width,
		p_size.height, p_frame_denominator, p_frame_numerator));
	formats.push_back(feed_format);
}

bool CameraFeedLinux::_request_buffers()
{
	struct v4l2_requestbuffers requestbuffers;

	memset(&requestbuffers, 0, sizeof(requestbuffers));
	requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	requestbuffers.memory = V4L2_MEMORY_MMAP;
	requestbuffers.count = 4;

	if (ioctl(file_descriptor, VIDIOC_REQBUFS, &requestbuffers) == -1) {
		ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_REQBUFS) error: %d.", errno));
	}

	ERR_FAIL_COND_V_MSG(requestbuffers.count < 2, false, "Not enough buffers granted.");

	buffer_count = requestbuffers.count;
	buffers = new StreamingBuffer[buffer_count];

	for (unsigned int i = 0; i < buffer_count; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = requestbuffers.type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(file_descriptor, VIDIOC_QUERYBUF, &buffer) == -1) {
			delete[] buffers;
			ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_QUERYBUF) error: %d.", errno));
		}

		buffers[i].length = buffer.length;
		buffers[i].start = mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
			file_descriptor, buffer.m.offset);

		if (buffers[i].start == MAP_FAILED) {
			_unmap_buffers(i);
			delete[] buffers;
			ERR_FAIL_V_MSG(false, "Mapping buffers failed.");
		}
	}

	return true;
}

bool CameraFeedLinux::_start_capturing()
{
	for (unsigned int i = 0; i < buffer_count; i++) {
		struct v4l2_buffer buffer;

		memset(&buffer, 0, sizeof(buffer));
		buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.index = i;

		if (ioctl(file_descriptor, VIDIOC_QBUF, &buffer) == -1) {
			ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_QBUF) error: %d.", errno));
		}
	}

	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(file_descriptor, VIDIOC_STREAMON, &type) == -1) {
		ERR_FAIL_V_MSG(false, vformat("ioctl(VIDIOC_STREAMON) error: %d.", errno));
	}

	return true;
}

void CameraFeedLinux::_read_frame()
{
	struct v4l2_buffer buffer;
	memset(&buffer, 0, sizeof(buffer));
	buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buffer.memory = V4L2_MEMORY_MMAP;

	if (ioctl(file_descriptor, VIDIOC_DQBUF, &buffer) == -1) {
		if (errno != EAGAIN) {
			print_error(vformat("ioctl(VIDIOC_DQBUF) error: %d.", errno));
			exit_flag.set();
		}
		return;
	}

	buffer_decoder->decode(buffers[buffer.index]);

	if (ioctl(file_descriptor, VIDIOC_QBUF, &buffer) == -1) {
		print_error(vformat("ioctl(VIDIOC_QBUF) error: %d.", errno));
	}
}

void CameraFeedLinux::_stop_capturing()
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (ioctl(file_descriptor, VIDIOC_STREAMOFF, &type) == -1) {
		print_error(vformat("ioctl(VIDIOC_STREAMOFF) error: %d.", errno));
	}
}

void CameraFeedLinux::_unmap_buffers(unsigned int p_count)
{
	for (unsigned int i = 0; i < p_count; i++) {
		munmap(buffers[i].start, buffers[i].length);
	}
}

void CameraFeedLinux::_start_thread()
{
	exit_flag.clear();
	thread = memnew(Thread);
	thread->start(CameraFeedLinux::update_buffer_thread_func, this);
}

String CameraFeedLinux::get_device_name() const { return device_name; }

bool CameraFeedLinux::activate_feed()
{
	ERR_FAIL_COND_V_MSG(
		selected_format == -1, false, "CameraFeed format needs to be set before activating.");
	file_descriptor = open(device_name.ascii().get_data(), O_RDWR | O_NONBLOCK, 0);
	if (_request_buffers() && _start_capturing()) {
		buffer_decoder = _create_buffer_decoder();
		_start_thread();
		return true;
	}
	ERR_FAIL_V_MSG(false, "Could not activate feed.");
}

CameraFeed::FeedFormat CameraFeedLinux::get_format() const
{
	FeedFormat feed_format = {};
	return selected_format == -1 ? feed_format : formats[selected_format];
}

CameraFeedLinux::CameraFeedLinux(const String& p_device_name)
{
	device_name = p_device_name;
	_query_device(device_name);
}

CameraFeedLinux::~CameraFeedLinux()
{
	if (is_active()) {
		deactivate_feed();
	}
}

void CameraFeedLinux::deactivate_feed() {}

BufferDecoder* CameraFeedLinux::_create_buffer_decoder()
{
	BufferDecoder bd = BufferDecoder();
	return &bd;
}
#endif // defined(LINUXBSD_ENABLED)


// --- Merged from modules/camera/camera_win.cpp ---
#if defined(WINDOWS_ENABLED)


#include "servers/camera/camera_feed.h"

///@TODO sorry guys, I got about 80% through implementing this using DirectShow only
// to find out Microsoft deprecated half the API and its replacement is as confusing
// as they could make it. Joey suggested looking into libuvc which offers a more direct
// route to webcams over USB and this is very promising but it wouldn't compile on
// windows for me...I've gutted the classes I implemented DirectShow in just to have
// a skeleton for someone to work on, mail me for more details or if you want a copy....

//////////////////////////////////////////////////////////////////////////
// CameraFeedWindows - Subclass for our camera feed on windows

/// @TODO need to implement this

class CameraFeedWindows : public CameraFeed {
private:
protected:
public:
	CameraFeedWindows();
	virtual ~CameraFeedWindows();

	bool activate_feed();
	void deactivate_feed();
};

CameraFeedWindows::CameraFeedWindows() {
	///@TODO implement this, should store information about our available camera
}

CameraFeedWindows::~CameraFeedWindows() {
	// make sure we stop recording if we are!
	if (is_active()) {
		deactivate_feed();
	};

	///@TODO free up anything used by this
}

bool CameraFeedWindows::activate_feed() {
	///@TODO this should activate our camera and start the process of capturing frames

	return true;
}

///@TODO we should probably have a callback method here that is being called by the
// camera API which provides frames and call back into the CameraServer to update our texture

void CameraFeedWindows::deactivate_feed() {
	///@TODO this should deactivate our camera and stop the process of capturing frames
}

//////////////////////////////////////////////////////////////////////////
// CameraWindows - Subclass for our camera server on windows

void CameraWindows::add_active_cameras() {
	///@TODO scan through any active cameras and create CameraFeedWindows objects for them
}

void CameraWindows::initialize() {
	// Find cameras active right now
	add_active_cameras();

	// need to add something that will react to devices being connected/removed...
}
#endif // defined(WINDOWS_ENABLED)


// --- Merged from modules/camera/camera_android.cpp ---
#if defined(ANDROID_ENABLED)


#include "core/os/os.h"

#include "platform/android/display_server_android.h"
#include "platform/android/java_godot_io_wrapper.h"
#include "platform/android/os_android.h"

// Scope guard to ensure AImage instances are always deleted.
class ScopedAImage {
	AImage *image = nullptr;

public:
	ScopedAImage() = default;
	~ScopedAImage() {
		reset();
	}

	ScopedAImage(const ScopedAImage &) = delete;
	ScopedAImage &operator=(const ScopedAImage &) = delete;

	AImage **out() {
		return &image;
	}

	AImage *get() const {
		return image;
	}

	void reset(AImage *p_image = nullptr) {
		if (image != nullptr) {
			AImage_delete(image);
		}
		image = p_image;
	}
};

//////////////////////////////////////////////////////////////////////////
// Helper functions
//
// The following code enables you to view the contents of a media type while
// debugging.

#ifndef IF_EQUAL_RETURN
#define MAKE_FORMAT_CONST(suffix) AIMAGE_FORMAT_##suffix
#define IF_EQUAL_RETURN(param, val) \
	if (MAKE_FORMAT_CONST(val) == param) \
	return #val
#endif

String GetFormatName(const int32_t &format) {
	IF_EQUAL_RETURN(format, YUV_420_888);
	IF_EQUAL_RETURN(format, RGB_888);
	IF_EQUAL_RETURN(format, RGBA_8888);

	return "Unsupported";
}

//////////////////////////////////////////////////////////////////////////
// CameraFeedAndroid - Subclass for our camera feed on Android

CameraFeedAndroid::CameraFeedAndroid(ACameraManager *manager, ACameraMetadata *metadata, const char *id,
		CameraFeed::FeedPosition position, int32_t orientation) {
	this->manager = manager;
	this->metadata = metadata;
	this->orientation = orientation;
	_add_formats();
	camera_id = id;
	set_position(position);

	// Position
	switch (position) {
		case CameraFeed::FEED_BACK:
			name = vformat("%s | BACK", id);
			break;
		case CameraFeed::FEED_FRONT:
			name = vformat("%s | FRONT", id);
			break;
		default:
			name = vformat("%s", id);
			break;
	}

	image_y.instantiate();
	image_uv.instantiate();
}

CameraFeedAndroid::~CameraFeedAndroid() {
	if (is_active()) {
		deactivate_feed();
	}
	if (metadata != nullptr) {
		ACameraMetadata_free(metadata);
	}
}

void CameraFeedAndroid::refresh_camera_metadata() {
	ERR_FAIL_NULL_MSG(manager, vformat("Camera %s: Cannot refresh metadata, manager is null.", camera_id));

	if (metadata != nullptr) {
		ACameraMetadata_free(metadata);
		metadata = nullptr;
	}

	camera_status_t status = ACameraManager_getCameraCharacteristics(manager, camera_id.utf8().get_data(), &metadata);
	if (status != ACAMERA_OK || metadata == nullptr) {
		ERR_FAIL_MSG(vformat("Camera %s: Failed to refresh metadata (status: %d).", camera_id, status));
	}

	ACameraMetadata_const_entry orientation_entry;
	status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &orientation_entry);
	if (status == ACAMERA_OK) {
		orientation = orientation_entry.data.i32[0];
		print_verbose(vformat("Camera %s: Orientation updated to %d.", camera_id, orientation));
	} else {
		ERR_PRINT(vformat("Camera %s: Failed to get sensor orientation after refresh (status: %d).", camera_id, status));
	}

	formats.clear();
	_add_formats();

	print_verbose(vformat("Camera %s: Metadata refreshed successfully.", camera_id));
}

void CameraFeedAndroid::_set_rotation() {
	if (!metadata) {
		print_verbose(vformat("Camera %s: Metadata is null in _set_rotation, attempting refresh.", camera_id));
		refresh_camera_metadata();
	}

	float image_rotation = 0.0f;
	std::optional<int> result;

	if (metadata) {
		CameraRotationParams params;
		params.sensor_orientation = orientation;
		params.camera_facing = (position == CameraFeed::FEED_FRONT) ? CameraFacing::FRONT : CameraFacing::BACK;
		params.display_rotation = get_app_orientation();

		result = calculate_rotation(params);
	} else {
		ERR_PRINT(vformat("Camera %s: Cannot update rotation, metadata unavailable after refresh, using fallback.", camera_id));
	}

	if (result.has_value()) {
		image_rotation = static_cast<float>(result.value());
	} else {
		int display_rotation = DisplayServer::driver_as<DisplayServerAndroid>()->get_display_rotation();
		switch (display_rotation) {
			case 90:
				display_rotation = 270;
				break;
			case 270:
				display_rotation = 90;
				break;
			default:
				break;
		}

		int sign = position == CameraFeed::FEED_FRONT ? 1 : -1;
		image_rotation = (orientation - display_rotation * sign + 360) % 360;
	}

	transform = Transform2D();
	transform = transform.rotated(Math::deg_to_rad(image_rotation));
}

void CameraFeedAndroid::_add_formats() {
	// Get supported formats
	ACameraMetadata_const_entry formats;
	camera_status_t status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &formats);

	if (status == ACAMERA_OK) {
		for (uint32_t f = 0; f < formats.count; f += 4) {
			// Only support output streams
			int32_t input = formats.data.i32[f + 3];
			if (input) {
				continue;
			}

			// Get format and resolution
			int32_t format = formats.data.i32[f + 0];
			if (format == AIMAGE_FORMAT_YUV_420_888 ||
					format == AIMAGE_FORMAT_RGBA_8888 ||
					format == AIMAGE_FORMAT_RGB_888) {
				CameraFeed::FeedFormat feed_format;
				feed_format.width = formats.data.i32[f + 1];
				feed_format.height = formats.data.i32[f + 2];
				feed_format.format = GetFormatName(format);
				feed_format.pixel_format = format;
				this->formats.append(feed_format);
			}
		}
	}
}

bool CameraFeedAndroid::activate_feed() {
	ERR_FAIL_COND_V_MSG(formats.is_empty(), false, "No camera formats available.");
	ERR_FAIL_INDEX_V_MSG(selected_format, formats.size(), false,
			vformat("CameraFeed format needs to be set before activating. Selected format index: %d (formats size: %d)", selected_format, formats.size()));
	if (is_active()) {
		deactivate_feed();
	};

	// Clear deactivation and error flags before starting activation.
	is_deactivating.clear();
	device_error_occurred.clear();

	// Request permission
	if (!OS::request_permission("CAMERA")) {
		return false;
	}

	// Open device
	device_callbacks = {
		.context = this,
		.onDisconnected = onDisconnected,
		.onError = onError,
	};
	camera_status_t c_status = ACameraManager_openCamera(manager, camera_id.utf8().get_data(), &device_callbacks, &device);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to open camera (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Create image reader
	const FeedFormat &feed_format = formats[selected_format];
	media_status_t m_status = AImageReader_new(feed_format.width, feed_format.height, feed_format.pixel_format, 1, &reader);
	if (m_status != AMEDIA_OK) {
		print_error(vformat("Camera %s: Failed to create image reader (status: %d)", camera_id, m_status));
		deactivate_feed();
		return false;
	}

	// Get image listener
	image_listener = {
		.context = this,
		.onImageAvailable = onImage,
	};
	m_status = AImageReader_setImageListener(reader, &image_listener);
	if (m_status != AMEDIA_OK) {
		print_error(vformat("Camera %s: Failed to set image listener (status: %d)", camera_id, m_status));
		deactivate_feed();
		return false;
	}

	// Get image surface
	ANativeWindow *surface;
	m_status = AImageReader_getWindow(reader, &surface);
	if (m_status != AMEDIA_OK) {
		print_error(vformat("Camera %s: Failed to get image surface (status: %d)", camera_id, m_status));
		deactivate_feed();
		return false;
	}

	// Prepare session outputs
	c_status = ACaptureSessionOutput_create(surface, &session_output);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create session output (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	c_status = ACaptureSessionOutputContainer_create(&session_output_container);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create session output container (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	c_status = ACaptureSessionOutputContainer_add(session_output_container, session_output);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to add session output to container (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Create capture session
	session_callbacks = {
		.context = this,
		.onClosed = onSessionClosed,
		.onReady = onSessionReady,
		.onActive = onSessionActive
	};
	c_status = ACameraDevice_createCaptureSession(device, session_output_container, &session_callbacks, &session);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create capture session (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Create capture request
	c_status = ACameraDevice_createCaptureRequest(device, TEMPLATE_PREVIEW, &request);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create capture request (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Set capture target
	c_status = ACameraOutputTarget_create(surface, &camera_output_target);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to create camera output target (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	c_status = ACaptureRequest_addTarget(request, camera_output_target);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to add target to capture request (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	// Start capture
	c_status = ACameraCaptureSession_setRepeatingRequest(session, nullptr, 1, &request, nullptr);
	if (c_status != ACAMERA_OK) {
		print_error(vformat("Camera %s: Failed to start repeating request (status: %d)", camera_id, c_status));
		deactivate_feed();
		return false;
	}

	return true;
}

bool CameraFeedAndroid::set_format(int p_index, const Dictionary &p_parameters) {
	ERR_FAIL_COND_V_MSG(active, false, "Feed is active.");
	ERR_FAIL_INDEX_V_MSG(p_index, formats.size(), false, "Invalid format index.");

	selected_format = p_index;

	// Reset base dimensions to force texture recreation on next frame.
	// This ensures proper handling when switching between different resolutions.
	base_width = 0;
	base_height = 0;

	return true;
}

Array CameraFeedAndroid::get_formats() const {
	Array result;
	for (const FeedFormat &feed_format : formats) {
		Dictionary dictionary;
		dictionary["width"] = feed_format.width;
		dictionary["height"] = feed_format.height;
		dictionary["format"] = feed_format.format;
		result.push_back(dictionary);
	}
	return result;
}

CameraFeed::FeedFormat CameraFeedAndroid::get_format() const {
	CameraFeed::FeedFormat feed_format = {};
	ERR_FAIL_INDEX_V_MSG(selected_format, formats.size(), feed_format,
			vformat("Invalid format index: %d (formats size: %d)", selected_format, formats.size()));
	return formats[selected_format];
}

void CameraFeedAndroid::handle_pause() {
	if (is_active()) {
		was_active_before_pause = true;
		print_verbose(vformat("Camera %s: Pausing (was active).", camera_id));
		deactivate_feed();
	} else {
		was_active_before_pause = false;
	}
}

void CameraFeedAndroid::handle_resume() {
	if (was_active_before_pause) {
		print_verbose(vformat("Camera %s: Resuming.", camera_id));
		activate_feed();
		was_active_before_pause = false;
	}
}

void CameraFeedAndroid::handle_rotation_change() {
	if (!is_active()) {
		return;
	}

	print_verbose(vformat("Camera %s: Handling rotation change.", camera_id));
	refresh_camera_metadata();
	_set_rotation();
}

// In-place stride compaction (handles row padding).
void CameraFeedAndroid::compact_stride_inplace(uint8_t *data, size_t width, int height, size_t stride) {
	if (stride <= width) {
		return;
	}

	uint8_t *src_row = data + stride;
	uint8_t *dst_row = data + width;

	for (int y = 1; y < height; y++) {
		memmove(dst_row, src_row, width);
		src_row += stride;
		dst_row += width;
	}
}

void CameraFeedAndroid::onImage(void *context, AImageReader *p_reader) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);

	// Check deactivation flag before acquiring mutex to avoid using resources
	// that may be deleted during deactivate_feed(). This is a racy check but
	// safe because we only transition is_deactivating from false->true during
	// deactivation, and back to false only at the start of activation.
	if (feed->is_deactivating.is_set()) {
		// Don't try to acquire images - the reader may be deleted.
		// Android will clean up pending images when the reader is deleted.
		return;
	}

	MutexLock lock(feed->callback_mutex);

	// Re-check after acquiring mutex in case deactivation started while waiting.
	if (feed->is_deactivating.is_set()) {
		return;
	}

	// If feed is not active, we must still acquire and discard the image to
	// free the buffer slot. Otherwise, with maxImages=1, the buffer stays full
	// and no new frames will arrive (onImage won't be called again).
	// This can happen when a frame arrives between activate_feed() returning
	// and active=true being set in the base class.
	if (!feed->is_active()) {
		AImage *pending_image = nullptr;
		if (AImageReader_acquireNextImage(p_reader, &pending_image) == AMEDIA_OK && pending_image != nullptr) {
			AImage_delete(pending_image);
		}
		return;
	}

	Vector<uint8_t> data_y;
	Vector<uint8_t> data_uv;

	// Get image
	ScopedAImage image_guard;
	media_status_t status = AImageReader_acquireNextImage(p_reader, image_guard.out());
	ERR_FAIL_COND(status != AMEDIA_OK);
	AImage *image = image_guard.get();

	// Get image data
	uint8_t *data = nullptr;
	int len = 0;
	int32_t pixel_stride, row_stride;
	FeedFormat format = feed->get_format();
	int width = format.width;
	int height = format.height;

	switch (format.pixel_format) {
		case AIMAGE_FORMAT_YUV_420_888: {
			int32_t y_row_stride;
			AImage_getPlaneRowStride(image, 0, &y_row_stride);
			AImage_getPlaneData(image, 0, &data, &len);

			if (len <= 0) {
				return;
			}

			int64_t y_size = Image::get_image_data_size(width, height, Image::FORMAT_R8, false);

			if (y_row_stride == width && len == y_size) {
				data_y.resize(y_size);
				memcpy(data_y.ptrw(), data, y_size);
			} else {
				// Validate buffer size before compaction to prevent out-of-bounds read.
				int64_t required_y_len = (int64_t)y_row_stride * (height - 1) + width;
				if (len < required_y_len) {
					return;
				}
				if (feed->scratch_y.size() < len) {
					feed->scratch_y.resize(len);
				}
				memcpy(feed->scratch_y.ptrw(), data, len);
				CameraFeedAndroid::compact_stride_inplace(feed->scratch_y.ptrw(), width, height, y_row_stride);
				data_y.resize(y_size);
				memcpy(data_y.ptrw(), feed->scratch_y.ptr(), y_size);
			}

			AImage_getPlanePixelStride(image, 1, &pixel_stride);
			AImage_getPlaneRowStride(image, 1, &row_stride);
			AImage_getPlaneData(image, 1, &data, &len);

			if (len <= 0) {
				return;
			}

			int64_t uv_size = Image::get_image_data_size(width / 2, height / 2, Image::FORMAT_RG8, false);

			int uv_width = width / 2;
			int uv_height = height / 2;

			uint8_t *data_v = nullptr;
			int32_t v_pixel_stride = 0;
			int32_t v_row_stride = 0;
			int len_v = 0;

			if (pixel_stride != 2) {
				AImage_getPlanePixelStride(image, 2, &v_pixel_stride);
				AImage_getPlaneRowStride(image, 2, &v_row_stride);
				AImage_getPlaneData(image, 2, &data_v, &len_v);
				if (len_v <= 0) {
					return;
				}
			}

			if (pixel_stride == 2 && row_stride == uv_width * 2 && len == uv_size) {
				data_uv.resize(uv_size);
				memcpy(data_uv.ptrw(), data, uv_size);
			} else if (pixel_stride == 2) {
				// Allow 1-2 byte tolerance for UV buffer (some devices omit final bytes).
				int64_t required_uv_len = (int64_t)row_stride * (uv_height - 1) + uv_width * 2;
				const int64_t UV_TOLERANCE = 2;
				if (len < required_uv_len - UV_TOLERANCE) {
					return;
				}

				if (feed->scratch_uv.size() < required_uv_len) {
					feed->scratch_uv.resize(required_uv_len);
				}
				if (len < required_uv_len) {
					memset(feed->scratch_uv.ptrw() + len, 128, required_uv_len - len);
				}
				memcpy(feed->scratch_uv.ptrw(), data, len);
				if (row_stride != uv_width * 2) {
					CameraFeedAndroid::compact_stride_inplace(feed->scratch_uv.ptrw(), uv_width * 2, uv_height, row_stride);
				}
				data_uv.resize(uv_size);
				memcpy(data_uv.ptrw(), feed->scratch_uv.ptr(), uv_size);
			} else {
				if (data_v && len_v > 0) {
					data_uv.resize(uv_size);
					uint8_t *dst = data_uv.ptrw();
					uint8_t *src_u = data;
					uint8_t *src_v = data_v;
					for (int row = 0; row < uv_height; row++) {
						for (int col = 0; col < uv_width; col++) {
							dst[col * 2] = src_u[col * pixel_stride];
							dst[col * 2 + 1] = src_v[col * v_pixel_stride];
						}
						dst += uv_width * 2;
						src_u += row_stride;
						src_v += v_row_stride;
					}
				} else {
					data_uv.resize(uv_size);
					memset(data_uv.ptrw(), 128, uv_size);
				}
			}

			// Defer to main thread to avoid race conditions with Renderer.
			feed->image_y.instantiate();
			feed->image_y->initialize_data(width, height, false, Image::FORMAT_R8, data_y);

			feed->image_uv.instantiate();
			feed->image_uv->initialize_data(width / 2, height / 2, false, Image::FORMAT_RG8, data_uv);

			feed->call_deferred("set_ycbcr_images", feed->image_y, feed->image_uv);
			break;
		}
		case AIMAGE_FORMAT_RGBA_8888: {
			AImage_getPlaneData(image, 0, &data, &len);
			if (len <= 0) {
				return;
			}
			int64_t size = Image::get_image_data_size(width, height, Image::FORMAT_RGBA8, false);
			data_y.resize(len > size ? len : size);
			memcpy(data_y.ptrw(), data, len);

			feed->image_y.instantiate();
			feed->image_y->initialize_data(width, height, false, Image::FORMAT_RGBA8, data_y);

			feed->call_deferred("set_rgb_image", feed->image_y);
			break;
		}
		case AIMAGE_FORMAT_RGB_888: {
			AImage_getPlaneData(image, 0, &data, &len);
			if (len <= 0) {
				return;
			}
			int64_t size = Image::get_image_data_size(width, height, Image::FORMAT_RGB8, false);
			data_y.resize(len > size ? len : size);
			memcpy(data_y.ptrw(), data, len);

			feed->image_y.instantiate();
			feed->image_y->initialize_data(width, height, false, Image::FORMAT_RGB8, data_y);

			feed->call_deferred("set_rgb_image", feed->image_y);
			break;
		}
		default:
			return;
	}

	if (!feed->formats.is_empty()) {
		if (feed->metadata != nullptr) {
			feed->_set_rotation();
		} else {
			print_verbose(vformat("Camera %s: Metadata invalidated in onImage, attempting refresh.", feed->camera_id));
			feed->refresh_camera_metadata();
			if (feed->metadata != nullptr && !feed->formats.is_empty()) {
				feed->_set_rotation();
			}
		}
	}

	// Release image happens automatically via ScopedAImage.
}

void CameraFeedAndroid::onSessionReady(void *context, ACameraCaptureSession *session) {
	print_verbose("Capture session ready");
}

void CameraFeedAndroid::onSessionActive(void *context, ACameraCaptureSession *session) {
	print_verbose("Capture session active");
}

void CameraFeedAndroid::onSessionClosed(void *context, ACameraCaptureSession *p_session) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);
	// Only post if deactivate_feed() is waiting for us. This prevents
	// spurious posts from error-triggered session closes that could
	// desynchronize the semaphore count.
	if (feed->session_close_pending.is_set()) {
		feed->session_closed_semaphore.post();
	}
}

void CameraFeedAndroid::deactivate_feed() {
	// Signal that deactivation is in progress. This prevents onImage callbacks
	// from using resources that may be deleted during cleanup.
	is_deactivating.set();

	// First, remove image listener to prevent new callbacks.
	if (reader != nullptr) {
		AImageReader_setImageListener(reader, nullptr);
	}

	// Stop and close capture session.
	// Must wait for session to fully close before closing device.
	if (session != nullptr) {
		ACameraCaptureSession_stopRepeating(session);

		// If an error occurred, the session may have already been closed by
		// Android. In that case, ACameraCaptureSession_close() may not trigger
		// onSessionClosed, so we skip waiting to avoid a deadlock.
		bool skip_wait = device_error_occurred.is_set();

		if (!skip_wait) {
			// Set flag before closing to indicate we're waiting for the callback.
			// This ensures we only wait for the post from THIS close operation.
			session_close_pending.set();
		}

		ACameraCaptureSession_close(session);

		if (!skip_wait) {
			// Wait for onSessionClosed callback to ensure session is fully closed
			// before proceeding to close the device.
			session_closed_semaphore.wait();
			session_close_pending.clear();
		}

		session = nullptr;
	}

	// Now safe to acquire lock and clean up resources.
	// No new callbacks will be triggered after this point.
	MutexLock lock(callback_mutex);

	if (device != nullptr) {
		ACameraDevice_close(device);
		device = nullptr;
	}

	if (reader != nullptr) {
		AImageReader_delete(reader);
		reader = nullptr;
	}

	if (request != nullptr) {
		ACaptureRequest_free(request);
		request = nullptr;
	}

	if (camera_output_target != nullptr) {
		ACameraOutputTarget_free(camera_output_target);
		camera_output_target = nullptr;
	}

	if (session_output_container != nullptr) {
		ACaptureSessionOutputContainer_free(session_output_container);
		session_output_container = nullptr;
	}

	if (session_output != nullptr) {
		ACaptureSessionOutput_free(session_output);
		session_output = nullptr;
	}
}

void CameraFeedAndroid::onError(void *context, ACameraDevice *p_device, int error) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);
	print_error(vformat("Camera %s error: %d", feed->camera_id, error));
	// Mark that an error occurred. This signals to deactivate_feed() that
	// the session may have been closed by Android, so we shouldn't wait
	// for onSessionClosed.
	feed->device_error_occurred.set();
	onDisconnected(context, p_device);
}

void CameraFeedAndroid::onDisconnected(void *context, ACameraDevice *p_device) {
	CameraFeedAndroid *feed = static_cast<CameraFeedAndroid *>(context);
	// Defer to main thread to avoid reentrancy issues when called from
	// ACameraDevice_close() during deactivate_feed().
	feed->call_deferred("set_active", false);
}

//////////////////////////////////////////////////////////////////////////
// CameraAndroid - Subclass for our camera server on Android

void CameraAndroid::initialize() {
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void CameraAndroid::finalize() {
	if (!data) {
		return;
	}
	remove_all_feeds();
	memdelete(data);
	data = nullptr;
}

void CameraAndroid::update_feeds() {
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(CameraServer::data);

	ACameraIdList *cameraIds = nullptr;
	camera_status_t c_status = ACameraManager_getCameraIdList(data->camera_manager, &cameraIds);
	ERR_FAIL_COND(c_status != ACAMERA_OK);

	// Deactivate all feeds before removing to ensure proper cleanup.
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid() && feed->is_active()) {
			feed->deactivate_feed();
		}
	}

	// remove existing devices
	for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
		CameraServer::remove_feed(CameraServer::data->feeds[i]);
	}

	for (int c = 0; c < cameraIds->numCameras; ++c) {
		const char *id = cameraIds->cameraIds[c];
		ACameraMetadata *metadata = nullptr;
		ACameraManager_getCameraCharacteristics(data->camera_manager, id, &metadata);
		if (!metadata) {
			continue;
		}

		// Get sensor orientation
		ACameraMetadata_const_entry orientation;
		c_status = ACameraMetadata_getConstEntry(metadata, ACAMERA_SENSOR_ORIENTATION, &orientation);
		int32_t cameraOrientation;
		if (c_status == ACAMERA_OK) {
			cameraOrientation = orientation.data.i32[0];
		} else {
			cameraOrientation = 0;
			print_error(vformat("Unable to get sensor orientation: %s", id));
		}

		// Get position
		ACameraMetadata_const_entry lensInfo;
		CameraFeed::FeedPosition position = CameraFeed::FEED_UNSPECIFIED;
		camera_status_t status;
		status = ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &lensInfo);
		if (status != ACAMERA_OK) {
			ACameraMetadata_free(metadata);
			continue;
		}
		uint8_t lens_facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(lensInfo.data.u8[0]);
		if (lens_facing == ACAMERA_LENS_FACING_FRONT) {
			position = CameraFeed::FEED_FRONT;
		} else if (lens_facing == ACAMERA_LENS_FACING_BACK) {
			position = CameraFeed::FEED_BACK;
		} else {
			ACameraMetadata_free(metadata);
			continue;
		}

		Ref<CameraFeedAndroid> feed = memnew(CameraFeedAndroid(data->camera_manager, metadata, id, position, cameraOrientation));
		CameraServer::add_feed(feed);
	}

	ACameraManager_deleteCameraIdList(cameraIds);
}

void CameraAndroid::remove_all_feeds() {
	ERR_FAIL_NULL(data);
	ERR_FAIL_NULL(CameraServer::data);

	// Deactivate all feeds before removing to ensure proper cleanup.
	// This prevents "Device is closed but session is not notified" warnings
	// that can occur if feeds are destroyed while still active.
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid() && feed->is_active()) {
			feed->deactivate_feed();
		}
	}

	// remove existing devices
	for (int i = CameraServer::data->feeds.size() - 1; i >= 0; i--) {
		CameraServer::remove_feed(CameraServer::data->feeds[i]);
	}

	if (data->camera_manager != nullptr) {
		ACameraManager_delete(data->camera_manager);
		data->camera_manager = nullptr;
	}
}

void CameraAndroid::set_monitoring_feeds(bool p_monitoring_feeds) {
	ERR_FAIL_NULL(data);
	if (p_monitoring_feeds == CameraServer::is_monitoring_feeds()) {
		return;
	}

	CameraServer::set_monitoring_feeds_state(p_monitoring_feeds);
	if (p_monitoring_feeds) {
		if (data->camera_manager == nullptr) {
			data->camera_manager = ACameraManager_create();
		}

		// Update feeds
		update_feeds();
	} else {
		remove_all_feeds();
	}
}

void CameraAndroid::handle_application_pause() {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_pause();
		}
	}
}

void CameraAndroid::handle_application_resume() {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_resume();
		}
	}
}

void CameraAndroid::handle_display_rotation_change(int) {
	ERR_FAIL_NULL(CameraServer::data);
	for (int i = 0; i < CameraServer::data->feeds.size(); i++) {
		Ref<CameraFeedAndroid> feed = CameraServer::data->feeds[i];
		if (feed.is_valid()) {
			feed->handle_rotation_change();
		}
	}
}

std::optional<int> CameraFeedAndroid::calculate_rotation(const CameraRotationParams &p_params) {
	if (p_params.sensor_orientation < 0 || p_params.sensor_orientation > 270 || p_params.sensor_orientation % 90 != 0) {
		return std::nullopt;
	}

	int rotation_angle = p_params.sensor_orientation - p_params.display_rotation;
	return normalize_angle(rotation_angle);
}

int CameraFeedAndroid::normalize_angle(int p_angle) {
	while (p_angle < 0) {
		p_angle += 360;
	}
	return p_angle % 360;
}

int CameraFeedAndroid::get_display_rotation() {
	return DisplayServer::driver_as<DisplayServerAndroid>()->get_display_rotation();
}

int CameraFeedAndroid::get_app_orientation() {
	GodotIOJavaWrapper *godot_io_java = OS_Android::get_godot_io_java();
	ERR_FAIL_NULL_V(godot_io_java, 0);

	int orientation = godot_io_java->get_screen_orientation();
	switch (orientation) {
		case 0: // SCREEN_LANDSCAPE
			return 90;
		case 1: // SCREEN_PORTRAIT
			return 0;
		case 2: // SCREEN_REVERSE_LANDSCAPE
			return 270;
		case 3: // SCREEN_REVERSE_PORTRAIT
			return 180;
		case 4: // SCREEN_SENSOR_LANDSCAPE
		case 5: // SCREEN_SENSOR_PORTRAIT
		case 6: // SCREEN_SENSOR
		default:
			return get_display_rotation();
	}
}
#endif // defined(ANDROID_ENABLED)
