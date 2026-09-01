/**************************************************************************/
/*  tiles_editor_plugin.cpp                                               */
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

#include "core/os/mutex.h"
#include "core/os/os.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/scene/2d/tiles/tile_set_editor.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/tile_map.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/tile_set.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/rendering_server.h"
#include "tiles_editor_plugin.h"

TilesEditorUtils* TilesEditorUtils::singleton = nullptr;
TileMapEditorPlugin* tile_map_plugin_singleton = nullptr;
TileSetEditorPlugin* tile_set_plugin_singleton = nullptr;

void TilesEditorUtils::_pattern_preview_done() { pattern_preview_done.post(); }

void TilesEditorUtils::_thread_func(void* ud)
{
	TilesEditorUtils* te = static_cast<TilesEditorUtils*>(ud);
	set_current_thread_safe_for_nodes(true);
	te->_thread();
}

void TilesEditorUtils::set_sources_lists_current(int p_current)
{
	atlas_sources_lists_current = p_current;
}

void TilesEditorUtils::set_atlas_view_transform(float p_zoom, Vector2 p_scroll)
{
	atlas_view_zoom = p_zoom;
	atlas_view_scroll = p_scroll;
}

void TilesEditorUtils::set_sorting_option(int p_option) { source_sort = p_option; }

List<int> TilesEditorUtils::get_sorted_sources(const Ref<TileSet> p_tile_set) const
{
	SourceNameComparator::tile_set = p_tile_set;
	List<int> source_ids;

	for (int i = 0; i < p_tile_set->get_source_count(); i++) {
		source_ids.push_back(p_tile_set->get_source_id(i));
	}

	switch (source_sort) {
	case SOURCE_SORT_ID_REVERSE:
		// Already sorted.
		source_ids.reverse();
		break;
	case SOURCE_SORT_NAME:
		source_ids.sort_custom<SourceNameComparator>();
		break;
	case SOURCE_SORT_NAME_REVERSE:
		source_ids.sort_custom<SourceNameComparator>();
		source_ids.reverse();
		break;
	default: // SOURCE_SORT_ID
		break;
	}

	SourceNameComparator::tile_set.unref();
	return source_ids;
}

Ref<TileSet> TilesEditorUtils::SourceNameComparator::tile_set;

void TilesEditorUtils::draw_selection_rect(
	CanvasItem* p_ci, const Rect2& p_rect, const Color& p_color)
{
	Ref<Texture2D> selection_texture = EditorNode::get_singleton()->get_editor_theme()->get_icon(
		SNAME("TileSelection"), EditorStringName(EditorIcons));

	real_t scale = p_ci->get_global_transform().get_scale().x * 0.5;
	p_ci->draw_set_transform(p_rect.position, 0, Vector2(1, 1) / scale);
	RS::get_singleton()->canvas_item_add_nine_patch(p_ci->get_canvas_item(),
		Rect2(Vector2(), p_rect.size * scale), Rect2(), selection_texture->get_rid(), Vector2(2, 2),
		Vector2(2, 2), RSE::NINE_PATCH_STRETCH, RSE::NINE_PATCH_STRETCH, false, p_color);
	p_ci->draw_set_transform_matrix(Transform2D());
}

TilesEditorUtils::TilesEditorUtils()
{
	singleton = this;
	// Pattern preview generation thread.
	pattern_preview_thread.start(_thread_func, this);

	ED_SHORTCUT("tiles_editor/cut", TTRC("Cut"), KeyModifierMask::CMD_OR_CTRL | Key::X);
	ED_SHORTCUT("tiles_editor/copy", TTRC("Copy"), KeyModifierMask::CMD_OR_CTRL | Key::C);
	ED_SHORTCUT("tiles_editor/paste", TTRC("Paste"), KeyModifierMask::CMD_OR_CTRL | Key::V);
	ED_SHORTCUT("tiles_editor/cancel", TTRC("Cancel"), Key::ESCAPE);
	ED_SHORTCUT("tiles_editor/delete", TTRC("Delete"), Key::KEY_DELETE);

	ED_SHORTCUT("tiles_editor/paint_tool", TTRC("Paint Tool"), Key::D);
	ED_SHORTCUT("tiles_editor/line_tool", TTRC("Line Tool"), Key::L);
	ED_SHORTCUT("tiles_editor/rect_tool", TTRC("Rect Tool"), Key::R);
	ED_SHORTCUT("tiles_editor/bucket_tool", TTRC("Bucket Tool"), Key::B);
	ED_SHORTCUT("tiles_editor/eraser", TTRC("Eraser Tool"), Key::E);
	ED_SHORTCUT("tiles_editor/picker", TTRC("Picker Tool"), Key::P);
}

TilesEditorUtils::~TilesEditorUtils()
{
	if (pattern_preview_thread.is_started()) {
		pattern_thread_exit.set();
		pattern_preview_sem.post();
		while (!pattern_thread_exited.is_set()) {
			OS::get_singleton()->delay_usec(10000);
			RenderingServer::get_singleton()
				->sync(); // sync pending stuff, as thread may be blocked on visual server
		}
		pattern_preview_thread.wait_to_finish();
	}
	singleton = nullptr;
}

bool TileMapEditorPlugin::forward_canvas_gui_input(const Ref<InputEvent>& p_event)
{
	return editor->forward_canvas_gui_input(p_event);
}

void TileMapEditorPlugin::forward_canvas_draw_over_viewport(Control* p_overlay)
{
	editor->forward_canvas_draw_over_viewport(p_overlay);
}

bool TileMapEditorPlugin::is_editor_visible() const { return editor->is_visible_in_tree(); }

TileMapEditorPlugin::TileMapEditorPlugin()
{
	if (!TilesEditorUtils::get_singleton()) {
		memnew(TilesEditorUtils);
	}
	tile_map_plugin_singleton = this;

	editor = memnew(TileMapLayerEditor);
	editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	editor->set_custom_minimum_size(Size2(0, 200) * EDSCALE);
	editor->hide();

	EditorDockManager::get_singleton()->add_dock(editor);
	editor->close();
}

TileMapEditorPlugin::~TileMapEditorPlugin() { tile_map_plugin_singleton = nullptr; }

void TileSetEditorPlugin::open_editor() { editor->open(); }

TileSetEditorPlugin::TileSetEditorPlugin()
{
	if (!TilesEditorUtils::get_singleton()) {
		memnew(TilesEditorUtils);
	}
	tile_set_plugin_singleton = this;

	editor = memnew(TileSetEditor);
	editor->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	editor->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	editor->set_custom_minimum_size(Size2(0, 200) * EDSCALE);
	editor->hide();

	EditorDockManager::get_singleton()->add_dock(editor);
	editor->close();
}

TileSetEditorPlugin::~TileSetEditorPlugin() { tile_set_plugin_singleton = nullptr; }


