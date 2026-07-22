/**************************************************************************/
/*  webrtc_data_channel_extension.cpp                                     */
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

#include "webrtc_data_channel_extension.h"

#include "core/object/class_db.h" // IWYU pragma: keep. `VLTRVIRTUAL_BIND` macro.

void WebRTCDataChannelExtension::_bind_methods() {
	ADD_PROPERTY_DEFAULT("write_mode", WRITE_MODE_BINARY);

	VLTRVIRTUAL_BIND(_get_packet, "r_buffer", "r_buffer_size");
	VLTRVIRTUAL_BIND(_put_packet, "buffer", "buffer_size");
	VLTRVIRTUAL_BIND(_get_available_packet_count);
	VLTRVIRTUAL_BIND(_get_max_packet_size);

	VLTRVIRTUAL_BIND(_poll);
	VLTRVIRTUAL_BIND(_close);

	VLTRVIRTUAL_BIND(_set_write_mode, "write_mode");
	VLTRVIRTUAL_BIND(_get_write_mode);

	VLTRVIRTUAL_BIND(_was_string_packet);
	VLTRVIRTUAL_BIND(_get_ready_state);
	VLTRVIRTUAL_BIND(_get_label);
	VLTRVIRTUAL_BIND(_is_ordered);
	VLTRVIRTUAL_BIND(_get_id);
	VLTRVIRTUAL_BIND(_get_max_packet_life_time);
	VLTRVIRTUAL_BIND(_get_max_retransmits);
	VLTRVIRTUAL_BIND(_get_protocol);
	VLTRVIRTUAL_BIND(_is_negotiated);
	VLTRVIRTUAL_BIND(_get_buffered_amount);
}

Error WebRTCDataChannelExtension::get_packet(const uint8_t **r_buffer, int &r_buffer_size) {
	Error err;
	if (VLTRVIRTUAL_CALL(_get_packet, r_buffer, &r_buffer_size, err)) {
		return err;
	}
	WARN_PRINT_ONCE("WebRTCDataChannelExtension::_get_packet_native is unimplemented!");
	return FAILED;
}

Error WebRTCDataChannelExtension::put_packet(const uint8_t *p_buffer, int p_buffer_size) {
	Error err;
	if (VLTRVIRTUAL_CALL(_put_packet, p_buffer, p_buffer_size, err)) {
		return err;
	}
	WARN_PRINT_ONCE("WebRTCDataChannelExtension::_put_packet_native is unimplemented!");
	return FAILED;
}
