/**************************************************************************/
/*  camera_server.cpp                                                     */
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

#include "camera_server.h"
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

#include "camera_feed.h"
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
