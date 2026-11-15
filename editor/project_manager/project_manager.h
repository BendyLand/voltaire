/**************************************************************************/
/*  project_manager.h                                                     */
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

#include "core/math/color.h"
#include "core/math/rect2.h"
#include "core/string/ustring.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "scene/resources/font.h"

class ProjectManager
{
private:
	static inline bool _initialized = false;
public:
	ProjectManager() = delete;

	static constexpr int DEFAULT_WINDOW_WIDTH = 1152;
	static constexpr int DEFAULT_WINDOW_HEIGHT = 648;

	struct UIButton
	{
		String label;
		Rect2 rect;
		void (*on_click)() = nullptr;
		bool hovered = false;
	};

	struct Data
	{
		RID viewport_rid;
		RID canvas_rid;
		RID canvas_item_rid;

		Size2i window_size = Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
		String search_text;
		String status_text = "Voltaire Data-Oriented UI Online";

		Ref<Font> ui_font;
		LocalVector<UIButton> buttons;
		bool needs_redraw = true;
	};

	static inline Data* data = nullptr;

	// Lifecycle & Frame Hooks
	static bool is_initialized() { return _initialized; };
	static void initialize(RID p_viewport_rid = RID(),
		Size2i p_size = Size2i(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT));
	static void process_and_draw();
	static void finalize();

	// Button Actions
	static void _new_project();
	static void _import_project();
	static void _scan_projects();
	static void _open_donate_page();
};
