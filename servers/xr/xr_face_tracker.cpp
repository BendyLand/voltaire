/**************************************************************************/
/*  xr_face_tracker.cpp                                                   */
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

#include "core/object/class_db.h"
#include "xr_face_tracker.h"

void XRFaceTracker::_bind_methods() {}

void XRFaceTracker::set_tracker_type(XRServer::TrackerType p_type)
{
	ERR_FAIL_COND_MSG(
		p_type != XRServer::TRACKER_FACE, "XRFaceTracker must be of type TRACKER_FACE.");
}

float XRFaceTracker::get_blend_shape(BlendShapeEntry p_blend_shape) const
{
	// Fail if the blend shape index is out of range.
	ERR_FAIL_INDEX_V(p_blend_shape, FT_MAX, 0.0f);

	// Return the blend shape value.
	return blend_shape_values[p_blend_shape];
}

void XRFaceTracker::set_blend_shape(BlendShapeEntry p_blend_shape, float p_value)
{
	// Fail if the blend shape index is out of range.
	ERR_FAIL_INDEX(p_blend_shape, FT_MAX);

	// Save the new blend shape value.
	blend_shape_values[p_blend_shape] = p_value;
}

PackedFloat32Array XRFaceTracker::get_blend_shapes() const
{
	// Create a packed float32 array and copy the blend shape values into it.
	PackedFloat32Array data;
	data.resize(FT_MAX);
	memcpy(data.ptrw(), blend_shape_values, sizeof(blend_shape_values));

	// Return the blend shape array.
	return data;
}

void XRFaceTracker::set_blend_shapes(const PackedFloat32Array& p_blend_shapes)
{
	// Fail if the blend shape array is not the correct size.
	ERR_FAIL_COND(p_blend_shapes.size() != FT_MAX);

	// Copy the blend shape values into the blend shape array.
	memcpy(blend_shape_values, p_blend_shapes.ptr(), sizeof(blend_shape_values));
}

XRFaceTracker::XRFaceTracker() { type = XRServer::TRACKER_FACE; }


