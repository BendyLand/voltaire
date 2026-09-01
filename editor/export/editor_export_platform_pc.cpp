/**************************************************************************/
/*  editor_export_platform_pc.cpp                                         */
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"
#include "core/os/shared_object.h"
#include "editor_export_platform_pc.h"
#include "scene/resources/image_texture.h" // IWYU pragma: keep. Misdetection of `logo`.

String EditorExportPlatformPC::get_name() const { return name; }

String EditorExportPlatformPC::get_os_name() const { return os_name; }

Ref<Texture2D> EditorExportPlatformPC::get_logo() const { return logo; }

bool EditorExportPlatformPC::has_valid_project_configuration(
	const Ref<EditorExportPreset>& p_preset, String& r_error) const
{
	return true;
}

Error EditorExportPlatformPC::export_project(const Ref<EditorExportPreset>& p_preset, bool p_debug,
	const String& p_path, uint32_t p_flags, bool p_notify)
{
	ExportNotifier notifier(*this, p_preset, p_debug, p_path, p_flags, p_notify);

	Error err = prepare_template(p_preset, p_debug, p_path, p_flags);
	if (err == OK) {
		err = modify_template(p_preset, p_debug, p_path, p_flags);
	}
	if (err == OK) {
		err = export_project_data(p_preset, p_debug, p_path, p_flags);
	}

	return err;
}

Error EditorExportPlatformPC::sign_shared_object(
	const Ref<EditorExportPreset>& p_preset, bool p_debug, const String& p_path)
{
	return OK;
}

void EditorExportPlatformPC::set_name(const String& p_name) { name = p_name; }

void EditorExportPlatformPC::set_os_name(const String& p_name) { os_name = p_name; }

void EditorExportPlatformPC::set_logo(const Ref<Texture2D>& p_logo) { logo = p_logo; }

void EditorExportPlatformPC::get_platform_features(List<String>* r_features) const
{
	r_features->push_back("pc"); // Identify PC platforms as such.
}

void EditorExportPlatformPC::resolve_platform_feature_priorities(
	const Ref<EditorExportPreset>& p_preset, HashSet<String>& p_features)
{
}

int EditorExportPlatformPC::get_chmod_flags() const { return chmod_flags; }

void EditorExportPlatformPC::set_chmod_flags(int p_flags) { chmod_flags = p_flags; }


