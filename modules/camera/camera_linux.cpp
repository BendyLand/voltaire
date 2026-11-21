/**************************************************************************/
/*  camera_linux.cpp                                                      */
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

#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "camera_linux.h"

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
