/**************************************************************************/
/*  editor_run_native.cpp                                                 */
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

#include "editor/editor_node.h"
#include "editor/export/editor_export.h"
#include "editor/export/editor_export_platform.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_run_native.h"

void EditorRunNative::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		remote_debug->set_button_icon(get_editor_theme_icon(SNAME("PlayRemote")));
	} break;

	case NOTIFICATION_PROCESS: {
		bool changed = EditorExport::get_singleton()->poll_export_platforms() || first;

		if (changed) {
			PopupMenu* popup = remote_debug->get_popup();
			popup->clear();
			int device_shortcut_id = 1;
			for (int i = 0; i < EditorExport::get_singleton()->get_export_platform_count(); i++) {
				Ref<EditorExportPlatform> eep =
					EditorExport::get_singleton()->get_export_platform(i);
				Ref<EditorExportPreset> preset =
					EditorExport::get_singleton()->get_runnable_preset_for_platform(eep);
				if (preset.is_null()) {
					continue;
				}
				const int device_count = MIN(eep->get_options_count(), 9000);
				if (device_count > 0) {
					popup->add_icon_item(eep->get_run_icon(), eep->get_name(), -1);
					popup->set_item_disabled(-1, true);
					for (int j = 0; j < device_count; j++) {
						popup->add_icon_item(eep->get_option_icon(j), eep->get_option_label(j),
							EditorExport::encode_platform_device_id(i, j));
						popup->set_item_tooltip(-1, eep->get_option_tooltip(j));
						popup->set_item_indent(-1, 2);
						if (device_shortcut_id <= 4 && eep->is_option_runnable(j)) {
							// Assign shortcuts for the first 4 devices added in the list.
							popup->set_item_shortcut(-1,
								ED_GET_SHORTCUT(vformat(
									"remote_deploy/deploy_to_device_%d", device_shortcut_id)),
								true);
							device_shortcut_id += 1;
						}
					}
				}
			}
			if (popup->get_item_count() == 0) {
				remote_debug->hide();
			}
			else {
				remote_debug->show();
			}

			first = false;
		}
	} break;
	}
}

void EditorRunNative::_confirm_run_native()
{
	run_confirmed = true;
	resume_run_native();
}

void EditorRunNative::resume_run_native() { start_run_native(resume_id); }


