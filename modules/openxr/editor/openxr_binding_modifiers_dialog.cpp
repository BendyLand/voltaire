/**************************************************************************/
/*  openxr_binding_modifiers_dialog.cpp                                   */
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
#include "editor/themes/editor_scale.h"
#include "openxr_action_map_editor.h"
#include "openxr_binding_modifiers_dialog.h"

void OpenXRBindingModifiersDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		_create_binding_modifiers();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		if (binding_modifier_sc) {
			binding_modifier_sc->add_theme_style_override(SceneStringName(panel),
				get_theme_stylebox(SceneStringName(panel), SNAME("Tree")).ptr());
		}
	} break;
	}
}

void OpenXRBindingModifiersDialog::_on_add_binding_modifier()
{
	create_dialog->popup_create(false);
}

void OpenXRBindingModifiersDialog::_do_add_binding_modifier_editor(
	OpenXRBindingModifierEditor* p_binding_modifier_editor)
{
	Ref<OpenXRBindingModifier> binding_modifier = p_binding_modifier_editor->get_binding_modifier();
	ERR_FAIL_COND(binding_modifier.is_null());

	if (ip_binding.is_valid()) {
		// Add it to our binding
		ip_binding->add_binding_modifier(binding_modifier);
	}
	else if (interaction_profile.is_valid()) {
		// Add it to our interaction profile
		interaction_profile->add_binding_modifier(binding_modifier);
	}
	else {
		ERR_FAIL_MSG("No binding nor interaction profile specified.");
	}

	binding_modifiers_vb->add_child(p_binding_modifier_editor);
}

void OpenXRBindingModifiersDialog::_do_remove_binding_modifier_editor(
	OpenXRBindingModifierEditor* p_binding_modifier_editor)
{
	Ref<OpenXRBindingModifier> binding_modifier = p_binding_modifier_editor->get_binding_modifier();
	ERR_FAIL_COND(binding_modifier.is_null());

	if (ip_binding.is_valid()) {
		// Remove it from our binding.
		ip_binding->remove_binding_modifier(binding_modifier);
	}
	else if (interaction_profile.is_valid()) {
		// Removed it to from interaction profile.
		interaction_profile->remove_binding_modifier(binding_modifier);
	}
	else {
		ERR_FAIL_MSG("No binding nor interaction profile specified.");
	}

	binding_modifiers_vb->remove_child(p_binding_modifier_editor);
}

void OpenXRBindingModifiersDialog::setup(const Ref<OpenXRActionMap>& p_action_map,
	const Ref<OpenXRInteractionProfile>& p_interaction_profile,
	const Ref<OpenXRIPBinding>& p_ip_binding)
{
	OpenXRInteractionProfileMetadata* meta_data = OpenXRInteractionProfileMetadata::get_singleton();
	action_map = p_action_map;
	interaction_profile = p_interaction_profile;
	ip_binding = p_ip_binding;

	String profile_path = interaction_profile->get_interaction_profile_path();

	if (ip_binding.is_valid()) {
		String action_name = "unset";
		String path_name = "unset";

		Ref<OpenXRAction> action = p_ip_binding->get_action();
		if (action.is_valid()) {
			action_name = action->get_name_with_set();
		}

		const OpenXRInteractionProfileMetadata::IOPath* io_path =
			meta_data->get_io_path(profile_path, p_ip_binding->get_binding_path());
		if (io_path != nullptr) {
			path_name = io_path->display_name;
		}

		create_dialog->set_base_type("OpenXRActionBindingModifier");
		set_title(TTR("Binding modifiers for:") + " " + action_name + ": " + path_name);
	}
	else if (interaction_profile.is_valid()) {
		String profile_name = profile_path;

		const OpenXRInteractionProfileMetadata::InteractionProfile* profile_def =
			meta_data->get_profile(profile_path);
		if (profile_def != nullptr) {
			profile_name = profile_def->display_name;
		}

		create_dialog->set_base_type("OpenXRIPBindingModifier");
		set_title(TTR("Binding modifiers for:") + " " + profile_name);
	}
}


