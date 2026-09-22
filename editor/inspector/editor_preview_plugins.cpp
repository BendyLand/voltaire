/**************************************************************************/
/*  editor_preview_plugins.cpp                                            */
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
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_preview_plugins.h"
#include "scene/resources/atlas_texture.h"
#include "scene/resources/bit_map.h"
#include "scene/resources/font.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"
#include "servers/audio/audio_stream.h"
#include "servers/rendering/rendering_server.h"

void post_process_preview(Ref<Image> p_image)
{
	if (p_image->get_format() != Image::FORMAT_RGBA8) {
		p_image->convert(Image::FORMAT_RGBA8);
	}

	const int w = p_image->get_width();
	const int h = p_image->get_height();

	const int r = MIN(w, h) / 32;
	const int r2 = r * r;
	Color transparent = Color(0, 0, 0, 0);

	for (int i = 0; i < r; i++) {
		for (int j = 0; j < r; j++) {
			int dx = i - r;
			int dy = j - r;
			if (dx * dx + dy * dy > r2) {
				p_image->set_pixel(i, j, transparent);
				p_image->set_pixel(w - 1 - i, j, transparent);
				p_image->set_pixel(w - 1 - i, h - 1 - j, transparent);
				p_image->set_pixel(i, h - 1 - j, transparent);
			}
			else {
				break;
			}
		}
	}
}

bool EditorTexturePreviewPlugin::handles(const String& p_type) const { return true; }

bool EditorTexturePreviewPlugin::generate_small_preview_automatically() const { return true; }

////////////////////////////////////////////////////////////////////////////

bool EditorImagePreviewPlugin::handles(const String& p_type) const { return p_type == "Image"; }

bool EditorImagePreviewPlugin::generate_small_preview_automatically() const { return true; }

////////////////////////////////////////////////////////////////////////////

bool EditorBitmapPreviewPlugin::handles(const String& p_type) const {}

bool EditorBitmapPreviewPlugin::generate_small_preview_automatically() const { return true; }

///////////////////////////////////////////////////////////////////////////

bool EditorPackedScenePreviewPlugin::handles(const String& p_type) const { return true; }

//////////////////////////////////////////////////////////////////

void EditorMaterialPreviewPlugin::abort() { draw_requester.abort(); }

bool EditorMaterialPreviewPlugin::handles(const String& p_type) const { return true; }

bool EditorMaterialPreviewPlugin::generate_small_preview_automatically() const { return true; }

EditorMaterialPreviewPlugin::~EditorMaterialPreviewPlugin()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(sphere);
	RS::get_singleton()->free_rid(sphere_instance);
	RS::get_singleton()->free_rid(viewport);
	RS::get_singleton()->free_rid(light);
	RS::get_singleton()->free_rid(light_instance);
	RS::get_singleton()->free_rid(light2);
	RS::get_singleton()->free_rid(light_instance2);
	RS::get_singleton()->free_rid(camera);
	RS::get_singleton()->free_rid(camera_attributes);
	RS::get_singleton()->free_rid(scenario);
}

///////////////////////////////////////////////////////////////////////////

bool EditorScriptPreviewPlugin::handles(const String& p_type) const { return true; }

///////////////////////////////////////////////////////////////////

bool EditorAudioStreamPreviewPlugin::handles(const String& p_type) const { return true; }

///////////////////////////////////////////////////////////////////////////

void EditorMeshPreviewPlugin::abort() { draw_requester.abort(); }

bool EditorMeshPreviewPlugin::handles(const String& p_type) const { return true; }

EditorMeshPreviewPlugin::~EditorMeshPreviewPlugin()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	// RS::get_singleton()->free(sphere);
	RS::get_singleton()->free_rid(mesh_instance);
	RS::get_singleton()->free_rid(viewport);
	RS::get_singleton()->free_rid(light);
	RS::get_singleton()->free_rid(light_instance);
	RS::get_singleton()->free_rid(light2);
	RS::get_singleton()->free_rid(light_instance2);
	RS::get_singleton()->free_rid(camera);
	RS::get_singleton()->free_rid(camera_attributes);
	RS::get_singleton()->free_rid(scenario);
}

///////////////////////////////////////////////////////////////////////////

void EditorFontPreviewPlugin::abort() { draw_requester.abort(); }

bool EditorFontPreviewPlugin::handles(const String& p_type) const { return true; }

EditorFontPreviewPlugin::EditorFontPreviewPlugin()
{
	viewport = RS::get_singleton()->viewport_create();
	RS::get_singleton()->viewport_set_update_mode(viewport, RSE::VIEWPORT_UPDATE_DISABLED);
	RS::get_singleton()->viewport_set_size(viewport, 128, 128);
	RS::get_singleton()->viewport_set_active(viewport, true);
	viewport_texture = RS::get_singleton()->viewport_get_texture(viewport);

	canvas = RS::get_singleton()->canvas_create();
	canvas_item = RS::get_singleton()->canvas_item_create();

	RS::get_singleton()->viewport_attach_canvas(viewport, canvas);
	RS::get_singleton()->canvas_item_set_parent(canvas_item, canvas);
}

EditorFontPreviewPlugin::~EditorFontPreviewPlugin()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(canvas_item);
	RS::get_singleton()->free_rid(canvas);
	RS::get_singleton()->free_rid(viewport);
}

////////////////////////////////////////////////////////////////////////////

static const real_t GRADIENT_PREVIEW_TEXTURE_SCALE_FACTOR = 4.0;

bool EditorGradientPreviewPlugin::handles(const String& p_type) const { return true; }

bool EditorGradientPreviewPlugin::generate_small_preview_automatically() const { return true; }


