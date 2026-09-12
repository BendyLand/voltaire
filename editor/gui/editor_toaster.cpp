/**************************************************************************/
/*  editor_toaster.cpp                                                    */
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

#include "editor/editor_string_names.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_toaster.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"

EditorToaster* EditorToaster::singleton = nullptr;

// This is kind of a workaround because it's hard to keep the VBox anchored to the bottom.
void EditorToaster::_update_vbox_position()
{
	vbox_container->set_size(Vector2());

	Point2 pos = get_global_position();
	Size2 vbox_size = vbox_container->get_size();
	pos.y -= vbox_size.y + 5 * EDSCALE;
	if (!is_layout_rtl()) {
		pos.x = pos.x - vbox_size.x + get_size().x;
	}

	vbox_container->set_position(pos);
}

void EditorToaster::_draw_progress(Control* panel)
{
	if (toasts.has(panel) && toasts[panel].remaining_time > 0 && toasts[panel].duration > 0) {
		Ref<StyleBoxFlat> stylebox;
		switch (toasts[panel].severity) {
		case SEVERITY_INFO:
			stylebox = info_panel_style_progress;
			break;
		case SEVERITY_WARNING:
			stylebox = warning_panel_style_progress;
			break;
		case SEVERITY_ERROR:
			stylebox = error_panel_style_progress;
			break;
		default:
			break;
		}

		Size2 size = panel->get_size();
		Size2 progress = size;
		progress.width *=
			MIN(1, Math::remap(toasts[panel].remaining_time, 0, toasts[panel].duration, 0, 2));
		if (is_layout_rtl()) {
			panel->draw_style_box(stylebox.ptr(), Rect2(size - progress, progress));
		}
		else {
			panel->draw_style_box(stylebox.ptr(), Rect2(Vector2(), progress));
		}
	}
}

void EditorToaster::close(Control* p_control)
{
	ERR_FAIL_COND(!toasts.has(p_control));
	toasts[p_control].remaining_time = -1.0;
	toasts[p_control].popped = false;
}

void EditorToaster::instant_close(Control* p_control)
{
	close(p_control);
	p_control->set_modulate(Color(1, 1, 1, 0));
}

void EditorToaster::copy(Control* p_control)
{
	ERR_FAIL_COND(!toasts.has(p_control));
	DisplayServer::get_singleton()->clipboard_set(toasts[p_control].message);
}

EditorToaster* EditorToaster::get_singleton() { return singleton; }

EditorToaster::~EditorToaster()
{
	singleton = nullptr;
	remove_error_handler(&eh);
}


