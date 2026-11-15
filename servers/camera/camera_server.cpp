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


