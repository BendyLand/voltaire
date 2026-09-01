/**************************************************************************/
/*  atlas_merging_dialog.cpp                                              */
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

#include "atlas_merging_dialog.h"
#include "core/io/resource_loader.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/control.h"
#include "scene/gui/split_container.h"
#include "scene/resources/image_texture.h"

void AtlasMergingDialog::ok_pressed()
{
	delete_original_atlases = false;
	editor_file_dialog->popup_file_dialog();
}

void AtlasMergingDialog::cancel_pressed()
{
	EditorUndoRedoManager* undo_redo = EditorUndoRedoManager::get_singleton();
	for (int i = 0; i < committed_actions_count; i++) {
		undo_redo->undo();
	}
	committed_actions_count = 0;
}

void AtlasMergingDialog::custom_action(const String& p_action)
{
	if (p_action == "merge") {
		delete_original_atlases = true;
		editor_file_dialog->popup_file_dialog();
	}
}

void AtlasMergingDialog::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible()) {
			_update_texture();
		}
	} break;
	}
}


