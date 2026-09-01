/**************************************************************************/
/*  import_defaults_editor.cpp                                            */
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

#include "core/config/project_settings.h"
#include "core/io/resource_importer.h"
#include "core/templates/mem_unique_ptr.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/inspector/editor_sectioned_inspector.h"
#include "editor/settings/action_map_editor.h"
#include "import_defaults_editor.h"
#include "scene/gui/center_container.h"
#include "scene/gui/label.h"
#include "scene/gui/margin_container.h"

class ImportDefaultsEditorSettings
{
	friend class ImportDefaultsEditor;

	Ref<ResourceImporter> importer;
};

void ImportDefaultsEditor::_importer_selected(int p_index) { _update_importer(); }

void ImportDefaultsEditor::clear()
{
	String last_selected;

	if (importers->get_selected() >= 0) {
		last_selected = importers->get_item_text(importers->get_selected());
	}

	importers->clear();

	List<Ref<ResourceImporter>> importer_list;
	ResourceFormatImporter::get_singleton()->get_importers(&importer_list);
	Vector<String> names;
	for (const Ref<ResourceImporter>& E : importer_list) {
		String vn = E->get_visible_name();
		names.push_back(vn);
	}
	names.sort();

	// `last_selected.is_empty()` means it's the first time being called.
	if (last_selected.is_empty() && !names.is_empty()) {
		last_selected = names[0];
	}

	for (int i = 0; i < names.size(); i++) {
		importers->add_item(names[i]);

		if (names[i] == last_selected) {
			importers->select(i);
			_update_importer();
		}
	}
}

ImportDefaultsEditor::~ImportDefaultsEditor() { memdelete(settings); }


