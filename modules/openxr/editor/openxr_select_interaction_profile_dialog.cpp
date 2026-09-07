/**************************************************************************/
/*  openxr_select_interaction_profile_dialog.cpp                          */
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

#include "../action_map/openxr_interaction_profile_metadata.h"
#include "../openxr_api.h"
#include "editor/themes/editor_scale.h"
#include "openxr_select_interaction_profile_dialog.h"

void OpenXRSelectInteractionProfileDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		scroll->add_theme_style_override(SceneStringName(panel),
			get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());
	} break;
	}
}

OpenXRSelectInteractionProfileDialog::OpenXRSelectInteractionProfileDialog()
{
	set_title(TTR("Select an interaction profile"));

	scroll = memnew(ScrollContainer);
	scroll->set_custom_minimum_size(Size2(600.0 * EDSCALE, 400.0 * EDSCALE));
	add_child(scroll);

	main_vb = memnew(VBoxContainer);
	main_vb->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll->add_child(main_vb);

	all_selected = memnew(Label);
	all_selected->set_focus_mode(Control::FOCUS_ACCESSIBILITY);
	all_selected->set_text(TTR("All interaction profiles have been added to the action map."));
	main_vb->add_child(all_selected);
}


