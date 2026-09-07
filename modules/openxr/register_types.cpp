/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "action_map/openxr_action.h"
#include "action_map/openxr_action_map.h"
#include "action_map/openxr_action_set.h"
#include "action_map/openxr_haptic_feedback.h"
#include "action_map/openxr_interaction_profile.h"
#include "action_map/openxr_interaction_profile_metadata.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "modules/modules_enabled.gen.h"
#include "openxr_api_extension.h"
#include "openxr_interface.h"
#include "register_types.h"

#ifndef DISABLE_DEPRECATED
#include "scene/openxr_hand.h"
#endif // DISABLE_DEPRECATED

#include "extensions/openxr_android_thread_settings_extension.h"
#include "extensions/openxr_composition_layer_depth_extension.h"
#include "extensions/openxr_composition_layer_extension.h"
#include "extensions/openxr_debug_utils_extension.h"
#include "extensions/openxr_dpad_binding_extension.h"
#include "extensions/openxr_eye_gaze_interaction.h"
#include "extensions/openxr_fb_display_refresh_rate_extension.h"
#include "extensions/openxr_frame_synthesis_extension.h"
#include "extensions/openxr_future_extension.h"
#include "extensions/openxr_hand_interaction_extension.h"
#include "extensions/openxr_hand_tracking_extension.h"
#include "extensions/openxr_htc_controller_extension.h"
#include "extensions/openxr_htc_vive_tracker_extension.h"
#include "extensions/openxr_huawei_controller_extension.h"
#include "extensions/openxr_khr_generic_controller_extension.h"
#include "extensions/openxr_local_floor_extension.h"
#include "extensions/openxr_meta_controller_extension.h"
#include "extensions/openxr_ml2_controller_extension.h"
#include "extensions/openxr_mxink_extension.h"
#include "extensions/openxr_palm_pose_extension.h"
#include "extensions/openxr_performance_settings_extension.h"
#include "extensions/openxr_pico_controller_extension.h"
#include "extensions/openxr_user_presence_extension.h"
#include "extensions/openxr_valve_analog_threshold_extension.h"
#include "extensions/openxr_valve_controller_extension.h"
#include "extensions/openxr_visibility_mask_extension.h"
#include "extensions/openxr_wmr_controller_extension.h"
#include "extensions/spatial_entities/openxr_spatial_anchor.h"
#include "extensions/spatial_entities/openxr_spatial_entity_extension.h"
#include "extensions/spatial_entities/openxr_spatial_marker_tracking.h"
#include "extensions/spatial_entities/openxr_spatial_plane_tracking.h"
#include "scene/openxr_composition_layer.h"
#include "scene/openxr_composition_layer_cylinder.h"
#include "scene/openxr_composition_layer_equirect.h"
#include "scene/openxr_composition_layer_quad.h"
#include "scene/openxr_visibility_mask.h"

#ifdef MODULE_GLTF_ENABLED
#include "extensions/openxr_render_model_extension.h"
#include "scene/openxr_render_model.h"
#include "scene/openxr_render_model_manager.h"
#endif

#ifdef TOOLS_ENABLED
#include "editor/openxr_editor_plugin.h"
#endif

#ifdef ANDROID_ENABLED
#include "extensions/platform/openxr_android_extension.h"
#endif

#ifdef TOOLS_ENABLED
#include "editor/openxr_binding_modifier_editor.h"
#include "editor/openxr_interaction_profile_editor.h"
//
#include "editor/editor_node.h"
#endif

static OpenXRAPI* openxr_api = nullptr;
static OpenXRInteractionProfileMetadata* openxr_interaction_profile_metadata = nullptr;
static Ref<OpenXRInterface> openxr_interface;

#ifdef TOOLS_ENABLED
static void _editor_init()
{
	if (OpenXRAPI::openxr_is_enabled(false)) {
		if (openxr_interaction_profile_metadata == nullptr) {
			// If we didn't initialize our actionmap metadata at startup, we initialize it now.
			openxr_interaction_profile_metadata = memnew(OpenXRInteractionProfileMetadata);
			ERR_FAIL_NULL(openxr_interaction_profile_metadata);
		}
	}

	OpenXREditorPlugin* openxr_plugin = memnew(OpenXREditorPlugin());
	EditorNode::get_singleton()->add_editor_plugin(openxr_plugin);
}
#endif

void uninitialize_openxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (openxr_interface.is_valid()) {
		// uninitialize just in case
		if (openxr_interface->is_initialized()) {
			openxr_interface->uninitialize();
		}

		// unregister our interface from the XR server
		XRServer* xr_server = XRServer::get_singleton();
		if (xr_server) {
			if (xr_server->get_primary_interface() == openxr_interface) {
				xr_server->set_primary_interface(Ref<XRInterface>());
			}
			xr_server->remove_interface(openxr_interface);
		}

		// and release
		openxr_interface.unref();
	}

	if (openxr_api) {
		openxr_api->finish();

		memdelete(openxr_api);
		openxr_api = nullptr;
	}

	if (openxr_interaction_profile_metadata) {
		memdelete(openxr_interaction_profile_metadata);
		openxr_interaction_profile_metadata = nullptr;
	}

	// cleanup our extension wrappers
	OpenXRAPI::cleanup_extension_wrappers();
}


