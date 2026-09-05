/**************************************************************************/
/*  editor_build_profile.cpp                                              */
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

#include "core/config/project_settings.h"
#include "core/io/json.h"
#include "core/io/resource_importer.h"
#include "core/io/resource_loader.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_build_profile.h"
#include "modules/modules_enabled.gen.h" // IWYU pragma: keep. For mono.
#include "scene/gui/line_edit.h"
#include "scene/gui/margin_container.h"
#include "scene/gui/separator.h"

const char* EditorBuildProfile::build_option_identifiers[BUILD_OPTION_MAX] = {
	// This maps to SCons build options.
	"disable_3d",
	"disable_navigation_2d",
	"disable_navigation_3d",
	"accesskit",
	"sdl",
	"disable_xr",
	"module_openxr_enabled",
	"wayland",
	"x11",
	"pulseaudio",
	"alsa",
	"rendering_device", // FIXME: There's no scons option to disable rendering device.
	"forward_plus_renderer",
	"forward_mobile_renderer",
	"vulkan",
	"d3d12",
	"metal",
	"opengl3",
	"disable_physics_2d",
	"module_godot_physics_2d_enabled",
	"disable_physics_3d",
	"module_godot_physics_3d_enabled",
	"module_jolt_physics_enabled",
	"module_text_server_fb_enabled",
	"module_text_server_adv_enabled",
	"module_freetype_enabled",
	"brotli",
	"graphite",
	"module_msdfgen_enabled",
};

const bool EditorBuildProfile::build_option_disabled_by_default[BUILD_OPTION_MAX] = {
	// This maps to SCons build options.
	false, // 3D
	false, // NAVIGATION_2D
	false, // NAVIGATION_3D
	false, // ACCESSKIT
	false, // SDL
	false, // XR
	false, // OPENXR
	false, // WAYLAND
	false, // X11
	false, // PULSEAUDIO
	false, // ALSA
	false, // RENDERING_DEVICE
	false, // FORWARD_RENDERER
	false, // MOBILE_RENDERER
	false, // VULKAN
	false, // D3D12
	false, // METAL
	false, // OPENGL
	false, // PHYSICS_2D
	false, // PHYSICS_GODOT_2D
	false, // PHYSICS_3D
	false, // PHYSICS_GODOT_3D
	false, // PHYSICS_JOLT
	true,  // TEXT_SERVER_FALLBACK
	false, // TEXT_SERVER_ADVANCED
	false, // DYNAMIC_FONTS
	false, // WOFF2_FONTS
	false, // GRAPHITE_FONTS
	false, // MSDFGEN
};

const bool EditorBuildProfile::build_option_disable_values[BUILD_OPTION_MAX] = {
	// This maps to SCons build options.
	true,  // 3D
	true,  // NAVIGATION_2D
	true,  // NAVIGATION_3D
	false, // ACCESSKIT
	false, // SDL
	true,  // XR
	false, // OPENXR
	false, // WAYLAND
	false, // X11
	false, // PULSEAUDIO
	false, // ALSA
	false, // RENDERING_DEVICE
	false, // FORWARD_RENDERER
	false, // MOBILE_RENDERER
	false, // VULKAN
	false, // D3D12
	false, // METAL
	false, // OPENGL
	true,  // PHYSICS_2D
	false, // PHYSICS_GODOT_2D
	true,  // PHYSICS_3D
	false, // PHYSICS_GODOT_3D
	false, // PHYSICS_JOLT
	false, // TEXT_SERVER_FALLBACK
	false, // TEXT_SERVER_ADVANCED
	false, // DYNAMIC_FONTS
	false, // WOFF2_FONTS
	false, // GRAPHITE_FONTS
	false, // MSDFGEN
};

// Options that require some resource explicitly asking for them when detecting from the project.
const bool EditorBuildProfile::build_option_explicit_use[BUILD_OPTION_MAX] = {
	false, // 3D
	false, // NAVIGATION_2D
	false, // NAVIGATION_3D
	false, // ACCESSKIT
	false, // SDL
	false, // XR
	false, // OPENXR
	false, // WAYLAND
	false, // X11
	false, // PULSEAUDIO
	false, // ALSA
	false, // RENDERING_DEVICE
	false, // FORWARD_RENDERER
	false, // MOBILE_RENDERER
	false, // VULKAN
	false, // D3D12
	false, // METAL
	false, // OPENGL
	false, // PHYSICS_2D
	false, // PHYSICS_GODOT_2D
	false, // PHYSICS_3D
	false, // PHYSICS_GODOT_3D
	false, // PHYSICS_JOLT
	false, // TEXT_SERVER_FALLBACK
	false, // TEXT_SERVER_ADVANCED
	false, // DYNAMIC_FONTS
	false, // WOFF2_FONTS
	false, // GRAPHITE_FONTS
	true,  // MSDFGEN
};

const EditorBuildProfile::BuildOptionCategory
	EditorBuildProfile::build_option_category[BUILD_OPTION_MAX] = {
		BUILD_OPTION_CATEGORY_GENERAL,	   // 3D
		BUILD_OPTION_CATEGORY_GENERAL,	   // NAVIGATION_2D
		BUILD_OPTION_CATEGORY_GENERAL,	   // NAVIGATION_3D
		BUILD_OPTION_CATEGORY_GENERAL,	   // ACCESSKIT
		BUILD_OPTION_CATEGORY_GENERAL,	   // SDL
		BUILD_OPTION_CATEGORY_GENERAL,	   // XR
		BUILD_OPTION_CATEGORY_GENERAL,	   // OPENXR
		BUILD_OPTION_CATEGORY_GENERAL,	   // WAYLAND
		BUILD_OPTION_CATEGORY_GENERAL,	   // X11
		BUILD_OPTION_CATEGORY_GENERAL,	   // PULSEAUDIO
		BUILD_OPTION_CATEGORY_GENERAL,	   // ALSA
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // RENDERING_DEVICE
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // FORWARD_RENDERER
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // MOBILE_RENDERER
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // VULKAN
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // D3D12
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // METAL
		BUILD_OPTION_CATEGORY_GRAPHICS,	   // OPENGL
		BUILD_OPTION_CATEGORY_PHYSICS,	   // PHYSICS_2D
		BUILD_OPTION_CATEGORY_PHYSICS,	   // PHYSICS_GODOT_2D
		BUILD_OPTION_CATEGORY_PHYSICS,	   // PHYSICS_3D
		BUILD_OPTION_CATEGORY_PHYSICS,	   // PHYSICS_GODOT_3D
		BUILD_OPTION_CATEGORY_PHYSICS,	   // PHYSICS_JOLT
		BUILD_OPTION_CATEGORY_TEXT_SERVER, // TEXT_SERVER_FALLBACK
		BUILD_OPTION_CATEGORY_TEXT_SERVER, // TEXT_SERVER_ADVANCED
		BUILD_OPTION_CATEGORY_TEXT_SERVER, // DYNAMIC_FONTS
		BUILD_OPTION_CATEGORY_TEXT_SERVER, // WOFF2_FONTS
		BUILD_OPTION_CATEGORY_TEXT_SERVER, // GRAPHITE_FONTS
		BUILD_OPTION_CATEGORY_TEXT_SERVER, // MSDFGEN
};

/* clang-format off */

const HashMap<EditorBuildProfile::BuildOption, LocalVector<EditorBuildProfile::BuildOption>> EditorBuildProfile::build_option_dependencies = {
	{ BUILD_OPTION_OPENXR, {
			BUILD_OPTION_XR,
	} },
	{ BUILD_OPTION_FORWARD_RENDERER, {
			BUILD_OPTION_RENDERING_DEVICE,
	} },
	{ BUILD_OPTION_MOBILE_RENDERER, {
			BUILD_OPTION_RENDERING_DEVICE,
	} },
	{ BUILD_OPTION_VULKAN, {
			BUILD_OPTION_FORWARD_RENDERER,
			BUILD_OPTION_MOBILE_RENDERER,
	} },
	{ BUILD_OPTION_D3D12, {
			BUILD_OPTION_FORWARD_RENDERER,
			BUILD_OPTION_MOBILE_RENDERER,
	} },
	{ BUILD_OPTION_METAL, {
			BUILD_OPTION_FORWARD_RENDERER,
			BUILD_OPTION_MOBILE_RENDERER,
	} },
	{ BUILD_OPTION_PHYSICS_GODOT_2D, {
			BUILD_OPTION_PHYSICS_2D,
	} },
	{ BUILD_OPTION_PHYSICS_GODOT_3D, {
			BUILD_OPTION_PHYSICS_3D,
	} },
	{ BUILD_OPTION_PHYSICS_JOLT, {
			BUILD_OPTION_PHYSICS_3D,
	} },
	{ BUILD_OPTION_DYNAMIC_FONTS, {
			BUILD_OPTION_TEXT_SERVER_ADVANCED,
	} },
	{ BUILD_OPTION_WOFF2_FONTS, {
			BUILD_OPTION_TEXT_SERVER_ADVANCED,
	} },
	{ BUILD_OPTION_GRAPHITE_FONTS, {
			BUILD_OPTION_TEXT_SERVER_ADVANCED,
	} },
};

// Should also contain classes not derived from either `Resource` or `Node`.
const HashMap<EditorBuildProfile::BuildOption, LocalVector<String>> EditorBuildProfile::build_option_classes = {
	{ BUILD_OPTION_3D, {
			"Node3D",
	} },
	{ BUILD_OPTION_NAVIGATION_2D, {
			"NavigationAgent2D",
			"NavigationLink2D",
			"NavigationMeshSourceGeometryData2D",
			"NavigationObstacle2D",
			"NavigationPolygon",
			"NavigationRegion2D",
	} },
	{ BUILD_OPTION_NAVIGATION_3D, {
			"NavigationAgent3D",
			"NavigationLink3D",
			"NavigationMeshSourceGeometryData3D",
			"NavigationObstacle3D",
			"NavigationRegion3D",
	} },
	{ BUILD_OPTION_XR, {
			"XRBodyModifier3D",
			"XRBodyTracker",
			"XRControllerTracker",
			"XRFaceModifier3D",
			"XRFaceTracker",
			"XRHandModifier3D",
			"XRHandTracker",
			"XRInterface",
			"XRInterfaceExtension",
			"XRNode3D",
			"XROrigin3D",
			"XRPose",
			"XRPositionalTracker",
			"XRServer",
			"XRTracker",
			"XRVRS",
	} },
	{ BUILD_OPTION_RENDERING_DEVICE, {
			"RenderingDevice",
	} },
	{ BUILD_OPTION_PHYSICS_2D, {
			"CollisionObject2D",
			"CollisionPolygon2D",
			"CollisionShape2D",
			"Joint2D",
			"PhysicsServer2D",
			"PhysicsServer2DManager",
			"ShapeCast2D",
			"RayCast2D",
			"TouchScreenButton",
	} },
	{ BUILD_OPTION_PHYSICS_3D, {
			"CollisionObject3D",
			"CollisionPolygon3D",
			"CollisionShape3D",
			"CSGShape3D",
			"GPUParticlesAttractor3D",
			"GPUParticlesCollision3D",
			"Joint3D",
			"PhysicalBoneSimulator3D",
			"PhysicsServer3D",
			"PhysicsServer3DManager",
			"PhysicsServer3DRenderingServerHandler",
			"RayCast3D",
			"SoftBody3D",
			"SpringArm3D",
			"VehicleWheel3D",
	} },
	{ BUILD_OPTION_TEXT_SERVER_ADVANCED, {
			"CanvasItem",
			"Label3D",
			"TextServerAdvanced",
	} },
};

/* clang-format on */

void EditorBuildProfile::set_disable_class(const StringName& p_class, bool p_disabled)
{
	if (p_disabled) {
		disabled_classes.insert(p_class);
	}
	else {
		disabled_classes.erase(p_class);
	}
}

bool EditorBuildProfile::is_class_disabled(const StringName& p_class) const
{
	if (p_class == StringName()) {
		return false;
	}
	return disabled_classes.has(p_class);
}

void EditorBuildProfile::set_item_collapsed(const StringName& p_class, bool p_collapsed)
{
	if (p_collapsed) {
		collapsed_classes.insert(p_class);
	}
	else {
		collapsed_classes.erase(p_class);
	}
}

bool EditorBuildProfile::is_item_collapsed(const StringName& p_class) const
{
	return collapsed_classes.has(p_class);
}

void EditorBuildProfile::set_disable_build_option(BuildOption p_build_option, bool p_disable)
{
	ERR_FAIL_INDEX(p_build_option, BUILD_OPTION_MAX);
	build_options_disabled[p_build_option] = p_disable;
}

void EditorBuildProfile::clear_disabled_classes()
{
	disabled_classes.clear();
	collapsed_classes.clear();
}

bool EditorBuildProfile::is_build_option_disabled(BuildOption p_build_option) const
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, false);
	return build_options_disabled[p_build_option];
}

bool EditorBuildProfile::get_build_option_disable_value(BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, false);
	return build_option_disable_values[p_build_option];
}

bool EditorBuildProfile::get_build_option_explicit_use(BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, false);
	return build_option_explicit_use[p_build_option];
}

void EditorBuildProfile::reset_build_options()
{
	for (int i = 0; i < EditorBuildProfile::BUILD_OPTION_MAX; i++) {
		build_options_disabled[i] = build_option_disabled_by_default[i];
	}
}

void EditorBuildProfile::set_force_detect_classes(const String& p_classes)
{
	force_detect_classes = p_classes;
}

String EditorBuildProfile::get_force_detect_classes() const { return force_detect_classes; }

String EditorBuildProfile::get_build_option_name(BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, String());
	const char* build_option_names[BUILD_OPTION_MAX] = {
		TTRC("3D Engine"),
		TTRC("Navigation (2D)"),
		TTRC("Navigation (3D)"),
		TTRC("Accessibility Support (AccessKit)"),
		TTRC("Improved Gamepad Support (SDL)"),
		TTRC("XR"),
		TTRC("OpenXR"),
		TTRC("Wayland"),
		TTRC("X11"),
		TTRC("PulseAudio"),
		TTRC("ALSA"),
		TTRC("RenderingDevice"),
		TTRC("Forward+ Renderer"),
		TTRC("Mobile Renderer"),
		TTRC("Vulkan"),
		TTRC("D3D12"),
		TTRC("Metal"),
		TTRC("OpenGL"),
		TTRC("Physics Server (2D)"),
		TTRC("Godot Physics (2D)"),
		TTRC("Physics Server (3D)"),
		TTRC("Godot Physics (3D)"),
		TTRC("Jolt Physics"),
		TTRC("Text Server: Fallback"),
		TTRC("Text Server: Advanced"),
		TTRC("TTF, OTF, Type 1, WOFF1 Fonts"),
		TTRC("WOFF2 Fonts"),
		TTRC("SIL Graphite Fonts"),
		TTRC("Multi-channel Signed Distance Field Font Rendering"),
	};
	return TTRGET(build_option_names[p_build_option]);
}

String EditorBuildProfile::get_build_option_description(BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, String());

	const char* build_option_descriptions[BUILD_OPTION_MAX] = {
		TTRC("3D Nodes as well as RenderingServer access to 3D features.\nNote that the Geometry3D "
			 "singleton remains available even with this item disabled."),
		TTRC("NavigationServer and capabilities for 2D."),
		TTRC("NavigationServer and capabilities for 3D."),
		TTRC("Support for screen readers using the AccessKit library."),
		TTRC("Improved gamepad support on Windows, macOS, and Linux using the SDL library.\nIf "
			 "disabled, built-in custom code is used for gamepad support instead, which may be "
			 "less reliable for certain controller models."),
		TTRC("XR (AR and VR)."),
		TTRC("OpenXR standard implementation (requires XR to be enabled)."),
		TTRC("Wayland display server support (Linux only)."),
		TTRC("X11 display server support (Linux only)."),
		TTRC("PulseAudio audio driver (Linux only)."),
		TTRC("ALSA audio driver (Linux only)."),
		TTRC("RenderingDevice-based rendering (if disabled, the OpenGL backend is required)."),
		TTRC("Forward+ renderer for advanced 3D graphics."),
		TTRC("Mobile renderer for less advanced 3D graphics."),
		TTRC("Vulkan backend of RenderingDevice."),
		TTRC("Direct3D 12 backend of RenderingDevice."),
		TTRC("Metal backend of RenderingDevice (Apple arm64 only)."),
		TTRC("OpenGL backend (if disabled, the RenderingDevice backend is required)."),
		TTRC("PhysicsServer and capabilities for 2D."),
		TTRC("Godot Physics backend (2D)."),
		TTRC("PhysicsServer and capabilities for 3D."),
		TTRC("Godot Physics backend (3D)."),
		TTRC("Jolt Physics backend (3D only)."),
		TTRC("Fallback implementation of Text Server\nSupports basic text layouts."),
		TTRC("Text Server implementation powered by ICU and HarfBuzz libraries.\nSupports complex "
			 "text layouts, BiDi, and contextual OpenType font features."),
		TTRC("TrueType, OpenType, Type 1, and WOFF1 font format support using FreeType library (if "
			 "disabled, WOFF2 support is also disabled)."),
		TTRC("WOFF2 font format support using FreeType and Brotli libraries."),
		TTRC(
			"SIL Graphite smart font technology support (supported by Advanced Text Server only)."),
		TTRC("Multi-channel signed distance field font rendering support using msdfgen library "
			 "(pre-rendered MSDF fonts can be used even if this option is disabled)."),
	};

	return TTRGET(build_option_descriptions[p_build_option]);
}

String EditorBuildProfile::get_build_option_identifier(BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, String());
	return build_option_identifiers[p_build_option];
}

EditorBuildProfile::BuildOptionCategory EditorBuildProfile::get_build_option_category(
	BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, BUILD_OPTION_CATEGORY_GENERAL);
	return build_option_category[p_build_option];
}

LocalVector<EditorBuildProfile::BuildOption> EditorBuildProfile::get_build_option_dependencies(
	BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(
		p_build_option, BUILD_OPTION_MAX, LocalVector<EditorBuildProfile::BuildOption>());
	return build_option_dependencies.has(p_build_option)
			   ? LocalVector<EditorBuildProfile::BuildOption>(
					 build_option_dependencies[p_build_option])
			   : LocalVector<EditorBuildProfile::BuildOption>();
}

LocalVector<String> EditorBuildProfile::get_build_option_classes(BuildOption p_build_option)
{
	ERR_FAIL_INDEX_V(p_build_option, BUILD_OPTION_MAX, LocalVector<String>());
	return build_option_classes.has(p_build_option)
			   ? LocalVector<String>(build_option_classes[p_build_option])
			   : LocalVector<String>();
}

String EditorBuildProfile::get_build_option_category_name(
	BuildOptionCategory p_build_option_category)
{
	ERR_FAIL_INDEX_V(p_build_option_category, BUILD_OPTION_CATEGORY_MAX, String());

	const char* build_option_subcategories[BUILD_OPTION_CATEGORY_MAX]{
		TTRC("General Features:"),
		TTRC("Graphics and Rendering:"),
		TTRC("Physics Systems:"),
		TTRC("Text Rendering and Font Options:"),
	};

	return TTRGET(build_option_subcategories[p_build_option_category]);
}

//////////////////////////

void EditorBuildProfileManager::_profile_action(int p_action)
{
	last_action = Action(p_action);

	switch (p_action) {
	case ACTION_RESET: {
		confirm_dialog->set_text(TTRC("Reset the edited profile?"));
		confirm_dialog->popup_centered();
	} break;

	case ACTION_LOAD: {
		import_profile->popup_file_dialog();
	} break;

	case ACTION_SAVE: {
		if (!profile_path->get_text().is_empty()) {
			Error err = edited->save_to_file(profile_path->get_text());
			if (err != OK) {
				EditorNode::get_singleton()->show_warning(TTRC("File saving failed."));
			}
			break;
		}
		[[fallthrough]];
	}
	case ACTION_SAVE_AS: {
		export_profile->popup_file_dialog();
		export_profile->set_current_file(profile_path->get_text());
	} break;

	case ACTION_NEW: {
		confirm_dialog->set_text(TTRC("Create a new profile?"));
		confirm_dialog->popup_centered();
	} break;

	case ACTION_DETECT: {
		String text =
			TTR("This will scan all files in the current project to detect used classes.\nNote "
				"that the first scan may take a while, specially in larger projects.");
#ifdef MODULE_MONO_ENABLED
		text += "\n\n" + TTR("Warning: Class detection for C# scripts is not currently available, "
							 "and such files will be ignored.");
#endif // MODULE_MONO_ENABLED
		confirm_dialog->set_text(text);
		confirm_dialog->popup_centered();
	} break;

	case ACTION_CLEAR_CACHE: {
		confirm_dialog->set_text(TTRC(
			"Clear cache of used classes per file? This will make it so that those files will need "
			"to be re-scanned, but it can also help fix problems related to outdated caching."));
		confirm_dialog->popup_centered();
	} break;

	case ACTION_MAX: {
	} break;
	}
}

void EditorBuildProfileManager::_find_files(EditorFileSystemDirectory* p_dir,
	const HashMap<String, DetectedFile>& p_cache, HashMap<String, DetectedFile>& r_detected)
{
	if (p_dir == nullptr || p_dir->get_path().get_file().begins_with(".")) {
		return;
	}

	for (int i = 0; i < p_dir->get_file_count(); i++) {
		String p = p_dir->get_file_path(i);

		if (EditorNode::get_singleton()->progress_task_step("detect_classes_from_project", p, 1)) {
			project_scan_canceled = true;
			return;
		}

		String p_check = p;
		// Make so that the import file is the one checked if available,
		// so the cache can be updated when it changes.
		if (ResourceFormatImporter::get_singleton()->exists(p_check)) {
			p_check += ".import";
		}

		uint64_t timestamp = 0;
		String md5;

		if (p_cache.has(p)) {
			const DetectedFile& cache = p_cache[p];
			// Check if timestamp and MD5 match.
			timestamp = FileAccess::get_modified_time(p_check);
			bool cache_valid = true;
			if (cache.timestamp != timestamp) {
				md5 = FileAccess::get_md5(p_check);
				if (md5 != cache.md5) {
					cache_valid = false;
				}
			}

			if (cache_valid) {
				r_detected.insert(p, cache);
				continue;
			}
		}

		// Not cached, or cache invalid.

		DetectedFile cache;

		HashSet<StringName> classes;
		ResourceLoader::get_classes_used(p, &classes);
		for (const StringName& E : classes) {
			cache.classes.push_back(E);
		}

		HashSet<String> build_deps;
		ResourceFormatImporter::get_singleton()->get_build_dependencies(p, &build_deps);
		for (const String& E : build_deps) {
			cache.build_deps.push_back(E);
		}

		if (md5.is_empty()) {
			cache.timestamp = FileAccess::get_modified_time(p_check);
			cache.md5 = FileAccess::get_md5(p_check);
		}
		else {
			cache.timestamp = timestamp;
			cache.md5 = md5;
		}

		r_detected.insert(p, cache);
	}

	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_find_files(p_dir->get_subdir(i), p_cache, r_detected);
	}
}

void EditorBuildProfileManager::_action_confirm()
{
	switch (last_action) {
	case ACTION_RESET: {
		edited.instantiate();
		_update_edited_profile();
	} break;

	case ACTION_NEW: {
		profile_path->set_text("");
		edited.instantiate();
		_update_edited_profile();
	} break;

	case ACTION_DETECT: {
		_detect_from_project();
		_update_edited_profile();
	} break;

	case ACTION_CLEAR_CACHE: {
		String cache_path =
			EditorPaths::get_singleton()->get_project_settings_dir().path_join("used_class_cache");
		Error err = DirAccess::remove_absolute(cache_path);
		if (err != OK) {
			ERR_FAIL_MSG(vformat("Cannot remove cache file: '%s'.", cache_path));
		}
		else {
			profile_actions[ACTION_CLEAR_CACHE]->set_disabled(true);
		}
	} break;

	default: {
	} break;
	}
}

void EditorBuildProfileManager::_hide_requested()
{
	_cancel_pressed(); // From AcceptDialog.
}

void EditorBuildProfileManager::_force_detect_classes_changed(const String& p_text)
{
	if (updating_build_options) {
		return;
	}
	edited->set_force_detect_classes(force_detect_classes->get_text());
}

Ref<EditorBuildProfile> EditorBuildProfileManager::get_current_profile() { return edited; }

EditorBuildProfileManager* EditorBuildProfileManager::singleton = nullptr;


