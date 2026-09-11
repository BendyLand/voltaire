/**************************************************************************/
/*  camera_feed.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "camera_feed.h"
#include "servers/rendering/rendering_server.h"

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
	return RenderingServer::get_singleton()->texture_get_native_handle(texture[p_which]);
}

CameraFeed::CameraFeed()
{
	// initialize our feed
	id = CameraServer::get_singleton()->get_free_id();
	base_width = 0;
	base_height = 0;
	name = "???";
	active = false;
	datatype = CameraFeed::FEED_RGB;
	position = CameraFeed::FEED_UNSPECIFIED;
	transform = Transform2D(1.0, 0.0, 0.0, -1.0, 0.0, 1.0);
	texture[CameraServer::FEED_Y_IMAGE] =
		RenderingServer::get_singleton()->texture_2d_placeholder_create();
	texture[CameraServer::FEED_CBCR_IMAGE] =
		RenderingServer::get_singleton()->texture_2d_placeholder_create();
}

CameraFeed::CameraFeed(String p_name, FeedPosition p_position)
{
	// initialize our feed
	id = CameraServer::get_singleton()->get_free_id();
	base_width = 0;
	base_height = 0;
	name = p_name;
	active = false;
	datatype = CameraFeed::FEED_NOIMAGE;
	position = p_position;
	transform = Transform2D(1.0, 0.0, 0.0, -1.0, 0.0, 1.0);
	texture[CameraServer::FEED_Y_IMAGE] =
		RenderingServer::get_singleton()->texture_2d_placeholder_create();
	texture[CameraServer::FEED_CBCR_IMAGE] =
		RenderingServer::get_singleton()->texture_2d_placeholder_create();
}

CameraFeed::~CameraFeed()
{
	// Free our textures
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RenderingServer::get_singleton()->free_rid(texture[CameraServer::FEED_Y_IMAGE]);
	RenderingServer::get_singleton()->free_rid(texture[CameraServer::FEED_CBCR_IMAGE]);
}

CameraFeed::FeedFormat CameraFeed::get_format() const
{
	FeedFormat feed_format = {};
	return feed_format;
}

bool CameraFeed::activate_feed() { return false; }

void CameraFeed::deactivate_feed() {}


