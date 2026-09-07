/**************************************************************************/
/*  openxr_render_model_manager.cpp                                       */
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

#include "openxr_render_model_manager.h"

#ifdef MODULE_GLTF_ENABLED

#include "../extensions/openxr_render_model_extension.h"
#include "../openxr_api.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "openxr_render_model.h"
#include "scene/3d/xr/xr_nodes.h"
#include "servers/xr/xr_server.h"

bool OpenXRRenderModelManager::_has_filters() { return tracker != 0; }

void OpenXRRenderModelManager::_on_render_model_added(RID p_render_model)
{
	if (_has_filters()) {
		// We'll update this in internal process.
		is_dirty = true;
	}
	else {
		// No filters? Do this right away.
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_removed(RID p_render_model)
{
	if (_has_filters()) {
		// We'll update this in internal process.
		is_dirty = true;
	}
	else {
		// No filters? Do this right away.
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_top_level_path_changed(RID p_path)
{
	if (_has_filters()) {
		// We'll update this in internal process.
		is_dirty = true;
	}
}

void OpenXRRenderModelManager::set_tracker(RenderModelTracker p_tracker)
{
	if (tracker != p_tracker) {
		tracker = p_tracker;
		is_dirty = true;

		if (tracker == RENDER_MODEL_TRACKER_ANY || tracker == RENDER_MODEL_TRACKER_NONE_SET) {
			xr_path = XR_NULL_PATH;
		}
		else if (!Engine::get_singleton()->is_editor_hint()) {
			XRServer* xr_server = XRServer::get_singleton();
			OpenXRAPI* openxr_api = OpenXRAPI::get_singleton();
			if (openxr_api && xr_server) {
				String toplevel_path;
				String tracker_name;
				if (tracker == RENDER_MODEL_TRACKER_LEFT_HAND) {
					tracker_name = "left_hand";
					toplevel_path = "/user/hand/left";
				}
				else if (tracker == RENDER_MODEL_TRACKER_RIGHT_HAND) {
					tracker_name = "right_hand";
					toplevel_path = "/user/hand/right";
				}
				else {
					ERR_FAIL_MSG("Unsupported tracker value set.");
				}

				positional_tracker = xr_server->get_tracker(tracker_name);
				if (positional_tracker.is_null()) {
					WARN_PRINT("OpenXR: Can't find tracker " + tracker_name);
				}

				xr_path = openxr_api->get_xr_path(toplevel_path);
				if (xr_path == XR_NULL_PATH) {
					WARN_PRINT("OpenXR: Can't find path for " + toplevel_path);
				}
			}
		}

		// Even if we now no longer have filters, we must update at least once.
		set_process_internal(true);
	}
}

OpenXRRenderModelManager::RenderModelTracker OpenXRRenderModelManager::get_tracker() const
{
	return tracker;
}

void OpenXRRenderModelManager::set_make_local_to_pose(const String& p_action)
{
	if (make_local_to_pose != p_action) {
		make_local_to_pose = p_action;

		if (container) {
			// Reset just in case. It'll be set to the correct transform
			// in our process if required.
			container->set_transform(Transform3D());
		}
	}
}

String OpenXRRenderModelManager::get_make_local_to_pose() const { return make_local_to_pose; }
#endif // MODULE_GLTF_ENABLED


