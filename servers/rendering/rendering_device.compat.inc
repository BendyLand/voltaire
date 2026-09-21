/**************************************************************************/
/*  rendering_device.compat.inc                                           */
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

#ifndef DISABLE_DEPRECATED

#include "rendering_device.h"

RenderingDevice::InitialAction RenderingDevice::_convert_initial_action_84976(
	InitialAction p_old_initial_action)
{
	switch (uint32_t(p_old_initial_action)) {
	case 0: // INITIAL_ACTION_CLEAR
		return INITIAL_ACTION_CLEAR;
	case 1: // INITIAL_ACTION_CLEAR_REGION
	case 2: // INITIAL_ACTION_CLEAR_REGION_CONTINUE
		return INITIAL_ACTION_CLEAR;
	case 3: // INITIAL_ACTION_KEEP
		return INITIAL_ACTION_LOAD;
	case 4: // INITIAL_ACTION_DROP
		return INITIAL_ACTION_DISCARD;
	case 5: // INITIAL_ACTION_CONTINUE
		return INITIAL_ACTION_LOAD;
	default:
		return INITIAL_ACTION_LOAD;
	}
}

RenderingDevice::FinalAction RenderingDevice::_convert_final_action_84976(
	FinalAction p_old_final_action)
{
	switch (uint32_t(p_old_final_action)) {
	case 0: // FINAL_ACTION_READ
		return FINAL_ACTION_STORE;
	case 1: // FINAL_ACTION_DISCARD
		return FINAL_ACTION_DISCARD;
	case 2: // FINAL_ACTION_CONTINUE
		return FINAL_ACTION_STORE;
	default:
		return FINAL_ACTION_STORE;
	}
}

RenderingDevice::ComputeListID RenderingDevice::_compute_list_begin_bind_compat_84976(
	bool p_allow_draw_overlap)
{
	return compute_list_begin();
}

RenderingDevice::FramebufferFormatID
RenderingDevice::_screen_get_framebuffer_format_bind_compat_87340() const
{
	return screen_get_framebuffer_format(DisplayServerEnums::MAIN_WINDOW_ID);
}

RID RenderingDevice::_vertex_buffer_create_bind_compat_101561(
	uint32_t p_size_bytes, const Vector<uint8_t>& p_data, bool p_use_as_storage)
{
	return _vertex_buffer_create(
		p_size_bytes, p_data, p_use_as_storage ? RD::BUFFER_CREATION_AS_STORAGE_BIT : 0);
}

#endif // DISABLE_DEPRECATED


