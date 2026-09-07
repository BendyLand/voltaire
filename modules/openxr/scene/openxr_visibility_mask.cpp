/**************************************************************************/
/*  openxr_visibility_mask.cpp                                            */
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

#include "../extensions/openxr_visibility_mask_extension.h"
#include "../openxr_interface.h"
#include "openxr_visibility_mask.h"
#include "scene/3d/xr/xr_nodes.h"
#include "servers/rendering/rendering_server.h"

void OpenXRVisibilityMask::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		OpenXRVisibilityMaskExtension* vis_mask_ext =
			OpenXRVisibilityMaskExtension::get_singleton();
		if (vis_mask_ext && vis_mask_ext->is_available()) {
			set_base(vis_mask_ext->get_mesh());
		}
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_base(RID());
	} break;
	}
}

void OpenXRVisibilityMask::_on_openxr_session_begun()
{
	if (is_inside_tree()) {
		OpenXRVisibilityMaskExtension* vis_mask_ext =
			OpenXRVisibilityMaskExtension::get_singleton();
		if (vis_mask_ext && vis_mask_ext->is_available()) {
			set_base(vis_mask_ext->get_mesh());
		}
	}
}

void OpenXRVisibilityMask::_on_openxr_session_stopping() { set_base(RID()); }

AABB OpenXRVisibilityMask::get_aabb() const
{
	AABB ret;

	// Make sure it's always visible, this is positioned through its shader.
	ret.position = Vector3(-1000.0, -1000.0, -1000.0);
	ret.size = Vector3(2000.0, 2000.0, 2000.0);

	return ret;
}


