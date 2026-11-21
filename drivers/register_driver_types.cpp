/**************************************************************************/
/*  register_driver_types.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#include "register_driver_types.h"

#include "core/io/resource_saver.h"
#include "drivers/png/image_loader_png.h"

#ifdef ACCESSKIT_ENABLED
#include "drivers/accesskit/accessibility_server_accesskit.h"
#endif

static Ref<ImageLoaderPNG> image_loader_png;
static Ref<ResourceSaverPNG> resource_saver_png;

void register_core_driver_types() {
#ifdef ACCESSKIT_ENABLED
	AccessibilityServerAccessKit::register_create_func();
#endif

	image_loader_png.instantiate();
	ImageLoader::add_image_format_loader(image_loader_png);

	resource_saver_png.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_png);
}

void unregister_core_driver_types() {
	ImageLoader::remove_image_format_loader(image_loader_png);
	image_loader_png.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_png);
	resource_saver_png.unref();
}

void register_driver_types() {
}

void unregister_driver_types() {
}

// ============================================================================
// Merged from drivers/accesskit/accessibility_server_accesskit.cpp
// ============================================================================

#ifdef ACCESSKIT_ENABLED

#include "drivers/accesskit/accessibility_server_accesskit.h"
#include "servers/text/text_server.h"

_FORCE_INLINE_ accesskit_role AccessibilityServerAccessKit::_accessibility_role(
	AccessibilityServerEnums::AccessibilityRole p_role) const
{
	if (role_map.has(p_role)) {
		return role_map[p_role];
	}
	return ACCESSKIT_ROLE_UNKNOWN;
}

_FORCE_INLINE_ accesskit_action AccessibilityServerAccessKit::_accessibility_action(
	AccessibilityServerEnums::AccessibilityAction p_action) const
{
	if (action_map.has(p_action)) {
		return action_map[p_action];
	}
	return ACCESSKIT_ACTION_CLICK;
}

bool AccessibilityServerAccessKit::window_create(
	DisplayServerEnums::WindowID p_window_id, void* p_handle)
{
	ERR_FAIL_COND_V(windows.has(p_window_id), false);

	WindowData& wd = windows[p_window_id];

	AccessibilityElement* ae = memnew(AccessibilityElement);
	ae->role = ACCESSKIT_ROLE_WINDOW;
	ae->window_id = p_window_id;
	wd.root_id = rid_owner.make_rid(ae);

#ifdef WINDOWS_ENABLED
	wd.adapter = accesskit_windows_subclassing_adapter_new(static_cast<HWND>(p_handle),
		&_accessibility_initial_tree_update_callback, (void*)(size_t)p_window_id,
		&_accessibility_action_callback, (void*)(size_t)p_window_id);
#endif
#ifdef MACOS_ENABLED
	wd.adapter = accesskit_macos_subclassing_adapter_for_window(p_handle,
		&_accessibility_initial_tree_update_callback, (void*)(size_t)p_window_id,
		&_accessibility_action_callback, (void*)(size_t)p_window_id);
#endif
#ifdef LINUXBSD_ENABLED
	wd.adapter = accesskit_unix_adapter_new(&_accessibility_initial_tree_update_callback,
		(void*)(size_t)p_window_id, &_accessibility_action_callback, (void*)(size_t)p_window_id,
		&_accessibility_deactivation_callback, (void*)(size_t)p_window_id);
#endif
	print_verbose(vformat("Accessibility: window %d adapter created.", p_window_id));

	if (wd.adapter == nullptr) {
		memdelete(ae);
		rid_owner.free(wd.root_id);
		windows.erase(p_window_id);

		return false;
	}
	else {
		return true;
	}
}

void AccessibilityServerAccessKit::window_destroy(DisplayServerEnums::WindowID p_window_id)
{
	WindowData* wd = windows.getptr(p_window_id);
	if (!wd) {
		return;
	}

	print_verbose(vformat("Accessibility: window %d adapter destroyed.", p_window_id));

#ifdef WINDOWS_ENABLED
	accesskit_windows_subclassing_adapter_free(wd->adapter);
#endif
#ifdef MACOS_ENABLED
	accesskit_macos_subclassing_adapter_free(wd->adapter);
#endif
#ifdef LINUXBSD_ENABLED
	accesskit_unix_adapter_free(wd->adapter);
#endif
	free_element(wd->root_id);

	windows.erase(p_window_id);
}

void AccessibilityServerAccessKit::_accessibility_deactivation_callback(void* p_user_data)
{
	AccessibilityServerAccessKit* self =
		AccessibilityServer::driver_as<AccessibilityServerAccessKit>();
	ERR_FAIL_NULL(self);

	DisplayServerEnums::WindowID window_id = (DisplayServerEnums::WindowID)(size_t)p_user_data;
	WindowData* wd = self->windows.getptr(window_id);
	ERR_FAIL_NULL(wd);

	print_verbose(vformat("Accessibility: window %d adapter deactivated.", window_id));

	if (self->focus.is_valid()) {
		AccessibilityElement* ae = self->rid_owner.get_or_null(self->focus);
		if (ae && ae->window_id == window_id) {
			self->focus = RID();
		}
	}

	wd->activated = false;
	wd->update.clear();
}

void AccessibilityServerAccessKit::_accessibility_action_callback(
	struct accesskit_action_request* p_request, void* p_user_data)
{
	AccessibilityServerAccessKit* self =
		AccessibilityServer::driver_as<AccessibilityServerAccessKit>();
	ERR_FAIL_NULL(self);

	DisplayServerEnums::WindowID window_id = (DisplayServerEnums::WindowID)(size_t)p_user_data;
	ERR_FAIL_COND(!self->windows.has(window_id));

	RID rid = RID::from_uint64(p_request->target_node);
	AccessibilityElement* ae = self->rid_owner.get_or_null(rid);
	ERR_FAIL_NULL(ae);
}

accesskit_tree_update* AccessibilityServerAccessKit::_accessibility_initial_tree_update_callback(
	void* p_user_data)
{
	AccessibilityServerAccessKit* self =
		AccessibilityServer::driver_as<AccessibilityServerAccessKit>();
	ERR_FAIL_NULL_V(self, nullptr);

	DisplayServerEnums::WindowID window_id = (DisplayServerEnums::WindowID)(size_t)p_user_data;
	WindowData* wd = self->windows.getptr(window_id);
	ERR_FAIL_NULL_V(wd, nullptr);

	accesskit_node* win_node = accesskit_node_new(ACCESSKIT_ROLE_WINDOW);
	accesskit_node_set_label(win_node, "Godot Engine");
	accesskit_node_set_busy(win_node);

	accesskit_node_id win_id = (accesskit_node_id)wd->root_id.get_id();

	accesskit_tree_update* tree_update = accesskit_tree_update_with_capacity_and_focus(1, win_id);

	accesskit_tree_update_set_tree(tree_update, accesskit_tree_new(win_id));
	accesskit_tree_update_push_node(tree_update, win_id, win_node);

	print_verbose(vformat("Accessibility: window %d adapter activated.", window_id));

	wd->activated = true;

	return tree_update;
}

void AccessibilityServerAccessKit::window_activation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	WindowData* wd = windows.getptr(p_window_id);
	if (!wd) {
		return;
	}

	print_verbose(
		vformat("Accessibility: window %d adapter initial update completed.", p_window_id));

	wd->initial_update_completed = true;
}

void AccessibilityServerAccessKit::window_deactivation_completed(
	DisplayServerEnums::WindowID p_window_id)
{
	WindowData* wd = windows.getptr(p_window_id);
	if (!wd) {
		return;
	}

	print_verbose(vformat("Accessibility: window %d adapter deactivation completed.", p_window_id));

#ifdef DEV_ENABLED
	LocalVector<RID> to_delete;
	for (const RID& rid : rid_owner.get_owned_list()) {
		AccessibilityElement* ae = rid_owner.get_or_null(rid);
		if (rid != wd->root_id && ae && ae->window_id == p_window_id) {
			ERR_PRINT(vformat("Accessibility/BUG: Accessibility element %d was not deleted on "
							  "window %d adapter deactivation.",
				rid.get_id(), p_window_id));
			to_delete.push_back(rid);
		}
	}
	for (const RID& rid : to_delete) {
		_free_recursive(wd, rid);
	}
#endif
	wd->initial_update_completed = false;
}

RID AccessibilityServerAccessKit::create_element(
	DisplayServerEnums::WindowID p_window_id, AccessibilityServerEnums::AccessibilityRole p_role)
{
	AccessibilityElement* ae = memnew(AccessibilityElement);
	ae->role = _accessibility_role(p_role);
	ae->window_id = p_window_id;
	RID rid = rid_owner.make_rid(ae);

	return rid;
}

RID AccessibilityServerAccessKit::create_sub_element(
	const RID& p_parent_rid, AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos)
{
	AccessibilityElement* parent_ae = rid_owner.get_or_null(p_parent_rid);
	ERR_FAIL_NULL_V(parent_ae, RID());

	WindowData* wd = windows.getptr(parent_ae->window_id);
	ERR_FAIL_NULL_V(wd, RID());

	AccessibilityElement* ae = memnew(AccessibilityElement);
	ae->role = _accessibility_role(p_role);
	ae->window_id = parent_ae->window_id;
	ae->parent = p_parent_rid;
	ae->node = accesskit_node_new(ae->role);
	RID rid = rid_owner.make_rid(ae);
	if (p_insert_pos == -1) {
		parent_ae->children.push_back(rid);
	}
	else {
		parent_ae->children.insert(p_insert_pos, rid);
	}
	wd->update.insert(rid);

	return rid;
}

RID AccessibilityServerAccessKit::create_sub_text_edit_elements(const RID& p_parent_rid,
	const RID& p_shaped_text, float p_min_height, int p_insert_pos, bool p_is_last_line)
{
	AccessibilityElement* parent_ae = rid_owner.get_or_null(p_parent_rid);
	ERR_FAIL_NULL_V(parent_ae, RID());

	WindowData* wd = windows.getptr(parent_ae->window_id);
	ERR_FAIL_NULL_V(wd, RID());

	AccessibilityElement* root_ae = memnew(AccessibilityElement);
	root_ae->role = ACCESSKIT_ROLE_GENERIC_CONTAINER;
	root_ae->window_id = parent_ae->window_id;
	root_ae->parent = p_parent_rid;
	root_ae->node = accesskit_node_new(root_ae->role);
	RID root_rid = rid_owner.make_rid(root_ae);
	if (p_insert_pos == -1) {
		parent_ae->children.push_back(root_rid);
	}
	else {
		parent_ae->children.insert(p_insert_pos, root_rid);
	}
	wd->update.insert(root_rid);

	float text_width = 0;
	float text_height = p_min_height;
	Vector<int32_t> words;
	int64_t run_count = 0; // Note: runs in visual order.
	const Glyph* gl = nullptr;
	float run_off_x = 0.0;
	Vector2i full_range;

	if (p_shaped_text.is_valid()) {
		text_width = TS->shaped_text_get_size(p_shaped_text).x;
		text_height = MAX(text_height, TS->shaped_text_get_size(p_shaped_text).y);
		words = TS->shaped_text_get_word_breaks(p_shaped_text);
		run_count = TS->shaped_get_run_count(p_shaped_text);
		gl = TS->shaped_text_get_glyphs(p_shaped_text);
		full_range = TS->shaped_text_get_range(p_shaped_text);
	}

	accesskit_rect root_rect;
	root_rect.x0 = 0;
	root_rect.y0 = 0;
	root_rect.x1 = text_width;
	root_rect.y1 = MAX(p_min_height, text_height);
	accesskit_node_set_bounds(root_ae->node, root_rect);

	// Create text element for each run.
	Vector<AccessibilityElement*> text_elements;
	for (int64_t i = 0; i < run_count; i++) {
		const Vector2i range = TS->shaped_get_run_range(p_shaped_text, i);
		const Vector2i gl_range = TS->shaped_get_run_glyph_range(p_shaped_text, i);
		String run_t = TS->shaped_get_run_text(p_shaped_text, i);
		TextServer::Direction dir = TS->shaped_get_run_direction(p_shaped_text, i);

		if (run_t.is_empty()) {
			continue;
		}

		// Split long runs in to < 254 char subruns due to AccessKit limitation.
		Vector<Vector2i> subrun_ranges;
		Vector<Vector<uint8_t>> subrun_word_starts;
		{
			Vector2i cur_range = range;
			bool slice = false;
			do {
				// Word starts.
				Vector<uint8_t> word_starts;
				word_starts.push_back(0);

				slice = false;
				for (int j = 0; j < words.size(); j += 2) {
					if (words[j] <= cur_range.x) {
						continue;
					}
					if (words[j] >= cur_range.y) {
						break;
					}
					int32_t wstart = words[j] - cur_range.x;
					int32_t wend = words[j + 1] - cur_range.x;
					if (wend >= 254) {
						cur_range.y = words[j];
						slice = true;
						break;
					}
					word_starts.push_back(wstart);
				}
				subrun_ranges.push_back(cur_range);
				subrun_word_starts.push_back(word_starts);

				if (slice) {
					cur_range.x = cur_range.y;
					cur_range.y = range.y;
				}
			} while (slice);
		}

		// Process subruns.
		int start, end, delta;
		if (dir == TextServer::DIRECTION_LTR) {
			start = 0;
			end = subrun_ranges.size();
			delta = +1;
		}
		else {
			start = subrun_ranges.size() - 1;
			end = -1;
			delta = -1;
		}
		for (int rr = start; rr != end; rr += delta) {
			// Word starts.
			const Vector<uint8_t>& word_starts = subrun_word_starts[rr];
			const Vector2i& cur_range = subrun_ranges[rr];

			AccessibilityElement* ae = memnew(AccessibilityElement);
			ae->role = ACCESSKIT_ROLE_TEXT_RUN;
			ae->window_id = parent_ae->window_id;
			ae->parent = root_rid;
			ae->run = Vector3i(cur_range.x, cur_range.y, i);
			ae->node = accesskit_node_new(ae->role);

			text_elements.push_back(ae);

			String sub_t = run_t.substr(cur_range.x - range.x, cur_range.y - cur_range.x);
			if (sub_t.is_empty()) {
				continue;
			}

			// UTF-8 text and char lengths.
			Vector<uint8_t> char_lengths;
			CharString text = sub_t.utf8(&char_lengths);

			accesskit_node_set_value(ae->node, text.ptr());
			accesskit_node_set_character_lengths(ae->node, char_lengths.size(), char_lengths.ptr());

			accesskit_node_set_word_starts(ae->node, word_starts.size(), word_starts.ptr());

			// Char widths and positions.
			Vector<float> char_positions;
			Vector<float> char_widths;

			char_positions.resize_initialized(sub_t.length());
			float* positions_ptr = char_positions.ptrw();

			char_widths.resize_initialized(sub_t.length());
			float* widths_ptr = char_widths.ptrw();

			float size_x = 0.0;
			for (int j = gl_range.x; j <= gl_range.y; j += gl[j].count) {
				if (gl[j].start >= ae->run.y || gl[j].start < ae->run.x) {
					continue;
				}

				float advance = 0.0; // Graphame advance.
				for (int k = 0; k < gl[j].count; k++) {
					advance += gl[j + k].advance;
				}
				int chars = gl[j].end - gl[j].start;
				float adv_per_char = advance / (float)chars;

				for (int k = 0; k < chars; k++) {
					int index = gl[j].start + k - ae->run.x;
					ERR_CONTINUE(index < 0 || index >= sub_t.length());
					positions_ptr[index] = size_x + adv_per_char * k;
					widths_ptr[index] = adv_per_char;
				}
				size_x += advance * gl[j].repeat;
			}
			positions_ptr[sub_t.length() - 1] = size_x;
			widths_ptr[sub_t.length() - 1] = 1.0;

			accesskit_node_set_character_positions(
				ae->node, char_positions.size(), char_positions.ptr());
			accesskit_node_set_character_widths(ae->node, char_widths.size(), char_widths.ptr());

			RID font_rid = TS->shaped_get_run_font_rid(p_shaped_text, i);
			if (font_rid != RID()) {
				CharString font_name = TS->font_get_name(font_rid).utf8();
				if (font_name.length() > 0) {
					accesskit_node_set_font_family(ae->node, font_name.ptr());
				}
				if ((TS->font_get_style(font_rid) & (TextServer::FONT_ITALIC)) != 0) {
					accesskit_node_set_italic(ae->node);
				}
				accesskit_node_set_font_weight(ae->node, TS->font_get_weight(font_rid));
			}
			accesskit_node_set_font_size(ae->node, TS->shaped_get_run_font_size(p_shaped_text, i));
			CharString language = TS->shaped_get_run_language(p_shaped_text, i).utf8();
			if (language.length() > 0) {
				accesskit_node_set_language(ae->node, language.ptr());
			}
			accesskit_node_set_text_direction(ae->node, ACCESSKIT_TEXT_DIRECTION_LEFT_TO_RIGHT);

			accesskit_rect rect;
			rect.x0 = run_off_x;
			rect.y0 = 0;
			rect.x1 = run_off_x + size_x;
			rect.y1 = text_height;
			accesskit_node_set_bounds(ae->node, rect);
			accesskit_node_add_action(ae->node, ACCESSKIT_ACTION_SCROLL_INTO_VIEW);

			run_off_x += size_x;
		}
	}
	if (!p_is_last_line || text_elements.is_empty()) {
		// Add "\n" at the end.
		AccessibilityElement* ae = memnew(AccessibilityElement);
		ae->role = ACCESSKIT_ROLE_TEXT_RUN;
		ae->window_id = parent_ae->window_id;
		ae->parent = root_rid;
		ae->run = Vector3i(full_range.y, full_range.y, run_count);
		ae->node = accesskit_node_new(ae->role);

		text_elements.push_back(ae);

		if (!p_is_last_line) {
			accesskit_node_set_value(ae->node, "\n");

			Vector<uint8_t> char_lengths;
			Vector<float> char_positions;
			Vector<float> char_widths;
			char_lengths.push_back(1);
			char_positions.push_back(0.0);
			char_widths.push_back(1.0);

			accesskit_node_set_character_lengths(ae->node, char_lengths.size(), char_lengths.ptr());
			accesskit_node_set_character_positions(
				ae->node, char_positions.size(), char_positions.ptr());
			accesskit_node_set_character_widths(ae->node, char_widths.size(), char_widths.ptr());
		}
		else {
			accesskit_node_set_value(ae->node, "");
		}
		accesskit_node_set_text_direction(ae->node, ACCESSKIT_TEXT_DIRECTION_LEFT_TO_RIGHT);

		accesskit_rect rect;
		rect.x0 = run_off_x;
		rect.y0 = 0;
		rect.x1 = run_off_x + 1;
		rect.y1 = text_height;
		accesskit_node_set_bounds(ae->node, rect);
	}

	// Sort runs in logical order.
	struct RunCompare
	{
		_FORCE_INLINE_ bool operator()(
			const AccessibilityElement* l, const AccessibilityElement* r) const
		{
			return l->run.x < r->run.x;
		}
	};

	text_elements.sort_custom<RunCompare>();
	for (int i = 0; i < text_elements.size(); i++) {
		RID rid = rid_owner.make_rid(text_elements[i]);
		root_ae->children.push_back(rid);
		wd->update.insert(rid);

		// Link adjacent TextRuns on the same line.
		if (i > 0) {
			RID prev_rid = root_ae->children[i - 1];
			AccessibilityElement* prev_ae = rid_owner.get_or_null(prev_rid);
			accesskit_node_set_previous_on_line(
				text_elements[i]->node, (accesskit_node_id)prev_rid.get_id());
			accesskit_node_set_next_on_line(prev_ae->node, (accesskit_node_id)rid.get_id());
		}
	}

	return root_rid;
}

bool AccessibilityServerAccessKit::has_element(const RID& p_id) const
{
	return rid_owner.owns(p_id);
}

void AccessibilityServerAccessKit::_free_recursive(WindowData* p_wd, const RID& p_id)
{
	if (p_wd && p_wd->update.has(p_id)) {
		p_wd->update.erase(p_id);
	}
	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	for (const RID& rid : ae->children) {
		_free_recursive(p_wd, rid);
	}
	if (ae->node) {
		accesskit_node_free(ae->node);
	}
	memdelete(ae);
	rid_owner.free(p_id);
}

void AccessibilityServerAccessKit::free_element(const RID& p_id)
{
	ERR_FAIL_COND_MSG(in_accessibility_update,
		"Element can't be removed inside NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	if (ae) {
		WindowData* wd = windows.getptr(ae->window_id);
		AccessibilityElement* parent_ae = rid_owner.get_or_null(ae->parent);
		if (parent_ae) {
			parent_ae->children.erase(p_id);
		}
		_free_recursive(wd, p_id);
	}
}

void AccessibilityServerAccessKit::update_set_focus(const RID& p_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	if (p_id.is_valid() && rid_owner.owns(p_id)) {
		focus = p_id;
	}
	else {
		focus = RID();
	}
}

RID AccessibilityServerAccessKit::get_window_root(DisplayServerEnums::WindowID p_window_id) const
{
	const WindowData* wd = windows.getptr(p_window_id);
	ERR_FAIL_NULL_V(wd, RID());

	return wd->root_id;
}

accesskit_tree_update* AccessibilityServerAccessKit::_accessibility_build_tree_update(
	void* p_user_data)
{
	AccessibilityServerAccessKit* self =
		AccessibilityServer::driver_as<AccessibilityServerAccessKit>();
	ERR_FAIL_NULL_V(self, nullptr);

	DisplayServerEnums::WindowID window_id = (DisplayServerEnums::WindowID)(size_t)p_user_data;

	ERR_FAIL_COND_V(!self->windows.has(window_id), nullptr);
	WindowData& wd = self->windows[window_id];

	self->in_accessibility_update = false;

	AccessibilityElement* focus_ae = self->rid_owner.get_or_null(self->focus);
	uint32_t update_size = wd.update.size();

	accesskit_node_id ac_focus = (accesskit_node_id)wd.root_id.get_id();
	if (focus_ae && focus_ae->window_id == window_id) {
		ac_focus = (accesskit_node_id)self->focus.get_id();
	}
	accesskit_tree_update* tree_update =
		(update_size > 0) ? accesskit_tree_update_with_capacity_and_focus(update_size, ac_focus)
						  : accesskit_tree_update_with_focus(ac_focus);
	for (const RID& rid : wd.update) {
		AccessibilityElement* ae = self->rid_owner.get_or_null(rid);
		if (ae && ae->node) {
			for (const RID& child_rid : ae->children) {
				accesskit_node_push_child(ae->node, (accesskit_node_id)child_rid.get_id());
			}
			accesskit_tree_update_push_node(tree_update, (accesskit_node_id)rid.get_id(), ae->node);
			ae->node = nullptr;
		}
	}
	wd.update.clear();

	return tree_update;
}

_FORCE_INLINE_ void AccessibilityServerAccessKit::_ensure_node(
	const RID& p_id, AccessibilityElement* p_ae)
{
	if (unlikely(!p_ae->node)) {
		WindowData* wd = windows.getptr(p_ae->window_id);
		ERR_FAIL_NULL(wd);

		wd->update.insert(p_id);
		p_ae->node = accesskit_node_new(p_ae->role);

		// Re-apply stored name if any, so nodes recreated by _ensure_node
		// retain their label even if the caller doesn't re-set all properties.
		String full_name = (p_ae->name + " " + p_ae->name_extra_info).strip_edges();
		if (!full_name.is_empty()) {
			accesskit_node_set_label(p_ae->node, full_name.utf8().ptr());
		}
	}
}

void AccessibilityServerAccessKit::set_window_rect(
	DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out, const Rect2& p_rect_in)
{
#ifdef LINUXBSD_ENABLED
	const WindowData* wd = windows.getptr(p_window_id);
	ERR_FAIL_NULL(wd);

	accesskit_rect outer_bounds = {p_rect_out.position.x, p_rect_out.position.y,
		p_rect_out.position.x + p_rect_out.size.width,
		p_rect_out.position.y + p_rect_out.size.height};
	accesskit_rect inner_bounds = {p_rect_in.position.x, p_rect_in.position.y,
		p_rect_in.position.x + p_rect_in.size.width, p_rect_in.position.y + p_rect_in.size.height};
	accesskit_unix_adapter_set_root_window_bounds(wd->adapter, outer_bounds, inner_bounds);
#endif
}

void AccessibilityServerAccessKit::set_window_focused(
	DisplayServerEnums::WindowID p_window_id, bool p_focused)
{
	const WindowData* wd = windows.getptr(p_window_id);
	ERR_FAIL_NULL(wd);

#ifdef LINUXBSD_ENABLED
	accesskit_unix_adapter_update_window_focus_state(wd->adapter, p_focused);
#endif
#ifdef MACOS_ENABLED
	accesskit_macos_queued_events* events =
		accesskit_macos_subclassing_adapter_update_view_focus_state(wd->adapter, p_focused);
	if (events != nullptr) {
		accesskit_macos_queued_events_raise(events);
	}
#endif
	// Note: On Windows, the subclassing adapter takes care of this.
}

void AccessibilityServerAccessKit::update_set_role(
	const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	if (ae->role == _accessibility_role(p_role)) {
		return;
	}
	ae->role = _accessibility_role(p_role);
	_ensure_node(p_id, ae);

	accesskit_node_set_role(ae->node, ae->role);
}

void AccessibilityServerAccessKit::update_set_name(const RID& p_id, const String& p_name)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	ae->name = p_name;
	String full_name = (ae->name + " " + ae->name_extra_info).strip_edges();
	if (!full_name.is_empty()) {
		accesskit_node_set_label(ae->node, full_name.utf8().ptr());
	}
	else {
		accesskit_node_clear_label(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_braille_label(const RID& p_id, const String& p_name)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_name.is_empty()) {
		accesskit_node_set_braille_label(ae->node, p_name.utf8().ptr());
	}
	else {
		accesskit_node_clear_braille_label(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_braille_role_description(
	const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_description.is_empty()) {
		accesskit_node_set_braille_role_description(ae->node, p_description.utf8().ptr());
	}
	else {
		accesskit_node_clear_braille_role_description(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_extra_info(
	const RID& p_id, const String& p_name_extra_info)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	ae->name_extra_info = p_name_extra_info;
	String full_name = (ae->name + " " + ae->name_extra_info).strip_edges();
	if (!full_name.is_empty()) {
		accesskit_node_set_label(ae->node, full_name.utf8().ptr());
	}
	else {
		accesskit_node_clear_label(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_description(
	const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_description.is_empty()) {
		accesskit_node_set_description(ae->node, p_description.utf8().ptr());
	}
	else {
		accesskit_node_clear_description(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_value(const RID& p_id, const String& p_value)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_value.is_empty()) {
		Vector<uint8_t> ch_length;
		accesskit_node_set_value(ae->node, p_value.utf8(&ch_length).ptr());
		accesskit_node_set_character_lengths(ae->node, ch_length.size(), ch_length.ptr());
	}
	else {
		accesskit_node_clear_value(ae->node);
		accesskit_node_clear_character_lengths(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_tooltip(const RID& p_id, const String& p_tooltip)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_tooltip.is_empty()) {
		accesskit_node_set_tooltip(ae->node, p_tooltip.utf8().ptr());
	}
	else {
		accesskit_node_clear_tooltip(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_bounds(const RID& p_id, const Rect2& p_rect)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_rect rect;
	rect.x0 = p_rect.position.x;
	rect.y0 = p_rect.position.y;
	rect.x1 = p_rect.position.x + p_rect.size.x;
	rect.y1 = p_rect.position.y + p_rect.size.y;
	accesskit_node_set_bounds(ae->node, rect);
}

void AccessibilityServerAccessKit::update_set_transform(
	const RID& p_id, const Transform2D& p_transform)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_affine transform = {p_transform.columns[0][0], p_transform.columns[0][1],
		p_transform.columns[1][0], p_transform.columns[1][1], p_transform.columns[2][0],
		p_transform.columns[2][1]};
	accesskit_node_set_transform(ae->node, transform);
}

void AccessibilityServerAccessKit::update_add_child(const RID& p_id, const RID& p_child_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_child_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_child(ae->node, (accesskit_node_id)p_child_id.get_id());
}

void AccessibilityServerAccessKit::update_add_related_controls(
	const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_related_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_controlled(ae->node, (accesskit_node_id)p_related_id.get_id());
}

void AccessibilityServerAccessKit::update_add_related_details(
	const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_related_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_detail(ae->node, (accesskit_node_id)p_related_id.get_id());
}

void AccessibilityServerAccessKit::update_add_related_described_by(
	const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_related_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_described_by(ae->node, (accesskit_node_id)p_related_id.get_id());
}

void AccessibilityServerAccessKit::update_add_related_flow_to(
	const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_related_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_flow_to(ae->node, (accesskit_node_id)p_related_id.get_id());
}

void AccessibilityServerAccessKit::update_add_related_labeled_by(
	const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_related_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_labelled_by(ae->node, (accesskit_node_id)p_related_id.get_id());
}

void AccessibilityServerAccessKit::update_add_related_radio_group(
	const RID& p_id, const RID& p_related_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_related_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_push_to_radio_group(ae->node, (accesskit_node_id)p_related_id.get_id());
}

void AccessibilityServerAccessKit::update_set_active_descendant(
	const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_other_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_set_active_descendant(ae->node, (accesskit_node_id)p_other_id.get_id());
}

void AccessibilityServerAccessKit::update_set_next_on_line(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_other_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_set_next_on_line(ae->node, (accesskit_node_id)p_other_id.get_id());
}

void AccessibilityServerAccessKit::update_set_previous_on_line(
	const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_other_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_set_previous_on_line(ae->node, (accesskit_node_id)p_other_id.get_id());
}

void AccessibilityServerAccessKit::update_set_member_of(const RID& p_id, const RID& p_group_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_group_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_set_member_of(ae->node, (accesskit_node_id)p_group_id.get_id());
}

void AccessibilityServerAccessKit::update_set_in_page_link_target(
	const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_other_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_set_in_page_link_target(ae->node, (accesskit_node_id)p_other_id.get_id());
}

void AccessibilityServerAccessKit::update_set_error_message(const RID& p_id, const RID& p_other_id)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* other_ae = rid_owner.get_or_null(p_other_id);
	ERR_FAIL_NULL(other_ae);
	ERR_FAIL_COND(other_ae->window_id != ae->window_id);
	_ensure_node(p_id, ae);

	accesskit_node_set_error_message(ae->node, (accesskit_node_id)p_other_id.get_id());
}

void AccessibilityServerAccessKit::update_set_live(
	const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	switch (p_live) {
	case AccessibilityServerEnums::AccessibilityLiveMode::LIVE_OFF: {
		accesskit_node_set_live(ae->node, ACCESSKIT_LIVE_OFF);
	} break;
	case AccessibilityServerEnums::AccessibilityLiveMode::LIVE_POLITE: {
		accesskit_node_set_live(ae->node, ACCESSKIT_LIVE_POLITE);
	} break;
	case AccessibilityServerEnums::AccessibilityLiveMode::LIVE_ASSERTIVE: {
		accesskit_node_set_live(ae->node, ACCESSKIT_LIVE_ASSERTIVE);
	} break;
	}
}

void AccessibilityServerAccessKit::update_add_custom_action(
	const RID& p_id, int p_action_id, const String& p_action_description)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_action_description.is_empty()) {
		accesskit_custom_action* ca = accesskit_custom_action_new(p_action_id);
		accesskit_custom_action_set_description(ca, p_action_description.utf8().ptr());
		accesskit_node_push_custom_action(ae->node, ca);
	}
	else {
		String cs_name = vformat("Custom Action %d", p_action_id);
		accesskit_custom_action* ca = accesskit_custom_action_new(p_action_id);
		accesskit_custom_action_set_description(ca, cs_name.utf8().ptr());
		accesskit_node_push_custom_action(ae->node, ca);
	}
}

void AccessibilityServerAccessKit::update_set_table_row_count(const RID& p_id, int p_count)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_row_count(ae->node, p_count);
}

void AccessibilityServerAccessKit::update_set_table_column_count(const RID& p_id, int p_count)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_column_count(ae->node, p_count);
}

void AccessibilityServerAccessKit::update_set_table_row_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_row_index(ae->node, p_index);
}

void AccessibilityServerAccessKit::update_set_table_column_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_column_index(ae->node, p_index);
}

void AccessibilityServerAccessKit::update_set_table_cell_position(
	const RID& p_id, int p_row_index, int p_column_index)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_row_index(ae->node, p_row_index);
	accesskit_node_set_column_index(ae->node, p_column_index);
}

void AccessibilityServerAccessKit::update_set_table_cell_span(
	const RID& p_id, int p_row_span, int p_column_span)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_row_span(ae->node, p_row_span);
	accesskit_node_set_column_span(ae->node, p_column_span);
}

void AccessibilityServerAccessKit::update_set_list_item_count(const RID& p_id, int p_size)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_size_of_set(ae->node, p_size);
}

void AccessibilityServerAccessKit::update_set_list_item_index(const RID& p_id, int p_index)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_position_in_set(ae->node, p_index);
}

void AccessibilityServerAccessKit::update_set_list_item_level(const RID& p_id, int p_level)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_level(ae->node, p_level);
}

void AccessibilityServerAccessKit::update_set_list_item_selected(const RID& p_id, bool p_selected)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_selected(ae->node, p_selected);
}

void AccessibilityServerAccessKit::update_set_list_item_expanded(const RID& p_id, bool p_expanded)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_expanded(ae->node, p_expanded);
}

void AccessibilityServerAccessKit::update_set_popup_type(
	const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	switch (p_popup) {
	case AccessibilityServerEnums::AccessibilityPopupType::POPUP_MENU: {
		accesskit_node_set_has_popup(ae->node, ACCESSKIT_HAS_POPUP_MENU);
	} break;
	case AccessibilityServerEnums::AccessibilityPopupType::POPUP_LIST: {
		accesskit_node_set_has_popup(ae->node, ACCESSKIT_HAS_POPUP_LISTBOX);
	} break;
	case AccessibilityServerEnums::AccessibilityPopupType::POPUP_TREE: {
		accesskit_node_set_has_popup(ae->node, ACCESSKIT_HAS_POPUP_TREE);
	} break;
	case AccessibilityServerEnums::AccessibilityPopupType::POPUP_DIALOG: {
		accesskit_node_set_has_popup(ae->node, ACCESSKIT_HAS_POPUP_DIALOG);
	} break;
	}
}

void AccessibilityServerAccessKit::update_set_checked(const RID& p_id, bool p_checekd)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (p_checekd) {
		accesskit_node_set_toggled(ae->node, ACCESSKIT_TOGGLED_TRUE);
	}
	else {
		accesskit_node_set_toggled(ae->node, ACCESSKIT_TOGGLED_FALSE);
	}
}

void AccessibilityServerAccessKit::update_set_num_value(const RID& p_id, double p_position)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_numeric_value(ae->node, p_position);
}

void AccessibilityServerAccessKit::update_set_num_range(const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_min_numeric_value(ae->node, p_min);
	accesskit_node_set_max_numeric_value(ae->node, p_max);
}

void AccessibilityServerAccessKit::update_set_num_step(const RID& p_id, double p_step)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_numeric_value_step(ae->node, p_step);
}

void AccessibilityServerAccessKit::update_set_num_jump(const RID& p_id, double p_jump)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_numeric_value_jump(ae->node, p_jump);
}

void AccessibilityServerAccessKit::update_set_scroll_x(const RID& p_id, double p_position)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_scroll_x(ae->node, p_position);
}

void AccessibilityServerAccessKit::update_set_scroll_x_range(
	const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_scroll_x_min(ae->node, p_min);
	accesskit_node_set_scroll_x_max(ae->node, p_max);
}

void AccessibilityServerAccessKit::update_set_scroll_y(const RID& p_id, double p_position)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_scroll_y(ae->node, p_position);
}

void AccessibilityServerAccessKit::update_set_scroll_y_range(
	const RID& p_id, double p_min, double p_max)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_scroll_y_min(ae->node, p_min);
	accesskit_node_set_scroll_y_max(ae->node, p_max);
}

void AccessibilityServerAccessKit::update_set_text_decorations(
	const RID& p_id, bool p_underline, bool p_strikethrough, bool p_overline, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_color color;
	color.red = p_color.get_r8();
	color.blue = p_color.get_b8();
	color.green = p_color.get_g8();
	color.alpha = p_color.get_a8();

	if (p_underline) {
		accesskit_node_set_underline(ae->node, {ACCESSKIT_TEXT_DECORATION_STYLE_SOLID, color});
	}
	else {
		accesskit_node_clear_underline(ae->node);
	}
	if (p_overline) {
		accesskit_node_set_overline(ae->node, {ACCESSKIT_TEXT_DECORATION_STYLE_SOLID, color});
	}
	else {
		accesskit_node_clear_overline(ae->node);
	}
	if (p_strikethrough) {
		accesskit_node_set_strikethrough(ae->node, {ACCESSKIT_TEXT_DECORATION_STYLE_SOLID, color});
	}
	else {
		accesskit_node_clear_strikethrough(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_text_align(
	const RID& p_id, HorizontalAlignment p_align)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	switch (p_align) {
	case HORIZONTAL_ALIGNMENT_LEFT: {
		accesskit_node_set_text_align(ae->node, ACCESSKIT_TEXT_ALIGN_LEFT);
	} break;
	case HORIZONTAL_ALIGNMENT_CENTER: {
		accesskit_node_set_text_align(ae->node, ACCESSKIT_TEXT_ALIGN_RIGHT);
	} break;
	case HORIZONTAL_ALIGNMENT_RIGHT: {
		accesskit_node_set_text_align(ae->node, ACCESSKIT_TEXT_ALIGN_CENTER);
	} break;
	case HORIZONTAL_ALIGNMENT_FILL: {
		accesskit_node_set_text_align(ae->node, ACCESSKIT_TEXT_ALIGN_JUSTIFY);
	} break;
	}
}

void AccessibilityServerAccessKit::update_set_text_selection(const RID& p_id,
	const RID& p_text_start_id, int p_start_char, const RID& p_text_end_id, int p_end_char)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	AccessibilityElement* start_ae = rid_owner.get_or_null(p_text_start_id);
	ERR_FAIL_NULL(start_ae);
	ERR_FAIL_COND(start_ae->window_id != ae->window_id);
	AccessibilityElement* end_ae = rid_owner.get_or_null(p_text_end_id);
	ERR_FAIL_NULL(end_ae);
	ERR_FAIL_COND(end_ae->window_id != ae->window_id);

	int start_pos = p_start_char;
	int end_pos = p_end_char;
	RID start_rid;
	RID end_rid;
	for (const RID& rid : start_ae->children) {
		const AccessibilityElement* child_ae = rid_owner.get_or_null(rid);
		if (child_ae && child_ae->role == ACCESSKIT_ROLE_TEXT_RUN) {
			if (p_start_char >= child_ae->run.x && p_start_char <= child_ae->run.y) {
				start_rid = rid;
				start_pos = p_start_char - child_ae->run.x;
				break;
			}
		}
	}
	for (const RID& rid : end_ae->children) {
		const AccessibilityElement* child_ae = rid_owner.get_or_null(rid);
		if (child_ae && child_ae->role == ACCESSKIT_ROLE_TEXT_RUN) {
			if (p_end_char >= child_ae->run.x && p_end_char <= child_ae->run.y) {
				end_rid = rid;
				end_pos = p_end_char - child_ae->run.x;
				break;
			}
		}
	}
	ERR_FAIL_COND(start_rid.is_null() && end_rid.is_null());
	_ensure_node(p_id, ae);

	accesskit_text_selection sel;
	sel.anchor.node = (accesskit_node_id)start_rid.get_id();
	sel.anchor.character_index = start_pos;
	sel.focus.node = (accesskit_node_id)end_rid.get_id();
	sel.focus.character_index = end_pos;
	accesskit_node_set_text_selection(ae->node, sel);
}

void AccessibilityServerAccessKit::update_set_flag(
	const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag, bool p_value)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (p_value) {
		ae->flags |= (1ULL << (uint64_t)p_flag);
	}
	else {
		ae->flags &= ~(1ULL << (uint64_t)p_flag);
	}
	switch (p_flag) {
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_HIDDEN: {
		if (p_value) {
			accesskit_node_set_hidden(ae->node);
		}
		else {
			accesskit_node_clear_hidden(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_MULTISELECTABLE: {
		if (p_value) {
			accesskit_node_set_multiselectable(ae->node);
		}
		else {
			accesskit_node_clear_multiselectable(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_REQUIRED: {
		if (p_value) {
			accesskit_node_set_required(ae->node);
		}
		else {
			accesskit_node_clear_required(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_VISITED: {
		if (p_value) {
			accesskit_node_set_visited(ae->node);
		}
		else {
			accesskit_node_clear_visited(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_BUSY: {
		if (p_value) {
			accesskit_node_set_busy(ae->node);
		}
		else {
			accesskit_node_clear_busy(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_MODAL: {
		if (p_value) {
			accesskit_node_set_modal(ae->node);
		}
		else {
			accesskit_node_clear_modal(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_TOUCH_PASSTHROUGH: {
		if (p_value) {
			accesskit_node_set_touch_transparent(ae->node);
		}
		else {
			accesskit_node_clear_touch_transparent(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_READONLY: {
		if (p_value) {
			accesskit_node_set_read_only(ae->node);
		}
		else {
			accesskit_node_clear_read_only(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_DISABLED: {
		if (p_value) {
			accesskit_node_set_disabled(ae->node);
		}
		else {
			accesskit_node_clear_disabled(ae->node);
		}
	} break;
	case AccessibilityServerEnums::AccessibilityFlags::FLAG_CLIPS_CHILDREN: {
		if (p_value) {
			accesskit_node_set_clips_children(ae->node);
		}
		else {
			accesskit_node_clear_clips_children(ae->node);
		}
	} break;
	}
}

void AccessibilityServerAccessKit::update_set_classname(const RID& p_id, const String& p_classname)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_classname.is_empty()) {
		accesskit_node_set_class_name(ae->node, p_classname.utf8().ptr());
	}
	else {
		accesskit_node_clear_class_name(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_placeholder(
	const RID& p_id, const String& p_placeholder)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_placeholder.is_empty()) {
		accesskit_node_set_placeholder(ae->node, p_placeholder.utf8().ptr());
	}
	else {
		accesskit_node_clear_placeholder(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_language(const RID& p_id, const String& p_language)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_node_set_language(ae->node, p_language.utf8().ptr());
}

void AccessibilityServerAccessKit::update_set_text_orientation(const RID& p_id, bool p_vertical)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (p_vertical) {
		accesskit_node_set_text_direction(ae->node, ACCESSKIT_TEXT_DIRECTION_TOP_TO_BOTTOM);
	}
	else {
		accesskit_node_set_text_direction(ae->node, ACCESSKIT_TEXT_DIRECTION_LEFT_TO_RIGHT);
	}
}

void AccessibilityServerAccessKit::update_set_list_orientation(const RID& p_id, bool p_vertical)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (p_vertical) {
		accesskit_node_set_orientation(ae->node, ACCESSKIT_ORIENTATION_VERTICAL);
	}
	else {
		accesskit_node_set_orientation(ae->node, ACCESSKIT_ORIENTATION_HORIZONTAL);
	}
}

void AccessibilityServerAccessKit::update_set_shortcut(const RID& p_id, const String& p_shortcut)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_shortcut.is_empty()) {
		accesskit_node_set_keyboard_shortcut(ae->node, p_shortcut.utf8().ptr());
	}
	else {
		accesskit_node_clear_keyboard_shortcut(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_url(const RID& p_id, const String& p_url)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_url.is_empty()) {
		accesskit_node_set_url(ae->node, p_url.utf8().ptr());
	}
	else {
		accesskit_node_clear_url(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_role_description(
	const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_description.is_empty()) {
		accesskit_node_set_role_description(ae->node, p_description.utf8().ptr());
	}
	else {
		accesskit_node_clear_role_description(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_state_description(
	const RID& p_id, const String& p_description)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	if (!p_description.is_empty()) {
		accesskit_node_set_state_description(ae->node, p_description.utf8().ptr());
	}
	else {
		accesskit_node_clear_state_description(ae->node);
	}
}

void AccessibilityServerAccessKit::update_set_color_value(const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_color color;
	color.red = p_color.get_r8();
	color.blue = p_color.get_b8();
	color.green = p_color.get_g8();
	color.alpha = p_color.get_a8();

	accesskit_node_set_color_value(ae->node, color);
}

void AccessibilityServerAccessKit::update_set_background_color(
	const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_color color;
	color.red = p_color.get_r8();
	color.blue = p_color.get_b8();
	color.green = p_color.get_g8();
	color.alpha = p_color.get_a8();

	accesskit_node_set_background_color(ae->node, color);
}

void AccessibilityServerAccessKit::update_set_foreground_color(
	const RID& p_id, const Color& p_color)
{
	ERR_FAIL_COND_MSG(!in_accessibility_update, "Accessibility updates are only allowed inside the "
												"NOTIFICATION_ACCESSIBILITY_UPDATE notification.");

	AccessibilityElement* ae = rid_owner.get_or_null(p_id);
	ERR_FAIL_NULL(ae);
	_ensure_node(p_id, ae);

	accesskit_color color;
	color.red = p_color.get_r8();
	color.blue = p_color.get_b8();
	color.green = p_color.get_g8();
	color.alpha = p_color.get_a8();

	accesskit_node_set_foreground_color(ae->node, color);
}

AccessibilityServerDriver* AccessibilityServerAccessKit::create_func(Error& r_error)
{
	print_verbose("Accessibility: AccessKit driver loaded.");
	r_error = OK;
	return memnew(AccessibilityServerAccessKit);
}

AccessibilityServerAccessKit::AccessibilityServerAccessKit()
{
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_UNKNOWN] = ACCESSKIT_ROLE_UNKNOWN;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_DEFAULT_BUTTON] =
		ACCESSKIT_ROLE_DEFAULT_BUTTON;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_AUDIO] = ACCESSKIT_ROLE_AUDIO;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_VIDEO] = ACCESSKIT_ROLE_VIDEO;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_STATIC_TEXT] = ACCESSKIT_ROLE_LABEL;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_CONTAINER] =
		ACCESSKIT_ROLE_GENERIC_CONTAINER;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_PANEL] = ACCESSKIT_ROLE_PANE;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_BUTTON] = ACCESSKIT_ROLE_BUTTON;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_LINK] = ACCESSKIT_ROLE_LINK;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_CHECK_BOX] =
		ACCESSKIT_ROLE_CHECK_BOX;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_RADIO_BUTTON] =
		ACCESSKIT_ROLE_RADIO_BUTTON;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_CHECK_BUTTON] =
		ACCESSKIT_ROLE_SWITCH;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_SCROLL_BAR] =
		ACCESSKIT_ROLE_SCROLL_BAR;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_SCROLL_VIEW] =
		ACCESSKIT_ROLE_SCROLL_VIEW;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_SPLITTER] = ACCESSKIT_ROLE_SPLITTER;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_SLIDER] = ACCESSKIT_ROLE_SLIDER;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_SPIN_BUTTON] =
		ACCESSKIT_ROLE_SPIN_BUTTON;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_PROGRESS_INDICATOR] =
		ACCESSKIT_ROLE_PROGRESS_INDICATOR;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TEXT_FIELD] =
		ACCESSKIT_ROLE_TEXT_INPUT;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_MULTILINE_TEXT_FIELD] =
		ACCESSKIT_ROLE_MULTILINE_TEXT_INPUT;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_COLOR_PICKER] =
		ACCESSKIT_ROLE_COLOR_WELL;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TABLE] = ACCESSKIT_ROLE_TABLE;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_CELL] = ACCESSKIT_ROLE_CELL;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_ROW] = ACCESSKIT_ROLE_ROW;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_ROW_GROUP] =
		ACCESSKIT_ROLE_ROW_GROUP;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_ROW_HEADER] =
		ACCESSKIT_ROLE_ROW_HEADER;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_COLUMN_HEADER] =
		ACCESSKIT_ROLE_COLUMN_HEADER;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TREE] = ACCESSKIT_ROLE_TREE;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TREE_ITEM] =
		ACCESSKIT_ROLE_TREE_ITEM;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_LIST] = ACCESSKIT_ROLE_LIST;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_LIST_ITEM] =
		ACCESSKIT_ROLE_LIST_ITEM;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_LIST_BOX] = ACCESSKIT_ROLE_LIST_BOX;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_LIST_BOX_OPTION] =
		ACCESSKIT_ROLE_LIST_BOX_OPTION;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TAB_BAR] = ACCESSKIT_ROLE_TAB_LIST;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TAB] = ACCESSKIT_ROLE_TAB;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TAB_PANEL] =
		ACCESSKIT_ROLE_TAB_PANEL;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_MENU_BAR] = ACCESSKIT_ROLE_MENU_BAR;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_MENU] = ACCESSKIT_ROLE_MENU;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_MENU_ITEM] =
		ACCESSKIT_ROLE_MENU_ITEM;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_MENU_ITEM_CHECK_BOX] =
		ACCESSKIT_ROLE_MENU_ITEM_CHECK_BOX;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_MENU_ITEM_RADIO] =
		ACCESSKIT_ROLE_MENU_ITEM_RADIO;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_IMAGE] = ACCESSKIT_ROLE_IMAGE;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_WINDOW] = ACCESSKIT_ROLE_WINDOW;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TITLE_BAR] =
		ACCESSKIT_ROLE_TITLE_BAR;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_DIALOG] = ACCESSKIT_ROLE_DIALOG;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TOOLTIP] = ACCESSKIT_ROLE_TOOLTIP;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_REGION] = ACCESSKIT_ROLE_REGION;
	role_map[AccessibilityServerEnums::AccessibilityRole::ROLE_TEXT_RUN] = ACCESSKIT_ROLE_TEXT_RUN;

	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_CLICK] =
		ACCESSKIT_ACTION_CLICK;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_FOCUS] =
		ACCESSKIT_ACTION_FOCUS;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_BLUR] = ACCESSKIT_ACTION_BLUR;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_COLLAPSE] =
		ACCESSKIT_ACTION_COLLAPSE;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_EXPAND] =
		ACCESSKIT_ACTION_EXPAND;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_DECREMENT] =
		ACCESSKIT_ACTION_DECREMENT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_INCREMENT] =
		ACCESSKIT_ACTION_INCREMENT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_HIDE_TOOLTIP] =
		ACCESSKIT_ACTION_HIDE_TOOLTIP;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SHOW_TOOLTIP] =
		ACCESSKIT_ACTION_SHOW_TOOLTIP;
	// action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_INVALIDATE_TREE] =
	// ACCESSKIT_ACTION_INVALIDATE_TREE;
	// action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_LOAD_INLINE_TEXT_BOXES] =
	// ACCESSKIT_ACTION_LOAD_INLINE_TEXT_BOXES;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SET_TEXT_SELECTION] =
		ACCESSKIT_ACTION_SET_TEXT_SELECTION;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_REPLACE_SELECTED_TEXT] =
		ACCESSKIT_ACTION_REPLACE_SELECTED_TEXT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_BACKWARD] =
		ACCESSKIT_ACTION_SCROLL_UP;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_DOWN] =
		ACCESSKIT_ACTION_SCROLL_DOWN;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_FORWARD] =
		ACCESSKIT_ACTION_SCROLL_DOWN;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_LEFT] =
		ACCESSKIT_ACTION_SCROLL_LEFT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_RIGHT] =
		ACCESSKIT_ACTION_SCROLL_RIGHT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_UP] =
		ACCESSKIT_ACTION_SCROLL_UP;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_INTO_VIEW] =
		ACCESSKIT_ACTION_SCROLL_INTO_VIEW;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SCROLL_TO_POINT] =
		ACCESSKIT_ACTION_SCROLL_TO_POINT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SET_SCROLL_OFFSET] =
		ACCESSKIT_ACTION_SET_SCROLL_OFFSET;
	// action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SET_SEQUENTIAL_FOCUS_NAVIGATION_STARTING_POINT]
	// = ACCESSKIT_ACTION_SET_SEQUENTIAL_FOCUS_NAVIGATION_STARTING_POINT;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SET_VALUE] =
		ACCESSKIT_ACTION_SET_VALUE;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_SHOW_CONTEXT_MENU] =
		ACCESSKIT_ACTION_SHOW_CONTEXT_MENU;
	action_map[AccessibilityServerEnums::AccessibilityAction::ACTION_CUSTOM] =
		ACCESSKIT_ACTION_CUSTOM_ACTION;
}

AccessibilityServerAccessKit::~AccessibilityServerAccessKit() {}

void AccessibilityServerAccessKit::register_create_func()
{
	AccessibilityServer::register_create_function("accesskit", create_func);
}

#endif // ACCESSKIT_ENABLED



// ============================================================================
// Merged from drivers/alsa/audio_driver_alsa.cpp
// ============================================================================

#include "platform/linuxbsd/os_linuxbsd.h"

#ifdef ALSA_ENABLED

#include "core/config/engine.h"
#include "core/math/math_funcs_binary.h"
#include "core/os/os.h"

#ifdef SOWRAP_ENABLED
#include "drivers/alsa/asound-so_wrap.h"
#else
#include <alsa/asoundlib.h>
#endif

#include <cerrno>

#if defined(PULSEAUDIO_ENABLED) && defined(SOWRAP_ENABLED)
extern "C" {
extern int initialize_pulse(int verbose);
}
#endif

Error AudioDriverALSA::init_output_device() {
	mix_rate = _get_configured_mix_rate();

	speaker_mode = SPEAKER_MODE_STEREO;
	channels = 2;

	// If there is a specified output device check that it is really present
	if (output_device_name != "Default") {
		PackedStringArray list = get_output_device_list();
		if (!list.has(output_device_name)) {
			output_device_name = "Default";
			new_output_device = "Default";
		}
	}

	int status;
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;

#define CHECK_FAIL(m_cond) \
	if (m_cond) { \
		fprintf(stderr, "ALSA ERR: %s\n", snd_strerror(status)); \
		if (pcm_handle) { \
			snd_pcm_close(pcm_handle); \
			pcm_handle = nullptr; \
		} \
		ERR_FAIL_COND_V(m_cond, ERR_CANT_OPEN); \
	}

	//todo, add
	//6 chans - "plug:surround51"
	//4 chans - "plug:surround40";

	if (output_device_name == "Default") {
		status = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	} else {
		String device = output_device_name;
		int pos = device.find_char(';');
		if (pos != -1) {
			device = device.substr(0, pos);
		}
		status = snd_pcm_open(&pcm_handle, device.utf8().get_data(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	}

	ERR_FAIL_COND_V(status < 0, ERR_CANT_OPEN);

	snd_pcm_hw_params_alloca(&hwparams); // NOLINT(modernize-use-bool-literals)

	status = snd_pcm_hw_params_any(pcm_handle, hwparams);
	CHECK_FAIL(status < 0);

	status = snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
	CHECK_FAIL(status < 0);

	//not interested in anything else
	status = snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE);
	CHECK_FAIL(status < 0);

	//todo: support 4 and 6
	status = snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2);
	CHECK_FAIL(status < 0);

	status = snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &mix_rate, nullptr);
	CHECK_FAIL(status < 0);

	// In ALSA the period size seems to be the one that will determine the actual latency
	// Ref: https://www.alsa-project.org/main/index.php/FramesPeriods
	unsigned int periods = 2;
	int latency = Engine::get_audio_output_latency();
	buffer_frames = Math::closest_power_of_2(latency * mix_rate / 1000);
	buffer_size = buffer_frames * periods;
	period_size = buffer_frames;

	// set buffer size from project settings
	status = snd_pcm_hw_params_set_buffer_size_near(pcm_handle, hwparams, &buffer_size);
	CHECK_FAIL(status < 0);

	status = snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &period_size, nullptr);
	CHECK_FAIL(status < 0);

	print_verbose("Audio buffer frames: " + itos(period_size) + " calculated latency: " + itos(period_size * 1000 / mix_rate) + "ms");

	status = snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &periods, nullptr);
	CHECK_FAIL(status < 0);

	status = snd_pcm_hw_params(pcm_handle, hwparams);
	CHECK_FAIL(status < 0);

	//snd_pcm_hw_params_free(&hwparams);

	snd_pcm_sw_params_alloca(&swparams); // NOLINT(modernize-use-bool-literals)

	status = snd_pcm_sw_params_current(pcm_handle, swparams);
	CHECK_FAIL(status < 0);

	status = snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, period_size);
	CHECK_FAIL(status < 0);

	status = snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, 1);
	CHECK_FAIL(status < 0);

	status = snd_pcm_sw_params(pcm_handle, swparams);
	CHECK_FAIL(status < 0);

	samples_in.resize(period_size * channels);
	samples_out.resize(period_size * channels);

	return OK;
}

Error AudioDriverALSA::init() {
#ifdef SOWRAP_ENABLED
#ifdef DEBUG_ENABLED
	int dylibloader_verbose = 1;
#else
	int dylibloader_verbose = 0;
#endif
#ifdef PULSEAUDIO_ENABLED
	// On pulse enabled systems Alsa will silently use pulse.
	// It doesn't matter if this fails as that likely means there is no pulse
	initialize_pulse(dylibloader_verbose);
#endif

	if (initialize_asound(dylibloader_verbose)) {
		return ERR_CANT_OPEN;
	}
#endif
	bool ver_ok = false;
	String version = String::utf8(snd_asoundlib_version());
	Vector<String> ver_parts = version.split(".");
	if (ver_parts.size() >= 2) {
		ver_ok = ((ver_parts[0].to_int() == 1 && ver_parts[1].to_int() >= 1)) || (ver_parts[0].to_int() > 1); // 1.1.0
	}
	print_verbose(vformat("ALSA %s detected.", version));
	if (!ver_ok) {
		print_verbose("Unsupported ALSA library version!");
		return ERR_CANT_OPEN;
	}

	active.clear();
	exit_thread.clear();

	Error err = init_output_device();
	if (err == OK) {
		thread.start(AudioDriverALSA::thread_func, this);
	}

	return err;
}

void AudioDriverALSA::thread_func(void *p_udata) {
	AudioDriverALSA *ad = static_cast<AudioDriverALSA *>(p_udata);

	while (!ad->exit_thread.is_set()) {
		ad->lock();
		ad->start_counting_ticks();

		if (!ad->active.is_set()) {
			ad->samples_out.fill(0);

		} else {
			ad->audio_server_process(ad->period_size, ad->samples_in.ptrw());

			int16_t *out_ptr = ad->samples_out.ptrw();
			for (uint64_t i = 0; i < ad->period_size * ad->channels; i++) {
				out_ptr[i] = ad->samples_in[i] >> 16;
			}
		}

		int todo = ad->period_size;
		int total = 0;

		while (todo && !ad->exit_thread.is_set()) {
			int16_t *src = (int16_t *)ad->samples_out.ptr();
			int wrote = snd_pcm_writei(ad->pcm_handle, (void *)(src + (total * ad->channels)), todo);

			if (wrote > 0) {
				total += wrote;
				todo -= wrote;
			} else if (wrote == -EAGAIN) {
				ad->stop_counting_ticks();
				ad->unlock();

				OS::delay_usec(1000);

				ad->lock();
				ad->start_counting_ticks();
			} else {
				wrote = snd_pcm_recover(ad->pcm_handle, wrote, 0);
				if (wrote < 0) {
					ERR_PRINT("ALSA: Failed and can't recover: " + String(snd_strerror(wrote)));
					ad->active.clear();
					ad->exit_thread.set();
				}
			}
		}

		// User selected a new output device, finish the current one so we'll init the new device.
		if (ad->output_device_name != ad->new_output_device) {
			ad->output_device_name = ad->new_output_device;
			ad->finish_output_device();

			Error err = ad->init_output_device();
			if (err != OK) {
				ERR_PRINT("ALSA: init_output_device error");
				ad->output_device_name = "Default";
				ad->new_output_device = "Default";

				err = ad->init_output_device();
				if (err != OK) {
					ad->active.clear();
					ad->exit_thread.set();
				}
			}
		}

		ad->stop_counting_ticks();
		ad->unlock();
	}
}

void AudioDriverALSA::start() {
	active.set();
}

int AudioDriverALSA::get_mix_rate() const {
	return mix_rate;
}

AudioDriver::SpeakerMode AudioDriverALSA::get_speaker_mode() const {
	return speaker_mode;
}

PackedStringArray AudioDriverALSA::get_output_device_list() {
	PackedStringArray list;

	list.push_back("Default");

	void **hints;

	if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
		return list;
	}

	for (void **n = hints; *n != nullptr; n++) {
		char *name = snd_device_name_get_hint(*n, "NAME");
		char *desc = snd_device_name_get_hint(*n, "DESC");

		if (name != nullptr && !strncmp(name, "plughw", 6)) {
			if (desc) {
				list.push_back(String::utf8(name) + ";" + String::utf8(desc));
			} else {
				list.push_back(String::utf8(name));
			}
		}

		if (desc != nullptr) {
			free(desc);
		}
		if (name != nullptr) {
			free(name);
		}
	}
	snd_device_name_free_hint(hints);

	return list;
}

String AudioDriverALSA::get_output_device() {
	return output_device_name;
}

void AudioDriverALSA::set_output_device(const String &p_name) {
	lock();
	new_output_device = p_name;
	unlock();
}

void AudioDriverALSA::lock() {
	mutex.lock();
}

void AudioDriverALSA::unlock() {
	mutex.unlock();
}

void AudioDriverALSA::finish_output_device() {
	if (pcm_handle) {
		snd_pcm_close(pcm_handle);
		pcm_handle = nullptr;
	}
}

void AudioDriverALSA::finish() {
	exit_thread.set();
	if (thread.is_started()) {
		thread.wait_to_finish();
	}

	finish_output_device();
}

#endif // ALSA_ENABLED

// ============================================================================
// Merged from drivers/alsamidi/midi_driver_alsamidi.cpp
// ============================================================================

#ifdef ALSAMIDI_ENABLED

#include "platform/linuxbsd/os_linuxbsd.h"

#include "core/os/os.h"

#ifdef SOWRAP_ENABLED
#include "drivers/alsa/asound-so_wrap.h"
#else
#include <alsa/asoundlib.h>
#endif

#include <cerrno>

MIDIDriverALSAMidi::InputConnection::InputConnection(int p_device_index,
		snd_rawmidi_t *p_rawmidi) :
		parser(p_device_index), rawmidi_ptr(p_rawmidi) {}

void MIDIDriverALSAMidi::InputConnection::read() {
	int read_count;
	do {
		uint8_t buffer[32];
		read_count = snd_rawmidi_read(rawmidi_ptr, buffer, sizeof(buffer));

		if (read_count < 0) {
			if (read_count != -EAGAIN) {
				ERR_PRINT("snd_rawmidi_read error: " + String(snd_strerror(read_count)));
			}
		} else {
			for (int i = 0; i < read_count; i++) {
				parser.parse_fragment(buffer[i]);
			}
		}
	} while (read_count > 0);
}

void MIDIDriverALSAMidi::thread_func(void* p_udata)
{
	(void)p_udata;
	if (!data) {
		return;
	}

	while (!data->exit_thread.is_set()) {
		lock();
		for (InputConnection& conn : data->connected_inputs) {
			conn.read();
		}
		unlock();

		OS::delay_usec(1000);
	}
}

void MIDIDriverALSAMidi::initialize()
{
	if (data != nullptr) {
		return;
	}
	data = memnew(Data);
	data->exit_thread.clear();
}

void MIDIDriverALSAMidi::finalize()
{
	if (data == nullptr) {
		return;
	}
	close();
	memdelete(data);
	data = nullptr;
}

Error MIDIDriverALSAMidi::open()
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);

	void** hints;

	if (snd_device_name_hint(-1, "rawmidi", &hints) < 0) {
		return ERR_CANT_OPEN;
	}

	lock();
	int device_index = 0;
	for (void** h = hints; *h != nullptr; h++) {
		char* name = snd_device_name_get_hint(*h, "NAME");

		if (name != nullptr) {
			snd_rawmidi_t* midi_in;
			int ret = snd_rawmidi_open(&midi_in, nullptr, name, SND_RAWMIDI_NONBLOCK);
			if (ret >= 0) {
				// Get display name.
				snd_rawmidi_info_t* info;
				snd_rawmidi_info_malloc(&info);
				snd_rawmidi_info(midi_in, info);
				MIDIDriver::add_connected_input(snd_rawmidi_info_get_name(info));
				snd_rawmidi_info_free(info);

				data->connected_inputs.push_back(InputConnection(device_index, midi_in));
				// Only increment device_index for successfully connected devices.
				device_index++;
			}
		}

		if (name != nullptr) {
			free(name);
		}
	}
	snd_device_name_free_hint(hints);
	unlock();

	data->exit_thread.clear();
	data->thread.start(MIDIDriverALSAMidi::thread_func, nullptr);

	return OK;
}

void MIDIDriverALSAMidi::close()
{
	if (data == nullptr) {
		return;
	}

	data->exit_thread.set();
	if (data->thread.is_started()) {
		data->thread.wait_to_finish();
	}

	for (const InputConnection& conn : data->connected_inputs) {
		snd_rawmidi_close(conn.rawmidi_ptr);
	}

	data->connected_inputs.clear();
	MIDIDriver::clear_connected_inputs();
}

void MIDIDriverALSAMidi::lock()
{
	if (data) {
		data->mutex.lock();
	}
}

void MIDIDriverALSAMidi::unlock()
{
	if (data) {
		data->mutex.unlock();
	}
}

#endif // ALSAMIDI_ENABLED

// ============================================================================
// Merged from drivers/egl/egl_manager.cpp
// ============================================================================

#include "drivers/egl/egl_manager.h"

#ifdef EGL_ENABLED

#include "core/config/engine.h"
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/os/os.h"

#ifdef WINDOWS_ENABLED
#include "drivers/gles3/rasterizer_gles3.h"
#endif

#if defined(EGL_STATIC)

#define GLAD_EGL_VERSION_1_5 true

#ifdef EGL_EXT_platform_base
#define GLAD_EGL_EXT_platform_base 1
#endif

#define KHRONOS_STATIC 1
extern "C" EGLAPI void EGLAPIENTRY eglSetBlobCacheFuncsANDROID(EGLDisplay dpy, EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get);
extern "C" EGLAPI EGLDisplay EGLAPIENTRY eglGetPlatformDisplayEXT(EGLenum platform, void *native_display, const EGLint *attrib_list);
#undef KHRONOS_STATIC

#endif // defined(EGL_STATIC)

#ifndef EGL_EXT_platform_base
#define GLAD_EGL_EXT_platform_base 0
#endif

#ifdef WINDOWS_ENABLED
// Unofficial ANGLE extension: EGL_ANGLE_surface_orientation
#ifndef EGL_OPTIMAL_SURFACE_ORIENTATION_ANGLE
#define EGL_OPTIMAL_SURFACE_ORIENTATION_ANGLE 0x33A7
#define EGL_SURFACE_ORIENTATION_ANGLE 0x33A8
#define EGL_SURFACE_ORIENTATION_INVERT_X_ANGLE 0x0001
#define EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE 0x0002
#endif
#endif

// Creates and caches a GLDisplay. Returns -1 on error.
int EGLManager::_get_gldisplay_id(void *p_display) {
	// Look for a cached GLDisplay.
	for (unsigned int i = 0; i < displays.size(); i++) {
		if (displays[i].display == p_display) {
			return i;
		}
	}

	// We didn't find any, so we'll have to create one, along with its own
	// EGLDisplay and EGLContext.
	GLDisplay new_gldisplay;
	new_gldisplay.display = p_display;

	if (GLAD_EGL_VERSION_1_5) {
		Vector<EGLAttrib> attribs = _get_platform_display_attributes();
		new_gldisplay.egl_display = eglGetPlatformDisplay(_get_platform_extension_enum(), new_gldisplay.display, (attribs.size() > 0) ? attribs.ptr() : nullptr);
	} else if (GLAD_EGL_EXT_platform_base) {
#ifdef EGL_EXT_platform_base
		// eglGetPlatformDisplayEXT wants its attributes as EGLint, so we'll truncate
		// what we already have. It's a bit naughty but I'm really not sure what else
		// we could do here.
		Vector<EGLint> attribs;
		for (const EGLAttrib &attrib : _get_platform_display_attributes()) {
			attribs.push_back((EGLint)attrib);
		}

		new_gldisplay.egl_display = eglGetPlatformDisplayEXT(_get_platform_extension_enum(), new_gldisplay.display, (attribs.size() > 0) ? attribs.ptr() : nullptr);
#endif // EGL_EXT_platform_base
	} else {
		NativeDisplayType *native_display_type = (NativeDisplayType *)new_gldisplay.display;
		new_gldisplay.egl_display = eglGetDisplay(*native_display_type);
	}

	ERR_FAIL_COND_V(eglGetError() != EGL_SUCCESS, -1);

	ERR_FAIL_COND_V_MSG(new_gldisplay.egl_display == EGL_NO_DISPLAY, -1, "Can't create an EGL display.");

	if (!eglInitialize(new_gldisplay.egl_display, nullptr, nullptr)) {
		ERR_FAIL_V_MSG(-1, "Can't initialize an EGL display.");
	}

	if (!eglBindAPI(_get_platform_api_enum())) {
		ERR_FAIL_V_MSG(-1, "OpenGL not supported.");
	}

	Error err = _gldisplay_create_context(new_gldisplay);

	if (err != OK) {
		eglTerminate(new_gldisplay.egl_display);
		ERR_FAIL_V(-1);
	}

#ifdef EGL_ANDROID_blob_cache
#if defined(EGL_STATIC)
	bool has_blob_cache = true;
#else
	bool has_blob_cache = (eglSetBlobCacheFuncsANDROID != nullptr);
#endif
	if (has_blob_cache && !shader_cache_dir.is_empty()) {
		eglSetBlobCacheFuncsANDROID(new_gldisplay.egl_display, &EGLManager::_set_cache, &EGLManager::_get_cache);
	}
#endif

#ifdef WINDOWS_ENABLED
	String client_extensions_string = eglQueryString(new_gldisplay.egl_display, EGL_EXTENSIONS);
	if (eglGetError() == EGL_SUCCESS) {
		Vector<String> egl_extensions = client_extensions_string.split(" ");

		if (egl_extensions.has("EGL_ANGLE_surface_orientation")) {
			new_gldisplay.has_EGL_ANGLE_surface_orientation = true;
			print_verbose("EGL: EGL_ANGLE_surface_orientation is supported.");
		}
	}
#endif

	displays.push_back(new_gldisplay);

	// Return the new GLDisplay's ID.
	return displays.size() - 1;
}

#ifdef EGL_ANDROID_blob_cache
String EGLManager::shader_cache_dir;

void EGLManager::_set_cache(const void *p_key, EGLsizeiANDROID p_key_size, const void *p_value, EGLsizeiANDROID p_value_size) {
	String name = CryptoCore::b64_encode_str((const uint8_t *)p_key, p_key_size).replace_char('/', '_');
	String path = shader_cache_dir.path_join(name) + ".cache";

	Error err = OK;
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE, &err);
	if (err != OK) {
		return;
	}
	file->store_buffer((const uint8_t *)p_value, p_value_size);
}

EGLsizeiANDROID EGLManager::_get_cache(const void *p_key, EGLsizeiANDROID p_key_size, void *p_value, EGLsizeiANDROID p_value_size) {
	String name = CryptoCore::b64_encode_str((const uint8_t *)p_key, p_key_size).replace_char('/', '_');
	String path = shader_cache_dir.path_join(name) + ".cache";

	Error err = OK;
	Ref<FileAccess> file = FileAccess::open(path, FileAccess::READ, &err);
	if (err != OK) {
		return 0;
	}
	EGLsizeiANDROID len = file->get_length();
	if (len <= p_value_size) {
		file->get_buffer((uint8_t *)p_value, len);
	}
	return len;
}
#endif

Error EGLManager::_gldisplay_create_context(GLDisplay &p_gldisplay) {
	EGLint attribs[] = {
		EGL_RED_SIZE,
		1,
		EGL_BLUE_SIZE,
		1,
		EGL_GREEN_SIZE,
		1,
		EGL_DEPTH_SIZE,
		24,
		EGL_NONE,
	};

	EGLint attribs_layered[] = {
		EGL_RED_SIZE,
		8,
		EGL_GREEN_SIZE,
		8,
		EGL_GREEN_SIZE,
		8,
		EGL_ALPHA_SIZE,
		8,
		EGL_DEPTH_SIZE,
		24,
		EGL_NONE,
	};

	EGLint config_count = 0;

	if (OS::is_layered_allowed()) {
		eglChooseConfig(p_gldisplay.egl_display, attribs_layered, &p_gldisplay.egl_config, 1, &config_count);
	} else {
		eglChooseConfig(p_gldisplay.egl_display, attribs, &p_gldisplay.egl_config, 1, &config_count);
	}

	ERR_FAIL_COND_V(eglGetError() != EGL_SUCCESS, ERR_BUG);
	ERR_FAIL_COND_V(config_count == 0, ERR_UNCONFIGURED);

	Vector<EGLint> context_attribs = _get_platform_context_attribs();
	p_gldisplay.egl_context = eglCreateContext(p_gldisplay.egl_display, p_gldisplay.egl_config, EGL_NO_CONTEXT, (context_attribs.size() > 0) ? context_attribs.ptr() : nullptr);
	ERR_FAIL_COND_V_MSG(p_gldisplay.egl_context == EGL_NO_CONTEXT, ERR_CANT_CREATE, vformat("Can't create an EGL context. Error code: %d", eglGetError()));

	return OK;
}

Error EGLManager::open_display(void *p_display) {
	int gldisplay_id = _get_gldisplay_id(p_display);
	if (gldisplay_id < 0) {
		return ERR_CANT_CREATE;
	} else {
		return OK;
	}
}

int EGLManager::display_get_native_visual_id(void *p_display) {
	int gldisplay_id = _get_gldisplay_id(p_display);
	ERR_FAIL_COND_V(gldisplay_id < 0, ERR_CANT_CREATE);

	GLDisplay gldisplay = displays[gldisplay_id];

	EGLint native_visual_id = -1;

	if (!eglGetConfigAttrib(gldisplay.egl_display, gldisplay.egl_config, EGL_NATIVE_VISUAL_ID, &native_visual_id)) {
		ERR_FAIL_V(-1);
	}

	return native_visual_id;
}

Error EGLManager::window_create(DisplayServerEnums::WindowID p_window_id, void *p_display, void *p_native_window, int p_width, int p_height) {
	int gldisplay_id = _get_gldisplay_id(p_display);
	ERR_FAIL_COND_V(gldisplay_id < 0, ERR_CANT_CREATE);

	GLDisplay &gldisplay = displays[gldisplay_id];

	// In order to ensure a fast lookup, make sure we got enough elements in the
	// windows local vector to use the window id as an index.
	if (p_window_id >= (int)windows.size()) {
		windows.resize(p_window_id + 1);
	}

	GLWindow &glwindow = windows[p_window_id];
	glwindow.gldisplay_id = gldisplay_id;

	Vector<EGLAttrib> egl_attribs;

#ifdef WINDOWS_ENABLED
	if (gldisplay.has_EGL_ANGLE_surface_orientation) {
		EGLint optimal_orientation;
		if (eglGetConfigAttrib(gldisplay.egl_display, gldisplay.egl_config, EGL_OPTIMAL_SURFACE_ORIENTATION_ANGLE, &optimal_orientation)) {
			// We only need to support inverting Y for optimizing ANGLE on D3D11.
			if (optimal_orientation & EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE && !(optimal_orientation & EGL_SURFACE_ORIENTATION_INVERT_X_ANGLE)) {
				egl_attribs.push_back(EGL_SURFACE_ORIENTATION_ANGLE);
				egl_attribs.push_back(EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE);
			}
		} else {
			ERR_PRINT(vformat("Failed to get EGL_OPTIMAL_SURFACE_ORIENTATION_ANGLE, error: 0x%08X", eglGetError()));
		}
	}

	if (!egl_attribs.is_empty()) {
		egl_attribs.push_back(EGL_NONE);
	}
#endif

	if (GLAD_EGL_VERSION_1_5) {
		glwindow.egl_surface = eglCreatePlatformWindowSurface(gldisplay.egl_display, gldisplay.egl_config, p_native_window, egl_attribs.ptr());
	} else {
		EGLNativeWindowType *native_window_type = (EGLNativeWindowType *)p_native_window;
		glwindow.egl_surface = eglCreateWindowSurface(gldisplay.egl_display, gldisplay.egl_config, *native_window_type, nullptr);
	}

	if (glwindow.egl_surface == EGL_NO_SURFACE) {
		return ERR_CANT_CREATE;
	}

	glwindow.initialized = true;

#ifdef WINDOWS_ENABLED
	if (gldisplay.has_EGL_ANGLE_surface_orientation) {
		EGLint orientation;
		if (eglQuerySurface(gldisplay.egl_display, glwindow.egl_surface, EGL_SURFACE_ORIENTATION_ANGLE, &orientation)) {
			if (orientation & EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE && !(orientation & EGL_SURFACE_ORIENTATION_INVERT_X_ANGLE)) {
				glwindow.flipped_y = true;
				print_verbose("EGL: Using optimal surface orientation: Invert Y");
			}
		} else {
			ERR_PRINT(vformat("Failed to get EGL_SURFACE_ORIENTATION_ANGLE, error: 0x%08X", eglGetError()));
		}
	}
#endif

	window_make_current(p_window_id);

	return OK;
}

void EGLManager::window_destroy(DisplayServerEnums::WindowID p_window_id) {
	ERR_FAIL_INDEX(p_window_id, (int)windows.size());

	GLWindow &glwindow = windows[p_window_id];

	if (!glwindow.initialized) {
		return;
	}

	glwindow.initialized = false;

	ERR_FAIL_INDEX(glwindow.gldisplay_id, (int)displays.size());
	GLDisplay &gldisplay = displays[glwindow.gldisplay_id];

	if (glwindow.egl_surface != EGL_NO_SURFACE) {
		eglDestroySurface(gldisplay.egl_display, glwindow.egl_surface);
		glwindow.egl_surface = nullptr;
	}
}

void EGLManager::release_current() {
	if (!current_window) {
		return;
	}

	GLDisplay &current_display = displays[current_window->gldisplay_id];

	eglMakeCurrent(current_display.egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void EGLManager::swap_buffers() {
	if (!current_window) {
		return;
	}

	if (!current_window->initialized) {
		WARN_PRINT("Current OpenGL window is uninitialized!");
		return;
	}

	GLDisplay &current_display = displays[current_window->gldisplay_id];

	eglSwapBuffers(current_display.egl_display, current_window->egl_surface);
}

void EGLManager::window_make_current(DisplayServerEnums::WindowID p_window_id) {
	if (p_window_id == DisplayServerEnums::INVALID_WINDOW_ID) {
		return;
	}

	GLWindow &glwindow = windows[p_window_id];

	if (&glwindow == current_window || !glwindow.initialized) {
		return;
	}

	current_window = &glwindow;

	GLDisplay &current_display = displays[current_window->gldisplay_id];

	eglMakeCurrent(current_display.egl_display, current_window->egl_surface, current_window->egl_surface, current_display.egl_context);

#ifdef WINDOWS_ENABLED
	RasterizerGLES3::set_screen_flipped_y(glwindow.flipped_y);
#endif
}

void EGLManager::set_use_vsync(bool p_use) {
	// We need an active window to get a display to set the vsync.
	if (!current_window) {
		return;
	}

	GLDisplay &disp = displays[current_window->gldisplay_id];

	int swap_interval = p_use ? 1 : 0;

	if (!eglSwapInterval(disp.egl_display, swap_interval)) {
		WARN_PRINT("Could not set V-Sync mode.");
	}

	use_vsync = p_use;
}

bool EGLManager::is_using_vsync() const {
	return use_vsync;
}

EGLContext EGLManager::get_context(DisplayServerEnums::WindowID p_window_id) {
	GLWindow &glwindow = windows[p_window_id];

	if (!glwindow.initialized) {
		return EGL_NO_CONTEXT;
	}

	GLDisplay &display = displays[glwindow.gldisplay_id];

	return display.egl_context;
}

EGLDisplay EGLManager::get_display(DisplayServerEnums::WindowID p_window_id) {
	GLWindow &glwindow = windows[p_window_id];

	if (!glwindow.initialized) {
		return EGL_NO_CONTEXT;
	}

	GLDisplay &display = displays[glwindow.gldisplay_id];

	return display.egl_display;
}

EGLConfig EGLManager::get_config(DisplayServerEnums::WindowID p_window_id) {
	GLWindow &glwindow = windows[p_window_id];

	if (!glwindow.initialized) {
		return nullptr;
	}

	GLDisplay &display = displays[glwindow.gldisplay_id];

	return display.egl_config;
}

Error EGLManager::initialize(void *p_native_display) {
#if defined(GLAD_ENABLED) && !defined(EGL_STATIC)
	// Loading EGL with a new display gets us just the bare minimum API. We'll then
	// have to temporarily get a proper display and reload EGL once again to
	// initialize everything else.
	if (!gladLoaderLoadEGL(EGL_NO_DISPLAY)) {
		ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "Can't load EGL dynamic library.");
	}

	EGLDisplay tmp_display = EGL_NO_DISPLAY;

	if (GLAD_EGL_EXT_platform_base) {
#ifdef EGL_EXT_platform_base
		// eglGetPlatformDisplayEXT wants its attributes as EGLint.
		Vector<EGLint> attribs;
		for (const EGLAttrib &attrib : _get_platform_display_attributes()) {
			attribs.push_back((EGLint)attrib);
		}
		tmp_display = eglGetPlatformDisplayEXT(_get_platform_extension_enum(), p_native_display, attribs.ptr());
#endif // EGL_EXT_platform_base
	} else {
		WARN_PRINT("EGL: EGL_EXT_platform_base not found during init, using default platform.");
		EGLNativeDisplayType *native_display_type = (EGLNativeDisplayType *)p_native_display;
		tmp_display = eglGetDisplay(*native_display_type);
	}

	if (tmp_display == EGL_NO_DISPLAY) {
		eglTerminate(tmp_display);
		ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "Can't get a valid initial EGL display.");
	}

	eglInitialize(tmp_display, nullptr, nullptr);

	int version = gladLoaderLoadEGL(tmp_display);
	if (!version) {
		eglTerminate(tmp_display);
		ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "Can't load EGL dynamic library.");
	}

	int major = GLAD_VERSION_MAJOR(version);
	int minor = GLAD_VERSION_MINOR(version);

	print_verbose(vformat("Loaded EGL %d.%d", major, minor));

	ERR_FAIL_COND_V_MSG(!GLAD_EGL_VERSION_1_4, ERR_UNAVAILABLE, vformat("EGL version is too old! %d.%d < 1.4", major, minor));

	eglTerminate(tmp_display);
#endif

#ifdef EGL_ANDROID_blob_cache
	shader_cache_dir = Engine::get_shader_cache_path();
	if (shader_cache_dir.is_empty()) {
		shader_cache_dir = "user://";
	}
	Error err = OK;
	Ref<DirAccess> da = DirAccess::open(shader_cache_dir);
	if (da.is_null()) {
		ERR_PRINT("EGL: Can't create shader cache folder, no shader caching will happen: " + shader_cache_dir);
		shader_cache_dir = String();
	} else {
		err = da->change_dir(String("shader_cache").path_join("EGL"));
		if (err != OK) {
			err = da->make_dir_recursive(String("shader_cache").path_join("EGL"));
		}
		if (err != OK) {
			ERR_PRINT("EGL: Can't create shader cache folder, no shader caching will happen: " + shader_cache_dir);
			shader_cache_dir = String();
		} else {
			shader_cache_dir = shader_cache_dir.path_join(String("shader_cache").path_join("EGL"));
		}
	}
#endif

	String client_extensions_string = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	// If the above method fails, we don't support client extensions, so there's nothing to check.
	if (eglGetError() == EGL_SUCCESS) {
		const char *platform = _get_platform_extension_name();
		if (!client_extensions_string.split(" ").has(platform)) {
			ERR_FAIL_V_MSG(ERR_UNAVAILABLE, vformat("EGL platform extension \"%s\" not found.", platform));
		}
	}

	return OK;
}

EGLManager::EGLManager() {
}

EGLManager::~EGLManager() {
	for (unsigned int i = 0; i < displays.size(); i++) {
		eglTerminate(displays[i].egl_display);
	}
}

#endif // EGL_ENABLED

// ============================================================================
// Merged from drivers/pulseaudio/audio_driver_pulseaudio.cpp
// ============================================================================

#include "platform/linuxbsd/os_linuxbsd.h"

#ifdef PULSEAUDIO_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs_binary.h"
#include "core/os/os.h"
#include "core/version.h"

#ifdef ALSAMIDI_ENABLED
#ifdef SOWRAP_ENABLED
#include "drivers/alsa/asound-so_wrap.h"
#else
#include <alsa/asoundlib.h>
#endif
#endif

void AudioDriverPulseAudio::pa_state_cb(pa_context* c, void* userdata)
{
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(userdata);

	switch (pa_context_get_state(c)) {
	case PA_CONTEXT_TERMINATED:
		print_verbose("PulseAudio: context terminated");
		ad->pa_ready = -1;
		break;
	case PA_CONTEXT_FAILED:
		print_verbose("PulseAudio: context failed");
		ad->pa_ready = -1;
		break;
	case PA_CONTEXT_READY:
		print_verbose("PulseAudio: context ready");
		ad->pa_ready = 1;
		break;
	default:
		print_verbose("PulseAudio: context other");
		// TODO: Check if we want to handle some of the other
		// PA context states like PA_CONTEXT_UNCONNECTED.
		break;
	}
}

void AudioDriverPulseAudio::pa_sink_info_cb(
	pa_context* c, const pa_sink_info* l, int eol, void* userdata)
{
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(userdata);

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) {
		return;
	}

	// If eol is set to a negative number there's an error.
	if (eol < 0) {
		ERR_PRINT("PulseAudio: sink info error: " + String(pa_strerror(pa_context_errno(c))));
		ad->pa_status--;
		return;
	}

	ad->pa_map = l->channel_map;
	ad->pa_status++;
}

void AudioDriverPulseAudio::pa_source_info_cb(
	pa_context* c, const pa_source_info* l, int eol, void* userdata)
{
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(userdata);

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) {
		return;
	}

	// If eol is set to a negative number there's an error.
	if (eol < 0) {
		ERR_PRINT("PulseAudio: sink info error: " + String(pa_strerror(pa_context_errno(c))));
		ad->pa_status--;
		return;
	}

	ad->pa_rec_map = l->channel_map;
	ad->pa_status++;
}

void AudioDriverPulseAudio::pa_server_info_cb(
	pa_context* c, const pa_server_info* i, void* userdata)
{
	ERR_FAIL_NULL_MSG(i, "PulseAudio server info is null.");
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(userdata);

	ad->default_input_device = i->default_source_name;
	ad->default_output_device = i->default_sink_name;
	ad->pa_status++;
}

Error AudioDriverPulseAudio::detect_channels(bool input)
{
	pa_channel_map_init_stereo(input ? &pa_rec_map : &pa_map);

	String device = input ? input_device_name : output_device_name;
	if (device == "Default") {
		// Get the default output device name
		pa_status = 0;
		pa_operation* pa_op = pa_context_get_server_info(
			pa_ctx, &AudioDriverPulseAudio::pa_server_info_cb, (void*)this);
		if (pa_op) {
			while (pa_status == 0) {
				int ret = pa_mainloop_iterate(pa_ml, 1, nullptr);
				if (ret < 0) {
					ERR_PRINT("pa_mainloop_iterate error");
				}
			}

			pa_operation_unref(pa_op);
		}
		else {
			ERR_PRINT("pa_context_get_server_info error: " +
					  String(pa_strerror(pa_context_errno(pa_ctx))));
			return FAILED;
		}
	}

	if (device == "Default") {
		device = input ? default_input_device : default_output_device;
	}
	print_verbose("PulseAudio: Detecting channels for device: " + device);

	CharString device_utf8 = device.utf8();

	// Now using the device name get the amount of channels
	pa_status = 0;
	pa_operation* pa_op;
	if (input) {
		pa_op = pa_context_get_source_info_by_name(
			pa_ctx, device_utf8.get_data(), &AudioDriverPulseAudio::pa_source_info_cb, (void*)this);
	}
	else {
		pa_op = pa_context_get_sink_info_by_name(
			pa_ctx, device_utf8.get_data(), &AudioDriverPulseAudio::pa_sink_info_cb, (void*)this);
	}

	if (pa_op) {
		while (pa_status == 0) {
			int ret = pa_mainloop_iterate(pa_ml, 1, nullptr);
			if (ret < 0) {
				ERR_PRINT("pa_mainloop_iterate error");
			}
		}

		pa_operation_unref(pa_op);

		if (pa_status == -1) {
			return FAILED;
		}
	}
	else {
		if (input) {
			ERR_PRINT("pa_context_get_source_info_by_name error");
		}
		else {
			ERR_PRINT("pa_context_get_sink_info_by_name error");
		}
	}

	return OK;
}

Error AudioDriverPulseAudio::init_output_device()
{
	// If there is a specified output device, check that it is really present
	if (output_device_name != "Default") {
		PackedStringArray list = get_output_device_list();
		if (!list.has(output_device_name)) {
			output_device_name = "Default";
			new_output_device = "Default";
		}
	}

	// Detect the amount of channels PulseAudio is using
	// Note: If using an even amount of channels (2, 4, etc) channels and pa_map.channels will be
	// equal, if not then pa_map.channels will have the real amount of channels PulseAudio is using
	// and channels will have the amount of channels Godot is using (in this case it's
	// pa_map.channels + 1)
	Error err = detect_channels();
	if (err != OK) {
		// This most likely means there are no sinks.
		ERR_PRINT("PulseAudio: init_output_device failed to detect number of output channels");
		return err;
	}

	switch (pa_map.channels) {
	case 1: // Mono
	case 3: // Surround 2.1
	case 5: // Surround 5.0
	case 7: // Surround 7.0
		channels = pa_map.channels + 1;
		break;

	case 2: // Stereo
	case 4: // Surround 4.0
	case 6: // Surround 5.1
	case 8: // Surround 7.1
		channels = pa_map.channels;
		break;

	default:
		WARN_PRINT("PulseAudio: Unsupported number of output channels: " + itos(pa_map.channels));
		pa_channel_map_init_stereo(&pa_map);
		channels = 2;
		break;
	}

	int tmp_latency = Engine::get_audio_output_latency();
	buffer_frames = Math::closest_power_of_2(tmp_latency * mix_rate / 1000);
	pa_buffer_size = buffer_frames * pa_map.channels;

	print_verbose("PulseAudio: detected " + itos(pa_map.channels) + " output channels");
	print_verbose("PulseAudio: audio buffer frames: " + itos(buffer_frames) +
				  " calculated output latency: " + itos(buffer_frames * 1000 / mix_rate) + "ms");

	pa_sample_spec spec;
	spec.format = PA_SAMPLE_S16LE;
	spec.channels = pa_map.channels;
	spec.rate = mix_rate;
	pa_map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
	pa_map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
	pa_map.map[2] = PA_CHANNEL_POSITION_FRONT_CENTER;
	pa_map.map[3] = PA_CHANNEL_POSITION_LFE;
	pa_map.map[4] = PA_CHANNEL_POSITION_REAR_LEFT;
	pa_map.map[5] = PA_CHANNEL_POSITION_REAR_RIGHT;
	pa_map.map[6] = PA_CHANNEL_POSITION_SIDE_LEFT;
	pa_map.map[7] = PA_CHANNEL_POSITION_SIDE_RIGHT;

	pa_str = pa_stream_new(pa_ctx, "Sound", &spec, &pa_map);
	if (pa_str == nullptr) {
		ERR_PRINT(
			"PulseAudio: pa_stream_new error: " + String(pa_strerror(pa_context_errno(pa_ctx))));
		ERR_FAIL_V(ERR_CANT_OPEN);
	}

	pa_buffer_attr attr;
	// set to appropriate buffer length (in bytes) from global settings
	// Note: PulseAudio defaults to 4 fragments, which means that the actual
	// latency is tlength / fragments. It seems that the PulseAudio has no way
	// to get the fragments number so we're hardcoding this to the default of 4
	const int fragments = 4;
	attr.tlength = pa_buffer_size * sizeof(int16_t) * fragments;
	// set them to be automatically chosen
	attr.prebuf = (uint32_t)-1;
	attr.maxlength = (uint32_t)-1;
	attr.minreq = (uint32_t)-1;

	CharString output_device_name_nondefault =
		output_device_name == "Default" ? CharString() : output_device_name.utf8();
	const char* dev = output_device_name_nondefault.ptr(); // nullptr on CharString()

	pa_stream_flags flags = pa_stream_flags(
		PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE);
	int error_code = pa_stream_connect_playback(pa_str, dev, &attr, flags, nullptr, nullptr);
	ERR_FAIL_COND_V(error_code < 0, ERR_CANT_OPEN);

	samples_in.resize(buffer_frames * channels);
	samples_out.resize(pa_buffer_size);

	// Reset audio input to keep synchronization.
	input_position = 0;
	input_size = 0;

	return OK;
}

Error AudioDriverPulseAudio::init()
{
#ifdef SOWRAP_ENABLED
#ifdef DEBUG_ENABLED
	int dylibloader_verbose = 1;
#else
	int dylibloader_verbose = 0;
#endif
#ifdef ALSAMIDI_ENABLED
	// If using PulseAudio with ALSA MIDI, we need to initialize ALSA as well
	initialize_asound(dylibloader_verbose);
#endif
	if (initialize_pulse(dylibloader_verbose)) {
		return ERR_CANT_OPEN;
	}
#endif
	bool ver_ok = false;
	String version = String::utf8(pa_get_library_version());
	Vector<String> ver_parts = version.split(".");
	if (ver_parts.size() >= 2) {
		ver_ok = (ver_parts[0].to_int() >= 8); // 8.0.0
	}
	print_verbose(vformat("PulseAudio %s detected.", version));
	if (!ver_ok) {
		print_verbose("Unsupported PulseAudio library version!");
		return ERR_CANT_OPEN;
	}

	active.clear();
	exit_thread.clear();

	mix_rate = _get_configured_mix_rate();

	pa_ml = pa_mainloop_new();
	ERR_FAIL_NULL_V(pa_ml, ERR_CANT_OPEN);

	String context_name;
	if (Engine::is_editor_hint()) {
		context_name = VLTR_VERSION_NAME " Editor";
	}
	else {
		if (context_name.is_empty()) {
			context_name = VLTR_VERSION_NAME " Project";
		}
	}

	pa_ctx = pa_context_new(pa_mainloop_get_api(pa_ml), context_name.utf8().ptr());
	ERR_FAIL_NULL_V(pa_ctx, ERR_CANT_OPEN);

	pa_ready = 0;
	pa_context_set_state_callback(pa_ctx, pa_state_cb, (void*)this);

	int ret = pa_context_connect(pa_ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
	if (ret < 0) {
		if (pa_ctx) {
			pa_context_unref(pa_ctx);
			pa_ctx = nullptr;
		}

		if (pa_ml) {
			pa_mainloop_free(pa_ml);
			pa_ml = nullptr;
		}

		return ERR_CANT_OPEN;
	}

	while (pa_ready == 0) {
		ret = pa_mainloop_iterate(pa_ml, 1, nullptr);
		if (ret < 0) {
			ERR_PRINT("pa_mainloop_iterate error");
		}
	}

	if (pa_ready < 0) {
		if (pa_ctx) {
			pa_context_disconnect(pa_ctx);
			pa_context_unref(pa_ctx);
			pa_ctx = nullptr;
		}

		if (pa_ml) {
			pa_mainloop_free(pa_ml);
			pa_ml = nullptr;
		}

		return ERR_CANT_OPEN;
	}

	init_output_device();
	thread.start(AudioDriverPulseAudio::thread_func, this);

	return OK;
}

float AudioDriverPulseAudio::get_latency()
{
	lock();

	if (pa_str == nullptr) {
		unlock();
		return 0;
	}

	pa_usec_t pa_lat = 0;
	if (pa_stream_get_state(pa_str) == PA_STREAM_READY) {
		int negative = 0;

		if (pa_stream_get_latency(pa_str, &pa_lat, &negative) >= 0) {
			if (negative) {
				pa_lat = 0;
			}
		}
	}

	if (pa_lat > 0) {
		latency = double(pa_lat) / 1000000.0;
	}

	unlock();
	return latency;
}

void AudioDriverPulseAudio::thread_func(void* p_udata)
{
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(p_udata);
	unsigned int write_ofs = 0;
	size_t avail_bytes = 0;
	uint64_t default_device_msec = OS::get_ticks_msec();
	int last_reported_errno = PA_OK;

	while (!ad->exit_thread.is_set()) {
		size_t read_bytes = 0;
		size_t written_bytes = 0;

		if (avail_bytes == 0) {
			ad->lock();
			ad->start_counting_ticks();

			if (!ad->active.is_set()) {
				ad->samples_out.fill(0);
			}
			else {
				ad->audio_server_process(ad->buffer_frames, ad->samples_in.ptrw());

				int16_t* out_ptr = ad->samples_out.ptrw();

				if (ad->channels == ad->pa_map.channels) {
					for (unsigned int i = 0; i < ad->pa_buffer_size; i++) {
						out_ptr[i] = ad->samples_in[i] >> 16;
					}
				}
				else {
					// Uneven amount of channels
					unsigned int in_idx = 0;
					unsigned int out_idx = 0;

					for (unsigned int i = 0; i < ad->buffer_frames; i++) {
						for (int j = 0; j < ad->pa_map.channels - 1; j++) {
							out_ptr[out_idx++] = ad->samples_in[in_idx++] >> 16;
						}
						uint32_t l = ad->samples_in[in_idx++] >> 16;
						uint32_t r = ad->samples_in[in_idx++] >> 16;
						out_ptr[out_idx++] = (l + r) / 2;
					}
				}
			}

			avail_bytes = ad->pa_buffer_size * sizeof(int16_t);
			write_ofs = 0;
			ad->stop_counting_ticks();
			ad->unlock();
		}

		ad->lock();
		ad->start_counting_ticks();

		int ret;
		do {
			ret = pa_mainloop_iterate(ad->pa_ml, 0, nullptr);
		} while (ret > 0);

		if (avail_bytes > 0 && pa_stream_get_state(ad->pa_str) == PA_STREAM_READY) {
			size_t bytes = pa_stream_writable_size(ad->pa_str);
			if (bytes > 0) {
				size_t bytes_to_write = MIN(bytes, avail_bytes);
				const void* ptr = ad->samples_out.ptr();
				ret = pa_stream_write(ad->pa_str, (char*)ptr + write_ofs, bytes_to_write, nullptr,
					0LL, PA_SEEK_RELATIVE);
				if (ret != 0) {
					ERR_PRINT("PulseAudio: pa_stream_write error: " + String(pa_strerror(ret)));
				}
				else {
					avail_bytes -= bytes_to_write;
					write_ofs += bytes_to_write;
					written_bytes += bytes_to_write;
				}
			}
		}

		// User selected a new output device, finish the current one so we'll init the new output
		// device
		if (ad->output_device_name != ad->new_output_device) {
			ad->output_device_name = ad->new_output_device;
			ad->finish_output_device();

			Error err = ad->init_output_device();
			if (err != OK) {
				ERR_PRINT("PulseAudio: init_output_device error");
				ad->output_device_name = "Default";
				ad->new_output_device = "Default";

				err = ad->init_output_device();
				if (err != OK) {
					ad->active.clear();
					ad->exit_thread.set();
					break;
				}
			}

			avail_bytes = 0;
			write_ofs = 0;
		}

		// If we're using the default output device, check that the current output device is still
		// the default
		if (ad->output_device_name == "Default") {
			uint64_t msec = OS::get_ticks_msec();
			if (msec > (default_device_msec + 1000)) {
				String old_default_device = ad->default_output_device;

				default_device_msec = msec;

				ad->pa_status = 0;
				pa_operation* pa_op = pa_context_get_server_info(
					ad->pa_ctx, &AudioDriverPulseAudio::pa_server_info_cb, (void*)ad);
				if (pa_op) {
					while (ad->pa_status == 0) {
						ret = pa_mainloop_iterate(ad->pa_ml, 1, nullptr);
						if (ret < 0) {
							ERR_PRINT("pa_mainloop_iterate error");
						}
					}

					pa_operation_unref(pa_op);
				}
				else {
					int pa_errno = pa_context_errno(ad->pa_ctx);
					if (pa_errno != last_reported_errno) {
						last_reported_errno = pa_errno;
						ERR_PRINT(
							"pa_context_get_server_info error: " + String(pa_strerror(pa_errno)));
					}
				}

				if (old_default_device != ad->default_output_device) {
					ad->finish_output_device();

					Error err = ad->init_output_device();
					if (err != OK) {
						ERR_PRINT("PulseAudio: init_output_device error");
						ad->active.clear();
						ad->exit_thread.set();
						break;
					}

					avail_bytes = 0;
					write_ofs = 0;
				}
			}
		}

		if (ad->pa_rec_str && pa_stream_get_state(ad->pa_rec_str) == PA_STREAM_READY) {
			size_t bytes = pa_stream_readable_size(ad->pa_rec_str);
			if (bytes > 0) {
				const void* ptr = nullptr;
				size_t maxbytes = ad->input_buffer.size() * sizeof(int16_t);

				bytes = MIN(bytes, maxbytes);
				ret = pa_stream_peek(ad->pa_rec_str, &ptr, &bytes);
				if (ret != 0) {
					ERR_PRINT("pa_stream_peek error");
				}
				else {
					int16_t* srcptr = (int16_t*)ptr;
					for (size_t i = bytes >> 1; i > 0; i--) {
						int32_t sample = int32_t(*srcptr++) << 16;
						ad->input_buffer_write(sample);

						if (ad->pa_rec_map.channels == 1) {
							// In case input device is single channel convert it to Stereo
							ad->input_buffer_write(sample);
						}
					}

					read_bytes += bytes;
					ret = pa_stream_drop(ad->pa_rec_str);
					if (ret != 0) {
						ERR_PRINT("pa_stream_drop error");
					}
				}
			}

			// User selected a new input device, finish the current one so we'll init the new input
			// device (If `AudioServer.set_input_device()` did not set the value when the microphone
			// was running,
			//  this section with its problematic error handling could be deleted.)
			if (ad->input_device_name != ad->new_input_device) {
				ad->input_device_name = ad->new_input_device;
				ad->finish_input_device();

				Error err = ad->init_input_device();
				if (err != OK) {
					ERR_PRINT("PulseAudio: init_input_device error");
					ad->input_device_name = "Default";
					ad->new_input_device = "Default";

					err = ad->init_input_device();
					if (err != OK) {
						ad->active.clear();
						ad->exit_thread.set();
						break;
					}
				}
			}
		}

		ad->stop_counting_ticks();
		ad->unlock();

		// Let the thread rest a while if we haven't read or write anything
		if (written_bytes == 0 && read_bytes == 0) {
			OS::delay_usec(1000);
		}
	}
}

void AudioDriverPulseAudio::start() { active.set(); }

int AudioDriverPulseAudio::get_mix_rate() const { return mix_rate; }

AudioDriver::SpeakerMode AudioDriverPulseAudio::get_speaker_mode() const
{
	return get_speaker_mode_by_total_channels(channels);
}

void AudioDriverPulseAudio::pa_sinklist_cb(
	pa_context* c, const pa_sink_info* l, int eol, void* userdata)
{
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(userdata);

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) {
		return;
	}

	ad->pa_devices.push_back(l->name);
	ad->pa_status++;
}

PackedStringArray AudioDriverPulseAudio::get_output_device_list()
{
	pa_devices.clear();
	pa_devices.push_back("Default");

	if (pa_ctx == nullptr) {
		return pa_devices;
	}

	lock();

	// Get the output device list
	pa_status = 0;
	pa_operation* pa_op = pa_context_get_sink_info_list(pa_ctx, pa_sinklist_cb, (void*)this);
	if (pa_op) {
		while (pa_status == 0) {
			int ret = pa_mainloop_iterate(pa_ml, 1, nullptr);
			if (ret < 0) {
				ERR_PRINT("pa_mainloop_iterate error");
			}
		}

		pa_operation_unref(pa_op);
	}
	else {
		ERR_PRINT("pa_context_get_server_info error");
	}

	unlock();

	return pa_devices;
}

String AudioDriverPulseAudio::get_output_device() { return output_device_name; }

void AudioDriverPulseAudio::set_output_device(const String& p_name)
{
	lock();
	new_output_device = p_name;
	unlock();
}

void AudioDriverPulseAudio::lock() { mutex.lock(); }

void AudioDriverPulseAudio::unlock() { mutex.unlock(); }

void AudioDriverPulseAudio::finish_output_device()
{
	if (pa_str) {
		pa_stream_disconnect(pa_str);
		pa_stream_unref(pa_str);
		pa_str = nullptr;
	}
}

void AudioDriverPulseAudio::finish()
{
	if (!thread.is_started()) {
		return;
	}

	exit_thread.set();
	if (thread.is_started()) {
		thread.wait_to_finish();
	}

	finish_output_device();

	if (pa_ctx) {
		pa_context_disconnect(pa_ctx);
		pa_context_unref(pa_ctx);
		pa_ctx = nullptr;
	}

	if (pa_ml) {
		pa_mainloop_free(pa_ml);
		pa_ml = nullptr;
	}
}

Error AudioDriverPulseAudio::init_input_device()
{
	if (pa_rec_str) {
		return ERR_ALREADY_IN_USE;
	}

	// If there is a specified input device, check that it is really present
	if (input_device_name != "Default") {
		PackedStringArray list = get_input_device_list();
		if (!list.has(input_device_name)) {
			input_device_name = "Default";
			new_input_device = "Default";
		}
	}

	detect_channels(true);
	switch (pa_rec_map.channels) {
	case 1: // Mono
	case 2: // Stereo
		break;

	default:
		WARN_PRINT(
			"PulseAudio: Unsupported number of input channels: " + itos(pa_rec_map.channels));
		pa_channel_map_init_stereo(&pa_rec_map);
		break;
	}

	print_verbose("PulseAudio: detected " + itos(pa_rec_map.channels) + " input channels");

	pa_sample_spec spec;

	spec.format = PA_SAMPLE_S16LE;
	spec.channels = pa_rec_map.channels;
	spec.rate = mix_rate;

	int input_latency = 30;
	int input_buffer_frames = Math::closest_power_of_2(input_latency * mix_rate / 1000);
	int input_buffer_size = input_buffer_frames * spec.channels;

	pa_buffer_attr attr = {};
	attr.maxlength = (uint32_t)-1;
	attr.fragsize = input_buffer_size * sizeof(int16_t);

	pa_rec_str = pa_stream_new(pa_ctx, "Record", &spec, &pa_rec_map);
	if (pa_rec_str == nullptr) {
		ERR_PRINT(
			"PulseAudio: pa_stream_new error: " + String(pa_strerror(pa_context_errno(pa_ctx))));
		ERR_FAIL_V(ERR_CANT_OPEN);
	}

	CharString output_device_name_nondefault =
		output_device_name == "Default" ? CharString() : output_device_name.utf8();
	const char* dev = output_device_name_nondefault.ptr(); // nullptr on CharString()

	pa_stream_flags flags = pa_stream_flags(
		PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_ADJUST_LATENCY | PA_STREAM_AUTO_TIMING_UPDATE);
	int error_code = pa_stream_connect_record(pa_rec_str, dev, &attr, flags);
	if (error_code < 0) {
		ERR_PRINT("PulseAudio: pa_stream_connect_record error: " + String(pa_strerror(error_code)));
		ERR_FAIL_V(ERR_CANT_OPEN);
	}

	input_buffer_init(input_buffer_frames);

	print_verbose("PulseAudio: detected " + itos(pa_rec_map.channels) + " input channels");
	print_verbose("PulseAudio: input buffer frames: " + itos(input_buffer_frames) +
				  " calculated latency: " + itos(input_buffer_frames * 1000 / mix_rate) + "ms");

	return OK;
}

void AudioDriverPulseAudio::finish_input_device()
{
	if (pa_rec_str) {
		int ret = pa_stream_disconnect(pa_rec_str);
		if (ret != 0) {
			ERR_PRINT("PulseAudio: pa_stream_disconnect error: " + String(pa_strerror(ret)));
		}
		pa_stream_unref(pa_rec_str);
		pa_rec_str = nullptr;
	}
}

Error AudioDriverPulseAudio::input_start()
{
	lock();
	Error err = init_input_device();
	unlock();

	return err;
}

Error AudioDriverPulseAudio::input_stop()
{
	lock();
	finish_input_device();
	unlock();

	return OK;
}

void AudioDriverPulseAudio::pa_sourcelist_cb(
	pa_context* c, const pa_source_info* l, int eol, void* userdata)
{
	AudioDriverPulseAudio* ad = static_cast<AudioDriverPulseAudio*>(userdata);

	// If eol is set to a positive number, you're at the end of the list
	if (eol > 0) {
		return;
	}

	if (l->monitor_of_sink == PA_INVALID_INDEX) {
		ad->pa_rec_devices.push_back(l->name);
	}

	ad->pa_status++;
}

PackedStringArray AudioDriverPulseAudio::get_input_device_list()
{
	pa_rec_devices.clear();
	pa_rec_devices.push_back("Default");

	if (pa_ctx == nullptr) {
		return pa_rec_devices;
	}

	lock();

	// Get the device list
	pa_status = 0;
	pa_operation* pa_op = pa_context_get_source_info_list(pa_ctx, pa_sourcelist_cb, (void*)this);
	if (pa_op) {
		while (pa_status == 0) {
			int ret = pa_mainloop_iterate(pa_ml, 1, nullptr);
			if (ret < 0) {
				ERR_PRINT("pa_mainloop_iterate error");
			}
		}

		pa_operation_unref(pa_op);
	}
	else {
		ERR_PRINT("pa_context_get_server_info error");
	}

	unlock();

	return pa_rec_devices;
}

String AudioDriverPulseAudio::get_input_device()
{
	lock();
	String name = input_device_name;
	unlock();

	return name;
}

void AudioDriverPulseAudio::set_input_device(const String& p_name)
{
	lock();
	new_input_device = p_name;
	unlock();
}

AudioDriverPulseAudio::AudioDriverPulseAudio()
{
	samples_in.clear();
	samples_out.clear();
}

#endif // PULSEAUDIO_ENABLED



// ============================================================================
// Merged from drivers/windows/file_access_windows.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "platform/windows/os_windows.h"

#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/string/print_string.h"

#include <io.h>
#include <share.h> // _SH_DENYNO
#include <shlwapi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <tchar.h>

#include <cerrno>
#include <cwchar>

#ifdef _MSC_VER
#define S_ISREG(m) ((m) & _S_IFREG)
#endif

void FileAccessWindows::check_errors(bool p_write) const {
	ERR_FAIL_NULL(f);

	last_error = OK;
	if (ferror(f)) {
		if (p_write) {
			last_error = ERR_FILE_CANT_WRITE;
		} else {
			last_error = ERR_FILE_CANT_READ;
		}
	}
	if (!p_write && feof(f)) {
		last_error = ERR_FILE_EOF;
	}
}

bool FileAccessWindows::is_path_invalid(const String &p_path) {
	// Check for invalid operating system file.
	String fname = p_path.get_file().to_lower();
	if (fname.ends_with(".") || fname.ends_with(" ")) {
		return true;
	}

	int dot = fname.find_char('.');
	if (dot != -1) {
		fname = fname.substr(0, dot);
	}
	return invalid_files.has(fname);
}

String FileAccessWindows::fix_path(const String &p_path) const {
	String r_path = FileAccess::fix_path(p_path);

	if (r_path.is_relative_path()) {
		Char16String current_dir_name;
		size_t str_len = GetCurrentDirectoryW(0, nullptr);
		current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(current_dir_name.size(), (LPWSTR)current_dir_name.ptrw());
		r_path = String::utf16((const char16_t *)current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/').path_join(r_path);
	}
	r_path = r_path.simplify_path();
	r_path = r_path.replace_char('/', '\\');
	if (!r_path.is_network_share_path() && !r_path.begins_with(R"(\\?\)")) {
		r_path = R"(\\?\)" + r_path;
	}
	return r_path;
}

Error FileAccessWindows::open_internal(const String &p_path, int p_mode_flags) {
	if (is_path_invalid(p_path)) {
#ifdef DEBUG_ENABLED
		if (p_mode_flags != READ) {
			WARN_PRINT("The path :" + p_path + " is a reserved Windows system pipe, so it can't be used for creating files.");
		}
#endif
		return ERR_INVALID_PARAMETER;
	}

	_close();

	path_src = p_path;
	path = fix_path(p_path);

	const WCHAR *mode_string;

	if (p_mode_flags == READ) {
		mode_string = L"rb";
	} else if (p_mode_flags == WRITE) {
		mode_string = L"wb";
	} else if (p_mode_flags == READ_WRITE) {
		mode_string = L"rb+";
	} else if (p_mode_flags == WRITE_READ) {
		mode_string = L"wb+";
	} else {
		return ERR_INVALID_PARAMETER;
	}

	if (path.ends_with(":\\") || path.ends_with(":")) {
		return ERR_FILE_CANT_OPEN;
	}
	DWORD file_attr = GetFileAttributesW((LPCWSTR)(path.utf16().get_data()));
	if (file_attr != INVALID_FILE_ATTRIBUTES && (file_attr & FILE_ATTRIBUTE_DIRECTORY)) {
		return ERR_FILE_CANT_OPEN;
	}

#ifdef TOOLS_ENABLED
	// Windows is case insensitive in the default configuration, but other platforms can be sensitive to it
	// To ease cross-platform development, we issue a warning if users try to access
	// a file using the wrong case (which *works* on Windows, but won't on other
	// platforms), we only check for relative paths, or paths in res:// or user://,
	// other paths aren't likely to be portable anyway.
	if (p_mode_flags == READ && (p_path.is_relative_path() || get_access_type() != ACCESS_FILESYSTEM)) {
		String base_path = FileAccess::fix_path(p_path);
		String working_path;
		String proper_path;

		if (get_access_type() == ACCESS_RESOURCES) {
			if (ProjectSettings::is_initialized()) {
				working_path = ProjectSettings::get_resource_path();
				if (!working_path.is_empty()) {
					base_path = working_path.path_to_file(base_path);
				}
			}
			proper_path = "res://";
		} else if (get_access_type() == ACCESS_USERDATA) {
			working_path = OS::get_user_data_dir();
			if (!working_path.is_empty()) {
				base_path = working_path.path_to_file(base_path);
			}
			proper_path = "user://";
		}
		working_path = fix_path(working_path);

		WIN32_FIND_DATAW d;
		Vector<String> parts = base_path.simplify_path().split("/");

		bool mismatch = false;

		for (const String &part : parts) {
			working_path = working_path + "\\" + part;

			HANDLE fnd = FindFirstFileW((LPCWSTR)(working_path.utf16().get_data()), &d);
			if (fnd == INVALID_HANDLE_VALUE) {
				mismatch = false;
				break;
			}

			const String fname = String::utf16((const char16_t *)(d.cFileName));

			FindClose(fnd);

			if (!mismatch) {
				mismatch = (part != fname && part.findn(fname) == 0);
			}

			proper_path = proper_path.path_join(fname);
		}

		if (mismatch) {
			WARN_PRINT("Case mismatch opening requested file '" + p_path + "', stored as '" + proper_path + "' in the filesystem. This file will not open when exported to other case-sensitive platforms.");
		}
	}
#endif

	if (is_backup_save_enabled() && p_mode_flags == WRITE) {
		save_path = path;
		// Create a temporary file in the same directory as the target file.
		// Note: do not use GetTempFileNameW, it's not long path aware!
		String tmpfile;
		uint64_t id = OS::get_ticks_usec();
		while (true) {
			tmpfile = path + itos(id++) + ".tmp";
			HANDLE handle = CreateFileW((LPCWSTR)tmpfile.utf16().get_data(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
				break;
			}
			if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION) {
				last_error = ERR_FILE_CANT_WRITE;
				return FAILED;
			}
		}
		path = tmpfile;
	}

	f = _wfsopen((LPCWSTR)(path.utf16().get_data()), mode_string, is_backup_save_enabled() ? ((p_mode_flags == READ) ? _SH_DENYWR : _SH_DENYRW) : _SH_DENYNO);

	if (f == nullptr) {
		switch (errno) {
			case ENOENT: {
				last_error = ERR_FILE_NOT_FOUND;
			} break;
			default: {
				last_error = ERR_FILE_CANT_OPEN;
			} break;
		}
		return last_error;
	} else {
		last_error = OK;
		flags = p_mode_flags;
		return OK;
	}
}

void FileAccessWindows::_close() {
	if (!f) {
		return;
	}

	fclose(f);
	f = nullptr;

	if (!save_path.is_empty()) {
		// This workaround of trying multiple times is added to deal with paranoid Windows
		// antiviruses that love reading just written files even if they are not executable, thus
		// locking the file and preventing renaming from happening.

		bool rename_error = true;
		const Char16String &path_utf16 = path.utf16();
		const Char16String &save_path_utf16 = save_path.utf16();
		for (int i = 0; i < 1000; i++) {
			if (ReplaceFileW((LPCWSTR)(save_path_utf16.get_data()), (LPCWSTR)(path_utf16.get_data()), nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS, nullptr, nullptr)) {
				rename_error = false;
			} else {
				// Either the target exists and is locked (temporarily, hopefully)
				// or it doesn't exist; let's assume the latter before re-trying.
				rename_error = MoveFileW((LPCWSTR)(path_utf16.get_data()), (LPCWSTR)(save_path_utf16.get_data())) == 0;
			}

			if (!rename_error) {
				break;
			}

			OS::delay_usec(1000);
		}

		if (rename_error) {
			if (close_fail_notify) {
				close_fail_notify(save_path);
			}
		}

		save_path = "";

		ERR_FAIL_COND_MSG(rename_error, "Safe save failed. This may be a permissions problem, but also may happen because you are running a paranoid antivirus. If this is the case, please switch to Windows Defender or disable the 'safe save' option in editor settings. This makes it work, but increases the risk of file corruption in a crash.");
	}
}

String FileAccessWindows::get_path() const {
	return path_src;
}

String FileAccessWindows::get_path_absolute() const {
	return path.trim_prefix(R"(\\?\)").replace_char('\\', '/');
}

bool FileAccessWindows::is_open() const {
	return (f != nullptr);
}

void FileAccessWindows::seek(uint64_t p_position) {
	ERR_FAIL_NULL(f);

	if (_fseeki64(f, p_position, SEEK_SET)) {
		check_errors();
	}
	prev_op = 0;
}

void FileAccessWindows::seek_end(int64_t p_position) {
	ERR_FAIL_NULL(f);

	if (_fseeki64(f, p_position, SEEK_END)) {
		check_errors();
	}
	prev_op = 0;
}

uint64_t FileAccessWindows::get_position() const {
	ERR_FAIL_NULL_V_MSG(f, 0, "File must be opened before use.");

	int64_t aux_position = _ftelli64(f);
	if (aux_position < 0) {
		check_errors();
	}
	return aux_position;
}

uint64_t FileAccessWindows::get_length() const {
	ERR_FAIL_NULL_V(f, 0);

	uint64_t pos = get_position();
	_fseeki64(f, 0, SEEK_END);
	uint64_t size = get_position();
	_fseeki64(f, pos, SEEK_SET);

	return size;
}

bool FileAccessWindows::eof_reached() const {
	return feof(f);
}

uint64_t FileAccessWindows::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_NULL_V(f, -1);
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	if (flags == READ_WRITE || flags == WRITE_READ) {
		if (prev_op == WRITE) {
			fflush(f);
		}
		prev_op = READ;
	}

	uint64_t read = fread(p_dst, 1, p_length, f);
	check_errors();

	return read;
}

Error FileAccessWindows::get_error() const {
	return last_error;
}

Error FileAccessWindows::resize(int64_t p_length) {
	ERR_FAIL_NULL_V_MSG(f, FAILED, "File must be opened before use.");
	errno_t res = _chsize_s(_fileno(f), p_length);
	switch (res) {
		case 0:
			return OK;
		case EACCES:
		case EBADF:
			return ERR_FILE_CANT_OPEN;
		case ENOSPC:
			return ERR_OUT_OF_MEMORY;
		case EINVAL:
			return ERR_INVALID_PARAMETER;
		default:
			return FAILED;
	}
}

void FileAccessWindows::flush() {
	ERR_FAIL_NULL(f);

	fflush(f);
	if (prev_op == WRITE) {
		prev_op = 0;
	}
}

bool FileAccessWindows::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_NULL_V(f, false);
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);

	if (flags == READ_WRITE || flags == WRITE_READ) {
		if (prev_op == READ) {
			if (last_error != ERR_FILE_EOF) {
				fseek(f, 0, SEEK_CUR);
			}
		}
		prev_op = WRITE;
	}

	bool res = fwrite(p_src, 1, p_length, f) == (size_t)p_length;
	check_errors(true);
	return res;
}

bool FileAccessWindows::file_exists(const String &p_name) {
	if (is_path_invalid(p_name)) {
		return false;
	}

	String filename = fix_path(p_name);
	DWORD file_attr = GetFileAttributesW((LPCWSTR)(filename.utf16().get_data()));
	return (file_attr != INVALID_FILE_ATTRIBUTES) && !(file_attr & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t FileAccessWindows::_get_modified_time(const String &p_file) {
	if (is_path_invalid(p_file)) {
		return 0;
	}

	String file = fix_path(p_file);
	if (file.ends_with("\\") && file != "\\") {
		file = file.substr(0, file.length() - 1);
	}

	HANDLE handle = CreateFileW((LPCWSTR)(file.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		FILETIME ft_create, ft_write;

		bool status = GetFileTime(handle, &ft_create, nullptr, &ft_write);

		CloseHandle(handle);

		if (status) {
			uint64_t ret = 0;

			// If write time is invalid, fallback to creation time.
			if (ft_write.dwHighDateTime == 0 && ft_write.dwLowDateTime == 0) {
				ret = ft_create.dwHighDateTime;
				ret <<= 32;
				ret |= ft_create.dwLowDateTime;
			} else {
				ret = ft_write.dwHighDateTime;
				ret <<= 32;
				ret |= ft_write.dwLowDateTime;
			}

			const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
			const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

			if (ret >= TICKS_TO_UNIX_EPOCH) {
				return (ret - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
			}
		}
	}

	return 0;
}

uint64_t FileAccessWindows::_get_access_time(const String &p_file) {
	if (is_path_invalid(p_file)) {
		return 0;
	}

	String file = fix_path(p_file);
	if (file.ends_with("\\") && file != "\\") {
		file = file.substr(0, file.length() - 1);
	}

	HANDLE handle = CreateFileW((LPCWSTR)(file.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		FILETIME ft_create, ft_access;

		bool status = GetFileTime(handle, &ft_create, &ft_access, nullptr);

		CloseHandle(handle);

		if (status) {
			uint64_t ret = 0;

			// If access time is invalid, fallback to creation time.
			if (ft_access.dwHighDateTime == 0 && ft_access.dwLowDateTime == 0) {
				ret = ft_create.dwHighDateTime;
				ret <<= 32;
				ret |= ft_create.dwLowDateTime;
			} else {
				ret = ft_access.dwHighDateTime;
				ret <<= 32;
				ret |= ft_access.dwLowDateTime;
			}

			const uint64_t WINDOWS_TICKS_PER_SECOND = 10000000;
			const uint64_t TICKS_TO_UNIX_EPOCH = 116444736000000000LL;

			if (ret >= TICKS_TO_UNIX_EPOCH) {
				return (ret - TICKS_TO_UNIX_EPOCH) / WINDOWS_TICKS_PER_SECOND;
			}
		}
	}

	ERR_FAIL_V_MSG(0, "Failed to get access time for: " + p_file + "");
}

int64_t FileAccessWindows::_get_size(const String &p_file) {
	if (is_path_invalid(p_file)) {
		return 0;
	}

	String file = fix_path(p_file);
	if (file.ends_with("\\") && file != "\\") {
		file = file.substr(0, file.length() - 1);
	}

	DWORD file_attr = GetFileAttributesW((LPCWSTR)(file.utf16().get_data()));
	HANDLE handle = CreateFileW((LPCWSTR)(file.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (handle != INVALID_HANDLE_VALUE && !(file_attr & FILE_ATTRIBUTE_DIRECTORY)) {
		LARGE_INTEGER fsize;

		bool status = GetFileSizeEx(handle, &fsize);

		CloseHandle(handle);

		if (status) {
			return (int64_t)fsize.QuadPart;
		}
	}
	ERR_FAIL_V_MSG(-1, "Failed to get size for: " + p_file + "");
}

uint32_t FileAccessWindows::_get_unix_permissions(const String &p_file) {
	return 0;
}

Error FileAccessWindows::_set_unix_permissions(const String &p_file, uint32_t p_permissions) {
	return ERR_UNAVAILABLE;
}

bool FileAccessWindows::_get_hidden_attribute(const String &p_file) {
	String file = fix_path(p_file);

	DWORD attrib = GetFileAttributesW((LPCWSTR)file.utf16().get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, false, "Failed to get attributes for: " + p_file);
	return (attrib & FILE_ATTRIBUTE_HIDDEN);
}

Error FileAccessWindows::_set_hidden_attribute(const String &p_file, bool p_hidden) {
	String file = fix_path(p_file);
	const Char16String &file_utf16 = file.utf16();

	DWORD attrib = GetFileAttributesW((LPCWSTR)file_utf16.get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, FAILED, "Failed to get attributes for: " + p_file);
	BOOL ok;
	if (p_hidden) {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib | FILE_ATTRIBUTE_HIDDEN);
	} else {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib & ~FILE_ATTRIBUTE_HIDDEN);
	}
	ERR_FAIL_COND_V_MSG(!ok, FAILED, "Failed to set attributes for: " + p_file);

	return OK;
}

bool FileAccessWindows::_get_read_only_attribute(const String &p_file) {
	String file = fix_path(p_file);

	DWORD attrib = GetFileAttributesW((LPCWSTR)file.utf16().get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, false, "Failed to get attributes for: " + p_file);
	return (attrib & FILE_ATTRIBUTE_READONLY);
}

Error FileAccessWindows::_set_read_only_attribute(const String &p_file, bool p_ro) {
	String file = fix_path(p_file);
	const Char16String &file_utf16 = file.utf16();

	DWORD attrib = GetFileAttributesW((LPCWSTR)file_utf16.get_data());
	ERR_FAIL_COND_V_MSG(attrib == INVALID_FILE_ATTRIBUTES, FAILED, "Failed to get attributes for: " + p_file);
	BOOL ok;
	if (p_ro) {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib | FILE_ATTRIBUTE_READONLY);
	} else {
		ok = SetFileAttributesW((LPCWSTR)file_utf16.get_data(), attrib & ~FILE_ATTRIBUTE_READONLY);
	}
	ERR_FAIL_COND_V_MSG(!ok, FAILED, "Failed to set attributes for: " + p_file);

	return OK;
}

PackedByteArray FileAccessWindows::_get_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), PackedByteArray());

	String file = fix_path(p_file);
	file += ":" + p_attribute_name;
	const Char16String &file_utf16 = file.utf16();

	PackedByteArray data;
	HANDLE h = CreateFileW((LPCWSTR)file_utf16.get_data(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h != INVALID_HANDLE_VALUE) {
		size_t bytes_in_buffer = 0;
		const int CHUNK_SIZE = 4096;

		DWORD read = 0;
		for (;;) {
			data.resize(bytes_in_buffer + CHUNK_SIZE);
			bool success = ReadFile(h, data.ptrw() + bytes_in_buffer, CHUNK_SIZE, &read, nullptr);
			if (!success || read == 0) {
				break;
			}
			bytes_in_buffer += read;
		}
		data.resize(bytes_in_buffer);
		CloseHandle(h);
	}
	return data;
}

Error FileAccessWindows::_set_extended_attribute(const String &p_file, const String &p_attribute_name, const PackedByteArray &p_data) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

	String file = fix_path(p_file);
	file += ":" + p_attribute_name;
	const Char16String &file_utf16 = file.utf16();

	HANDLE h = CreateFileW((LPCWSTR)file_utf16.get_data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return FAILED;
	}

	DWORD written = 0;
	bool ok = true;
	if (p_data.size() > 0) {
		ok = WriteFile(h, p_data.ptr(), p_data.size(), &written, nullptr);
	}
	CloseHandle(h);

	ERR_FAIL_COND_V_MSG(!ok || written != p_data.size(), FAILED, "Failed to set extended attributes for: " + p_file);

	return OK;
}

Error FileAccessWindows::_remove_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

	String file = fix_path(p_file);
	file += ":" + p_attribute_name;
	const Char16String &file_utf16 = file.utf16();

	return DeleteFileW((LPCWSTR)(file_utf16.get_data())) != 0 ? OK : FAILED;
}

PackedStringArray FileAccessWindows::_get_extended_attributes_list(const String &p_file) {
	PackedStringArray ret;
	String file = fix_path(p_file);

	char info_block[65536] = {};
	PFILE_STREAM_INFO stream_info = (PFILE_STREAM_INFO)info_block;

	HANDLE h = CreateFileW((LPCWSTR)file.utf16().get_data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return ret;
	}
	BOOL out = GetFileInformationByHandleEx(h, FileStreamInfo, info_block, sizeof(info_block));
	CloseHandle(h);
	if (!out) {
		return ret;
	}
	while (true) {
		if (stream_info->StreamNameLength != 0) {
			String name = String::utf16((const char16_t *)stream_info->StreamName, stream_info->StreamNameLength / sizeof(WCHAR));
			if (name.ends_with(":$DATA")) {
				name = name.trim_prefix(":").trim_suffix(":$DATA");
				if (!name.is_empty()) {
					ret.push_back(name);
				}
			}
		}
		if (stream_info->NextEntryOffset == 0) {
			break;
		}
		stream_info = (PFILE_STREAM_INFO)((LPBYTE)stream_info + stream_info->NextEntryOffset);
	}

	return ret;
}

void FileAccessWindows::close() {
	_close();
}

FileAccessWindows::~FileAccessWindows() {
	_close();
}

HashSet<String> FileAccessWindows::invalid_files;

void FileAccessWindows::initialize() {
	static const char *reserved_files[]{
		"con", "prn", "aux", "nul", "com0", "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9", "com¹", "com²", "com³", "lpt0", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9", "lpt¹", "lpt²", "lpt³", nullptr
	};
	int reserved_file_index = 0;
	while (reserved_files[reserved_file_index] != nullptr) {
		invalid_files.insert(String::utf8(reserved_files[reserved_file_index]));
		reserved_file_index++;
	}

	_setmaxstdio(8192);
	print_verbose(vformat("Maximum number of file handles: %d", _getmaxstdio()));
}

void FileAccessWindows::finalize() {
	invalid_files.clear();
}

#endif // WINDOWS_ENABLED

#ifdef S_ISREG
#undef S_ISREG
#endif

// ============================================================================
// Merged from drivers/windows/dir_access_windows.cpp
// ============================================================================

#if defined(WINDOWS_ENABLED)

#include "platform/windows/os_windows.h"

#include "core/os/memory.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "platform/windows/os_windows.h"

#include <windows.h>

#include <cstdio>
#include <cwchar>

typedef struct _NT_IO_STATUS_BLOCK {
	union {
		LONG Status;
		PVOID Pointer;
	} DUMMY;
	ULONG_PTR Information;
} NT_IO_STATUS_BLOCK;

typedef struct _NT_FILE_CASE_SENSITIVE_INFO {
	ULONG Flags;
} NT_FILE_CASE_SENSITIVE_INFO;

typedef enum _NT_FILE_INFORMATION_CLASS {
	FileCaseSensitiveInformation = 71,
} NT_FILE_INFORMATION_CLASS;

#define NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR 0x00000001

extern "C" NTSYSAPI LONG NTAPI NtQueryInformationFile(HANDLE FileHandle, NT_IO_STATUS_BLOCK *IoStatusBlock, PVOID FileInformation, ULONG Length, NT_FILE_INFORMATION_CLASS FileInformationClass);

struct DirAccessWindowsPrivate {
	HANDLE h; // handle for FindFirstFile.
	WIN32_FIND_DATA f;
	WIN32_FIND_DATAW fu; // Unicode version.
};

String DirAccessWindows::fix_path(const String &p_path) const {
	String r_path = DirAccess::fix_path(p_path.trim_prefix(R"(\\?\)").replace_char('\\', '/'));
	if (r_path.ends_with(":")) {
		r_path += "/";
	}
	if (r_path.is_relative_path()) {
		r_path = current_dir.trim_prefix(R"(\\?\)").replace_char('\\', '/').path_join(r_path);
	} else if (r_path == ".") {
		r_path = current_dir.trim_prefix(R"(\\?\)").replace_char('\\', '/');
	}
	r_path = r_path.simplify_path();
	r_path = r_path.replace_char('/', '\\');
	if (!r_path.is_network_share_path() && !r_path.begins_with(R"(\\?\)")) {
		r_path = R"(\\?\)" + r_path;
	}
	return r_path;
}

// CreateFolderAsync

Error DirAccessWindows::list_dir_begin() {
	_cisdir = false;
	_cishidden = false;

	list_dir_end();
	p->h = FindFirstFileExW((LPCWSTR)(String(current_dir + "\\*").utf16().get_data()), FindExInfoStandard, &p->fu, FindExSearchNameMatch, nullptr, 0);

	if (p->h == INVALID_HANDLE_VALUE) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

String DirAccessWindows::get_next() {
	if (p->h == INVALID_HANDLE_VALUE) {
		return "";
	}

	_cisdir = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	_cishidden = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);

	String name = String::utf16((const char16_t *)(p->fu.cFileName));

	if (FindNextFileW(p->h, &p->fu) == 0) {
		FindClose(p->h);
		p->h = INVALID_HANDLE_VALUE;
	}

	return name;
}

bool DirAccessWindows::current_is_dir() const {
	return _cisdir;
}

bool DirAccessWindows::current_is_hidden() const {
	return _cishidden;
}

void DirAccessWindows::list_dir_end() {
	if (p->h != INVALID_HANDLE_VALUE) {
		FindClose(p->h);
		p->h = INVALID_HANDLE_VALUE;
	}
}

void DirAccessWindows::_update_drives() {
	drives.clear();
	DWORD mask = GetLogicalDrives();
	for (int i = 0; i < MAX_DRIVES; i++) {
		if (mask & (1 << i)) {
			String drive = String::chr('A' + i) + ':';
			String path = drive + '\\';
			String label;
			Char16String cs = path.utf16();
			UINT type = GetDriveTypeW((LPCWSTR)cs.get_data());
			if (type == DRIVE_REMOTE) {
				drives.push_back(DriveInfo{ drive, "<network>" });
				continue;
			}
			char16_t wlabel[4096];
			if (GetVolumeInformationW((LPCWSTR)cs.get_data(), (LPWSTR)wlabel, 4096, nullptr, nullptr, nullptr, nullptr, 0)) {
				label = String::utf16(wlabel);
			}
			drives.push_back(DriveInfo{ drive, label });
		}
	}
}

int DirAccessWindows::get_drive_count() {
	if (drives.size() == 0) {
		_update_drives();
	}
	return drives.size();
}

String DirAccessWindows::get_drive_label(int p_drive) {
	if (drives.size() == 0) {
		_update_drives();
	}
	if (p_drive < 0 || p_drive >= (int)drives.size()) {
		return String();
	}
	return drives[p_drive].label;
}

String DirAccessWindows::get_drive(int p_drive) {
	if (drives.size() == 0) {
		_update_drives();
	}
	if (p_drive < 0 || p_drive >= (int)drives.size()) {
		return String();
	}
	return drives[p_drive].path;
}

Error DirAccessWindows::change_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	String dir = fix_path(p_dir);

	Char16String real_current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	real_current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
	String prev_dir = String::utf16((const char16_t *)real_current_dir_name.get_data());

	SetCurrentDirectoryW((LPCWSTR)(current_dir.utf16().get_data()));
	bool worked = (SetCurrentDirectoryW((LPCWSTR)(dir.utf16().get_data())) != 0);

	String base = _get_root_path();
	if (!base.is_empty()) {
		str_len = GetCurrentDirectoryW(0, nullptr);
		real_current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
		String new_dir = String::utf16((const char16_t *)real_current_dir_name.get_data()).trim_prefix(R"(\\?\)").replace_char('\\', '/');
		if (!new_dir.begins_with(base)) {
			worked = false;
		}
	}

	if (worked) {
		str_len = GetCurrentDirectoryW(0, nullptr);
		real_current_dir_name.resize_uninitialized(str_len + 1);
		GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
		current_dir = String::utf16((const char16_t *)real_current_dir_name.get_data());
	}

	SetCurrentDirectoryW((LPCWSTR)(prev_dir.utf16().get_data()));

	return worked ? OK : ERR_INVALID_PARAMETER;
}

Error DirAccessWindows::make_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (FileAccessWindows::is_path_invalid(p_dir)) {
#ifdef DEBUG_ENABLED
		WARN_PRINT("The path :" + p_dir + " is a reserved Windows system pipe, so it can't be used for creating directories.");
#endif
		return ERR_INVALID_PARAMETER;
	}

	String dir = fix_path(p_dir);

	bool success;
	int err;

	success = CreateDirectoryW((LPCWSTR)(dir.utf16().get_data()), nullptr);
	err = GetLastError();

	if (success) {
		return OK;
	}

	if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED) {
		return ERR_ALREADY_EXISTS;
	}

	return ERR_CANT_CREATE;
}

String DirAccessWindows::get_current_dir(bool p_include_drive) const {
	String cdir = current_dir.trim_prefix(R"(\\?\)").replace_char('\\', '/');
	String base = _get_root_path();
	if (!base.is_empty()) {
		String bd = cdir.replace_first(base, "");
		if (bd.begins_with("/")) {
			return _get_root_string() + bd.substr(1);
		} else {
			return _get_root_string() + bd;
		}
	}

	if (p_include_drive) {
		return cdir;
	} else {
		if (_get_root_string().is_empty()) {
			int pos = cdir.find_char(':');
			if (pos != -1) {
				return cdir.substr(pos + 1);
			}
		}
		return cdir;
	}
}

bool DirAccessWindows::file_exists(String p_file) {
	GLOBAL_LOCK_FUNCTION

	String file = fix_path(p_file);

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(file.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}

	return !(fileAttr & FILE_ATTRIBUTE_DIRECTORY);
}

bool DirAccessWindows::dir_exists(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	String dir = fix_path(p_dir);

	DWORD fileAttr;
	fileAttr = GetFileAttributesW((LPCWSTR)(dir.utf16().get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return false;
	}
	return (fileAttr & FILE_ATTRIBUTE_DIRECTORY);
}

Error DirAccessWindows::rename(String p_path, String p_new_path) {
	String path = fix_path(p_path);
	String new_path = fix_path(p_new_path);

	// If we're only changing file name case we need to do a little juggling
	if (path.to_lower() == new_path.to_lower()) {
		if (dir_exists(path)) {
			// The path is a dir; just rename
			return MoveFileW((LPCWSTR)(path.utf16().get_data()), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;
		}
		// The path is a file; juggle
		// Note: do not use GetTempFileNameW, it's not long path aware!
		Char16String tmpfile_utf16;
		uint64_t id = OS::get_ticks_usec();
		while (true) {
			tmpfile_utf16 = (path + itos(id++) + ".tmp").utf16();
			HANDLE handle = CreateFileW((LPCWSTR)tmpfile_utf16.get_data(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE) {
				CloseHandle(handle);
				break;
			}
			if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_SHARING_VIOLATION) {
				return FAILED;
			}
		}

		if (!::ReplaceFileW((LPCWSTR)tmpfile_utf16.get_data(), (LPCWSTR)(path.utf16().get_data()), nullptr, 0, nullptr, nullptr)) {
			DeleteFileW((LPCWSTR)tmpfile_utf16.get_data());
			return FAILED;
		}

		return MoveFileW((LPCWSTR)tmpfile_utf16.get_data(), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;

	} else {
		if (file_exists(new_path)) {
			if (remove(new_path) != OK) {
				return FAILED;
			}
		}

		return MoveFileW((LPCWSTR)(path.utf16().get_data()), (LPCWSTR)(new_path.utf16().get_data())) != 0 ? OK : FAILED;
	}
}

Error DirAccessWindows::remove(String p_path) {
	String path = fix_path(p_path);
	const Char16String &path_utf16 = path.utf16();

	DWORD fileAttr;

	fileAttr = GetFileAttributesW((LPCWSTR)(path_utf16.get_data()));
	if (INVALID_FILE_ATTRIBUTES == fileAttr) {
		return FAILED;
	}
	if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY)) {
		return RemoveDirectoryW((LPCWSTR)(path_utf16.get_data())) != 0 ? OK : FAILED;
	} else {
		return DeleteFileW((LPCWSTR)(path_utf16.get_data())) != 0 ? OK : FAILED;
	}
}

uint64_t DirAccessWindows::get_space_left() {
	uint64_t bytes = 0;

	String path = fix_path(current_dir);

	if (!path.ends_with("\\")) {
		path += "\\";
	}

	if (!GetDiskFreeSpaceExW((LPCWSTR)(path.utf16().get_data()), (PULARGE_INTEGER)&bytes, nullptr, nullptr)) {
		return 0;
	}

	// This is either 0 or a value in bytes.
	return bytes;
}

String DirAccessWindows::get_filesystem_type() const {
	String path = current_dir.trim_prefix(R"(\\?\)");

	if (path.is_network_share_path()) {
		return "Network Share";
	}

	int unit_end = path.find_char(':');
	ERR_FAIL_COND_V(unit_end == -1, String());
	String unit = path.substr(0, unit_end + 1) + "\\";

	WCHAR szVolumeName[100];
	WCHAR szFileSystemName[10];
	DWORD dwSerialNumber = 0;
	DWORD dwMaxFileNameLength = 0;
	DWORD dwFileSystemFlags = 0;

	if (::GetVolumeInformationW((LPCWSTR)(unit.utf16().get_data()),
				szVolumeName,
				sizeof(szVolumeName),
				&dwSerialNumber,
				&dwMaxFileNameLength,
				&dwFileSystemFlags,
				szFileSystemName,
				sizeof(szFileSystemName)) == TRUE) {
		return String::utf16((const char16_t *)szFileSystemName).to_upper();
	}

	ERR_FAIL_V("");
}

bool DirAccessWindows::is_case_sensitive(const String &p_path) const {
	String f = fix_path(p_path);

	HANDLE h_file = ::CreateFileW((LPCWSTR)(f.utf16().get_data()), 0,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

	if (h_file == INVALID_HANDLE_VALUE) {
		return false;
	}

	NT_IO_STATUS_BLOCK io_status_block;
	NT_FILE_CASE_SENSITIVE_INFO file_info;
	LONG out = NtQueryInformationFile(h_file, &io_status_block, &file_info, sizeof(NT_FILE_CASE_SENSITIVE_INFO), FileCaseSensitiveInformation);
	::CloseHandle(h_file);

	if (out >= 0) {
		return file_info.Flags & NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR;
	} else {
		return false;
	}
}

typedef struct {
	ULONGLONG LowPart;
	ULONGLONG HighPart;
} GD_FILE_ID_128;

typedef struct {
	ULONGLONG VolumeSerialNumber;
	GD_FILE_ID_128 FileId;
} GD_FILE_ID_INFO;

bool DirAccessWindows::is_equivalent(const String &p_path_a, const String &p_path_b) const {
	GD_FILE_ID_INFO ex1;
	bool did_fallback1 = false;
	BY_HANDLE_FILE_INFORMATION fallback1 = {};

	String f1 = fix_path(p_path_a);
	HANDLE h1 = ::CreateFileW((LPCWSTR)(f1.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (h1 == INVALID_HANDLE_VALUE) {
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}
	if (!::GetFileInformationByHandleEx(h1, (FILE_INFO_BY_HANDLE_CLASS)0x12 /*FileIdInfo*/, &ex1, sizeof(ex1))) {
		// Try `GetFileInformationByHandle` since the Ex version is not supported by certain USB and network drives.
		did_fallback1 = true;
		if (!::GetFileInformationByHandle(h1, &fallback1)) {
			::CloseHandle(h1);
			return DirAccess::is_equivalent(p_path_a, p_path_b);
		}
	}

	GD_FILE_ID_INFO ex2;
	bool did_fallback2 = false;
	BY_HANDLE_FILE_INFORMATION fallback2 = {};

	String f2 = fix_path(p_path_b);
	HANDLE h2 = ::CreateFileW((LPCWSTR)(f2.utf16().get_data()), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (h2 == INVALID_HANDLE_VALUE) {
		::CloseHandle(h1);
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}
	if (!::GetFileInformationByHandleEx(h2, (FILE_INFO_BY_HANDLE_CLASS)0x12 /*FileIdInfo*/, &ex2, sizeof(ex2))) {
		// Try `GetFileInformationByHandle` since the Ex version is not supported by certain USB and network drives.
		did_fallback2 = true;
		if (!::GetFileInformationByHandle(h2, &fallback2)) {
			::CloseHandle(h1);
			::CloseHandle(h2);
			return DirAccess::is_equivalent(p_path_a, p_path_b);
		}
	}

	// Keep both handles open while retrieving ids, since file ids are only unique for open handles.
	::CloseHandle(h1);
	::CloseHandle(h2);

	if (did_fallback1 != did_fallback2) {
		// Assuming different drives due to different syscall support.
		return false;
	}

	if (did_fallback1) {
		return fallback1.dwVolumeSerialNumber == fallback2.dwVolumeSerialNumber && fallback1.nFileIndexLow == fallback2.nFileIndexLow && fallback1.nFileIndexHigh == fallback2.nFileIndexHigh;
	}

	return ex1.VolumeSerialNumber == ex2.VolumeSerialNumber && ex1.FileId.LowPart == ex2.FileId.LowPart && ex1.FileId.HighPart == ex2.FileId.HighPart;
}

bool DirAccessWindows::is_link(String p_file) {
	String f = fix_path(p_file);

	DWORD attr = GetFileAttributesW((LPCWSTR)(f.utf16().get_data()));
	if (attr == INVALID_FILE_ATTRIBUTES) {
		return false;
	}

	return (attr & FILE_ATTRIBUTE_REPARSE_POINT);
}

String DirAccessWindows::read_link(String p_file) {
	String f = fix_path(p_file);

	HANDLE hfile = CreateFileW((LPCWSTR)(f.utf16().get_data()), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (hfile == INVALID_HANDLE_VALUE) {
		return f;
	}

	DWORD ret = GetFinalPathNameByHandleW(hfile, nullptr, 0, VOLUME_NAME_DOS | FILE_NAME_NORMALIZED);
	if (ret == 0) {
		return f;
	}
	Char16String cs;
	cs.resize_uninitialized(ret + 1);
	GetFinalPathNameByHandleW(hfile, (LPWSTR)cs.ptrw(), ret, VOLUME_NAME_DOS | FILE_NAME_NORMALIZED);
	CloseHandle(hfile);

	return String::utf16((const char16_t *)cs.ptr(), ret).trim_prefix(R"(\\?\)").replace_char('\\', '/');
}

Error DirAccessWindows::create_link(String p_source, String p_target) {
	String source = fix_path(p_source);
	String target = fix_path(p_target);

	DWORD file_attr = GetFileAttributesW((LPCWSTR)(source.utf16().get_data()));
	bool is_dir = (file_attr & FILE_ATTRIBUTE_DIRECTORY);

	DWORD flags = ((is_dir) ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0) | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
	if (CreateSymbolicLinkW((LPCWSTR)target.utf16().get_data(), (LPCWSTR)source.utf16().get_data(), flags) != 0) {
		return OK;
	} else {
		return FAILED;
	}
}

DirAccessWindows::DirAccessWindows() {
	p = memnew(DirAccessWindowsPrivate);
	p->h = INVALID_HANDLE_VALUE;

	Char16String real_current_dir_name;
	size_t str_len = GetCurrentDirectoryW(0, nullptr);
	real_current_dir_name.resize_uninitialized(str_len + 1);
	GetCurrentDirectoryW(real_current_dir_name.size(), (LPWSTR)real_current_dir_name.ptrw());
	current_dir = String::utf16((const char16_t *)real_current_dir_name.get_data());

	change_dir(".");
}

DirAccessWindows::~DirAccessWindows() {
	list_dir_end();

	memdelete(p);
}

#endif // WINDOWS_ENABLED

#ifdef NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR
#undef NT_FILE_CS_FLAG_CASE_SENSITIVE_DIR
#endif

// ============================================================================
// Merged from drivers/windows/file_access_windows_pipe.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "platform/windows/os_windows.h"

#include "core/string/ustring.h"

#include <winbase.h>

Error FileAccessWindowsPipe::open_existing(HANDLE p_rfd, HANDLE p_wfd, bool p_blocking) {
	// Open pipe using handles created by CreatePipe(rfd, wfd, NULL, 4096) call in the OS.execute_with_pipe.
	_close();

	path_src = String();
	ERR_FAIL_COND_V_MSG(fd[0] != nullptr || fd[1] != nullptr, ERR_ALREADY_IN_USE, "Pipe is already in use.");
	fd[0] = p_rfd;
	fd[1] = p_wfd;

	if (!p_blocking) {
		DWORD mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
		SetNamedPipeHandleState(fd[0], &mode, nullptr, nullptr);
		SetNamedPipeHandleState(fd[1], &mode, nullptr, nullptr);
	}

	last_error = OK;
	return OK;
}

Error FileAccessWindowsPipe::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	ERR_FAIL_COND_V_MSG(fd[0] != nullptr || fd[1] != nullptr, ERR_ALREADY_IN_USE, "Pipe is already in use.");

	path = String("\\\\.\\pipe\\LOCAL\\") + p_path.replace("pipe://", "").replace_char('/', '_');

	HANDLE h = CreateFileW((LPCWSTR)path.utf16().get_data(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		h = CreateNamedPipeW((LPCWSTR)path.utf16().get_data(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT, 1, 4096, 4096, 0, nullptr);
		if (h == INVALID_HANDLE_VALUE) {
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}
		ConnectNamedPipe(h, nullptr);
	}
	fd[0] = h;
	fd[1] = h;

	last_error = OK;
	return OK;
}

void FileAccessWindowsPipe::_close() {
	if (fd[0] == nullptr) {
		return;
	}
	if (fd[1] != fd[0]) {
		CloseHandle(fd[1]);
	}
	CloseHandle(fd[0]);
	fd[0] = nullptr;
	fd[1] = nullptr;
}

bool FileAccessWindowsPipe::is_open() const {
	return (fd[0] != nullptr || fd[1] != nullptr);
}

String FileAccessWindowsPipe::get_path() const {
	return path_src;
}

String FileAccessWindowsPipe::get_path_absolute() const {
	return path_src;
}

uint64_t FileAccessWindowsPipe::get_length() const {
	ERR_FAIL_COND_V_MSG(fd[0] == nullptr, -1, "Pipe must be opened before use.");

	DWORD buf_rem = 0;
	ERR_FAIL_COND_V(!PeekNamedPipe(fd[0], nullptr, 0, nullptr, &buf_rem, nullptr), 0);
	return buf_rem;
}

uint64_t FileAccessWindowsPipe::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_COND_V_MSG(fd[0] == nullptr, -1, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	DWORD read = 0;
	if (!ReadFile(fd[0], p_dst, p_length, &read, nullptr) || read != p_length) {
		last_error = ERR_FILE_CANT_READ;
	} else {
		last_error = OK;
	}
	return read;
}

Error FileAccessWindowsPipe::get_error() const {
	return last_error;
}

bool FileAccessWindowsPipe::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_COND_V_MSG(fd[1] == nullptr, false, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);

	DWORD read = -1;
	bool ok = WriteFile(fd[1], p_src, p_length, &read, nullptr);
	if (!ok || read != p_length) {
		last_error = ERR_FILE_CANT_WRITE;
		return false;
	} else {
		last_error = OK;
		return true;
	}
}

void FileAccessWindowsPipe::close() {
	_close();
}

FileAccessWindowsPipe::~FileAccessWindowsPipe() {
	_close();
}

#endif // WINDOWS_ENABLED

// ============================================================================
// Merged from drivers/windows/ip_windows.cpp
// ============================================================================

#if defined(WINDOWS_ENABLED)

#include "platform/windows/os_windows.h"

#include <winsock2.h>
#include <ws2tcpip.h>
// Include winsock2 before iphlpapi.
#include <iphlpapi.h>

static IPAddress _sockaddr2ip(struct sockaddr *p_addr) {
	IPAddress ip;

	if (p_addr->sa_family == AF_INET) {
		struct sockaddr_in *addr = (struct sockaddr_in *)p_addr;
		ip.set_ipv4((uint8_t *)&(addr->sin_addr));
	} else if (p_addr->sa_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		ip.set_ipv6(addr6->sin6_addr.s6_addr);
	}

	return ip;
}

void IPWindows::_resolve_hostname(
	List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type)
{
	struct addrinfo hints;
	struct addrinfo* result = nullptr;

	memset(&hints, 0, sizeof(struct addrinfo));
	if (p_type == IP::TYPE_IPV4) {
		hints.ai_family = AF_INET;
	}
	else if (p_type == IP::TYPE_IPV6) {
		hints.ai_family = AF_INET6;
		hints.ai_flags = 0;
	}
	else {
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_ADDRCONFIG;
	}
	hints.ai_flags &= ~AI_NUMERICHOST;

	int s = getaddrinfo(p_hostname.utf8().get_data(), nullptr, &hints, &result);
	if (s != 0) {
		print_verbose("getaddrinfo failed! Cannot resolve hostname.");
		return;
	}

	if (result == nullptr || result->ai_addr == nullptr) {
		print_verbose("Invalid response from getaddrinfo.");
		if (result) {
			freeaddrinfo(result);
		}
		return;
	}

	struct addrinfo *next = result;

	do {
		if (next->ai_addr == nullptr) {
			next = next->ai_next;
			continue;
		}
		IPAddress ip = _sockaddr2ip(next->ai_addr);
		if (ip.is_valid() && !r_addresses.find(ip)) {
			r_addresses.push_back(ip);
		}
		next = next->ai_next;
	} while (next);

	freeaddrinfo(result);
}

void IPWindows::get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces)
{
	ULONG buf_size = 1024;
	IP_ADAPTER_ADDRESSES* addrs;

	while (true) {
		addrs = (IP_ADAPTER_ADDRESSES*)memalloc(buf_size);
		int err = GetAdaptersAddresses(AF_UNSPEC,
			GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER |
				GAA_FLAG_SKIP_FRIENDLY_NAME,
			nullptr, addrs, &buf_size);
		if (err == NO_ERROR) {
			break;
		}
		memfree(addrs);
		if (err == ERROR_BUFFER_OVERFLOW) {
			continue; // Will go back and alloc the right size.
		}

		ERR_FAIL_MSG("Call to GetAdaptersAddresses failed with error " + itos(err) + ".");
	}

	IP_ADAPTER_ADDRESSES* adapter = addrs;

	while (adapter != nullptr) {
		IP::Interface_Info info;
		info.name = adapter->AdapterName;
		info.name_friendly = adapter->FriendlyName;
		info.index = String::num_uint64(adapter->IfIndex);

		IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress;
		while (address != nullptr) {
			int family = address->Address.lpSockaddr->sa_family;
			if (family != AF_INET && family != AF_INET6) {
				continue;
			}
			info.ip_addresses.push_front(_sockaddr2ip(address->Address.lpSockaddr));
			address = address->Next;
		}
		adapter = adapter->Next;
		// Only add interface if it has at least one IP.
		if (info.ip_addresses.size() > 0) {
			r_interfaces->insert(info.name, info);
		}
	}

	memfree(addrs);
}

void IPWindows::make_default()
{
	IP::bind<IPWindows>();
}

#endif // WINDOWS_ENABLED

// ============================================================================
// Merged from drivers/windows/net_socket_winsock.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "platform/windows/os_windows.h"

#include <mswsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Workaround missing flag in MinGW
#if defined(__MINGW32__) && !defined(SIO_UDP_NETRESET)
#define SIO_UDP_NETRESET _WSAIOW(IOC_VENDOR, 15)
#endif

size_t NetSocketWinSock::_set_addr_storage(struct sockaddr_storage *p_addr, const IPAddress &p_ip, uint16_t p_port, IP::Type p_ip_type) {
	memset(p_addr, 0, sizeof(struct sockaddr_storage));
	if (p_ip_type == IP::TYPE_IPV6 || p_ip_type == IP::TYPE_ANY) { // IPv6 socket.

		// IPv6 only socket with IPv4 address.
		ERR_FAIL_COND_V(!p_ip.is_wildcard() && p_ip_type == IP::TYPE_IPV6 && p_ip.is_ipv4(), 0);

		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		addr6->sin6_family = AF_INET6;
		addr6->sin6_port = htons(p_port);
		if (p_ip.is_valid()) {
			memcpy(&addr6->sin6_addr.s6_addr, p_ip.get_ipv6(), 16);
		} else {
			addr6->sin6_addr = in6addr_any;
		}
		return sizeof(sockaddr_in6);
	} else { // IPv4 socket.

		// IPv4 socket with IPv6 address.
		ERR_FAIL_COND_V(!p_ip.is_wildcard() && !p_ip.is_ipv4(), 0);

		struct sockaddr_in *addr4 = (struct sockaddr_in *)p_addr;
		addr4->sin_family = AF_INET;
		addr4->sin_port = htons(p_port); // Short, network byte order.

		if (p_ip.is_valid()) {
			memcpy(&addr4->sin_addr.s_addr, p_ip.get_ipv4(), 4);
		} else {
			addr4->sin_addr.s_addr = INADDR_ANY;
		}

		return sizeof(sockaddr_in);
	}
}

void NetSocketWinSock::_set_ip_port(struct sockaddr_storage *p_addr, IPAddress *r_ip, uint16_t *r_port) {
	if (p_addr->ss_family == AF_INET) {
		struct sockaddr_in *addr4 = (struct sockaddr_in *)p_addr;
		if (r_ip) {
			r_ip->set_ipv4((uint8_t *)&(addr4->sin_addr.s_addr));
		}
		if (r_port) {
			*r_port = ntohs(addr4->sin_port);
		}
	} else if (p_addr->ss_family == AF_INET6) {
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p_addr;
		if (r_ip) {
			r_ip->set_ipv6(addr6->sin6_addr.s6_addr);
		}
		if (r_port) {
			*r_port = ntohs(addr6->sin6_port);
		}
	}
}

Ref<NetSocket> NetSocketWinSock::_create_func() {
	return memnew(NetSocketWinSock);
}

void NetSocketWinSock::make_default() {
	ERR_FAIL_COND(_create != nullptr);

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	_create = _create_func;
}

void NetSocketWinSock::cleanup() {
	ERR_FAIL_COND(_create == nullptr);

	WSACleanup();
	_create = nullptr;
}

NetSocketWinSock::NetSocketWinSock() {
}

NetSocketWinSock::~NetSocketWinSock() {
	close();
}

NetSocketWinSock::NetError NetSocketWinSock::_get_socket_error() const {
	int err = WSAGetLastError();
	if (err == WSAEISCONN) {
		return ERR_NET_IS_CONNECTED;
	}
	if (err == WSAEINPROGRESS || err == WSAEALREADY) {
		return ERR_NET_IN_PROGRESS;
	}
	if (err == WSAEWOULDBLOCK) {
		return ERR_NET_WOULD_BLOCK;
	}
	if (err == WSAEADDRINUSE || err == WSAEADDRNOTAVAIL) {
		return ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE;
	}
	if (err == WSAEACCES) {
		return ERR_NET_UNAUTHORIZED;
	}
	if (err == WSAEMSGSIZE || err == WSAENOBUFS) {
		return ERR_NET_BUFFER_TOO_SMALL;
	}
	print_verbose("Socket error: " + itos(err) + ".");
	return ERR_NET_OTHER;
}

bool NetSocketWinSock::_can_use_ip(const IPAddress &p_ip, const bool p_for_bind) const {
	if (p_for_bind && !(p_ip.is_valid() || p_ip.is_wildcard())) {
		return false;
	} else if (!p_for_bind && !p_ip.is_valid()) {
		return false;
	}
	// Check if socket support this IP type.
	IP::Type type = p_ip.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	return !(_ip_type != IP::TYPE_ANY && !p_ip.is_wildcard() && _ip_type != type);
}

_FORCE_INLINE_ Error NetSocketWinSock::_change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!_can_use_ip(p_ip, false), ERR_INVALID_PARAMETER);

	// Need to force level and af_family to IP(v4) when using dual stacking and provided multicast group is IPv4.
	IP::Type type = _ip_type == IP::TYPE_ANY && p_ip.is_ipv4() ? IP::TYPE_IPV4 : _ip_type;
	// This needs to be the proper level for the multicast group, no matter if the socket is dual stacking.
	int level = type == IP::TYPE_IPV4 ? IPPROTO_IP : IPPROTO_IPV6;
	int ret = -1;

	IPAddress if_ip;
	uint32_t if_v6id = 0;
	HashMap<String, IP::Interface_Info> if_info;
	IP::get_local_interfaces(&if_info);
	for (KeyValue<String, IP::Interface_Info> &E : if_info) {
		IP::Interface_Info &c = E.value;
		if (c.name != p_if_name) {
			continue;
		}

		if_v6id = (uint32_t)c.index.to_int();
		if (type == IP::TYPE_IPV6) {
			break; // IPv6 uses index.
		}

		for (const IPAddress &F : c.ip_addresses) {
			if (!F.is_ipv4()) {
				continue; // Wrong IP type.
			}
			if_ip = F;
			break;
		}
		break;
	}

	if (level == IPPROTO_IP) {
		ERR_FAIL_COND_V(!if_ip.is_valid(), ERR_INVALID_PARAMETER);
		struct ip_mreq greq;
		int sock_opt = p_add ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP;
		memcpy(&greq.imr_multiaddr, p_ip.get_ipv4(), 4);
		memcpy(&greq.imr_interface, if_ip.get_ipv4(), 4);
		ret = setsockopt(_sock, level, sock_opt, (const char *)&greq, sizeof(greq));
	} else {
		struct ipv6_mreq greq;
		int sock_opt = p_add ? IPV6_ADD_MEMBERSHIP : IPV6_DROP_MEMBERSHIP;
		memcpy(&greq.ipv6mr_multiaddr, p_ip.get_ipv6(), 16);
		greq.ipv6mr_interface = if_v6id;
		ret = setsockopt(_sock, level, sock_opt, (const char *)&greq, sizeof(greq));
	}
	ERR_FAIL_COND_V(ret != 0, FAILED);

	return OK;
}

void NetSocketWinSock::_set_socket(SOCKET p_sock, IP::Type p_ip_type, bool p_is_stream) {
	_sock = p_sock;
	_ip_type = p_ip_type;
	_is_stream = p_is_stream;
}

Error NetSocketWinSock::open(Family p_family, Type p_sock_type, IP::Type &ip_type) {
	ERR_FAIL_COND_V(p_family != Family::INET, ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(is_open(), ERR_ALREADY_IN_USE);
	ERR_FAIL_COND_V(ip_type > IP::TYPE_ANY || ip_type < IP::TYPE_NONE, ERR_INVALID_PARAMETER);

	int family = ip_type == IP::TYPE_IPV4 ? AF_INET : AF_INET6;
	int protocol = p_sock_type == TYPE_TCP ? IPPROTO_TCP : IPPROTO_UDP;
	int type = p_sock_type == TYPE_TCP ? SOCK_STREAM : SOCK_DGRAM;
	_sock = socket(family, type, protocol);

	if (_sock == INVALID_SOCKET && ip_type == IP::TYPE_ANY) {
		// Careful here, changing the referenced parameter so the caller knows that we are using an IPv4 socket
		// in place of a dual stack one, and further calls to _set_sock_addr will work as expected.
		ip_type = IP::TYPE_IPV4;
		family = AF_INET;
		_sock = socket(family, type, protocol);
	}

	ERR_FAIL_COND_V(_sock == INVALID_SOCKET, FAILED);
	_ip_type = ip_type;

	if (family == AF_INET6) {
		// Select IPv4 over IPv6 mapping.
		set_ipv6_only_enabled(ip_type != IP::TYPE_ANY);
	}

	if (protocol == IPPROTO_UDP) {
		// Make sure to disable broadcasting for UDP sockets.
		// Depending on the OS, this option might or might not be enabled by default. Let's normalize it.
		set_broadcasting_enabled(false);
	}

	_is_stream = p_sock_type == TYPE_TCP;

	if (!_is_stream) {
		// Disable windows feature/bug reporting WSAECONNRESET/WSAENETRESET when
		// recv/recvfrom and an ICMP reply was received from a previous send/sendto.
		unsigned long disable = 0;
		if (ioctlsocket(_sock, SIO_UDP_CONNRESET, &disable) == SOCKET_ERROR) {
			print_verbose("Unable to turn off UDP WSAECONNRESET behavior on Windows.");
		}
		if (ioctlsocket(_sock, SIO_UDP_NETRESET, &disable) == SOCKET_ERROR) {
			// This feature seems not to be supported on wine.
			print_verbose("Unable to turn off UDP WSAENETRESET behavior on Windows.");
		}
	}
	return OK;
}

void NetSocketWinSock::close() {
	if (_sock != INVALID_SOCKET) {
		closesocket(_sock);
	}

	_sock = INVALID_SOCKET;
	_ip_type = IP::TYPE_NONE;
	_is_stream = false;
}

Error NetSocketWinSock::bind(Address p_addr) {
	ERR_FAIL_COND_V(!p_addr.is_inet(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!_can_use_ip(p_addr.ip(), true), ERR_INVALID_PARAMETER);

	sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_addr.ip(), p_addr.port(), _ip_type);

	if (::bind(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		print_verbose("Failed to bind socket. Error: " + itos(err) + ".");
		close();
		return ERR_UNAVAILABLE;
	}

	return OK;
}

Error NetSocketWinSock::listen(int p_max_pending) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	if (::listen(_sock, p_max_pending) != 0) {
		_get_socket_error();
		print_verbose("Failed to listen from socket.");
		close();
		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::connect_to_host(Address p_addr) {
	ERR_FAIL_COND_V(!p_addr.is_inet(), ERR_UNAVAILABLE);
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(!_can_use_ip(p_addr.ip(), false), ERR_INVALID_PARAMETER);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_addr.ip(), p_addr.port(), _ip_type);

	if (::WSAConnect(_sock, (struct sockaddr *)&addr, addr_size, nullptr, nullptr, nullptr, nullptr) != 0) {
		NetError err = _get_socket_error();

		switch (err) {
			// We are already connected.
			case ERR_NET_IS_CONNECTED:
				return OK;
			// Still waiting to connect, try again in a while.
			case ERR_NET_WOULD_BLOCK:
			case ERR_NET_IN_PROGRESS:
				return ERR_BUSY;
			default:
				print_verbose("Connection to remote host failed.");
				close();
				return FAILED;
		}
	}

	return OK;
}

Error NetSocketWinSock::poll(PollType p_type, int p_timeout) const {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	bool ready = false;
	fd_set rd, wr, ex;
	fd_set *rdp = nullptr;
	fd_set *wrp = nullptr;
	FD_ZERO(&rd);
	FD_ZERO(&wr);
	FD_ZERO(&ex);
	FD_SET(_sock, &ex);
	struct timeval timeout = { p_timeout / 1000, (p_timeout % 1000) * 1000 };
	// For blocking operation, pass nullptr timeout pointer to select.
	struct timeval *tp = nullptr;
	if (p_timeout >= 0) {
		// If timeout is non-negative, we want to specify the timeout instead.
		tp = &timeout;
	}

	switch (p_type) {
		case POLL_TYPE_IN:
			FD_SET(_sock, &rd);
			rdp = &rd;
			break;
		case POLL_TYPE_OUT:
			FD_SET(_sock, &wr);
			wrp = &wr;
			break;
		case POLL_TYPE_IN_OUT:
			FD_SET(_sock, &rd);
			FD_SET(_sock, &wr);
			rdp = &rd;
			wrp = &wr;
	}
	// WSAPoll is broken: https://daniel.haxx.se/blog/2012/10/10/wsapoll-is-broken/.
	int ret = select(1, rdp, wrp, &ex, tp);

	if (ret == SOCKET_ERROR) {
		return FAILED;
	}

	if (ret == 0) {
		return ERR_BUSY;
	}

	if (FD_ISSET(_sock, &ex)) {
		_get_socket_error();
		print_verbose("Exception when polling socket.");
		return FAILED;
	}

	if (rdp && FD_ISSET(_sock, rdp)) {
		ready = true;
	}
	if (wrp && FD_ISSET(_sock, wrp)) {
		ready = true;
	}

	return ready ? OK : ERR_BUSY;
}

Error NetSocketWinSock::recv(uint8_t *p_buffer, int p_len, int &r_read) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	r_read = ::recv(_sock, (char *)p_buffer, p_len, 0);

	if (r_read < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}

		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	struct sockaddr_storage from;
	socklen_t len = sizeof(struct sockaddr_storage);
	memset(&from, 0, len);

	r_read = ::recvfrom(_sock, (char *)p_buffer, p_len, p_peek ? MSG_PEEK : 0, (struct sockaddr *)&from, &len);

	if (r_read < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}

		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	if (from.ss_family == AF_INET) {
		struct sockaddr_in *sin_from = (struct sockaddr_in *)&from;
		r_ip.set_ipv4((uint8_t *)&sin_from->sin_addr);
		r_port = ntohs(sin_from->sin_port);
	} else if (from.ss_family == AF_INET6) {
		struct sockaddr_in6 *s6_from = (struct sockaddr_in6 *)&from;
		r_ip.set_ipv6((uint8_t *)&s6_from->sin6_addr);
		r_port = ntohs(s6_from->sin6_port);
	} else {
		// Unsupported socket family, should never happen.
		ERR_FAIL_V(FAILED);
	}

	return OK;
}

Error NetSocketWinSock::send(const uint8_t *p_buffer, int p_len, int &r_sent) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	int flags = 0;
	r_sent = ::send(_sock, (const char *)p_buffer, p_len, flags);

	if (r_sent < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}
		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_ip, p_port, _ip_type);
	r_sent = ::sendto(_sock, (const char *)p_buffer, p_len, 0, (struct sockaddr *)&addr, addr_size);

	if (r_sent < 0) {
		NetError err = _get_socket_error();
		if (err == ERR_NET_WOULD_BLOCK) {
			return ERR_BUSY;
		}
		if (err == ERR_NET_BUFFER_TOO_SMALL) {
			return ERR_OUT_OF_MEMORY;
		}

		return FAILED;
	}

	return OK;
}

Error NetSocketWinSock::set_broadcasting_enabled(bool p_enabled) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	// IPv6 has no broadcast support.
	if (_ip_type == IP::TYPE_IPV6) {
		return ERR_UNAVAILABLE;
	}

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, (const char *)&par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change broadcast setting.");
		return FAILED;
	}
	return OK;
}

void NetSocketWinSock::set_blocking_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	int ret = 0;
	unsigned long par = p_enabled ? 0 : 1;
	ret = ioctlsocket(_sock, FIONBIO, &par);
	if (ret != 0) {
		WARN_PRINT("Unable to change non-block mode.");
	}
}

void NetSocketWinSock::set_ipv6_only_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	// This option is only available in IPv6 sockets.
	ERR_FAIL_COND(_ip_type == IP::TYPE_IPV4);

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change IPv4 address mapping over IPv6 option.");
	}
}

void NetSocketWinSock::set_tcp_no_delay_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	ERR_FAIL_COND(!_is_stream); // Not TCP.

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&par, sizeof(int)) < 0) {
		WARN_PRINT("Unable to set TCP no delay option.");
	}
}

void NetSocketWinSock::set_reuse_address_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	// On Windows, enabling SO_REUSEADDR actually would also enable reuse port, very bad on TCP. Denying...
	// Windows does not have this option, SO_REUSEADDR in this magical world means SO_REUSEPORT
}

bool NetSocketWinSock::is_open() const {
	return _sock != INVALID_SOCKET;
}

int NetSocketWinSock::get_available_bytes() const {
	ERR_FAIL_COND_V(!is_open(), -1);

	unsigned long len;
	int ret = ioctlsocket(_sock, FIONREAD, &len);
	if (ret == -1) {
		_get_socket_error();
		print_verbose("Error when checking available bytes on socket.");
		return -1;
	}
	return len;
}

Error NetSocketWinSock::get_socket_address(Address *r_addr) const {
	ERR_FAIL_COND_V(!is_open(), FAILED);

	struct sockaddr_storage saddr;
	socklen_t len = sizeof(saddr);
	if (getsockname(_sock, (struct sockaddr *)&saddr, &len) != 0) {
		_get_socket_error();
		print_verbose("Error when reading local socket address.");
		return FAILED;
	}
	IPAddress ip;
	uint16_t port = 0;
	_set_ip_port(&saddr, &ip, &port);
	if (r_addr) {
		*r_addr = Address(ip, port);
	}
	return OK;
}

Ref<NetSocket> NetSocketWinSock::accept(Address &r_addr) {
	Ref<NetSocket> out;
	ERR_FAIL_COND_V(!is_open(), out);

	struct sockaddr_storage their_addr;
	socklen_t size = sizeof(their_addr);
	SOCKET fd = ::accept(_sock, (struct sockaddr *)&their_addr, &size);
	if (fd == INVALID_SOCKET) {
		_get_socket_error();
		print_verbose("Error when accepting socket connection.");
		return out;
	}

	IPAddress ip;
	uint16_t port = 0;
	_set_ip_port(&their_addr, &ip, &port);
	r_addr = Address(ip, port);

	Ref<NetSocketWinSock> ns = memnew(NetSocketWinSock);
	ns->_set_socket(fd, _ip_type, _is_stream);
	ns->set_blocking_enabled(false);
	return Ref<NetSocket>(ns);
}

Error NetSocketWinSock::join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, true);
}

Error NetSocketWinSock::leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, false);
}

#endif // WINDOWS_ENABLED

#ifdef SIO_UDP_NETRESET
#undef SIO_UDP_NETRESET
#endif

// ============================================================================
// Merged from drivers/windows/thread_windows.cpp
// ============================================================================

#ifdef WINDOWS_ENABLED

#include "platform/windows/os_windows.h"

#include "core/os/thread.h"
#include "core/string/ustring.h"

#include <windows.h>

typedef HRESULT(WINAPI *SetThreadDescriptionPtr)(HANDLE p_thread, PCWSTR p_thread_description);
SetThreadDescriptionPtr w10_SetThreadDescription = nullptr;

static Error set_name(const String &p_name) {
	HANDLE hThread = GetCurrentThread();
	HRESULT res = E_FAIL;
	if (w10_SetThreadDescription) {
		res = w10_SetThreadDescription(hThread, (LPCWSTR)p_name.utf16().get_data()); // Windows 10 Redstone (1607) only.
	}
	return SUCCEEDED(res) ? OK : ERR_INVALID_PARAMETER;
}

void init_thread_win() {
	w10_SetThreadDescription = (SetThreadDescriptionPtr)(void *)GetProcAddress(LoadLibraryW(L"kernel32.dll"), "SetThreadDescription");

	Thread::_set_platform_functions({ set_name });
}

#endif // WINDOWS_ENABLED

// ============================================================================
// Merged from drivers/winmidi/midi_driver_winmidi.cpp
// ============================================================================

#ifdef WINMIDI_ENABLED

#include "platform/windows/os_windows.h"

#include "core/string/print_string.h"

void MIDIDriverWinMidi::read(
	HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	(void)hMidiIn;
	(void)dwParam2;
	if (wMsg == MIM_DATA) {
		// For MIM_DATA: dwParam1 = wMidiMessage, dwParam2 = dwTimestamp.
		// Windows implementation has already unpacked running status and dropped any SysEx,
		// so we can just forward straight to the event.
		const uint8_t* midi_msg = (uint8_t*)&dwParam1;
		MIDIDriver::send_event((int)dwInstance, midi_msg[0], &midi_msg[1], 2);
	}
}

void MIDIDriverWinMidi::initialize()
{
	if (data == nullptr) {
		data = memnew(Data);
	}
}

void MIDIDriverWinMidi::finalize()
{
	if (data == nullptr) {
		return;
	}
	close();
	memdelete(data);
	data = nullptr;
}

Error MIDIDriverWinMidi::open()
{
	ERR_FAIL_NULL_V(data, ERR_UNCONFIGURED);

	int device_index = 0;
	for (UINT i = 0; i < midiInGetNumDevs(); i++) {
		HMIDIIN midi_in;
		MIDIINCAPS caps;

		MMRESULT open_res =
			midiInOpen(&midi_in, i, (DWORD_PTR)read, (DWORD_PTR)device_index, CALLBACK_FUNCTION);
		MMRESULT caps_res = midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));

		if (open_res == MMSYSERR_NOERROR) {
			midiInStart(midi_in);
			data->connected_sources.push_back(midi_in);
			if (caps_res == MMSYSERR_NOERROR) {
				MIDIDriver::add_connected_input(caps.szPname);
			}
			else {
				// Should push something even if we don't get a name,
				// so that the IDs line up correctly on the script side.
				MIDIDriver::add_connected_input("ERROR");
			}
			// Only increment device index for successfully connected devices.
			device_index++;
		}
		else {
			char err[256];
			midiInGetErrorText(open_res, err, 256);
			ERR_PRINT("midiInOpen error: " + String(err));

			if (caps_res == MMSYSERR_NOERROR) {
				ERR_PRINT("Can't open MIDI device \"" + String(caps.szPname) +
						  "\", is it being used by another application?");
			}
		}
	}

	return OK;
}

void MIDIDriverWinMidi::close()
{
	if (data == nullptr) {
		return;
	}

	for (int i = 0; i < data->connected_sources.size(); i++) {
		HMIDIIN midi_in = data->connected_sources[i];
		midiInStop(midi_in);
		midiInClose(midi_in);
	}
	data->connected_sources.clear();
	MIDIDriver::clear_connected_inputs();
}

#endif // WINMIDI_ENABLED
