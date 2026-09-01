/**************************************************************************/
/*  editor_resource_preview.cpp                                           */
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
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/os.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_paths.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_resource_preview.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "servers/display/display_server.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/rendering_server_globals.h"

void EditorResourcePreviewGenerator::DrawRequester::abort()
{
	if (EditorResourcePreview::get_singleton()->is_threaded()) {
		semaphore.post();
	}
}

void EditorResourcePreviewGenerator::request_draw_and_wait(RID viewport) const
{
	DrawRequester draw_requester;
	draw_requester.request_and_wait(viewport);
}

void EditorResourcePreviewGenerator::DrawRequester::_post_semaphore() { semaphore.post(); }

bool EditorResourcePreview::is_threaded() const
{
	return RSG::rasterizer->can_create_resources_async();
}

void EditorResourcePreview::_thread_func(void* ud)
{
	EditorResourcePreview* erp = (EditorResourcePreview*)ud;
	erp->_thread();
}

void EditorResourcePreview::_thread()
{
	exited.clear();
	while (!exiting.is_set()) {
		preview_sem.wait();
		_iterate();
	}
	exited.set();
}

void EditorResourcePreview::_idle_callback()
{
	if (!singleton) {
		// Just in case the shutdown of the editor involves the deletion of the singleton
		// happening while additional idle callbacks can happen.
		return;
	}

	// Process preview tasks, trying to leave a little bit of responsiveness worst case.
	uint64_t start = OS::get_singleton()->get_ticks_msec();
	while (!singleton->queue.is_empty() && OS::get_singleton()->get_ticks_msec() - start < 100) {
		singleton->_iterate();
	}
}

void EditorResourcePreview::_update_thumbnail_sizes()
{
	if (small_thumbnail_size == -1) {
		// Kind of a workaround to retrieve the default icon size.
		small_thumbnail_size = EditorNode::get_singleton()
								   ->get_editor_theme()
								   ->get_icon(SNAME("Object"), EditorStringName(EditorIcons))
								   ->get_width();
	}
}

EditorResourcePreview::PreviewItem EditorResourcePreview::get_resource_preview_if_available(
	const String& p_path)
{
	PreviewItem item;
	{
		MutexLock lock(preview_mutex);

		HashMap<String, EditorResourcePreview::Item>::Iterator I = cache.find(p_path);
		if (!I) {
			return item;
		}

		EditorResourcePreview::Item& cached_item = I->value;
		item.preview = cached_item.preview;
		item.small_preview = cached_item.small_preview;
	}
	preview_sem.post();
	return item;
}

void EditorResourcePreview::add_preview_generator(
	const Ref<EditorResourcePreviewGenerator>& p_generator)
{
	preview_generators.push_back(p_generator);
}

void EditorResourcePreview::remove_preview_generator(
	const Ref<EditorResourcePreviewGenerator>& p_generator)
{
	preview_generators.erase(p_generator);
}

EditorResourcePreview* EditorResourcePreview::get_singleton() { return singleton; }

void EditorResourcePreview::_bind_methods() {}

void EditorResourcePreview::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_EXIT_TREE: {
		stop();
	} break;
	}
}

EditorResourcePreview::EditorResourcePreview() { singleton = this; }

EditorResourcePreview::~EditorResourcePreview() { stop(); }

bool EditorResourcePreviewGenerator::can_generate_small_preview() const { return false; }

bool EditorResourcePreviewGenerator::generate_small_preview_automatically() const { return false; }

bool EditorResourcePreviewGenerator::handles(const String& p_type) const { return false; }


