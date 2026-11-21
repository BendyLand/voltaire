/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "action_map/openxr_action_map.h"
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

#include "extensions/openxr_composition_layer_extension.h"
#include "extensions/openxr_extensions.h"
#include "extensions/openxr_frame_synthesis_extension.h"
#include "extensions/openxr_future_extension.h"
#include "extensions/openxr_hand_tracking_extension.h"
#include "extensions/openxr_interaction_profile_extension.h"
#include "extensions/spatial_entities/openxr_spatial_anchor.h"
#include "extensions/spatial_entities/openxr_spatial_entity_extension.h"
#include "extensions/spatial_entities/openxr_spatial_marker_tracking.h"
#include "extensions/spatial_entities/openxr_spatial_plane_tracking.h"
#include "scene/openxr_composition_layer.h"
#include "scene/openxr_visibility_mask.h"

#ifdef MODULE_GLTF_ENABLED
#include "extensions/openxr_render_model_extension.h"
#include "scene/openxr_render_model.h"
#include "scene/openxr_render_model_manager.h"
#endif

#ifdef ANDROID_ENABLED
#include "extensions/platform/openxr_android_extension.h"
#endif

static Ref<OpenXRInterface> openxr_interface;

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
		if (XRServer::is_initialized()) {
			if (XRServer::get_primary_interface() == openxr_interface) {
				XRServer::set_primary_interface(Ref<XRInterface>());
			}
			XRServer::remove_interface(openxr_interface);
		}

		// and release
		openxr_interface.unref();
	}

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::finalize();
	}

	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		OpenXRInteractionProfileMetadata::finalize();
	}

	if (OpenXRInteractionProfileExtension::is_initialized()) {
		OpenXRInteractionProfileExtension::finalize();
	}

	// cleanup our extension wrappers
	OpenXRAPI::cleanup_extension_wrappers();
}

void initialize_openxr_module(ModuleInitializationLevel p_level) {}
