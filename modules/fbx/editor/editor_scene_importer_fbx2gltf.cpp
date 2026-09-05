/**************************************************************************/
/*  editor_scene_importer_fbx2gltf.cpp                                    */
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
#include "core/os/os.h"
#include "editor/settings/editor_settings.h"
#include "editor_scene_importer_fbx2gltf.h"
#include "editor_scene_importer_ufbx.h"
#include "modules/gltf/gltf_document.h"

void EditorSceneFormatImporterFBX2GLTF::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("fbx");
}

#define ADD_OPTION_ENUM(PATH, ENUM_HINT, VALUE)                                                    \
	r_options->push_back(ResourceImporter::ImportOption(                                           \
		PropertyInfo(Variant::INT, SNAME(PATH), PROPERTY_HINT_ENUM, ENUM_HINT), VALUE));

void EditorSceneFormatImporterFBX2GLTF::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
	// This function must be empty to avoid both FBX2glTF and UFBX adding the same options to FBX
	// files.
}


