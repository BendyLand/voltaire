/**************************************************************************/
/*  editor_zoom_widget.cpp                                                */
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

#include "core/input/input.h"
#include "core/os/keyboard.h"
#include "core/string/translation_server.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_zoom_widget.h"

float EditorZoomWidget::get_zoom() { return zoom; }

void EditorZoomWidget::set_zoom(float p_zoom)
{
	float new_zoom = CLAMP(p_zoom, min_zoom, max_zoom);
	if (zoom != new_zoom) {
		zoom = new_zoom;
		_update_zoom_label();
	}
}

float EditorZoomWidget::get_min_zoom() { return min_zoom; }

float EditorZoomWidget::get_max_zoom() { return max_zoom; }

void EditorZoomWidget::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		zoom_minus->set_button_icon(get_editor_theme_icon(SNAME("ZoomLess")));
		zoom_plus->set_button_icon(get_editor_theme_icon(SNAME("ZoomMore")));
	} break;
	}
}

void EditorZoomWidget::_bind_methods() {}

void EditorZoomWidget::set_shortcut_context(Node* p_node) const
{
	zoom_minus->set_shortcut_context(p_node);
	zoom_plus->set_shortcut_context(p_node);
	zoom_reset->set_shortcut_context(p_node);
}


