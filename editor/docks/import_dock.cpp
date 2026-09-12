/**************************************************************************/
/*  import_dock.cpp                                                       */
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
#include "core/io/resource_loader.h"
#include "core/templates/mem_unique_ptr.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "import_dock.h"
#include "scene/gui/box_container.h"

class ImportDockParameters
{
public:
	Ref<ResourceImporter> importer;
	Vector<String> paths;
	HashSet<StringName> checked;
	bool checking = false;
	bool skip = false;
	String base_options_path;
};

ImportDock* ImportDock::singleton = nullptr;

void ImportDock::reimport_resources(const Vector<String>& p_paths)
{
	switch (p_paths.size()) {
	case 0:
		ERR_FAIL_MSG("You need to select files to reimport them.");
	case 1:
		set_edit_path(p_paths[0]);
		break;
	default:
		set_edit_multiple_paths(p_paths);
		break;
	}

	_reimport_attempt();
}

static bool _find_owners(EditorFileSystemDirectory* efsd, const String& p_path)
{
	if (!efsd) {
		return false;
	}

	for (int i = 0; i < efsd->get_subdir_count(); i++) {
		if (_find_owners(efsd->get_subdir(i), p_path)) {
			return true;
		}
	}

	for (int i = 0; i < efsd->get_file_count(); i++) {
		Vector<String> deps = efsd->get_file_deps(i);
		if (deps.has(p_path)) {
			return true;
		}
	}

	return false;
}

void ImportDock::_advanced_options()
{
	if (params->paths.size() == 1 && params->importer.is_valid()) {
		params->importer->show_advanced_options(params->paths[0]);
	}
}

void ImportDock::_property_edited(const StringName& p_prop) { _set_dirty(true); }

void ImportDock::_set_dirty(bool p_dirty)
{
	if (p_dirty) {
		// Add a dirty marker to notify the user that they should reimport the selected resource to
		// see changes.
		import->set_text(TTR("Reimport") + " (*)");
		import->add_theme_color_override(SceneStringName(font_color),
			get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));
		import->set_tooltip_text(TTRC(
			"You have pending changes that haven't been applied yet. Click Reimport to apply "
			"changes made to the import options.\nSelecting another resource in the FileSystem "
			"dock without clicking Reimport first will discard changes made in the Import dock."));
	}
	else {
		// Remove the dirty marker on the Reimport button.
		import->set_text(TTRC("Reimport"));
		import->remove_theme_color_override(SceneStringName(font_color));
		import->set_tooltip_text("");
	}
}

void ImportDock::_property_toggled(const StringName& p_prop, bool p_checked)
{
	if (p_checked) {
		params->checked.insert(p_prop);
	}
	else {
		params->checked.erase(p_prop);
	}
}

ImportDock::~ImportDock()
{
	singleton = nullptr;
	memdelete(params);
}


