/**************************************************************************/
/*  editor_interface.h                                                    */
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

#pragma once

#include "core/io/resource.h"

class Control;
class CreateDialog;
class EditorCommandPalette;
class EditorFileSystem;
class EditorInspector;
class EditorPaths;
class EditorPlugin;
class EditorResourcePreview;
class EditorSelection;
class EditorSettings;
class EditorToaster;
class EditorUndoRedoManager;
class ScenePaint2DEditor;
class FileSystemDock;
class Mesh;
class Node;
class PropertySelector;
class SceneTreeDialog;
class ScriptEditor;
class SubViewport;
class Texture2D;
class Theme;
class VBoxContainer;
class Window;

class EditorInterface final
{
	struct Data
	{
		PropertySelector* property_selector = nullptr;
		PropertySelector* method_selector = nullptr;
		SceneTreeDialog* node_selector = nullptr;
		CreateDialog* create_dialog = nullptr;
	};

public:
	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static bool is_exiting();
	static void restart_editor(bool p_save = true);

	// Editor tools.

	static EditorCommandPalette* get_command_palette();
	static EditorFileSystem* get_resource_filesystem();
	static EditorResourcePreview* get_resource_previewer();
	static EditorSelection* get_selection();
	static EditorToaster* get_editor_toaster();
	static ScenePaint2DEditor* get_scene_paint_2d();

	static bool is_plugin_enabled(const String& p_plugin);

	// Editor GUI.

	static Control* get_base_control();
	static VBoxContainer* get_editor_main_screen();
	static ScriptEditor* get_script_editor();
	static SubViewport* get_editor_viewport_2d();
	static SubViewport* get_editor_viewport_3d(int p_idx = 0);

	static void set_main_screen_editor(const String& p_name);
	static bool is_distraction_free_mode_enabled();

	static float get_editor_scale();
	static String get_editor_language();

	static bool is_node_3d_snap_enabled();
	static real_t get_node_3d_translate_snap();
	static real_t get_node_3d_rotate_snap();
	static real_t get_node_3d_scale_snap();

	static void popup_dialog(Window* p_dialog, const Rect2i& p_screen_rect = Rect2i());
	static void popup_dialog_centered_ratio(Window* p_dialog, float p_ratio = 0.8);
	static void popup_dialog_centered_clamped(
		Window* p_dialog, const Size2i& p_size = Size2i(), float p_fallback_ratio = 0.75);

	static String get_current_feature_profile();
	static void set_current_feature_profile(const String& p_profile_name);

	// Editor docks.

	static FileSystemDock* get_file_system_dock();
	static String get_current_path();
	static String get_current_directory();

	static EditorInspector* get_inspector();

	// Object/Resource/Node editing.

	static PackedStringArray get_open_scenes();
	static PackedStringArray get_unsaved_scenes();

	static Vector<Node*> get_open_scene_roots();
	static Node* get_edited_scene_root();

	static void mark_scene_as_unsaved();

	// Scene playback.
	static void play_main_scene();
	static void play_custom_scene(const String& scene_path);
	static void stop_playing_scene();
	static bool is_playing_scene();
	static String get_playing_scene();

	static void set_movie_maker_enabled(bool p_enabled);
	static bool is_movie_maker_enabled();

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	EditorInterface() = delete;
	~EditorInterface() = delete;
};

