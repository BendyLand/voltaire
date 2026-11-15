/**************************************************************************/
/*  tiles_editor_plugin.h                                                 */
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

#include "core/os/semaphore.h"
#include "editor/plugins/editor_plugin.h"
#include "editor/scene/2d/tiles/tile_map_layer_editor.h"
#include "editor/scene/2d/tiles/tile_set_editor.h"

class TilesEditorUtils final
{
public:
	enum SourceSortOption
	{
		SOURCE_SORT_ID = 0,
		SOURCE_SORT_ID_REVERSE,
		SOURCE_SORT_NAME,
		SOURCE_SORT_NAME_REVERSE,
		SOURCE_SORT_MAX
	};

private:
	struct SourceNameComparator
	{
		static Ref<TileSet> tile_set;
		bool operator()(const int& p_a, const int& p_b) const;
	};

	// Patterns preview generation.
	struct QueueItem
	{
		Ref<TileSet> tile_set;
		Ref<TileMapPattern> pattern;
	};

	struct Data
	{
		// For synchronization.
		int atlas_sources_lists_current = 0;
		float atlas_view_zoom = 1.0;
		Vector2 atlas_view_scroll;

		// Source sorting.
		int source_sort = SOURCE_SORT_ID;

		List<QueueItem> pattern_preview_queue;
		Mutex pattern_preview_mutex;
		Semaphore pattern_preview_sem;
		Thread pattern_preview_thread;
		SafeFlag pattern_thread_exit;
		SafeFlag pattern_thread_exited;
		Semaphore pattern_preview_done;
	};

	static inline Data* data = nullptr;

	static void _preview_frame_started();
	static void _pattern_preview_done();
	static void _thread_func(void* ud);
	static void _thread();

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	// To synchronize the atlas sources lists.
	static void set_sources_lists_current(int p_current);

	static void set_atlas_view_transform(float p_zoom, Vector2 p_scroll);

	// Sorting.
	static void set_sorting_option(int p_option);
	static List<int> get_sorted_sources(const Ref<TileSet> p_tile_set);

	// Misc.
	static void display_tile_set_editor_panel();

	TilesEditorUtils() = delete;
	TilesEditorUtils(const TilesEditorUtils&) = delete;
	TilesEditorUtils& operator=(const TilesEditorUtils&) = delete;
	~TilesEditorUtils() = delete;
};

class TileSetSourceItemList : public ItemList
{
public:
	Ref<TileSet> tile_set;

	TileSetSourceItemList();
};

class TileMapEditorPlugin : public EditorPlugin
{
	TileMapLayerEditor* editor = nullptr;

	bool tile_map_changed_needs_update = false;

	void _tile_map_layer_changed();
	void _tile_map_layer_removed();
	void _update_tile_map();
	void _select_layer(const String& p_name);

	void _edit_tile_map_layer(TileMapLayer* p_tile_map_layer, bool p_show_layer_selector);
	void _edit_tile_map(TileMap* p_tile_map);

protected:
	void _notification(int p_notification);

public:
	virtual bool forward_canvas_gui_input(const Ref<InputEvent>& p_event) override;
	virtual void forward_canvas_draw_over_viewport(Control* p_overlay) override;

	bool is_editor_visible() const;

	TileMapEditorPlugin() = default;
	~TileMapEditorPlugin();
};

class TileSetEditorPlugin : public EditorPlugin
{
	TileSetEditor* editor = nullptr;

public:
	TileSetEditorPlugin() = default;
	~TileSetEditorPlugin();
};


