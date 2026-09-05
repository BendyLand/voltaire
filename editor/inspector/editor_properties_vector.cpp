/**************************************************************************/
/*  editor_properties_vector.cpp                                          */
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
#include "editor/gui/editor_spin_slider.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_properties_vector.h"
#include "scene/gui/box_container.h"
#include "scene/gui/texture_button.h"

const String EditorPropertyVectorN::COMPONENT_LABELS[4] = {"x", "y", "z", "w"};

void EditorPropertyVectorN::_set_read_only(bool p_read_only)
{
	for (EditorSpinSlider* spin : spin_sliders) {
		spin->set_read_only(p_read_only);
	}
}

void EditorPropertyVectorN::_update_ratio()
{
	linked->set_modulate(Color(1, 1, 1, linked->is_pressed() ? 1.0 : 0.5));

	double* ratio_write = ratio.ptrw();
	for (int i = 0; i < ratio.size(); i++) {
		int base_slider_idx = i / (component_count - 1);
		int secondary_slider_idx =
			((base_slider_idx + 1) + i % (component_count - 1)) % component_count;

		if (spin_sliders[base_slider_idx]->get_value() != 0) {
			ratio_write[i] = spin_sliders[secondary_slider_idx]->get_value() /
							 spin_sliders[base_slider_idx]->get_value();
		}
	}
}

void EditorPropertyVectorN::setup(
	const EditorPropertyRangeHint& p_range_hint, bool p_link, bool p_is_int)
{
	radians_as_degrees = p_range_hint.radians_as_degrees;

	for (EditorSpinSlider* spin : spin_sliders) {
		spin->set_min(p_range_hint.min);
		spin->set_max(p_range_hint.max);
		spin->set_step(p_range_hint.step);
		if (p_range_hint.hide_control) {
			spin->set_control_state(EditorSpinSlider::CONTROL_STATE_HIDE);
		}
		spin->set_allow_greater(true);
		spin->set_allow_lesser(true);
		spin->set_suffix(p_range_hint.suffix);
		spin->set_editing_integer(p_is_int);
	}

	if (!p_link) {
		linked->hide();
	}
}

void EditorPropertyVectorN::set_deferred_drag_mode_enabled(bool p_enabled)
{
	EditorProperty::set_deferred_drag_mode_enabled(p_enabled);

	for (int i = 0; i < component_count; i++) {
		spin_sliders[i]->set_deferred_drag_mode_enabled(p_enabled);
	}
}


