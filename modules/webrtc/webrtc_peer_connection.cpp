/**************************************************************************/
/*  webrtc_peer_connection.cpp                                            */
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

#include "webrtc_peer_connection.h"

#ifdef WEB_ENABLED
#include "webrtc_peer_connection_js.h"
#endif

#include "core/object/class_db.h"

StringName WebRTCPeerConnection::default_extension;

void WebRTCPeerConnection::set_default_extension(const StringName& p_extension)
{
	default_extension = StringName(p_extension, true);
}

WebRTCPeerConnection* WebRTCPeerConnection::create(bool p_notify_postinitialize)
{
#ifdef WEB_ENABLED
	return static_cast<WebRTCPeerConnection*>(
		ClassDB::creator<WebRTCPeerConnectionJS>(p_notify_postinitialize));
#else
	if (default_extension == StringName()) {
		WARN_PRINT_ONCE("No default WebRTC extension configured.");
	}
	return memnew(WebRTCPeerConnection).ptr();
#endif
}

void WebRTCPeerConnection::_bind_methods() {}

WebRTCPeerConnection::ConnectionState WebRTCPeerConnection::get_connection_state() const
{
	return STATE_DISCONNECTED;
}

WebRTCPeerConnection::GatheringState WebRTCPeerConnection::get_gathering_state() const
{
	return GATHERING_STATE_NEW;
}

WebRTCPeerConnection::SignalingState WebRTCPeerConnection::get_signaling_state() const
{
	return SIGNALING_STATE_STABLE;
}

Error WebRTCPeerConnection::initialize(const Dictionary& p_config) { return OK; }

Ref<WebRTCDataChannel> WebRTCPeerConnection::create_data_channel(
	const String& p_label, const Dictionary& p_options)
{
	return nullptr;
}

Error WebRTCPeerConnection::create_offer() { return OK; }

Error WebRTCPeerConnection::set_remote_description(const String& p_type, const String& p_sdp)
{
	return OK;
}

Error WebRTCPeerConnection::set_local_description(const String& p_type, const String& p_sdp)
{
	return OK;
}

Error WebRTCPeerConnection::add_ice_candidate(
	const String& p_sdp_mid, int p_sdp_mline_index_name, const String& p_sdp_name)
{
	return OK;
}

Error WebRTCPeerConnection::poll() { return OK; }

WebRTCPeerConnection::WebRTCPeerConnection() {}

WebRTCPeerConnection::~WebRTCPeerConnection() {}

void WebRTCPeerConnection::close() {}


