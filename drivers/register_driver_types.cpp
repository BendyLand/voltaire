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

#include "main/main.h"

#include "core/io/resource_saver.h"
// --- Inlined/Folded from drivers/png/image_loader_png.h ---
#include "core/io/resource_loader.h"

class ImageLoaderPNG : public ImageFormatLoader {
private:
	static Vector<uint8_t> lossless_pack_png(const Ref<Image> &p_image);
	static Ref<Image> lossless_unpack_png(const Vector<uint8_t> &p_data);
	static Ref<Image> unpack_mem_png(const uint8_t *p_png, int p_size);
	static Ref<Image> load_mem_png(const uint8_t *p_png, int p_size);

public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderPNG();
};

// --- Folded from drivers/png/resource_saver_png.h ---
#include "core/io/image.h"
#include "core/io/resource_saver.h"

class ResourceSaverPNG : public ResourceFormatSaver
{
public:
	static Error save_image(const String& p_path, const Ref<Image>& p_img);
	static Vector<uint8_t> save_image_to_buffer(const Ref<Image>& p_img);

	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceSaverPNG();
};





// --- End drivers/png/resource_saver_png.h ---

#ifdef ACCESSKIT_ENABLED
// --- Inlined/Folded from drivers/accesskit/accessibility_server_accesskit.h ---
#ifdef ACCESSKIT_ENABLED

#include <accesskit.h>
#include "core/templates/rid_owner.h"
#include "servers/display/accessibility_server.h"

class AccessibilityServerAccessKit : public AccessibilityServerDriver
{
	static AccessibilityServerDriver* create_func(Error& r_error);

	struct AccessibilityElement
	{

		DisplayServerEnums::WindowID window_id = DisplayServerEnums::INVALID_WINDOW_ID;
		RID parent;
		LocalVector<RID> children;
		Vector3i run;
		String name;
		String name_extra_info;
		int64_t flags = 0;

		accesskit_role role = ACCESSKIT_ROLE_UNKNOWN;
		accesskit_node* node = nullptr;
	};

	mutable RID_PtrOwner<AccessibilityElement> rid_owner{65536, 1048576};

	struct WindowData
	{
		// Adapter.
#ifdef WINDOWS_ENABLED
		accesskit_windows_subclassing_adapter* adapter = nullptr;
#endif
#ifdef MACOS_ENABLED
		accesskit_macos_subclassing_adapter* adapter = nullptr;
#endif
#ifdef LINUXBSD_ENABLED
		accesskit_unix_adapter* adapter = nullptr;
#endif

		RID root_id;
		bool initial_update_completed = false;
		HashSet<RID> update;
		bool activated = false;
	};

	RID focus;

	HashMap<DisplayServerEnums::WindowID, WindowData> windows;

	HashMap<AccessibilityServerEnums::AccessibilityRole, accesskit_role> role_map;
	HashMap<AccessibilityServerEnums::AccessibilityAction, accesskit_action> action_map;

	_FORCE_INLINE_ accesskit_role _accessibility_role(
		AccessibilityServerEnums::AccessibilityRole p_role) const;
	_FORCE_INLINE_ accesskit_action _accessibility_action(
		AccessibilityServerEnums::AccessibilityAction p_action) const;

	void _free_recursive(WindowData* p_wd, const RID& p_id);
	_FORCE_INLINE_ void _ensure_node(const RID& p_id, AccessibilityElement* p_ae);

	static void _accessibility_action_callback(
		struct accesskit_action_request* p_request, void* p_user_data);
	static accesskit_tree_update* _accessibility_initial_tree_update_callback(void* p_user_data);
	static void _accessibility_deactivation_callback(void* p_user_data);
	static accesskit_tree_update* _accessibility_build_tree_update(void* p_user_data);

	bool in_accessibility_update = false;

public:
	bool is_supported() const override { return true; }

	bool window_create(DisplayServerEnums::WindowID p_window_id, void* p_handle) override;
	void window_destroy(DisplayServerEnums::WindowID p_window_id) override;

	RID create_element(DisplayServerEnums::WindowID p_window_id,
		AccessibilityServerEnums::AccessibilityRole p_role) override;
	RID create_sub_element(const RID& p_parent_rid,
		AccessibilityServerEnums::AccessibilityRole p_role, int p_insert_pos = -1) override;
	virtual RID create_sub_text_edit_elements(const RID& p_parent_rid, const RID& p_shaped_text,
		float p_min_height, int p_insert_pos = -1, bool p_is_last_line = false) override;
	bool has_element(const RID& p_id) const override;
	void free_element(const RID& p_id) override;


	void update_set_focus(const RID& p_id) override;
	RID get_window_root(DisplayServerEnums::WindowID p_window_id) const override;

	void set_window_rect(DisplayServerEnums::WindowID p_window_id, const Rect2& p_rect_out,
		const Rect2& p_rect_in) override;
	void set_window_focused(DisplayServerEnums::WindowID p_window_id, bool p_focused) override;
	void window_activation_completed(DisplayServerEnums::WindowID p_window_id) override;
	void window_deactivation_completed(DisplayServerEnums::WindowID p_window_id) override;

	void update_set_role(
		const RID& p_id, AccessibilityServerEnums::AccessibilityRole p_role) override;
	void update_set_name(const RID& p_id, const String& p_name) override;
	void update_set_braille_label(const RID& p_id, const String& p_name) override;
	void update_set_braille_role_description(const RID& p_id, const String& p_description) override;
	void update_set_extra_info(const RID& p_id, const String& p_name_extra_info) override;
	void update_set_description(const RID& p_id, const String& p_description) override;
	void update_set_value(const RID& p_id, const String& p_value) override;
	void update_set_tooltip(const RID& p_id, const String& p_tooltip) override;
	void update_set_bounds(const RID& p_id, const Rect2& p_rect) override;
	void update_set_transform(const RID& p_id, const Transform2D& p_transform) override;
	void update_add_child(const RID& p_id, const RID& p_child_id) override;
	void update_add_related_controls(const RID& p_id, const RID& p_related_id) override;
	void update_add_related_details(const RID& p_id, const RID& p_related_id) override;
	void update_add_related_described_by(const RID& p_id, const RID& p_related_id) override;
	void update_add_related_flow_to(const RID& p_id, const RID& p_related_id) override;
	void update_add_related_labeled_by(const RID& p_id, const RID& p_related_id) override;
	void update_add_related_radio_group(const RID& p_id, const RID& p_related_id) override;
	void update_set_active_descendant(const RID& p_id, const RID& p_other_id) override;
	void update_set_next_on_line(const RID& p_id, const RID& p_other_id) override;
	void update_set_previous_on_line(const RID& p_id, const RID& p_other_id) override;
	void update_set_member_of(const RID& p_id, const RID& p_group_id) override;
	void update_set_in_page_link_target(const RID& p_id, const RID& p_other_id) override;
	void update_set_error_message(const RID& p_id, const RID& p_other_id) override;
	void update_set_live(
		const RID& p_id, AccessibilityServerEnums::AccessibilityLiveMode p_live) override;
	void update_add_custom_action(
		const RID& p_id, int p_action_id, const String& p_action_description) override;
	void update_set_table_row_count(const RID& p_id, int p_count) override;
	void update_set_table_column_count(const RID& p_id, int p_count) override;
	void update_set_table_row_index(const RID& p_id, int p_index) override;
	void update_set_table_column_index(const RID& p_id, int p_index) override;
	void update_set_table_cell_position(
		const RID& p_id, int p_row_index, int p_column_index) override;
	void update_set_table_cell_span(const RID& p_id, int p_row_span, int p_column_span) override;
	void update_set_list_item_count(const RID& p_id, int p_size) override;
	void update_set_list_item_index(const RID& p_id, int p_index) override;
	void update_set_list_item_level(const RID& p_id, int p_level) override;
	void update_set_list_item_selected(const RID& p_id, bool p_selected) override;
	void update_set_list_item_expanded(const RID& p_id, bool p_expanded) override;
	void update_set_popup_type(
		const RID& p_id, AccessibilityServerEnums::AccessibilityPopupType p_popup) override;
	void update_set_checked(const RID& p_id, bool p_checekd) override;
	void update_set_num_value(const RID& p_id, double p_position) override;
	void update_set_num_range(const RID& p_id, double p_min, double p_max) override;
	void update_set_num_step(const RID& p_id, double p_step) override;
	void update_set_num_jump(const RID& p_id, double p_jump) override;
	void update_set_scroll_x(const RID& p_id, double p_position) override;
	void update_set_scroll_x_range(const RID& p_id, double p_min, double p_max) override;
	void update_set_scroll_y(const RID& p_id, double p_position) override;
	void update_set_scroll_y_range(const RID& p_id, double p_min, double p_max) override;
	void update_set_text_decorations(const RID& p_id, bool p_underline, bool p_strikethrough,
		bool p_overline, const Color& p_color) override;
	void update_set_text_align(const RID& p_id, HorizontalAlignment p_align) override;
	void update_set_text_selection(const RID& p_id, const RID& p_text_start_id, int p_start_char,
		const RID& p_text_end_id, int p_end_char) override;
	void update_set_flag(const RID& p_id, AccessibilityServerEnums::AccessibilityFlags p_flag,
		bool p_value) override;
	void update_set_classname(const RID& p_id, const String& p_classname) override;
	void update_set_placeholder(const RID& p_id, const String& p_placeholder) override;
	void update_set_language(const RID& p_id, const String& p_language) override;
	void update_set_text_orientation(const RID& p_id, bool p_vertical) override;
	void update_set_list_orientation(const RID& p_id, bool p_vertical) override;
	void update_set_shortcut(const RID& p_id, const String& p_shortcut) override;
	void update_set_url(const RID& p_id, const String& p_url) override;
	void update_set_role_description(const RID& p_id, const String& p_description) override;
	void update_set_state_description(const RID& p_id, const String& p_description) override;
	void update_set_color_value(const RID& p_id, const Color& p_color) override;
	void update_set_background_color(const RID& p_id, const Color& p_color) override;
	void update_set_foreground_color(const RID& p_id, const Color& p_color) override;

	static void register_create_func();

	AccessibilityServerAccessKit();
	~AccessibilityServerAccessKit();
};

#endif // ACCESSKIT_ENABLED
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
// --- Inlined/Folded from drivers/alsa/asound-so_wrap.h ---
#ifndef DYLIBLOAD_WRAPPER_ASOUND
#define DYLIBLOAD_WRAPPER_ASOUND
// This file is generated. Do not edit!
// see https://github.com/hpvb/dynload-wrapper for details
// generated by generate-wrapper.py 0.3 on 2023-01-12 10:26:13
// flags: generate-wrapper.py --include ./thirdparty/linuxbsd_headers/alsa/asoundlib.h --sys-include "thirdparty/linuxbsd_headers/alsa/asoundlib.h" --soname libasound.so.2 --init-name asound --omit-prefix snd_pcm_sw_params_set_tstamp_type --omit-prefix snd_pcm_status_get_audio_htstamp_report --omit-prefix snd_pcm_sw_params_get_tstamp_type --omit-prefix snd_pcm_status_set_audio_htstamp_config --output-header ./drivers/alsa/asound-so_wrap.h --output-implementation ./drivers/alsa/asound-so_wrap.c
//
#include <stdint.h>

#define snd_asoundlib_version snd_asoundlib_version_dylibloader_orig_asound
#define snd_dlopen snd_dlopen_dylibloader_orig_asound
#define snd_dlsym snd_dlsym_dylibloader_orig_asound
#define snd_dlclose snd_dlclose_dylibloader_orig_asound
#define snd_async_add_handler snd_async_add_handler_dylibloader_orig_asound
#define snd_async_del_handler snd_async_del_handler_dylibloader_orig_asound
#define snd_async_handler_get_fd snd_async_handler_get_fd_dylibloader_orig_asound
#define snd_async_handler_get_signo snd_async_handler_get_signo_dylibloader_orig_asound
#define snd_async_handler_get_callback_private snd_async_handler_get_callback_private_dylibloader_orig_asound
#define snd_shm_area_create snd_shm_area_create_dylibloader_orig_asound
#define snd_shm_area_share snd_shm_area_share_dylibloader_orig_asound
#define snd_shm_area_destroy snd_shm_area_destroy_dylibloader_orig_asound
#define snd_user_file snd_user_file_dylibloader_orig_asound
#define snd_input_stdio_open snd_input_stdio_open_dylibloader_orig_asound
#define snd_input_stdio_attach snd_input_stdio_attach_dylibloader_orig_asound
#define snd_input_buffer_open snd_input_buffer_open_dylibloader_orig_asound
#define snd_input_close snd_input_close_dylibloader_orig_asound
#define snd_input_scanf snd_input_scanf_dylibloader_orig_asound
#define snd_input_gets snd_input_gets_dylibloader_orig_asound
#define snd_input_getc snd_input_getc_dylibloader_orig_asound
#define snd_input_ungetc snd_input_ungetc_dylibloader_orig_asound
#define snd_output_stdio_open snd_output_stdio_open_dylibloader_orig_asound
#define snd_output_stdio_attach snd_output_stdio_attach_dylibloader_orig_asound
#define snd_output_buffer_open snd_output_buffer_open_dylibloader_orig_asound
#define snd_output_buffer_string snd_output_buffer_string_dylibloader_orig_asound
#define snd_output_close snd_output_close_dylibloader_orig_asound
#define snd_output_printf snd_output_printf_dylibloader_orig_asound
#define snd_output_vprintf snd_output_vprintf_dylibloader_orig_asound
#define snd_output_puts snd_output_puts_dylibloader_orig_asound
#define snd_output_putc snd_output_putc_dylibloader_orig_asound
#define snd_output_flush snd_output_flush_dylibloader_orig_asound
#define snd_strerror snd_strerror_dylibloader_orig_asound
#define snd_lib_error_set_handler snd_lib_error_set_handler_dylibloader_orig_asound
#define snd_lib_error_set_local snd_lib_error_set_local_dylibloader_orig_asound
#define snd_config_top snd_config_top_dylibloader_orig_asound
#define snd_config_load snd_config_load_dylibloader_orig_asound
#define snd_config_load_override snd_config_load_override_dylibloader_orig_asound
#define snd_config_save snd_config_save_dylibloader_orig_asound
#define snd_config_update snd_config_update_dylibloader_orig_asound
#define snd_config_update_r snd_config_update_r_dylibloader_orig_asound
#define snd_config_update_free snd_config_update_free_dylibloader_orig_asound
#define snd_config_update_free_global snd_config_update_free_global_dylibloader_orig_asound
#define snd_config_update_ref snd_config_update_ref_dylibloader_orig_asound
#define snd_config_ref snd_config_ref_dylibloader_orig_asound
#define snd_config_unref snd_config_unref_dylibloader_orig_asound
#define snd_config_search snd_config_search_dylibloader_orig_asound
#define snd_config_searchv snd_config_searchv_dylibloader_orig_asound
#define snd_config_search_definition snd_config_search_definition_dylibloader_orig_asound
#define snd_config_expand snd_config_expand_dylibloader_orig_asound
#define snd_config_evaluate snd_config_evaluate_dylibloader_orig_asound
#define snd_config_add snd_config_add_dylibloader_orig_asound
#define snd_config_delete snd_config_delete_dylibloader_orig_asound
#define snd_config_delete_compound_members snd_config_delete_compound_members_dylibloader_orig_asound
#define snd_config_copy snd_config_copy_dylibloader_orig_asound
#define snd_config_make snd_config_make_dylibloader_orig_asound
#define snd_config_make_integer snd_config_make_integer_dylibloader_orig_asound
#define snd_config_make_integer64 snd_config_make_integer64_dylibloader_orig_asound
#define snd_config_make_real snd_config_make_real_dylibloader_orig_asound
#define snd_config_make_string snd_config_make_string_dylibloader_orig_asound
#define snd_config_make_pointer snd_config_make_pointer_dylibloader_orig_asound
#define snd_config_make_compound snd_config_make_compound_dylibloader_orig_asound
#define snd_config_imake_integer snd_config_imake_integer_dylibloader_orig_asound
#define snd_config_imake_integer64 snd_config_imake_integer64_dylibloader_orig_asound
#define snd_config_imake_real snd_config_imake_real_dylibloader_orig_asound
#define snd_config_imake_string snd_config_imake_string_dylibloader_orig_asound
#define snd_config_imake_safe_string snd_config_imake_safe_string_dylibloader_orig_asound
#define snd_config_imake_pointer snd_config_imake_pointer_dylibloader_orig_asound
#define snd_config_get_type snd_config_get_type_dylibloader_orig_asound
#define snd_config_set_id snd_config_set_id_dylibloader_orig_asound
#define snd_config_set_integer snd_config_set_integer_dylibloader_orig_asound
#define snd_config_set_integer64 snd_config_set_integer64_dylibloader_orig_asound
#define snd_config_set_real snd_config_set_real_dylibloader_orig_asound
#define snd_config_set_string snd_config_set_string_dylibloader_orig_asound
#define snd_config_set_ascii snd_config_set_ascii_dylibloader_orig_asound
#define snd_config_set_pointer snd_config_set_pointer_dylibloader_orig_asound
#define snd_config_get_id snd_config_get_id_dylibloader_orig_asound
#define snd_config_get_integer snd_config_get_integer_dylibloader_orig_asound
#define snd_config_get_integer64 snd_config_get_integer64_dylibloader_orig_asound
#define snd_config_get_real snd_config_get_real_dylibloader_orig_asound
#define snd_config_get_ireal snd_config_get_ireal_dylibloader_orig_asound
#define snd_config_get_string snd_config_get_string_dylibloader_orig_asound
#define snd_config_get_ascii snd_config_get_ascii_dylibloader_orig_asound
#define snd_config_get_pointer snd_config_get_pointer_dylibloader_orig_asound
#define snd_config_test_id snd_config_test_id_dylibloader_orig_asound
#define snd_config_iterator_first snd_config_iterator_first_dylibloader_orig_asound
#define snd_config_iterator_next snd_config_iterator_next_dylibloader_orig_asound
#define snd_config_iterator_end snd_config_iterator_end_dylibloader_orig_asound
#define snd_config_iterator_entry snd_config_iterator_entry_dylibloader_orig_asound
#define snd_config_get_bool_ascii snd_config_get_bool_ascii_dylibloader_orig_asound
#define snd_config_get_bool snd_config_get_bool_dylibloader_orig_asound
#define snd_config_get_ctl_iface_ascii snd_config_get_ctl_iface_ascii_dylibloader_orig_asound
#define snd_config_get_ctl_iface snd_config_get_ctl_iface_dylibloader_orig_asound
#define snd_names_list snd_names_list_dylibloader_orig_asound
#define snd_names_list_free snd_names_list_free_dylibloader_orig_asound
#define snd_pcm_open snd_pcm_open_dylibloader_orig_asound
#define snd_pcm_open_lconf snd_pcm_open_lconf_dylibloader_orig_asound
#define snd_pcm_open_fallback snd_pcm_open_fallback_dylibloader_orig_asound
#define snd_pcm_close snd_pcm_close_dylibloader_orig_asound
#define snd_pcm_name snd_pcm_name_dylibloader_orig_asound
#define snd_pcm_type snd_pcm_type_dylibloader_orig_asound
#define snd_pcm_stream snd_pcm_stream_dylibloader_orig_asound
#define snd_pcm_poll_descriptors_count snd_pcm_poll_descriptors_count_dylibloader_orig_asound
#define snd_pcm_poll_descriptors snd_pcm_poll_descriptors_dylibloader_orig_asound
#define snd_pcm_poll_descriptors_revents snd_pcm_poll_descriptors_revents_dylibloader_orig_asound
#define snd_pcm_nonblock snd_pcm_nonblock_dylibloader_orig_asound
#define snd_async_add_pcm_handler snd_async_add_pcm_handler_dylibloader_orig_asound
#define snd_async_handler_get_pcm snd_async_handler_get_pcm_dylibloader_orig_asound
#define snd_pcm_info snd_pcm_info_dylibloader_orig_asound
#define snd_pcm_hw_params_current snd_pcm_hw_params_current_dylibloader_orig_asound
#define snd_pcm_hw_params snd_pcm_hw_params_dylibloader_orig_asound
#define snd_pcm_hw_free snd_pcm_hw_free_dylibloader_orig_asound
#define snd_pcm_sw_params_current snd_pcm_sw_params_current_dylibloader_orig_asound
#define snd_pcm_sw_params snd_pcm_sw_params_dylibloader_orig_asound
#define snd_pcm_prepare snd_pcm_prepare_dylibloader_orig_asound
#define snd_pcm_reset snd_pcm_reset_dylibloader_orig_asound
#define snd_pcm_status snd_pcm_status_dylibloader_orig_asound
#define snd_pcm_start snd_pcm_start_dylibloader_orig_asound
#define snd_pcm_drop snd_pcm_drop_dylibloader_orig_asound
#define snd_pcm_drain snd_pcm_drain_dylibloader_orig_asound
#define snd_pcm_pause snd_pcm_pause_dylibloader_orig_asound
#define snd_pcm_state snd_pcm_state_dylibloader_orig_asound
#define snd_pcm_hwsync snd_pcm_hwsync_dylibloader_orig_asound
#define snd_pcm_delay snd_pcm_delay_dylibloader_orig_asound
#define snd_pcm_resume snd_pcm_resume_dylibloader_orig_asound
#define snd_pcm_htimestamp snd_pcm_htimestamp_dylibloader_orig_asound
#define snd_pcm_avail snd_pcm_avail_dylibloader_orig_asound
#define snd_pcm_avail_update snd_pcm_avail_update_dylibloader_orig_asound
#define snd_pcm_avail_delay snd_pcm_avail_delay_dylibloader_orig_asound
#define snd_pcm_rewindable snd_pcm_rewindable_dylibloader_orig_asound
#define snd_pcm_rewind snd_pcm_rewind_dylibloader_orig_asound
#define snd_pcm_forwardable snd_pcm_forwardable_dylibloader_orig_asound
#define snd_pcm_forward snd_pcm_forward_dylibloader_orig_asound
#define snd_pcm_writei snd_pcm_writei_dylibloader_orig_asound
#define snd_pcm_readi snd_pcm_readi_dylibloader_orig_asound
#define snd_pcm_writen snd_pcm_writen_dylibloader_orig_asound
#define snd_pcm_readn snd_pcm_readn_dylibloader_orig_asound
#define snd_pcm_wait snd_pcm_wait_dylibloader_orig_asound
#define snd_pcm_link snd_pcm_link_dylibloader_orig_asound
#define snd_pcm_unlink snd_pcm_unlink_dylibloader_orig_asound
#define snd_pcm_query_chmaps snd_pcm_query_chmaps_dylibloader_orig_asound
#define snd_pcm_query_chmaps_from_hw snd_pcm_query_chmaps_from_hw_dylibloader_orig_asound
#define snd_pcm_free_chmaps snd_pcm_free_chmaps_dylibloader_orig_asound
#define snd_pcm_get_chmap snd_pcm_get_chmap_dylibloader_orig_asound
#define snd_pcm_set_chmap snd_pcm_set_chmap_dylibloader_orig_asound
#define snd_pcm_chmap_type_name snd_pcm_chmap_type_name_dylibloader_orig_asound
#define snd_pcm_chmap_name snd_pcm_chmap_name_dylibloader_orig_asound
#define snd_pcm_chmap_long_name snd_pcm_chmap_long_name_dylibloader_orig_asound
#define snd_pcm_chmap_print snd_pcm_chmap_print_dylibloader_orig_asound
#define snd_pcm_chmap_from_string snd_pcm_chmap_from_string_dylibloader_orig_asound
#define snd_pcm_chmap_parse_string snd_pcm_chmap_parse_string_dylibloader_orig_asound
#define snd_pcm_recover snd_pcm_recover_dylibloader_orig_asound
#define snd_pcm_set_params snd_pcm_set_params_dylibloader_orig_asound
#define snd_pcm_get_params snd_pcm_get_params_dylibloader_orig_asound
#define snd_pcm_info_sizeof snd_pcm_info_sizeof_dylibloader_orig_asound
#define snd_pcm_info_malloc snd_pcm_info_malloc_dylibloader_orig_asound
#define snd_pcm_info_free snd_pcm_info_free_dylibloader_orig_asound
#define snd_pcm_info_copy snd_pcm_info_copy_dylibloader_orig_asound
#define snd_pcm_info_get_device snd_pcm_info_get_device_dylibloader_orig_asound
#define snd_pcm_info_get_subdevice snd_pcm_info_get_subdevice_dylibloader_orig_asound
#define snd_pcm_info_get_stream snd_pcm_info_get_stream_dylibloader_orig_asound
#define snd_pcm_info_get_card snd_pcm_info_get_card_dylibloader_orig_asound
#define snd_pcm_info_get_id snd_pcm_info_get_id_dylibloader_orig_asound
#define snd_pcm_info_get_name snd_pcm_info_get_name_dylibloader_orig_asound
#define snd_pcm_info_get_subdevice_name snd_pcm_info_get_subdevice_name_dylibloader_orig_asound
#define snd_pcm_info_get_class snd_pcm_info_get_class_dylibloader_orig_asound
#define snd_pcm_info_get_subclass snd_pcm_info_get_subclass_dylibloader_orig_asound
#define snd_pcm_info_get_subdevices_count snd_pcm_info_get_subdevices_count_dylibloader_orig_asound
#define snd_pcm_info_get_subdevices_avail snd_pcm_info_get_subdevices_avail_dylibloader_orig_asound
#define snd_pcm_info_get_sync snd_pcm_info_get_sync_dylibloader_orig_asound
#define snd_pcm_info_set_device snd_pcm_info_set_device_dylibloader_orig_asound
#define snd_pcm_info_set_subdevice snd_pcm_info_set_subdevice_dylibloader_orig_asound
#define snd_pcm_info_set_stream snd_pcm_info_set_stream_dylibloader_orig_asound
#define snd_pcm_hw_params_any snd_pcm_hw_params_any_dylibloader_orig_asound
#define snd_pcm_hw_params_can_mmap_sample_resolution snd_pcm_hw_params_can_mmap_sample_resolution_dylibloader_orig_asound
#define snd_pcm_hw_params_is_double snd_pcm_hw_params_is_double_dylibloader_orig_asound
#define snd_pcm_hw_params_is_batch snd_pcm_hw_params_is_batch_dylibloader_orig_asound
#define snd_pcm_hw_params_is_block_transfer snd_pcm_hw_params_is_block_transfer_dylibloader_orig_asound
#define snd_pcm_hw_params_is_monotonic snd_pcm_hw_params_is_monotonic_dylibloader_orig_asound
#define snd_pcm_hw_params_can_overrange snd_pcm_hw_params_can_overrange_dylibloader_orig_asound
#define snd_pcm_hw_params_can_pause snd_pcm_hw_params_can_pause_dylibloader_orig_asound
#define snd_pcm_hw_params_can_resume snd_pcm_hw_params_can_resume_dylibloader_orig_asound
#define snd_pcm_hw_params_is_half_duplex snd_pcm_hw_params_is_half_duplex_dylibloader_orig_asound
#define snd_pcm_hw_params_is_joint_duplex snd_pcm_hw_params_is_joint_duplex_dylibloader_orig_asound
#define snd_pcm_hw_params_can_sync_start snd_pcm_hw_params_can_sync_start_dylibloader_orig_asound
#define snd_pcm_hw_params_can_disable_period_wakeup snd_pcm_hw_params_can_disable_period_wakeup_dylibloader_orig_asound
#define snd_pcm_hw_params_supports_audio_wallclock_ts snd_pcm_hw_params_supports_audio_wallclock_ts_dylibloader_orig_asound
#define snd_pcm_hw_params_supports_audio_ts_type snd_pcm_hw_params_supports_audio_ts_type_dylibloader_orig_asound
#define snd_pcm_hw_params_get_rate_numden snd_pcm_hw_params_get_rate_numden_dylibloader_orig_asound
#define snd_pcm_hw_params_get_sbits snd_pcm_hw_params_get_sbits_dylibloader_orig_asound
#define snd_pcm_hw_params_get_fifo_size snd_pcm_hw_params_get_fifo_size_dylibloader_orig_asound
#define snd_pcm_hw_params_sizeof snd_pcm_hw_params_sizeof_dylibloader_orig_asound
#define snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc_dylibloader_orig_asound
#define snd_pcm_hw_params_free snd_pcm_hw_params_free_dylibloader_orig_asound
#define snd_pcm_hw_params_copy snd_pcm_hw_params_copy_dylibloader_orig_asound
#define snd_pcm_hw_params_get_access snd_pcm_hw_params_get_access_dylibloader_orig_asound
#define snd_pcm_hw_params_test_access snd_pcm_hw_params_test_access_dylibloader_orig_asound
#define snd_pcm_hw_params_set_access snd_pcm_hw_params_set_access_dylibloader_orig_asound
#define snd_pcm_hw_params_set_access_first snd_pcm_hw_params_set_access_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_access_last snd_pcm_hw_params_set_access_last_dylibloader_orig_asound
#define snd_pcm_hw_params_set_access_mask snd_pcm_hw_params_set_access_mask_dylibloader_orig_asound
#define snd_pcm_hw_params_get_access_mask snd_pcm_hw_params_get_access_mask_dylibloader_orig_asound
#define snd_pcm_hw_params_get_format snd_pcm_hw_params_get_format_dylibloader_orig_asound
#define snd_pcm_hw_params_test_format snd_pcm_hw_params_test_format_dylibloader_orig_asound
#define snd_pcm_hw_params_set_format snd_pcm_hw_params_set_format_dylibloader_orig_asound
#define snd_pcm_hw_params_set_format_first snd_pcm_hw_params_set_format_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_format_last snd_pcm_hw_params_set_format_last_dylibloader_orig_asound
#define snd_pcm_hw_params_set_format_mask snd_pcm_hw_params_set_format_mask_dylibloader_orig_asound
#define snd_pcm_hw_params_get_format_mask snd_pcm_hw_params_get_format_mask_dylibloader_orig_asound
#define snd_pcm_hw_params_get_subformat snd_pcm_hw_params_get_subformat_dylibloader_orig_asound
#define snd_pcm_hw_params_test_subformat snd_pcm_hw_params_test_subformat_dylibloader_orig_asound
#define snd_pcm_hw_params_set_subformat snd_pcm_hw_params_set_subformat_dylibloader_orig_asound
#define snd_pcm_hw_params_set_subformat_first snd_pcm_hw_params_set_subformat_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_subformat_last snd_pcm_hw_params_set_subformat_last_dylibloader_orig_asound
#define snd_pcm_hw_params_set_subformat_mask snd_pcm_hw_params_set_subformat_mask_dylibloader_orig_asound
#define snd_pcm_hw_params_get_subformat_mask snd_pcm_hw_params_get_subformat_mask_dylibloader_orig_asound
#define snd_pcm_hw_params_get_channels snd_pcm_hw_params_get_channels_dylibloader_orig_asound
#define snd_pcm_hw_params_get_channels_min snd_pcm_hw_params_get_channels_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_channels_max snd_pcm_hw_params_get_channels_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_channels snd_pcm_hw_params_test_channels_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels snd_pcm_hw_params_set_channels_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels_min snd_pcm_hw_params_set_channels_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels_max snd_pcm_hw_params_set_channels_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels_minmax snd_pcm_hw_params_set_channels_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels_near snd_pcm_hw_params_set_channels_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels_first snd_pcm_hw_params_set_channels_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_channels_last snd_pcm_hw_params_set_channels_last_dylibloader_orig_asound
#define snd_pcm_hw_params_get_rate snd_pcm_hw_params_get_rate_dylibloader_orig_asound
#define snd_pcm_hw_params_get_rate_min snd_pcm_hw_params_get_rate_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_rate_max snd_pcm_hw_params_get_rate_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_rate snd_pcm_hw_params_test_rate_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate snd_pcm_hw_params_set_rate_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_min snd_pcm_hw_params_set_rate_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_max snd_pcm_hw_params_set_rate_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_minmax snd_pcm_hw_params_set_rate_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_near snd_pcm_hw_params_set_rate_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_first snd_pcm_hw_params_set_rate_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_last snd_pcm_hw_params_set_rate_last_dylibloader_orig_asound
#define snd_pcm_hw_params_set_rate_resample snd_pcm_hw_params_set_rate_resample_dylibloader_orig_asound
#define snd_pcm_hw_params_get_rate_resample snd_pcm_hw_params_get_rate_resample_dylibloader_orig_asound
#define snd_pcm_hw_params_set_export_buffer snd_pcm_hw_params_set_export_buffer_dylibloader_orig_asound
#define snd_pcm_hw_params_get_export_buffer snd_pcm_hw_params_get_export_buffer_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_wakeup snd_pcm_hw_params_set_period_wakeup_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_wakeup snd_pcm_hw_params_get_period_wakeup_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_time snd_pcm_hw_params_get_period_time_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_time_min snd_pcm_hw_params_get_period_time_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_time_max snd_pcm_hw_params_get_period_time_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_period_time snd_pcm_hw_params_test_period_time_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time snd_pcm_hw_params_set_period_time_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time_min snd_pcm_hw_params_set_period_time_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time_max snd_pcm_hw_params_set_period_time_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time_minmax snd_pcm_hw_params_set_period_time_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time_near snd_pcm_hw_params_set_period_time_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time_first snd_pcm_hw_params_set_period_time_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_time_last snd_pcm_hw_params_set_period_time_last_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_size snd_pcm_hw_params_get_period_size_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_size_min snd_pcm_hw_params_get_period_size_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_period_size_max snd_pcm_hw_params_get_period_size_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_period_size snd_pcm_hw_params_test_period_size_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size snd_pcm_hw_params_set_period_size_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_min snd_pcm_hw_params_set_period_size_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_max snd_pcm_hw_params_set_period_size_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_minmax snd_pcm_hw_params_set_period_size_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_near snd_pcm_hw_params_set_period_size_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_first snd_pcm_hw_params_set_period_size_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_last snd_pcm_hw_params_set_period_size_last_dylibloader_orig_asound
#define snd_pcm_hw_params_set_period_size_integer snd_pcm_hw_params_set_period_size_integer_dylibloader_orig_asound
#define snd_pcm_hw_params_get_periods snd_pcm_hw_params_get_periods_dylibloader_orig_asound
#define snd_pcm_hw_params_get_periods_min snd_pcm_hw_params_get_periods_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_periods_max snd_pcm_hw_params_get_periods_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_periods snd_pcm_hw_params_test_periods_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods snd_pcm_hw_params_set_periods_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_min snd_pcm_hw_params_set_periods_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_max snd_pcm_hw_params_set_periods_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_minmax snd_pcm_hw_params_set_periods_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_near snd_pcm_hw_params_set_periods_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_first snd_pcm_hw_params_set_periods_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_last snd_pcm_hw_params_set_periods_last_dylibloader_orig_asound
#define snd_pcm_hw_params_set_periods_integer snd_pcm_hw_params_set_periods_integer_dylibloader_orig_asound
#define snd_pcm_hw_params_get_buffer_time snd_pcm_hw_params_get_buffer_time_dylibloader_orig_asound
#define snd_pcm_hw_params_get_buffer_time_min snd_pcm_hw_params_get_buffer_time_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_buffer_time_max snd_pcm_hw_params_get_buffer_time_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_buffer_time snd_pcm_hw_params_test_buffer_time_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time snd_pcm_hw_params_set_buffer_time_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time_min snd_pcm_hw_params_set_buffer_time_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time_max snd_pcm_hw_params_set_buffer_time_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time_minmax snd_pcm_hw_params_set_buffer_time_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time_near snd_pcm_hw_params_set_buffer_time_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time_first snd_pcm_hw_params_set_buffer_time_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_time_last snd_pcm_hw_params_set_buffer_time_last_dylibloader_orig_asound
#define snd_pcm_hw_params_get_buffer_size snd_pcm_hw_params_get_buffer_size_dylibloader_orig_asound
#define snd_pcm_hw_params_get_buffer_size_min snd_pcm_hw_params_get_buffer_size_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_buffer_size_max snd_pcm_hw_params_get_buffer_size_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_buffer_size snd_pcm_hw_params_test_buffer_size_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size snd_pcm_hw_params_set_buffer_size_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size_min snd_pcm_hw_params_set_buffer_size_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size_max snd_pcm_hw_params_set_buffer_size_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size_minmax snd_pcm_hw_params_set_buffer_size_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size_near snd_pcm_hw_params_set_buffer_size_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size_first snd_pcm_hw_params_set_buffer_size_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_buffer_size_last snd_pcm_hw_params_set_buffer_size_last_dylibloader_orig_asound
#define snd_pcm_hw_params_get_min_align snd_pcm_hw_params_get_min_align_dylibloader_orig_asound
#define snd_pcm_sw_params_sizeof snd_pcm_sw_params_sizeof_dylibloader_orig_asound
#define snd_pcm_sw_params_malloc snd_pcm_sw_params_malloc_dylibloader_orig_asound
#define snd_pcm_sw_params_free snd_pcm_sw_params_free_dylibloader_orig_asound
#define snd_pcm_sw_params_copy snd_pcm_sw_params_copy_dylibloader_orig_asound
#define snd_pcm_sw_params_get_boundary snd_pcm_sw_params_get_boundary_dylibloader_orig_asound
#define snd_pcm_sw_params_set_tstamp_mode snd_pcm_sw_params_set_tstamp_mode_dylibloader_orig_asound
#define snd_pcm_sw_params_get_tstamp_mode snd_pcm_sw_params_get_tstamp_mode_dylibloader_orig_asound
#define snd_pcm_sw_params_set_avail_min snd_pcm_sw_params_set_avail_min_dylibloader_orig_asound
#define snd_pcm_sw_params_get_avail_min snd_pcm_sw_params_get_avail_min_dylibloader_orig_asound
#define snd_pcm_sw_params_set_period_event snd_pcm_sw_params_set_period_event_dylibloader_orig_asound
#define snd_pcm_sw_params_get_period_event snd_pcm_sw_params_get_period_event_dylibloader_orig_asound
#define snd_pcm_sw_params_set_start_threshold snd_pcm_sw_params_set_start_threshold_dylibloader_orig_asound
#define snd_pcm_sw_params_get_start_threshold snd_pcm_sw_params_get_start_threshold_dylibloader_orig_asound
#define snd_pcm_sw_params_set_stop_threshold snd_pcm_sw_params_set_stop_threshold_dylibloader_orig_asound
#define snd_pcm_sw_params_get_stop_threshold snd_pcm_sw_params_get_stop_threshold_dylibloader_orig_asound
#define snd_pcm_sw_params_set_silence_threshold snd_pcm_sw_params_set_silence_threshold_dylibloader_orig_asound
#define snd_pcm_sw_params_get_silence_threshold snd_pcm_sw_params_get_silence_threshold_dylibloader_orig_asound
#define snd_pcm_sw_params_set_silence_size snd_pcm_sw_params_set_silence_size_dylibloader_orig_asound
#define snd_pcm_sw_params_get_silence_size snd_pcm_sw_params_get_silence_size_dylibloader_orig_asound
#define snd_pcm_access_mask_sizeof snd_pcm_access_mask_sizeof_dylibloader_orig_asound
#define snd_pcm_access_mask_malloc snd_pcm_access_mask_malloc_dylibloader_orig_asound
#define snd_pcm_access_mask_free snd_pcm_access_mask_free_dylibloader_orig_asound
#define snd_pcm_access_mask_copy snd_pcm_access_mask_copy_dylibloader_orig_asound
#define snd_pcm_access_mask_none snd_pcm_access_mask_none_dylibloader_orig_asound
#define snd_pcm_access_mask_any snd_pcm_access_mask_any_dylibloader_orig_asound
#define snd_pcm_access_mask_test snd_pcm_access_mask_test_dylibloader_orig_asound
#define snd_pcm_access_mask_empty snd_pcm_access_mask_empty_dylibloader_orig_asound
#define snd_pcm_access_mask_set snd_pcm_access_mask_set_dylibloader_orig_asound
#define snd_pcm_access_mask_reset snd_pcm_access_mask_reset_dylibloader_orig_asound
#define snd_pcm_format_mask_sizeof snd_pcm_format_mask_sizeof_dylibloader_orig_asound
#define snd_pcm_format_mask_malloc snd_pcm_format_mask_malloc_dylibloader_orig_asound
#define snd_pcm_format_mask_free snd_pcm_format_mask_free_dylibloader_orig_asound
#define snd_pcm_format_mask_copy snd_pcm_format_mask_copy_dylibloader_orig_asound
#define snd_pcm_format_mask_none snd_pcm_format_mask_none_dylibloader_orig_asound
#define snd_pcm_format_mask_any snd_pcm_format_mask_any_dylibloader_orig_asound
#define snd_pcm_format_mask_test snd_pcm_format_mask_test_dylibloader_orig_asound
#define snd_pcm_format_mask_empty snd_pcm_format_mask_empty_dylibloader_orig_asound
#define snd_pcm_format_mask_set snd_pcm_format_mask_set_dylibloader_orig_asound
#define snd_pcm_format_mask_reset snd_pcm_format_mask_reset_dylibloader_orig_asound
#define snd_pcm_subformat_mask_sizeof snd_pcm_subformat_mask_sizeof_dylibloader_orig_asound
#define snd_pcm_subformat_mask_malloc snd_pcm_subformat_mask_malloc_dylibloader_orig_asound
#define snd_pcm_subformat_mask_free snd_pcm_subformat_mask_free_dylibloader_orig_asound
#define snd_pcm_subformat_mask_copy snd_pcm_subformat_mask_copy_dylibloader_orig_asound
#define snd_pcm_subformat_mask_none snd_pcm_subformat_mask_none_dylibloader_orig_asound
#define snd_pcm_subformat_mask_any snd_pcm_subformat_mask_any_dylibloader_orig_asound
#define snd_pcm_subformat_mask_test snd_pcm_subformat_mask_test_dylibloader_orig_asound
#define snd_pcm_subformat_mask_empty snd_pcm_subformat_mask_empty_dylibloader_orig_asound
#define snd_pcm_subformat_mask_set snd_pcm_subformat_mask_set_dylibloader_orig_asound
#define snd_pcm_subformat_mask_reset snd_pcm_subformat_mask_reset_dylibloader_orig_asound
#define snd_pcm_status_sizeof snd_pcm_status_sizeof_dylibloader_orig_asound
#define snd_pcm_status_malloc snd_pcm_status_malloc_dylibloader_orig_asound
#define snd_pcm_status_free snd_pcm_status_free_dylibloader_orig_asound
#define snd_pcm_status_copy snd_pcm_status_copy_dylibloader_orig_asound
#define snd_pcm_status_get_state snd_pcm_status_get_state_dylibloader_orig_asound
#define snd_pcm_status_get_trigger_tstamp snd_pcm_status_get_trigger_tstamp_dylibloader_orig_asound
#define snd_pcm_status_get_trigger_htstamp snd_pcm_status_get_trigger_htstamp_dylibloader_orig_asound
#define snd_pcm_status_get_tstamp snd_pcm_status_get_tstamp_dylibloader_orig_asound
#define snd_pcm_status_get_htstamp snd_pcm_status_get_htstamp_dylibloader_orig_asound
#define snd_pcm_status_get_audio_htstamp snd_pcm_status_get_audio_htstamp_dylibloader_orig_asound
#define snd_pcm_status_get_driver_htstamp snd_pcm_status_get_driver_htstamp_dylibloader_orig_asound
#define snd_pcm_status_get_delay snd_pcm_status_get_delay_dylibloader_orig_asound
#define snd_pcm_status_get_avail snd_pcm_status_get_avail_dylibloader_orig_asound
#define snd_pcm_status_get_avail_max snd_pcm_status_get_avail_max_dylibloader_orig_asound
#define snd_pcm_status_get_overrange snd_pcm_status_get_overrange_dylibloader_orig_asound
#define snd_pcm_type_name snd_pcm_type_name_dylibloader_orig_asound
#define snd_pcm_stream_name snd_pcm_stream_name_dylibloader_orig_asound
#define snd_pcm_access_name snd_pcm_access_name_dylibloader_orig_asound
#define snd_pcm_format_name snd_pcm_format_name_dylibloader_orig_asound
#define snd_pcm_format_description snd_pcm_format_description_dylibloader_orig_asound
#define snd_pcm_subformat_name snd_pcm_subformat_name_dylibloader_orig_asound
#define snd_pcm_subformat_description snd_pcm_subformat_description_dylibloader_orig_asound
#define snd_pcm_format_value snd_pcm_format_value_dylibloader_orig_asound
#define snd_pcm_tstamp_mode_name snd_pcm_tstamp_mode_name_dylibloader_orig_asound
#define snd_pcm_state_name snd_pcm_state_name_dylibloader_orig_asound
#define snd_pcm_dump snd_pcm_dump_dylibloader_orig_asound
#define snd_pcm_dump_hw_setup snd_pcm_dump_hw_setup_dylibloader_orig_asound
#define snd_pcm_dump_sw_setup snd_pcm_dump_sw_setup_dylibloader_orig_asound
#define snd_pcm_dump_setup snd_pcm_dump_setup_dylibloader_orig_asound
#define snd_pcm_hw_params_dump snd_pcm_hw_params_dump_dylibloader_orig_asound
#define snd_pcm_sw_params_dump snd_pcm_sw_params_dump_dylibloader_orig_asound
#define snd_pcm_status_dump snd_pcm_status_dump_dylibloader_orig_asound
#define snd_pcm_mmap_begin snd_pcm_mmap_begin_dylibloader_orig_asound
#define snd_pcm_mmap_commit snd_pcm_mmap_commit_dylibloader_orig_asound
#define snd_pcm_mmap_writei snd_pcm_mmap_writei_dylibloader_orig_asound
#define snd_pcm_mmap_readi snd_pcm_mmap_readi_dylibloader_orig_asound
#define snd_pcm_mmap_writen snd_pcm_mmap_writen_dylibloader_orig_asound
#define snd_pcm_mmap_readn snd_pcm_mmap_readn_dylibloader_orig_asound
#define snd_pcm_format_signed snd_pcm_format_signed_dylibloader_orig_asound
#define snd_pcm_format_unsigned snd_pcm_format_unsigned_dylibloader_orig_asound
#define snd_pcm_format_linear snd_pcm_format_linear_dylibloader_orig_asound
#define snd_pcm_format_float snd_pcm_format_float_dylibloader_orig_asound
#define snd_pcm_format_little_endian snd_pcm_format_little_endian_dylibloader_orig_asound
#define snd_pcm_format_big_endian snd_pcm_format_big_endian_dylibloader_orig_asound
#define snd_pcm_format_cpu_endian snd_pcm_format_cpu_endian_dylibloader_orig_asound
#define snd_pcm_format_width snd_pcm_format_width_dylibloader_orig_asound
#define snd_pcm_format_physical_width snd_pcm_format_physical_width_dylibloader_orig_asound
#define snd_pcm_build_linear_format snd_pcm_build_linear_format_dylibloader_orig_asound
#define snd_pcm_format_size snd_pcm_format_size_dylibloader_orig_asound
#define snd_pcm_format_silence snd_pcm_format_silence_dylibloader_orig_asound
#define snd_pcm_format_silence_16 snd_pcm_format_silence_16_dylibloader_orig_asound
#define snd_pcm_format_silence_32 snd_pcm_format_silence_32_dylibloader_orig_asound
#define snd_pcm_format_silence_64 snd_pcm_format_silence_64_dylibloader_orig_asound
#define snd_pcm_format_set_silence snd_pcm_format_set_silence_dylibloader_orig_asound
#define snd_pcm_bytes_to_frames snd_pcm_bytes_to_frames_dylibloader_orig_asound
#define snd_pcm_frames_to_bytes snd_pcm_frames_to_bytes_dylibloader_orig_asound
#define snd_pcm_bytes_to_samples snd_pcm_bytes_to_samples_dylibloader_orig_asound
#define snd_pcm_samples_to_bytes snd_pcm_samples_to_bytes_dylibloader_orig_asound
#define snd_pcm_area_silence snd_pcm_area_silence_dylibloader_orig_asound
#define snd_pcm_areas_silence snd_pcm_areas_silence_dylibloader_orig_asound
#define snd_pcm_area_copy snd_pcm_area_copy_dylibloader_orig_asound
#define snd_pcm_areas_copy snd_pcm_areas_copy_dylibloader_orig_asound
#define snd_pcm_hook_get_pcm snd_pcm_hook_get_pcm_dylibloader_orig_asound
#define snd_pcm_hook_get_private snd_pcm_hook_get_private_dylibloader_orig_asound
#define snd_pcm_hook_set_private snd_pcm_hook_set_private_dylibloader_orig_asound
#define snd_pcm_hook_add snd_pcm_hook_add_dylibloader_orig_asound
#define snd_pcm_hook_remove snd_pcm_hook_remove_dylibloader_orig_asound
#define snd_pcm_meter_get_bufsize snd_pcm_meter_get_bufsize_dylibloader_orig_asound
#define snd_pcm_meter_get_channels snd_pcm_meter_get_channels_dylibloader_orig_asound
#define snd_pcm_meter_get_rate snd_pcm_meter_get_rate_dylibloader_orig_asound
#define snd_pcm_meter_get_now snd_pcm_meter_get_now_dylibloader_orig_asound
#define snd_pcm_meter_get_boundary snd_pcm_meter_get_boundary_dylibloader_orig_asound
#define snd_pcm_meter_add_scope snd_pcm_meter_add_scope_dylibloader_orig_asound
#define snd_pcm_meter_search_scope snd_pcm_meter_search_scope_dylibloader_orig_asound
#define snd_pcm_scope_malloc snd_pcm_scope_malloc_dylibloader_orig_asound
#define snd_pcm_scope_set_ops snd_pcm_scope_set_ops_dylibloader_orig_asound
#define snd_pcm_scope_set_name snd_pcm_scope_set_name_dylibloader_orig_asound
#define snd_pcm_scope_get_name snd_pcm_scope_get_name_dylibloader_orig_asound
#define snd_pcm_scope_get_callback_private snd_pcm_scope_get_callback_private_dylibloader_orig_asound
#define snd_pcm_scope_set_callback_private snd_pcm_scope_set_callback_private_dylibloader_orig_asound
#define snd_pcm_scope_s16_open snd_pcm_scope_s16_open_dylibloader_orig_asound
#define snd_pcm_scope_s16_get_channel_buffer snd_pcm_scope_s16_get_channel_buffer_dylibloader_orig_asound
#define snd_spcm_init snd_spcm_init_dylibloader_orig_asound
#define snd_spcm_init_duplex snd_spcm_init_duplex_dylibloader_orig_asound
#define snd_spcm_init_get_params snd_spcm_init_get_params_dylibloader_orig_asound
#define snd_pcm_start_mode_name snd_pcm_start_mode_name_dylibloader_orig_asound
#define snd_pcm_xrun_mode_name snd_pcm_xrun_mode_name_dylibloader_orig_asound
#define snd_pcm_sw_params_set_start_mode snd_pcm_sw_params_set_start_mode_dylibloader_orig_asound
#define snd_pcm_sw_params_get_start_mode snd_pcm_sw_params_get_start_mode_dylibloader_orig_asound
#define snd_pcm_sw_params_set_xrun_mode snd_pcm_sw_params_set_xrun_mode_dylibloader_orig_asound
#define snd_pcm_sw_params_get_xrun_mode snd_pcm_sw_params_get_xrun_mode_dylibloader_orig_asound
#define snd_pcm_sw_params_set_xfer_align snd_pcm_sw_params_set_xfer_align_dylibloader_orig_asound
#define snd_pcm_sw_params_get_xfer_align snd_pcm_sw_params_get_xfer_align_dylibloader_orig_asound
#define snd_pcm_sw_params_set_sleep_min snd_pcm_sw_params_set_sleep_min_dylibloader_orig_asound
#define snd_pcm_sw_params_get_sleep_min snd_pcm_sw_params_get_sleep_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_tick_time snd_pcm_hw_params_get_tick_time_dylibloader_orig_asound
#define snd_pcm_hw_params_get_tick_time_min snd_pcm_hw_params_get_tick_time_min_dylibloader_orig_asound
#define snd_pcm_hw_params_get_tick_time_max snd_pcm_hw_params_get_tick_time_max_dylibloader_orig_asound
#define snd_pcm_hw_params_test_tick_time snd_pcm_hw_params_test_tick_time_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time snd_pcm_hw_params_set_tick_time_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time_min snd_pcm_hw_params_set_tick_time_min_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time_max snd_pcm_hw_params_set_tick_time_max_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time_minmax snd_pcm_hw_params_set_tick_time_minmax_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time_near snd_pcm_hw_params_set_tick_time_near_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time_first snd_pcm_hw_params_set_tick_time_first_dylibloader_orig_asound
#define snd_pcm_hw_params_set_tick_time_last snd_pcm_hw_params_set_tick_time_last_dylibloader_orig_asound
#define snd_rawmidi_open snd_rawmidi_open_dylibloader_orig_asound
#define snd_rawmidi_open_lconf snd_rawmidi_open_lconf_dylibloader_orig_asound
#define snd_rawmidi_close snd_rawmidi_close_dylibloader_orig_asound
#define snd_rawmidi_poll_descriptors_count snd_rawmidi_poll_descriptors_count_dylibloader_orig_asound
#define snd_rawmidi_poll_descriptors snd_rawmidi_poll_descriptors_dylibloader_orig_asound
#define snd_rawmidi_poll_descriptors_revents snd_rawmidi_poll_descriptors_revents_dylibloader_orig_asound
#define snd_rawmidi_nonblock snd_rawmidi_nonblock_dylibloader_orig_asound
#define snd_rawmidi_info_sizeof snd_rawmidi_info_sizeof_dylibloader_orig_asound
#define snd_rawmidi_info_malloc snd_rawmidi_info_malloc_dylibloader_orig_asound
#define snd_rawmidi_info_free snd_rawmidi_info_free_dylibloader_orig_asound
#define snd_rawmidi_info_copy snd_rawmidi_info_copy_dylibloader_orig_asound
#define snd_rawmidi_info_get_device snd_rawmidi_info_get_device_dylibloader_orig_asound
#define snd_rawmidi_info_get_subdevice snd_rawmidi_info_get_subdevice_dylibloader_orig_asound
#define snd_rawmidi_info_get_stream snd_rawmidi_info_get_stream_dylibloader_orig_asound
#define snd_rawmidi_info_get_card snd_rawmidi_info_get_card_dylibloader_orig_asound
#define snd_rawmidi_info_get_flags snd_rawmidi_info_get_flags_dylibloader_orig_asound
#define snd_rawmidi_info_get_id snd_rawmidi_info_get_id_dylibloader_orig_asound
#define snd_rawmidi_info_get_name snd_rawmidi_info_get_name_dylibloader_orig_asound
#define snd_rawmidi_info_get_subdevice_name snd_rawmidi_info_get_subdevice_name_dylibloader_orig_asound
#define snd_rawmidi_info_get_subdevices_count snd_rawmidi_info_get_subdevices_count_dylibloader_orig_asound
#define snd_rawmidi_info_get_subdevices_avail snd_rawmidi_info_get_subdevices_avail_dylibloader_orig_asound
#define snd_rawmidi_info_set_device snd_rawmidi_info_set_device_dylibloader_orig_asound
#define snd_rawmidi_info_set_subdevice snd_rawmidi_info_set_subdevice_dylibloader_orig_asound
#define snd_rawmidi_info_set_stream snd_rawmidi_info_set_stream_dylibloader_orig_asound
#define snd_rawmidi_info snd_rawmidi_info_dylibloader_orig_asound
#define snd_rawmidi_params_sizeof snd_rawmidi_params_sizeof_dylibloader_orig_asound
#define snd_rawmidi_params_malloc snd_rawmidi_params_malloc_dylibloader_orig_asound
#define snd_rawmidi_params_free snd_rawmidi_params_free_dylibloader_orig_asound
#define snd_rawmidi_params_copy snd_rawmidi_params_copy_dylibloader_orig_asound
#define snd_rawmidi_params_set_buffer_size snd_rawmidi_params_set_buffer_size_dylibloader_orig_asound
#define snd_rawmidi_params_get_buffer_size snd_rawmidi_params_get_buffer_size_dylibloader_orig_asound
#define snd_rawmidi_params_set_avail_min snd_rawmidi_params_set_avail_min_dylibloader_orig_asound
#define snd_rawmidi_params_get_avail_min snd_rawmidi_params_get_avail_min_dylibloader_orig_asound
#define snd_rawmidi_params_set_no_active_sensing snd_rawmidi_params_set_no_active_sensing_dylibloader_orig_asound
#define snd_rawmidi_params_get_no_active_sensing snd_rawmidi_params_get_no_active_sensing_dylibloader_orig_asound
#define snd_rawmidi_params snd_rawmidi_params_dylibloader_orig_asound
#define snd_rawmidi_params_current snd_rawmidi_params_current_dylibloader_orig_asound
#define snd_rawmidi_status_sizeof snd_rawmidi_status_sizeof_dylibloader_orig_asound
#define snd_rawmidi_status_malloc snd_rawmidi_status_malloc_dylibloader_orig_asound
#define snd_rawmidi_status_free snd_rawmidi_status_free_dylibloader_orig_asound
#define snd_rawmidi_status_copy snd_rawmidi_status_copy_dylibloader_orig_asound
#define snd_rawmidi_status_get_tstamp snd_rawmidi_status_get_tstamp_dylibloader_orig_asound
#define snd_rawmidi_status_get_avail snd_rawmidi_status_get_avail_dylibloader_orig_asound
#define snd_rawmidi_status_get_xruns snd_rawmidi_status_get_xruns_dylibloader_orig_asound
#define snd_rawmidi_status snd_rawmidi_status_dylibloader_orig_asound
#define snd_rawmidi_drain snd_rawmidi_drain_dylibloader_orig_asound
#define snd_rawmidi_drop snd_rawmidi_drop_dylibloader_orig_asound
#define snd_rawmidi_write snd_rawmidi_write_dylibloader_orig_asound
#define snd_rawmidi_read snd_rawmidi_read_dylibloader_orig_asound
#define snd_rawmidi_name snd_rawmidi_name_dylibloader_orig_asound
#define snd_rawmidi_type snd_rawmidi_type_dylibloader_orig_asound
#define snd_rawmidi_stream snd_rawmidi_stream_dylibloader_orig_asound
#define snd_timer_query_open snd_timer_query_open_dylibloader_orig_asound
#define snd_timer_query_open_lconf snd_timer_query_open_lconf_dylibloader_orig_asound
#define snd_timer_query_close snd_timer_query_close_dylibloader_orig_asound
#define snd_timer_query_next_device snd_timer_query_next_device_dylibloader_orig_asound
#define snd_timer_query_info snd_timer_query_info_dylibloader_orig_asound
#define snd_timer_query_params snd_timer_query_params_dylibloader_orig_asound
#define snd_timer_query_status snd_timer_query_status_dylibloader_orig_asound
#define snd_timer_open snd_timer_open_dylibloader_orig_asound
#define snd_timer_open_lconf snd_timer_open_lconf_dylibloader_orig_asound
#define snd_timer_close snd_timer_close_dylibloader_orig_asound
#define snd_async_add_timer_handler snd_async_add_timer_handler_dylibloader_orig_asound
#define snd_async_handler_get_timer snd_async_handler_get_timer_dylibloader_orig_asound
#define snd_timer_poll_descriptors_count snd_timer_poll_descriptors_count_dylibloader_orig_asound
#define snd_timer_poll_descriptors snd_timer_poll_descriptors_dylibloader_orig_asound
#define snd_timer_poll_descriptors_revents snd_timer_poll_descriptors_revents_dylibloader_orig_asound
#define snd_timer_info snd_timer_info_dylibloader_orig_asound
#define snd_timer_params snd_timer_params_dylibloader_orig_asound
#define snd_timer_status snd_timer_status_dylibloader_orig_asound
#define snd_timer_start snd_timer_start_dylibloader_orig_asound
#define snd_timer_stop snd_timer_stop_dylibloader_orig_asound
#define snd_timer_continue snd_timer_continue_dylibloader_orig_asound
#define snd_timer_read snd_timer_read_dylibloader_orig_asound
#define snd_timer_id_sizeof snd_timer_id_sizeof_dylibloader_orig_asound
#define snd_timer_id_malloc snd_timer_id_malloc_dylibloader_orig_asound
#define snd_timer_id_free snd_timer_id_free_dylibloader_orig_asound
#define snd_timer_id_copy snd_timer_id_copy_dylibloader_orig_asound
#define snd_timer_id_set_class snd_timer_id_set_class_dylibloader_orig_asound
#define snd_timer_id_get_class snd_timer_id_get_class_dylibloader_orig_asound
#define snd_timer_id_set_sclass snd_timer_id_set_sclass_dylibloader_orig_asound
#define snd_timer_id_get_sclass snd_timer_id_get_sclass_dylibloader_orig_asound
#define snd_timer_id_set_card snd_timer_id_set_card_dylibloader_orig_asound
#define snd_timer_id_get_card snd_timer_id_get_card_dylibloader_orig_asound
#define snd_timer_id_set_device snd_timer_id_set_device_dylibloader_orig_asound
#define snd_timer_id_get_device snd_timer_id_get_device_dylibloader_orig_asound
#define snd_timer_id_set_subdevice snd_timer_id_set_subdevice_dylibloader_orig_asound
#define snd_timer_id_get_subdevice snd_timer_id_get_subdevice_dylibloader_orig_asound
#define snd_timer_ginfo_sizeof snd_timer_ginfo_sizeof_dylibloader_orig_asound
#define snd_timer_ginfo_malloc snd_timer_ginfo_malloc_dylibloader_orig_asound
#define snd_timer_ginfo_free snd_timer_ginfo_free_dylibloader_orig_asound
#define snd_timer_ginfo_copy snd_timer_ginfo_copy_dylibloader_orig_asound
#define snd_timer_ginfo_set_tid snd_timer_ginfo_set_tid_dylibloader_orig_asound
#define snd_timer_ginfo_get_tid snd_timer_ginfo_get_tid_dylibloader_orig_asound
#define snd_timer_ginfo_get_flags snd_timer_ginfo_get_flags_dylibloader_orig_asound
#define snd_timer_ginfo_get_card snd_timer_ginfo_get_card_dylibloader_orig_asound
#define snd_timer_ginfo_get_id snd_timer_ginfo_get_id_dylibloader_orig_asound
#define snd_timer_ginfo_get_name snd_timer_ginfo_get_name_dylibloader_orig_asound
#define snd_timer_ginfo_get_resolution snd_timer_ginfo_get_resolution_dylibloader_orig_asound
#define snd_timer_ginfo_get_resolution_min snd_timer_ginfo_get_resolution_min_dylibloader_orig_asound
#define snd_timer_ginfo_get_resolution_max snd_timer_ginfo_get_resolution_max_dylibloader_orig_asound
#define snd_timer_ginfo_get_clients snd_timer_ginfo_get_clients_dylibloader_orig_asound
#define snd_timer_info_sizeof snd_timer_info_sizeof_dylibloader_orig_asound
#define snd_timer_info_malloc snd_timer_info_malloc_dylibloader_orig_asound
#define snd_timer_info_free snd_timer_info_free_dylibloader_orig_asound
#define snd_timer_info_copy snd_timer_info_copy_dylibloader_orig_asound
#define snd_timer_info_is_slave snd_timer_info_is_slave_dylibloader_orig_asound
#define snd_timer_info_get_card snd_timer_info_get_card_dylibloader_orig_asound
#define snd_timer_info_get_id snd_timer_info_get_id_dylibloader_orig_asound
#define snd_timer_info_get_name snd_timer_info_get_name_dylibloader_orig_asound
#define snd_timer_info_get_resolution snd_timer_info_get_resolution_dylibloader_orig_asound
#define snd_timer_params_sizeof snd_timer_params_sizeof_dylibloader_orig_asound
#define snd_timer_params_malloc snd_timer_params_malloc_dylibloader_orig_asound
#define snd_timer_params_free snd_timer_params_free_dylibloader_orig_asound
#define snd_timer_params_copy snd_timer_params_copy_dylibloader_orig_asound
#define snd_timer_params_set_auto_start snd_timer_params_set_auto_start_dylibloader_orig_asound
#define snd_timer_params_get_auto_start snd_timer_params_get_auto_start_dylibloader_orig_asound
#define snd_timer_params_set_exclusive snd_timer_params_set_exclusive_dylibloader_orig_asound
#define snd_timer_params_get_exclusive snd_timer_params_get_exclusive_dylibloader_orig_asound
#define snd_timer_params_set_early_event snd_timer_params_set_early_event_dylibloader_orig_asound
#define snd_timer_params_get_early_event snd_timer_params_get_early_event_dylibloader_orig_asound
#define snd_timer_params_set_ticks snd_timer_params_set_ticks_dylibloader_orig_asound
#define snd_timer_params_get_ticks snd_timer_params_get_ticks_dylibloader_orig_asound
#define snd_timer_params_set_queue_size snd_timer_params_set_queue_size_dylibloader_orig_asound
#define snd_timer_params_get_queue_size snd_timer_params_get_queue_size_dylibloader_orig_asound
#define snd_timer_params_set_filter snd_timer_params_set_filter_dylibloader_orig_asound
#define snd_timer_params_get_filter snd_timer_params_get_filter_dylibloader_orig_asound
#define snd_timer_status_sizeof snd_timer_status_sizeof_dylibloader_orig_asound
#define snd_timer_status_malloc snd_timer_status_malloc_dylibloader_orig_asound
#define snd_timer_status_free snd_timer_status_free_dylibloader_orig_asound
#define snd_timer_status_copy snd_timer_status_copy_dylibloader_orig_asound
#define snd_timer_status_get_timestamp snd_timer_status_get_timestamp_dylibloader_orig_asound
#define snd_timer_status_get_resolution snd_timer_status_get_resolution_dylibloader_orig_asound
#define snd_timer_status_get_lost snd_timer_status_get_lost_dylibloader_orig_asound
#define snd_timer_status_get_overrun snd_timer_status_get_overrun_dylibloader_orig_asound
#define snd_timer_status_get_queue snd_timer_status_get_queue_dylibloader_orig_asound
#define snd_timer_info_get_ticks snd_timer_info_get_ticks_dylibloader_orig_asound
#define snd_hwdep_open snd_hwdep_open_dylibloader_orig_asound
#define snd_hwdep_close snd_hwdep_close_dylibloader_orig_asound
#define snd_hwdep_poll_descriptors snd_hwdep_poll_descriptors_dylibloader_orig_asound
#define snd_hwdep_poll_descriptors_count snd_hwdep_poll_descriptors_count_dylibloader_orig_asound
#define snd_hwdep_poll_descriptors_revents snd_hwdep_poll_descriptors_revents_dylibloader_orig_asound
#define snd_hwdep_nonblock snd_hwdep_nonblock_dylibloader_orig_asound
#define snd_hwdep_info snd_hwdep_info_dylibloader_orig_asound
#define snd_hwdep_dsp_status snd_hwdep_dsp_status_dylibloader_orig_asound
#define snd_hwdep_dsp_load snd_hwdep_dsp_load_dylibloader_orig_asound
#define snd_hwdep_ioctl snd_hwdep_ioctl_dylibloader_orig_asound
#define snd_hwdep_write snd_hwdep_write_dylibloader_orig_asound
#define snd_hwdep_read snd_hwdep_read_dylibloader_orig_asound
#define snd_hwdep_info_sizeof snd_hwdep_info_sizeof_dylibloader_orig_asound
#define snd_hwdep_info_malloc snd_hwdep_info_malloc_dylibloader_orig_asound
#define snd_hwdep_info_free snd_hwdep_info_free_dylibloader_orig_asound
#define snd_hwdep_info_copy snd_hwdep_info_copy_dylibloader_orig_asound
#define snd_hwdep_info_get_device snd_hwdep_info_get_device_dylibloader_orig_asound
#define snd_hwdep_info_get_card snd_hwdep_info_get_card_dylibloader_orig_asound
#define snd_hwdep_info_get_id snd_hwdep_info_get_id_dylibloader_orig_asound
#define snd_hwdep_info_get_name snd_hwdep_info_get_name_dylibloader_orig_asound
#define snd_hwdep_info_get_iface snd_hwdep_info_get_iface_dylibloader_orig_asound
#define snd_hwdep_info_set_device snd_hwdep_info_set_device_dylibloader_orig_asound
#define snd_hwdep_dsp_status_sizeof snd_hwdep_dsp_status_sizeof_dylibloader_orig_asound
#define snd_hwdep_dsp_status_malloc snd_hwdep_dsp_status_malloc_dylibloader_orig_asound
#define snd_hwdep_dsp_status_free snd_hwdep_dsp_status_free_dylibloader_orig_asound
#define snd_hwdep_dsp_status_copy snd_hwdep_dsp_status_copy_dylibloader_orig_asound
#define snd_hwdep_dsp_status_get_version snd_hwdep_dsp_status_get_version_dylibloader_orig_asound
#define snd_hwdep_dsp_status_get_id snd_hwdep_dsp_status_get_id_dylibloader_orig_asound
#define snd_hwdep_dsp_status_get_num_dsps snd_hwdep_dsp_status_get_num_dsps_dylibloader_orig_asound
#define snd_hwdep_dsp_status_get_dsp_loaded snd_hwdep_dsp_status_get_dsp_loaded_dylibloader_orig_asound
#define snd_hwdep_dsp_status_get_chip_ready snd_hwdep_dsp_status_get_chip_ready_dylibloader_orig_asound
#define snd_hwdep_dsp_image_sizeof snd_hwdep_dsp_image_sizeof_dylibloader_orig_asound
#define snd_hwdep_dsp_image_malloc snd_hwdep_dsp_image_malloc_dylibloader_orig_asound
#define snd_hwdep_dsp_image_free snd_hwdep_dsp_image_free_dylibloader_orig_asound
#define snd_hwdep_dsp_image_copy snd_hwdep_dsp_image_copy_dylibloader_orig_asound
#define snd_hwdep_dsp_image_get_index snd_hwdep_dsp_image_get_index_dylibloader_orig_asound
#define snd_hwdep_dsp_image_get_name snd_hwdep_dsp_image_get_name_dylibloader_orig_asound
#define snd_hwdep_dsp_image_get_image snd_hwdep_dsp_image_get_image_dylibloader_orig_asound
#define snd_hwdep_dsp_image_get_length snd_hwdep_dsp_image_get_length_dylibloader_orig_asound
#define snd_hwdep_dsp_image_set_index snd_hwdep_dsp_image_set_index_dylibloader_orig_asound
#define snd_hwdep_dsp_image_set_name snd_hwdep_dsp_image_set_name_dylibloader_orig_asound
#define snd_hwdep_dsp_image_set_image snd_hwdep_dsp_image_set_image_dylibloader_orig_asound
#define snd_hwdep_dsp_image_set_length snd_hwdep_dsp_image_set_length_dylibloader_orig_asound
#define snd_card_load snd_card_load_dylibloader_orig_asound
#define snd_card_next snd_card_next_dylibloader_orig_asound
#define snd_card_get_index snd_card_get_index_dylibloader_orig_asound
#define snd_card_get_name snd_card_get_name_dylibloader_orig_asound
#define snd_card_get_longname snd_card_get_longname_dylibloader_orig_asound
#define snd_device_name_hint snd_device_name_hint_dylibloader_orig_asound
#define snd_device_name_free_hint snd_device_name_free_hint_dylibloader_orig_asound
#define snd_device_name_get_hint snd_device_name_get_hint_dylibloader_orig_asound
#define snd_ctl_open snd_ctl_open_dylibloader_orig_asound
#define snd_ctl_open_lconf snd_ctl_open_lconf_dylibloader_orig_asound
#define snd_ctl_open_fallback snd_ctl_open_fallback_dylibloader_orig_asound
#define snd_ctl_close snd_ctl_close_dylibloader_orig_asound
#define snd_ctl_nonblock snd_ctl_nonblock_dylibloader_orig_asound
#define snd_async_add_ctl_handler snd_async_add_ctl_handler_dylibloader_orig_asound
#define snd_async_handler_get_ctl snd_async_handler_get_ctl_dylibloader_orig_asound
#define snd_ctl_poll_descriptors_count snd_ctl_poll_descriptors_count_dylibloader_orig_asound
#define snd_ctl_poll_descriptors snd_ctl_poll_descriptors_dylibloader_orig_asound
#define snd_ctl_poll_descriptors_revents snd_ctl_poll_descriptors_revents_dylibloader_orig_asound
#define snd_ctl_subscribe_events snd_ctl_subscribe_events_dylibloader_orig_asound
#define snd_ctl_card_info snd_ctl_card_info_dylibloader_orig_asound
#define snd_ctl_elem_list snd_ctl_elem_list_dylibloader_orig_asound
#define snd_ctl_elem_info snd_ctl_elem_info_dylibloader_orig_asound
#define snd_ctl_elem_read snd_ctl_elem_read_dylibloader_orig_asound
#define snd_ctl_elem_write snd_ctl_elem_write_dylibloader_orig_asound
#define snd_ctl_elem_lock snd_ctl_elem_lock_dylibloader_orig_asound
#define snd_ctl_elem_unlock snd_ctl_elem_unlock_dylibloader_orig_asound
#define snd_ctl_elem_tlv_read snd_ctl_elem_tlv_read_dylibloader_orig_asound
#define snd_ctl_elem_tlv_write snd_ctl_elem_tlv_write_dylibloader_orig_asound
#define snd_ctl_elem_tlv_command snd_ctl_elem_tlv_command_dylibloader_orig_asound
#define snd_ctl_hwdep_next_device snd_ctl_hwdep_next_device_dylibloader_orig_asound
#define snd_ctl_hwdep_info snd_ctl_hwdep_info_dylibloader_orig_asound
#define snd_ctl_pcm_next_device snd_ctl_pcm_next_device_dylibloader_orig_asound
#define snd_ctl_pcm_info snd_ctl_pcm_info_dylibloader_orig_asound
#define snd_ctl_pcm_prefer_subdevice snd_ctl_pcm_prefer_subdevice_dylibloader_orig_asound
#define snd_ctl_rawmidi_next_device snd_ctl_rawmidi_next_device_dylibloader_orig_asound
#define snd_ctl_rawmidi_info snd_ctl_rawmidi_info_dylibloader_orig_asound
#define snd_ctl_rawmidi_prefer_subdevice snd_ctl_rawmidi_prefer_subdevice_dylibloader_orig_asound
#define snd_ctl_set_power_state snd_ctl_set_power_state_dylibloader_orig_asound
#define snd_ctl_get_power_state snd_ctl_get_power_state_dylibloader_orig_asound
#define snd_ctl_read snd_ctl_read_dylibloader_orig_asound
#define snd_ctl_wait snd_ctl_wait_dylibloader_orig_asound
#define snd_ctl_name snd_ctl_name_dylibloader_orig_asound
#define snd_ctl_type snd_ctl_type_dylibloader_orig_asound
#define snd_ctl_elem_type_name snd_ctl_elem_type_name_dylibloader_orig_asound
#define snd_ctl_elem_iface_name snd_ctl_elem_iface_name_dylibloader_orig_asound
#define snd_ctl_event_type_name snd_ctl_event_type_name_dylibloader_orig_asound
#define snd_ctl_event_elem_get_mask snd_ctl_event_elem_get_mask_dylibloader_orig_asound
#define snd_ctl_event_elem_get_numid snd_ctl_event_elem_get_numid_dylibloader_orig_asound
#define snd_ctl_event_elem_get_id snd_ctl_event_elem_get_id_dylibloader_orig_asound
#define snd_ctl_event_elem_get_interface snd_ctl_event_elem_get_interface_dylibloader_orig_asound
#define snd_ctl_event_elem_get_device snd_ctl_event_elem_get_device_dylibloader_orig_asound
#define snd_ctl_event_elem_get_subdevice snd_ctl_event_elem_get_subdevice_dylibloader_orig_asound
#define snd_ctl_event_elem_get_name snd_ctl_event_elem_get_name_dylibloader_orig_asound
#define snd_ctl_event_elem_get_index snd_ctl_event_elem_get_index_dylibloader_orig_asound
#define snd_ctl_elem_list_alloc_space snd_ctl_elem_list_alloc_space_dylibloader_orig_asound
#define snd_ctl_elem_list_free_space snd_ctl_elem_list_free_space_dylibloader_orig_asound
#define snd_ctl_ascii_elem_id_get snd_ctl_ascii_elem_id_get_dylibloader_orig_asound
#define snd_ctl_ascii_elem_id_parse snd_ctl_ascii_elem_id_parse_dylibloader_orig_asound
#define snd_ctl_ascii_value_parse snd_ctl_ascii_value_parse_dylibloader_orig_asound
#define snd_ctl_elem_id_sizeof snd_ctl_elem_id_sizeof_dylibloader_orig_asound
#define snd_ctl_elem_id_malloc snd_ctl_elem_id_malloc_dylibloader_orig_asound
#define snd_ctl_elem_id_free snd_ctl_elem_id_free_dylibloader_orig_asound
#define snd_ctl_elem_id_clear snd_ctl_elem_id_clear_dylibloader_orig_asound
#define snd_ctl_elem_id_copy snd_ctl_elem_id_copy_dylibloader_orig_asound
#define snd_ctl_elem_id_get_numid snd_ctl_elem_id_get_numid_dylibloader_orig_asound
#define snd_ctl_elem_id_get_interface snd_ctl_elem_id_get_interface_dylibloader_orig_asound
#define snd_ctl_elem_id_get_device snd_ctl_elem_id_get_device_dylibloader_orig_asound
#define snd_ctl_elem_id_get_subdevice snd_ctl_elem_id_get_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_id_get_name snd_ctl_elem_id_get_name_dylibloader_orig_asound
#define snd_ctl_elem_id_get_index snd_ctl_elem_id_get_index_dylibloader_orig_asound
#define snd_ctl_elem_id_set_numid snd_ctl_elem_id_set_numid_dylibloader_orig_asound
#define snd_ctl_elem_id_set_interface snd_ctl_elem_id_set_interface_dylibloader_orig_asound
#define snd_ctl_elem_id_set_device snd_ctl_elem_id_set_device_dylibloader_orig_asound
#define snd_ctl_elem_id_set_subdevice snd_ctl_elem_id_set_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_id_set_name snd_ctl_elem_id_set_name_dylibloader_orig_asound
#define snd_ctl_elem_id_set_index snd_ctl_elem_id_set_index_dylibloader_orig_asound
#define snd_ctl_card_info_sizeof snd_ctl_card_info_sizeof_dylibloader_orig_asound
#define snd_ctl_card_info_malloc snd_ctl_card_info_malloc_dylibloader_orig_asound
#define snd_ctl_card_info_free snd_ctl_card_info_free_dylibloader_orig_asound
#define snd_ctl_card_info_clear snd_ctl_card_info_clear_dylibloader_orig_asound
#define snd_ctl_card_info_copy snd_ctl_card_info_copy_dylibloader_orig_asound
#define snd_ctl_card_info_get_card snd_ctl_card_info_get_card_dylibloader_orig_asound
#define snd_ctl_card_info_get_id snd_ctl_card_info_get_id_dylibloader_orig_asound
#define snd_ctl_card_info_get_driver snd_ctl_card_info_get_driver_dylibloader_orig_asound
#define snd_ctl_card_info_get_name snd_ctl_card_info_get_name_dylibloader_orig_asound
#define snd_ctl_card_info_get_longname snd_ctl_card_info_get_longname_dylibloader_orig_asound
#define snd_ctl_card_info_get_mixername snd_ctl_card_info_get_mixername_dylibloader_orig_asound
#define snd_ctl_card_info_get_components snd_ctl_card_info_get_components_dylibloader_orig_asound
#define snd_ctl_event_sizeof snd_ctl_event_sizeof_dylibloader_orig_asound
#define snd_ctl_event_malloc snd_ctl_event_malloc_dylibloader_orig_asound
#define snd_ctl_event_free snd_ctl_event_free_dylibloader_orig_asound
#define snd_ctl_event_clear snd_ctl_event_clear_dylibloader_orig_asound
#define snd_ctl_event_copy snd_ctl_event_copy_dylibloader_orig_asound
#define snd_ctl_event_get_type snd_ctl_event_get_type_dylibloader_orig_asound
#define snd_ctl_elem_list_sizeof snd_ctl_elem_list_sizeof_dylibloader_orig_asound
#define snd_ctl_elem_list_malloc snd_ctl_elem_list_malloc_dylibloader_orig_asound
#define snd_ctl_elem_list_free snd_ctl_elem_list_free_dylibloader_orig_asound
#define snd_ctl_elem_list_clear snd_ctl_elem_list_clear_dylibloader_orig_asound
#define snd_ctl_elem_list_copy snd_ctl_elem_list_copy_dylibloader_orig_asound
#define snd_ctl_elem_list_set_offset snd_ctl_elem_list_set_offset_dylibloader_orig_asound
#define snd_ctl_elem_list_get_used snd_ctl_elem_list_get_used_dylibloader_orig_asound
#define snd_ctl_elem_list_get_count snd_ctl_elem_list_get_count_dylibloader_orig_asound
#define snd_ctl_elem_list_get_id snd_ctl_elem_list_get_id_dylibloader_orig_asound
#define snd_ctl_elem_list_get_numid snd_ctl_elem_list_get_numid_dylibloader_orig_asound
#define snd_ctl_elem_list_get_interface snd_ctl_elem_list_get_interface_dylibloader_orig_asound
#define snd_ctl_elem_list_get_device snd_ctl_elem_list_get_device_dylibloader_orig_asound
#define snd_ctl_elem_list_get_subdevice snd_ctl_elem_list_get_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_list_get_name snd_ctl_elem_list_get_name_dylibloader_orig_asound
#define snd_ctl_elem_list_get_index snd_ctl_elem_list_get_index_dylibloader_orig_asound
#define snd_ctl_elem_info_sizeof snd_ctl_elem_info_sizeof_dylibloader_orig_asound
#define snd_ctl_elem_info_malloc snd_ctl_elem_info_malloc_dylibloader_orig_asound
#define snd_ctl_elem_info_free snd_ctl_elem_info_free_dylibloader_orig_asound
#define snd_ctl_elem_info_clear snd_ctl_elem_info_clear_dylibloader_orig_asound
#define snd_ctl_elem_info_copy snd_ctl_elem_info_copy_dylibloader_orig_asound
#define snd_ctl_elem_info_get_type snd_ctl_elem_info_get_type_dylibloader_orig_asound
#define snd_ctl_elem_info_is_readable snd_ctl_elem_info_is_readable_dylibloader_orig_asound
#define snd_ctl_elem_info_is_writable snd_ctl_elem_info_is_writable_dylibloader_orig_asound
#define snd_ctl_elem_info_is_volatile snd_ctl_elem_info_is_volatile_dylibloader_orig_asound
#define snd_ctl_elem_info_is_inactive snd_ctl_elem_info_is_inactive_dylibloader_orig_asound
#define snd_ctl_elem_info_is_locked snd_ctl_elem_info_is_locked_dylibloader_orig_asound
#define snd_ctl_elem_info_is_tlv_readable snd_ctl_elem_info_is_tlv_readable_dylibloader_orig_asound
#define snd_ctl_elem_info_is_tlv_writable snd_ctl_elem_info_is_tlv_writable_dylibloader_orig_asound
#define snd_ctl_elem_info_is_tlv_commandable snd_ctl_elem_info_is_tlv_commandable_dylibloader_orig_asound
#define snd_ctl_elem_info_is_owner snd_ctl_elem_info_is_owner_dylibloader_orig_asound
#define snd_ctl_elem_info_is_user snd_ctl_elem_info_is_user_dylibloader_orig_asound
#define snd_ctl_elem_info_get_owner snd_ctl_elem_info_get_owner_dylibloader_orig_asound
#define snd_ctl_elem_info_get_count snd_ctl_elem_info_get_count_dylibloader_orig_asound
#define snd_ctl_elem_info_get_min snd_ctl_elem_info_get_min_dylibloader_orig_asound
#define snd_ctl_elem_info_get_max snd_ctl_elem_info_get_max_dylibloader_orig_asound
#define snd_ctl_elem_info_get_step snd_ctl_elem_info_get_step_dylibloader_orig_asound
#define snd_ctl_elem_info_get_min64 snd_ctl_elem_info_get_min64_dylibloader_orig_asound
#define snd_ctl_elem_info_get_max64 snd_ctl_elem_info_get_max64_dylibloader_orig_asound
#define snd_ctl_elem_info_get_step64 snd_ctl_elem_info_get_step64_dylibloader_orig_asound
#define snd_ctl_elem_info_get_items snd_ctl_elem_info_get_items_dylibloader_orig_asound
#define snd_ctl_elem_info_set_item snd_ctl_elem_info_set_item_dylibloader_orig_asound
#define snd_ctl_elem_info_get_item_name snd_ctl_elem_info_get_item_name_dylibloader_orig_asound
#define snd_ctl_elem_info_get_dimensions snd_ctl_elem_info_get_dimensions_dylibloader_orig_asound
#define snd_ctl_elem_info_get_dimension snd_ctl_elem_info_get_dimension_dylibloader_orig_asound
#define snd_ctl_elem_info_set_dimension snd_ctl_elem_info_set_dimension_dylibloader_orig_asound
#define snd_ctl_elem_info_get_id snd_ctl_elem_info_get_id_dylibloader_orig_asound
#define snd_ctl_elem_info_get_numid snd_ctl_elem_info_get_numid_dylibloader_orig_asound
#define snd_ctl_elem_info_get_interface snd_ctl_elem_info_get_interface_dylibloader_orig_asound
#define snd_ctl_elem_info_get_device snd_ctl_elem_info_get_device_dylibloader_orig_asound
#define snd_ctl_elem_info_get_subdevice snd_ctl_elem_info_get_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_info_get_name snd_ctl_elem_info_get_name_dylibloader_orig_asound
#define snd_ctl_elem_info_get_index snd_ctl_elem_info_get_index_dylibloader_orig_asound
#define snd_ctl_elem_info_set_id snd_ctl_elem_info_set_id_dylibloader_orig_asound
#define snd_ctl_elem_info_set_numid snd_ctl_elem_info_set_numid_dylibloader_orig_asound
#define snd_ctl_elem_info_set_interface snd_ctl_elem_info_set_interface_dylibloader_orig_asound
#define snd_ctl_elem_info_set_device snd_ctl_elem_info_set_device_dylibloader_orig_asound
#define snd_ctl_elem_info_set_subdevice snd_ctl_elem_info_set_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_info_set_name snd_ctl_elem_info_set_name_dylibloader_orig_asound
#define snd_ctl_elem_info_set_index snd_ctl_elem_info_set_index_dylibloader_orig_asound
#define snd_ctl_add_integer_elem_set snd_ctl_add_integer_elem_set_dylibloader_orig_asound
#define snd_ctl_add_integer64_elem_set snd_ctl_add_integer64_elem_set_dylibloader_orig_asound
#define snd_ctl_add_boolean_elem_set snd_ctl_add_boolean_elem_set_dylibloader_orig_asound
#define snd_ctl_add_enumerated_elem_set snd_ctl_add_enumerated_elem_set_dylibloader_orig_asound
#define snd_ctl_add_bytes_elem_set snd_ctl_add_bytes_elem_set_dylibloader_orig_asound
#define snd_ctl_elem_add_integer snd_ctl_elem_add_integer_dylibloader_orig_asound
#define snd_ctl_elem_add_integer64 snd_ctl_elem_add_integer64_dylibloader_orig_asound
#define snd_ctl_elem_add_boolean snd_ctl_elem_add_boolean_dylibloader_orig_asound
#define snd_ctl_elem_add_enumerated snd_ctl_elem_add_enumerated_dylibloader_orig_asound
#define snd_ctl_elem_add_iec958 snd_ctl_elem_add_iec958_dylibloader_orig_asound
#define snd_ctl_elem_remove snd_ctl_elem_remove_dylibloader_orig_asound
#define snd_ctl_elem_value_sizeof snd_ctl_elem_value_sizeof_dylibloader_orig_asound
#define snd_ctl_elem_value_malloc snd_ctl_elem_value_malloc_dylibloader_orig_asound
#define snd_ctl_elem_value_free snd_ctl_elem_value_free_dylibloader_orig_asound
#define snd_ctl_elem_value_clear snd_ctl_elem_value_clear_dylibloader_orig_asound
#define snd_ctl_elem_value_copy snd_ctl_elem_value_copy_dylibloader_orig_asound
#define snd_ctl_elem_value_compare snd_ctl_elem_value_compare_dylibloader_orig_asound
#define snd_ctl_elem_value_get_id snd_ctl_elem_value_get_id_dylibloader_orig_asound
#define snd_ctl_elem_value_get_numid snd_ctl_elem_value_get_numid_dylibloader_orig_asound
#define snd_ctl_elem_value_get_interface snd_ctl_elem_value_get_interface_dylibloader_orig_asound
#define snd_ctl_elem_value_get_device snd_ctl_elem_value_get_device_dylibloader_orig_asound
#define snd_ctl_elem_value_get_subdevice snd_ctl_elem_value_get_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_value_get_name snd_ctl_elem_value_get_name_dylibloader_orig_asound
#define snd_ctl_elem_value_get_index snd_ctl_elem_value_get_index_dylibloader_orig_asound
#define snd_ctl_elem_value_set_id snd_ctl_elem_value_set_id_dylibloader_orig_asound
#define snd_ctl_elem_value_set_numid snd_ctl_elem_value_set_numid_dylibloader_orig_asound
#define snd_ctl_elem_value_set_interface snd_ctl_elem_value_set_interface_dylibloader_orig_asound
#define snd_ctl_elem_value_set_device snd_ctl_elem_value_set_device_dylibloader_orig_asound
#define snd_ctl_elem_value_set_subdevice snd_ctl_elem_value_set_subdevice_dylibloader_orig_asound
#define snd_ctl_elem_value_set_name snd_ctl_elem_value_set_name_dylibloader_orig_asound
#define snd_ctl_elem_value_set_index snd_ctl_elem_value_set_index_dylibloader_orig_asound
#define snd_ctl_elem_value_get_boolean snd_ctl_elem_value_get_boolean_dylibloader_orig_asound
#define snd_ctl_elem_value_get_integer snd_ctl_elem_value_get_integer_dylibloader_orig_asound
#define snd_ctl_elem_value_get_integer64 snd_ctl_elem_value_get_integer64_dylibloader_orig_asound
#define snd_ctl_elem_value_get_enumerated snd_ctl_elem_value_get_enumerated_dylibloader_orig_asound
#define snd_ctl_elem_value_get_byte snd_ctl_elem_value_get_byte_dylibloader_orig_asound
#define snd_ctl_elem_value_set_boolean snd_ctl_elem_value_set_boolean_dylibloader_orig_asound
#define snd_ctl_elem_value_set_integer snd_ctl_elem_value_set_integer_dylibloader_orig_asound
#define snd_ctl_elem_value_set_integer64 snd_ctl_elem_value_set_integer64_dylibloader_orig_asound
#define snd_ctl_elem_value_set_enumerated snd_ctl_elem_value_set_enumerated_dylibloader_orig_asound
#define snd_ctl_elem_value_set_byte snd_ctl_elem_value_set_byte_dylibloader_orig_asound
#define snd_ctl_elem_set_bytes snd_ctl_elem_set_bytes_dylibloader_orig_asound
#define snd_ctl_elem_value_get_bytes snd_ctl_elem_value_get_bytes_dylibloader_orig_asound
#define snd_ctl_elem_value_get_iec958 snd_ctl_elem_value_get_iec958_dylibloader_orig_asound
#define snd_ctl_elem_value_set_iec958 snd_ctl_elem_value_set_iec958_dylibloader_orig_asound
#define snd_tlv_parse_dB_info snd_tlv_parse_dB_info_dylibloader_orig_asound
#define snd_tlv_get_dB_range snd_tlv_get_dB_range_dylibloader_orig_asound
#define snd_tlv_convert_to_dB snd_tlv_convert_to_dB_dylibloader_orig_asound
#define snd_tlv_convert_from_dB snd_tlv_convert_from_dB_dylibloader_orig_asound
#define snd_ctl_get_dB_range snd_ctl_get_dB_range_dylibloader_orig_asound
#define snd_ctl_convert_to_dB snd_ctl_convert_to_dB_dylibloader_orig_asound
#define snd_ctl_convert_from_dB snd_ctl_convert_from_dB_dylibloader_orig_asound
#define snd_hctl_compare_fast snd_hctl_compare_fast_dylibloader_orig_asound
#define snd_hctl_open snd_hctl_open_dylibloader_orig_asound
#define snd_hctl_open_ctl snd_hctl_open_ctl_dylibloader_orig_asound
#define snd_hctl_close snd_hctl_close_dylibloader_orig_asound
#define snd_hctl_nonblock snd_hctl_nonblock_dylibloader_orig_asound
#define snd_hctl_poll_descriptors_count snd_hctl_poll_descriptors_count_dylibloader_orig_asound
#define snd_hctl_poll_descriptors snd_hctl_poll_descriptors_dylibloader_orig_asound
#define snd_hctl_poll_descriptors_revents snd_hctl_poll_descriptors_revents_dylibloader_orig_asound
#define snd_hctl_get_count snd_hctl_get_count_dylibloader_orig_asound
#define snd_hctl_set_compare snd_hctl_set_compare_dylibloader_orig_asound
#define snd_hctl_first_elem snd_hctl_first_elem_dylibloader_orig_asound
#define snd_hctl_last_elem snd_hctl_last_elem_dylibloader_orig_asound
#define snd_hctl_find_elem snd_hctl_find_elem_dylibloader_orig_asound
#define snd_hctl_set_callback snd_hctl_set_callback_dylibloader_orig_asound
#define snd_hctl_set_callback_private snd_hctl_set_callback_private_dylibloader_orig_asound
#define snd_hctl_get_callback_private snd_hctl_get_callback_private_dylibloader_orig_asound
#define snd_hctl_load snd_hctl_load_dylibloader_orig_asound
#define snd_hctl_free snd_hctl_free_dylibloader_orig_asound
#define snd_hctl_handle_events snd_hctl_handle_events_dylibloader_orig_asound
#define snd_hctl_name snd_hctl_name_dylibloader_orig_asound
#define snd_hctl_wait snd_hctl_wait_dylibloader_orig_asound
#define snd_hctl_ctl snd_hctl_ctl_dylibloader_orig_asound
#define snd_hctl_elem_next snd_hctl_elem_next_dylibloader_orig_asound
#define snd_hctl_elem_prev snd_hctl_elem_prev_dylibloader_orig_asound
#define snd_hctl_elem_info snd_hctl_elem_info_dylibloader_orig_asound
#define snd_hctl_elem_read snd_hctl_elem_read_dylibloader_orig_asound
#define snd_hctl_elem_write snd_hctl_elem_write_dylibloader_orig_asound
#define snd_hctl_elem_tlv_read snd_hctl_elem_tlv_read_dylibloader_orig_asound
#define snd_hctl_elem_tlv_write snd_hctl_elem_tlv_write_dylibloader_orig_asound
#define snd_hctl_elem_tlv_command snd_hctl_elem_tlv_command_dylibloader_orig_asound
#define snd_hctl_elem_get_hctl snd_hctl_elem_get_hctl_dylibloader_orig_asound
#define snd_hctl_elem_get_id snd_hctl_elem_get_id_dylibloader_orig_asound
#define snd_hctl_elem_get_numid snd_hctl_elem_get_numid_dylibloader_orig_asound
#define snd_hctl_elem_get_interface snd_hctl_elem_get_interface_dylibloader_orig_asound
#define snd_hctl_elem_get_device snd_hctl_elem_get_device_dylibloader_orig_asound
#define snd_hctl_elem_get_subdevice snd_hctl_elem_get_subdevice_dylibloader_orig_asound
#define snd_hctl_elem_get_name snd_hctl_elem_get_name_dylibloader_orig_asound
#define snd_hctl_elem_get_index snd_hctl_elem_get_index_dylibloader_orig_asound
#define snd_hctl_elem_set_callback snd_hctl_elem_set_callback_dylibloader_orig_asound
#define snd_hctl_elem_get_callback_private snd_hctl_elem_get_callback_private_dylibloader_orig_asound
#define snd_hctl_elem_set_callback_private snd_hctl_elem_set_callback_private_dylibloader_orig_asound
#define snd_sctl_build snd_sctl_build_dylibloader_orig_asound
#define snd_sctl_free snd_sctl_free_dylibloader_orig_asound
#define snd_sctl_install snd_sctl_install_dylibloader_orig_asound
#define snd_sctl_remove snd_sctl_remove_dylibloader_orig_asound
#define snd_mixer_open snd_mixer_open_dylibloader_orig_asound
#define snd_mixer_close snd_mixer_close_dylibloader_orig_asound
#define snd_mixer_first_elem snd_mixer_first_elem_dylibloader_orig_asound
#define snd_mixer_last_elem snd_mixer_last_elem_dylibloader_orig_asound
#define snd_mixer_handle_events snd_mixer_handle_events_dylibloader_orig_asound
#define snd_mixer_attach snd_mixer_attach_dylibloader_orig_asound
#define snd_mixer_attach_hctl snd_mixer_attach_hctl_dylibloader_orig_asound
#define snd_mixer_detach snd_mixer_detach_dylibloader_orig_asound
#define snd_mixer_detach_hctl snd_mixer_detach_hctl_dylibloader_orig_asound
#define snd_mixer_get_hctl snd_mixer_get_hctl_dylibloader_orig_asound
#define snd_mixer_poll_descriptors_count snd_mixer_poll_descriptors_count_dylibloader_orig_asound
#define snd_mixer_poll_descriptors snd_mixer_poll_descriptors_dylibloader_orig_asound
#define snd_mixer_poll_descriptors_revents snd_mixer_poll_descriptors_revents_dylibloader_orig_asound
#define snd_mixer_load snd_mixer_load_dylibloader_orig_asound
#define snd_mixer_free snd_mixer_free_dylibloader_orig_asound
#define snd_mixer_wait snd_mixer_wait_dylibloader_orig_asound
#define snd_mixer_set_compare snd_mixer_set_compare_dylibloader_orig_asound
#define snd_mixer_set_callback snd_mixer_set_callback_dylibloader_orig_asound
#define snd_mixer_get_callback_private snd_mixer_get_callback_private_dylibloader_orig_asound
#define snd_mixer_set_callback_private snd_mixer_set_callback_private_dylibloader_orig_asound
#define snd_mixer_get_count snd_mixer_get_count_dylibloader_orig_asound
#define snd_mixer_class_unregister snd_mixer_class_unregister_dylibloader_orig_asound
#define snd_mixer_elem_next snd_mixer_elem_next_dylibloader_orig_asound
#define snd_mixer_elem_prev snd_mixer_elem_prev_dylibloader_orig_asound
#define snd_mixer_elem_set_callback snd_mixer_elem_set_callback_dylibloader_orig_asound
#define snd_mixer_elem_get_callback_private snd_mixer_elem_get_callback_private_dylibloader_orig_asound
#define snd_mixer_elem_set_callback_private snd_mixer_elem_set_callback_private_dylibloader_orig_asound
#define snd_mixer_elem_get_type snd_mixer_elem_get_type_dylibloader_orig_asound
#define snd_mixer_class_register snd_mixer_class_register_dylibloader_orig_asound
#define snd_mixer_elem_new snd_mixer_elem_new_dylibloader_orig_asound
#define snd_mixer_elem_add snd_mixer_elem_add_dylibloader_orig_asound
#define snd_mixer_elem_remove snd_mixer_elem_remove_dylibloader_orig_asound
#define snd_mixer_elem_free snd_mixer_elem_free_dylibloader_orig_asound
#define snd_mixer_elem_info snd_mixer_elem_info_dylibloader_orig_asound
#define snd_mixer_elem_value snd_mixer_elem_value_dylibloader_orig_asound
#define snd_mixer_elem_attach snd_mixer_elem_attach_dylibloader_orig_asound
#define snd_mixer_elem_detach snd_mixer_elem_detach_dylibloader_orig_asound
#define snd_mixer_elem_empty snd_mixer_elem_empty_dylibloader_orig_asound
#define snd_mixer_elem_get_private snd_mixer_elem_get_private_dylibloader_orig_asound
#define snd_mixer_class_sizeof snd_mixer_class_sizeof_dylibloader_orig_asound
#define snd_mixer_class_malloc snd_mixer_class_malloc_dylibloader_orig_asound
#define snd_mixer_class_free snd_mixer_class_free_dylibloader_orig_asound
#define snd_mixer_class_copy snd_mixer_class_copy_dylibloader_orig_asound
#define snd_mixer_class_get_mixer snd_mixer_class_get_mixer_dylibloader_orig_asound
#define snd_mixer_class_get_event snd_mixer_class_get_event_dylibloader_orig_asound
#define snd_mixer_class_get_private snd_mixer_class_get_private_dylibloader_orig_asound
#define snd_mixer_class_get_compare snd_mixer_class_get_compare_dylibloader_orig_asound
#define snd_mixer_class_set_event snd_mixer_class_set_event_dylibloader_orig_asound
#define snd_mixer_class_set_private snd_mixer_class_set_private_dylibloader_orig_asound
#define snd_mixer_class_set_private_free snd_mixer_class_set_private_free_dylibloader_orig_asound
#define snd_mixer_class_set_compare snd_mixer_class_set_compare_dylibloader_orig_asound
#define snd_mixer_selem_channel_name snd_mixer_selem_channel_name_dylibloader_orig_asound
#define snd_mixer_selem_register snd_mixer_selem_register_dylibloader_orig_asound
#define snd_mixer_selem_get_id snd_mixer_selem_get_id_dylibloader_orig_asound
#define snd_mixer_selem_get_name snd_mixer_selem_get_name_dylibloader_orig_asound
#define snd_mixer_selem_get_index snd_mixer_selem_get_index_dylibloader_orig_asound
#define snd_mixer_find_selem snd_mixer_find_selem_dylibloader_orig_asound
#define snd_mixer_selem_is_active snd_mixer_selem_is_active_dylibloader_orig_asound
#define snd_mixer_selem_is_playback_mono snd_mixer_selem_is_playback_mono_dylibloader_orig_asound
#define snd_mixer_selem_has_playback_channel snd_mixer_selem_has_playback_channel_dylibloader_orig_asound
#define snd_mixer_selem_is_capture_mono snd_mixer_selem_is_capture_mono_dylibloader_orig_asound
#define snd_mixer_selem_has_capture_channel snd_mixer_selem_has_capture_channel_dylibloader_orig_asound
#define snd_mixer_selem_get_capture_group snd_mixer_selem_get_capture_group_dylibloader_orig_asound
#define snd_mixer_selem_has_common_volume snd_mixer_selem_has_common_volume_dylibloader_orig_asound
#define snd_mixer_selem_has_playback_volume snd_mixer_selem_has_playback_volume_dylibloader_orig_asound
#define snd_mixer_selem_has_playback_volume_joined snd_mixer_selem_has_playback_volume_joined_dylibloader_orig_asound
#define snd_mixer_selem_has_capture_volume snd_mixer_selem_has_capture_volume_dylibloader_orig_asound
#define snd_mixer_selem_has_capture_volume_joined snd_mixer_selem_has_capture_volume_joined_dylibloader_orig_asound
#define snd_mixer_selem_has_common_switch snd_mixer_selem_has_common_switch_dylibloader_orig_asound
#define snd_mixer_selem_has_playback_switch snd_mixer_selem_has_playback_switch_dylibloader_orig_asound
#define snd_mixer_selem_has_playback_switch_joined snd_mixer_selem_has_playback_switch_joined_dylibloader_orig_asound
#define snd_mixer_selem_has_capture_switch snd_mixer_selem_has_capture_switch_dylibloader_orig_asound
#define snd_mixer_selem_has_capture_switch_joined snd_mixer_selem_has_capture_switch_joined_dylibloader_orig_asound
#define snd_mixer_selem_has_capture_switch_exclusive snd_mixer_selem_has_capture_switch_exclusive_dylibloader_orig_asound
#define snd_mixer_selem_ask_playback_vol_dB snd_mixer_selem_ask_playback_vol_dB_dylibloader_orig_asound
#define snd_mixer_selem_ask_capture_vol_dB snd_mixer_selem_ask_capture_vol_dB_dylibloader_orig_asound
#define snd_mixer_selem_ask_playback_dB_vol snd_mixer_selem_ask_playback_dB_vol_dylibloader_orig_asound
#define snd_mixer_selem_ask_capture_dB_vol snd_mixer_selem_ask_capture_dB_vol_dylibloader_orig_asound
#define snd_mixer_selem_get_playback_volume snd_mixer_selem_get_playback_volume_dylibloader_orig_asound
#define snd_mixer_selem_get_capture_volume snd_mixer_selem_get_capture_volume_dylibloader_orig_asound
#define snd_mixer_selem_get_playback_dB snd_mixer_selem_get_playback_dB_dylibloader_orig_asound
#define snd_mixer_selem_get_capture_dB snd_mixer_selem_get_capture_dB_dylibloader_orig_asound
#define snd_mixer_selem_get_playback_switch snd_mixer_selem_get_playback_switch_dylibloader_orig_asound
#define snd_mixer_selem_get_capture_switch snd_mixer_selem_get_capture_switch_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_volume snd_mixer_selem_set_playback_volume_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_volume snd_mixer_selem_set_capture_volume_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_dB snd_mixer_selem_set_playback_dB_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_dB snd_mixer_selem_set_capture_dB_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_volume_all snd_mixer_selem_set_playback_volume_all_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_volume_all snd_mixer_selem_set_capture_volume_all_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_dB_all snd_mixer_selem_set_playback_dB_all_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_dB_all snd_mixer_selem_set_capture_dB_all_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_switch snd_mixer_selem_set_playback_switch_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_switch snd_mixer_selem_set_capture_switch_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_switch_all snd_mixer_selem_set_playback_switch_all_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_switch_all snd_mixer_selem_set_capture_switch_all_dylibloader_orig_asound
#define snd_mixer_selem_get_playback_volume_range snd_mixer_selem_get_playback_volume_range_dylibloader_orig_asound
#define snd_mixer_selem_get_playback_dB_range snd_mixer_selem_get_playback_dB_range_dylibloader_orig_asound
#define snd_mixer_selem_set_playback_volume_range snd_mixer_selem_set_playback_volume_range_dylibloader_orig_asound
#define snd_mixer_selem_get_capture_volume_range snd_mixer_selem_get_capture_volume_range_dylibloader_orig_asound
#define snd_mixer_selem_get_capture_dB_range snd_mixer_selem_get_capture_dB_range_dylibloader_orig_asound
#define snd_mixer_selem_set_capture_volume_range snd_mixer_selem_set_capture_volume_range_dylibloader_orig_asound
#define snd_mixer_selem_is_enumerated snd_mixer_selem_is_enumerated_dylibloader_orig_asound
#define snd_mixer_selem_is_enum_playback snd_mixer_selem_is_enum_playback_dylibloader_orig_asound
#define snd_mixer_selem_is_enum_capture snd_mixer_selem_is_enum_capture_dylibloader_orig_asound
#define snd_mixer_selem_get_enum_items snd_mixer_selem_get_enum_items_dylibloader_orig_asound
#define snd_mixer_selem_get_enum_item_name snd_mixer_selem_get_enum_item_name_dylibloader_orig_asound
#define snd_mixer_selem_get_enum_item snd_mixer_selem_get_enum_item_dylibloader_orig_asound
#define snd_mixer_selem_set_enum_item snd_mixer_selem_set_enum_item_dylibloader_orig_asound
#define snd_mixer_selem_id_sizeof snd_mixer_selem_id_sizeof_dylibloader_orig_asound
#define snd_mixer_selem_id_malloc snd_mixer_selem_id_malloc_dylibloader_orig_asound
#define snd_mixer_selem_id_free snd_mixer_selem_id_free_dylibloader_orig_asound
#define snd_mixer_selem_id_copy snd_mixer_selem_id_copy_dylibloader_orig_asound
#define snd_mixer_selem_id_get_name snd_mixer_selem_id_get_name_dylibloader_orig_asound
#define snd_mixer_selem_id_get_index snd_mixer_selem_id_get_index_dylibloader_orig_asound
#define snd_mixer_selem_id_set_name snd_mixer_selem_id_set_name_dylibloader_orig_asound
#define snd_mixer_selem_id_set_index snd_mixer_selem_id_set_index_dylibloader_orig_asound
#define snd_seq_open snd_seq_open_dylibloader_orig_asound
#define snd_seq_open_lconf snd_seq_open_lconf_dylibloader_orig_asound
#define snd_seq_name snd_seq_name_dylibloader_orig_asound
#define snd_seq_type snd_seq_type_dylibloader_orig_asound
#define snd_seq_close snd_seq_close_dylibloader_orig_asound
#define snd_seq_poll_descriptors_count snd_seq_poll_descriptors_count_dylibloader_orig_asound
#define snd_seq_poll_descriptors snd_seq_poll_descriptors_dylibloader_orig_asound
#define snd_seq_poll_descriptors_revents snd_seq_poll_descriptors_revents_dylibloader_orig_asound
#define snd_seq_nonblock snd_seq_nonblock_dylibloader_orig_asound
#define snd_seq_client_id snd_seq_client_id_dylibloader_orig_asound
#define snd_seq_get_output_buffer_size snd_seq_get_output_buffer_size_dylibloader_orig_asound
#define snd_seq_get_input_buffer_size snd_seq_get_input_buffer_size_dylibloader_orig_asound
#define snd_seq_set_output_buffer_size snd_seq_set_output_buffer_size_dylibloader_orig_asound
#define snd_seq_set_input_buffer_size snd_seq_set_input_buffer_size_dylibloader_orig_asound
#define snd_seq_system_info_sizeof snd_seq_system_info_sizeof_dylibloader_orig_asound
#define snd_seq_system_info_malloc snd_seq_system_info_malloc_dylibloader_orig_asound
#define snd_seq_system_info_free snd_seq_system_info_free_dylibloader_orig_asound
#define snd_seq_system_info_copy snd_seq_system_info_copy_dylibloader_orig_asound
#define snd_seq_system_info_get_queues snd_seq_system_info_get_queues_dylibloader_orig_asound
#define snd_seq_system_info_get_clients snd_seq_system_info_get_clients_dylibloader_orig_asound
#define snd_seq_system_info_get_ports snd_seq_system_info_get_ports_dylibloader_orig_asound
#define snd_seq_system_info_get_channels snd_seq_system_info_get_channels_dylibloader_orig_asound
#define snd_seq_system_info_get_cur_clients snd_seq_system_info_get_cur_clients_dylibloader_orig_asound
#define snd_seq_system_info_get_cur_queues snd_seq_system_info_get_cur_queues_dylibloader_orig_asound
#define snd_seq_system_info snd_seq_system_info_dylibloader_orig_asound
#define snd_seq_client_info_sizeof snd_seq_client_info_sizeof_dylibloader_orig_asound
#define snd_seq_client_info_malloc snd_seq_client_info_malloc_dylibloader_orig_asound
#define snd_seq_client_info_free snd_seq_client_info_free_dylibloader_orig_asound
#define snd_seq_client_info_copy snd_seq_client_info_copy_dylibloader_orig_asound
#define snd_seq_client_info_get_client snd_seq_client_info_get_client_dylibloader_orig_asound
#define snd_seq_client_info_get_type snd_seq_client_info_get_type_dylibloader_orig_asound
#define snd_seq_client_info_get_name snd_seq_client_info_get_name_dylibloader_orig_asound
#define snd_seq_client_info_get_broadcast_filter snd_seq_client_info_get_broadcast_filter_dylibloader_orig_asound
#define snd_seq_client_info_get_error_bounce snd_seq_client_info_get_error_bounce_dylibloader_orig_asound
#define snd_seq_client_info_get_card snd_seq_client_info_get_card_dylibloader_orig_asound
#define snd_seq_client_info_get_pid snd_seq_client_info_get_pid_dylibloader_orig_asound
#define snd_seq_client_info_get_event_filter snd_seq_client_info_get_event_filter_dylibloader_orig_asound
#define snd_seq_client_info_get_num_ports snd_seq_client_info_get_num_ports_dylibloader_orig_asound
#define snd_seq_client_info_get_event_lost snd_seq_client_info_get_event_lost_dylibloader_orig_asound
#define snd_seq_client_info_set_client snd_seq_client_info_set_client_dylibloader_orig_asound
#define snd_seq_client_info_set_name snd_seq_client_info_set_name_dylibloader_orig_asound
#define snd_seq_client_info_set_broadcast_filter snd_seq_client_info_set_broadcast_filter_dylibloader_orig_asound
#define snd_seq_client_info_set_error_bounce snd_seq_client_info_set_error_bounce_dylibloader_orig_asound
#define snd_seq_client_info_set_event_filter snd_seq_client_info_set_event_filter_dylibloader_orig_asound
#define snd_seq_client_info_event_filter_clear snd_seq_client_info_event_filter_clear_dylibloader_orig_asound
#define snd_seq_client_info_event_filter_add snd_seq_client_info_event_filter_add_dylibloader_orig_asound
#define snd_seq_client_info_event_filter_del snd_seq_client_info_event_filter_del_dylibloader_orig_asound
#define snd_seq_client_info_event_filter_check snd_seq_client_info_event_filter_check_dylibloader_orig_asound
#define snd_seq_get_client_info snd_seq_get_client_info_dylibloader_orig_asound
#define snd_seq_get_any_client_info snd_seq_get_any_client_info_dylibloader_orig_asound
#define snd_seq_set_client_info snd_seq_set_client_info_dylibloader_orig_asound
#define snd_seq_query_next_client snd_seq_query_next_client_dylibloader_orig_asound
#define snd_seq_client_pool_sizeof snd_seq_client_pool_sizeof_dylibloader_orig_asound
#define snd_seq_client_pool_malloc snd_seq_client_pool_malloc_dylibloader_orig_asound
#define snd_seq_client_pool_free snd_seq_client_pool_free_dylibloader_orig_asound
#define snd_seq_client_pool_copy snd_seq_client_pool_copy_dylibloader_orig_asound
#define snd_seq_client_pool_get_client snd_seq_client_pool_get_client_dylibloader_orig_asound
#define snd_seq_client_pool_get_output_pool snd_seq_client_pool_get_output_pool_dylibloader_orig_asound
#define snd_seq_client_pool_get_input_pool snd_seq_client_pool_get_input_pool_dylibloader_orig_asound
#define snd_seq_client_pool_get_output_room snd_seq_client_pool_get_output_room_dylibloader_orig_asound
#define snd_seq_client_pool_get_output_free snd_seq_client_pool_get_output_free_dylibloader_orig_asound
#define snd_seq_client_pool_get_input_free snd_seq_client_pool_get_input_free_dylibloader_orig_asound
#define snd_seq_client_pool_set_output_pool snd_seq_client_pool_set_output_pool_dylibloader_orig_asound
#define snd_seq_client_pool_set_input_pool snd_seq_client_pool_set_input_pool_dylibloader_orig_asound
#define snd_seq_client_pool_set_output_room snd_seq_client_pool_set_output_room_dylibloader_orig_asound
#define snd_seq_get_client_pool snd_seq_get_client_pool_dylibloader_orig_asound
#define snd_seq_set_client_pool snd_seq_set_client_pool_dylibloader_orig_asound
#define snd_seq_port_info_sizeof snd_seq_port_info_sizeof_dylibloader_orig_asound
#define snd_seq_port_info_malloc snd_seq_port_info_malloc_dylibloader_orig_asound
#define snd_seq_port_info_free snd_seq_port_info_free_dylibloader_orig_asound
#define snd_seq_port_info_copy snd_seq_port_info_copy_dylibloader_orig_asound
#define snd_seq_port_info_get_client snd_seq_port_info_get_client_dylibloader_orig_asound
#define snd_seq_port_info_get_port snd_seq_port_info_get_port_dylibloader_orig_asound
#define snd_seq_port_info_get_addr snd_seq_port_info_get_addr_dylibloader_orig_asound
#define snd_seq_port_info_get_name snd_seq_port_info_get_name_dylibloader_orig_asound
#define snd_seq_port_info_get_capability snd_seq_port_info_get_capability_dylibloader_orig_asound
#define snd_seq_port_info_get_type snd_seq_port_info_get_type_dylibloader_orig_asound
#define snd_seq_port_info_get_midi_channels snd_seq_port_info_get_midi_channels_dylibloader_orig_asound
#define snd_seq_port_info_get_midi_voices snd_seq_port_info_get_midi_voices_dylibloader_orig_asound
#define snd_seq_port_info_get_synth_voices snd_seq_port_info_get_synth_voices_dylibloader_orig_asound
#define snd_seq_port_info_get_read_use snd_seq_port_info_get_read_use_dylibloader_orig_asound
#define snd_seq_port_info_get_write_use snd_seq_port_info_get_write_use_dylibloader_orig_asound
#define snd_seq_port_info_get_port_specified snd_seq_port_info_get_port_specified_dylibloader_orig_asound
#define snd_seq_port_info_get_timestamping snd_seq_port_info_get_timestamping_dylibloader_orig_asound
#define snd_seq_port_info_get_timestamp_real snd_seq_port_info_get_timestamp_real_dylibloader_orig_asound
#define snd_seq_port_info_get_timestamp_queue snd_seq_port_info_get_timestamp_queue_dylibloader_orig_asound
#define snd_seq_port_info_set_client snd_seq_port_info_set_client_dylibloader_orig_asound
#define snd_seq_port_info_set_port snd_seq_port_info_set_port_dylibloader_orig_asound
#define snd_seq_port_info_set_addr snd_seq_port_info_set_addr_dylibloader_orig_asound
#define snd_seq_port_info_set_name snd_seq_port_info_set_name_dylibloader_orig_asound
#define snd_seq_port_info_set_capability snd_seq_port_info_set_capability_dylibloader_orig_asound
#define snd_seq_port_info_set_type snd_seq_port_info_set_type_dylibloader_orig_asound
#define snd_seq_port_info_set_midi_channels snd_seq_port_info_set_midi_channels_dylibloader_orig_asound
#define snd_seq_port_info_set_midi_voices snd_seq_port_info_set_midi_voices_dylibloader_orig_asound
#define snd_seq_port_info_set_synth_voices snd_seq_port_info_set_synth_voices_dylibloader_orig_asound
#define snd_seq_port_info_set_port_specified snd_seq_port_info_set_port_specified_dylibloader_orig_asound
#define snd_seq_port_info_set_timestamping snd_seq_port_info_set_timestamping_dylibloader_orig_asound
#define snd_seq_port_info_set_timestamp_real snd_seq_port_info_set_timestamp_real_dylibloader_orig_asound
#define snd_seq_port_info_set_timestamp_queue snd_seq_port_info_set_timestamp_queue_dylibloader_orig_asound
#define snd_seq_create_port snd_seq_create_port_dylibloader_orig_asound
#define snd_seq_delete_port snd_seq_delete_port_dylibloader_orig_asound
#define snd_seq_get_port_info snd_seq_get_port_info_dylibloader_orig_asound
#define snd_seq_get_any_port_info snd_seq_get_any_port_info_dylibloader_orig_asound
#define snd_seq_set_port_info snd_seq_set_port_info_dylibloader_orig_asound
#define snd_seq_query_next_port snd_seq_query_next_port_dylibloader_orig_asound
#define snd_seq_port_subscribe_sizeof snd_seq_port_subscribe_sizeof_dylibloader_orig_asound
#define snd_seq_port_subscribe_malloc snd_seq_port_subscribe_malloc_dylibloader_orig_asound
#define snd_seq_port_subscribe_free snd_seq_port_subscribe_free_dylibloader_orig_asound
#define snd_seq_port_subscribe_copy snd_seq_port_subscribe_copy_dylibloader_orig_asound
#define snd_seq_port_subscribe_get_sender snd_seq_port_subscribe_get_sender_dylibloader_orig_asound
#define snd_seq_port_subscribe_get_dest snd_seq_port_subscribe_get_dest_dylibloader_orig_asound
#define snd_seq_port_subscribe_get_queue snd_seq_port_subscribe_get_queue_dylibloader_orig_asound
#define snd_seq_port_subscribe_get_exclusive snd_seq_port_subscribe_get_exclusive_dylibloader_orig_asound
#define snd_seq_port_subscribe_get_time_update snd_seq_port_subscribe_get_time_update_dylibloader_orig_asound
#define snd_seq_port_subscribe_get_time_real snd_seq_port_subscribe_get_time_real_dylibloader_orig_asound
#define snd_seq_port_subscribe_set_sender snd_seq_port_subscribe_set_sender_dylibloader_orig_asound
#define snd_seq_port_subscribe_set_dest snd_seq_port_subscribe_set_dest_dylibloader_orig_asound
#define snd_seq_port_subscribe_set_queue snd_seq_port_subscribe_set_queue_dylibloader_orig_asound
#define snd_seq_port_subscribe_set_exclusive snd_seq_port_subscribe_set_exclusive_dylibloader_orig_asound
#define snd_seq_port_subscribe_set_time_update snd_seq_port_subscribe_set_time_update_dylibloader_orig_asound
#define snd_seq_port_subscribe_set_time_real snd_seq_port_subscribe_set_time_real_dylibloader_orig_asound
#define snd_seq_get_port_subscription snd_seq_get_port_subscription_dylibloader_orig_asound
#define snd_seq_subscribe_port snd_seq_subscribe_port_dylibloader_orig_asound
#define snd_seq_unsubscribe_port snd_seq_unsubscribe_port_dylibloader_orig_asound
#define snd_seq_query_subscribe_sizeof snd_seq_query_subscribe_sizeof_dylibloader_orig_asound
#define snd_seq_query_subscribe_malloc snd_seq_query_subscribe_malloc_dylibloader_orig_asound
#define snd_seq_query_subscribe_free snd_seq_query_subscribe_free_dylibloader_orig_asound
#define snd_seq_query_subscribe_copy snd_seq_query_subscribe_copy_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_client snd_seq_query_subscribe_get_client_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_port snd_seq_query_subscribe_get_port_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_root snd_seq_query_subscribe_get_root_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_type snd_seq_query_subscribe_get_type_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_index snd_seq_query_subscribe_get_index_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_num_subs snd_seq_query_subscribe_get_num_subs_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_addr snd_seq_query_subscribe_get_addr_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_queue snd_seq_query_subscribe_get_queue_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_exclusive snd_seq_query_subscribe_get_exclusive_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_time_update snd_seq_query_subscribe_get_time_update_dylibloader_orig_asound
#define snd_seq_query_subscribe_get_time_real snd_seq_query_subscribe_get_time_real_dylibloader_orig_asound
#define snd_seq_query_subscribe_set_client snd_seq_query_subscribe_set_client_dylibloader_orig_asound
#define snd_seq_query_subscribe_set_port snd_seq_query_subscribe_set_port_dylibloader_orig_asound
#define snd_seq_query_subscribe_set_root snd_seq_query_subscribe_set_root_dylibloader_orig_asound
#define snd_seq_query_subscribe_set_type snd_seq_query_subscribe_set_type_dylibloader_orig_asound
#define snd_seq_query_subscribe_set_index snd_seq_query_subscribe_set_index_dylibloader_orig_asound
#define snd_seq_query_port_subscribers snd_seq_query_port_subscribers_dylibloader_orig_asound
#define snd_seq_queue_info_sizeof snd_seq_queue_info_sizeof_dylibloader_orig_asound
#define snd_seq_queue_info_malloc snd_seq_queue_info_malloc_dylibloader_orig_asound
#define snd_seq_queue_info_free snd_seq_queue_info_free_dylibloader_orig_asound
#define snd_seq_queue_info_copy snd_seq_queue_info_copy_dylibloader_orig_asound
#define snd_seq_queue_info_get_queue snd_seq_queue_info_get_queue_dylibloader_orig_asound
#define snd_seq_queue_info_get_name snd_seq_queue_info_get_name_dylibloader_orig_asound
#define snd_seq_queue_info_get_owner snd_seq_queue_info_get_owner_dylibloader_orig_asound
#define snd_seq_queue_info_get_locked snd_seq_queue_info_get_locked_dylibloader_orig_asound
#define snd_seq_queue_info_get_flags snd_seq_queue_info_get_flags_dylibloader_orig_asound
#define snd_seq_queue_info_set_name snd_seq_queue_info_set_name_dylibloader_orig_asound
#define snd_seq_queue_info_set_owner snd_seq_queue_info_set_owner_dylibloader_orig_asound
#define snd_seq_queue_info_set_locked snd_seq_queue_info_set_locked_dylibloader_orig_asound
#define snd_seq_queue_info_set_flags snd_seq_queue_info_set_flags_dylibloader_orig_asound
#define snd_seq_create_queue snd_seq_create_queue_dylibloader_orig_asound
#define snd_seq_alloc_named_queue snd_seq_alloc_named_queue_dylibloader_orig_asound
#define snd_seq_alloc_queue snd_seq_alloc_queue_dylibloader_orig_asound
#define snd_seq_free_queue snd_seq_free_queue_dylibloader_orig_asound
#define snd_seq_get_queue_info snd_seq_get_queue_info_dylibloader_orig_asound
#define snd_seq_set_queue_info snd_seq_set_queue_info_dylibloader_orig_asound
#define snd_seq_query_named_queue snd_seq_query_named_queue_dylibloader_orig_asound
#define snd_seq_get_queue_usage snd_seq_get_queue_usage_dylibloader_orig_asound
#define snd_seq_set_queue_usage snd_seq_set_queue_usage_dylibloader_orig_asound
#define snd_seq_queue_status_sizeof snd_seq_queue_status_sizeof_dylibloader_orig_asound
#define snd_seq_queue_status_malloc snd_seq_queue_status_malloc_dylibloader_orig_asound
#define snd_seq_queue_status_free snd_seq_queue_status_free_dylibloader_orig_asound
#define snd_seq_queue_status_copy snd_seq_queue_status_copy_dylibloader_orig_asound
#define snd_seq_queue_status_get_queue snd_seq_queue_status_get_queue_dylibloader_orig_asound
#define snd_seq_queue_status_get_events snd_seq_queue_status_get_events_dylibloader_orig_asound
#define snd_seq_queue_status_get_tick_time snd_seq_queue_status_get_tick_time_dylibloader_orig_asound
#define snd_seq_queue_status_get_real_time snd_seq_queue_status_get_real_time_dylibloader_orig_asound
#define snd_seq_queue_status_get_status snd_seq_queue_status_get_status_dylibloader_orig_asound
#define snd_seq_get_queue_status snd_seq_get_queue_status_dylibloader_orig_asound
#define snd_seq_queue_tempo_sizeof snd_seq_queue_tempo_sizeof_dylibloader_orig_asound
#define snd_seq_queue_tempo_malloc snd_seq_queue_tempo_malloc_dylibloader_orig_asound
#define snd_seq_queue_tempo_free snd_seq_queue_tempo_free_dylibloader_orig_asound
#define snd_seq_queue_tempo_copy snd_seq_queue_tempo_copy_dylibloader_orig_asound
#define snd_seq_queue_tempo_get_queue snd_seq_queue_tempo_get_queue_dylibloader_orig_asound
#define snd_seq_queue_tempo_get_tempo snd_seq_queue_tempo_get_tempo_dylibloader_orig_asound
#define snd_seq_queue_tempo_get_ppq snd_seq_queue_tempo_get_ppq_dylibloader_orig_asound
#define snd_seq_queue_tempo_get_skew snd_seq_queue_tempo_get_skew_dylibloader_orig_asound
#define snd_seq_queue_tempo_get_skew_base snd_seq_queue_tempo_get_skew_base_dylibloader_orig_asound
#define snd_seq_queue_tempo_set_tempo snd_seq_queue_tempo_set_tempo_dylibloader_orig_asound
#define snd_seq_queue_tempo_set_ppq snd_seq_queue_tempo_set_ppq_dylibloader_orig_asound
#define snd_seq_queue_tempo_set_skew snd_seq_queue_tempo_set_skew_dylibloader_orig_asound
#define snd_seq_queue_tempo_set_skew_base snd_seq_queue_tempo_set_skew_base_dylibloader_orig_asound
#define snd_seq_get_queue_tempo snd_seq_get_queue_tempo_dylibloader_orig_asound
#define snd_seq_set_queue_tempo snd_seq_set_queue_tempo_dylibloader_orig_asound
#define snd_seq_queue_timer_sizeof snd_seq_queue_timer_sizeof_dylibloader_orig_asound
#define snd_seq_queue_timer_malloc snd_seq_queue_timer_malloc_dylibloader_orig_asound
#define snd_seq_queue_timer_free snd_seq_queue_timer_free_dylibloader_orig_asound
#define snd_seq_queue_timer_copy snd_seq_queue_timer_copy_dylibloader_orig_asound
#define snd_seq_queue_timer_get_queue snd_seq_queue_timer_get_queue_dylibloader_orig_asound
#define snd_seq_queue_timer_get_type snd_seq_queue_timer_get_type_dylibloader_orig_asound
#define snd_seq_queue_timer_get_id snd_seq_queue_timer_get_id_dylibloader_orig_asound
#define snd_seq_queue_timer_get_resolution snd_seq_queue_timer_get_resolution_dylibloader_orig_asound
#define snd_seq_queue_timer_set_type snd_seq_queue_timer_set_type_dylibloader_orig_asound
#define snd_seq_queue_timer_set_id snd_seq_queue_timer_set_id_dylibloader_orig_asound
#define snd_seq_queue_timer_set_resolution snd_seq_queue_timer_set_resolution_dylibloader_orig_asound
#define snd_seq_get_queue_timer snd_seq_get_queue_timer_dylibloader_orig_asound
#define snd_seq_set_queue_timer snd_seq_set_queue_timer_dylibloader_orig_asound
#define snd_seq_free_event snd_seq_free_event_dylibloader_orig_asound
#define snd_seq_event_length snd_seq_event_length_dylibloader_orig_asound
#define snd_seq_event_output snd_seq_event_output_dylibloader_orig_asound
#define snd_seq_event_output_buffer snd_seq_event_output_buffer_dylibloader_orig_asound
#define snd_seq_event_output_direct snd_seq_event_output_direct_dylibloader_orig_asound
#define snd_seq_event_input snd_seq_event_input_dylibloader_orig_asound
#define snd_seq_event_input_pending snd_seq_event_input_pending_dylibloader_orig_asound
#define snd_seq_drain_output snd_seq_drain_output_dylibloader_orig_asound
#define snd_seq_event_output_pending snd_seq_event_output_pending_dylibloader_orig_asound
#define snd_seq_extract_output snd_seq_extract_output_dylibloader_orig_asound
#define snd_seq_drop_output snd_seq_drop_output_dylibloader_orig_asound
#define snd_seq_drop_output_buffer snd_seq_drop_output_buffer_dylibloader_orig_asound
#define snd_seq_drop_input snd_seq_drop_input_dylibloader_orig_asound
#define snd_seq_drop_input_buffer snd_seq_drop_input_buffer_dylibloader_orig_asound
#define snd_seq_remove_events_sizeof snd_seq_remove_events_sizeof_dylibloader_orig_asound
#define snd_seq_remove_events_malloc snd_seq_remove_events_malloc_dylibloader_orig_asound
#define snd_seq_remove_events_free snd_seq_remove_events_free_dylibloader_orig_asound
#define snd_seq_remove_events_copy snd_seq_remove_events_copy_dylibloader_orig_asound
#define snd_seq_remove_events_get_condition snd_seq_remove_events_get_condition_dylibloader_orig_asound
#define snd_seq_remove_events_get_queue snd_seq_remove_events_get_queue_dylibloader_orig_asound
#define snd_seq_remove_events_get_time snd_seq_remove_events_get_time_dylibloader_orig_asound
#define snd_seq_remove_events_get_dest snd_seq_remove_events_get_dest_dylibloader_orig_asound
#define snd_seq_remove_events_get_channel snd_seq_remove_events_get_channel_dylibloader_orig_asound
#define snd_seq_remove_events_get_event_type snd_seq_remove_events_get_event_type_dylibloader_orig_asound
#define snd_seq_remove_events_get_tag snd_seq_remove_events_get_tag_dylibloader_orig_asound
#define snd_seq_remove_events_set_condition snd_seq_remove_events_set_condition_dylibloader_orig_asound
#define snd_seq_remove_events_set_queue snd_seq_remove_events_set_queue_dylibloader_orig_asound
#define snd_seq_remove_events_set_time snd_seq_remove_events_set_time_dylibloader_orig_asound
#define snd_seq_remove_events_set_dest snd_seq_remove_events_set_dest_dylibloader_orig_asound
#define snd_seq_remove_events_set_channel snd_seq_remove_events_set_channel_dylibloader_orig_asound
#define snd_seq_remove_events_set_event_type snd_seq_remove_events_set_event_type_dylibloader_orig_asound
#define snd_seq_remove_events_set_tag snd_seq_remove_events_set_tag_dylibloader_orig_asound
#define snd_seq_remove_events snd_seq_remove_events_dylibloader_orig_asound
#define snd_seq_set_bit snd_seq_set_bit_dylibloader_orig_asound
#define snd_seq_unset_bit snd_seq_unset_bit_dylibloader_orig_asound
#define snd_seq_change_bit snd_seq_change_bit_dylibloader_orig_asound
#define snd_seq_get_bit snd_seq_get_bit_dylibloader_orig_asound
#define snd_seq_control_queue snd_seq_control_queue_dylibloader_orig_asound
#define snd_seq_create_simple_port snd_seq_create_simple_port_dylibloader_orig_asound
#define snd_seq_delete_simple_port snd_seq_delete_simple_port_dylibloader_orig_asound
#define snd_seq_connect_from snd_seq_connect_from_dylibloader_orig_asound
#define snd_seq_connect_to snd_seq_connect_to_dylibloader_orig_asound
#define snd_seq_disconnect_from snd_seq_disconnect_from_dylibloader_orig_asound
#define snd_seq_disconnect_to snd_seq_disconnect_to_dylibloader_orig_asound
#define snd_seq_set_client_name snd_seq_set_client_name_dylibloader_orig_asound
#define snd_seq_set_client_event_filter snd_seq_set_client_event_filter_dylibloader_orig_asound
#define snd_seq_set_client_pool_output snd_seq_set_client_pool_output_dylibloader_orig_asound
#define snd_seq_set_client_pool_output_room snd_seq_set_client_pool_output_room_dylibloader_orig_asound
#define snd_seq_set_client_pool_input snd_seq_set_client_pool_input_dylibloader_orig_asound
#define snd_seq_sync_output_queue snd_seq_sync_output_queue_dylibloader_orig_asound
#define snd_seq_parse_address snd_seq_parse_address_dylibloader_orig_asound
#define snd_seq_reset_pool_output snd_seq_reset_pool_output_dylibloader_orig_asound
#define snd_seq_reset_pool_input snd_seq_reset_pool_input_dylibloader_orig_asound
#define snd_midi_event_new snd_midi_event_new_dylibloader_orig_asound
#define snd_midi_event_resize_buffer snd_midi_event_resize_buffer_dylibloader_orig_asound
#define snd_midi_event_free snd_midi_event_free_dylibloader_orig_asound
#define snd_midi_event_init snd_midi_event_init_dylibloader_orig_asound
#define snd_midi_event_reset_encode snd_midi_event_reset_encode_dylibloader_orig_asound
#define snd_midi_event_reset_decode snd_midi_event_reset_decode_dylibloader_orig_asound
#define snd_midi_event_no_status snd_midi_event_no_status_dylibloader_orig_asound
#define snd_midi_event_encode snd_midi_event_encode_dylibloader_orig_asound
#define snd_midi_event_encode_byte snd_midi_event_encode_byte_dylibloader_orig_asound
#define snd_midi_event_decode snd_midi_event_decode_dylibloader_orig_asound
#include <thirdparty/linuxbsd_headers/alsa/asoundlib.h> // IWYU pragma: export.
#undef snd_asoundlib_version
#undef snd_dlopen
#undef snd_dlsym
#undef snd_dlclose
#undef snd_async_add_handler
#undef snd_async_del_handler
#undef snd_async_handler_get_fd
#undef snd_async_handler_get_signo
#undef snd_async_handler_get_callback_private
#undef snd_shm_area_create
#undef snd_shm_area_share
#undef snd_shm_area_destroy
#undef snd_user_file
#undef snd_input_stdio_open
#undef snd_input_stdio_attach
#undef snd_input_buffer_open
#undef snd_input_close
#undef snd_input_scanf
#undef snd_input_gets
#undef snd_input_getc
#undef snd_input_ungetc
#undef snd_output_stdio_open
#undef snd_output_stdio_attach
#undef snd_output_buffer_open
#undef snd_output_buffer_string
#undef snd_output_close
#undef snd_output_printf
#undef snd_output_vprintf
#undef snd_output_puts
#undef snd_output_putc
#undef snd_output_flush
#undef snd_strerror
#undef snd_lib_error_set_handler
#undef snd_lib_error_set_local
#undef snd_config_top
#undef snd_config_load
#undef snd_config_load_override
#undef snd_config_save
#undef snd_config_update
#undef snd_config_update_r
#undef snd_config_update_free
#undef snd_config_update_free_global
#undef snd_config_update_ref
#undef snd_config_ref
#undef snd_config_unref
#undef snd_config_search
#undef snd_config_searchv
#undef snd_config_search_definition
#undef snd_config_expand
#undef snd_config_evaluate
#undef snd_config_add
#undef snd_config_delete
#undef snd_config_delete_compound_members
#undef snd_config_copy
#undef snd_config_make
#undef snd_config_make_integer
#undef snd_config_make_integer64
#undef snd_config_make_real
#undef snd_config_make_string
#undef snd_config_make_pointer
#undef snd_config_make_compound
#undef snd_config_imake_integer
#undef snd_config_imake_integer64
#undef snd_config_imake_real
#undef snd_config_imake_string
#undef snd_config_imake_safe_string
#undef snd_config_imake_pointer
#undef snd_config_get_type
#undef snd_config_set_id
#undef snd_config_set_integer
#undef snd_config_set_integer64
#undef snd_config_set_real
#undef snd_config_set_string
#undef snd_config_set_ascii
#undef snd_config_set_pointer
#undef snd_config_get_id
#undef snd_config_get_integer
#undef snd_config_get_integer64
#undef snd_config_get_real
#undef snd_config_get_ireal
#undef snd_config_get_string
#undef snd_config_get_ascii
#undef snd_config_get_pointer
#undef snd_config_test_id
#undef snd_config_iterator_first
#undef snd_config_iterator_next
#undef snd_config_iterator_end
#undef snd_config_iterator_entry
#undef snd_config_get_bool_ascii
#undef snd_config_get_bool
#undef snd_config_get_ctl_iface_ascii
#undef snd_config_get_ctl_iface
#undef snd_names_list
#undef snd_names_list_free
#undef snd_pcm_open
#undef snd_pcm_open_lconf
#undef snd_pcm_open_fallback
#undef snd_pcm_close
#undef snd_pcm_name
#undef snd_pcm_type
#undef snd_pcm_stream
#undef snd_pcm_poll_descriptors_count
#undef snd_pcm_poll_descriptors
#undef snd_pcm_poll_descriptors_revents
#undef snd_pcm_nonblock
#undef snd_async_add_pcm_handler
#undef snd_async_handler_get_pcm
#undef snd_pcm_info
#undef snd_pcm_hw_params_current
#undef snd_pcm_hw_params
#undef snd_pcm_hw_free
#undef snd_pcm_sw_params_current
#undef snd_pcm_sw_params
#undef snd_pcm_prepare
#undef snd_pcm_reset
#undef snd_pcm_status
#undef snd_pcm_start
#undef snd_pcm_drop
#undef snd_pcm_drain
#undef snd_pcm_pause
#undef snd_pcm_state
#undef snd_pcm_hwsync
#undef snd_pcm_delay
#undef snd_pcm_resume
#undef snd_pcm_htimestamp
#undef snd_pcm_avail
#undef snd_pcm_avail_update
#undef snd_pcm_avail_delay
#undef snd_pcm_rewindable
#undef snd_pcm_rewind
#undef snd_pcm_forwardable
#undef snd_pcm_forward
#undef snd_pcm_writei
#undef snd_pcm_readi
#undef snd_pcm_writen
#undef snd_pcm_readn
#undef snd_pcm_wait
#undef snd_pcm_link
#undef snd_pcm_unlink
#undef snd_pcm_query_chmaps
#undef snd_pcm_query_chmaps_from_hw
#undef snd_pcm_free_chmaps
#undef snd_pcm_get_chmap
#undef snd_pcm_set_chmap
#undef snd_pcm_chmap_type_name
#undef snd_pcm_chmap_name
#undef snd_pcm_chmap_long_name
#undef snd_pcm_chmap_print
#undef snd_pcm_chmap_from_string
#undef snd_pcm_chmap_parse_string
#undef snd_pcm_recover
#undef snd_pcm_set_params
#undef snd_pcm_get_params
#undef snd_pcm_info_sizeof
#undef snd_pcm_info_malloc
#undef snd_pcm_info_free
#undef snd_pcm_info_copy
#undef snd_pcm_info_get_device
#undef snd_pcm_info_get_subdevice
#undef snd_pcm_info_get_stream
#undef snd_pcm_info_get_card
#undef snd_pcm_info_get_id
#undef snd_pcm_info_get_name
#undef snd_pcm_info_get_subdevice_name
#undef snd_pcm_info_get_class
#undef snd_pcm_info_get_subclass
#undef snd_pcm_info_get_subdevices_count
#undef snd_pcm_info_get_subdevices_avail
#undef snd_pcm_info_get_sync
#undef snd_pcm_info_set_device
#undef snd_pcm_info_set_subdevice
#undef snd_pcm_info_set_stream
#undef snd_pcm_hw_params_any
#undef snd_pcm_hw_params_can_mmap_sample_resolution
#undef snd_pcm_hw_params_is_double
#undef snd_pcm_hw_params_is_batch
#undef snd_pcm_hw_params_is_block_transfer
#undef snd_pcm_hw_params_is_monotonic
#undef snd_pcm_hw_params_can_overrange
#undef snd_pcm_hw_params_can_pause
#undef snd_pcm_hw_params_can_resume
#undef snd_pcm_hw_params_is_half_duplex
#undef snd_pcm_hw_params_is_joint_duplex
#undef snd_pcm_hw_params_can_sync_start
#undef snd_pcm_hw_params_can_disable_period_wakeup
#undef snd_pcm_hw_params_supports_audio_wallclock_ts
#undef snd_pcm_hw_params_supports_audio_ts_type
#undef snd_pcm_hw_params_get_rate_numden
#undef snd_pcm_hw_params_get_sbits
#undef snd_pcm_hw_params_get_fifo_size
#undef snd_pcm_hw_params_sizeof
#undef snd_pcm_hw_params_malloc
#undef snd_pcm_hw_params_free
#undef snd_pcm_hw_params_copy
#undef snd_pcm_hw_params_get_access
#undef snd_pcm_hw_params_test_access
#undef snd_pcm_hw_params_set_access
#undef snd_pcm_hw_params_set_access_first
#undef snd_pcm_hw_params_set_access_last
#undef snd_pcm_hw_params_set_access_mask
#undef snd_pcm_hw_params_get_access_mask
#undef snd_pcm_hw_params_get_format
#undef snd_pcm_hw_params_test_format
#undef snd_pcm_hw_params_set_format
#undef snd_pcm_hw_params_set_format_first
#undef snd_pcm_hw_params_set_format_last
#undef snd_pcm_hw_params_set_format_mask
#undef snd_pcm_hw_params_get_format_mask
#undef snd_pcm_hw_params_get_subformat
#undef snd_pcm_hw_params_test_subformat
#undef snd_pcm_hw_params_set_subformat
#undef snd_pcm_hw_params_set_subformat_first
#undef snd_pcm_hw_params_set_subformat_last
#undef snd_pcm_hw_params_set_subformat_mask
#undef snd_pcm_hw_params_get_subformat_mask
#undef snd_pcm_hw_params_get_channels
#undef snd_pcm_hw_params_get_channels_min
#undef snd_pcm_hw_params_get_channels_max
#undef snd_pcm_hw_params_test_channels
#undef snd_pcm_hw_params_set_channels
#undef snd_pcm_hw_params_set_channels_min
#undef snd_pcm_hw_params_set_channels_max
#undef snd_pcm_hw_params_set_channels_minmax
#undef snd_pcm_hw_params_set_channels_near
#undef snd_pcm_hw_params_set_channels_first
#undef snd_pcm_hw_params_set_channels_last
#undef snd_pcm_hw_params_get_rate
#undef snd_pcm_hw_params_get_rate_min
#undef snd_pcm_hw_params_get_rate_max
#undef snd_pcm_hw_params_test_rate
#undef snd_pcm_hw_params_set_rate
#undef snd_pcm_hw_params_set_rate_min
#undef snd_pcm_hw_params_set_rate_max
#undef snd_pcm_hw_params_set_rate_minmax
#undef snd_pcm_hw_params_set_rate_near
#undef snd_pcm_hw_params_set_rate_first
#undef snd_pcm_hw_params_set_rate_last
#undef snd_pcm_hw_params_set_rate_resample
#undef snd_pcm_hw_params_get_rate_resample
#undef snd_pcm_hw_params_set_export_buffer
#undef snd_pcm_hw_params_get_export_buffer
#undef snd_pcm_hw_params_set_period_wakeup
#undef snd_pcm_hw_params_get_period_wakeup
#undef snd_pcm_hw_params_get_period_time
#undef snd_pcm_hw_params_get_period_time_min
#undef snd_pcm_hw_params_get_period_time_max
#undef snd_pcm_hw_params_test_period_time
#undef snd_pcm_hw_params_set_period_time
#undef snd_pcm_hw_params_set_period_time_min
#undef snd_pcm_hw_params_set_period_time_max
#undef snd_pcm_hw_params_set_period_time_minmax
#undef snd_pcm_hw_params_set_period_time_near
#undef snd_pcm_hw_params_set_period_time_first
#undef snd_pcm_hw_params_set_period_time_last
#undef snd_pcm_hw_params_get_period_size
#undef snd_pcm_hw_params_get_period_size_min
#undef snd_pcm_hw_params_get_period_size_max
#undef snd_pcm_hw_params_test_period_size
#undef snd_pcm_hw_params_set_period_size
#undef snd_pcm_hw_params_set_period_size_min
#undef snd_pcm_hw_params_set_period_size_max
#undef snd_pcm_hw_params_set_period_size_minmax
#undef snd_pcm_hw_params_set_period_size_near
#undef snd_pcm_hw_params_set_period_size_first
#undef snd_pcm_hw_params_set_period_size_last
#undef snd_pcm_hw_params_set_period_size_integer
#undef snd_pcm_hw_params_get_periods
#undef snd_pcm_hw_params_get_periods_min
#undef snd_pcm_hw_params_get_periods_max
#undef snd_pcm_hw_params_test_periods
#undef snd_pcm_hw_params_set_periods
#undef snd_pcm_hw_params_set_periods_min
#undef snd_pcm_hw_params_set_periods_max
#undef snd_pcm_hw_params_set_periods_minmax
#undef snd_pcm_hw_params_set_periods_near
#undef snd_pcm_hw_params_set_periods_first
#undef snd_pcm_hw_params_set_periods_last
#undef snd_pcm_hw_params_set_periods_integer
#undef snd_pcm_hw_params_get_buffer_time
#undef snd_pcm_hw_params_get_buffer_time_min
#undef snd_pcm_hw_params_get_buffer_time_max
#undef snd_pcm_hw_params_test_buffer_time
#undef snd_pcm_hw_params_set_buffer_time
#undef snd_pcm_hw_params_set_buffer_time_min
#undef snd_pcm_hw_params_set_buffer_time_max
#undef snd_pcm_hw_params_set_buffer_time_minmax
#undef snd_pcm_hw_params_set_buffer_time_near
#undef snd_pcm_hw_params_set_buffer_time_first
#undef snd_pcm_hw_params_set_buffer_time_last
#undef snd_pcm_hw_params_get_buffer_size
#undef snd_pcm_hw_params_get_buffer_size_min
#undef snd_pcm_hw_params_get_buffer_size_max
#undef snd_pcm_hw_params_test_buffer_size
#undef snd_pcm_hw_params_set_buffer_size
#undef snd_pcm_hw_params_set_buffer_size_min
#undef snd_pcm_hw_params_set_buffer_size_max
#undef snd_pcm_hw_params_set_buffer_size_minmax
#undef snd_pcm_hw_params_set_buffer_size_near
#undef snd_pcm_hw_params_set_buffer_size_first
#undef snd_pcm_hw_params_set_buffer_size_last
#undef snd_pcm_hw_params_get_min_align
#undef snd_pcm_sw_params_sizeof
#undef snd_pcm_sw_params_malloc
#undef snd_pcm_sw_params_free
#undef snd_pcm_sw_params_copy
#undef snd_pcm_sw_params_get_boundary
#undef snd_pcm_sw_params_set_tstamp_mode
#undef snd_pcm_sw_params_get_tstamp_mode
#undef snd_pcm_sw_params_set_avail_min
#undef snd_pcm_sw_params_get_avail_min
#undef snd_pcm_sw_params_set_period_event
#undef snd_pcm_sw_params_get_period_event
#undef snd_pcm_sw_params_set_start_threshold
#undef snd_pcm_sw_params_get_start_threshold
#undef snd_pcm_sw_params_set_stop_threshold
#undef snd_pcm_sw_params_get_stop_threshold
#undef snd_pcm_sw_params_set_silence_threshold
#undef snd_pcm_sw_params_get_silence_threshold
#undef snd_pcm_sw_params_set_silence_size
#undef snd_pcm_sw_params_get_silence_size
#undef snd_pcm_access_mask_sizeof
#undef snd_pcm_access_mask_malloc
#undef snd_pcm_access_mask_free
#undef snd_pcm_access_mask_copy
#undef snd_pcm_access_mask_none
#undef snd_pcm_access_mask_any
#undef snd_pcm_access_mask_test
#undef snd_pcm_access_mask_empty
#undef snd_pcm_access_mask_set
#undef snd_pcm_access_mask_reset
#undef snd_pcm_format_mask_sizeof
#undef snd_pcm_format_mask_malloc
#undef snd_pcm_format_mask_free
#undef snd_pcm_format_mask_copy
#undef snd_pcm_format_mask_none
#undef snd_pcm_format_mask_any
#undef snd_pcm_format_mask_test
#undef snd_pcm_format_mask_empty
#undef snd_pcm_format_mask_set
#undef snd_pcm_format_mask_reset
#undef snd_pcm_subformat_mask_sizeof
#undef snd_pcm_subformat_mask_malloc
#undef snd_pcm_subformat_mask_free
#undef snd_pcm_subformat_mask_copy
#undef snd_pcm_subformat_mask_none
#undef snd_pcm_subformat_mask_any
#undef snd_pcm_subformat_mask_test
#undef snd_pcm_subformat_mask_empty
#undef snd_pcm_subformat_mask_set
#undef snd_pcm_subformat_mask_reset
#undef snd_pcm_status_sizeof
#undef snd_pcm_status_malloc
#undef snd_pcm_status_free
#undef snd_pcm_status_copy
#undef snd_pcm_status_get_state
#undef snd_pcm_status_get_trigger_tstamp
#undef snd_pcm_status_get_trigger_htstamp
#undef snd_pcm_status_get_tstamp
#undef snd_pcm_status_get_htstamp
#undef snd_pcm_status_get_audio_htstamp
#undef snd_pcm_status_get_driver_htstamp
#undef snd_pcm_status_get_delay
#undef snd_pcm_status_get_avail
#undef snd_pcm_status_get_avail_max
#undef snd_pcm_status_get_overrange
#undef snd_pcm_type_name
#undef snd_pcm_stream_name
#undef snd_pcm_access_name
#undef snd_pcm_format_name
#undef snd_pcm_format_description
#undef snd_pcm_subformat_name
#undef snd_pcm_subformat_description
#undef snd_pcm_format_value
#undef snd_pcm_tstamp_mode_name
#undef snd_pcm_state_name
#undef snd_pcm_dump
#undef snd_pcm_dump_hw_setup
#undef snd_pcm_dump_sw_setup
#undef snd_pcm_dump_setup
#undef snd_pcm_hw_params_dump
#undef snd_pcm_sw_params_dump
#undef snd_pcm_status_dump
#undef snd_pcm_mmap_begin
#undef snd_pcm_mmap_commit
#undef snd_pcm_mmap_writei
#undef snd_pcm_mmap_readi
#undef snd_pcm_mmap_writen
#undef snd_pcm_mmap_readn
#undef snd_pcm_format_signed
#undef snd_pcm_format_unsigned
#undef snd_pcm_format_linear
#undef snd_pcm_format_float
#undef snd_pcm_format_little_endian
#undef snd_pcm_format_big_endian
#undef snd_pcm_format_cpu_endian
#undef snd_pcm_format_width
#undef snd_pcm_format_physical_width
#undef snd_pcm_build_linear_format
#undef snd_pcm_format_size
#undef snd_pcm_format_silence
#undef snd_pcm_format_silence_16
#undef snd_pcm_format_silence_32
#undef snd_pcm_format_silence_64
#undef snd_pcm_format_set_silence
#undef snd_pcm_bytes_to_frames
#undef snd_pcm_frames_to_bytes
#undef snd_pcm_bytes_to_samples
#undef snd_pcm_samples_to_bytes
#undef snd_pcm_area_silence
#undef snd_pcm_areas_silence
#undef snd_pcm_area_copy
#undef snd_pcm_areas_copy
#undef snd_pcm_hook_get_pcm
#undef snd_pcm_hook_get_private
#undef snd_pcm_hook_set_private
#undef snd_pcm_hook_add
#undef snd_pcm_hook_remove
#undef snd_pcm_meter_get_bufsize
#undef snd_pcm_meter_get_channels
#undef snd_pcm_meter_get_rate
#undef snd_pcm_meter_get_now
#undef snd_pcm_meter_get_boundary
#undef snd_pcm_meter_add_scope
#undef snd_pcm_meter_search_scope
#undef snd_pcm_scope_malloc
#undef snd_pcm_scope_set_ops
#undef snd_pcm_scope_set_name
#undef snd_pcm_scope_get_name
#undef snd_pcm_scope_get_callback_private
#undef snd_pcm_scope_set_callback_private
#undef snd_pcm_scope_s16_open
#undef snd_pcm_scope_s16_get_channel_buffer
#undef snd_spcm_init
#undef snd_spcm_init_duplex
#undef snd_spcm_init_get_params
#undef snd_pcm_start_mode_name
#undef snd_pcm_xrun_mode_name
#undef snd_pcm_sw_params_set_start_mode
#undef snd_pcm_sw_params_get_start_mode
#undef snd_pcm_sw_params_set_xrun_mode
#undef snd_pcm_sw_params_get_xrun_mode
#undef snd_pcm_sw_params_set_xfer_align
#undef snd_pcm_sw_params_get_xfer_align
#undef snd_pcm_sw_params_set_sleep_min
#undef snd_pcm_sw_params_get_sleep_min
#undef snd_pcm_hw_params_get_tick_time
#undef snd_pcm_hw_params_get_tick_time_min
#undef snd_pcm_hw_params_get_tick_time_max
#undef snd_pcm_hw_params_test_tick_time
#undef snd_pcm_hw_params_set_tick_time
#undef snd_pcm_hw_params_set_tick_time_min
#undef snd_pcm_hw_params_set_tick_time_max
#undef snd_pcm_hw_params_set_tick_time_minmax
#undef snd_pcm_hw_params_set_tick_time_near
#undef snd_pcm_hw_params_set_tick_time_first
#undef snd_pcm_hw_params_set_tick_time_last
#undef snd_rawmidi_open
#undef snd_rawmidi_open_lconf
#undef snd_rawmidi_close
#undef snd_rawmidi_poll_descriptors_count
#undef snd_rawmidi_poll_descriptors
#undef snd_rawmidi_poll_descriptors_revents
#undef snd_rawmidi_nonblock
#undef snd_rawmidi_info_sizeof
#undef snd_rawmidi_info_malloc
#undef snd_rawmidi_info_free
#undef snd_rawmidi_info_copy
#undef snd_rawmidi_info_get_device
#undef snd_rawmidi_info_get_subdevice
#undef snd_rawmidi_info_get_stream
#undef snd_rawmidi_info_get_card
#undef snd_rawmidi_info_get_flags
#undef snd_rawmidi_info_get_id
#undef snd_rawmidi_info_get_name
#undef snd_rawmidi_info_get_subdevice_name
#undef snd_rawmidi_info_get_subdevices_count
#undef snd_rawmidi_info_get_subdevices_avail
#undef snd_rawmidi_info_set_device
#undef snd_rawmidi_info_set_subdevice
#undef snd_rawmidi_info_set_stream
#undef snd_rawmidi_info
#undef snd_rawmidi_params_sizeof
#undef snd_rawmidi_params_malloc
#undef snd_rawmidi_params_free
#undef snd_rawmidi_params_copy
#undef snd_rawmidi_params_set_buffer_size
#undef snd_rawmidi_params_get_buffer_size
#undef snd_rawmidi_params_set_avail_min
#undef snd_rawmidi_params_get_avail_min
#undef snd_rawmidi_params_set_no_active_sensing
#undef snd_rawmidi_params_get_no_active_sensing
#undef snd_rawmidi_params
#undef snd_rawmidi_params_current
#undef snd_rawmidi_status_sizeof
#undef snd_rawmidi_status_malloc
#undef snd_rawmidi_status_free
#undef snd_rawmidi_status_copy
#undef snd_rawmidi_status_get_tstamp
#undef snd_rawmidi_status_get_avail
#undef snd_rawmidi_status_get_xruns
#undef snd_rawmidi_status
#undef snd_rawmidi_drain
#undef snd_rawmidi_drop
#undef snd_rawmidi_write
#undef snd_rawmidi_read
#undef snd_rawmidi_name
#undef snd_rawmidi_type
#undef snd_rawmidi_stream
#undef snd_timer_query_open
#undef snd_timer_query_open_lconf
#undef snd_timer_query_close
#undef snd_timer_query_next_device
#undef snd_timer_query_info
#undef snd_timer_query_params
#undef snd_timer_query_status
#undef snd_timer_open
#undef snd_timer_open_lconf
#undef snd_timer_close
#undef snd_async_add_timer_handler
#undef snd_async_handler_get_timer
#undef snd_timer_poll_descriptors_count
#undef snd_timer_poll_descriptors
#undef snd_timer_poll_descriptors_revents
#undef snd_timer_info
#undef snd_timer_params
#undef snd_timer_status
#undef snd_timer_start
#undef snd_timer_stop
#undef snd_timer_continue
#undef snd_timer_read
#undef snd_timer_id_sizeof
#undef snd_timer_id_malloc
#undef snd_timer_id_free
#undef snd_timer_id_copy
#undef snd_timer_id_set_class
#undef snd_timer_id_get_class
#undef snd_timer_id_set_sclass
#undef snd_timer_id_get_sclass
#undef snd_timer_id_set_card
#undef snd_timer_id_get_card
#undef snd_timer_id_set_device
#undef snd_timer_id_get_device
#undef snd_timer_id_set_subdevice
#undef snd_timer_id_get_subdevice
#undef snd_timer_ginfo_sizeof
#undef snd_timer_ginfo_malloc
#undef snd_timer_ginfo_free
#undef snd_timer_ginfo_copy
#undef snd_timer_ginfo_set_tid
#undef snd_timer_ginfo_get_tid
#undef snd_timer_ginfo_get_flags
#undef snd_timer_ginfo_get_card
#undef snd_timer_ginfo_get_id
#undef snd_timer_ginfo_get_name
#undef snd_timer_ginfo_get_resolution
#undef snd_timer_ginfo_get_resolution_min
#undef snd_timer_ginfo_get_resolution_max
#undef snd_timer_ginfo_get_clients
#undef snd_timer_info_sizeof
#undef snd_timer_info_malloc
#undef snd_timer_info_free
#undef snd_timer_info_copy
#undef snd_timer_info_is_slave
#undef snd_timer_info_get_card
#undef snd_timer_info_get_id
#undef snd_timer_info_get_name
#undef snd_timer_info_get_resolution
#undef snd_timer_params_sizeof
#undef snd_timer_params_malloc
#undef snd_timer_params_free
#undef snd_timer_params_copy
#undef snd_timer_params_set_auto_start
#undef snd_timer_params_get_auto_start
#undef snd_timer_params_set_exclusive
#undef snd_timer_params_get_exclusive
#undef snd_timer_params_set_early_event
#undef snd_timer_params_get_early_event
#undef snd_timer_params_set_ticks
#undef snd_timer_params_get_ticks
#undef snd_timer_params_set_queue_size
#undef snd_timer_params_get_queue_size
#undef snd_timer_params_set_filter
#undef snd_timer_params_get_filter
#undef snd_timer_status_sizeof
#undef snd_timer_status_malloc
#undef snd_timer_status_free
#undef snd_timer_status_copy
#undef snd_timer_status_get_timestamp
#undef snd_timer_status_get_resolution
#undef snd_timer_status_get_lost
#undef snd_timer_status_get_overrun
#undef snd_timer_status_get_queue
#undef snd_timer_info_get_ticks
#undef snd_hwdep_open
#undef snd_hwdep_close
#undef snd_hwdep_poll_descriptors
#undef snd_hwdep_poll_descriptors_count
#undef snd_hwdep_poll_descriptors_revents
#undef snd_hwdep_nonblock
#undef snd_hwdep_info
#undef snd_hwdep_dsp_status
#undef snd_hwdep_dsp_load
#undef snd_hwdep_ioctl
#undef snd_hwdep_write
#undef snd_hwdep_read
#undef snd_hwdep_info_sizeof
#undef snd_hwdep_info_malloc
#undef snd_hwdep_info_free
#undef snd_hwdep_info_copy
#undef snd_hwdep_info_get_device
#undef snd_hwdep_info_get_card
#undef snd_hwdep_info_get_id
#undef snd_hwdep_info_get_name
#undef snd_hwdep_info_get_iface
#undef snd_hwdep_info_set_device
#undef snd_hwdep_dsp_status_sizeof
#undef snd_hwdep_dsp_status_malloc
#undef snd_hwdep_dsp_status_free
#undef snd_hwdep_dsp_status_copy
#undef snd_hwdep_dsp_status_get_version
#undef snd_hwdep_dsp_status_get_id
#undef snd_hwdep_dsp_status_get_num_dsps
#undef snd_hwdep_dsp_status_get_dsp_loaded
#undef snd_hwdep_dsp_status_get_chip_ready
#undef snd_hwdep_dsp_image_sizeof
#undef snd_hwdep_dsp_image_malloc
#undef snd_hwdep_dsp_image_free
#undef snd_hwdep_dsp_image_copy
#undef snd_hwdep_dsp_image_get_index
#undef snd_hwdep_dsp_image_get_name
#undef snd_hwdep_dsp_image_get_image
#undef snd_hwdep_dsp_image_get_length
#undef snd_hwdep_dsp_image_set_index
#undef snd_hwdep_dsp_image_set_name
#undef snd_hwdep_dsp_image_set_image
#undef snd_hwdep_dsp_image_set_length
#undef snd_card_load
#undef snd_card_next
#undef snd_card_get_index
#undef snd_card_get_name
#undef snd_card_get_longname
#undef snd_device_name_hint
#undef snd_device_name_free_hint
#undef snd_device_name_get_hint
#undef snd_ctl_open
#undef snd_ctl_open_lconf
#undef snd_ctl_open_fallback
#undef snd_ctl_close
#undef snd_ctl_nonblock
#undef snd_async_add_ctl_handler
#undef snd_async_handler_get_ctl
#undef snd_ctl_poll_descriptors_count
#undef snd_ctl_poll_descriptors
#undef snd_ctl_poll_descriptors_revents
#undef snd_ctl_subscribe_events
#undef snd_ctl_card_info
#undef snd_ctl_elem_list
#undef snd_ctl_elem_info
#undef snd_ctl_elem_read
#undef snd_ctl_elem_write
#undef snd_ctl_elem_lock
#undef snd_ctl_elem_unlock
#undef snd_ctl_elem_tlv_read
#undef snd_ctl_elem_tlv_write
#undef snd_ctl_elem_tlv_command
#undef snd_ctl_hwdep_next_device
#undef snd_ctl_hwdep_info
#undef snd_ctl_pcm_next_device
#undef snd_ctl_pcm_info
#undef snd_ctl_pcm_prefer_subdevice
#undef snd_ctl_rawmidi_next_device
#undef snd_ctl_rawmidi_info
#undef snd_ctl_rawmidi_prefer_subdevice
#undef snd_ctl_set_power_state
#undef snd_ctl_get_power_state
#undef snd_ctl_read
#undef snd_ctl_wait
#undef snd_ctl_name
#undef snd_ctl_type
#undef snd_ctl_elem_type_name
#undef snd_ctl_elem_iface_name
#undef snd_ctl_event_type_name
#undef snd_ctl_event_elem_get_mask
#undef snd_ctl_event_elem_get_numid
#undef snd_ctl_event_elem_get_id
#undef snd_ctl_event_elem_get_interface
#undef snd_ctl_event_elem_get_device
#undef snd_ctl_event_elem_get_subdevice
#undef snd_ctl_event_elem_get_name
#undef snd_ctl_event_elem_get_index
#undef snd_ctl_elem_list_alloc_space
#undef snd_ctl_elem_list_free_space
#undef snd_ctl_ascii_elem_id_get
#undef snd_ctl_ascii_elem_id_parse
#undef snd_ctl_ascii_value_parse
#undef snd_ctl_elem_id_sizeof
#undef snd_ctl_elem_id_malloc
#undef snd_ctl_elem_id_free
#undef snd_ctl_elem_id_clear
#undef snd_ctl_elem_id_copy
#undef snd_ctl_elem_id_get_numid
#undef snd_ctl_elem_id_get_interface
#undef snd_ctl_elem_id_get_device
#undef snd_ctl_elem_id_get_subdevice
#undef snd_ctl_elem_id_get_name
#undef snd_ctl_elem_id_get_index
#undef snd_ctl_elem_id_set_numid
#undef snd_ctl_elem_id_set_interface
#undef snd_ctl_elem_id_set_device
#undef snd_ctl_elem_id_set_subdevice
#undef snd_ctl_elem_id_set_name
#undef snd_ctl_elem_id_set_index
#undef snd_ctl_card_info_sizeof
#undef snd_ctl_card_info_malloc
#undef snd_ctl_card_info_free
#undef snd_ctl_card_info_clear
#undef snd_ctl_card_info_copy
#undef snd_ctl_card_info_get_card
#undef snd_ctl_card_info_get_id
#undef snd_ctl_card_info_get_driver
#undef snd_ctl_card_info_get_name
#undef snd_ctl_card_info_get_longname
#undef snd_ctl_card_info_get_mixername
#undef snd_ctl_card_info_get_components
#undef snd_ctl_event_sizeof
#undef snd_ctl_event_malloc
#undef snd_ctl_event_free
#undef snd_ctl_event_clear
#undef snd_ctl_event_copy
#undef snd_ctl_event_get_type
#undef snd_ctl_elem_list_sizeof
#undef snd_ctl_elem_list_malloc
#undef snd_ctl_elem_list_free
#undef snd_ctl_elem_list_clear
#undef snd_ctl_elem_list_copy
#undef snd_ctl_elem_list_set_offset
#undef snd_ctl_elem_list_get_used
#undef snd_ctl_elem_list_get_count
#undef snd_ctl_elem_list_get_id
#undef snd_ctl_elem_list_get_numid
#undef snd_ctl_elem_list_get_interface
#undef snd_ctl_elem_list_get_device
#undef snd_ctl_elem_list_get_subdevice
#undef snd_ctl_elem_list_get_name
#undef snd_ctl_elem_list_get_index
#undef snd_ctl_elem_info_sizeof
#undef snd_ctl_elem_info_malloc
#undef snd_ctl_elem_info_free
#undef snd_ctl_elem_info_clear
#undef snd_ctl_elem_info_copy
#undef snd_ctl_elem_info_get_type
#undef snd_ctl_elem_info_is_readable
#undef snd_ctl_elem_info_is_writable
#undef snd_ctl_elem_info_is_volatile
#undef snd_ctl_elem_info_is_inactive
#undef snd_ctl_elem_info_is_locked
#undef snd_ctl_elem_info_is_tlv_readable
#undef snd_ctl_elem_info_is_tlv_writable
#undef snd_ctl_elem_info_is_tlv_commandable
#undef snd_ctl_elem_info_is_owner
#undef snd_ctl_elem_info_is_user
#undef snd_ctl_elem_info_get_owner
#undef snd_ctl_elem_info_get_count
#undef snd_ctl_elem_info_get_min
#undef snd_ctl_elem_info_get_max
#undef snd_ctl_elem_info_get_step
#undef snd_ctl_elem_info_get_min64
#undef snd_ctl_elem_info_get_max64
#undef snd_ctl_elem_info_get_step64
#undef snd_ctl_elem_info_get_items
#undef snd_ctl_elem_info_set_item
#undef snd_ctl_elem_info_get_item_name
#undef snd_ctl_elem_info_get_dimensions
#undef snd_ctl_elem_info_get_dimension
#undef snd_ctl_elem_info_set_dimension
#undef snd_ctl_elem_info_get_id
#undef snd_ctl_elem_info_get_numid
#undef snd_ctl_elem_info_get_interface
#undef snd_ctl_elem_info_get_device
#undef snd_ctl_elem_info_get_subdevice
#undef snd_ctl_elem_info_get_name
#undef snd_ctl_elem_info_get_index
#undef snd_ctl_elem_info_set_id
#undef snd_ctl_elem_info_set_numid
#undef snd_ctl_elem_info_set_interface
#undef snd_ctl_elem_info_set_device
#undef snd_ctl_elem_info_set_subdevice
#undef snd_ctl_elem_info_set_name
#undef snd_ctl_elem_info_set_index
#undef snd_ctl_add_integer_elem_set
#undef snd_ctl_add_integer64_elem_set
#undef snd_ctl_add_boolean_elem_set
#undef snd_ctl_add_enumerated_elem_set
#undef snd_ctl_add_bytes_elem_set
#undef snd_ctl_elem_add_integer
#undef snd_ctl_elem_add_integer64
#undef snd_ctl_elem_add_boolean
#undef snd_ctl_elem_add_enumerated
#undef snd_ctl_elem_add_iec958
#undef snd_ctl_elem_remove
#undef snd_ctl_elem_value_sizeof
#undef snd_ctl_elem_value_malloc
#undef snd_ctl_elem_value_free
#undef snd_ctl_elem_value_clear
#undef snd_ctl_elem_value_copy
#undef snd_ctl_elem_value_compare
#undef snd_ctl_elem_value_get_id
#undef snd_ctl_elem_value_get_numid
#undef snd_ctl_elem_value_get_interface
#undef snd_ctl_elem_value_get_device
#undef snd_ctl_elem_value_get_subdevice
#undef snd_ctl_elem_value_get_name
#undef snd_ctl_elem_value_get_index
#undef snd_ctl_elem_value_set_id
#undef snd_ctl_elem_value_set_numid
#undef snd_ctl_elem_value_set_interface
#undef snd_ctl_elem_value_set_device
#undef snd_ctl_elem_value_set_subdevice
#undef snd_ctl_elem_value_set_name
#undef snd_ctl_elem_value_set_index
#undef snd_ctl_elem_value_get_boolean
#undef snd_ctl_elem_value_get_integer
#undef snd_ctl_elem_value_get_integer64
#undef snd_ctl_elem_value_get_enumerated
#undef snd_ctl_elem_value_get_byte
#undef snd_ctl_elem_value_set_boolean
#undef snd_ctl_elem_value_set_integer
#undef snd_ctl_elem_value_set_integer64
#undef snd_ctl_elem_value_set_enumerated
#undef snd_ctl_elem_value_set_byte
#undef snd_ctl_elem_set_bytes
#undef snd_ctl_elem_value_get_bytes
#undef snd_ctl_elem_value_get_iec958
#undef snd_ctl_elem_value_set_iec958
#undef snd_tlv_parse_dB_info
#undef snd_tlv_get_dB_range
#undef snd_tlv_convert_to_dB
#undef snd_tlv_convert_from_dB
#undef snd_ctl_get_dB_range
#undef snd_ctl_convert_to_dB
#undef snd_ctl_convert_from_dB
#undef snd_hctl_compare_fast
#undef snd_hctl_open
#undef snd_hctl_open_ctl
#undef snd_hctl_close
#undef snd_hctl_nonblock
#undef snd_hctl_poll_descriptors_count
#undef snd_hctl_poll_descriptors
#undef snd_hctl_poll_descriptors_revents
#undef snd_hctl_get_count
#undef snd_hctl_set_compare
#undef snd_hctl_first_elem
#undef snd_hctl_last_elem
#undef snd_hctl_find_elem
#undef snd_hctl_set_callback
#undef snd_hctl_set_callback_private
#undef snd_hctl_get_callback_private
#undef snd_hctl_load
#undef snd_hctl_free
#undef snd_hctl_handle_events
#undef snd_hctl_name
#undef snd_hctl_wait
#undef snd_hctl_ctl
#undef snd_hctl_elem_next
#undef snd_hctl_elem_prev
#undef snd_hctl_elem_info
#undef snd_hctl_elem_read
#undef snd_hctl_elem_write
#undef snd_hctl_elem_tlv_read
#undef snd_hctl_elem_tlv_write
#undef snd_hctl_elem_tlv_command
#undef snd_hctl_elem_get_hctl
#undef snd_hctl_elem_get_id
#undef snd_hctl_elem_get_numid
#undef snd_hctl_elem_get_interface
#undef snd_hctl_elem_get_device
#undef snd_hctl_elem_get_subdevice
#undef snd_hctl_elem_get_name
#undef snd_hctl_elem_get_index
#undef snd_hctl_elem_set_callback
#undef snd_hctl_elem_get_callback_private
#undef snd_hctl_elem_set_callback_private
#undef snd_sctl_build
#undef snd_sctl_free
#undef snd_sctl_install
#undef snd_sctl_remove
#undef snd_mixer_open
#undef snd_mixer_close
#undef snd_mixer_first_elem
#undef snd_mixer_last_elem
#undef snd_mixer_handle_events
#undef snd_mixer_attach
#undef snd_mixer_attach_hctl
#undef snd_mixer_detach
#undef snd_mixer_detach_hctl
#undef snd_mixer_get_hctl
#undef snd_mixer_poll_descriptors_count
#undef snd_mixer_poll_descriptors
#undef snd_mixer_poll_descriptors_revents
#undef snd_mixer_load
#undef snd_mixer_free
#undef snd_mixer_wait
#undef snd_mixer_set_compare
#undef snd_mixer_set_callback
#undef snd_mixer_get_callback_private
#undef snd_mixer_set_callback_private
#undef snd_mixer_get_count
#undef snd_mixer_class_unregister
#undef snd_mixer_elem_next
#undef snd_mixer_elem_prev
#undef snd_mixer_elem_set_callback
#undef snd_mixer_elem_get_callback_private
#undef snd_mixer_elem_set_callback_private
#undef snd_mixer_elem_get_type
#undef snd_mixer_class_register
#undef snd_mixer_elem_new
#undef snd_mixer_elem_add
#undef snd_mixer_elem_remove
#undef snd_mixer_elem_free
#undef snd_mixer_elem_info
#undef snd_mixer_elem_value
#undef snd_mixer_elem_attach
#undef snd_mixer_elem_detach
#undef snd_mixer_elem_empty
#undef snd_mixer_elem_get_private
#undef snd_mixer_class_sizeof
#undef snd_mixer_class_malloc
#undef snd_mixer_class_free
#undef snd_mixer_class_copy
#undef snd_mixer_class_get_mixer
#undef snd_mixer_class_get_event
#undef snd_mixer_class_get_private
#undef snd_mixer_class_get_compare
#undef snd_mixer_class_set_event
#undef snd_mixer_class_set_private
#undef snd_mixer_class_set_private_free
#undef snd_mixer_class_set_compare
#undef snd_mixer_selem_channel_name
#undef snd_mixer_selem_register
#undef snd_mixer_selem_get_id
#undef snd_mixer_selem_get_name
#undef snd_mixer_selem_get_index
#undef snd_mixer_find_selem
#undef snd_mixer_selem_is_active
#undef snd_mixer_selem_is_playback_mono
#undef snd_mixer_selem_has_playback_channel
#undef snd_mixer_selem_is_capture_mono
#undef snd_mixer_selem_has_capture_channel
#undef snd_mixer_selem_get_capture_group
#undef snd_mixer_selem_has_common_volume
#undef snd_mixer_selem_has_playback_volume
#undef snd_mixer_selem_has_playback_volume_joined
#undef snd_mixer_selem_has_capture_volume
#undef snd_mixer_selem_has_capture_volume_joined
#undef snd_mixer_selem_has_common_switch
#undef snd_mixer_selem_has_playback_switch
#undef snd_mixer_selem_has_playback_switch_joined
#undef snd_mixer_selem_has_capture_switch
#undef snd_mixer_selem_has_capture_switch_joined
#undef snd_mixer_selem_has_capture_switch_exclusive
#undef snd_mixer_selem_ask_playback_vol_dB
#undef snd_mixer_selem_ask_capture_vol_dB
#undef snd_mixer_selem_ask_playback_dB_vol
#undef snd_mixer_selem_ask_capture_dB_vol
#undef snd_mixer_selem_get_playback_volume
#undef snd_mixer_selem_get_capture_volume
#undef snd_mixer_selem_get_playback_dB
#undef snd_mixer_selem_get_capture_dB
#undef snd_mixer_selem_get_playback_switch
#undef snd_mixer_selem_get_capture_switch
#undef snd_mixer_selem_set_playback_volume
#undef snd_mixer_selem_set_capture_volume
#undef snd_mixer_selem_set_playback_dB
#undef snd_mixer_selem_set_capture_dB
#undef snd_mixer_selem_set_playback_volume_all
#undef snd_mixer_selem_set_capture_volume_all
#undef snd_mixer_selem_set_playback_dB_all
#undef snd_mixer_selem_set_capture_dB_all
#undef snd_mixer_selem_set_playback_switch
#undef snd_mixer_selem_set_capture_switch
#undef snd_mixer_selem_set_playback_switch_all
#undef snd_mixer_selem_set_capture_switch_all
#undef snd_mixer_selem_get_playback_volume_range
#undef snd_mixer_selem_get_playback_dB_range
#undef snd_mixer_selem_set_playback_volume_range
#undef snd_mixer_selem_get_capture_volume_range
#undef snd_mixer_selem_get_capture_dB_range
#undef snd_mixer_selem_set_capture_volume_range
#undef snd_mixer_selem_is_enumerated
#undef snd_mixer_selem_is_enum_playback
#undef snd_mixer_selem_is_enum_capture
#undef snd_mixer_selem_get_enum_items
#undef snd_mixer_selem_get_enum_item_name
#undef snd_mixer_selem_get_enum_item
#undef snd_mixer_selem_set_enum_item
#undef snd_mixer_selem_id_sizeof
#undef snd_mixer_selem_id_malloc
#undef snd_mixer_selem_id_free
#undef snd_mixer_selem_id_copy
#undef snd_mixer_selem_id_get_name
#undef snd_mixer_selem_id_get_index
#undef snd_mixer_selem_id_set_name
#undef snd_mixer_selem_id_set_index
#undef snd_seq_open
#undef snd_seq_open_lconf
#undef snd_seq_name
#undef snd_seq_type
#undef snd_seq_close
#undef snd_seq_poll_descriptors_count
#undef snd_seq_poll_descriptors
#undef snd_seq_poll_descriptors_revents
#undef snd_seq_nonblock
#undef snd_seq_client_id
#undef snd_seq_get_output_buffer_size
#undef snd_seq_get_input_buffer_size
#undef snd_seq_set_output_buffer_size
#undef snd_seq_set_input_buffer_size
#undef snd_seq_system_info_sizeof
#undef snd_seq_system_info_malloc
#undef snd_seq_system_info_free
#undef snd_seq_system_info_copy
#undef snd_seq_system_info_get_queues
#undef snd_seq_system_info_get_clients
#undef snd_seq_system_info_get_ports
#undef snd_seq_system_info_get_channels
#undef snd_seq_system_info_get_cur_clients
#undef snd_seq_system_info_get_cur_queues
#undef snd_seq_system_info
#undef snd_seq_client_info_sizeof
#undef snd_seq_client_info_malloc
#undef snd_seq_client_info_free
#undef snd_seq_client_info_copy
#undef snd_seq_client_info_get_client
#undef snd_seq_client_info_get_type
#undef snd_seq_client_info_get_name
#undef snd_seq_client_info_get_broadcast_filter
#undef snd_seq_client_info_get_error_bounce
#undef snd_seq_client_info_get_card
#undef snd_seq_client_info_get_pid
#undef snd_seq_client_info_get_event_filter
#undef snd_seq_client_info_get_num_ports
#undef snd_seq_client_info_get_event_lost
#undef snd_seq_client_info_set_client
#undef snd_seq_client_info_set_name
#undef snd_seq_client_info_set_broadcast_filter
#undef snd_seq_client_info_set_error_bounce
#undef snd_seq_client_info_set_event_filter
#undef snd_seq_client_info_event_filter_clear
#undef snd_seq_client_info_event_filter_add
#undef snd_seq_client_info_event_filter_del
#undef snd_seq_client_info_event_filter_check
#undef snd_seq_get_client_info
#undef snd_seq_get_any_client_info
#undef snd_seq_set_client_info
#undef snd_seq_query_next_client
#undef snd_seq_client_pool_sizeof
#undef snd_seq_client_pool_malloc
#undef snd_seq_client_pool_free
#undef snd_seq_client_pool_copy
#undef snd_seq_client_pool_get_client
#undef snd_seq_client_pool_get_output_pool
#undef snd_seq_client_pool_get_input_pool
#undef snd_seq_client_pool_get_output_room
#undef snd_seq_client_pool_get_output_free
#undef snd_seq_client_pool_get_input_free
#undef snd_seq_client_pool_set_output_pool
#undef snd_seq_client_pool_set_input_pool
#undef snd_seq_client_pool_set_output_room
#undef snd_seq_get_client_pool
#undef snd_seq_set_client_pool
#undef snd_seq_port_info_sizeof
#undef snd_seq_port_info_malloc
#undef snd_seq_port_info_free
#undef snd_seq_port_info_copy
#undef snd_seq_port_info_get_client
#undef snd_seq_port_info_get_port
#undef snd_seq_port_info_get_addr
#undef snd_seq_port_info_get_name
#undef snd_seq_port_info_get_capability
#undef snd_seq_port_info_get_type
#undef snd_seq_port_info_get_midi_channels
#undef snd_seq_port_info_get_midi_voices
#undef snd_seq_port_info_get_synth_voices
#undef snd_seq_port_info_get_read_use
#undef snd_seq_port_info_get_write_use
#undef snd_seq_port_info_get_port_specified
#undef snd_seq_port_info_get_timestamping
#undef snd_seq_port_info_get_timestamp_real
#undef snd_seq_port_info_get_timestamp_queue
#undef snd_seq_port_info_set_client
#undef snd_seq_port_info_set_port
#undef snd_seq_port_info_set_addr
#undef snd_seq_port_info_set_name
#undef snd_seq_port_info_set_capability
#undef snd_seq_port_info_set_type
#undef snd_seq_port_info_set_midi_channels
#undef snd_seq_port_info_set_midi_voices
#undef snd_seq_port_info_set_synth_voices
#undef snd_seq_port_info_set_port_specified
#undef snd_seq_port_info_set_timestamping
#undef snd_seq_port_info_set_timestamp_real
#undef snd_seq_port_info_set_timestamp_queue
#undef snd_seq_create_port
#undef snd_seq_delete_port
#undef snd_seq_get_port_info
#undef snd_seq_get_any_port_info
#undef snd_seq_set_port_info
#undef snd_seq_query_next_port
#undef snd_seq_port_subscribe_sizeof
#undef snd_seq_port_subscribe_malloc
#undef snd_seq_port_subscribe_free
#undef snd_seq_port_subscribe_copy
#undef snd_seq_port_subscribe_get_sender
#undef snd_seq_port_subscribe_get_dest
#undef snd_seq_port_subscribe_get_queue
#undef snd_seq_port_subscribe_get_exclusive
#undef snd_seq_port_subscribe_get_time_update
#undef snd_seq_port_subscribe_get_time_real
#undef snd_seq_port_subscribe_set_sender
#undef snd_seq_port_subscribe_set_dest
#undef snd_seq_port_subscribe_set_queue
#undef snd_seq_port_subscribe_set_exclusive
#undef snd_seq_port_subscribe_set_time_update
#undef snd_seq_port_subscribe_set_time_real
#undef snd_seq_get_port_subscription
#undef snd_seq_subscribe_port
#undef snd_seq_unsubscribe_port
#undef snd_seq_query_subscribe_sizeof
#undef snd_seq_query_subscribe_malloc
#undef snd_seq_query_subscribe_free
#undef snd_seq_query_subscribe_copy
#undef snd_seq_query_subscribe_get_client
#undef snd_seq_query_subscribe_get_port
#undef snd_seq_query_subscribe_get_root
#undef snd_seq_query_subscribe_get_type
#undef snd_seq_query_subscribe_get_index
#undef snd_seq_query_subscribe_get_num_subs
#undef snd_seq_query_subscribe_get_addr
#undef snd_seq_query_subscribe_get_queue
#undef snd_seq_query_subscribe_get_exclusive
#undef snd_seq_query_subscribe_get_time_update
#undef snd_seq_query_subscribe_get_time_real
#undef snd_seq_query_subscribe_set_client
#undef snd_seq_query_subscribe_set_port
#undef snd_seq_query_subscribe_set_root
#undef snd_seq_query_subscribe_set_type
#undef snd_seq_query_subscribe_set_index
#undef snd_seq_query_port_subscribers
#undef snd_seq_queue_info_sizeof
#undef snd_seq_queue_info_malloc
#undef snd_seq_queue_info_free
#undef snd_seq_queue_info_copy
#undef snd_seq_queue_info_get_queue
#undef snd_seq_queue_info_get_name
#undef snd_seq_queue_info_get_owner
#undef snd_seq_queue_info_get_locked
#undef snd_seq_queue_info_get_flags
#undef snd_seq_queue_info_set_name
#undef snd_seq_queue_info_set_owner
#undef snd_seq_queue_info_set_locked
#undef snd_seq_queue_info_set_flags
#undef snd_seq_create_queue
#undef snd_seq_alloc_named_queue
#undef snd_seq_alloc_queue
#undef snd_seq_free_queue
#undef snd_seq_get_queue_info
#undef snd_seq_set_queue_info
#undef snd_seq_query_named_queue
#undef snd_seq_get_queue_usage
#undef snd_seq_set_queue_usage
#undef snd_seq_queue_status_sizeof
#undef snd_seq_queue_status_malloc
#undef snd_seq_queue_status_free
#undef snd_seq_queue_status_copy
#undef snd_seq_queue_status_get_queue
#undef snd_seq_queue_status_get_events
#undef snd_seq_queue_status_get_tick_time
#undef snd_seq_queue_status_get_real_time
#undef snd_seq_queue_status_get_status
#undef snd_seq_get_queue_status
#undef snd_seq_queue_tempo_sizeof
#undef snd_seq_queue_tempo_malloc
#undef snd_seq_queue_tempo_free
#undef snd_seq_queue_tempo_copy
#undef snd_seq_queue_tempo_get_queue
#undef snd_seq_queue_tempo_get_tempo
#undef snd_seq_queue_tempo_get_ppq
#undef snd_seq_queue_tempo_get_skew
#undef snd_seq_queue_tempo_get_skew_base
#undef snd_seq_queue_tempo_set_tempo
#undef snd_seq_queue_tempo_set_ppq
#undef snd_seq_queue_tempo_set_skew
#undef snd_seq_queue_tempo_set_skew_base
#undef snd_seq_get_queue_tempo
#undef snd_seq_set_queue_tempo
#undef snd_seq_queue_timer_sizeof
#undef snd_seq_queue_timer_malloc
#undef snd_seq_queue_timer_free
#undef snd_seq_queue_timer_copy
#undef snd_seq_queue_timer_get_queue
#undef snd_seq_queue_timer_get_type
#undef snd_seq_queue_timer_get_id
#undef snd_seq_queue_timer_get_resolution
#undef snd_seq_queue_timer_set_type
#undef snd_seq_queue_timer_set_id
#undef snd_seq_queue_timer_set_resolution
#undef snd_seq_get_queue_timer
#undef snd_seq_set_queue_timer
#undef snd_seq_free_event
#undef snd_seq_event_length
#undef snd_seq_event_output
#undef snd_seq_event_output_buffer
#undef snd_seq_event_output_direct
#undef snd_seq_event_input
#undef snd_seq_event_input_pending
#undef snd_seq_drain_output
#undef snd_seq_event_output_pending
#undef snd_seq_extract_output
#undef snd_seq_drop_output
#undef snd_seq_drop_output_buffer
#undef snd_seq_drop_input
#undef snd_seq_drop_input_buffer
#undef snd_seq_remove_events_sizeof
#undef snd_seq_remove_events_malloc
#undef snd_seq_remove_events_free
#undef snd_seq_remove_events_copy
#undef snd_seq_remove_events_get_condition
#undef snd_seq_remove_events_get_queue
#undef snd_seq_remove_events_get_time
#undef snd_seq_remove_events_get_dest
#undef snd_seq_remove_events_get_channel
#undef snd_seq_remove_events_get_event_type
#undef snd_seq_remove_events_get_tag
#undef snd_seq_remove_events_set_condition
#undef snd_seq_remove_events_set_queue
#undef snd_seq_remove_events_set_time
#undef snd_seq_remove_events_set_dest
#undef snd_seq_remove_events_set_channel
#undef snd_seq_remove_events_set_event_type
#undef snd_seq_remove_events_set_tag
#undef snd_seq_remove_events
#undef snd_seq_set_bit
#undef snd_seq_unset_bit
#undef snd_seq_change_bit
#undef snd_seq_get_bit
#undef snd_seq_control_queue
#undef snd_seq_create_simple_port
#undef snd_seq_delete_simple_port
#undef snd_seq_connect_from
#undef snd_seq_connect_to
#undef snd_seq_disconnect_from
#undef snd_seq_disconnect_to
#undef snd_seq_set_client_name
#undef snd_seq_set_client_event_filter
#undef snd_seq_set_client_pool_output
#undef snd_seq_set_client_pool_output_room
#undef snd_seq_set_client_pool_input
#undef snd_seq_sync_output_queue
#undef snd_seq_parse_address
#undef snd_seq_reset_pool_output
#undef snd_seq_reset_pool_input
#undef snd_midi_event_new
#undef snd_midi_event_resize_buffer
#undef snd_midi_event_free
#undef snd_midi_event_init
#undef snd_midi_event_reset_encode
#undef snd_midi_event_reset_decode
#undef snd_midi_event_no_status
#undef snd_midi_event_encode
#undef snd_midi_event_encode_byte
#undef snd_midi_event_decode
#ifdef __cplusplus
extern "C" {
#endif
#define snd_asoundlib_version snd_asoundlib_version_dylibloader_wrapper_asound
#define snd_dlopen snd_dlopen_dylibloader_wrapper_asound
#define snd_dlsym snd_dlsym_dylibloader_wrapper_asound
#define snd_dlclose snd_dlclose_dylibloader_wrapper_asound
#define snd_async_add_handler snd_async_add_handler_dylibloader_wrapper_asound
#define snd_async_del_handler snd_async_del_handler_dylibloader_wrapper_asound
#define snd_async_handler_get_fd snd_async_handler_get_fd_dylibloader_wrapper_asound
#define snd_async_handler_get_signo snd_async_handler_get_signo_dylibloader_wrapper_asound
#define snd_async_handler_get_callback_private snd_async_handler_get_callback_private_dylibloader_wrapper_asound
#define snd_shm_area_create snd_shm_area_create_dylibloader_wrapper_asound
#define snd_shm_area_share snd_shm_area_share_dylibloader_wrapper_asound
#define snd_shm_area_destroy snd_shm_area_destroy_dylibloader_wrapper_asound
#define snd_user_file snd_user_file_dylibloader_wrapper_asound
#define snd_input_stdio_open snd_input_stdio_open_dylibloader_wrapper_asound
#define snd_input_stdio_attach snd_input_stdio_attach_dylibloader_wrapper_asound
#define snd_input_buffer_open snd_input_buffer_open_dylibloader_wrapper_asound
#define snd_input_close snd_input_close_dylibloader_wrapper_asound
#define snd_input_scanf snd_input_scanf_dylibloader_wrapper_asound
#define snd_input_gets snd_input_gets_dylibloader_wrapper_asound
#define snd_input_getc snd_input_getc_dylibloader_wrapper_asound
#define snd_input_ungetc snd_input_ungetc_dylibloader_wrapper_asound
#define snd_output_stdio_open snd_output_stdio_open_dylibloader_wrapper_asound
#define snd_output_stdio_attach snd_output_stdio_attach_dylibloader_wrapper_asound
#define snd_output_buffer_open snd_output_buffer_open_dylibloader_wrapper_asound
#define snd_output_buffer_string snd_output_buffer_string_dylibloader_wrapper_asound
#define snd_output_close snd_output_close_dylibloader_wrapper_asound
#define snd_output_printf snd_output_printf_dylibloader_wrapper_asound
#define snd_output_vprintf snd_output_vprintf_dylibloader_wrapper_asound
#define snd_output_puts snd_output_puts_dylibloader_wrapper_asound
#define snd_output_putc snd_output_putc_dylibloader_wrapper_asound
#define snd_output_flush snd_output_flush_dylibloader_wrapper_asound
#define snd_strerror snd_strerror_dylibloader_wrapper_asound
#define snd_lib_error_set_handler snd_lib_error_set_handler_dylibloader_wrapper_asound
#define snd_lib_error_set_local snd_lib_error_set_local_dylibloader_wrapper_asound
#define snd_config_top snd_config_top_dylibloader_wrapper_asound
#define snd_config_load snd_config_load_dylibloader_wrapper_asound
#define snd_config_load_override snd_config_load_override_dylibloader_wrapper_asound
#define snd_config_save snd_config_save_dylibloader_wrapper_asound
#define snd_config_update snd_config_update_dylibloader_wrapper_asound
#define snd_config_update_r snd_config_update_r_dylibloader_wrapper_asound
#define snd_config_update_free snd_config_update_free_dylibloader_wrapper_asound
#define snd_config_update_free_global snd_config_update_free_global_dylibloader_wrapper_asound
#define snd_config_update_ref snd_config_update_ref_dylibloader_wrapper_asound
#define snd_config_ref snd_config_ref_dylibloader_wrapper_asound
#define snd_config_unref snd_config_unref_dylibloader_wrapper_asound
#define snd_config_search snd_config_search_dylibloader_wrapper_asound
#define snd_config_searchv snd_config_searchv_dylibloader_wrapper_asound
#define snd_config_search_definition snd_config_search_definition_dylibloader_wrapper_asound
#define snd_config_expand snd_config_expand_dylibloader_wrapper_asound
#define snd_config_evaluate snd_config_evaluate_dylibloader_wrapper_asound
#define snd_config_add snd_config_add_dylibloader_wrapper_asound
#define snd_config_delete snd_config_delete_dylibloader_wrapper_asound
#define snd_config_delete_compound_members snd_config_delete_compound_members_dylibloader_wrapper_asound
#define snd_config_copy snd_config_copy_dylibloader_wrapper_asound
#define snd_config_make snd_config_make_dylibloader_wrapper_asound
#define snd_config_make_integer snd_config_make_integer_dylibloader_wrapper_asound
#define snd_config_make_integer64 snd_config_make_integer64_dylibloader_wrapper_asound
#define snd_config_make_real snd_config_make_real_dylibloader_wrapper_asound
#define snd_config_make_string snd_config_make_string_dylibloader_wrapper_asound
#define snd_config_make_pointer snd_config_make_pointer_dylibloader_wrapper_asound
#define snd_config_make_compound snd_config_make_compound_dylibloader_wrapper_asound
#define snd_config_imake_integer snd_config_imake_integer_dylibloader_wrapper_asound
#define snd_config_imake_integer64 snd_config_imake_integer64_dylibloader_wrapper_asound
#define snd_config_imake_real snd_config_imake_real_dylibloader_wrapper_asound
#define snd_config_imake_string snd_config_imake_string_dylibloader_wrapper_asound
#define snd_config_imake_safe_string snd_config_imake_safe_string_dylibloader_wrapper_asound
#define snd_config_imake_pointer snd_config_imake_pointer_dylibloader_wrapper_asound
#define snd_config_get_type snd_config_get_type_dylibloader_wrapper_asound
#define snd_config_set_id snd_config_set_id_dylibloader_wrapper_asound
#define snd_config_set_integer snd_config_set_integer_dylibloader_wrapper_asound
#define snd_config_set_integer64 snd_config_set_integer64_dylibloader_wrapper_asound
#define snd_config_set_real snd_config_set_real_dylibloader_wrapper_asound
#define snd_config_set_string snd_config_set_string_dylibloader_wrapper_asound
#define snd_config_set_ascii snd_config_set_ascii_dylibloader_wrapper_asound
#define snd_config_set_pointer snd_config_set_pointer_dylibloader_wrapper_asound
#define snd_config_get_id snd_config_get_id_dylibloader_wrapper_asound
#define snd_config_get_integer snd_config_get_integer_dylibloader_wrapper_asound
#define snd_config_get_integer64 snd_config_get_integer64_dylibloader_wrapper_asound
#define snd_config_get_real snd_config_get_real_dylibloader_wrapper_asound
#define snd_config_get_ireal snd_config_get_ireal_dylibloader_wrapper_asound
#define snd_config_get_string snd_config_get_string_dylibloader_wrapper_asound
#define snd_config_get_ascii snd_config_get_ascii_dylibloader_wrapper_asound
#define snd_config_get_pointer snd_config_get_pointer_dylibloader_wrapper_asound
#define snd_config_test_id snd_config_test_id_dylibloader_wrapper_asound
#define snd_config_iterator_first snd_config_iterator_first_dylibloader_wrapper_asound
#define snd_config_iterator_next snd_config_iterator_next_dylibloader_wrapper_asound
#define snd_config_iterator_end snd_config_iterator_end_dylibloader_wrapper_asound
#define snd_config_iterator_entry snd_config_iterator_entry_dylibloader_wrapper_asound
#define snd_config_get_bool_ascii snd_config_get_bool_ascii_dylibloader_wrapper_asound
#define snd_config_get_bool snd_config_get_bool_dylibloader_wrapper_asound
#define snd_config_get_ctl_iface_ascii snd_config_get_ctl_iface_ascii_dylibloader_wrapper_asound
#define snd_config_get_ctl_iface snd_config_get_ctl_iface_dylibloader_wrapper_asound
#define snd_names_list snd_names_list_dylibloader_wrapper_asound
#define snd_names_list_free snd_names_list_free_dylibloader_wrapper_asound
#define snd_pcm_open snd_pcm_open_dylibloader_wrapper_asound
#define snd_pcm_open_lconf snd_pcm_open_lconf_dylibloader_wrapper_asound
#define snd_pcm_open_fallback snd_pcm_open_fallback_dylibloader_wrapper_asound
#define snd_pcm_close snd_pcm_close_dylibloader_wrapper_asound
#define snd_pcm_name snd_pcm_name_dylibloader_wrapper_asound
#define snd_pcm_type snd_pcm_type_dylibloader_wrapper_asound
#define snd_pcm_stream snd_pcm_stream_dylibloader_wrapper_asound
#define snd_pcm_poll_descriptors_count snd_pcm_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_pcm_poll_descriptors snd_pcm_poll_descriptors_dylibloader_wrapper_asound
#define snd_pcm_poll_descriptors_revents snd_pcm_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_pcm_nonblock snd_pcm_nonblock_dylibloader_wrapper_asound
#define snd_async_add_pcm_handler snd_async_add_pcm_handler_dylibloader_wrapper_asound
#define snd_async_handler_get_pcm snd_async_handler_get_pcm_dylibloader_wrapper_asound
#define snd_pcm_info snd_pcm_info_dylibloader_wrapper_asound
#define snd_pcm_hw_params_current snd_pcm_hw_params_current_dylibloader_wrapper_asound
#define snd_pcm_hw_params snd_pcm_hw_params_dylibloader_wrapper_asound
#define snd_pcm_hw_free snd_pcm_hw_free_dylibloader_wrapper_asound
#define snd_pcm_sw_params_current snd_pcm_sw_params_current_dylibloader_wrapper_asound
#define snd_pcm_sw_params snd_pcm_sw_params_dylibloader_wrapper_asound
#define snd_pcm_prepare snd_pcm_prepare_dylibloader_wrapper_asound
#define snd_pcm_reset snd_pcm_reset_dylibloader_wrapper_asound
#define snd_pcm_status snd_pcm_status_dylibloader_wrapper_asound
#define snd_pcm_start snd_pcm_start_dylibloader_wrapper_asound
#define snd_pcm_drop snd_pcm_drop_dylibloader_wrapper_asound
#define snd_pcm_drain snd_pcm_drain_dylibloader_wrapper_asound
#define snd_pcm_pause snd_pcm_pause_dylibloader_wrapper_asound
#define snd_pcm_state snd_pcm_state_dylibloader_wrapper_asound
#define snd_pcm_hwsync snd_pcm_hwsync_dylibloader_wrapper_asound
#define snd_pcm_delay snd_pcm_delay_dylibloader_wrapper_asound
#define snd_pcm_resume snd_pcm_resume_dylibloader_wrapper_asound
#define snd_pcm_htimestamp snd_pcm_htimestamp_dylibloader_wrapper_asound
#define snd_pcm_avail snd_pcm_avail_dylibloader_wrapper_asound
#define snd_pcm_avail_update snd_pcm_avail_update_dylibloader_wrapper_asound
#define snd_pcm_avail_delay snd_pcm_avail_delay_dylibloader_wrapper_asound
#define snd_pcm_rewindable snd_pcm_rewindable_dylibloader_wrapper_asound
#define snd_pcm_rewind snd_pcm_rewind_dylibloader_wrapper_asound
#define snd_pcm_forwardable snd_pcm_forwardable_dylibloader_wrapper_asound
#define snd_pcm_forward snd_pcm_forward_dylibloader_wrapper_asound
#define snd_pcm_writei snd_pcm_writei_dylibloader_wrapper_asound
#define snd_pcm_readi snd_pcm_readi_dylibloader_wrapper_asound
#define snd_pcm_writen snd_pcm_writen_dylibloader_wrapper_asound
#define snd_pcm_readn snd_pcm_readn_dylibloader_wrapper_asound
#define snd_pcm_wait snd_pcm_wait_dylibloader_wrapper_asound
#define snd_pcm_link snd_pcm_link_dylibloader_wrapper_asound
#define snd_pcm_unlink snd_pcm_unlink_dylibloader_wrapper_asound
#define snd_pcm_query_chmaps snd_pcm_query_chmaps_dylibloader_wrapper_asound
#define snd_pcm_query_chmaps_from_hw snd_pcm_query_chmaps_from_hw_dylibloader_wrapper_asound
#define snd_pcm_free_chmaps snd_pcm_free_chmaps_dylibloader_wrapper_asound
#define snd_pcm_get_chmap snd_pcm_get_chmap_dylibloader_wrapper_asound
#define snd_pcm_set_chmap snd_pcm_set_chmap_dylibloader_wrapper_asound
#define snd_pcm_chmap_type_name snd_pcm_chmap_type_name_dylibloader_wrapper_asound
#define snd_pcm_chmap_name snd_pcm_chmap_name_dylibloader_wrapper_asound
#define snd_pcm_chmap_long_name snd_pcm_chmap_long_name_dylibloader_wrapper_asound
#define snd_pcm_chmap_print snd_pcm_chmap_print_dylibloader_wrapper_asound
#define snd_pcm_chmap_from_string snd_pcm_chmap_from_string_dylibloader_wrapper_asound
#define snd_pcm_chmap_parse_string snd_pcm_chmap_parse_string_dylibloader_wrapper_asound
#define snd_pcm_recover snd_pcm_recover_dylibloader_wrapper_asound
#define snd_pcm_set_params snd_pcm_set_params_dylibloader_wrapper_asound
#define snd_pcm_get_params snd_pcm_get_params_dylibloader_wrapper_asound
#define snd_pcm_info_sizeof snd_pcm_info_sizeof_dylibloader_wrapper_asound
#define snd_pcm_info_malloc snd_pcm_info_malloc_dylibloader_wrapper_asound
#define snd_pcm_info_free snd_pcm_info_free_dylibloader_wrapper_asound
#define snd_pcm_info_copy snd_pcm_info_copy_dylibloader_wrapper_asound
#define snd_pcm_info_get_device snd_pcm_info_get_device_dylibloader_wrapper_asound
#define snd_pcm_info_get_subdevice snd_pcm_info_get_subdevice_dylibloader_wrapper_asound
#define snd_pcm_info_get_stream snd_pcm_info_get_stream_dylibloader_wrapper_asound
#define snd_pcm_info_get_card snd_pcm_info_get_card_dylibloader_wrapper_asound
#define snd_pcm_info_get_id snd_pcm_info_get_id_dylibloader_wrapper_asound
#define snd_pcm_info_get_name snd_pcm_info_get_name_dylibloader_wrapper_asound
#define snd_pcm_info_get_subdevice_name snd_pcm_info_get_subdevice_name_dylibloader_wrapper_asound
#define snd_pcm_info_get_class snd_pcm_info_get_class_dylibloader_wrapper_asound
#define snd_pcm_info_get_subclass snd_pcm_info_get_subclass_dylibloader_wrapper_asound
#define snd_pcm_info_get_subdevices_count snd_pcm_info_get_subdevices_count_dylibloader_wrapper_asound
#define snd_pcm_info_get_subdevices_avail snd_pcm_info_get_subdevices_avail_dylibloader_wrapper_asound
#define snd_pcm_info_get_sync snd_pcm_info_get_sync_dylibloader_wrapper_asound
#define snd_pcm_info_set_device snd_pcm_info_set_device_dylibloader_wrapper_asound
#define snd_pcm_info_set_subdevice snd_pcm_info_set_subdevice_dylibloader_wrapper_asound
#define snd_pcm_info_set_stream snd_pcm_info_set_stream_dylibloader_wrapper_asound
#define snd_pcm_hw_params_any snd_pcm_hw_params_any_dylibloader_wrapper_asound
#define snd_pcm_hw_params_can_mmap_sample_resolution snd_pcm_hw_params_can_mmap_sample_resolution_dylibloader_wrapper_asound
#define snd_pcm_hw_params_is_double snd_pcm_hw_params_is_double_dylibloader_wrapper_asound
#define snd_pcm_hw_params_is_batch snd_pcm_hw_params_is_batch_dylibloader_wrapper_asound
#define snd_pcm_hw_params_is_block_transfer snd_pcm_hw_params_is_block_transfer_dylibloader_wrapper_asound
#define snd_pcm_hw_params_is_monotonic snd_pcm_hw_params_is_monotonic_dylibloader_wrapper_asound
#define snd_pcm_hw_params_can_overrange snd_pcm_hw_params_can_overrange_dylibloader_wrapper_asound
#define snd_pcm_hw_params_can_pause snd_pcm_hw_params_can_pause_dylibloader_wrapper_asound
#define snd_pcm_hw_params_can_resume snd_pcm_hw_params_can_resume_dylibloader_wrapper_asound
#define snd_pcm_hw_params_is_half_duplex snd_pcm_hw_params_is_half_duplex_dylibloader_wrapper_asound
#define snd_pcm_hw_params_is_joint_duplex snd_pcm_hw_params_is_joint_duplex_dylibloader_wrapper_asound
#define snd_pcm_hw_params_can_sync_start snd_pcm_hw_params_can_sync_start_dylibloader_wrapper_asound
#define snd_pcm_hw_params_can_disable_period_wakeup snd_pcm_hw_params_can_disable_period_wakeup_dylibloader_wrapper_asound
#define snd_pcm_hw_params_supports_audio_wallclock_ts snd_pcm_hw_params_supports_audio_wallclock_ts_dylibloader_wrapper_asound
#define snd_pcm_hw_params_supports_audio_ts_type snd_pcm_hw_params_supports_audio_ts_type_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_rate_numden snd_pcm_hw_params_get_rate_numden_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_sbits snd_pcm_hw_params_get_sbits_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_fifo_size snd_pcm_hw_params_get_fifo_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_sizeof snd_pcm_hw_params_sizeof_dylibloader_wrapper_asound
#define snd_pcm_hw_params_malloc snd_pcm_hw_params_malloc_dylibloader_wrapper_asound
#define snd_pcm_hw_params_free snd_pcm_hw_params_free_dylibloader_wrapper_asound
#define snd_pcm_hw_params_copy snd_pcm_hw_params_copy_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_access snd_pcm_hw_params_get_access_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_access snd_pcm_hw_params_test_access_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_access snd_pcm_hw_params_set_access_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_access_first snd_pcm_hw_params_set_access_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_access_last snd_pcm_hw_params_set_access_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_access_mask snd_pcm_hw_params_set_access_mask_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_access_mask snd_pcm_hw_params_get_access_mask_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_format snd_pcm_hw_params_get_format_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_format snd_pcm_hw_params_test_format_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_format snd_pcm_hw_params_set_format_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_format_first snd_pcm_hw_params_set_format_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_format_last snd_pcm_hw_params_set_format_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_format_mask snd_pcm_hw_params_set_format_mask_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_format_mask snd_pcm_hw_params_get_format_mask_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_subformat snd_pcm_hw_params_get_subformat_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_subformat snd_pcm_hw_params_test_subformat_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_subformat snd_pcm_hw_params_set_subformat_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_subformat_first snd_pcm_hw_params_set_subformat_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_subformat_last snd_pcm_hw_params_set_subformat_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_subformat_mask snd_pcm_hw_params_set_subformat_mask_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_subformat_mask snd_pcm_hw_params_get_subformat_mask_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_channels snd_pcm_hw_params_get_channels_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_channels_min snd_pcm_hw_params_get_channels_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_channels_max snd_pcm_hw_params_get_channels_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_channels snd_pcm_hw_params_test_channels_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels snd_pcm_hw_params_set_channels_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels_min snd_pcm_hw_params_set_channels_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels_max snd_pcm_hw_params_set_channels_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels_minmax snd_pcm_hw_params_set_channels_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels_near snd_pcm_hw_params_set_channels_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels_first snd_pcm_hw_params_set_channels_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_channels_last snd_pcm_hw_params_set_channels_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_rate snd_pcm_hw_params_get_rate_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_rate_min snd_pcm_hw_params_get_rate_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_rate_max snd_pcm_hw_params_get_rate_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_rate snd_pcm_hw_params_test_rate_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate snd_pcm_hw_params_set_rate_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_min snd_pcm_hw_params_set_rate_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_max snd_pcm_hw_params_set_rate_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_minmax snd_pcm_hw_params_set_rate_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_near snd_pcm_hw_params_set_rate_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_first snd_pcm_hw_params_set_rate_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_last snd_pcm_hw_params_set_rate_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_rate_resample snd_pcm_hw_params_set_rate_resample_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_rate_resample snd_pcm_hw_params_get_rate_resample_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_export_buffer snd_pcm_hw_params_set_export_buffer_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_export_buffer snd_pcm_hw_params_get_export_buffer_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_wakeup snd_pcm_hw_params_set_period_wakeup_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_wakeup snd_pcm_hw_params_get_period_wakeup_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_time snd_pcm_hw_params_get_period_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_time_min snd_pcm_hw_params_get_period_time_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_time_max snd_pcm_hw_params_get_period_time_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_period_time snd_pcm_hw_params_test_period_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time snd_pcm_hw_params_set_period_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time_min snd_pcm_hw_params_set_period_time_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time_max snd_pcm_hw_params_set_period_time_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time_minmax snd_pcm_hw_params_set_period_time_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time_near snd_pcm_hw_params_set_period_time_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time_first snd_pcm_hw_params_set_period_time_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_time_last snd_pcm_hw_params_set_period_time_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_size snd_pcm_hw_params_get_period_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_size_min snd_pcm_hw_params_get_period_size_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_period_size_max snd_pcm_hw_params_get_period_size_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_period_size snd_pcm_hw_params_test_period_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size snd_pcm_hw_params_set_period_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_min snd_pcm_hw_params_set_period_size_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_max snd_pcm_hw_params_set_period_size_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_minmax snd_pcm_hw_params_set_period_size_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_near snd_pcm_hw_params_set_period_size_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_first snd_pcm_hw_params_set_period_size_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_last snd_pcm_hw_params_set_period_size_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_period_size_integer snd_pcm_hw_params_set_period_size_integer_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_periods snd_pcm_hw_params_get_periods_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_periods_min snd_pcm_hw_params_get_periods_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_periods_max snd_pcm_hw_params_get_periods_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_periods snd_pcm_hw_params_test_periods_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods snd_pcm_hw_params_set_periods_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_min snd_pcm_hw_params_set_periods_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_max snd_pcm_hw_params_set_periods_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_minmax snd_pcm_hw_params_set_periods_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_near snd_pcm_hw_params_set_periods_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_first snd_pcm_hw_params_set_periods_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_last snd_pcm_hw_params_set_periods_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_periods_integer snd_pcm_hw_params_set_periods_integer_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_buffer_time snd_pcm_hw_params_get_buffer_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_buffer_time_min snd_pcm_hw_params_get_buffer_time_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_buffer_time_max snd_pcm_hw_params_get_buffer_time_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_buffer_time snd_pcm_hw_params_test_buffer_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time snd_pcm_hw_params_set_buffer_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time_min snd_pcm_hw_params_set_buffer_time_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time_max snd_pcm_hw_params_set_buffer_time_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time_minmax snd_pcm_hw_params_set_buffer_time_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time_near snd_pcm_hw_params_set_buffer_time_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time_first snd_pcm_hw_params_set_buffer_time_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_time_last snd_pcm_hw_params_set_buffer_time_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_buffer_size snd_pcm_hw_params_get_buffer_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_buffer_size_min snd_pcm_hw_params_get_buffer_size_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_buffer_size_max snd_pcm_hw_params_get_buffer_size_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_buffer_size snd_pcm_hw_params_test_buffer_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size snd_pcm_hw_params_set_buffer_size_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size_min snd_pcm_hw_params_set_buffer_size_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size_max snd_pcm_hw_params_set_buffer_size_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size_minmax snd_pcm_hw_params_set_buffer_size_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size_near snd_pcm_hw_params_set_buffer_size_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size_first snd_pcm_hw_params_set_buffer_size_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_buffer_size_last snd_pcm_hw_params_set_buffer_size_last_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_min_align snd_pcm_hw_params_get_min_align_dylibloader_wrapper_asound
#define snd_pcm_sw_params_sizeof snd_pcm_sw_params_sizeof_dylibloader_wrapper_asound
#define snd_pcm_sw_params_malloc snd_pcm_sw_params_malloc_dylibloader_wrapper_asound
#define snd_pcm_sw_params_free snd_pcm_sw_params_free_dylibloader_wrapper_asound
#define snd_pcm_sw_params_copy snd_pcm_sw_params_copy_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_boundary snd_pcm_sw_params_get_boundary_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_tstamp_mode snd_pcm_sw_params_set_tstamp_mode_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_tstamp_mode snd_pcm_sw_params_get_tstamp_mode_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_avail_min snd_pcm_sw_params_set_avail_min_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_avail_min snd_pcm_sw_params_get_avail_min_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_period_event snd_pcm_sw_params_set_period_event_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_period_event snd_pcm_sw_params_get_period_event_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_start_threshold snd_pcm_sw_params_set_start_threshold_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_start_threshold snd_pcm_sw_params_get_start_threshold_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_stop_threshold snd_pcm_sw_params_set_stop_threshold_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_stop_threshold snd_pcm_sw_params_get_stop_threshold_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_silence_threshold snd_pcm_sw_params_set_silence_threshold_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_silence_threshold snd_pcm_sw_params_get_silence_threshold_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_silence_size snd_pcm_sw_params_set_silence_size_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_silence_size snd_pcm_sw_params_get_silence_size_dylibloader_wrapper_asound
#define snd_pcm_access_mask_sizeof snd_pcm_access_mask_sizeof_dylibloader_wrapper_asound
#define snd_pcm_access_mask_malloc snd_pcm_access_mask_malloc_dylibloader_wrapper_asound
#define snd_pcm_access_mask_free snd_pcm_access_mask_free_dylibloader_wrapper_asound
#define snd_pcm_access_mask_copy snd_pcm_access_mask_copy_dylibloader_wrapper_asound
#define snd_pcm_access_mask_none snd_pcm_access_mask_none_dylibloader_wrapper_asound
#define snd_pcm_access_mask_any snd_pcm_access_mask_any_dylibloader_wrapper_asound
#define snd_pcm_access_mask_test snd_pcm_access_mask_test_dylibloader_wrapper_asound
#define snd_pcm_access_mask_empty snd_pcm_access_mask_empty_dylibloader_wrapper_asound
#define snd_pcm_access_mask_set snd_pcm_access_mask_set_dylibloader_wrapper_asound
#define snd_pcm_access_mask_reset snd_pcm_access_mask_reset_dylibloader_wrapper_asound
#define snd_pcm_format_mask_sizeof snd_pcm_format_mask_sizeof_dylibloader_wrapper_asound
#define snd_pcm_format_mask_malloc snd_pcm_format_mask_malloc_dylibloader_wrapper_asound
#define snd_pcm_format_mask_free snd_pcm_format_mask_free_dylibloader_wrapper_asound
#define snd_pcm_format_mask_copy snd_pcm_format_mask_copy_dylibloader_wrapper_asound
#define snd_pcm_format_mask_none snd_pcm_format_mask_none_dylibloader_wrapper_asound
#define snd_pcm_format_mask_any snd_pcm_format_mask_any_dylibloader_wrapper_asound
#define snd_pcm_format_mask_test snd_pcm_format_mask_test_dylibloader_wrapper_asound
#define snd_pcm_format_mask_empty snd_pcm_format_mask_empty_dylibloader_wrapper_asound
#define snd_pcm_format_mask_set snd_pcm_format_mask_set_dylibloader_wrapper_asound
#define snd_pcm_format_mask_reset snd_pcm_format_mask_reset_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_sizeof snd_pcm_subformat_mask_sizeof_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_malloc snd_pcm_subformat_mask_malloc_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_free snd_pcm_subformat_mask_free_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_copy snd_pcm_subformat_mask_copy_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_none snd_pcm_subformat_mask_none_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_any snd_pcm_subformat_mask_any_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_test snd_pcm_subformat_mask_test_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_empty snd_pcm_subformat_mask_empty_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_set snd_pcm_subformat_mask_set_dylibloader_wrapper_asound
#define snd_pcm_subformat_mask_reset snd_pcm_subformat_mask_reset_dylibloader_wrapper_asound
#define snd_pcm_status_sizeof snd_pcm_status_sizeof_dylibloader_wrapper_asound
#define snd_pcm_status_malloc snd_pcm_status_malloc_dylibloader_wrapper_asound
#define snd_pcm_status_free snd_pcm_status_free_dylibloader_wrapper_asound
#define snd_pcm_status_copy snd_pcm_status_copy_dylibloader_wrapper_asound
#define snd_pcm_status_get_state snd_pcm_status_get_state_dylibloader_wrapper_asound
#define snd_pcm_status_get_trigger_tstamp snd_pcm_status_get_trigger_tstamp_dylibloader_wrapper_asound
#define snd_pcm_status_get_trigger_htstamp snd_pcm_status_get_trigger_htstamp_dylibloader_wrapper_asound
#define snd_pcm_status_get_tstamp snd_pcm_status_get_tstamp_dylibloader_wrapper_asound
#define snd_pcm_status_get_htstamp snd_pcm_status_get_htstamp_dylibloader_wrapper_asound
#define snd_pcm_status_get_audio_htstamp snd_pcm_status_get_audio_htstamp_dylibloader_wrapper_asound
#define snd_pcm_status_get_driver_htstamp snd_pcm_status_get_driver_htstamp_dylibloader_wrapper_asound
#define snd_pcm_status_get_delay snd_pcm_status_get_delay_dylibloader_wrapper_asound
#define snd_pcm_status_get_avail snd_pcm_status_get_avail_dylibloader_wrapper_asound
#define snd_pcm_status_get_avail_max snd_pcm_status_get_avail_max_dylibloader_wrapper_asound
#define snd_pcm_status_get_overrange snd_pcm_status_get_overrange_dylibloader_wrapper_asound
#define snd_pcm_type_name snd_pcm_type_name_dylibloader_wrapper_asound
#define snd_pcm_stream_name snd_pcm_stream_name_dylibloader_wrapper_asound
#define snd_pcm_access_name snd_pcm_access_name_dylibloader_wrapper_asound
#define snd_pcm_format_name snd_pcm_format_name_dylibloader_wrapper_asound
#define snd_pcm_format_description snd_pcm_format_description_dylibloader_wrapper_asound
#define snd_pcm_subformat_name snd_pcm_subformat_name_dylibloader_wrapper_asound
#define snd_pcm_subformat_description snd_pcm_subformat_description_dylibloader_wrapper_asound
#define snd_pcm_format_value snd_pcm_format_value_dylibloader_wrapper_asound
#define snd_pcm_tstamp_mode_name snd_pcm_tstamp_mode_name_dylibloader_wrapper_asound
#define snd_pcm_state_name snd_pcm_state_name_dylibloader_wrapper_asound
#define snd_pcm_dump snd_pcm_dump_dylibloader_wrapper_asound
#define snd_pcm_dump_hw_setup snd_pcm_dump_hw_setup_dylibloader_wrapper_asound
#define snd_pcm_dump_sw_setup snd_pcm_dump_sw_setup_dylibloader_wrapper_asound
#define snd_pcm_dump_setup snd_pcm_dump_setup_dylibloader_wrapper_asound
#define snd_pcm_hw_params_dump snd_pcm_hw_params_dump_dylibloader_wrapper_asound
#define snd_pcm_sw_params_dump snd_pcm_sw_params_dump_dylibloader_wrapper_asound
#define snd_pcm_status_dump snd_pcm_status_dump_dylibloader_wrapper_asound
#define snd_pcm_mmap_begin snd_pcm_mmap_begin_dylibloader_wrapper_asound
#define snd_pcm_mmap_commit snd_pcm_mmap_commit_dylibloader_wrapper_asound
#define snd_pcm_mmap_writei snd_pcm_mmap_writei_dylibloader_wrapper_asound
#define snd_pcm_mmap_readi snd_pcm_mmap_readi_dylibloader_wrapper_asound
#define snd_pcm_mmap_writen snd_pcm_mmap_writen_dylibloader_wrapper_asound
#define snd_pcm_mmap_readn snd_pcm_mmap_readn_dylibloader_wrapper_asound
#define snd_pcm_format_signed snd_pcm_format_signed_dylibloader_wrapper_asound
#define snd_pcm_format_unsigned snd_pcm_format_unsigned_dylibloader_wrapper_asound
#define snd_pcm_format_linear snd_pcm_format_linear_dylibloader_wrapper_asound
#define snd_pcm_format_float snd_pcm_format_float_dylibloader_wrapper_asound
#define snd_pcm_format_little_endian snd_pcm_format_little_endian_dylibloader_wrapper_asound
#define snd_pcm_format_big_endian snd_pcm_format_big_endian_dylibloader_wrapper_asound
#define snd_pcm_format_cpu_endian snd_pcm_format_cpu_endian_dylibloader_wrapper_asound
#define snd_pcm_format_width snd_pcm_format_width_dylibloader_wrapper_asound
#define snd_pcm_format_physical_width snd_pcm_format_physical_width_dylibloader_wrapper_asound
#define snd_pcm_build_linear_format snd_pcm_build_linear_format_dylibloader_wrapper_asound
#define snd_pcm_format_size snd_pcm_format_size_dylibloader_wrapper_asound
#define snd_pcm_format_silence snd_pcm_format_silence_dylibloader_wrapper_asound
#define snd_pcm_format_silence_16 snd_pcm_format_silence_16_dylibloader_wrapper_asound
#define snd_pcm_format_silence_32 snd_pcm_format_silence_32_dylibloader_wrapper_asound
#define snd_pcm_format_silence_64 snd_pcm_format_silence_64_dylibloader_wrapper_asound
#define snd_pcm_format_set_silence snd_pcm_format_set_silence_dylibloader_wrapper_asound
#define snd_pcm_bytes_to_frames snd_pcm_bytes_to_frames_dylibloader_wrapper_asound
#define snd_pcm_frames_to_bytes snd_pcm_frames_to_bytes_dylibloader_wrapper_asound
#define snd_pcm_bytes_to_samples snd_pcm_bytes_to_samples_dylibloader_wrapper_asound
#define snd_pcm_samples_to_bytes snd_pcm_samples_to_bytes_dylibloader_wrapper_asound
#define snd_pcm_area_silence snd_pcm_area_silence_dylibloader_wrapper_asound
#define snd_pcm_areas_silence snd_pcm_areas_silence_dylibloader_wrapper_asound
#define snd_pcm_area_copy snd_pcm_area_copy_dylibloader_wrapper_asound
#define snd_pcm_areas_copy snd_pcm_areas_copy_dylibloader_wrapper_asound
#define snd_pcm_hook_get_pcm snd_pcm_hook_get_pcm_dylibloader_wrapper_asound
#define snd_pcm_hook_get_private snd_pcm_hook_get_private_dylibloader_wrapper_asound
#define snd_pcm_hook_set_private snd_pcm_hook_set_private_dylibloader_wrapper_asound
#define snd_pcm_hook_add snd_pcm_hook_add_dylibloader_wrapper_asound
#define snd_pcm_hook_remove snd_pcm_hook_remove_dylibloader_wrapper_asound
#define snd_pcm_meter_get_bufsize snd_pcm_meter_get_bufsize_dylibloader_wrapper_asound
#define snd_pcm_meter_get_channels snd_pcm_meter_get_channels_dylibloader_wrapper_asound
#define snd_pcm_meter_get_rate snd_pcm_meter_get_rate_dylibloader_wrapper_asound
#define snd_pcm_meter_get_now snd_pcm_meter_get_now_dylibloader_wrapper_asound
#define snd_pcm_meter_get_boundary snd_pcm_meter_get_boundary_dylibloader_wrapper_asound
#define snd_pcm_meter_add_scope snd_pcm_meter_add_scope_dylibloader_wrapper_asound
#define snd_pcm_meter_search_scope snd_pcm_meter_search_scope_dylibloader_wrapper_asound
#define snd_pcm_scope_malloc snd_pcm_scope_malloc_dylibloader_wrapper_asound
#define snd_pcm_scope_set_ops snd_pcm_scope_set_ops_dylibloader_wrapper_asound
#define snd_pcm_scope_set_name snd_pcm_scope_set_name_dylibloader_wrapper_asound
#define snd_pcm_scope_get_name snd_pcm_scope_get_name_dylibloader_wrapper_asound
#define snd_pcm_scope_get_callback_private snd_pcm_scope_get_callback_private_dylibloader_wrapper_asound
#define snd_pcm_scope_set_callback_private snd_pcm_scope_set_callback_private_dylibloader_wrapper_asound
#define snd_pcm_scope_s16_open snd_pcm_scope_s16_open_dylibloader_wrapper_asound
#define snd_pcm_scope_s16_get_channel_buffer snd_pcm_scope_s16_get_channel_buffer_dylibloader_wrapper_asound
#define snd_spcm_init snd_spcm_init_dylibloader_wrapper_asound
#define snd_spcm_init_duplex snd_spcm_init_duplex_dylibloader_wrapper_asound
#define snd_spcm_init_get_params snd_spcm_init_get_params_dylibloader_wrapper_asound
#define snd_pcm_start_mode_name snd_pcm_start_mode_name_dylibloader_wrapper_asound
#define snd_pcm_xrun_mode_name snd_pcm_xrun_mode_name_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_start_mode snd_pcm_sw_params_set_start_mode_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_start_mode snd_pcm_sw_params_get_start_mode_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_xrun_mode snd_pcm_sw_params_set_xrun_mode_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_xrun_mode snd_pcm_sw_params_get_xrun_mode_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_xfer_align snd_pcm_sw_params_set_xfer_align_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_xfer_align snd_pcm_sw_params_get_xfer_align_dylibloader_wrapper_asound
#define snd_pcm_sw_params_set_sleep_min snd_pcm_sw_params_set_sleep_min_dylibloader_wrapper_asound
#define snd_pcm_sw_params_get_sleep_min snd_pcm_sw_params_get_sleep_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_tick_time snd_pcm_hw_params_get_tick_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_tick_time_min snd_pcm_hw_params_get_tick_time_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_get_tick_time_max snd_pcm_hw_params_get_tick_time_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_test_tick_time snd_pcm_hw_params_test_tick_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time snd_pcm_hw_params_set_tick_time_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time_min snd_pcm_hw_params_set_tick_time_min_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time_max snd_pcm_hw_params_set_tick_time_max_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time_minmax snd_pcm_hw_params_set_tick_time_minmax_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time_near snd_pcm_hw_params_set_tick_time_near_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time_first snd_pcm_hw_params_set_tick_time_first_dylibloader_wrapper_asound
#define snd_pcm_hw_params_set_tick_time_last snd_pcm_hw_params_set_tick_time_last_dylibloader_wrapper_asound
#define snd_rawmidi_open snd_rawmidi_open_dylibloader_wrapper_asound
#define snd_rawmidi_open_lconf snd_rawmidi_open_lconf_dylibloader_wrapper_asound
#define snd_rawmidi_close snd_rawmidi_close_dylibloader_wrapper_asound
#define snd_rawmidi_poll_descriptors_count snd_rawmidi_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_rawmidi_poll_descriptors snd_rawmidi_poll_descriptors_dylibloader_wrapper_asound
#define snd_rawmidi_poll_descriptors_revents snd_rawmidi_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_rawmidi_nonblock snd_rawmidi_nonblock_dylibloader_wrapper_asound
#define snd_rawmidi_info_sizeof snd_rawmidi_info_sizeof_dylibloader_wrapper_asound
#define snd_rawmidi_info_malloc snd_rawmidi_info_malloc_dylibloader_wrapper_asound
#define snd_rawmidi_info_free snd_rawmidi_info_free_dylibloader_wrapper_asound
#define snd_rawmidi_info_copy snd_rawmidi_info_copy_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_device snd_rawmidi_info_get_device_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_subdevice snd_rawmidi_info_get_subdevice_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_stream snd_rawmidi_info_get_stream_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_card snd_rawmidi_info_get_card_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_flags snd_rawmidi_info_get_flags_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_id snd_rawmidi_info_get_id_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_name snd_rawmidi_info_get_name_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_subdevice_name snd_rawmidi_info_get_subdevice_name_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_subdevices_count snd_rawmidi_info_get_subdevices_count_dylibloader_wrapper_asound
#define snd_rawmidi_info_get_subdevices_avail snd_rawmidi_info_get_subdevices_avail_dylibloader_wrapper_asound
#define snd_rawmidi_info_set_device snd_rawmidi_info_set_device_dylibloader_wrapper_asound
#define snd_rawmidi_info_set_subdevice snd_rawmidi_info_set_subdevice_dylibloader_wrapper_asound
#define snd_rawmidi_info_set_stream snd_rawmidi_info_set_stream_dylibloader_wrapper_asound
#define snd_rawmidi_info snd_rawmidi_info_dylibloader_wrapper_asound
#define snd_rawmidi_params_sizeof snd_rawmidi_params_sizeof_dylibloader_wrapper_asound
#define snd_rawmidi_params_malloc snd_rawmidi_params_malloc_dylibloader_wrapper_asound
#define snd_rawmidi_params_free snd_rawmidi_params_free_dylibloader_wrapper_asound
#define snd_rawmidi_params_copy snd_rawmidi_params_copy_dylibloader_wrapper_asound
#define snd_rawmidi_params_set_buffer_size snd_rawmidi_params_set_buffer_size_dylibloader_wrapper_asound
#define snd_rawmidi_params_get_buffer_size snd_rawmidi_params_get_buffer_size_dylibloader_wrapper_asound
#define snd_rawmidi_params_set_avail_min snd_rawmidi_params_set_avail_min_dylibloader_wrapper_asound
#define snd_rawmidi_params_get_avail_min snd_rawmidi_params_get_avail_min_dylibloader_wrapper_asound
#define snd_rawmidi_params_set_no_active_sensing snd_rawmidi_params_set_no_active_sensing_dylibloader_wrapper_asound
#define snd_rawmidi_params_get_no_active_sensing snd_rawmidi_params_get_no_active_sensing_dylibloader_wrapper_asound
#define snd_rawmidi_params snd_rawmidi_params_dylibloader_wrapper_asound
#define snd_rawmidi_params_current snd_rawmidi_params_current_dylibloader_wrapper_asound
#define snd_rawmidi_status_sizeof snd_rawmidi_status_sizeof_dylibloader_wrapper_asound
#define snd_rawmidi_status_malloc snd_rawmidi_status_malloc_dylibloader_wrapper_asound
#define snd_rawmidi_status_free snd_rawmidi_status_free_dylibloader_wrapper_asound
#define snd_rawmidi_status_copy snd_rawmidi_status_copy_dylibloader_wrapper_asound
#define snd_rawmidi_status_get_tstamp snd_rawmidi_status_get_tstamp_dylibloader_wrapper_asound
#define snd_rawmidi_status_get_avail snd_rawmidi_status_get_avail_dylibloader_wrapper_asound
#define snd_rawmidi_status_get_xruns snd_rawmidi_status_get_xruns_dylibloader_wrapper_asound
#define snd_rawmidi_status snd_rawmidi_status_dylibloader_wrapper_asound
#define snd_rawmidi_drain snd_rawmidi_drain_dylibloader_wrapper_asound
#define snd_rawmidi_drop snd_rawmidi_drop_dylibloader_wrapper_asound
#define snd_rawmidi_write snd_rawmidi_write_dylibloader_wrapper_asound
#define snd_rawmidi_read snd_rawmidi_read_dylibloader_wrapper_asound
#define snd_rawmidi_name snd_rawmidi_name_dylibloader_wrapper_asound
#define snd_rawmidi_type snd_rawmidi_type_dylibloader_wrapper_asound
#define snd_rawmidi_stream snd_rawmidi_stream_dylibloader_wrapper_asound
#define snd_timer_query_open snd_timer_query_open_dylibloader_wrapper_asound
#define snd_timer_query_open_lconf snd_timer_query_open_lconf_dylibloader_wrapper_asound
#define snd_timer_query_close snd_timer_query_close_dylibloader_wrapper_asound
#define snd_timer_query_next_device snd_timer_query_next_device_dylibloader_wrapper_asound
#define snd_timer_query_info snd_timer_query_info_dylibloader_wrapper_asound
#define snd_timer_query_params snd_timer_query_params_dylibloader_wrapper_asound
#define snd_timer_query_status snd_timer_query_status_dylibloader_wrapper_asound
#define snd_timer_open snd_timer_open_dylibloader_wrapper_asound
#define snd_timer_open_lconf snd_timer_open_lconf_dylibloader_wrapper_asound
#define snd_timer_close snd_timer_close_dylibloader_wrapper_asound
#define snd_async_add_timer_handler snd_async_add_timer_handler_dylibloader_wrapper_asound
#define snd_async_handler_get_timer snd_async_handler_get_timer_dylibloader_wrapper_asound
#define snd_timer_poll_descriptors_count snd_timer_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_timer_poll_descriptors snd_timer_poll_descriptors_dylibloader_wrapper_asound
#define snd_timer_poll_descriptors_revents snd_timer_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_timer_info snd_timer_info_dylibloader_wrapper_asound
#define snd_timer_params snd_timer_params_dylibloader_wrapper_asound
#define snd_timer_status snd_timer_status_dylibloader_wrapper_asound
#define snd_timer_start snd_timer_start_dylibloader_wrapper_asound
#define snd_timer_stop snd_timer_stop_dylibloader_wrapper_asound
#define snd_timer_continue snd_timer_continue_dylibloader_wrapper_asound
#define snd_timer_read snd_timer_read_dylibloader_wrapper_asound
#define snd_timer_id_sizeof snd_timer_id_sizeof_dylibloader_wrapper_asound
#define snd_timer_id_malloc snd_timer_id_malloc_dylibloader_wrapper_asound
#define snd_timer_id_free snd_timer_id_free_dylibloader_wrapper_asound
#define snd_timer_id_copy snd_timer_id_copy_dylibloader_wrapper_asound
#define snd_timer_id_set_class snd_timer_id_set_class_dylibloader_wrapper_asound
#define snd_timer_id_get_class snd_timer_id_get_class_dylibloader_wrapper_asound
#define snd_timer_id_set_sclass snd_timer_id_set_sclass_dylibloader_wrapper_asound
#define snd_timer_id_get_sclass snd_timer_id_get_sclass_dylibloader_wrapper_asound
#define snd_timer_id_set_card snd_timer_id_set_card_dylibloader_wrapper_asound
#define snd_timer_id_get_card snd_timer_id_get_card_dylibloader_wrapper_asound
#define snd_timer_id_set_device snd_timer_id_set_device_dylibloader_wrapper_asound
#define snd_timer_id_get_device snd_timer_id_get_device_dylibloader_wrapper_asound
#define snd_timer_id_set_subdevice snd_timer_id_set_subdevice_dylibloader_wrapper_asound
#define snd_timer_id_get_subdevice snd_timer_id_get_subdevice_dylibloader_wrapper_asound
#define snd_timer_ginfo_sizeof snd_timer_ginfo_sizeof_dylibloader_wrapper_asound
#define snd_timer_ginfo_malloc snd_timer_ginfo_malloc_dylibloader_wrapper_asound
#define snd_timer_ginfo_free snd_timer_ginfo_free_dylibloader_wrapper_asound
#define snd_timer_ginfo_copy snd_timer_ginfo_copy_dylibloader_wrapper_asound
#define snd_timer_ginfo_set_tid snd_timer_ginfo_set_tid_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_tid snd_timer_ginfo_get_tid_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_flags snd_timer_ginfo_get_flags_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_card snd_timer_ginfo_get_card_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_id snd_timer_ginfo_get_id_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_name snd_timer_ginfo_get_name_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_resolution snd_timer_ginfo_get_resolution_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_resolution_min snd_timer_ginfo_get_resolution_min_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_resolution_max snd_timer_ginfo_get_resolution_max_dylibloader_wrapper_asound
#define snd_timer_ginfo_get_clients snd_timer_ginfo_get_clients_dylibloader_wrapper_asound
#define snd_timer_info_sizeof snd_timer_info_sizeof_dylibloader_wrapper_asound
#define snd_timer_info_malloc snd_timer_info_malloc_dylibloader_wrapper_asound
#define snd_timer_info_free snd_timer_info_free_dylibloader_wrapper_asound
#define snd_timer_info_copy snd_timer_info_copy_dylibloader_wrapper_asound
#define snd_timer_info_is_slave snd_timer_info_is_slave_dylibloader_wrapper_asound
#define snd_timer_info_get_card snd_timer_info_get_card_dylibloader_wrapper_asound
#define snd_timer_info_get_id snd_timer_info_get_id_dylibloader_wrapper_asound
#define snd_timer_info_get_name snd_timer_info_get_name_dylibloader_wrapper_asound
#define snd_timer_info_get_resolution snd_timer_info_get_resolution_dylibloader_wrapper_asound
#define snd_timer_params_sizeof snd_timer_params_sizeof_dylibloader_wrapper_asound
#define snd_timer_params_malloc snd_timer_params_malloc_dylibloader_wrapper_asound
#define snd_timer_params_free snd_timer_params_free_dylibloader_wrapper_asound
#define snd_timer_params_copy snd_timer_params_copy_dylibloader_wrapper_asound
#define snd_timer_params_set_auto_start snd_timer_params_set_auto_start_dylibloader_wrapper_asound
#define snd_timer_params_get_auto_start snd_timer_params_get_auto_start_dylibloader_wrapper_asound
#define snd_timer_params_set_exclusive snd_timer_params_set_exclusive_dylibloader_wrapper_asound
#define snd_timer_params_get_exclusive snd_timer_params_get_exclusive_dylibloader_wrapper_asound
#define snd_timer_params_set_early_event snd_timer_params_set_early_event_dylibloader_wrapper_asound
#define snd_timer_params_get_early_event snd_timer_params_get_early_event_dylibloader_wrapper_asound
#define snd_timer_params_set_ticks snd_timer_params_set_ticks_dylibloader_wrapper_asound
#define snd_timer_params_get_ticks snd_timer_params_get_ticks_dylibloader_wrapper_asound
#define snd_timer_params_set_queue_size snd_timer_params_set_queue_size_dylibloader_wrapper_asound
#define snd_timer_params_get_queue_size snd_timer_params_get_queue_size_dylibloader_wrapper_asound
#define snd_timer_params_set_filter snd_timer_params_set_filter_dylibloader_wrapper_asound
#define snd_timer_params_get_filter snd_timer_params_get_filter_dylibloader_wrapper_asound
#define snd_timer_status_sizeof snd_timer_status_sizeof_dylibloader_wrapper_asound
#define snd_timer_status_malloc snd_timer_status_malloc_dylibloader_wrapper_asound
#define snd_timer_status_free snd_timer_status_free_dylibloader_wrapper_asound
#define snd_timer_status_copy snd_timer_status_copy_dylibloader_wrapper_asound
#define snd_timer_status_get_timestamp snd_timer_status_get_timestamp_dylibloader_wrapper_asound
#define snd_timer_status_get_resolution snd_timer_status_get_resolution_dylibloader_wrapper_asound
#define snd_timer_status_get_lost snd_timer_status_get_lost_dylibloader_wrapper_asound
#define snd_timer_status_get_overrun snd_timer_status_get_overrun_dylibloader_wrapper_asound
#define snd_timer_status_get_queue snd_timer_status_get_queue_dylibloader_wrapper_asound
#define snd_timer_info_get_ticks snd_timer_info_get_ticks_dylibloader_wrapper_asound
#define snd_hwdep_open snd_hwdep_open_dylibloader_wrapper_asound
#define snd_hwdep_close snd_hwdep_close_dylibloader_wrapper_asound
#define snd_hwdep_poll_descriptors snd_hwdep_poll_descriptors_dylibloader_wrapper_asound
#define snd_hwdep_poll_descriptors_count snd_hwdep_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_hwdep_poll_descriptors_revents snd_hwdep_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_hwdep_nonblock snd_hwdep_nonblock_dylibloader_wrapper_asound
#define snd_hwdep_info snd_hwdep_info_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status snd_hwdep_dsp_status_dylibloader_wrapper_asound
#define snd_hwdep_dsp_load snd_hwdep_dsp_load_dylibloader_wrapper_asound
#define snd_hwdep_ioctl snd_hwdep_ioctl_dylibloader_wrapper_asound
#define snd_hwdep_write snd_hwdep_write_dylibloader_wrapper_asound
#define snd_hwdep_read snd_hwdep_read_dylibloader_wrapper_asound
#define snd_hwdep_info_sizeof snd_hwdep_info_sizeof_dylibloader_wrapper_asound
#define snd_hwdep_info_malloc snd_hwdep_info_malloc_dylibloader_wrapper_asound
#define snd_hwdep_info_free snd_hwdep_info_free_dylibloader_wrapper_asound
#define snd_hwdep_info_copy snd_hwdep_info_copy_dylibloader_wrapper_asound
#define snd_hwdep_info_get_device snd_hwdep_info_get_device_dylibloader_wrapper_asound
#define snd_hwdep_info_get_card snd_hwdep_info_get_card_dylibloader_wrapper_asound
#define snd_hwdep_info_get_id snd_hwdep_info_get_id_dylibloader_wrapper_asound
#define snd_hwdep_info_get_name snd_hwdep_info_get_name_dylibloader_wrapper_asound
#define snd_hwdep_info_get_iface snd_hwdep_info_get_iface_dylibloader_wrapper_asound
#define snd_hwdep_info_set_device snd_hwdep_info_set_device_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_sizeof snd_hwdep_dsp_status_sizeof_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_malloc snd_hwdep_dsp_status_malloc_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_free snd_hwdep_dsp_status_free_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_copy snd_hwdep_dsp_status_copy_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_get_version snd_hwdep_dsp_status_get_version_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_get_id snd_hwdep_dsp_status_get_id_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_get_num_dsps snd_hwdep_dsp_status_get_num_dsps_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_get_dsp_loaded snd_hwdep_dsp_status_get_dsp_loaded_dylibloader_wrapper_asound
#define snd_hwdep_dsp_status_get_chip_ready snd_hwdep_dsp_status_get_chip_ready_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_sizeof snd_hwdep_dsp_image_sizeof_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_malloc snd_hwdep_dsp_image_malloc_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_free snd_hwdep_dsp_image_free_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_copy snd_hwdep_dsp_image_copy_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_get_index snd_hwdep_dsp_image_get_index_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_get_name snd_hwdep_dsp_image_get_name_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_get_image snd_hwdep_dsp_image_get_image_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_get_length snd_hwdep_dsp_image_get_length_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_set_index snd_hwdep_dsp_image_set_index_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_set_name snd_hwdep_dsp_image_set_name_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_set_image snd_hwdep_dsp_image_set_image_dylibloader_wrapper_asound
#define snd_hwdep_dsp_image_set_length snd_hwdep_dsp_image_set_length_dylibloader_wrapper_asound
#define snd_card_load snd_card_load_dylibloader_wrapper_asound
#define snd_card_next snd_card_next_dylibloader_wrapper_asound
#define snd_card_get_index snd_card_get_index_dylibloader_wrapper_asound
#define snd_card_get_name snd_card_get_name_dylibloader_wrapper_asound
#define snd_card_get_longname snd_card_get_longname_dylibloader_wrapper_asound
#define snd_device_name_hint snd_device_name_hint_dylibloader_wrapper_asound
#define snd_device_name_free_hint snd_device_name_free_hint_dylibloader_wrapper_asound
#define snd_device_name_get_hint snd_device_name_get_hint_dylibloader_wrapper_asound
#define snd_ctl_open snd_ctl_open_dylibloader_wrapper_asound
#define snd_ctl_open_lconf snd_ctl_open_lconf_dylibloader_wrapper_asound
#define snd_ctl_open_fallback snd_ctl_open_fallback_dylibloader_wrapper_asound
#define snd_ctl_close snd_ctl_close_dylibloader_wrapper_asound
#define snd_ctl_nonblock snd_ctl_nonblock_dylibloader_wrapper_asound
#define snd_async_add_ctl_handler snd_async_add_ctl_handler_dylibloader_wrapper_asound
#define snd_async_handler_get_ctl snd_async_handler_get_ctl_dylibloader_wrapper_asound
#define snd_ctl_poll_descriptors_count snd_ctl_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_ctl_poll_descriptors snd_ctl_poll_descriptors_dylibloader_wrapper_asound
#define snd_ctl_poll_descriptors_revents snd_ctl_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_ctl_subscribe_events snd_ctl_subscribe_events_dylibloader_wrapper_asound
#define snd_ctl_card_info snd_ctl_card_info_dylibloader_wrapper_asound
#define snd_ctl_elem_list snd_ctl_elem_list_dylibloader_wrapper_asound
#define snd_ctl_elem_info snd_ctl_elem_info_dylibloader_wrapper_asound
#define snd_ctl_elem_read snd_ctl_elem_read_dylibloader_wrapper_asound
#define snd_ctl_elem_write snd_ctl_elem_write_dylibloader_wrapper_asound
#define snd_ctl_elem_lock snd_ctl_elem_lock_dylibloader_wrapper_asound
#define snd_ctl_elem_unlock snd_ctl_elem_unlock_dylibloader_wrapper_asound
#define snd_ctl_elem_tlv_read snd_ctl_elem_tlv_read_dylibloader_wrapper_asound
#define snd_ctl_elem_tlv_write snd_ctl_elem_tlv_write_dylibloader_wrapper_asound
#define snd_ctl_elem_tlv_command snd_ctl_elem_tlv_command_dylibloader_wrapper_asound
#define snd_ctl_hwdep_next_device snd_ctl_hwdep_next_device_dylibloader_wrapper_asound
#define snd_ctl_hwdep_info snd_ctl_hwdep_info_dylibloader_wrapper_asound
#define snd_ctl_pcm_next_device snd_ctl_pcm_next_device_dylibloader_wrapper_asound
#define snd_ctl_pcm_info snd_ctl_pcm_info_dylibloader_wrapper_asound
#define snd_ctl_pcm_prefer_subdevice snd_ctl_pcm_prefer_subdevice_dylibloader_wrapper_asound
#define snd_ctl_rawmidi_next_device snd_ctl_rawmidi_next_device_dylibloader_wrapper_asound
#define snd_ctl_rawmidi_info snd_ctl_rawmidi_info_dylibloader_wrapper_asound
#define snd_ctl_rawmidi_prefer_subdevice snd_ctl_rawmidi_prefer_subdevice_dylibloader_wrapper_asound
#define snd_ctl_set_power_state snd_ctl_set_power_state_dylibloader_wrapper_asound
#define snd_ctl_get_power_state snd_ctl_get_power_state_dylibloader_wrapper_asound
#define snd_ctl_read snd_ctl_read_dylibloader_wrapper_asound
#define snd_ctl_wait snd_ctl_wait_dylibloader_wrapper_asound
#define snd_ctl_name snd_ctl_name_dylibloader_wrapper_asound
#define snd_ctl_type snd_ctl_type_dylibloader_wrapper_asound
#define snd_ctl_elem_type_name snd_ctl_elem_type_name_dylibloader_wrapper_asound
#define snd_ctl_elem_iface_name snd_ctl_elem_iface_name_dylibloader_wrapper_asound
#define snd_ctl_event_type_name snd_ctl_event_type_name_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_mask snd_ctl_event_elem_get_mask_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_numid snd_ctl_event_elem_get_numid_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_id snd_ctl_event_elem_get_id_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_interface snd_ctl_event_elem_get_interface_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_device snd_ctl_event_elem_get_device_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_subdevice snd_ctl_event_elem_get_subdevice_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_name snd_ctl_event_elem_get_name_dylibloader_wrapper_asound
#define snd_ctl_event_elem_get_index snd_ctl_event_elem_get_index_dylibloader_wrapper_asound
#define snd_ctl_elem_list_alloc_space snd_ctl_elem_list_alloc_space_dylibloader_wrapper_asound
#define snd_ctl_elem_list_free_space snd_ctl_elem_list_free_space_dylibloader_wrapper_asound
#define snd_ctl_ascii_elem_id_get snd_ctl_ascii_elem_id_get_dylibloader_wrapper_asound
#define snd_ctl_ascii_elem_id_parse snd_ctl_ascii_elem_id_parse_dylibloader_wrapper_asound
#define snd_ctl_ascii_value_parse snd_ctl_ascii_value_parse_dylibloader_wrapper_asound
#define snd_ctl_elem_id_sizeof snd_ctl_elem_id_sizeof_dylibloader_wrapper_asound
#define snd_ctl_elem_id_malloc snd_ctl_elem_id_malloc_dylibloader_wrapper_asound
#define snd_ctl_elem_id_free snd_ctl_elem_id_free_dylibloader_wrapper_asound
#define snd_ctl_elem_id_clear snd_ctl_elem_id_clear_dylibloader_wrapper_asound
#define snd_ctl_elem_id_copy snd_ctl_elem_id_copy_dylibloader_wrapper_asound
#define snd_ctl_elem_id_get_numid snd_ctl_elem_id_get_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_id_get_interface snd_ctl_elem_id_get_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_id_get_device snd_ctl_elem_id_get_device_dylibloader_wrapper_asound
#define snd_ctl_elem_id_get_subdevice snd_ctl_elem_id_get_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_id_get_name snd_ctl_elem_id_get_name_dylibloader_wrapper_asound
#define snd_ctl_elem_id_get_index snd_ctl_elem_id_get_index_dylibloader_wrapper_asound
#define snd_ctl_elem_id_set_numid snd_ctl_elem_id_set_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_id_set_interface snd_ctl_elem_id_set_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_id_set_device snd_ctl_elem_id_set_device_dylibloader_wrapper_asound
#define snd_ctl_elem_id_set_subdevice snd_ctl_elem_id_set_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_id_set_name snd_ctl_elem_id_set_name_dylibloader_wrapper_asound
#define snd_ctl_elem_id_set_index snd_ctl_elem_id_set_index_dylibloader_wrapper_asound
#define snd_ctl_card_info_sizeof snd_ctl_card_info_sizeof_dylibloader_wrapper_asound
#define snd_ctl_card_info_malloc snd_ctl_card_info_malloc_dylibloader_wrapper_asound
#define snd_ctl_card_info_free snd_ctl_card_info_free_dylibloader_wrapper_asound
#define snd_ctl_card_info_clear snd_ctl_card_info_clear_dylibloader_wrapper_asound
#define snd_ctl_card_info_copy snd_ctl_card_info_copy_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_card snd_ctl_card_info_get_card_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_id snd_ctl_card_info_get_id_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_driver snd_ctl_card_info_get_driver_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_name snd_ctl_card_info_get_name_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_longname snd_ctl_card_info_get_longname_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_mixername snd_ctl_card_info_get_mixername_dylibloader_wrapper_asound
#define snd_ctl_card_info_get_components snd_ctl_card_info_get_components_dylibloader_wrapper_asound
#define snd_ctl_event_sizeof snd_ctl_event_sizeof_dylibloader_wrapper_asound
#define snd_ctl_event_malloc snd_ctl_event_malloc_dylibloader_wrapper_asound
#define snd_ctl_event_free snd_ctl_event_free_dylibloader_wrapper_asound
#define snd_ctl_event_clear snd_ctl_event_clear_dylibloader_wrapper_asound
#define snd_ctl_event_copy snd_ctl_event_copy_dylibloader_wrapper_asound
#define snd_ctl_event_get_type snd_ctl_event_get_type_dylibloader_wrapper_asound
#define snd_ctl_elem_list_sizeof snd_ctl_elem_list_sizeof_dylibloader_wrapper_asound
#define snd_ctl_elem_list_malloc snd_ctl_elem_list_malloc_dylibloader_wrapper_asound
#define snd_ctl_elem_list_free snd_ctl_elem_list_free_dylibloader_wrapper_asound
#define snd_ctl_elem_list_clear snd_ctl_elem_list_clear_dylibloader_wrapper_asound
#define snd_ctl_elem_list_copy snd_ctl_elem_list_copy_dylibloader_wrapper_asound
#define snd_ctl_elem_list_set_offset snd_ctl_elem_list_set_offset_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_used snd_ctl_elem_list_get_used_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_count snd_ctl_elem_list_get_count_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_id snd_ctl_elem_list_get_id_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_numid snd_ctl_elem_list_get_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_interface snd_ctl_elem_list_get_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_device snd_ctl_elem_list_get_device_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_subdevice snd_ctl_elem_list_get_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_name snd_ctl_elem_list_get_name_dylibloader_wrapper_asound
#define snd_ctl_elem_list_get_index snd_ctl_elem_list_get_index_dylibloader_wrapper_asound
#define snd_ctl_elem_info_sizeof snd_ctl_elem_info_sizeof_dylibloader_wrapper_asound
#define snd_ctl_elem_info_malloc snd_ctl_elem_info_malloc_dylibloader_wrapper_asound
#define snd_ctl_elem_info_free snd_ctl_elem_info_free_dylibloader_wrapper_asound
#define snd_ctl_elem_info_clear snd_ctl_elem_info_clear_dylibloader_wrapper_asound
#define snd_ctl_elem_info_copy snd_ctl_elem_info_copy_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_type snd_ctl_elem_info_get_type_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_readable snd_ctl_elem_info_is_readable_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_writable snd_ctl_elem_info_is_writable_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_volatile snd_ctl_elem_info_is_volatile_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_inactive snd_ctl_elem_info_is_inactive_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_locked snd_ctl_elem_info_is_locked_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_tlv_readable snd_ctl_elem_info_is_tlv_readable_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_tlv_writable snd_ctl_elem_info_is_tlv_writable_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_tlv_commandable snd_ctl_elem_info_is_tlv_commandable_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_owner snd_ctl_elem_info_is_owner_dylibloader_wrapper_asound
#define snd_ctl_elem_info_is_user snd_ctl_elem_info_is_user_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_owner snd_ctl_elem_info_get_owner_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_count snd_ctl_elem_info_get_count_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_min snd_ctl_elem_info_get_min_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_max snd_ctl_elem_info_get_max_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_step snd_ctl_elem_info_get_step_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_min64 snd_ctl_elem_info_get_min64_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_max64 snd_ctl_elem_info_get_max64_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_step64 snd_ctl_elem_info_get_step64_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_items snd_ctl_elem_info_get_items_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_item snd_ctl_elem_info_set_item_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_item_name snd_ctl_elem_info_get_item_name_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_dimensions snd_ctl_elem_info_get_dimensions_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_dimension snd_ctl_elem_info_get_dimension_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_dimension snd_ctl_elem_info_set_dimension_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_id snd_ctl_elem_info_get_id_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_numid snd_ctl_elem_info_get_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_interface snd_ctl_elem_info_get_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_device snd_ctl_elem_info_get_device_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_subdevice snd_ctl_elem_info_get_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_name snd_ctl_elem_info_get_name_dylibloader_wrapper_asound
#define snd_ctl_elem_info_get_index snd_ctl_elem_info_get_index_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_id snd_ctl_elem_info_set_id_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_numid snd_ctl_elem_info_set_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_interface snd_ctl_elem_info_set_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_device snd_ctl_elem_info_set_device_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_subdevice snd_ctl_elem_info_set_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_name snd_ctl_elem_info_set_name_dylibloader_wrapper_asound
#define snd_ctl_elem_info_set_index snd_ctl_elem_info_set_index_dylibloader_wrapper_asound
#define snd_ctl_add_integer_elem_set snd_ctl_add_integer_elem_set_dylibloader_wrapper_asound
#define snd_ctl_add_integer64_elem_set snd_ctl_add_integer64_elem_set_dylibloader_wrapper_asound
#define snd_ctl_add_boolean_elem_set snd_ctl_add_boolean_elem_set_dylibloader_wrapper_asound
#define snd_ctl_add_enumerated_elem_set snd_ctl_add_enumerated_elem_set_dylibloader_wrapper_asound
#define snd_ctl_add_bytes_elem_set snd_ctl_add_bytes_elem_set_dylibloader_wrapper_asound
#define snd_ctl_elem_add_integer snd_ctl_elem_add_integer_dylibloader_wrapper_asound
#define snd_ctl_elem_add_integer64 snd_ctl_elem_add_integer64_dylibloader_wrapper_asound
#define snd_ctl_elem_add_boolean snd_ctl_elem_add_boolean_dylibloader_wrapper_asound
#define snd_ctl_elem_add_enumerated snd_ctl_elem_add_enumerated_dylibloader_wrapper_asound
#define snd_ctl_elem_add_iec958 snd_ctl_elem_add_iec958_dylibloader_wrapper_asound
#define snd_ctl_elem_remove snd_ctl_elem_remove_dylibloader_wrapper_asound
#define snd_ctl_elem_value_sizeof snd_ctl_elem_value_sizeof_dylibloader_wrapper_asound
#define snd_ctl_elem_value_malloc snd_ctl_elem_value_malloc_dylibloader_wrapper_asound
#define snd_ctl_elem_value_free snd_ctl_elem_value_free_dylibloader_wrapper_asound
#define snd_ctl_elem_value_clear snd_ctl_elem_value_clear_dylibloader_wrapper_asound
#define snd_ctl_elem_value_copy snd_ctl_elem_value_copy_dylibloader_wrapper_asound
#define snd_ctl_elem_value_compare snd_ctl_elem_value_compare_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_id snd_ctl_elem_value_get_id_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_numid snd_ctl_elem_value_get_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_interface snd_ctl_elem_value_get_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_device snd_ctl_elem_value_get_device_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_subdevice snd_ctl_elem_value_get_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_name snd_ctl_elem_value_get_name_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_index snd_ctl_elem_value_get_index_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_id snd_ctl_elem_value_set_id_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_numid snd_ctl_elem_value_set_numid_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_interface snd_ctl_elem_value_set_interface_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_device snd_ctl_elem_value_set_device_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_subdevice snd_ctl_elem_value_set_subdevice_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_name snd_ctl_elem_value_set_name_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_index snd_ctl_elem_value_set_index_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_boolean snd_ctl_elem_value_get_boolean_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_integer snd_ctl_elem_value_get_integer_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_integer64 snd_ctl_elem_value_get_integer64_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_enumerated snd_ctl_elem_value_get_enumerated_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_byte snd_ctl_elem_value_get_byte_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_boolean snd_ctl_elem_value_set_boolean_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_integer snd_ctl_elem_value_set_integer_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_integer64 snd_ctl_elem_value_set_integer64_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_enumerated snd_ctl_elem_value_set_enumerated_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_byte snd_ctl_elem_value_set_byte_dylibloader_wrapper_asound
#define snd_ctl_elem_set_bytes snd_ctl_elem_set_bytes_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_bytes snd_ctl_elem_value_get_bytes_dylibloader_wrapper_asound
#define snd_ctl_elem_value_get_iec958 snd_ctl_elem_value_get_iec958_dylibloader_wrapper_asound
#define snd_ctl_elem_value_set_iec958 snd_ctl_elem_value_set_iec958_dylibloader_wrapper_asound
#define snd_tlv_parse_dB_info snd_tlv_parse_dB_info_dylibloader_wrapper_asound
#define snd_tlv_get_dB_range snd_tlv_get_dB_range_dylibloader_wrapper_asound
#define snd_tlv_convert_to_dB snd_tlv_convert_to_dB_dylibloader_wrapper_asound
#define snd_tlv_convert_from_dB snd_tlv_convert_from_dB_dylibloader_wrapper_asound
#define snd_ctl_get_dB_range snd_ctl_get_dB_range_dylibloader_wrapper_asound
#define snd_ctl_convert_to_dB snd_ctl_convert_to_dB_dylibloader_wrapper_asound
#define snd_ctl_convert_from_dB snd_ctl_convert_from_dB_dylibloader_wrapper_asound
#define snd_hctl_compare_fast snd_hctl_compare_fast_dylibloader_wrapper_asound
#define snd_hctl_open snd_hctl_open_dylibloader_wrapper_asound
#define snd_hctl_open_ctl snd_hctl_open_ctl_dylibloader_wrapper_asound
#define snd_hctl_close snd_hctl_close_dylibloader_wrapper_asound
#define snd_hctl_nonblock snd_hctl_nonblock_dylibloader_wrapper_asound
#define snd_hctl_poll_descriptors_count snd_hctl_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_hctl_poll_descriptors snd_hctl_poll_descriptors_dylibloader_wrapper_asound
#define snd_hctl_poll_descriptors_revents snd_hctl_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_hctl_get_count snd_hctl_get_count_dylibloader_wrapper_asound
#define snd_hctl_set_compare snd_hctl_set_compare_dylibloader_wrapper_asound
#define snd_hctl_first_elem snd_hctl_first_elem_dylibloader_wrapper_asound
#define snd_hctl_last_elem snd_hctl_last_elem_dylibloader_wrapper_asound
#define snd_hctl_find_elem snd_hctl_find_elem_dylibloader_wrapper_asound
#define snd_hctl_set_callback snd_hctl_set_callback_dylibloader_wrapper_asound
#define snd_hctl_set_callback_private snd_hctl_set_callback_private_dylibloader_wrapper_asound
#define snd_hctl_get_callback_private snd_hctl_get_callback_private_dylibloader_wrapper_asound
#define snd_hctl_load snd_hctl_load_dylibloader_wrapper_asound
#define snd_hctl_free snd_hctl_free_dylibloader_wrapper_asound
#define snd_hctl_handle_events snd_hctl_handle_events_dylibloader_wrapper_asound
#define snd_hctl_name snd_hctl_name_dylibloader_wrapper_asound
#define snd_hctl_wait snd_hctl_wait_dylibloader_wrapper_asound
#define snd_hctl_ctl snd_hctl_ctl_dylibloader_wrapper_asound
#define snd_hctl_elem_next snd_hctl_elem_next_dylibloader_wrapper_asound
#define snd_hctl_elem_prev snd_hctl_elem_prev_dylibloader_wrapper_asound
#define snd_hctl_elem_info snd_hctl_elem_info_dylibloader_wrapper_asound
#define snd_hctl_elem_read snd_hctl_elem_read_dylibloader_wrapper_asound
#define snd_hctl_elem_write snd_hctl_elem_write_dylibloader_wrapper_asound
#define snd_hctl_elem_tlv_read snd_hctl_elem_tlv_read_dylibloader_wrapper_asound
#define snd_hctl_elem_tlv_write snd_hctl_elem_tlv_write_dylibloader_wrapper_asound
#define snd_hctl_elem_tlv_command snd_hctl_elem_tlv_command_dylibloader_wrapper_asound
#define snd_hctl_elem_get_hctl snd_hctl_elem_get_hctl_dylibloader_wrapper_asound
#define snd_hctl_elem_get_id snd_hctl_elem_get_id_dylibloader_wrapper_asound
#define snd_hctl_elem_get_numid snd_hctl_elem_get_numid_dylibloader_wrapper_asound
#define snd_hctl_elem_get_interface snd_hctl_elem_get_interface_dylibloader_wrapper_asound
#define snd_hctl_elem_get_device snd_hctl_elem_get_device_dylibloader_wrapper_asound
#define snd_hctl_elem_get_subdevice snd_hctl_elem_get_subdevice_dylibloader_wrapper_asound
#define snd_hctl_elem_get_name snd_hctl_elem_get_name_dylibloader_wrapper_asound
#define snd_hctl_elem_get_index snd_hctl_elem_get_index_dylibloader_wrapper_asound
#define snd_hctl_elem_set_callback snd_hctl_elem_set_callback_dylibloader_wrapper_asound
#define snd_hctl_elem_get_callback_private snd_hctl_elem_get_callback_private_dylibloader_wrapper_asound
#define snd_hctl_elem_set_callback_private snd_hctl_elem_set_callback_private_dylibloader_wrapper_asound
#define snd_sctl_build snd_sctl_build_dylibloader_wrapper_asound
#define snd_sctl_free snd_sctl_free_dylibloader_wrapper_asound
#define snd_sctl_install snd_sctl_install_dylibloader_wrapper_asound
#define snd_sctl_remove snd_sctl_remove_dylibloader_wrapper_asound
#define snd_mixer_open snd_mixer_open_dylibloader_wrapper_asound
#define snd_mixer_close snd_mixer_close_dylibloader_wrapper_asound
#define snd_mixer_first_elem snd_mixer_first_elem_dylibloader_wrapper_asound
#define snd_mixer_last_elem snd_mixer_last_elem_dylibloader_wrapper_asound
#define snd_mixer_handle_events snd_mixer_handle_events_dylibloader_wrapper_asound
#define snd_mixer_attach snd_mixer_attach_dylibloader_wrapper_asound
#define snd_mixer_attach_hctl snd_mixer_attach_hctl_dylibloader_wrapper_asound
#define snd_mixer_detach snd_mixer_detach_dylibloader_wrapper_asound
#define snd_mixer_detach_hctl snd_mixer_detach_hctl_dylibloader_wrapper_asound
#define snd_mixer_get_hctl snd_mixer_get_hctl_dylibloader_wrapper_asound
#define snd_mixer_poll_descriptors_count snd_mixer_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_mixer_poll_descriptors snd_mixer_poll_descriptors_dylibloader_wrapper_asound
#define snd_mixer_poll_descriptors_revents snd_mixer_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_mixer_load snd_mixer_load_dylibloader_wrapper_asound
#define snd_mixer_free snd_mixer_free_dylibloader_wrapper_asound
#define snd_mixer_wait snd_mixer_wait_dylibloader_wrapper_asound
#define snd_mixer_set_compare snd_mixer_set_compare_dylibloader_wrapper_asound
#define snd_mixer_set_callback snd_mixer_set_callback_dylibloader_wrapper_asound
#define snd_mixer_get_callback_private snd_mixer_get_callback_private_dylibloader_wrapper_asound
#define snd_mixer_set_callback_private snd_mixer_set_callback_private_dylibloader_wrapper_asound
#define snd_mixer_get_count snd_mixer_get_count_dylibloader_wrapper_asound
#define snd_mixer_class_unregister snd_mixer_class_unregister_dylibloader_wrapper_asound
#define snd_mixer_elem_next snd_mixer_elem_next_dylibloader_wrapper_asound
#define snd_mixer_elem_prev snd_mixer_elem_prev_dylibloader_wrapper_asound
#define snd_mixer_elem_set_callback snd_mixer_elem_set_callback_dylibloader_wrapper_asound
#define snd_mixer_elem_get_callback_private snd_mixer_elem_get_callback_private_dylibloader_wrapper_asound
#define snd_mixer_elem_set_callback_private snd_mixer_elem_set_callback_private_dylibloader_wrapper_asound
#define snd_mixer_elem_get_type snd_mixer_elem_get_type_dylibloader_wrapper_asound
#define snd_mixer_class_register snd_mixer_class_register_dylibloader_wrapper_asound
#define snd_mixer_elem_new snd_mixer_elem_new_dylibloader_wrapper_asound
#define snd_mixer_elem_add snd_mixer_elem_add_dylibloader_wrapper_asound
#define snd_mixer_elem_remove snd_mixer_elem_remove_dylibloader_wrapper_asound
#define snd_mixer_elem_free snd_mixer_elem_free_dylibloader_wrapper_asound
#define snd_mixer_elem_info snd_mixer_elem_info_dylibloader_wrapper_asound
#define snd_mixer_elem_value snd_mixer_elem_value_dylibloader_wrapper_asound
#define snd_mixer_elem_attach snd_mixer_elem_attach_dylibloader_wrapper_asound
#define snd_mixer_elem_detach snd_mixer_elem_detach_dylibloader_wrapper_asound
#define snd_mixer_elem_empty snd_mixer_elem_empty_dylibloader_wrapper_asound
#define snd_mixer_elem_get_private snd_mixer_elem_get_private_dylibloader_wrapper_asound
#define snd_mixer_class_sizeof snd_mixer_class_sizeof_dylibloader_wrapper_asound
#define snd_mixer_class_malloc snd_mixer_class_malloc_dylibloader_wrapper_asound
#define snd_mixer_class_free snd_mixer_class_free_dylibloader_wrapper_asound
#define snd_mixer_class_copy snd_mixer_class_copy_dylibloader_wrapper_asound
#define snd_mixer_class_get_mixer snd_mixer_class_get_mixer_dylibloader_wrapper_asound
#define snd_mixer_class_get_event snd_mixer_class_get_event_dylibloader_wrapper_asound
#define snd_mixer_class_get_private snd_mixer_class_get_private_dylibloader_wrapper_asound
#define snd_mixer_class_get_compare snd_mixer_class_get_compare_dylibloader_wrapper_asound
#define snd_mixer_class_set_event snd_mixer_class_set_event_dylibloader_wrapper_asound
#define snd_mixer_class_set_private snd_mixer_class_set_private_dylibloader_wrapper_asound
#define snd_mixer_class_set_private_free snd_mixer_class_set_private_free_dylibloader_wrapper_asound
#define snd_mixer_class_set_compare snd_mixer_class_set_compare_dylibloader_wrapper_asound
#define snd_mixer_selem_channel_name snd_mixer_selem_channel_name_dylibloader_wrapper_asound
#define snd_mixer_selem_register snd_mixer_selem_register_dylibloader_wrapper_asound
#define snd_mixer_selem_get_id snd_mixer_selem_get_id_dylibloader_wrapper_asound
#define snd_mixer_selem_get_name snd_mixer_selem_get_name_dylibloader_wrapper_asound
#define snd_mixer_selem_get_index snd_mixer_selem_get_index_dylibloader_wrapper_asound
#define snd_mixer_find_selem snd_mixer_find_selem_dylibloader_wrapper_asound
#define snd_mixer_selem_is_active snd_mixer_selem_is_active_dylibloader_wrapper_asound
#define snd_mixer_selem_is_playback_mono snd_mixer_selem_is_playback_mono_dylibloader_wrapper_asound
#define snd_mixer_selem_has_playback_channel snd_mixer_selem_has_playback_channel_dylibloader_wrapper_asound
#define snd_mixer_selem_is_capture_mono snd_mixer_selem_is_capture_mono_dylibloader_wrapper_asound
#define snd_mixer_selem_has_capture_channel snd_mixer_selem_has_capture_channel_dylibloader_wrapper_asound
#define snd_mixer_selem_get_capture_group snd_mixer_selem_get_capture_group_dylibloader_wrapper_asound
#define snd_mixer_selem_has_common_volume snd_mixer_selem_has_common_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_has_playback_volume snd_mixer_selem_has_playback_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_has_playback_volume_joined snd_mixer_selem_has_playback_volume_joined_dylibloader_wrapper_asound
#define snd_mixer_selem_has_capture_volume snd_mixer_selem_has_capture_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_has_capture_volume_joined snd_mixer_selem_has_capture_volume_joined_dylibloader_wrapper_asound
#define snd_mixer_selem_has_common_switch snd_mixer_selem_has_common_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_has_playback_switch snd_mixer_selem_has_playback_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_has_playback_switch_joined snd_mixer_selem_has_playback_switch_joined_dylibloader_wrapper_asound
#define snd_mixer_selem_has_capture_switch snd_mixer_selem_has_capture_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_has_capture_switch_joined snd_mixer_selem_has_capture_switch_joined_dylibloader_wrapper_asound
#define snd_mixer_selem_has_capture_switch_exclusive snd_mixer_selem_has_capture_switch_exclusive_dylibloader_wrapper_asound
#define snd_mixer_selem_ask_playback_vol_dB snd_mixer_selem_ask_playback_vol_dB_dylibloader_wrapper_asound
#define snd_mixer_selem_ask_capture_vol_dB snd_mixer_selem_ask_capture_vol_dB_dylibloader_wrapper_asound
#define snd_mixer_selem_ask_playback_dB_vol snd_mixer_selem_ask_playback_dB_vol_dylibloader_wrapper_asound
#define snd_mixer_selem_ask_capture_dB_vol snd_mixer_selem_ask_capture_dB_vol_dylibloader_wrapper_asound
#define snd_mixer_selem_get_playback_volume snd_mixer_selem_get_playback_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_get_capture_volume snd_mixer_selem_get_capture_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_get_playback_dB snd_mixer_selem_get_playback_dB_dylibloader_wrapper_asound
#define snd_mixer_selem_get_capture_dB snd_mixer_selem_get_capture_dB_dylibloader_wrapper_asound
#define snd_mixer_selem_get_playback_switch snd_mixer_selem_get_playback_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_get_capture_switch snd_mixer_selem_get_capture_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_volume snd_mixer_selem_set_playback_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_volume snd_mixer_selem_set_capture_volume_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_dB snd_mixer_selem_set_playback_dB_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_dB snd_mixer_selem_set_capture_dB_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_volume_all snd_mixer_selem_set_playback_volume_all_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_volume_all snd_mixer_selem_set_capture_volume_all_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_dB_all snd_mixer_selem_set_playback_dB_all_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_dB_all snd_mixer_selem_set_capture_dB_all_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_switch snd_mixer_selem_set_playback_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_switch snd_mixer_selem_set_capture_switch_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_switch_all snd_mixer_selem_set_playback_switch_all_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_switch_all snd_mixer_selem_set_capture_switch_all_dylibloader_wrapper_asound
#define snd_mixer_selem_get_playback_volume_range snd_mixer_selem_get_playback_volume_range_dylibloader_wrapper_asound
#define snd_mixer_selem_get_playback_dB_range snd_mixer_selem_get_playback_dB_range_dylibloader_wrapper_asound
#define snd_mixer_selem_set_playback_volume_range snd_mixer_selem_set_playback_volume_range_dylibloader_wrapper_asound
#define snd_mixer_selem_get_capture_volume_range snd_mixer_selem_get_capture_volume_range_dylibloader_wrapper_asound
#define snd_mixer_selem_get_capture_dB_range snd_mixer_selem_get_capture_dB_range_dylibloader_wrapper_asound
#define snd_mixer_selem_set_capture_volume_range snd_mixer_selem_set_capture_volume_range_dylibloader_wrapper_asound
#define snd_mixer_selem_is_enumerated snd_mixer_selem_is_enumerated_dylibloader_wrapper_asound
#define snd_mixer_selem_is_enum_playback snd_mixer_selem_is_enum_playback_dylibloader_wrapper_asound
#define snd_mixer_selem_is_enum_capture snd_mixer_selem_is_enum_capture_dylibloader_wrapper_asound
#define snd_mixer_selem_get_enum_items snd_mixer_selem_get_enum_items_dylibloader_wrapper_asound
#define snd_mixer_selem_get_enum_item_name snd_mixer_selem_get_enum_item_name_dylibloader_wrapper_asound
#define snd_mixer_selem_get_enum_item snd_mixer_selem_get_enum_item_dylibloader_wrapper_asound
#define snd_mixer_selem_set_enum_item snd_mixer_selem_set_enum_item_dylibloader_wrapper_asound
#define snd_mixer_selem_id_sizeof snd_mixer_selem_id_sizeof_dylibloader_wrapper_asound
#define snd_mixer_selem_id_malloc snd_mixer_selem_id_malloc_dylibloader_wrapper_asound
#define snd_mixer_selem_id_free snd_mixer_selem_id_free_dylibloader_wrapper_asound
#define snd_mixer_selem_id_copy snd_mixer_selem_id_copy_dylibloader_wrapper_asound
#define snd_mixer_selem_id_get_name snd_mixer_selem_id_get_name_dylibloader_wrapper_asound
#define snd_mixer_selem_id_get_index snd_mixer_selem_id_get_index_dylibloader_wrapper_asound
#define snd_mixer_selem_id_set_name snd_mixer_selem_id_set_name_dylibloader_wrapper_asound
#define snd_mixer_selem_id_set_index snd_mixer_selem_id_set_index_dylibloader_wrapper_asound
#define snd_seq_open snd_seq_open_dylibloader_wrapper_asound
#define snd_seq_open_lconf snd_seq_open_lconf_dylibloader_wrapper_asound
#define snd_seq_name snd_seq_name_dylibloader_wrapper_asound
#define snd_seq_type snd_seq_type_dylibloader_wrapper_asound
#define snd_seq_close snd_seq_close_dylibloader_wrapper_asound
#define snd_seq_poll_descriptors_count snd_seq_poll_descriptors_count_dylibloader_wrapper_asound
#define snd_seq_poll_descriptors snd_seq_poll_descriptors_dylibloader_wrapper_asound
#define snd_seq_poll_descriptors_revents snd_seq_poll_descriptors_revents_dylibloader_wrapper_asound
#define snd_seq_nonblock snd_seq_nonblock_dylibloader_wrapper_asound
#define snd_seq_client_id snd_seq_client_id_dylibloader_wrapper_asound
#define snd_seq_get_output_buffer_size snd_seq_get_output_buffer_size_dylibloader_wrapper_asound
#define snd_seq_get_input_buffer_size snd_seq_get_input_buffer_size_dylibloader_wrapper_asound
#define snd_seq_set_output_buffer_size snd_seq_set_output_buffer_size_dylibloader_wrapper_asound
#define snd_seq_set_input_buffer_size snd_seq_set_input_buffer_size_dylibloader_wrapper_asound
#define snd_seq_system_info_sizeof snd_seq_system_info_sizeof_dylibloader_wrapper_asound
#define snd_seq_system_info_malloc snd_seq_system_info_malloc_dylibloader_wrapper_asound
#define snd_seq_system_info_free snd_seq_system_info_free_dylibloader_wrapper_asound
#define snd_seq_system_info_copy snd_seq_system_info_copy_dylibloader_wrapper_asound
#define snd_seq_system_info_get_queues snd_seq_system_info_get_queues_dylibloader_wrapper_asound
#define snd_seq_system_info_get_clients snd_seq_system_info_get_clients_dylibloader_wrapper_asound
#define snd_seq_system_info_get_ports snd_seq_system_info_get_ports_dylibloader_wrapper_asound
#define snd_seq_system_info_get_channels snd_seq_system_info_get_channels_dylibloader_wrapper_asound
#define snd_seq_system_info_get_cur_clients snd_seq_system_info_get_cur_clients_dylibloader_wrapper_asound
#define snd_seq_system_info_get_cur_queues snd_seq_system_info_get_cur_queues_dylibloader_wrapper_asound
#define snd_seq_system_info snd_seq_system_info_dylibloader_wrapper_asound
#define snd_seq_client_info_sizeof snd_seq_client_info_sizeof_dylibloader_wrapper_asound
#define snd_seq_client_info_malloc snd_seq_client_info_malloc_dylibloader_wrapper_asound
#define snd_seq_client_info_free snd_seq_client_info_free_dylibloader_wrapper_asound
#define snd_seq_client_info_copy snd_seq_client_info_copy_dylibloader_wrapper_asound
#define snd_seq_client_info_get_client snd_seq_client_info_get_client_dylibloader_wrapper_asound
#define snd_seq_client_info_get_type snd_seq_client_info_get_type_dylibloader_wrapper_asound
#define snd_seq_client_info_get_name snd_seq_client_info_get_name_dylibloader_wrapper_asound
#define snd_seq_client_info_get_broadcast_filter snd_seq_client_info_get_broadcast_filter_dylibloader_wrapper_asound
#define snd_seq_client_info_get_error_bounce snd_seq_client_info_get_error_bounce_dylibloader_wrapper_asound
#define snd_seq_client_info_get_card snd_seq_client_info_get_card_dylibloader_wrapper_asound
#define snd_seq_client_info_get_pid snd_seq_client_info_get_pid_dylibloader_wrapper_asound
#define snd_seq_client_info_get_event_filter snd_seq_client_info_get_event_filter_dylibloader_wrapper_asound
#define snd_seq_client_info_get_num_ports snd_seq_client_info_get_num_ports_dylibloader_wrapper_asound
#define snd_seq_client_info_get_event_lost snd_seq_client_info_get_event_lost_dylibloader_wrapper_asound
#define snd_seq_client_info_set_client snd_seq_client_info_set_client_dylibloader_wrapper_asound
#define snd_seq_client_info_set_name snd_seq_client_info_set_name_dylibloader_wrapper_asound
#define snd_seq_client_info_set_broadcast_filter snd_seq_client_info_set_broadcast_filter_dylibloader_wrapper_asound
#define snd_seq_client_info_set_error_bounce snd_seq_client_info_set_error_bounce_dylibloader_wrapper_asound
#define snd_seq_client_info_set_event_filter snd_seq_client_info_set_event_filter_dylibloader_wrapper_asound
#define snd_seq_client_info_event_filter_clear snd_seq_client_info_event_filter_clear_dylibloader_wrapper_asound
#define snd_seq_client_info_event_filter_add snd_seq_client_info_event_filter_add_dylibloader_wrapper_asound
#define snd_seq_client_info_event_filter_del snd_seq_client_info_event_filter_del_dylibloader_wrapper_asound
#define snd_seq_client_info_event_filter_check snd_seq_client_info_event_filter_check_dylibloader_wrapper_asound
#define snd_seq_get_client_info snd_seq_get_client_info_dylibloader_wrapper_asound
#define snd_seq_get_any_client_info snd_seq_get_any_client_info_dylibloader_wrapper_asound
#define snd_seq_set_client_info snd_seq_set_client_info_dylibloader_wrapper_asound
#define snd_seq_query_next_client snd_seq_query_next_client_dylibloader_wrapper_asound
#define snd_seq_client_pool_sizeof snd_seq_client_pool_sizeof_dylibloader_wrapper_asound
#define snd_seq_client_pool_malloc snd_seq_client_pool_malloc_dylibloader_wrapper_asound
#define snd_seq_client_pool_free snd_seq_client_pool_free_dylibloader_wrapper_asound
#define snd_seq_client_pool_copy snd_seq_client_pool_copy_dylibloader_wrapper_asound
#define snd_seq_client_pool_get_client snd_seq_client_pool_get_client_dylibloader_wrapper_asound
#define snd_seq_client_pool_get_output_pool snd_seq_client_pool_get_output_pool_dylibloader_wrapper_asound
#define snd_seq_client_pool_get_input_pool snd_seq_client_pool_get_input_pool_dylibloader_wrapper_asound
#define snd_seq_client_pool_get_output_room snd_seq_client_pool_get_output_room_dylibloader_wrapper_asound
#define snd_seq_client_pool_get_output_free snd_seq_client_pool_get_output_free_dylibloader_wrapper_asound
#define snd_seq_client_pool_get_input_free snd_seq_client_pool_get_input_free_dylibloader_wrapper_asound
#define snd_seq_client_pool_set_output_pool snd_seq_client_pool_set_output_pool_dylibloader_wrapper_asound
#define snd_seq_client_pool_set_input_pool snd_seq_client_pool_set_input_pool_dylibloader_wrapper_asound
#define snd_seq_client_pool_set_output_room snd_seq_client_pool_set_output_room_dylibloader_wrapper_asound
#define snd_seq_get_client_pool snd_seq_get_client_pool_dylibloader_wrapper_asound
#define snd_seq_set_client_pool snd_seq_set_client_pool_dylibloader_wrapper_asound
#define snd_seq_port_info_sizeof snd_seq_port_info_sizeof_dylibloader_wrapper_asound
#define snd_seq_port_info_malloc snd_seq_port_info_malloc_dylibloader_wrapper_asound
#define snd_seq_port_info_free snd_seq_port_info_free_dylibloader_wrapper_asound
#define snd_seq_port_info_copy snd_seq_port_info_copy_dylibloader_wrapper_asound
#define snd_seq_port_info_get_client snd_seq_port_info_get_client_dylibloader_wrapper_asound
#define snd_seq_port_info_get_port snd_seq_port_info_get_port_dylibloader_wrapper_asound
#define snd_seq_port_info_get_addr snd_seq_port_info_get_addr_dylibloader_wrapper_asound
#define snd_seq_port_info_get_name snd_seq_port_info_get_name_dylibloader_wrapper_asound
#define snd_seq_port_info_get_capability snd_seq_port_info_get_capability_dylibloader_wrapper_asound
#define snd_seq_port_info_get_type snd_seq_port_info_get_type_dylibloader_wrapper_asound
#define snd_seq_port_info_get_midi_channels snd_seq_port_info_get_midi_channels_dylibloader_wrapper_asound
#define snd_seq_port_info_get_midi_voices snd_seq_port_info_get_midi_voices_dylibloader_wrapper_asound
#define snd_seq_port_info_get_synth_voices snd_seq_port_info_get_synth_voices_dylibloader_wrapper_asound
#define snd_seq_port_info_get_read_use snd_seq_port_info_get_read_use_dylibloader_wrapper_asound
#define snd_seq_port_info_get_write_use snd_seq_port_info_get_write_use_dylibloader_wrapper_asound
#define snd_seq_port_info_get_port_specified snd_seq_port_info_get_port_specified_dylibloader_wrapper_asound
#define snd_seq_port_info_get_timestamping snd_seq_port_info_get_timestamping_dylibloader_wrapper_asound
#define snd_seq_port_info_get_timestamp_real snd_seq_port_info_get_timestamp_real_dylibloader_wrapper_asound
#define snd_seq_port_info_get_timestamp_queue snd_seq_port_info_get_timestamp_queue_dylibloader_wrapper_asound
#define snd_seq_port_info_set_client snd_seq_port_info_set_client_dylibloader_wrapper_asound
#define snd_seq_port_info_set_port snd_seq_port_info_set_port_dylibloader_wrapper_asound
#define snd_seq_port_info_set_addr snd_seq_port_info_set_addr_dylibloader_wrapper_asound
#define snd_seq_port_info_set_name snd_seq_port_info_set_name_dylibloader_wrapper_asound
#define snd_seq_port_info_set_capability snd_seq_port_info_set_capability_dylibloader_wrapper_asound
#define snd_seq_port_info_set_type snd_seq_port_info_set_type_dylibloader_wrapper_asound
#define snd_seq_port_info_set_midi_channels snd_seq_port_info_set_midi_channels_dylibloader_wrapper_asound
#define snd_seq_port_info_set_midi_voices snd_seq_port_info_set_midi_voices_dylibloader_wrapper_asound
#define snd_seq_port_info_set_synth_voices snd_seq_port_info_set_synth_voices_dylibloader_wrapper_asound
#define snd_seq_port_info_set_port_specified snd_seq_port_info_set_port_specified_dylibloader_wrapper_asound
#define snd_seq_port_info_set_timestamping snd_seq_port_info_set_timestamping_dylibloader_wrapper_asound
#define snd_seq_port_info_set_timestamp_real snd_seq_port_info_set_timestamp_real_dylibloader_wrapper_asound
#define snd_seq_port_info_set_timestamp_queue snd_seq_port_info_set_timestamp_queue_dylibloader_wrapper_asound
#define snd_seq_create_port snd_seq_create_port_dylibloader_wrapper_asound
#define snd_seq_delete_port snd_seq_delete_port_dylibloader_wrapper_asound
#define snd_seq_get_port_info snd_seq_get_port_info_dylibloader_wrapper_asound
#define snd_seq_get_any_port_info snd_seq_get_any_port_info_dylibloader_wrapper_asound
#define snd_seq_set_port_info snd_seq_set_port_info_dylibloader_wrapper_asound
#define snd_seq_query_next_port snd_seq_query_next_port_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_sizeof snd_seq_port_subscribe_sizeof_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_malloc snd_seq_port_subscribe_malloc_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_free snd_seq_port_subscribe_free_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_copy snd_seq_port_subscribe_copy_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_get_sender snd_seq_port_subscribe_get_sender_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_get_dest snd_seq_port_subscribe_get_dest_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_get_queue snd_seq_port_subscribe_get_queue_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_get_exclusive snd_seq_port_subscribe_get_exclusive_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_get_time_update snd_seq_port_subscribe_get_time_update_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_get_time_real snd_seq_port_subscribe_get_time_real_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_set_sender snd_seq_port_subscribe_set_sender_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_set_dest snd_seq_port_subscribe_set_dest_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_set_queue snd_seq_port_subscribe_set_queue_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_set_exclusive snd_seq_port_subscribe_set_exclusive_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_set_time_update snd_seq_port_subscribe_set_time_update_dylibloader_wrapper_asound
#define snd_seq_port_subscribe_set_time_real snd_seq_port_subscribe_set_time_real_dylibloader_wrapper_asound
#define snd_seq_get_port_subscription snd_seq_get_port_subscription_dylibloader_wrapper_asound
#define snd_seq_subscribe_port snd_seq_subscribe_port_dylibloader_wrapper_asound
#define snd_seq_unsubscribe_port snd_seq_unsubscribe_port_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_sizeof snd_seq_query_subscribe_sizeof_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_malloc snd_seq_query_subscribe_malloc_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_free snd_seq_query_subscribe_free_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_copy snd_seq_query_subscribe_copy_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_client snd_seq_query_subscribe_get_client_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_port snd_seq_query_subscribe_get_port_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_root snd_seq_query_subscribe_get_root_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_type snd_seq_query_subscribe_get_type_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_index snd_seq_query_subscribe_get_index_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_num_subs snd_seq_query_subscribe_get_num_subs_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_addr snd_seq_query_subscribe_get_addr_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_queue snd_seq_query_subscribe_get_queue_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_exclusive snd_seq_query_subscribe_get_exclusive_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_time_update snd_seq_query_subscribe_get_time_update_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_get_time_real snd_seq_query_subscribe_get_time_real_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_set_client snd_seq_query_subscribe_set_client_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_set_port snd_seq_query_subscribe_set_port_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_set_root snd_seq_query_subscribe_set_root_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_set_type snd_seq_query_subscribe_set_type_dylibloader_wrapper_asound
#define snd_seq_query_subscribe_set_index snd_seq_query_subscribe_set_index_dylibloader_wrapper_asound
#define snd_seq_query_port_subscribers snd_seq_query_port_subscribers_dylibloader_wrapper_asound
#define snd_seq_queue_info_sizeof snd_seq_queue_info_sizeof_dylibloader_wrapper_asound
#define snd_seq_queue_info_malloc snd_seq_queue_info_malloc_dylibloader_wrapper_asound
#define snd_seq_queue_info_free snd_seq_queue_info_free_dylibloader_wrapper_asound
#define snd_seq_queue_info_copy snd_seq_queue_info_copy_dylibloader_wrapper_asound
#define snd_seq_queue_info_get_queue snd_seq_queue_info_get_queue_dylibloader_wrapper_asound
#define snd_seq_queue_info_get_name snd_seq_queue_info_get_name_dylibloader_wrapper_asound
#define snd_seq_queue_info_get_owner snd_seq_queue_info_get_owner_dylibloader_wrapper_asound
#define snd_seq_queue_info_get_locked snd_seq_queue_info_get_locked_dylibloader_wrapper_asound
#define snd_seq_queue_info_get_flags snd_seq_queue_info_get_flags_dylibloader_wrapper_asound
#define snd_seq_queue_info_set_name snd_seq_queue_info_set_name_dylibloader_wrapper_asound
#define snd_seq_queue_info_set_owner snd_seq_queue_info_set_owner_dylibloader_wrapper_asound
#define snd_seq_queue_info_set_locked snd_seq_queue_info_set_locked_dylibloader_wrapper_asound
#define snd_seq_queue_info_set_flags snd_seq_queue_info_set_flags_dylibloader_wrapper_asound
#define snd_seq_create_queue snd_seq_create_queue_dylibloader_wrapper_asound
#define snd_seq_alloc_named_queue snd_seq_alloc_named_queue_dylibloader_wrapper_asound
#define snd_seq_alloc_queue snd_seq_alloc_queue_dylibloader_wrapper_asound
#define snd_seq_free_queue snd_seq_free_queue_dylibloader_wrapper_asound
#define snd_seq_get_queue_info snd_seq_get_queue_info_dylibloader_wrapper_asound
#define snd_seq_set_queue_info snd_seq_set_queue_info_dylibloader_wrapper_asound
#define snd_seq_query_named_queue snd_seq_query_named_queue_dylibloader_wrapper_asound
#define snd_seq_get_queue_usage snd_seq_get_queue_usage_dylibloader_wrapper_asound
#define snd_seq_set_queue_usage snd_seq_set_queue_usage_dylibloader_wrapper_asound
#define snd_seq_queue_status_sizeof snd_seq_queue_status_sizeof_dylibloader_wrapper_asound
#define snd_seq_queue_status_malloc snd_seq_queue_status_malloc_dylibloader_wrapper_asound
#define snd_seq_queue_status_free snd_seq_queue_status_free_dylibloader_wrapper_asound
#define snd_seq_queue_status_copy snd_seq_queue_status_copy_dylibloader_wrapper_asound
#define snd_seq_queue_status_get_queue snd_seq_queue_status_get_queue_dylibloader_wrapper_asound
#define snd_seq_queue_status_get_events snd_seq_queue_status_get_events_dylibloader_wrapper_asound
#define snd_seq_queue_status_get_tick_time snd_seq_queue_status_get_tick_time_dylibloader_wrapper_asound
#define snd_seq_queue_status_get_real_time snd_seq_queue_status_get_real_time_dylibloader_wrapper_asound
#define snd_seq_queue_status_get_status snd_seq_queue_status_get_status_dylibloader_wrapper_asound
#define snd_seq_get_queue_status snd_seq_get_queue_status_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_sizeof snd_seq_queue_tempo_sizeof_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_malloc snd_seq_queue_tempo_malloc_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_free snd_seq_queue_tempo_free_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_copy snd_seq_queue_tempo_copy_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_get_queue snd_seq_queue_tempo_get_queue_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_get_tempo snd_seq_queue_tempo_get_tempo_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_get_ppq snd_seq_queue_tempo_get_ppq_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_get_skew snd_seq_queue_tempo_get_skew_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_get_skew_base snd_seq_queue_tempo_get_skew_base_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_set_tempo snd_seq_queue_tempo_set_tempo_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_set_ppq snd_seq_queue_tempo_set_ppq_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_set_skew snd_seq_queue_tempo_set_skew_dylibloader_wrapper_asound
#define snd_seq_queue_tempo_set_skew_base snd_seq_queue_tempo_set_skew_base_dylibloader_wrapper_asound
#define snd_seq_get_queue_tempo snd_seq_get_queue_tempo_dylibloader_wrapper_asound
#define snd_seq_set_queue_tempo snd_seq_set_queue_tempo_dylibloader_wrapper_asound
#define snd_seq_queue_timer_sizeof snd_seq_queue_timer_sizeof_dylibloader_wrapper_asound
#define snd_seq_queue_timer_malloc snd_seq_queue_timer_malloc_dylibloader_wrapper_asound
#define snd_seq_queue_timer_free snd_seq_queue_timer_free_dylibloader_wrapper_asound
#define snd_seq_queue_timer_copy snd_seq_queue_timer_copy_dylibloader_wrapper_asound
#define snd_seq_queue_timer_get_queue snd_seq_queue_timer_get_queue_dylibloader_wrapper_asound
#define snd_seq_queue_timer_get_type snd_seq_queue_timer_get_type_dylibloader_wrapper_asound
#define snd_seq_queue_timer_get_id snd_seq_queue_timer_get_id_dylibloader_wrapper_asound
#define snd_seq_queue_timer_get_resolution snd_seq_queue_timer_get_resolution_dylibloader_wrapper_asound
#define snd_seq_queue_timer_set_type snd_seq_queue_timer_set_type_dylibloader_wrapper_asound
#define snd_seq_queue_timer_set_id snd_seq_queue_timer_set_id_dylibloader_wrapper_asound
#define snd_seq_queue_timer_set_resolution snd_seq_queue_timer_set_resolution_dylibloader_wrapper_asound
#define snd_seq_get_queue_timer snd_seq_get_queue_timer_dylibloader_wrapper_asound
#define snd_seq_set_queue_timer snd_seq_set_queue_timer_dylibloader_wrapper_asound
#define snd_seq_free_event snd_seq_free_event_dylibloader_wrapper_asound
#define snd_seq_event_length snd_seq_event_length_dylibloader_wrapper_asound
#define snd_seq_event_output snd_seq_event_output_dylibloader_wrapper_asound
#define snd_seq_event_output_buffer snd_seq_event_output_buffer_dylibloader_wrapper_asound
#define snd_seq_event_output_direct snd_seq_event_output_direct_dylibloader_wrapper_asound
#define snd_seq_event_input snd_seq_event_input_dylibloader_wrapper_asound
#define snd_seq_event_input_pending snd_seq_event_input_pending_dylibloader_wrapper_asound
#define snd_seq_drain_output snd_seq_drain_output_dylibloader_wrapper_asound
#define snd_seq_event_output_pending snd_seq_event_output_pending_dylibloader_wrapper_asound
#define snd_seq_extract_output snd_seq_extract_output_dylibloader_wrapper_asound
#define snd_seq_drop_output snd_seq_drop_output_dylibloader_wrapper_asound
#define snd_seq_drop_output_buffer snd_seq_drop_output_buffer_dylibloader_wrapper_asound
#define snd_seq_drop_input snd_seq_drop_input_dylibloader_wrapper_asound
#define snd_seq_drop_input_buffer snd_seq_drop_input_buffer_dylibloader_wrapper_asound
#define snd_seq_remove_events_sizeof snd_seq_remove_events_sizeof_dylibloader_wrapper_asound
#define snd_seq_remove_events_malloc snd_seq_remove_events_malloc_dylibloader_wrapper_asound
#define snd_seq_remove_events_free snd_seq_remove_events_free_dylibloader_wrapper_asound
#define snd_seq_remove_events_copy snd_seq_remove_events_copy_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_condition snd_seq_remove_events_get_condition_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_queue snd_seq_remove_events_get_queue_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_time snd_seq_remove_events_get_time_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_dest snd_seq_remove_events_get_dest_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_channel snd_seq_remove_events_get_channel_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_event_type snd_seq_remove_events_get_event_type_dylibloader_wrapper_asound
#define snd_seq_remove_events_get_tag snd_seq_remove_events_get_tag_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_condition snd_seq_remove_events_set_condition_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_queue snd_seq_remove_events_set_queue_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_time snd_seq_remove_events_set_time_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_dest snd_seq_remove_events_set_dest_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_channel snd_seq_remove_events_set_channel_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_event_type snd_seq_remove_events_set_event_type_dylibloader_wrapper_asound
#define snd_seq_remove_events_set_tag snd_seq_remove_events_set_tag_dylibloader_wrapper_asound
#define snd_seq_remove_events snd_seq_remove_events_dylibloader_wrapper_asound
#define snd_seq_set_bit snd_seq_set_bit_dylibloader_wrapper_asound
#define snd_seq_unset_bit snd_seq_unset_bit_dylibloader_wrapper_asound
#define snd_seq_change_bit snd_seq_change_bit_dylibloader_wrapper_asound
#define snd_seq_get_bit snd_seq_get_bit_dylibloader_wrapper_asound
#define snd_seq_control_queue snd_seq_control_queue_dylibloader_wrapper_asound
#define snd_seq_create_simple_port snd_seq_create_simple_port_dylibloader_wrapper_asound
#define snd_seq_delete_simple_port snd_seq_delete_simple_port_dylibloader_wrapper_asound
#define snd_seq_connect_from snd_seq_connect_from_dylibloader_wrapper_asound
#define snd_seq_connect_to snd_seq_connect_to_dylibloader_wrapper_asound
#define snd_seq_disconnect_from snd_seq_disconnect_from_dylibloader_wrapper_asound
#define snd_seq_disconnect_to snd_seq_disconnect_to_dylibloader_wrapper_asound
#define snd_seq_set_client_name snd_seq_set_client_name_dylibloader_wrapper_asound
#define snd_seq_set_client_event_filter snd_seq_set_client_event_filter_dylibloader_wrapper_asound
#define snd_seq_set_client_pool_output snd_seq_set_client_pool_output_dylibloader_wrapper_asound
#define snd_seq_set_client_pool_output_room snd_seq_set_client_pool_output_room_dylibloader_wrapper_asound
#define snd_seq_set_client_pool_input snd_seq_set_client_pool_input_dylibloader_wrapper_asound
#define snd_seq_sync_output_queue snd_seq_sync_output_queue_dylibloader_wrapper_asound
#define snd_seq_parse_address snd_seq_parse_address_dylibloader_wrapper_asound
#define snd_seq_reset_pool_output snd_seq_reset_pool_output_dylibloader_wrapper_asound
#define snd_seq_reset_pool_input snd_seq_reset_pool_input_dylibloader_wrapper_asound
#define snd_midi_event_new snd_midi_event_new_dylibloader_wrapper_asound
#define snd_midi_event_resize_buffer snd_midi_event_resize_buffer_dylibloader_wrapper_asound
#define snd_midi_event_free snd_midi_event_free_dylibloader_wrapper_asound
#define snd_midi_event_init snd_midi_event_init_dylibloader_wrapper_asound
#define snd_midi_event_reset_encode snd_midi_event_reset_encode_dylibloader_wrapper_asound
#define snd_midi_event_reset_decode snd_midi_event_reset_decode_dylibloader_wrapper_asound
#define snd_midi_event_no_status snd_midi_event_no_status_dylibloader_wrapper_asound
#define snd_midi_event_encode snd_midi_event_encode_dylibloader_wrapper_asound
#define snd_midi_event_encode_byte snd_midi_event_encode_byte_dylibloader_wrapper_asound
#define snd_midi_event_decode snd_midi_event_decode_dylibloader_wrapper_asound
extern const char* (*snd_asoundlib_version_dylibloader_wrapper_asound)( void);
extern void* (*snd_dlopen_dylibloader_wrapper_asound)(const char*, int);
extern void* (*snd_dlsym_dylibloader_wrapper_asound)( void*,const char*,const char*);
extern int (*snd_dlclose_dylibloader_wrapper_asound)( void*);
extern int (*snd_async_add_handler_dylibloader_wrapper_asound)( snd_async_handler_t**, int, snd_async_callback_t, void*);
extern int (*snd_async_del_handler_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern int (*snd_async_handler_get_fd_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern int (*snd_async_handler_get_signo_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern void* (*snd_async_handler_get_callback_private_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern struct snd_shm_area* (*snd_shm_area_create_dylibloader_wrapper_asound)( int, void*);
extern struct snd_shm_area* (*snd_shm_area_share_dylibloader_wrapper_asound)(struct snd_shm_area*);
extern int (*snd_shm_area_destroy_dylibloader_wrapper_asound)(struct snd_shm_area*);
extern int (*snd_user_file_dylibloader_wrapper_asound)(const char*, char**);
extern int (*snd_input_stdio_open_dylibloader_wrapper_asound)( snd_input_t**,const char*,const char*);
extern int (*snd_input_stdio_attach_dylibloader_wrapper_asound)( snd_input_t**, FILE*, int);
extern int (*snd_input_buffer_open_dylibloader_wrapper_asound)( snd_input_t**,const char*, ssize_t);
extern int (*snd_input_close_dylibloader_wrapper_asound)( snd_input_t*);
extern int (*snd_input_scanf_dylibloader_wrapper_asound)( snd_input_t*,const char*,...);
extern char* (*snd_input_gets_dylibloader_wrapper_asound)( snd_input_t*, char*, size_t);
extern int (*snd_input_getc_dylibloader_wrapper_asound)( snd_input_t*);
extern int (*snd_input_ungetc_dylibloader_wrapper_asound)( snd_input_t*, int);
extern int (*snd_output_stdio_open_dylibloader_wrapper_asound)( snd_output_t**,const char*,const char*);
extern int (*snd_output_stdio_attach_dylibloader_wrapper_asound)( snd_output_t**, FILE*, int);
extern int (*snd_output_buffer_open_dylibloader_wrapper_asound)( snd_output_t**);
extern size_t (*snd_output_buffer_string_dylibloader_wrapper_asound)( snd_output_t*, char**);
extern int (*snd_output_close_dylibloader_wrapper_asound)( snd_output_t*);
extern int (*snd_output_printf_dylibloader_wrapper_asound)( snd_output_t*,const char*,...);
extern int (*snd_output_vprintf_dylibloader_wrapper_asound)( snd_output_t*,const char*, va_list);
extern int (*snd_output_puts_dylibloader_wrapper_asound)( snd_output_t*,const char*);
extern int (*snd_output_putc_dylibloader_wrapper_asound)( snd_output_t*, int);
extern int (*snd_output_flush_dylibloader_wrapper_asound)( snd_output_t*);
extern const char* (*snd_strerror_dylibloader_wrapper_asound)( int);
extern int (*snd_lib_error_set_handler_dylibloader_wrapper_asound)( snd_lib_error_handler_t);
extern snd_local_error_handler_t (*snd_lib_error_set_local_dylibloader_wrapper_asound)( snd_local_error_handler_t);
extern int (*snd_config_top_dylibloader_wrapper_asound)( snd_config_t**);
extern int (*snd_config_load_dylibloader_wrapper_asound)( snd_config_t*, snd_input_t*);
extern int (*snd_config_load_override_dylibloader_wrapper_asound)( snd_config_t*, snd_input_t*);
extern int (*snd_config_save_dylibloader_wrapper_asound)( snd_config_t*, snd_output_t*);
extern int (*snd_config_update_dylibloader_wrapper_asound)( void);
extern int (*snd_config_update_r_dylibloader_wrapper_asound)( snd_config_t**, snd_config_update_t**,const char*);
extern int (*snd_config_update_free_dylibloader_wrapper_asound)( snd_config_update_t*);
extern int (*snd_config_update_free_global_dylibloader_wrapper_asound)( void);
extern int (*snd_config_update_ref_dylibloader_wrapper_asound)( snd_config_t**);
extern void (*snd_config_ref_dylibloader_wrapper_asound)( snd_config_t*);
extern void (*snd_config_unref_dylibloader_wrapper_asound)( snd_config_t*);
extern int (*snd_config_search_dylibloader_wrapper_asound)( snd_config_t*,const char*, snd_config_t**);
extern int (*snd_config_searchv_dylibloader_wrapper_asound)( snd_config_t*, snd_config_t**,...);
extern int (*snd_config_search_definition_dylibloader_wrapper_asound)( snd_config_t*,const char*,const char*, snd_config_t**);
extern int (*snd_config_expand_dylibloader_wrapper_asound)( snd_config_t*, snd_config_t*,const char*, snd_config_t*, snd_config_t**);
extern int (*snd_config_evaluate_dylibloader_wrapper_asound)( snd_config_t*, snd_config_t*, snd_config_t*, snd_config_t**);
extern int (*snd_config_add_dylibloader_wrapper_asound)( snd_config_t*, snd_config_t*);
extern int (*snd_config_delete_dylibloader_wrapper_asound)( snd_config_t*);
extern int (*snd_config_delete_compound_members_dylibloader_wrapper_asound)(const snd_config_t*);
extern int (*snd_config_copy_dylibloader_wrapper_asound)( snd_config_t**, snd_config_t*);
extern int (*snd_config_make_dylibloader_wrapper_asound)( snd_config_t**,const char*, snd_config_type_t);
extern int (*snd_config_make_integer_dylibloader_wrapper_asound)( snd_config_t**,const char*);
extern int (*snd_config_make_integer64_dylibloader_wrapper_asound)( snd_config_t**,const char*);
extern int (*snd_config_make_real_dylibloader_wrapper_asound)( snd_config_t**,const char*);
extern int (*snd_config_make_string_dylibloader_wrapper_asound)( snd_config_t**,const char*);
extern int (*snd_config_make_pointer_dylibloader_wrapper_asound)( snd_config_t**,const char*);
extern int (*snd_config_make_compound_dylibloader_wrapper_asound)( snd_config_t**,const char*, int);
extern int (*snd_config_imake_integer_dylibloader_wrapper_asound)( snd_config_t**,const char*,const long);
extern int (*snd_config_imake_integer64_dylibloader_wrapper_asound)( snd_config_t**,const char*,const long long);
extern int (*snd_config_imake_real_dylibloader_wrapper_asound)( snd_config_t**,const char*,const double);
extern int (*snd_config_imake_string_dylibloader_wrapper_asound)( snd_config_t**,const char*,const char*);
extern int (*snd_config_imake_safe_string_dylibloader_wrapper_asound)( snd_config_t**,const char*,const char*);
extern int (*snd_config_imake_pointer_dylibloader_wrapper_asound)( snd_config_t**,const char*,const void*);
extern snd_config_type_t (*snd_config_get_type_dylibloader_wrapper_asound)(const snd_config_t*);
extern int (*snd_config_set_id_dylibloader_wrapper_asound)( snd_config_t*,const char*);
extern int (*snd_config_set_integer_dylibloader_wrapper_asound)( snd_config_t*, long);
extern int (*snd_config_set_integer64_dylibloader_wrapper_asound)( snd_config_t*, long long);
extern int (*snd_config_set_real_dylibloader_wrapper_asound)( snd_config_t*, double);
extern int (*snd_config_set_string_dylibloader_wrapper_asound)( snd_config_t*,const char*);
extern int (*snd_config_set_ascii_dylibloader_wrapper_asound)( snd_config_t*,const char*);
extern int (*snd_config_set_pointer_dylibloader_wrapper_asound)( snd_config_t*,const void*);
extern int (*snd_config_get_id_dylibloader_wrapper_asound)(const snd_config_t*,const char**);
extern int (*snd_config_get_integer_dylibloader_wrapper_asound)(const snd_config_t*, long*);
extern int (*snd_config_get_integer64_dylibloader_wrapper_asound)(const snd_config_t*, long long*);
extern int (*snd_config_get_real_dylibloader_wrapper_asound)(const snd_config_t*, double*);
extern int (*snd_config_get_ireal_dylibloader_wrapper_asound)(const snd_config_t*, double*);
extern int (*snd_config_get_string_dylibloader_wrapper_asound)(const snd_config_t*,const char**);
extern int (*snd_config_get_ascii_dylibloader_wrapper_asound)(const snd_config_t*, char**);
extern int (*snd_config_get_pointer_dylibloader_wrapper_asound)(const snd_config_t*,const void**);
extern int (*snd_config_test_id_dylibloader_wrapper_asound)(const snd_config_t*,const char*);
extern snd_config_iterator_t (*snd_config_iterator_first_dylibloader_wrapper_asound)(const snd_config_t*);
extern snd_config_iterator_t (*snd_config_iterator_next_dylibloader_wrapper_asound)(const snd_config_iterator_t);
extern snd_config_iterator_t (*snd_config_iterator_end_dylibloader_wrapper_asound)(const snd_config_t*);
extern snd_config_t* (*snd_config_iterator_entry_dylibloader_wrapper_asound)(const snd_config_iterator_t);
extern int (*snd_config_get_bool_ascii_dylibloader_wrapper_asound)(const char*);
extern int (*snd_config_get_bool_dylibloader_wrapper_asound)(const snd_config_t*);
extern int (*snd_config_get_ctl_iface_ascii_dylibloader_wrapper_asound)(const char*);
extern int (*snd_config_get_ctl_iface_dylibloader_wrapper_asound)(const snd_config_t*);
extern int (*snd_names_list_dylibloader_wrapper_asound)(const char*, snd_devname_t**);
extern void (*snd_names_list_free_dylibloader_wrapper_asound)( snd_devname_t*);
extern int (*snd_pcm_open_dylibloader_wrapper_asound)( snd_pcm_t**,const char*, snd_pcm_stream_t, int);
extern int (*snd_pcm_open_lconf_dylibloader_wrapper_asound)( snd_pcm_t**,const char*, snd_pcm_stream_t, int, snd_config_t*);
extern int (*snd_pcm_open_fallback_dylibloader_wrapper_asound)( snd_pcm_t**, snd_config_t*,const char*,const char*, snd_pcm_stream_t, int);
extern int (*snd_pcm_close_dylibloader_wrapper_asound)( snd_pcm_t*);
extern const char* (*snd_pcm_name_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_type_t (*snd_pcm_type_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_stream_t (*snd_pcm_stream_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_poll_descriptors_count_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_poll_descriptors_dylibloader_wrapper_asound)( snd_pcm_t*,struct pollfd*, unsigned int);
extern int (*snd_pcm_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_pcm_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_pcm_nonblock_dylibloader_wrapper_asound)( snd_pcm_t*, int);
extern int (*snd_async_add_pcm_handler_dylibloader_wrapper_asound)( snd_async_handler_t**, snd_pcm_t*, snd_async_callback_t, void*);
extern snd_pcm_t* (*snd_async_handler_get_pcm_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern int (*snd_pcm_info_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_info_t*);
extern int (*snd_pcm_hw_params_current_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_free_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_sw_params_current_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*);
extern int (*snd_pcm_sw_params_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*);
extern int (*snd_pcm_prepare_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_reset_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_status_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_status_t*);
extern int (*snd_pcm_start_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_drop_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_drain_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_pause_dylibloader_wrapper_asound)( snd_pcm_t*, int);
extern snd_pcm_state_t (*snd_pcm_state_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_hwsync_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_delay_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sframes_t*);
extern int (*snd_pcm_resume_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_htimestamp_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_uframes_t*, snd_htimestamp_t*);
extern snd_pcm_sframes_t (*snd_pcm_avail_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_sframes_t (*snd_pcm_avail_update_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_avail_delay_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sframes_t*, snd_pcm_sframes_t*);
extern snd_pcm_sframes_t (*snd_pcm_rewindable_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_sframes_t (*snd_pcm_rewind_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_forwardable_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_sframes_t (*snd_pcm_forward_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_writei_dylibloader_wrapper_asound)( snd_pcm_t*,const void*, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_readi_dylibloader_wrapper_asound)( snd_pcm_t*, void*, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_writen_dylibloader_wrapper_asound)( snd_pcm_t*, void**, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_readn_dylibloader_wrapper_asound)( snd_pcm_t*, void**, snd_pcm_uframes_t);
extern int (*snd_pcm_wait_dylibloader_wrapper_asound)( snd_pcm_t*, int);
extern int (*snd_pcm_link_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_t*);
extern int (*snd_pcm_unlink_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_chmap_query_t** (*snd_pcm_query_chmaps_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_chmap_query_t** (*snd_pcm_query_chmaps_from_hw_dylibloader_wrapper_asound)( int, int, int, snd_pcm_stream_t);
extern void (*snd_pcm_free_chmaps_dylibloader_wrapper_asound)( snd_pcm_chmap_query_t**);
extern snd_pcm_chmap_t* (*snd_pcm_get_chmap_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_set_chmap_dylibloader_wrapper_asound)( snd_pcm_t*,const snd_pcm_chmap_t*);
extern const char* (*snd_pcm_chmap_type_name_dylibloader_wrapper_asound)(enum snd_pcm_chmap_type);
extern const char* (*snd_pcm_chmap_name_dylibloader_wrapper_asound)(enum snd_pcm_chmap_position);
extern const char* (*snd_pcm_chmap_long_name_dylibloader_wrapper_asound)(enum snd_pcm_chmap_position);
extern int (*snd_pcm_chmap_print_dylibloader_wrapper_asound)(const snd_pcm_chmap_t*, size_t, char*);
extern unsigned int (*snd_pcm_chmap_from_string_dylibloader_wrapper_asound)(const char*);
extern snd_pcm_chmap_t* (*snd_pcm_chmap_parse_string_dylibloader_wrapper_asound)(const char*);
extern int (*snd_pcm_recover_dylibloader_wrapper_asound)( snd_pcm_t*, int, int);
extern int (*snd_pcm_set_params_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_format_t, snd_pcm_access_t, unsigned int, unsigned int, int, unsigned int);
extern int (*snd_pcm_get_params_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_uframes_t*, snd_pcm_uframes_t*);
extern size_t (*snd_pcm_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_info_malloc_dylibloader_wrapper_asound)( snd_pcm_info_t**);
extern void (*snd_pcm_info_free_dylibloader_wrapper_asound)( snd_pcm_info_t*);
extern void (*snd_pcm_info_copy_dylibloader_wrapper_asound)( snd_pcm_info_t*,const snd_pcm_info_t*);
extern unsigned int (*snd_pcm_info_get_device_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern unsigned int (*snd_pcm_info_get_subdevice_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern snd_pcm_stream_t (*snd_pcm_info_get_stream_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern int (*snd_pcm_info_get_card_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern const char* (*snd_pcm_info_get_id_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern const char* (*snd_pcm_info_get_name_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern const char* (*snd_pcm_info_get_subdevice_name_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern snd_pcm_class_t (*snd_pcm_info_get_class_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern snd_pcm_subclass_t (*snd_pcm_info_get_subclass_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern unsigned int (*snd_pcm_info_get_subdevices_count_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern unsigned int (*snd_pcm_info_get_subdevices_avail_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern snd_pcm_sync_id_t (*snd_pcm_info_get_sync_dylibloader_wrapper_asound)(const snd_pcm_info_t*);
extern void (*snd_pcm_info_set_device_dylibloader_wrapper_asound)( snd_pcm_info_t*, unsigned int);
extern void (*snd_pcm_info_set_subdevice_dylibloader_wrapper_asound)( snd_pcm_info_t*, unsigned int);
extern void (*snd_pcm_info_set_stream_dylibloader_wrapper_asound)( snd_pcm_info_t*, snd_pcm_stream_t);
extern int (*snd_pcm_hw_params_any_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_can_mmap_sample_resolution_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_is_double_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_is_batch_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_is_block_transfer_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_is_monotonic_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_can_overrange_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_can_pause_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_can_resume_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_is_half_duplex_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_is_joint_duplex_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_can_sync_start_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_can_disable_period_wakeup_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_supports_audio_wallclock_ts_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_supports_audio_ts_type_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, int);
extern int (*snd_pcm_hw_params_get_rate_numden_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, unsigned int*);
extern int (*snd_pcm_hw_params_get_sbits_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_get_fifo_size_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*);
extern size_t (*snd_pcm_hw_params_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_hw_params_malloc_dylibloader_wrapper_asound)( snd_pcm_hw_params_t**);
extern void (*snd_pcm_hw_params_free_dylibloader_wrapper_asound)( snd_pcm_hw_params_t*);
extern void (*snd_pcm_hw_params_copy_dylibloader_wrapper_asound)( snd_pcm_hw_params_t*,const snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_get_access_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_access_t*);
extern int (*snd_pcm_hw_params_test_access_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_t);
extern int (*snd_pcm_hw_params_set_access_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_t);
extern int (*snd_pcm_hw_params_set_access_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_t*);
extern int (*snd_pcm_hw_params_set_access_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_t*);
extern int (*snd_pcm_hw_params_set_access_mask_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_access_mask_t*);
extern int (*snd_pcm_hw_params_get_access_mask_dylibloader_wrapper_asound)( snd_pcm_hw_params_t*, snd_pcm_access_mask_t*);
extern int (*snd_pcm_hw_params_get_format_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_format_t*);
extern int (*snd_pcm_hw_params_test_format_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_t);
extern int (*snd_pcm_hw_params_set_format_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_t);
extern int (*snd_pcm_hw_params_set_format_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_t*);
extern int (*snd_pcm_hw_params_set_format_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_t*);
extern int (*snd_pcm_hw_params_set_format_mask_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_format_mask_t*);
extern void (*snd_pcm_hw_params_get_format_mask_dylibloader_wrapper_asound)( snd_pcm_hw_params_t*, snd_pcm_format_mask_t*);
extern int (*snd_pcm_hw_params_get_subformat_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_subformat_t*);
extern int (*snd_pcm_hw_params_test_subformat_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_subformat_t);
extern int (*snd_pcm_hw_params_set_subformat_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_subformat_t);
extern int (*snd_pcm_hw_params_set_subformat_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_subformat_t*);
extern int (*snd_pcm_hw_params_set_subformat_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_subformat_t*);
extern int (*snd_pcm_hw_params_set_subformat_mask_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_subformat_mask_t*);
extern void (*snd_pcm_hw_params_get_subformat_mask_dylibloader_wrapper_asound)( snd_pcm_hw_params_t*, snd_pcm_subformat_mask_t*);
extern int (*snd_pcm_hw_params_get_channels_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_get_channels_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_get_channels_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_test_channels_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int);
extern int (*snd_pcm_hw_params_set_channels_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int);
extern int (*snd_pcm_hw_params_set_channels_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_set_channels_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_set_channels_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, unsigned int*);
extern int (*snd_pcm_hw_params_set_channels_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_set_channels_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_set_channels_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_get_rate_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_rate_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_rate_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_test_rate_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_rate_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_rate_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_rate_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_rate_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_rate_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_rate_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_rate_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_rate_resample_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int);
extern int (*snd_pcm_hw_params_get_rate_resample_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_set_export_buffer_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int);
extern int (*snd_pcm_hw_params_get_export_buffer_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_set_period_wakeup_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int);
extern int (*snd_pcm_hw_params_get_period_wakeup_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_get_period_time_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_period_time_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_period_time_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_test_period_time_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_period_time_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_period_time_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_period_time_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_period_time_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_period_time_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_period_time_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_period_time_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_period_size_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_get_period_size_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_get_period_size_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_test_period_size_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t, int);
extern int (*snd_pcm_hw_params_set_period_size_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t, int);
extern int (*snd_pcm_hw_params_set_period_size_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_set_period_size_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_set_period_size_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_set_period_size_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_set_period_size_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_set_period_size_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, int*);
extern int (*snd_pcm_hw_params_set_period_size_integer_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_get_periods_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_periods_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_periods_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_test_periods_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_periods_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_periods_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_periods_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_periods_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_periods_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_periods_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_periods_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_periods_integer_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*);
extern int (*snd_pcm_hw_params_get_buffer_time_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_buffer_time_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_buffer_time_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_test_buffer_time_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_buffer_time_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_buffer_time_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_buffer_time_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_buffer_time_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_buffer_time_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_buffer_time_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_buffer_time_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_buffer_size_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_get_buffer_size_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_get_buffer_size_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_test_buffer_size_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_hw_params_set_buffer_size_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_hw_params_set_buffer_size_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_set_buffer_size_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_set_buffer_size_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_set_buffer_size_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_set_buffer_size_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_set_buffer_size_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_hw_params_get_min_align_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, snd_pcm_uframes_t*);
extern size_t (*snd_pcm_sw_params_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_sw_params_malloc_dylibloader_wrapper_asound)( snd_pcm_sw_params_t**);
extern void (*snd_pcm_sw_params_free_dylibloader_wrapper_asound)( snd_pcm_sw_params_t*);
extern void (*snd_pcm_sw_params_copy_dylibloader_wrapper_asound)( snd_pcm_sw_params_t*,const snd_pcm_sw_params_t*);
extern int (*snd_pcm_sw_params_get_boundary_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_sw_params_set_tstamp_mode_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_tstamp_t);
extern int (*snd_pcm_sw_params_get_tstamp_mode_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_tstamp_t*);
extern int (*snd_pcm_sw_params_set_avail_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_sw_params_get_avail_min_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_sw_params_set_period_event_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, int);
extern int (*snd_pcm_sw_params_get_period_event_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, int*);
extern int (*snd_pcm_sw_params_set_start_threshold_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_sw_params_get_start_threshold_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_sw_params_set_stop_threshold_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_sw_params_get_stop_threshold_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_sw_params_set_silence_threshold_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_sw_params_get_silence_threshold_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_sw_params_set_silence_size_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_sw_params_get_silence_size_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern size_t (*snd_pcm_access_mask_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_access_mask_malloc_dylibloader_wrapper_asound)( snd_pcm_access_mask_t**);
extern void (*snd_pcm_access_mask_free_dylibloader_wrapper_asound)( snd_pcm_access_mask_t*);
extern void (*snd_pcm_access_mask_copy_dylibloader_wrapper_asound)( snd_pcm_access_mask_t*,const snd_pcm_access_mask_t*);
extern void (*snd_pcm_access_mask_none_dylibloader_wrapper_asound)( snd_pcm_access_mask_t*);
extern void (*snd_pcm_access_mask_any_dylibloader_wrapper_asound)( snd_pcm_access_mask_t*);
extern int (*snd_pcm_access_mask_test_dylibloader_wrapper_asound)(const snd_pcm_access_mask_t*, snd_pcm_access_t);
extern int (*snd_pcm_access_mask_empty_dylibloader_wrapper_asound)(const snd_pcm_access_mask_t*);
extern void (*snd_pcm_access_mask_set_dylibloader_wrapper_asound)( snd_pcm_access_mask_t*, snd_pcm_access_t);
extern void (*snd_pcm_access_mask_reset_dylibloader_wrapper_asound)( snd_pcm_access_mask_t*, snd_pcm_access_t);
extern size_t (*snd_pcm_format_mask_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_format_mask_malloc_dylibloader_wrapper_asound)( snd_pcm_format_mask_t**);
extern void (*snd_pcm_format_mask_free_dylibloader_wrapper_asound)( snd_pcm_format_mask_t*);
extern void (*snd_pcm_format_mask_copy_dylibloader_wrapper_asound)( snd_pcm_format_mask_t*,const snd_pcm_format_mask_t*);
extern void (*snd_pcm_format_mask_none_dylibloader_wrapper_asound)( snd_pcm_format_mask_t*);
extern void (*snd_pcm_format_mask_any_dylibloader_wrapper_asound)( snd_pcm_format_mask_t*);
extern int (*snd_pcm_format_mask_test_dylibloader_wrapper_asound)(const snd_pcm_format_mask_t*, snd_pcm_format_t);
extern int (*snd_pcm_format_mask_empty_dylibloader_wrapper_asound)(const snd_pcm_format_mask_t*);
extern void (*snd_pcm_format_mask_set_dylibloader_wrapper_asound)( snd_pcm_format_mask_t*, snd_pcm_format_t);
extern void (*snd_pcm_format_mask_reset_dylibloader_wrapper_asound)( snd_pcm_format_mask_t*, snd_pcm_format_t);
extern size_t (*snd_pcm_subformat_mask_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_subformat_mask_malloc_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t**);
extern void (*snd_pcm_subformat_mask_free_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t*);
extern void (*snd_pcm_subformat_mask_copy_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t*,const snd_pcm_subformat_mask_t*);
extern void (*snd_pcm_subformat_mask_none_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t*);
extern void (*snd_pcm_subformat_mask_any_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t*);
extern int (*snd_pcm_subformat_mask_test_dylibloader_wrapper_asound)(const snd_pcm_subformat_mask_t*, snd_pcm_subformat_t);
extern int (*snd_pcm_subformat_mask_empty_dylibloader_wrapper_asound)(const snd_pcm_subformat_mask_t*);
extern void (*snd_pcm_subformat_mask_set_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t*, snd_pcm_subformat_t);
extern void (*snd_pcm_subformat_mask_reset_dylibloader_wrapper_asound)( snd_pcm_subformat_mask_t*, snd_pcm_subformat_t);
extern size_t (*snd_pcm_status_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_pcm_status_malloc_dylibloader_wrapper_asound)( snd_pcm_status_t**);
extern void (*snd_pcm_status_free_dylibloader_wrapper_asound)( snd_pcm_status_t*);
extern void (*snd_pcm_status_copy_dylibloader_wrapper_asound)( snd_pcm_status_t*,const snd_pcm_status_t*);
extern snd_pcm_state_t (*snd_pcm_status_get_state_dylibloader_wrapper_asound)(const snd_pcm_status_t*);
extern void (*snd_pcm_status_get_trigger_tstamp_dylibloader_wrapper_asound)(const snd_pcm_status_t*, snd_timestamp_t*);
extern void (*snd_pcm_status_get_trigger_htstamp_dylibloader_wrapper_asound)(const snd_pcm_status_t*, snd_htimestamp_t*);
extern void (*snd_pcm_status_get_tstamp_dylibloader_wrapper_asound)(const snd_pcm_status_t*, snd_timestamp_t*);
extern void (*snd_pcm_status_get_htstamp_dylibloader_wrapper_asound)(const snd_pcm_status_t*, snd_htimestamp_t*);
extern void (*snd_pcm_status_get_audio_htstamp_dylibloader_wrapper_asound)(const snd_pcm_status_t*, snd_htimestamp_t*);
extern void (*snd_pcm_status_get_driver_htstamp_dylibloader_wrapper_asound)(const snd_pcm_status_t*, snd_htimestamp_t*);
extern snd_pcm_sframes_t (*snd_pcm_status_get_delay_dylibloader_wrapper_asound)(const snd_pcm_status_t*);
extern snd_pcm_uframes_t (*snd_pcm_status_get_avail_dylibloader_wrapper_asound)(const snd_pcm_status_t*);
extern snd_pcm_uframes_t (*snd_pcm_status_get_avail_max_dylibloader_wrapper_asound)(const snd_pcm_status_t*);
extern snd_pcm_uframes_t (*snd_pcm_status_get_overrange_dylibloader_wrapper_asound)(const snd_pcm_status_t*);
extern const char* (*snd_pcm_type_name_dylibloader_wrapper_asound)( snd_pcm_type_t);
extern const char* (*snd_pcm_stream_name_dylibloader_wrapper_asound)(const snd_pcm_stream_t);
extern const char* (*snd_pcm_access_name_dylibloader_wrapper_asound)(const snd_pcm_access_t);
extern const char* (*snd_pcm_format_name_dylibloader_wrapper_asound)(const snd_pcm_format_t);
extern const char* (*snd_pcm_format_description_dylibloader_wrapper_asound)(const snd_pcm_format_t);
extern const char* (*snd_pcm_subformat_name_dylibloader_wrapper_asound)(const snd_pcm_subformat_t);
extern const char* (*snd_pcm_subformat_description_dylibloader_wrapper_asound)(const snd_pcm_subformat_t);
extern snd_pcm_format_t (*snd_pcm_format_value_dylibloader_wrapper_asound)(const char*);
extern const char* (*snd_pcm_tstamp_mode_name_dylibloader_wrapper_asound)(const snd_pcm_tstamp_t);
extern const char* (*snd_pcm_state_name_dylibloader_wrapper_asound)(const snd_pcm_state_t);
extern int (*snd_pcm_dump_dylibloader_wrapper_asound)( snd_pcm_t*, snd_output_t*);
extern int (*snd_pcm_dump_hw_setup_dylibloader_wrapper_asound)( snd_pcm_t*, snd_output_t*);
extern int (*snd_pcm_dump_sw_setup_dylibloader_wrapper_asound)( snd_pcm_t*, snd_output_t*);
extern int (*snd_pcm_dump_setup_dylibloader_wrapper_asound)( snd_pcm_t*, snd_output_t*);
extern int (*snd_pcm_hw_params_dump_dylibloader_wrapper_asound)( snd_pcm_hw_params_t*, snd_output_t*);
extern int (*snd_pcm_sw_params_dump_dylibloader_wrapper_asound)( snd_pcm_sw_params_t*, snd_output_t*);
extern int (*snd_pcm_status_dump_dylibloader_wrapper_asound)( snd_pcm_status_t*, snd_output_t*);
extern int (*snd_pcm_mmap_begin_dylibloader_wrapper_asound)( snd_pcm_t*,const snd_pcm_channel_area_t**, snd_pcm_uframes_t*, snd_pcm_uframes_t*);
extern snd_pcm_sframes_t (*snd_pcm_mmap_commit_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_uframes_t, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_mmap_writei_dylibloader_wrapper_asound)( snd_pcm_t*,const void*, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_mmap_readi_dylibloader_wrapper_asound)( snd_pcm_t*, void*, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_mmap_writen_dylibloader_wrapper_asound)( snd_pcm_t*, void**, snd_pcm_uframes_t);
extern snd_pcm_sframes_t (*snd_pcm_mmap_readn_dylibloader_wrapper_asound)( snd_pcm_t*, void**, snd_pcm_uframes_t);
extern int (*snd_pcm_format_signed_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_unsigned_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_linear_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_float_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_little_endian_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_big_endian_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_cpu_endian_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_width_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_physical_width_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern snd_pcm_format_t (*snd_pcm_build_linear_format_dylibloader_wrapper_asound)( int, int, int, int);
extern ssize_t (*snd_pcm_format_size_dylibloader_wrapper_asound)( snd_pcm_format_t, size_t);
extern uint8_t (*snd_pcm_format_silence_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern uint16_t (*snd_pcm_format_silence_16_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern uint32_t (*snd_pcm_format_silence_32_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern uint64_t (*snd_pcm_format_silence_64_dylibloader_wrapper_asound)( snd_pcm_format_t);
extern int (*snd_pcm_format_set_silence_dylibloader_wrapper_asound)( snd_pcm_format_t, void*, unsigned int);
extern snd_pcm_sframes_t (*snd_pcm_bytes_to_frames_dylibloader_wrapper_asound)( snd_pcm_t*, ssize_t);
extern ssize_t (*snd_pcm_frames_to_bytes_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sframes_t);
extern long (*snd_pcm_bytes_to_samples_dylibloader_wrapper_asound)( snd_pcm_t*, ssize_t);
extern ssize_t (*snd_pcm_samples_to_bytes_dylibloader_wrapper_asound)( snd_pcm_t*, long);
extern int (*snd_pcm_area_silence_dylibloader_wrapper_asound)(const snd_pcm_channel_area_t*, snd_pcm_uframes_t, unsigned int, snd_pcm_format_t);
extern int (*snd_pcm_areas_silence_dylibloader_wrapper_asound)(const snd_pcm_channel_area_t*, snd_pcm_uframes_t, unsigned int, snd_pcm_uframes_t, snd_pcm_format_t);
extern int (*snd_pcm_area_copy_dylibloader_wrapper_asound)(const snd_pcm_channel_area_t*, snd_pcm_uframes_t,const snd_pcm_channel_area_t*, snd_pcm_uframes_t, unsigned int, snd_pcm_format_t);
extern int (*snd_pcm_areas_copy_dylibloader_wrapper_asound)(const snd_pcm_channel_area_t*, snd_pcm_uframes_t,const snd_pcm_channel_area_t*, snd_pcm_uframes_t, unsigned int, snd_pcm_uframes_t, snd_pcm_format_t);
extern snd_pcm_t* (*snd_pcm_hook_get_pcm_dylibloader_wrapper_asound)( snd_pcm_hook_t*);
extern void* (*snd_pcm_hook_get_private_dylibloader_wrapper_asound)( snd_pcm_hook_t*);
extern void (*snd_pcm_hook_set_private_dylibloader_wrapper_asound)( snd_pcm_hook_t*, void*);
extern int (*snd_pcm_hook_add_dylibloader_wrapper_asound)( snd_pcm_hook_t**, snd_pcm_t*, snd_pcm_hook_type_t, snd_pcm_hook_func_t, void*);
extern int (*snd_pcm_hook_remove_dylibloader_wrapper_asound)( snd_pcm_hook_t*);
extern snd_pcm_uframes_t (*snd_pcm_meter_get_bufsize_dylibloader_wrapper_asound)( snd_pcm_t*);
extern unsigned int (*snd_pcm_meter_get_channels_dylibloader_wrapper_asound)( snd_pcm_t*);
extern unsigned int (*snd_pcm_meter_get_rate_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_uframes_t (*snd_pcm_meter_get_now_dylibloader_wrapper_asound)( snd_pcm_t*);
extern snd_pcm_uframes_t (*snd_pcm_meter_get_boundary_dylibloader_wrapper_asound)( snd_pcm_t*);
extern int (*snd_pcm_meter_add_scope_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_scope_t*);
extern snd_pcm_scope_t* (*snd_pcm_meter_search_scope_dylibloader_wrapper_asound)( snd_pcm_t*,const char*);
extern int (*snd_pcm_scope_malloc_dylibloader_wrapper_asound)( snd_pcm_scope_t**);
extern void (*snd_pcm_scope_set_ops_dylibloader_wrapper_asound)( snd_pcm_scope_t*,const snd_pcm_scope_ops_t*);
extern void (*snd_pcm_scope_set_name_dylibloader_wrapper_asound)( snd_pcm_scope_t*,const char*);
extern const char* (*snd_pcm_scope_get_name_dylibloader_wrapper_asound)( snd_pcm_scope_t*);
extern void* (*snd_pcm_scope_get_callback_private_dylibloader_wrapper_asound)( snd_pcm_scope_t*);
extern void (*snd_pcm_scope_set_callback_private_dylibloader_wrapper_asound)( snd_pcm_scope_t*, void*);
extern int (*snd_pcm_scope_s16_open_dylibloader_wrapper_asound)( snd_pcm_t*,const char*, snd_pcm_scope_t**);
extern int16_t* (*snd_pcm_scope_s16_get_channel_buffer_dylibloader_wrapper_asound)( snd_pcm_scope_t*, unsigned int);
extern int (*snd_spcm_init_dylibloader_wrapper_asound)( snd_pcm_t*, unsigned int, unsigned int, snd_pcm_format_t, snd_pcm_subformat_t, snd_spcm_latency_t, snd_pcm_access_t, snd_spcm_xrun_type_t);
extern int (*snd_spcm_init_duplex_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_t*, unsigned int, unsigned int, snd_pcm_format_t, snd_pcm_subformat_t, snd_spcm_latency_t, snd_pcm_access_t, snd_spcm_xrun_type_t, snd_spcm_duplex_type_t);
extern int (*snd_spcm_init_get_params_dylibloader_wrapper_asound)( snd_pcm_t*, unsigned int*, snd_pcm_uframes_t*, snd_pcm_uframes_t*);
extern const char* (*snd_pcm_start_mode_name_dylibloader_wrapper_asound)( snd_pcm_start_t);
extern const char* (*snd_pcm_xrun_mode_name_dylibloader_wrapper_asound)( snd_pcm_xrun_t);
extern int (*snd_pcm_sw_params_set_start_mode_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_start_t);
extern snd_pcm_start_t (*snd_pcm_sw_params_get_start_mode_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*);
extern int (*snd_pcm_sw_params_set_xrun_mode_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_xrun_t);
extern snd_pcm_xrun_t (*snd_pcm_sw_params_get_xrun_mode_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*);
extern int (*snd_pcm_sw_params_set_xfer_align_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, snd_pcm_uframes_t);
extern int (*snd_pcm_sw_params_get_xfer_align_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, snd_pcm_uframes_t*);
extern int (*snd_pcm_sw_params_set_sleep_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_sw_params_t*, unsigned int);
extern int (*snd_pcm_sw_params_get_sleep_min_dylibloader_wrapper_asound)(const snd_pcm_sw_params_t*, unsigned int*);
extern int (*snd_pcm_hw_params_get_tick_time_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_tick_time_min_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_get_tick_time_max_dylibloader_wrapper_asound)(const snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_test_tick_time_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_tick_time_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int, int);
extern int (*snd_pcm_hw_params_set_tick_time_min_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_tick_time_max_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_tick_time_minmax_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_tick_time_near_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_tick_time_first_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_pcm_hw_params_set_tick_time_last_dylibloader_wrapper_asound)( snd_pcm_t*, snd_pcm_hw_params_t*, unsigned int*, int*);
extern int (*snd_rawmidi_open_dylibloader_wrapper_asound)( snd_rawmidi_t**, snd_rawmidi_t**,const char*, int);
extern int (*snd_rawmidi_open_lconf_dylibloader_wrapper_asound)( snd_rawmidi_t**, snd_rawmidi_t**,const char*, int, snd_config_t*);
extern int (*snd_rawmidi_close_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern int (*snd_rawmidi_poll_descriptors_count_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern int (*snd_rawmidi_poll_descriptors_dylibloader_wrapper_asound)( snd_rawmidi_t*,struct pollfd*, unsigned int);
extern int (*snd_rawmidi_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_rawmidi_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_rawmidi_nonblock_dylibloader_wrapper_asound)( snd_rawmidi_t*, int);
extern size_t (*snd_rawmidi_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_rawmidi_info_malloc_dylibloader_wrapper_asound)( snd_rawmidi_info_t**);
extern void (*snd_rawmidi_info_free_dylibloader_wrapper_asound)( snd_rawmidi_info_t*);
extern void (*snd_rawmidi_info_copy_dylibloader_wrapper_asound)( snd_rawmidi_info_t*,const snd_rawmidi_info_t*);
extern unsigned int (*snd_rawmidi_info_get_device_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern unsigned int (*snd_rawmidi_info_get_subdevice_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern snd_rawmidi_stream_t (*snd_rawmidi_info_get_stream_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern int (*snd_rawmidi_info_get_card_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern unsigned int (*snd_rawmidi_info_get_flags_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern const char* (*snd_rawmidi_info_get_id_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern const char* (*snd_rawmidi_info_get_name_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern const char* (*snd_rawmidi_info_get_subdevice_name_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern unsigned int (*snd_rawmidi_info_get_subdevices_count_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern unsigned int (*snd_rawmidi_info_get_subdevices_avail_dylibloader_wrapper_asound)(const snd_rawmidi_info_t*);
extern void (*snd_rawmidi_info_set_device_dylibloader_wrapper_asound)( snd_rawmidi_info_t*, unsigned int);
extern void (*snd_rawmidi_info_set_subdevice_dylibloader_wrapper_asound)( snd_rawmidi_info_t*, unsigned int);
extern void (*snd_rawmidi_info_set_stream_dylibloader_wrapper_asound)( snd_rawmidi_info_t*, snd_rawmidi_stream_t);
extern int (*snd_rawmidi_info_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_info_t*);
extern size_t (*snd_rawmidi_params_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_rawmidi_params_malloc_dylibloader_wrapper_asound)( snd_rawmidi_params_t**);
extern void (*snd_rawmidi_params_free_dylibloader_wrapper_asound)( snd_rawmidi_params_t*);
extern void (*snd_rawmidi_params_copy_dylibloader_wrapper_asound)( snd_rawmidi_params_t*,const snd_rawmidi_params_t*);
extern int (*snd_rawmidi_params_set_buffer_size_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_params_t*, size_t);
extern size_t (*snd_rawmidi_params_get_buffer_size_dylibloader_wrapper_asound)(const snd_rawmidi_params_t*);
extern int (*snd_rawmidi_params_set_avail_min_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_params_t*, size_t);
extern size_t (*snd_rawmidi_params_get_avail_min_dylibloader_wrapper_asound)(const snd_rawmidi_params_t*);
extern int (*snd_rawmidi_params_set_no_active_sensing_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_params_t*, int);
extern int (*snd_rawmidi_params_get_no_active_sensing_dylibloader_wrapper_asound)(const snd_rawmidi_params_t*);
extern int (*snd_rawmidi_params_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_params_t*);
extern int (*snd_rawmidi_params_current_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_params_t*);
extern size_t (*snd_rawmidi_status_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_rawmidi_status_malloc_dylibloader_wrapper_asound)( snd_rawmidi_status_t**);
extern void (*snd_rawmidi_status_free_dylibloader_wrapper_asound)( snd_rawmidi_status_t*);
extern void (*snd_rawmidi_status_copy_dylibloader_wrapper_asound)( snd_rawmidi_status_t*,const snd_rawmidi_status_t*);
extern void (*snd_rawmidi_status_get_tstamp_dylibloader_wrapper_asound)(const snd_rawmidi_status_t*, snd_htimestamp_t*);
extern size_t (*snd_rawmidi_status_get_avail_dylibloader_wrapper_asound)(const snd_rawmidi_status_t*);
extern size_t (*snd_rawmidi_status_get_xruns_dylibloader_wrapper_asound)(const snd_rawmidi_status_t*);
extern int (*snd_rawmidi_status_dylibloader_wrapper_asound)( snd_rawmidi_t*, snd_rawmidi_status_t*);
extern int (*snd_rawmidi_drain_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern int (*snd_rawmidi_drop_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern ssize_t (*snd_rawmidi_write_dylibloader_wrapper_asound)( snd_rawmidi_t*,const void*, size_t);
extern ssize_t (*snd_rawmidi_read_dylibloader_wrapper_asound)( snd_rawmidi_t*, void*, size_t);
extern const char* (*snd_rawmidi_name_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern snd_rawmidi_type_t (*snd_rawmidi_type_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern snd_rawmidi_stream_t (*snd_rawmidi_stream_dylibloader_wrapper_asound)( snd_rawmidi_t*);
extern int (*snd_timer_query_open_dylibloader_wrapper_asound)( snd_timer_query_t**,const char*, int);
extern int (*snd_timer_query_open_lconf_dylibloader_wrapper_asound)( snd_timer_query_t**,const char*, int, snd_config_t*);
extern int (*snd_timer_query_close_dylibloader_wrapper_asound)( snd_timer_query_t*);
extern int (*snd_timer_query_next_device_dylibloader_wrapper_asound)( snd_timer_query_t*, snd_timer_id_t*);
extern int (*snd_timer_query_info_dylibloader_wrapper_asound)( snd_timer_query_t*, snd_timer_ginfo_t*);
extern int (*snd_timer_query_params_dylibloader_wrapper_asound)( snd_timer_query_t*, snd_timer_gparams_t*);
extern int (*snd_timer_query_status_dylibloader_wrapper_asound)( snd_timer_query_t*, snd_timer_gstatus_t*);
extern int (*snd_timer_open_dylibloader_wrapper_asound)( snd_timer_t**,const char*, int);
extern int (*snd_timer_open_lconf_dylibloader_wrapper_asound)( snd_timer_t**,const char*, int, snd_config_t*);
extern int (*snd_timer_close_dylibloader_wrapper_asound)( snd_timer_t*);
extern int (*snd_async_add_timer_handler_dylibloader_wrapper_asound)( snd_async_handler_t**, snd_timer_t*, snd_async_callback_t, void*);
extern snd_timer_t* (*snd_async_handler_get_timer_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern int (*snd_timer_poll_descriptors_count_dylibloader_wrapper_asound)( snd_timer_t*);
extern int (*snd_timer_poll_descriptors_dylibloader_wrapper_asound)( snd_timer_t*,struct pollfd*, unsigned int);
extern int (*snd_timer_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_timer_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_timer_info_dylibloader_wrapper_asound)( snd_timer_t*, snd_timer_info_t*);
extern int (*snd_timer_params_dylibloader_wrapper_asound)( snd_timer_t*, snd_timer_params_t*);
extern int (*snd_timer_status_dylibloader_wrapper_asound)( snd_timer_t*, snd_timer_status_t*);
extern int (*snd_timer_start_dylibloader_wrapper_asound)( snd_timer_t*);
extern int (*snd_timer_stop_dylibloader_wrapper_asound)( snd_timer_t*);
extern int (*snd_timer_continue_dylibloader_wrapper_asound)( snd_timer_t*);
extern ssize_t (*snd_timer_read_dylibloader_wrapper_asound)( snd_timer_t*, void*, size_t);
extern size_t (*snd_timer_id_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_timer_id_malloc_dylibloader_wrapper_asound)( snd_timer_id_t**);
extern void (*snd_timer_id_free_dylibloader_wrapper_asound)( snd_timer_id_t*);
extern void (*snd_timer_id_copy_dylibloader_wrapper_asound)( snd_timer_id_t*,const snd_timer_id_t*);
extern void (*snd_timer_id_set_class_dylibloader_wrapper_asound)( snd_timer_id_t*, int);
extern int (*snd_timer_id_get_class_dylibloader_wrapper_asound)( snd_timer_id_t*);
extern void (*snd_timer_id_set_sclass_dylibloader_wrapper_asound)( snd_timer_id_t*, int);
extern int (*snd_timer_id_get_sclass_dylibloader_wrapper_asound)( snd_timer_id_t*);
extern void (*snd_timer_id_set_card_dylibloader_wrapper_asound)( snd_timer_id_t*, int);
extern int (*snd_timer_id_get_card_dylibloader_wrapper_asound)( snd_timer_id_t*);
extern void (*snd_timer_id_set_device_dylibloader_wrapper_asound)( snd_timer_id_t*, int);
extern int (*snd_timer_id_get_device_dylibloader_wrapper_asound)( snd_timer_id_t*);
extern void (*snd_timer_id_set_subdevice_dylibloader_wrapper_asound)( snd_timer_id_t*, int);
extern int (*snd_timer_id_get_subdevice_dylibloader_wrapper_asound)( snd_timer_id_t*);
extern size_t (*snd_timer_ginfo_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_timer_ginfo_malloc_dylibloader_wrapper_asound)( snd_timer_ginfo_t**);
extern void (*snd_timer_ginfo_free_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern void (*snd_timer_ginfo_copy_dylibloader_wrapper_asound)( snd_timer_ginfo_t*,const snd_timer_ginfo_t*);
extern int (*snd_timer_ginfo_set_tid_dylibloader_wrapper_asound)( snd_timer_ginfo_t*, snd_timer_id_t*);
extern snd_timer_id_t* (*snd_timer_ginfo_get_tid_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern unsigned int (*snd_timer_ginfo_get_flags_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern int (*snd_timer_ginfo_get_card_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern char* (*snd_timer_ginfo_get_id_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern char* (*snd_timer_ginfo_get_name_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern unsigned long (*snd_timer_ginfo_get_resolution_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern unsigned long (*snd_timer_ginfo_get_resolution_min_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern unsigned long (*snd_timer_ginfo_get_resolution_max_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern unsigned int (*snd_timer_ginfo_get_clients_dylibloader_wrapper_asound)( snd_timer_ginfo_t*);
extern size_t (*snd_timer_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_timer_info_malloc_dylibloader_wrapper_asound)( snd_timer_info_t**);
extern void (*snd_timer_info_free_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern void (*snd_timer_info_copy_dylibloader_wrapper_asound)( snd_timer_info_t*,const snd_timer_info_t*);
extern int (*snd_timer_info_is_slave_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern int (*snd_timer_info_get_card_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern const char* (*snd_timer_info_get_id_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern const char* (*snd_timer_info_get_name_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern long (*snd_timer_info_get_resolution_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern size_t (*snd_timer_params_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_timer_params_malloc_dylibloader_wrapper_asound)( snd_timer_params_t**);
extern void (*snd_timer_params_free_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern void (*snd_timer_params_copy_dylibloader_wrapper_asound)( snd_timer_params_t*,const snd_timer_params_t*);
extern int (*snd_timer_params_set_auto_start_dylibloader_wrapper_asound)( snd_timer_params_t*, int);
extern int (*snd_timer_params_get_auto_start_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern int (*snd_timer_params_set_exclusive_dylibloader_wrapper_asound)( snd_timer_params_t*, int);
extern int (*snd_timer_params_get_exclusive_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern int (*snd_timer_params_set_early_event_dylibloader_wrapper_asound)( snd_timer_params_t*, int);
extern int (*snd_timer_params_get_early_event_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern void (*snd_timer_params_set_ticks_dylibloader_wrapper_asound)( snd_timer_params_t*, long);
extern long (*snd_timer_params_get_ticks_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern void (*snd_timer_params_set_queue_size_dylibloader_wrapper_asound)( snd_timer_params_t*, long);
extern long (*snd_timer_params_get_queue_size_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern void (*snd_timer_params_set_filter_dylibloader_wrapper_asound)( snd_timer_params_t*, unsigned int);
extern unsigned int (*snd_timer_params_get_filter_dylibloader_wrapper_asound)( snd_timer_params_t*);
extern size_t (*snd_timer_status_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_timer_status_malloc_dylibloader_wrapper_asound)( snd_timer_status_t**);
extern void (*snd_timer_status_free_dylibloader_wrapper_asound)( snd_timer_status_t*);
extern void (*snd_timer_status_copy_dylibloader_wrapper_asound)( snd_timer_status_t*,const snd_timer_status_t*);
extern snd_htimestamp_t (*snd_timer_status_get_timestamp_dylibloader_wrapper_asound)( snd_timer_status_t*);
extern long (*snd_timer_status_get_resolution_dylibloader_wrapper_asound)( snd_timer_status_t*);
extern long (*snd_timer_status_get_lost_dylibloader_wrapper_asound)( snd_timer_status_t*);
extern long (*snd_timer_status_get_overrun_dylibloader_wrapper_asound)( snd_timer_status_t*);
extern long (*snd_timer_status_get_queue_dylibloader_wrapper_asound)( snd_timer_status_t*);
extern long (*snd_timer_info_get_ticks_dylibloader_wrapper_asound)( snd_timer_info_t*);
extern int (*snd_hwdep_open_dylibloader_wrapper_asound)( snd_hwdep_t**,const char*, int);
extern int (*snd_hwdep_close_dylibloader_wrapper_asound)( snd_hwdep_t*);
extern int (*snd_hwdep_poll_descriptors_dylibloader_wrapper_asound)( snd_hwdep_t*,struct pollfd*, unsigned int);
extern int (*snd_hwdep_poll_descriptors_count_dylibloader_wrapper_asound)( snd_hwdep_t*);
extern int (*snd_hwdep_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_hwdep_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_hwdep_nonblock_dylibloader_wrapper_asound)( snd_hwdep_t*, int);
extern int (*snd_hwdep_info_dylibloader_wrapper_asound)( snd_hwdep_t*, snd_hwdep_info_t*);
extern int (*snd_hwdep_dsp_status_dylibloader_wrapper_asound)( snd_hwdep_t*, snd_hwdep_dsp_status_t*);
extern int (*snd_hwdep_dsp_load_dylibloader_wrapper_asound)( snd_hwdep_t*, snd_hwdep_dsp_image_t*);
extern int (*snd_hwdep_ioctl_dylibloader_wrapper_asound)( snd_hwdep_t*, unsigned int, void*);
extern ssize_t (*snd_hwdep_write_dylibloader_wrapper_asound)( snd_hwdep_t*,const void*, size_t);
extern ssize_t (*snd_hwdep_read_dylibloader_wrapper_asound)( snd_hwdep_t*, void*, size_t);
extern size_t (*snd_hwdep_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_hwdep_info_malloc_dylibloader_wrapper_asound)( snd_hwdep_info_t**);
extern void (*snd_hwdep_info_free_dylibloader_wrapper_asound)( snd_hwdep_info_t*);
extern void (*snd_hwdep_info_copy_dylibloader_wrapper_asound)( snd_hwdep_info_t*,const snd_hwdep_info_t*);
extern unsigned int (*snd_hwdep_info_get_device_dylibloader_wrapper_asound)(const snd_hwdep_info_t*);
extern int (*snd_hwdep_info_get_card_dylibloader_wrapper_asound)(const snd_hwdep_info_t*);
extern const char* (*snd_hwdep_info_get_id_dylibloader_wrapper_asound)(const snd_hwdep_info_t*);
extern const char* (*snd_hwdep_info_get_name_dylibloader_wrapper_asound)(const snd_hwdep_info_t*);
extern snd_hwdep_iface_t (*snd_hwdep_info_get_iface_dylibloader_wrapper_asound)(const snd_hwdep_info_t*);
extern void (*snd_hwdep_info_set_device_dylibloader_wrapper_asound)( snd_hwdep_info_t*, unsigned int);
extern size_t (*snd_hwdep_dsp_status_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_hwdep_dsp_status_malloc_dylibloader_wrapper_asound)( snd_hwdep_dsp_status_t**);
extern void (*snd_hwdep_dsp_status_free_dylibloader_wrapper_asound)( snd_hwdep_dsp_status_t*);
extern void (*snd_hwdep_dsp_status_copy_dylibloader_wrapper_asound)( snd_hwdep_dsp_status_t*,const snd_hwdep_dsp_status_t*);
extern unsigned int (*snd_hwdep_dsp_status_get_version_dylibloader_wrapper_asound)(const snd_hwdep_dsp_status_t*);
extern const char* (*snd_hwdep_dsp_status_get_id_dylibloader_wrapper_asound)(const snd_hwdep_dsp_status_t*);
extern unsigned int (*snd_hwdep_dsp_status_get_num_dsps_dylibloader_wrapper_asound)(const snd_hwdep_dsp_status_t*);
extern unsigned int (*snd_hwdep_dsp_status_get_dsp_loaded_dylibloader_wrapper_asound)(const snd_hwdep_dsp_status_t*);
extern unsigned int (*snd_hwdep_dsp_status_get_chip_ready_dylibloader_wrapper_asound)(const snd_hwdep_dsp_status_t*);
extern size_t (*snd_hwdep_dsp_image_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_hwdep_dsp_image_malloc_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t**);
extern void (*snd_hwdep_dsp_image_free_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t*);
extern void (*snd_hwdep_dsp_image_copy_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t*,const snd_hwdep_dsp_image_t*);
extern unsigned int (*snd_hwdep_dsp_image_get_index_dylibloader_wrapper_asound)(const snd_hwdep_dsp_image_t*);
extern const char* (*snd_hwdep_dsp_image_get_name_dylibloader_wrapper_asound)(const snd_hwdep_dsp_image_t*);
extern const void* (*snd_hwdep_dsp_image_get_image_dylibloader_wrapper_asound)(const snd_hwdep_dsp_image_t*);
extern size_t (*snd_hwdep_dsp_image_get_length_dylibloader_wrapper_asound)(const snd_hwdep_dsp_image_t*);
extern void (*snd_hwdep_dsp_image_set_index_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t*, unsigned int);
extern void (*snd_hwdep_dsp_image_set_name_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t*,const char*);
extern void (*snd_hwdep_dsp_image_set_image_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t*, void*);
extern void (*snd_hwdep_dsp_image_set_length_dylibloader_wrapper_asound)( snd_hwdep_dsp_image_t*, size_t);
extern int (*snd_card_load_dylibloader_wrapper_asound)( int);
extern int (*snd_card_next_dylibloader_wrapper_asound)( int*);
extern int (*snd_card_get_index_dylibloader_wrapper_asound)(const char*);
extern int (*snd_card_get_name_dylibloader_wrapper_asound)( int, char**);
extern int (*snd_card_get_longname_dylibloader_wrapper_asound)( int, char**);
extern int (*snd_device_name_hint_dylibloader_wrapper_asound)( int,const char*, void***);
extern int (*snd_device_name_free_hint_dylibloader_wrapper_asound)( void**);
extern char* (*snd_device_name_get_hint_dylibloader_wrapper_asound)(const void*,const char*);
extern int (*snd_ctl_open_dylibloader_wrapper_asound)( snd_ctl_t**,const char*, int);
extern int (*snd_ctl_open_lconf_dylibloader_wrapper_asound)( snd_ctl_t**,const char*, int, snd_config_t*);
extern int (*snd_ctl_open_fallback_dylibloader_wrapper_asound)( snd_ctl_t**, snd_config_t*,const char*,const char*, int);
extern int (*snd_ctl_close_dylibloader_wrapper_asound)( snd_ctl_t*);
extern int (*snd_ctl_nonblock_dylibloader_wrapper_asound)( snd_ctl_t*, int);
extern int (*snd_async_add_ctl_handler_dylibloader_wrapper_asound)( snd_async_handler_t**, snd_ctl_t*, snd_async_callback_t, void*);
extern snd_ctl_t* (*snd_async_handler_get_ctl_dylibloader_wrapper_asound)( snd_async_handler_t*);
extern int (*snd_ctl_poll_descriptors_count_dylibloader_wrapper_asound)( snd_ctl_t*);
extern int (*snd_ctl_poll_descriptors_dylibloader_wrapper_asound)( snd_ctl_t*,struct pollfd*, unsigned int);
extern int (*snd_ctl_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_ctl_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_ctl_subscribe_events_dylibloader_wrapper_asound)( snd_ctl_t*, int);
extern int (*snd_ctl_card_info_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_card_info_t*);
extern int (*snd_ctl_elem_list_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_list_t*);
extern int (*snd_ctl_elem_info_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_read_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_value_t*);
extern int (*snd_ctl_elem_write_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_value_t*);
extern int (*snd_ctl_elem_lock_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_id_t*);
extern int (*snd_ctl_elem_unlock_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_id_t*);
extern int (*snd_ctl_elem_tlv_read_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, unsigned int*, unsigned int);
extern int (*snd_ctl_elem_tlv_write_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*,const unsigned int*);
extern int (*snd_ctl_elem_tlv_command_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*,const unsigned int*);
extern int (*snd_ctl_hwdep_next_device_dylibloader_wrapper_asound)( snd_ctl_t*, int*);
extern int (*snd_ctl_hwdep_info_dylibloader_wrapper_asound)( snd_ctl_t*, snd_hwdep_info_t*);
extern int (*snd_ctl_pcm_next_device_dylibloader_wrapper_asound)( snd_ctl_t*, int*);
extern int (*snd_ctl_pcm_info_dylibloader_wrapper_asound)( snd_ctl_t*, snd_pcm_info_t*);
extern int (*snd_ctl_pcm_prefer_subdevice_dylibloader_wrapper_asound)( snd_ctl_t*, int);
extern int (*snd_ctl_rawmidi_next_device_dylibloader_wrapper_asound)( snd_ctl_t*, int*);
extern int (*snd_ctl_rawmidi_info_dylibloader_wrapper_asound)( snd_ctl_t*, snd_rawmidi_info_t*);
extern int (*snd_ctl_rawmidi_prefer_subdevice_dylibloader_wrapper_asound)( snd_ctl_t*, int);
extern int (*snd_ctl_set_power_state_dylibloader_wrapper_asound)( snd_ctl_t*, unsigned int);
extern int (*snd_ctl_get_power_state_dylibloader_wrapper_asound)( snd_ctl_t*, unsigned int*);
extern int (*snd_ctl_read_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_event_t*);
extern int (*snd_ctl_wait_dylibloader_wrapper_asound)( snd_ctl_t*, int);
extern const char* (*snd_ctl_name_dylibloader_wrapper_asound)( snd_ctl_t*);
extern snd_ctl_type_t (*snd_ctl_type_dylibloader_wrapper_asound)( snd_ctl_t*);
extern const char* (*snd_ctl_elem_type_name_dylibloader_wrapper_asound)( snd_ctl_elem_type_t);
extern const char* (*snd_ctl_elem_iface_name_dylibloader_wrapper_asound)( snd_ctl_elem_iface_t);
extern const char* (*snd_ctl_event_type_name_dylibloader_wrapper_asound)( snd_ctl_event_type_t);
extern unsigned int (*snd_ctl_event_elem_get_mask_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern unsigned int (*snd_ctl_event_elem_get_numid_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern void (*snd_ctl_event_elem_get_id_dylibloader_wrapper_asound)(const snd_ctl_event_t*, snd_ctl_elem_id_t*);
extern snd_ctl_elem_iface_t (*snd_ctl_event_elem_get_interface_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern unsigned int (*snd_ctl_event_elem_get_device_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern unsigned int (*snd_ctl_event_elem_get_subdevice_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern const char* (*snd_ctl_event_elem_get_name_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern unsigned int (*snd_ctl_event_elem_get_index_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern int (*snd_ctl_elem_list_alloc_space_dylibloader_wrapper_asound)( snd_ctl_elem_list_t*, unsigned int);
extern void (*snd_ctl_elem_list_free_space_dylibloader_wrapper_asound)( snd_ctl_elem_list_t*);
extern char* (*snd_ctl_ascii_elem_id_get_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*);
extern int (*snd_ctl_ascii_elem_id_parse_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*,const char*);
extern int (*snd_ctl_ascii_value_parse_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_value_t*, snd_ctl_elem_info_t*,const char*);
extern size_t (*snd_ctl_elem_id_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_ctl_elem_id_malloc_dylibloader_wrapper_asound)( snd_ctl_elem_id_t**);
extern void (*snd_ctl_elem_id_free_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*);
extern void (*snd_ctl_elem_id_clear_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*);
extern void (*snd_ctl_elem_id_copy_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*,const snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_id_get_numid_dylibloader_wrapper_asound)(const snd_ctl_elem_id_t*);
extern snd_ctl_elem_iface_t (*snd_ctl_elem_id_get_interface_dylibloader_wrapper_asound)(const snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_id_get_device_dylibloader_wrapper_asound)(const snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_id_get_subdevice_dylibloader_wrapper_asound)(const snd_ctl_elem_id_t*);
extern const char* (*snd_ctl_elem_id_get_name_dylibloader_wrapper_asound)(const snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_id_get_index_dylibloader_wrapper_asound)(const snd_ctl_elem_id_t*);
extern void (*snd_ctl_elem_id_set_numid_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*, unsigned int);
extern void (*snd_ctl_elem_id_set_interface_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*, snd_ctl_elem_iface_t);
extern void (*snd_ctl_elem_id_set_device_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*, unsigned int);
extern void (*snd_ctl_elem_id_set_subdevice_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*, unsigned int);
extern void (*snd_ctl_elem_id_set_name_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*,const char*);
extern void (*snd_ctl_elem_id_set_index_dylibloader_wrapper_asound)( snd_ctl_elem_id_t*, unsigned int);
extern size_t (*snd_ctl_card_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_ctl_card_info_malloc_dylibloader_wrapper_asound)( snd_ctl_card_info_t**);
extern void (*snd_ctl_card_info_free_dylibloader_wrapper_asound)( snd_ctl_card_info_t*);
extern void (*snd_ctl_card_info_clear_dylibloader_wrapper_asound)( snd_ctl_card_info_t*);
extern void (*snd_ctl_card_info_copy_dylibloader_wrapper_asound)( snd_ctl_card_info_t*,const snd_ctl_card_info_t*);
extern int (*snd_ctl_card_info_get_card_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern const char* (*snd_ctl_card_info_get_id_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern const char* (*snd_ctl_card_info_get_driver_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern const char* (*snd_ctl_card_info_get_name_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern const char* (*snd_ctl_card_info_get_longname_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern const char* (*snd_ctl_card_info_get_mixername_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern const char* (*snd_ctl_card_info_get_components_dylibloader_wrapper_asound)(const snd_ctl_card_info_t*);
extern size_t (*snd_ctl_event_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_ctl_event_malloc_dylibloader_wrapper_asound)( snd_ctl_event_t**);
extern void (*snd_ctl_event_free_dylibloader_wrapper_asound)( snd_ctl_event_t*);
extern void (*snd_ctl_event_clear_dylibloader_wrapper_asound)( snd_ctl_event_t*);
extern void (*snd_ctl_event_copy_dylibloader_wrapper_asound)( snd_ctl_event_t*,const snd_ctl_event_t*);
extern snd_ctl_event_type_t (*snd_ctl_event_get_type_dylibloader_wrapper_asound)(const snd_ctl_event_t*);
extern size_t (*snd_ctl_elem_list_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_ctl_elem_list_malloc_dylibloader_wrapper_asound)( snd_ctl_elem_list_t**);
extern void (*snd_ctl_elem_list_free_dylibloader_wrapper_asound)( snd_ctl_elem_list_t*);
extern void (*snd_ctl_elem_list_clear_dylibloader_wrapper_asound)( snd_ctl_elem_list_t*);
extern void (*snd_ctl_elem_list_copy_dylibloader_wrapper_asound)( snd_ctl_elem_list_t*,const snd_ctl_elem_list_t*);
extern void (*snd_ctl_elem_list_set_offset_dylibloader_wrapper_asound)( snd_ctl_elem_list_t*, unsigned int);
extern unsigned int (*snd_ctl_elem_list_get_used_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*);
extern unsigned int (*snd_ctl_elem_list_get_count_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*);
extern void (*snd_ctl_elem_list_get_id_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int, snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_list_get_numid_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int);
extern snd_ctl_elem_iface_t (*snd_ctl_elem_list_get_interface_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int);
extern unsigned int (*snd_ctl_elem_list_get_device_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int);
extern unsigned int (*snd_ctl_elem_list_get_subdevice_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int);
extern const char* (*snd_ctl_elem_list_get_name_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int);
extern unsigned int (*snd_ctl_elem_list_get_index_dylibloader_wrapper_asound)(const snd_ctl_elem_list_t*, unsigned int);
extern size_t (*snd_ctl_elem_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_ctl_elem_info_malloc_dylibloader_wrapper_asound)( snd_ctl_elem_info_t**);
extern void (*snd_ctl_elem_info_free_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*);
extern void (*snd_ctl_elem_info_clear_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*);
extern void (*snd_ctl_elem_info_copy_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*,const snd_ctl_elem_info_t*);
extern snd_ctl_elem_type_t (*snd_ctl_elem_info_get_type_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_readable_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_writable_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_volatile_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_inactive_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_locked_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_tlv_readable_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_tlv_writable_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_tlv_commandable_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_owner_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_is_user_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern pid_t (*snd_ctl_elem_info_get_owner_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern unsigned int (*snd_ctl_elem_info_get_count_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern long (*snd_ctl_elem_info_get_min_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern long (*snd_ctl_elem_info_get_max_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern long (*snd_ctl_elem_info_get_step_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern long long (*snd_ctl_elem_info_get_min64_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern long long (*snd_ctl_elem_info_get_max64_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern long long (*snd_ctl_elem_info_get_step64_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern unsigned int (*snd_ctl_elem_info_get_items_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern void (*snd_ctl_elem_info_set_item_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*, unsigned int);
extern const char* (*snd_ctl_elem_info_get_item_name_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_get_dimensions_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern int (*snd_ctl_elem_info_get_dimension_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*, unsigned int);
extern int (*snd_ctl_elem_info_set_dimension_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*,const int [4]);
extern void (*snd_ctl_elem_info_get_id_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*, snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_info_get_numid_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern snd_ctl_elem_iface_t (*snd_ctl_elem_info_get_interface_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern unsigned int (*snd_ctl_elem_info_get_device_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern unsigned int (*snd_ctl_elem_info_get_subdevice_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern const char* (*snd_ctl_elem_info_get_name_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern unsigned int (*snd_ctl_elem_info_get_index_dylibloader_wrapper_asound)(const snd_ctl_elem_info_t*);
extern void (*snd_ctl_elem_info_set_id_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*,const snd_ctl_elem_id_t*);
extern void (*snd_ctl_elem_info_set_numid_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*, unsigned int);
extern void (*snd_ctl_elem_info_set_interface_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*, snd_ctl_elem_iface_t);
extern void (*snd_ctl_elem_info_set_device_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*, unsigned int);
extern void (*snd_ctl_elem_info_set_subdevice_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*, unsigned int);
extern void (*snd_ctl_elem_info_set_name_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*,const char*);
extern void (*snd_ctl_elem_info_set_index_dylibloader_wrapper_asound)( snd_ctl_elem_info_t*, unsigned int);
extern int (*snd_ctl_add_integer_elem_set_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_info_t*, unsigned int, unsigned int, long, long, long);
extern int (*snd_ctl_add_integer64_elem_set_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_info_t*, unsigned int, unsigned int, long long, long long, long long);
extern int (*snd_ctl_add_boolean_elem_set_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_info_t*, unsigned int, unsigned int);
extern int (*snd_ctl_add_enumerated_elem_set_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_info_t*, unsigned int, unsigned int, unsigned int,const char* []);
extern int (*snd_ctl_add_bytes_elem_set_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_info_t*, unsigned int, unsigned int);
extern int (*snd_ctl_elem_add_integer_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, unsigned int, long, long, long);
extern int (*snd_ctl_elem_add_integer64_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, unsigned int, long long, long long, long long);
extern int (*snd_ctl_elem_add_boolean_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, unsigned int);
extern int (*snd_ctl_elem_add_enumerated_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, unsigned int, unsigned int,const char* []);
extern int (*snd_ctl_elem_add_iec958_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*);
extern int (*snd_ctl_elem_remove_dylibloader_wrapper_asound)( snd_ctl_t*, snd_ctl_elem_id_t*);
extern size_t (*snd_ctl_elem_value_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_ctl_elem_value_malloc_dylibloader_wrapper_asound)( snd_ctl_elem_value_t**);
extern void (*snd_ctl_elem_value_free_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*);
extern void (*snd_ctl_elem_value_clear_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*);
extern void (*snd_ctl_elem_value_copy_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*,const snd_ctl_elem_value_t*);
extern int (*snd_ctl_elem_value_compare_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*,const snd_ctl_elem_value_t*);
extern void (*snd_ctl_elem_value_get_id_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, snd_ctl_elem_id_t*);
extern unsigned int (*snd_ctl_elem_value_get_numid_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern snd_ctl_elem_iface_t (*snd_ctl_elem_value_get_interface_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern unsigned int (*snd_ctl_elem_value_get_device_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern unsigned int (*snd_ctl_elem_value_get_subdevice_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern const char* (*snd_ctl_elem_value_get_name_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern unsigned int (*snd_ctl_elem_value_get_index_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern void (*snd_ctl_elem_value_set_id_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*,const snd_ctl_elem_id_t*);
extern void (*snd_ctl_elem_value_set_numid_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int);
extern void (*snd_ctl_elem_value_set_interface_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, snd_ctl_elem_iface_t);
extern void (*snd_ctl_elem_value_set_device_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int);
extern void (*snd_ctl_elem_value_set_subdevice_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int);
extern void (*snd_ctl_elem_value_set_name_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*,const char*);
extern void (*snd_ctl_elem_value_set_index_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int);
extern int (*snd_ctl_elem_value_get_boolean_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, unsigned int);
extern long (*snd_ctl_elem_value_get_integer_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, unsigned int);
extern long long (*snd_ctl_elem_value_get_integer64_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, unsigned int);
extern unsigned int (*snd_ctl_elem_value_get_enumerated_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, unsigned int);
extern unsigned char (*snd_ctl_elem_value_get_byte_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, unsigned int);
extern void (*snd_ctl_elem_value_set_boolean_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int, long);
extern void (*snd_ctl_elem_value_set_integer_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int, long);
extern void (*snd_ctl_elem_value_set_integer64_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int, long long);
extern void (*snd_ctl_elem_value_set_enumerated_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int, unsigned int);
extern void (*snd_ctl_elem_value_set_byte_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, unsigned int, unsigned char);
extern void (*snd_ctl_elem_set_bytes_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*, void*, size_t);
extern const void* (*snd_ctl_elem_value_get_bytes_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*);
extern void (*snd_ctl_elem_value_get_iec958_dylibloader_wrapper_asound)(const snd_ctl_elem_value_t*, snd_aes_iec958_t*);
extern void (*snd_ctl_elem_value_set_iec958_dylibloader_wrapper_asound)( snd_ctl_elem_value_t*,const snd_aes_iec958_t*);
extern int (*snd_tlv_parse_dB_info_dylibloader_wrapper_asound)( unsigned int*, unsigned int, unsigned int**);
extern int (*snd_tlv_get_dB_range_dylibloader_wrapper_asound)( unsigned int*, long, long, long*, long*);
extern int (*snd_tlv_convert_to_dB_dylibloader_wrapper_asound)( unsigned int*, long, long, long, long*);
extern int (*snd_tlv_convert_from_dB_dylibloader_wrapper_asound)( unsigned int*, long, long, long, long*, int);
extern int (*snd_ctl_get_dB_range_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, long*, long*);
extern int (*snd_ctl_convert_to_dB_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, long, long*);
extern int (*snd_ctl_convert_from_dB_dylibloader_wrapper_asound)( snd_ctl_t*,const snd_ctl_elem_id_t*, long, long*, int);
extern int (*snd_hctl_compare_fast_dylibloader_wrapper_asound)(const snd_hctl_elem_t*,const snd_hctl_elem_t*);
extern int (*snd_hctl_open_dylibloader_wrapper_asound)( snd_hctl_t**,const char*, int);
extern int (*snd_hctl_open_ctl_dylibloader_wrapper_asound)( snd_hctl_t**, snd_ctl_t*);
extern int (*snd_hctl_close_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_nonblock_dylibloader_wrapper_asound)( snd_hctl_t*, int);
extern int (*snd_hctl_poll_descriptors_count_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_poll_descriptors_dylibloader_wrapper_asound)( snd_hctl_t*,struct pollfd*, unsigned int);
extern int (*snd_hctl_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_hctl_t*,struct pollfd*, unsigned int, unsigned short*);
extern unsigned int (*snd_hctl_get_count_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_set_compare_dylibloader_wrapper_asound)( snd_hctl_t*, snd_hctl_compare_t);
extern snd_hctl_elem_t* (*snd_hctl_first_elem_dylibloader_wrapper_asound)( snd_hctl_t*);
extern snd_hctl_elem_t* (*snd_hctl_last_elem_dylibloader_wrapper_asound)( snd_hctl_t*);
extern snd_hctl_elem_t* (*snd_hctl_find_elem_dylibloader_wrapper_asound)( snd_hctl_t*,const snd_ctl_elem_id_t*);
extern void (*snd_hctl_set_callback_dylibloader_wrapper_asound)( snd_hctl_t*, snd_hctl_callback_t);
extern void (*snd_hctl_set_callback_private_dylibloader_wrapper_asound)( snd_hctl_t*, void*);
extern void* (*snd_hctl_get_callback_private_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_load_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_free_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_handle_events_dylibloader_wrapper_asound)( snd_hctl_t*);
extern const char* (*snd_hctl_name_dylibloader_wrapper_asound)( snd_hctl_t*);
extern int (*snd_hctl_wait_dylibloader_wrapper_asound)( snd_hctl_t*, int);
extern snd_ctl_t* (*snd_hctl_ctl_dylibloader_wrapper_asound)( snd_hctl_t*);
extern snd_hctl_elem_t* (*snd_hctl_elem_next_dylibloader_wrapper_asound)( snd_hctl_elem_t*);
extern snd_hctl_elem_t* (*snd_hctl_elem_prev_dylibloader_wrapper_asound)( snd_hctl_elem_t*);
extern int (*snd_hctl_elem_info_dylibloader_wrapper_asound)( snd_hctl_elem_t*, snd_ctl_elem_info_t*);
extern int (*snd_hctl_elem_read_dylibloader_wrapper_asound)( snd_hctl_elem_t*, snd_ctl_elem_value_t*);
extern int (*snd_hctl_elem_write_dylibloader_wrapper_asound)( snd_hctl_elem_t*, snd_ctl_elem_value_t*);
extern int (*snd_hctl_elem_tlv_read_dylibloader_wrapper_asound)( snd_hctl_elem_t*, unsigned int*, unsigned int);
extern int (*snd_hctl_elem_tlv_write_dylibloader_wrapper_asound)( snd_hctl_elem_t*,const unsigned int*);
extern int (*snd_hctl_elem_tlv_command_dylibloader_wrapper_asound)( snd_hctl_elem_t*,const unsigned int*);
extern snd_hctl_t* (*snd_hctl_elem_get_hctl_dylibloader_wrapper_asound)( snd_hctl_elem_t*);
extern void (*snd_hctl_elem_get_id_dylibloader_wrapper_asound)(const snd_hctl_elem_t*, snd_ctl_elem_id_t*);
extern unsigned int (*snd_hctl_elem_get_numid_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern snd_ctl_elem_iface_t (*snd_hctl_elem_get_interface_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern unsigned int (*snd_hctl_elem_get_device_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern unsigned int (*snd_hctl_elem_get_subdevice_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern const char* (*snd_hctl_elem_get_name_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern unsigned int (*snd_hctl_elem_get_index_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern void (*snd_hctl_elem_set_callback_dylibloader_wrapper_asound)( snd_hctl_elem_t*, snd_hctl_elem_callback_t);
extern void* (*snd_hctl_elem_get_callback_private_dylibloader_wrapper_asound)(const snd_hctl_elem_t*);
extern void (*snd_hctl_elem_set_callback_private_dylibloader_wrapper_asound)( snd_hctl_elem_t*, void*);
extern int (*snd_sctl_build_dylibloader_wrapper_asound)( snd_sctl_t**, snd_ctl_t*, snd_config_t*, snd_config_t*, int);
extern int (*snd_sctl_free_dylibloader_wrapper_asound)( snd_sctl_t*);
extern int (*snd_sctl_install_dylibloader_wrapper_asound)( snd_sctl_t*);
extern int (*snd_sctl_remove_dylibloader_wrapper_asound)( snd_sctl_t*);
extern int (*snd_mixer_open_dylibloader_wrapper_asound)( snd_mixer_t**, int);
extern int (*snd_mixer_close_dylibloader_wrapper_asound)( snd_mixer_t*);
extern snd_mixer_elem_t* (*snd_mixer_first_elem_dylibloader_wrapper_asound)( snd_mixer_t*);
extern snd_mixer_elem_t* (*snd_mixer_last_elem_dylibloader_wrapper_asound)( snd_mixer_t*);
extern int (*snd_mixer_handle_events_dylibloader_wrapper_asound)( snd_mixer_t*);
extern int (*snd_mixer_attach_dylibloader_wrapper_asound)( snd_mixer_t*,const char*);
extern int (*snd_mixer_attach_hctl_dylibloader_wrapper_asound)( snd_mixer_t*, snd_hctl_t*);
extern int (*snd_mixer_detach_dylibloader_wrapper_asound)( snd_mixer_t*,const char*);
extern int (*snd_mixer_detach_hctl_dylibloader_wrapper_asound)( snd_mixer_t*, snd_hctl_t*);
extern int (*snd_mixer_get_hctl_dylibloader_wrapper_asound)( snd_mixer_t*,const char*, snd_hctl_t**);
extern int (*snd_mixer_poll_descriptors_count_dylibloader_wrapper_asound)( snd_mixer_t*);
extern int (*snd_mixer_poll_descriptors_dylibloader_wrapper_asound)( snd_mixer_t*,struct pollfd*, unsigned int);
extern int (*snd_mixer_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_mixer_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_mixer_load_dylibloader_wrapper_asound)( snd_mixer_t*);
extern void (*snd_mixer_free_dylibloader_wrapper_asound)( snd_mixer_t*);
extern int (*snd_mixer_wait_dylibloader_wrapper_asound)( snd_mixer_t*, int);
extern int (*snd_mixer_set_compare_dylibloader_wrapper_asound)( snd_mixer_t*, snd_mixer_compare_t);
extern void (*snd_mixer_set_callback_dylibloader_wrapper_asound)( snd_mixer_t*, snd_mixer_callback_t);
extern void* (*snd_mixer_get_callback_private_dylibloader_wrapper_asound)(const snd_mixer_t*);
extern void (*snd_mixer_set_callback_private_dylibloader_wrapper_asound)( snd_mixer_t*, void*);
extern unsigned int (*snd_mixer_get_count_dylibloader_wrapper_asound)(const snd_mixer_t*);
extern int (*snd_mixer_class_unregister_dylibloader_wrapper_asound)( snd_mixer_class_t*);
extern snd_mixer_elem_t* (*snd_mixer_elem_next_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern snd_mixer_elem_t* (*snd_mixer_elem_prev_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern void (*snd_mixer_elem_set_callback_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_elem_callback_t);
extern void* (*snd_mixer_elem_get_callback_private_dylibloader_wrapper_asound)(const snd_mixer_elem_t*);
extern void (*snd_mixer_elem_set_callback_private_dylibloader_wrapper_asound)( snd_mixer_elem_t*, void*);
extern snd_mixer_elem_type_t (*snd_mixer_elem_get_type_dylibloader_wrapper_asound)(const snd_mixer_elem_t*);
extern int (*snd_mixer_class_register_dylibloader_wrapper_asound)( snd_mixer_class_t*, snd_mixer_t*);
extern int (*snd_mixer_elem_new_dylibloader_wrapper_asound)( snd_mixer_elem_t**, snd_mixer_elem_type_t, int, void*, void*);
extern int (*snd_mixer_elem_add_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_class_t*);
extern int (*snd_mixer_elem_remove_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern void (*snd_mixer_elem_free_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_elem_info_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_elem_value_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_elem_attach_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_hctl_elem_t*);
extern int (*snd_mixer_elem_detach_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_hctl_elem_t*);
extern int (*snd_mixer_elem_empty_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern void* (*snd_mixer_elem_get_private_dylibloader_wrapper_asound)(const snd_mixer_elem_t*);
extern size_t (*snd_mixer_class_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_mixer_class_malloc_dylibloader_wrapper_asound)( snd_mixer_class_t**);
extern void (*snd_mixer_class_free_dylibloader_wrapper_asound)( snd_mixer_class_t*);
extern void (*snd_mixer_class_copy_dylibloader_wrapper_asound)( snd_mixer_class_t*,const snd_mixer_class_t*);
extern snd_mixer_t* (*snd_mixer_class_get_mixer_dylibloader_wrapper_asound)(const snd_mixer_class_t*);
extern snd_mixer_event_t (*snd_mixer_class_get_event_dylibloader_wrapper_asound)(const snd_mixer_class_t*);
extern void* (*snd_mixer_class_get_private_dylibloader_wrapper_asound)(const snd_mixer_class_t*);
extern snd_mixer_compare_t (*snd_mixer_class_get_compare_dylibloader_wrapper_asound)(const snd_mixer_class_t*);
extern int (*snd_mixer_class_set_event_dylibloader_wrapper_asound)( snd_mixer_class_t*, snd_mixer_event_t);
extern int (*snd_mixer_class_set_private_dylibloader_wrapper_asound)( snd_mixer_class_t*, void*);
extern int (*snd_mixer_class_set_private_free_dylibloader_wrapper_asound)( snd_mixer_class_t*, void*);
extern int (*snd_mixer_class_set_compare_dylibloader_wrapper_asound)( snd_mixer_class_t*, snd_mixer_compare_t);
extern const char* (*snd_mixer_selem_channel_name_dylibloader_wrapper_asound)( snd_mixer_selem_channel_id_t);
extern int (*snd_mixer_selem_register_dylibloader_wrapper_asound)( snd_mixer_t*,struct snd_mixer_selem_regopt*, snd_mixer_class_t**);
extern void (*snd_mixer_selem_get_id_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_id_t*);
extern const char* (*snd_mixer_selem_get_name_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern unsigned int (*snd_mixer_selem_get_index_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern snd_mixer_elem_t* (*snd_mixer_find_selem_dylibloader_wrapper_asound)( snd_mixer_t*,const snd_mixer_selem_id_t*);
extern int (*snd_mixer_selem_is_active_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_is_playback_mono_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_playback_channel_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t);
extern int (*snd_mixer_selem_is_capture_mono_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_capture_channel_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t);
extern int (*snd_mixer_selem_get_capture_group_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_common_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_playback_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_playback_volume_joined_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_capture_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_capture_volume_joined_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_common_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_playback_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_playback_switch_joined_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_capture_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_capture_switch_joined_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_has_capture_switch_exclusive_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_ask_playback_vol_dB_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, long*);
extern int (*snd_mixer_selem_ask_capture_vol_dB_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, long*);
extern int (*snd_mixer_selem_ask_playback_dB_vol_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, int, long*);
extern int (*snd_mixer_selem_ask_capture_dB_vol_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, int, long*);
extern int (*snd_mixer_selem_get_playback_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long*);
extern int (*snd_mixer_selem_get_capture_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long*);
extern int (*snd_mixer_selem_get_playback_dB_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long*);
extern int (*snd_mixer_selem_get_capture_dB_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long*);
extern int (*snd_mixer_selem_get_playback_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, int*);
extern int (*snd_mixer_selem_get_capture_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, int*);
extern int (*snd_mixer_selem_set_playback_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long);
extern int (*snd_mixer_selem_set_capture_volume_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long);
extern int (*snd_mixer_selem_set_playback_dB_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long, int);
extern int (*snd_mixer_selem_set_capture_dB_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, long, int);
extern int (*snd_mixer_selem_set_playback_volume_all_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long);
extern int (*snd_mixer_selem_set_capture_volume_all_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long);
extern int (*snd_mixer_selem_set_playback_dB_all_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, int);
extern int (*snd_mixer_selem_set_capture_dB_all_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, int);
extern int (*snd_mixer_selem_set_playback_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, int);
extern int (*snd_mixer_selem_set_capture_switch_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, int);
extern int (*snd_mixer_selem_set_playback_switch_all_dylibloader_wrapper_asound)( snd_mixer_elem_t*, int);
extern int (*snd_mixer_selem_set_capture_switch_all_dylibloader_wrapper_asound)( snd_mixer_elem_t*, int);
extern int (*snd_mixer_selem_get_playback_volume_range_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long*, long*);
extern int (*snd_mixer_selem_get_playback_dB_range_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long*, long*);
extern int (*snd_mixer_selem_set_playback_volume_range_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, long);
extern int (*snd_mixer_selem_get_capture_volume_range_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long*, long*);
extern int (*snd_mixer_selem_get_capture_dB_range_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long*, long*);
extern int (*snd_mixer_selem_set_capture_volume_range_dylibloader_wrapper_asound)( snd_mixer_elem_t*, long, long);
extern int (*snd_mixer_selem_is_enumerated_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_is_enum_playback_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_is_enum_capture_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_get_enum_items_dylibloader_wrapper_asound)( snd_mixer_elem_t*);
extern int (*snd_mixer_selem_get_enum_item_name_dylibloader_wrapper_asound)( snd_mixer_elem_t*, unsigned int, size_t, char*);
extern int (*snd_mixer_selem_get_enum_item_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, unsigned int*);
extern int (*snd_mixer_selem_set_enum_item_dylibloader_wrapper_asound)( snd_mixer_elem_t*, snd_mixer_selem_channel_id_t, unsigned int);
extern size_t (*snd_mixer_selem_id_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_mixer_selem_id_malloc_dylibloader_wrapper_asound)( snd_mixer_selem_id_t**);
extern void (*snd_mixer_selem_id_free_dylibloader_wrapper_asound)( snd_mixer_selem_id_t*);
extern void (*snd_mixer_selem_id_copy_dylibloader_wrapper_asound)( snd_mixer_selem_id_t*,const snd_mixer_selem_id_t*);
extern const char* (*snd_mixer_selem_id_get_name_dylibloader_wrapper_asound)(const snd_mixer_selem_id_t*);
extern unsigned int (*snd_mixer_selem_id_get_index_dylibloader_wrapper_asound)(const snd_mixer_selem_id_t*);
extern void (*snd_mixer_selem_id_set_name_dylibloader_wrapper_asound)( snd_mixer_selem_id_t*,const char*);
extern void (*snd_mixer_selem_id_set_index_dylibloader_wrapper_asound)( snd_mixer_selem_id_t*, unsigned int);
extern int (*snd_seq_open_dylibloader_wrapper_asound)( snd_seq_t**,const char*, int, int);
extern int (*snd_seq_open_lconf_dylibloader_wrapper_asound)( snd_seq_t**,const char*, int, int, snd_config_t*);
extern const char* (*snd_seq_name_dylibloader_wrapper_asound)( snd_seq_t*);
extern snd_seq_type_t (*snd_seq_type_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_close_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_poll_descriptors_count_dylibloader_wrapper_asound)( snd_seq_t*, short);
extern int (*snd_seq_poll_descriptors_dylibloader_wrapper_asound)( snd_seq_t*,struct pollfd*, unsigned int, short);
extern int (*snd_seq_poll_descriptors_revents_dylibloader_wrapper_asound)( snd_seq_t*,struct pollfd*, unsigned int, unsigned short*);
extern int (*snd_seq_nonblock_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_client_id_dylibloader_wrapper_asound)( snd_seq_t*);
extern size_t (*snd_seq_get_output_buffer_size_dylibloader_wrapper_asound)( snd_seq_t*);
extern size_t (*snd_seq_get_input_buffer_size_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_set_output_buffer_size_dylibloader_wrapper_asound)( snd_seq_t*, size_t);
extern int (*snd_seq_set_input_buffer_size_dylibloader_wrapper_asound)( snd_seq_t*, size_t);
extern size_t (*snd_seq_system_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_system_info_malloc_dylibloader_wrapper_asound)( snd_seq_system_info_t**);
extern void (*snd_seq_system_info_free_dylibloader_wrapper_asound)( snd_seq_system_info_t*);
extern void (*snd_seq_system_info_copy_dylibloader_wrapper_asound)( snd_seq_system_info_t*,const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_get_queues_dylibloader_wrapper_asound)(const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_get_clients_dylibloader_wrapper_asound)(const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_get_ports_dylibloader_wrapper_asound)(const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_get_channels_dylibloader_wrapper_asound)(const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_get_cur_clients_dylibloader_wrapper_asound)(const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_get_cur_queues_dylibloader_wrapper_asound)(const snd_seq_system_info_t*);
extern int (*snd_seq_system_info_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_system_info_t*);
extern size_t (*snd_seq_client_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_client_info_malloc_dylibloader_wrapper_asound)( snd_seq_client_info_t**);
extern void (*snd_seq_client_info_free_dylibloader_wrapper_asound)( snd_seq_client_info_t*);
extern void (*snd_seq_client_info_copy_dylibloader_wrapper_asound)( snd_seq_client_info_t*,const snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_client_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern snd_seq_client_type_t (*snd_seq_client_info_get_type_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern const char* (*snd_seq_client_info_get_name_dylibloader_wrapper_asound)( snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_broadcast_filter_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_error_bounce_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_card_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_pid_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern const unsigned char* (*snd_seq_client_info_get_event_filter_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_num_ports_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern int (*snd_seq_client_info_get_event_lost_dylibloader_wrapper_asound)(const snd_seq_client_info_t*);
extern void (*snd_seq_client_info_set_client_dylibloader_wrapper_asound)( snd_seq_client_info_t*, int);
extern void (*snd_seq_client_info_set_name_dylibloader_wrapper_asound)( snd_seq_client_info_t*,const char*);
extern void (*snd_seq_client_info_set_broadcast_filter_dylibloader_wrapper_asound)( snd_seq_client_info_t*, int);
extern void (*snd_seq_client_info_set_error_bounce_dylibloader_wrapper_asound)( snd_seq_client_info_t*, int);
extern void (*snd_seq_client_info_set_event_filter_dylibloader_wrapper_asound)( snd_seq_client_info_t*, unsigned char*);
extern void (*snd_seq_client_info_event_filter_clear_dylibloader_wrapper_asound)( snd_seq_client_info_t*);
extern void (*snd_seq_client_info_event_filter_add_dylibloader_wrapper_asound)( snd_seq_client_info_t*, int);
extern void (*snd_seq_client_info_event_filter_del_dylibloader_wrapper_asound)( snd_seq_client_info_t*, int);
extern int (*snd_seq_client_info_event_filter_check_dylibloader_wrapper_asound)( snd_seq_client_info_t*, int);
extern int (*snd_seq_get_client_info_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_client_info_t*);
extern int (*snd_seq_get_any_client_info_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_client_info_t*);
extern int (*snd_seq_set_client_info_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_client_info_t*);
extern int (*snd_seq_query_next_client_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_client_info_t*);
extern size_t (*snd_seq_client_pool_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_client_pool_malloc_dylibloader_wrapper_asound)( snd_seq_client_pool_t**);
extern void (*snd_seq_client_pool_free_dylibloader_wrapper_asound)( snd_seq_client_pool_t*);
extern void (*snd_seq_client_pool_copy_dylibloader_wrapper_asound)( snd_seq_client_pool_t*,const snd_seq_client_pool_t*);
extern int (*snd_seq_client_pool_get_client_dylibloader_wrapper_asound)(const snd_seq_client_pool_t*);
extern size_t (*snd_seq_client_pool_get_output_pool_dylibloader_wrapper_asound)(const snd_seq_client_pool_t*);
extern size_t (*snd_seq_client_pool_get_input_pool_dylibloader_wrapper_asound)(const snd_seq_client_pool_t*);
extern size_t (*snd_seq_client_pool_get_output_room_dylibloader_wrapper_asound)(const snd_seq_client_pool_t*);
extern size_t (*snd_seq_client_pool_get_output_free_dylibloader_wrapper_asound)(const snd_seq_client_pool_t*);
extern size_t (*snd_seq_client_pool_get_input_free_dylibloader_wrapper_asound)(const snd_seq_client_pool_t*);
extern void (*snd_seq_client_pool_set_output_pool_dylibloader_wrapper_asound)( snd_seq_client_pool_t*, size_t);
extern void (*snd_seq_client_pool_set_input_pool_dylibloader_wrapper_asound)( snd_seq_client_pool_t*, size_t);
extern void (*snd_seq_client_pool_set_output_room_dylibloader_wrapper_asound)( snd_seq_client_pool_t*, size_t);
extern int (*snd_seq_get_client_pool_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_client_pool_t*);
extern int (*snd_seq_set_client_pool_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_client_pool_t*);
extern size_t (*snd_seq_port_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_port_info_malloc_dylibloader_wrapper_asound)( snd_seq_port_info_t**);
extern void (*snd_seq_port_info_free_dylibloader_wrapper_asound)( snd_seq_port_info_t*);
extern void (*snd_seq_port_info_copy_dylibloader_wrapper_asound)( snd_seq_port_info_t*,const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_client_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_port_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern const snd_seq_addr_t* (*snd_seq_port_info_get_addr_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern const char* (*snd_seq_port_info_get_name_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern unsigned int (*snd_seq_port_info_get_capability_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern unsigned int (*snd_seq_port_info_get_type_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_midi_channels_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_midi_voices_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_synth_voices_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_read_use_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_write_use_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_port_specified_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_timestamping_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_timestamp_real_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern int (*snd_seq_port_info_get_timestamp_queue_dylibloader_wrapper_asound)(const snd_seq_port_info_t*);
extern void (*snd_seq_port_info_set_client_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_port_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_addr_dylibloader_wrapper_asound)( snd_seq_port_info_t*,const snd_seq_addr_t*);
extern void (*snd_seq_port_info_set_name_dylibloader_wrapper_asound)( snd_seq_port_info_t*,const char*);
extern void (*snd_seq_port_info_set_capability_dylibloader_wrapper_asound)( snd_seq_port_info_t*, unsigned int);
extern void (*snd_seq_port_info_set_type_dylibloader_wrapper_asound)( snd_seq_port_info_t*, unsigned int);
extern void (*snd_seq_port_info_set_midi_channels_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_midi_voices_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_synth_voices_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_port_specified_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_timestamping_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_timestamp_real_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern void (*snd_seq_port_info_set_timestamp_queue_dylibloader_wrapper_asound)( snd_seq_port_info_t*, int);
extern int (*snd_seq_create_port_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_port_info_t*);
extern int (*snd_seq_delete_port_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_get_port_info_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_port_info_t*);
extern int (*snd_seq_get_any_port_info_dylibloader_wrapper_asound)( snd_seq_t*, int, int, snd_seq_port_info_t*);
extern int (*snd_seq_set_port_info_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_port_info_t*);
extern int (*snd_seq_query_next_port_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_port_info_t*);
extern size_t (*snd_seq_port_subscribe_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_port_subscribe_malloc_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t**);
extern void (*snd_seq_port_subscribe_free_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*);
extern void (*snd_seq_port_subscribe_copy_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*,const snd_seq_port_subscribe_t*);
extern const snd_seq_addr_t* (*snd_seq_port_subscribe_get_sender_dylibloader_wrapper_asound)(const snd_seq_port_subscribe_t*);
extern const snd_seq_addr_t* (*snd_seq_port_subscribe_get_dest_dylibloader_wrapper_asound)(const snd_seq_port_subscribe_t*);
extern int (*snd_seq_port_subscribe_get_queue_dylibloader_wrapper_asound)(const snd_seq_port_subscribe_t*);
extern int (*snd_seq_port_subscribe_get_exclusive_dylibloader_wrapper_asound)(const snd_seq_port_subscribe_t*);
extern int (*snd_seq_port_subscribe_get_time_update_dylibloader_wrapper_asound)(const snd_seq_port_subscribe_t*);
extern int (*snd_seq_port_subscribe_get_time_real_dylibloader_wrapper_asound)(const snd_seq_port_subscribe_t*);
extern void (*snd_seq_port_subscribe_set_sender_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*,const snd_seq_addr_t*);
extern void (*snd_seq_port_subscribe_set_dest_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*,const snd_seq_addr_t*);
extern void (*snd_seq_port_subscribe_set_queue_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*, int);
extern void (*snd_seq_port_subscribe_set_exclusive_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*, int);
extern void (*snd_seq_port_subscribe_set_time_update_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*, int);
extern void (*snd_seq_port_subscribe_set_time_real_dylibloader_wrapper_asound)( snd_seq_port_subscribe_t*, int);
extern int (*snd_seq_get_port_subscription_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_port_subscribe_t*);
extern int (*snd_seq_subscribe_port_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_port_subscribe_t*);
extern int (*snd_seq_unsubscribe_port_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_port_subscribe_t*);
extern size_t (*snd_seq_query_subscribe_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_query_subscribe_malloc_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t**);
extern void (*snd_seq_query_subscribe_free_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*);
extern void (*snd_seq_query_subscribe_copy_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*,const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_client_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_port_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern const snd_seq_addr_t* (*snd_seq_query_subscribe_get_root_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern snd_seq_query_subs_type_t (*snd_seq_query_subscribe_get_type_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_index_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_num_subs_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern const snd_seq_addr_t* (*snd_seq_query_subscribe_get_addr_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_queue_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_exclusive_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_time_update_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern int (*snd_seq_query_subscribe_get_time_real_dylibloader_wrapper_asound)(const snd_seq_query_subscribe_t*);
extern void (*snd_seq_query_subscribe_set_client_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*, int);
extern void (*snd_seq_query_subscribe_set_port_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*, int);
extern void (*snd_seq_query_subscribe_set_root_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*,const snd_seq_addr_t*);
extern void (*snd_seq_query_subscribe_set_type_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*, snd_seq_query_subs_type_t);
extern void (*snd_seq_query_subscribe_set_index_dylibloader_wrapper_asound)( snd_seq_query_subscribe_t*, int);
extern int (*snd_seq_query_port_subscribers_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_query_subscribe_t*);
extern size_t (*snd_seq_queue_info_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_queue_info_malloc_dylibloader_wrapper_asound)( snd_seq_queue_info_t**);
extern void (*snd_seq_queue_info_free_dylibloader_wrapper_asound)( snd_seq_queue_info_t*);
extern void (*snd_seq_queue_info_copy_dylibloader_wrapper_asound)( snd_seq_queue_info_t*,const snd_seq_queue_info_t*);
extern int (*snd_seq_queue_info_get_queue_dylibloader_wrapper_asound)(const snd_seq_queue_info_t*);
extern const char* (*snd_seq_queue_info_get_name_dylibloader_wrapper_asound)(const snd_seq_queue_info_t*);
extern int (*snd_seq_queue_info_get_owner_dylibloader_wrapper_asound)(const snd_seq_queue_info_t*);
extern int (*snd_seq_queue_info_get_locked_dylibloader_wrapper_asound)(const snd_seq_queue_info_t*);
extern unsigned int (*snd_seq_queue_info_get_flags_dylibloader_wrapper_asound)(const snd_seq_queue_info_t*);
extern void (*snd_seq_queue_info_set_name_dylibloader_wrapper_asound)( snd_seq_queue_info_t*,const char*);
extern void (*snd_seq_queue_info_set_owner_dylibloader_wrapper_asound)( snd_seq_queue_info_t*, int);
extern void (*snd_seq_queue_info_set_locked_dylibloader_wrapper_asound)( snd_seq_queue_info_t*, int);
extern void (*snd_seq_queue_info_set_flags_dylibloader_wrapper_asound)( snd_seq_queue_info_t*, unsigned int);
extern int (*snd_seq_create_queue_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_queue_info_t*);
extern int (*snd_seq_alloc_named_queue_dylibloader_wrapper_asound)( snd_seq_t*,const char*);
extern int (*snd_seq_alloc_queue_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_free_queue_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_get_queue_info_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_info_t*);
extern int (*snd_seq_set_queue_info_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_info_t*);
extern int (*snd_seq_query_named_queue_dylibloader_wrapper_asound)( snd_seq_t*,const char*);
extern int (*snd_seq_get_queue_usage_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_set_queue_usage_dylibloader_wrapper_asound)( snd_seq_t*, int, int);
extern size_t (*snd_seq_queue_status_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_queue_status_malloc_dylibloader_wrapper_asound)( snd_seq_queue_status_t**);
extern void (*snd_seq_queue_status_free_dylibloader_wrapper_asound)( snd_seq_queue_status_t*);
extern void (*snd_seq_queue_status_copy_dylibloader_wrapper_asound)( snd_seq_queue_status_t*,const snd_seq_queue_status_t*);
extern int (*snd_seq_queue_status_get_queue_dylibloader_wrapper_asound)(const snd_seq_queue_status_t*);
extern int (*snd_seq_queue_status_get_events_dylibloader_wrapper_asound)(const snd_seq_queue_status_t*);
extern snd_seq_tick_time_t (*snd_seq_queue_status_get_tick_time_dylibloader_wrapper_asound)(const snd_seq_queue_status_t*);
extern const snd_seq_real_time_t* (*snd_seq_queue_status_get_real_time_dylibloader_wrapper_asound)(const snd_seq_queue_status_t*);
extern unsigned int (*snd_seq_queue_status_get_status_dylibloader_wrapper_asound)(const snd_seq_queue_status_t*);
extern int (*snd_seq_get_queue_status_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_status_t*);
extern size_t (*snd_seq_queue_tempo_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_queue_tempo_malloc_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t**);
extern void (*snd_seq_queue_tempo_free_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t*);
extern void (*snd_seq_queue_tempo_copy_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t*,const snd_seq_queue_tempo_t*);
extern int (*snd_seq_queue_tempo_get_queue_dylibloader_wrapper_asound)(const snd_seq_queue_tempo_t*);
extern unsigned int (*snd_seq_queue_tempo_get_tempo_dylibloader_wrapper_asound)(const snd_seq_queue_tempo_t*);
extern int (*snd_seq_queue_tempo_get_ppq_dylibloader_wrapper_asound)(const snd_seq_queue_tempo_t*);
extern unsigned int (*snd_seq_queue_tempo_get_skew_dylibloader_wrapper_asound)(const snd_seq_queue_tempo_t*);
extern unsigned int (*snd_seq_queue_tempo_get_skew_base_dylibloader_wrapper_asound)(const snd_seq_queue_tempo_t*);
extern void (*snd_seq_queue_tempo_set_tempo_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t*, unsigned int);
extern void (*snd_seq_queue_tempo_set_ppq_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t*, int);
extern void (*snd_seq_queue_tempo_set_skew_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t*, unsigned int);
extern void (*snd_seq_queue_tempo_set_skew_base_dylibloader_wrapper_asound)( snd_seq_queue_tempo_t*, unsigned int);
extern int (*snd_seq_get_queue_tempo_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_tempo_t*);
extern int (*snd_seq_set_queue_tempo_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_tempo_t*);
extern size_t (*snd_seq_queue_timer_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_queue_timer_malloc_dylibloader_wrapper_asound)( snd_seq_queue_timer_t**);
extern void (*snd_seq_queue_timer_free_dylibloader_wrapper_asound)( snd_seq_queue_timer_t*);
extern void (*snd_seq_queue_timer_copy_dylibloader_wrapper_asound)( snd_seq_queue_timer_t*,const snd_seq_queue_timer_t*);
extern int (*snd_seq_queue_timer_get_queue_dylibloader_wrapper_asound)(const snd_seq_queue_timer_t*);
extern snd_seq_queue_timer_type_t (*snd_seq_queue_timer_get_type_dylibloader_wrapper_asound)(const snd_seq_queue_timer_t*);
extern const snd_timer_id_t* (*snd_seq_queue_timer_get_id_dylibloader_wrapper_asound)(const snd_seq_queue_timer_t*);
extern unsigned int (*snd_seq_queue_timer_get_resolution_dylibloader_wrapper_asound)(const snd_seq_queue_timer_t*);
extern void (*snd_seq_queue_timer_set_type_dylibloader_wrapper_asound)( snd_seq_queue_timer_t*, snd_seq_queue_timer_type_t);
extern void (*snd_seq_queue_timer_set_id_dylibloader_wrapper_asound)( snd_seq_queue_timer_t*,const snd_timer_id_t*);
extern void (*snd_seq_queue_timer_set_resolution_dylibloader_wrapper_asound)( snd_seq_queue_timer_t*, unsigned int);
extern int (*snd_seq_get_queue_timer_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_timer_t*);
extern int (*snd_seq_set_queue_timer_dylibloader_wrapper_asound)( snd_seq_t*, int, snd_seq_queue_timer_t*);
extern int (*snd_seq_free_event_dylibloader_wrapper_asound)( snd_seq_event_t*);
extern ssize_t (*snd_seq_event_length_dylibloader_wrapper_asound)( snd_seq_event_t*);
extern int (*snd_seq_event_output_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_event_t*);
extern int (*snd_seq_event_output_buffer_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_event_t*);
extern int (*snd_seq_event_output_direct_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_event_t*);
extern int (*snd_seq_event_input_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_event_t**);
extern int (*snd_seq_event_input_pending_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_drain_output_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_event_output_pending_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_extract_output_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_event_t**);
extern int (*snd_seq_drop_output_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_drop_output_buffer_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_drop_input_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_drop_input_buffer_dylibloader_wrapper_asound)( snd_seq_t*);
extern size_t (*snd_seq_remove_events_sizeof_dylibloader_wrapper_asound)( void);
extern int (*snd_seq_remove_events_malloc_dylibloader_wrapper_asound)( snd_seq_remove_events_t**);
extern void (*snd_seq_remove_events_free_dylibloader_wrapper_asound)( snd_seq_remove_events_t*);
extern void (*snd_seq_remove_events_copy_dylibloader_wrapper_asound)( snd_seq_remove_events_t*,const snd_seq_remove_events_t*);
extern unsigned int (*snd_seq_remove_events_get_condition_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern int (*snd_seq_remove_events_get_queue_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern const snd_seq_timestamp_t* (*snd_seq_remove_events_get_time_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern const snd_seq_addr_t* (*snd_seq_remove_events_get_dest_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern int (*snd_seq_remove_events_get_channel_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern int (*snd_seq_remove_events_get_event_type_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern int (*snd_seq_remove_events_get_tag_dylibloader_wrapper_asound)(const snd_seq_remove_events_t*);
extern void (*snd_seq_remove_events_set_condition_dylibloader_wrapper_asound)( snd_seq_remove_events_t*, unsigned int);
extern void (*snd_seq_remove_events_set_queue_dylibloader_wrapper_asound)( snd_seq_remove_events_t*, int);
extern void (*snd_seq_remove_events_set_time_dylibloader_wrapper_asound)( snd_seq_remove_events_t*,const snd_seq_timestamp_t*);
extern void (*snd_seq_remove_events_set_dest_dylibloader_wrapper_asound)( snd_seq_remove_events_t*,const snd_seq_addr_t*);
extern void (*snd_seq_remove_events_set_channel_dylibloader_wrapper_asound)( snd_seq_remove_events_t*, int);
extern void (*snd_seq_remove_events_set_event_type_dylibloader_wrapper_asound)( snd_seq_remove_events_t*, int);
extern void (*snd_seq_remove_events_set_tag_dylibloader_wrapper_asound)( snd_seq_remove_events_t*, int);
extern int (*snd_seq_remove_events_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_remove_events_t*);
extern void (*snd_seq_set_bit_dylibloader_wrapper_asound)( int, void*);
extern void (*snd_seq_unset_bit_dylibloader_wrapper_asound)( int, void*);
extern int (*snd_seq_change_bit_dylibloader_wrapper_asound)( int, void*);
extern int (*snd_seq_get_bit_dylibloader_wrapper_asound)( int, void*);
extern int (*snd_seq_control_queue_dylibloader_wrapper_asound)( snd_seq_t*, int, int, int, snd_seq_event_t*);
extern int (*snd_seq_create_simple_port_dylibloader_wrapper_asound)( snd_seq_t*,const char*, unsigned int, unsigned int);
extern int (*snd_seq_delete_simple_port_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_connect_from_dylibloader_wrapper_asound)( snd_seq_t*, int, int, int);
extern int (*snd_seq_connect_to_dylibloader_wrapper_asound)( snd_seq_t*, int, int, int);
extern int (*snd_seq_disconnect_from_dylibloader_wrapper_asound)( snd_seq_t*, int, int, int);
extern int (*snd_seq_disconnect_to_dylibloader_wrapper_asound)( snd_seq_t*, int, int, int);
extern int (*snd_seq_set_client_name_dylibloader_wrapper_asound)( snd_seq_t*,const char*);
extern int (*snd_seq_set_client_event_filter_dylibloader_wrapper_asound)( snd_seq_t*, int);
extern int (*snd_seq_set_client_pool_output_dylibloader_wrapper_asound)( snd_seq_t*, size_t);
extern int (*snd_seq_set_client_pool_output_room_dylibloader_wrapper_asound)( snd_seq_t*, size_t);
extern int (*snd_seq_set_client_pool_input_dylibloader_wrapper_asound)( snd_seq_t*, size_t);
extern int (*snd_seq_sync_output_queue_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_parse_address_dylibloader_wrapper_asound)( snd_seq_t*, snd_seq_addr_t*,const char*);
extern int (*snd_seq_reset_pool_output_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_seq_reset_pool_input_dylibloader_wrapper_asound)( snd_seq_t*);
extern int (*snd_midi_event_new_dylibloader_wrapper_asound)( size_t, snd_midi_event_t**);
extern int (*snd_midi_event_resize_buffer_dylibloader_wrapper_asound)( snd_midi_event_t*, size_t);
extern void (*snd_midi_event_free_dylibloader_wrapper_asound)( snd_midi_event_t*);
extern void (*snd_midi_event_init_dylibloader_wrapper_asound)( snd_midi_event_t*);
extern void (*snd_midi_event_reset_encode_dylibloader_wrapper_asound)( snd_midi_event_t*);
extern void (*snd_midi_event_reset_decode_dylibloader_wrapper_asound)( snd_midi_event_t*);
extern void (*snd_midi_event_no_status_dylibloader_wrapper_asound)( snd_midi_event_t*, int);
extern long (*snd_midi_event_encode_dylibloader_wrapper_asound)( snd_midi_event_t*,const unsigned char*, long, snd_seq_event_t*);
extern int (*snd_midi_event_encode_byte_dylibloader_wrapper_asound)( snd_midi_event_t*, int, snd_seq_event_t*);
extern long (*snd_midi_event_decode_dylibloader_wrapper_asound)( snd_midi_event_t*, unsigned char*, long,const snd_seq_event_t*);
int initialize_asound(int verbose);
#ifdef __cplusplus
}
#endif
#endif
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

#ifdef _MSC_VER
#ifdef S_ISREG
#undef S_ISREG
#endif
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


// --- Merged from drivers/png/image_loader_png.cpp ---


#include "core/io/resource_loader.h"

Error ImageLoaderPNG::load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale) {
	const uint64_t buffer_size = f->get_length();
	Vector<uint8_t> file_buffer;
	Error err = file_buffer.resize(buffer_size);
	if (err) {
		return err;
	}
	{
		uint8_t *writer = file_buffer.ptrw();
		f->get_buffer(writer, buffer_size);
	}
	const uint8_t *reader = file_buffer.ptr();
	return PNGDriverCommon::png_to_image(reader, buffer_size, p_flags & FLAG_FORCE_LINEAR, p_image);
}

void ImageLoaderPNG::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("png");
}

Ref<Image> ImageLoaderPNG::load_mem_png(const uint8_t *p_png, int p_size) {
	Ref<Image> img;
	img.instantiate();

	// the value of p_force_linear does not matter since it only applies to 16 bit
	Error err = PNGDriverCommon::png_to_image(p_png, p_size, false, img);
	ERR_FAIL_COND_V(err, Ref<Image>());

	return img;
}

Ref<Image> ImageLoaderPNG::unpack_mem_png(const uint8_t *p_png, int p_size) {
	ERR_FAIL_COND_V(p_size < 4, Ref<Image>());
	ERR_FAIL_COND_V(p_png[0] != 'P' || p_png[1] != 'N' || p_png[2] != 'G' || p_png[3] != ' ', Ref<Image>());
	return load_mem_png(&p_png[4], p_size - 4);
}

Ref<Image> ImageLoaderPNG::lossless_unpack_png(const Vector<uint8_t> &p_data) {
	return unpack_mem_png(p_data.ptr(), p_data.size());
}

Vector<uint8_t> ImageLoaderPNG::lossless_pack_png(const Ref<Image> &p_image) {
	Vector<uint8_t> out_buffer;

	// add Godot's own "PNG " prefix
	if (out_buffer.resize(4) != OK) {
		ERR_FAIL_V(Vector<uint8_t>());
	}

	// scope for writer lifetime
	{
		// must be closed before call to image_to_png
		uint8_t *writer = out_buffer.ptrw();
		memcpy(writer, "PNG ", 4);
	}

	Error err = PNGDriverCommon::image_to_png(p_image, out_buffer);
	if (err) {
		ERR_FAIL_V(Vector<uint8_t>());
	}

	return out_buffer;
}

ImageLoaderPNG::ImageLoaderPNG() {
	Image::_png_mem_loader_func = load_mem_png;
	Image::_png_mem_unpacker_func = unpack_mem_png;
	Image::png_unpacker = lossless_unpack_png;
	Image::png_packer = lossless_pack_png;
}

// ============================================================================
// Merged from drivers/png/png_driver_common.cpp
// ============================================================================

#include "core/io/resource_loader.h"

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#endif

#include <png.h>

namespace PNGDriverCommon
{

// Print any warnings.
// On error, set explain and return true.
// Call should be wrapped in ERR_FAIL_COND
static bool check_error(const png_image& image)
{
	const png_uint_32 failed = PNG_IMAGE_FAILED(image);
	if (failed & PNG_IMAGE_ERROR) {
		return true;
	}
	else if (failed) {
#ifdef TOOLS_ENABLED
		// suppress this warning, to avoid log spam when opening assetlib
		const static char* const noisy = "iCCP: known incorrect sRGB profile";
		if (Engine::is_editor_hint() && !strcmp(image.message, noisy)) {
			return false;
		}
#endif
		WARN_PRINT(image.message);
	}
	return false;
}

Error png_to_image(const uint8_t* p_source, size_t p_size, bool p_force_linear, Ref<Image> p_image)
{
	png_image png_img;
	memset(&png_img, 0, sizeof(png_img));
	png_img.version = PNG_IMAGE_VERSION;

	// fetch image properties
	int success = png_image_begin_read_from_memory(&png_img, p_source, p_size);
	ERR_FAIL_COND_V_MSG(check_error(png_img), ERR_FILE_CORRUPT, png_img.message);
	ERR_FAIL_COND_V(!success, ERR_FILE_CORRUPT);

	// flags to be masked out of input format to give target format
	const png_uint_32 format_mask = ~(
		// convert component order to RGBA
		PNG_FORMAT_FLAG_BGR |
		PNG_FORMAT_FLAG_AFIRST
		// convert 16 bit components to 8 bit
		| PNG_FORMAT_FLAG_LINEAR
		// convert indexed image to direct color
		| PNG_FORMAT_FLAG_COLORMAP);

	png_img.format &= format_mask;

	Image::Format dest_format;
	switch (png_img.format) {
	case PNG_FORMAT_GRAY:
		dest_format = Image::FORMAT_L8;
		break;
	case PNG_FORMAT_GA:
		dest_format = Image::FORMAT_LA8;
		break;
	case PNG_FORMAT_RGB:
		dest_format = Image::FORMAT_RGB8;
		break;
	case PNG_FORMAT_RGBA:
		dest_format = Image::FORMAT_RGBA8;
		break;
	default:
		png_image_free(&png_img); // only required when we return before finish_read
		ERR_PRINT("Unsupported png format.");
		return ERR_UNAVAILABLE;
	}

	if (!p_force_linear) {
		// assume 16 bit pngs without sRGB or gAMA chunks are in sRGB format
		png_img.flags |= PNG_IMAGE_FLAG_16BIT_sRGB;
	}

	const png_uint_32 stride = PNG_IMAGE_ROW_STRIDE(png_img);
	Vector<uint8_t> buffer;
	Error err = buffer.resize(PNG_IMAGE_BUFFER_SIZE(png_img, stride));
	if (err) {
		png_image_free(&png_img); // only required when we return before finish_read
		return err;
	}
	uint8_t* writer = buffer.ptrw();

	// read image data to buffer and release libpng resources
	success = png_image_finish_read(&png_img, nullptr, writer, stride, nullptr);
	ERR_FAIL_COND_V_MSG(check_error(png_img), ERR_FILE_CORRUPT, png_img.message);
	ERR_FAIL_COND_V(!success, ERR_FILE_CORRUPT);

	// print_line("png width: "+itos(png_img.width)+" height: "+itos(png_img.height));
	p_image->set_data(png_img.width, png_img.height, false, dest_format, buffer);

	return OK;
}

} // namespace PNGDriverCommon



// ============================================================================
// Merged from drivers/png/resource_saver_png.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"

#include "scene/resources/image_texture.h"

Error ResourceSaverPNG::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Ref<ImageTexture> texture = p_resource;

	ERR_FAIL_COND_V_MSG(
		texture.is_null(), ERR_INVALID_PARAMETER, "Can't save invalid texture as PNG.");
	ERR_FAIL_COND_V_MSG(
		!texture->get_width(), ERR_INVALID_PARAMETER, "Can't save empty texture as PNG.");

	Ref<Image> img = texture->get_image();

	Error err = save_image(p_path, img);

	return err;
}

Error ResourceSaverPNG::save_image(const String& p_path, const Ref<Image>& p_img)
{
	Vector<uint8_t> buffer;
	Error err = PNGDriverCommon::image_to_png(p_img, buffer);
	ERR_FAIL_COND_V_MSG(err, err, "Can't convert image to PNG.");
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Can't save PNG at path: '%s'.", p_path));

	const uint8_t* reader = buffer.ptr();

	file->store_buffer(reader, buffer.size());
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Vector<uint8_t> ResourceSaverPNG::save_image_to_buffer(const Ref<Image>& p_img)
{
	Vector<uint8_t> buffer;
	Error err = PNGDriverCommon::image_to_png(p_img, buffer);
	ERR_FAIL_COND_V_MSG(err, Vector<uint8_t>(), "Can't convert image to PNG.");
	return buffer;
}

ResourceSaverPNG::ResourceSaverPNG()
{
	Image::save_png_func = &save_image;
	Image::save_png_buffer_func = &save_image_to_buffer;
}

Error PNGDriverCommon::image_to_png(Ref<Image> const&, Vector<unsigned char>&) { return OK; }

bool ResourceSaverPNG::recognize(const Ref<Resource>& p_resource) const { return false; }

void ResourceSaverPNG::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
}


// --- Merged from drivers/sdl/joypad_sdl.cpp ---
#if defined(SDL_ENABLED)
#include "core/input/input.h"

#ifdef SDL_ENABLED

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_joystick.h>
#include "core/input/input.h"

// Macro to skip the SDL joystick event handling if the device is an SDL gamepad, because
// there are separate events for SDL gamepads
#define SKIP_EVENT_FOR_GAMEPAD                                                                     \
	if (SDL_IsGamepad(sdl_event.jdevice.which)) {                                                  \
		continue;                                                                                  \
	}

JoypadSDL::~JoypadSDL()
{
	// Process any remaining input events
	process_events();
	for (int i = 0; i < Input::JOYPADS_MAX; i++) {
		if (joypads[i].attached) {
			close_joypad(i);
		}
	}
	SDL_Quit();
}

Error JoypadSDL::initialize()
{
	SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	ERR_FAIL_COND_V_MSG(!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD), FAILED, SDL_GetError());

	// Add Godot's mapping database from memory
	int i = 0;
	while (DefaultControllerMappings::mappings[i]) {
		String mapping_string = DefaultControllerMappings::mappings[i++];
		CharString data = mapping_string.utf8();
		SDL_IOStream* rw = SDL_IOFromMem((void*)data.ptr(), data.size());
		SDL_AddGamepadMappingsFromIO(rw, true);
	}

	// Make sure that we handle already connected joypads when the driver is initialized.
	process_events();

	print_verbose("SDL: Init OK!");
	return OK;
}

void JoypadSDL::process_events()
{
	// Update rumble first for it to be applied when we handle SDL events
	for (int i = 0; i < Input::JOYPADS_MAX; i++) {
		Joypad& joy = joypads[i];
		if (joy.attached && joy.supports_force_feedback) {
			uint64_t timestamp = Input::get_joy_vibration_timestamp(i);

			// Update the joypad rumble only if there was a new vibration request
			if (timestamp > joy.ff_effect_timestamp) {
				joy.ff_effect_timestamp = timestamp;

				SDL_Joystick* sdl_joy = SDL_GetJoystickFromID(joypads[i].sdl_instance_idx);
				Vector2 strength = Input::get_joy_vibration_strength(i);
				// Invalid values for strength are filtered by Input::start_joy_vibration().

				float duration = Input::get_joy_vibration_duration(i);
				Uint32 duration_ms = 0;
				if (duration < 0.0f) {
					continue; // Invalid duration.
				}
				else if (duration == 0.0f) {
					duration_ms = 0xFFFF; // SDL_MAX_RUMBLE_DURATION_MS
				}
				else {
					duration_ms = duration * 1000;
				}

				/*
					If the vibration was requested to start, SDL_RumbleJoystick will start it.
					If the vibration was requested to stop, strength and duration will be 0, so SDL
				   will stop the rumble.

					Here strength.y goes first and then strength.x, because
				   Input.get_joy_vibration_strength().x is vibration's weak magnitude (high
				   frequency rumble), and .y is strong magnitude (low frequency rumble),
					SDL_RumbleJoystick takes low frequency rumble first and then high frequency
				   rumble.

					Rumble strength goes from 0 to 0xFFFF.
				*/
				SDL_RumbleJoystick(
					sdl_joy, strength.y * UINT16_MAX, strength.x * UINT16_MAX, duration_ms);
			}
		}
	}

	SDL_Event sdl_event;
	while (SDL_PollEvent(&sdl_event)) {
		// A new joypad was attached
		if (sdl_event.type == SDL_EVENT_JOYSTICK_ADDED) {
			int joy_id = Input::get_unused_joy_id();
			if (joy_id == -1) {
				// There is no space for more joypads...
				print_error("A new joypad was attached but couldn't allocate a new id for it "
							"because joypad limit was reached.");
			}
			else {
				SDL_Joystick* joy = nullptr;
				SDL_Gamepad* gamepad = nullptr;
				String device_name;

				// Gamepads must be opened with SDL_OpenGamepad to get their special remapped events
				if (SDL_IsGamepad(sdl_event.jdevice.which)) {
					gamepad = SDL_OpenGamepad(sdl_event.jdevice.which);

					ERR_CONTINUE_MSG(!gamepad, vformat("Error opening gamepad at index %d: %s",
												   sdl_event.jdevice.which, SDL_GetError()));

					device_name = SDL_GetGamepadName(gamepad);
					joy = SDL_GetGamepadJoystick(gamepad);

					print_verbose(
						vformat("SDL: Gamepad %s connected", SDL_GetGamepadName(gamepad)));
				}
				else {
					joy = SDL_OpenJoystick(sdl_event.jdevice.which);
					ERR_CONTINUE_MSG(!joy, vformat("Error opening joystick at index %d: %s",
											   sdl_event.jdevice.which, SDL_GetError()));

					device_name = SDL_GetJoystickName(joy);

					print_verbose(vformat("SDL: Joystick %s connected", SDL_GetJoystickName(joy)));
				}

				const int MAX_GUID_SIZE = 64;
				char guid[MAX_GUID_SIZE] = {};
				SDL_GUID joy_guid = SDL_GetJoystickGUID(joy);
				SDL_GUIDToString(joy_guid, guid, MAX_GUID_SIZE);
				SDL_PropertiesID propertiesID = SDL_GetJoystickProperties(joy);

				joypads[joy_id].attached = true;
				joypads[joy_id].sdl_instance_idx = sdl_event.jdevice.which;
				joypads[joy_id].supports_force_feedback = SDL_GetBooleanProperty(
					propertiesID, SDL_PROP_JOYSTICK_CAP_RUMBLE_BOOLEAN, false);
				joypads[joy_id].guid = String(String(guid));
				joypads[joy_id].supports_motion_sensors =
					SDL_GamepadHasSensor(gamepad, SDL_SENSOR_ACCEL) ||
					SDL_GamepadHasSensor(gamepad, SDL_SENSOR_GYRO);

				sdl_instance_id_to_joypad_id.insert(sdl_event.jdevice.which, joy_id);

				const String serial = String(SDL_GetJoystickSerial(joy));

#if defined(WINDOWS_ENABLED) || defined(LINUXBSD_ENABLED) || defined(MACOS_ENABLED)
				const uint64_t steam_handle = SDL_GetGamepadSteamHandle(gamepad);
#endif

#ifdef WINDOWS_ENABLED
				const int player_index = SDL_GetJoystickPlayerIndex(joy);
				if (player_index >= 0 &&
					joy_guid.data[14] == 'x') { // See also "SDL_IsJoystickXInput" in
												// "thirdparty/sdl/joystick/SDL_joystick.c".
					// For XInput controllers SDL_GetJoystickPlayerIndex returns the XInput user
					// index.
					joypad_info["xinput_index"] = itos(player_index);
				}
#endif

				Input::set_joy_features(joy_id, &joypads[joy_id]);

				if (joypads[joy_id].supports_motion_sensors) {
					// Data rate for all sensors should be the same.
					Input::set_joy_motion_sensors_rate(
						joy_id, SDL_GetGamepadSensorDataRate(gamepad, SDL_SENSOR_ACCEL));
				}
			}
			// An event for an attached joypad
		}
		else if (sdl_event.type >= SDL_EVENT_JOYSTICK_AXIS_MOTION &&
				   sdl_event.type < SDL_EVENT_FINGER_DOWN &&
				   sdl_instance_id_to_joypad_id.has(sdl_event.jdevice.which)) {
			int joy_id = sdl_instance_id_to_joypad_id.get(sdl_event.jdevice.which);

			switch (sdl_event.type) {
			case SDL_EVENT_JOYSTICK_REMOVED:
				close_joypad(joy_id);
				break;

			case SDL_EVENT_JOYSTICK_AXIS_MOTION:
				SKIP_EVENT_FOR_GAMEPAD;

				Input::joy_axis(joy_id,
					static_cast<JoyAxis>(
						sdl_event.jaxis.axis), // Godot joy axis constants are already intentionally
											   // the same as SDL's
					((sdl_event.jaxis.value - SDL_JOYSTICK_AXIS_MIN) /
							(float)(SDL_JOYSTICK_AXIS_MAX - SDL_JOYSTICK_AXIS_MIN) -
						0.5f) *
						2.0f);
				break;

			case SDL_EVENT_JOYSTICK_BUTTON_UP:
			case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
				SKIP_EVENT_FOR_GAMEPAD;

				// Some devices report pressing buttons with indices like 232+, 241+, etc. that are
				// not valid, so we ignore them here.
				if (sdl_event.jbutton.button >= (int)JoyButton::MAX) {
					continue;
				}

				Input::joy_button(joy_id,
					static_cast<JoyButton>(
						sdl_event.jbutton
							.button), // Godot button constants are intentionally the same as SDL's,
									  // so we can just straight up use them
					sdl_event.jbutton.down);
				break;

			case SDL_EVENT_JOYSTICK_HAT_MOTION:
				SKIP_EVENT_FOR_GAMEPAD;

				Input::joy_hat(joy_id,
					(uint32_t)sdl_event.jhat.value // Godot hat masks are identical to SDL hat
												   // masks, so we can just use them as-is.
				);
				break;

			case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
				float axis_value;

				if (sdl_event.gaxis.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER ||
					sdl_event.gaxis.axis == SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) {
					// Gamepad triggers go from 0 to SDL_JOYSTICK_AXIS_MAX
					axis_value = sdl_event.gaxis.value / (float)SDL_JOYSTICK_AXIS_MAX;
				}
				else {
					// Other axis go from SDL_JOYSTICK_AXIS_MIN to SDL_JOYSTICK_AXIS_MAX
					axis_value = ((sdl_event.gaxis.value - SDL_JOYSTICK_AXIS_MIN) /
										 (float)(SDL_JOYSTICK_AXIS_MAX - SDL_JOYSTICK_AXIS_MIN) -
									 0.5f) *
								 2.0f;
				}

				Input::joy_axis(joy_id,
					static_cast<JoyAxis>(
						sdl_event.gaxis.axis), // Godot joy axis constants are already intentionally
											   // the same as SDL's
					axis_value);
			} break;

			// Do note SDL gamepads do not have separate events for the dpad
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				Input::joy_button(joy_id,
					static_cast<JoyButton>(
						sdl_event.gbutton
							.button), // Godot button constants are intentionally the same as SDL's,
									  // so we can just straight up use them
					sdl_event.gbutton.down);
				break;

			case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
			case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
			case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
				Input::joy_touchpad(joy_id, sdl_event.gtouchpad.touchpad,
					sdl_event.gtouchpad.finger,
					Vector2(sdl_event.gtouchpad.x, sdl_event.gtouchpad.y),
					sdl_event.gtouchpad.pressure, sdl_event.type != SDL_EVENT_GAMEPAD_TOUCHPAD_UP);
				break;
			}
		}
	}

	for (int i = 0; i < Input::JOYPADS_MAX; i++) {
		Joypad& joy = joypads[i];
		if (!joy.attached || !joy.supports_motion_sensors) {
			continue;
		}
		SDL_Gamepad* gamepad = SDL_GetGamepadFromID(joy.sdl_instance_idx);
		// gamepad should not be NULL since joy.supports_motion_sensors is true here.

		float accel_data[3];
		float gyro_data[3];
		SDL_GetGamepadSensorData(gamepad, SDL_SENSOR_ACCEL, accel_data, 3);
		SDL_GetGamepadSensorData(gamepad, SDL_SENSOR_GYRO, gyro_data, 3);

		Input::joy_motion_sensors(i,
			Vector3(-accel_data[0], -accel_data[1], -accel_data[2]),
			Vector3(gyro_data[0], gyro_data[1], gyro_data[2]));
	}
}

void JoypadSDL::close_joypad(int p_pad_idx)
{
	int sdl_instance_idx = joypads[p_pad_idx].sdl_instance_idx;

	joypads[p_pad_idx].attached = false;
	sdl_instance_id_to_joypad_id.erase(sdl_instance_idx);

	if (SDL_IsGamepad(sdl_instance_idx)) {
		SDL_Gamepad* gamepad = SDL_GetGamepadFromID(sdl_instance_idx);
		SDL_CloseGamepad(gamepad);
	}
	else {
		SDL_Joystick* joy = SDL_GetJoystickFromID(sdl_instance_idx);
		SDL_CloseJoystick(joy);
	}
}

bool JoypadSDL::Joypad::has_joy_light() const
{
	SDL_Joystick* joystick = get_sdl_joystick();
	SDL_PropertiesID properties_id = SDL_GetJoystickProperties(joystick);
	if (properties_id == 0) {
		return false;
	}
	return SDL_GetBooleanProperty(properties_id, SDL_PROP_JOYSTICK_CAP_RGB_LED_BOOLEAN, false) ||
		   SDL_GetBooleanProperty(properties_id, SDL_PROP_JOYSTICK_CAP_MONO_LED_BOOLEAN, false);
}

void JoypadSDL::Joypad::set_joy_light(const Color& p_color)
{
	SDL_SetJoystickLED(get_sdl_joystick(), p_color.get_r8(), p_color.get_g8(), p_color.get_b8());
}

bool JoypadSDL::Joypad::has_joy_motion_sensors() const { return supports_motion_sensors; }

void JoypadSDL::Joypad::set_joy_motion_sensors_enabled(bool p_enable)
{
	SDL_Gamepad* gamepad = get_sdl_gamepad();
	SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_ACCEL, p_enable);
	SDL_SetGamepadSensorEnabled(gamepad, SDL_SENSOR_GYRO, p_enable);
}

bool JoypadSDL::Joypad::has_joy_vibration() const { return supports_force_feedback; }

int JoypadSDL::Joypad::get_joy_num_touchpads() const
{
	return SDL_GetNumGamepadTouchpads(get_sdl_gamepad());
}

SDL_Joystick* JoypadSDL::Joypad::get_sdl_joystick() const
{
	return SDL_GetJoystickFromID(sdl_instance_idx);
}

SDL_Gamepad* JoypadSDL::Joypad::get_sdl_gamepad() const
{
	return SDL_GetGamepadFromID(sdl_instance_idx);
}

#endif // SDL_ENABLED
#endif // defined(SDL_ENABLED)


// --- Merged from drivers/unix/os_unix.cpp ---
#if defined(UNIX_ENABLED)
#include "drivers/unix/os_unix.h"

#ifdef UNIX_ENABLED

#include "drivers/unix/os_unix.h"
// --- Inlined from drivers/unix/file_access_unix_pipe.h ---
#if defined(UNIX_ENABLED)

#include "core/io/file_access.h"

class FileAccessUnixPipe : public FileAccess
{
	bool unlink_on_close = false;

	int fd[2] = {-1, -1};

	mutable Error last_error = OK;
	String path;
	String path_src;

	void _close();

public:
	Error open_existing(int p_rfd, int p_wfd, bool p_blocking);
	virtual Error open_internal(const String& p_path, int p_mode_flags) override; ///< open a file

	virtual bool is_open() const override; ///< true when file is open

	virtual String get_path() const override; /// returns the path for the current open file
	virtual String
	get_path_absolute() const override; /// returns the absolute path for the current open file

	virtual void seek(uint64_t p_position) override {}

	virtual void seek_end(int64_t p_position = 0) override {}

	virtual uint64_t get_position() const override { return 0; }

	virtual uint64_t get_length() const override;

	virtual bool eof_reached() const override { return false; }

	virtual uint64_t get_buffer(uint8_t* p_dst, uint64_t p_length) const override;

	virtual Error get_error() const override; ///< get last error

	virtual Error resize(int64_t p_length) override { return ERR_UNAVAILABLE; }

	virtual void flush() override {}

	virtual bool store_buffer(
		const uint8_t* p_src, uint64_t p_length) override; ///< store an array of bytes

	virtual bool file_exists(const String& p_path) override { return false; }

	virtual uint64_t _get_modified_time(const String& p_file) override { return 0; }

	virtual uint64_t _get_access_time(const String& p_file) override { return 0; }

	virtual int64_t _get_size(const String& p_file) override { return -1; }

	virtual uint32_t _get_unix_permissions(
		const String& p_file) override
	{
		return 0;
	}

	virtual Error _set_unix_permissions(
		const String& p_file, uint32_t p_permissions) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_hidden_attribute(const String& p_file) override { return false; }

	virtual Error _set_hidden_attribute(const String& p_file, bool p_hidden) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual bool _get_read_only_attribute(const String& p_file) override { return false; }

	virtual Error _set_read_only_attribute(const String& p_file, bool p_ro) override
	{
		return ERR_UNAVAILABLE;
	}

	virtual void close() override;

	FileAccessUnixPipe() {}

	virtual ~FileAccessUnixPipe();
};

#endif // UNIX_ENABLED



// --- End drivers/unix/file_access_unix_pipe.h ---
// --- Inlined from drivers/unix/thread_posix.h ---
void init_thread_posix();

// --- End drivers/unix/thread_posix.h ---

#ifndef UNIX_SOCKET_UNAVAILABLE
// --- Inlined from drivers/unix/ip_unix.h ---
#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)

#include "core/io/ip.h"

class IPUnix final
{
public:
	static void _resolve_hostname(
		List<IPAddress>& r_addresses, const String& p_hostname, IP::Type p_type = IP::TYPE_ANY);
	static void get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces);

	static void make_default();

	IPUnix() = delete;
	IPUnix(const IPUnix&) = delete;
	IPUnix& operator=(const IPUnix&) = delete;
	~IPUnix() = delete;
};

#endif // UNIX_ENABLED



// --- End drivers/unix/ip_unix.h ---
#include "drivers/unix/os_unix.h"
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#endif

#if defined(__FreeBSD__)
#include <kvm.h>
#endif

#if defined(__OpenBSD__)
#include <sys/swap.h>
#include <uvm/uvmexp.h>
#endif

#if defined(__NetBSD__)
#include <uvm/uvm_extern.h>
#endif

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <dlfcn.h>
#include <poll.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef RTLD_DEEPBIND
#define RTLD_DEEPBIND 0
#endif

#ifndef ASAN_ENABLED
#define GODOT_DLOPEN_MODE RTLD_NOW | RTLD_DEEPBIND
#else
#define GODOT_DLOPEN_MODE RTLD_NOW
#endif

#if defined(MACOS_ENABLED) || (defined(__ANDROID_API__) && __ANDROID_API__ >= 28)
// Random location for getentropy. Fitting.
#include <sys/random.h>
#define UNIX_GET_ENTROPY
#elif defined(__FreeBSD__) || defined(__OpenBSD__) ||                                              \
	(defined(__GLIBC_MINOR__) && (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 26))
// In <unistd.h>.
// One day... (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700)
// https://publications.opengroup.org/standards/unix/c211
#define UNIX_GET_ENTROPY
#endif

#if !defined(UNIX_GET_ENTROPY) && !defined(NO_URANDOM)
#include <fcntl.h>
#endif

/// Clock Setup function (used by get_ticks_usec)
static uint64_t _clock_start = 0;
#if defined(__APPLE__)
static double _clock_scale = 0;

static void _setup_clock()
{
	mach_timebase_info_data_t info;
	kern_return_t ret = mach_timebase_info(&info);
	ERR_FAIL_COND_MSG(ret != 0, "OS CLOCK IS NOT WORKING!");
	_clock_scale = ((double)info.numer / (double)info.denom) / 1000.0;
	_clock_start = mach_absolute_time() * _clock_scale;
}
#else
#if defined(CLOCK_MONOTONIC_RAW) && !defined(WEB_ENABLED) // This is a better clock on Linux.
#define GODOT_CLOCK CLOCK_MONOTONIC_RAW
#else
#define GODOT_CLOCK CLOCK_MONOTONIC
#endif
static void _setup_clock()
{
	struct timespec tv_now = {0, 0};
	ERR_FAIL_COND_MSG(clock_gettime(GODOT_CLOCK, &tv_now) != 0, "OS CLOCK IS NOT WORKING!");
	_clock_start = ((uint64_t)tv_now.tv_nsec / 1000L) + (uint64_t)tv_now.tv_sec * 1000000L;
}
#endif

struct sigaction old_action;

static void handle_interrupt(int sig)
{
	// Ensure we call the old action if it was configured.
	if (old_action.sa_handler && old_action.sa_handler != SIG_IGN &&
		old_action.sa_handler != SIG_DFL) {
		old_action.sa_handler(sig);
	}
}

void OS_Unix::initialize_debugging()
{
	struct sigaction action;
	memset(&action, 0, sizeof(action));
	action.sa_handler = handle_interrupt;
	sigaction(SIGINT, &action, &old_action);
}

int OS_Unix::unix_initialize_audio(int p_audio_driver) { return 0; }

void OS_Unix::initialize_core()
{
#ifdef THREADS_ENABLED
	init_thread_posix();
#endif

	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_RESOURCES);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_USERDATA);
	FileAccess::make_default<FileAccessUnix>(FileAccess::ACCESS_FILESYSTEM);
	FileAccess::make_default<FileAccessUnixPipe>(FileAccess::ACCESS_PIPE);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_RESOURCES);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_USERDATA);
	DirAccess::make_default<DirAccessUnix>(DirAccess::ACCESS_FILESYSTEM);

#ifndef UNIX_SOCKET_UNAVAILABLE
	NetSocketUnix::make_default();
	IPUnix::make_default();
#endif
	process_map = memnew((HashMap<ProcessID, ProcessInfo>));

	_setup_clock();
}

void OS_Unix::finalize_core()
{
    if (process_map) {
        memdelete(process_map);
        process_map = nullptr;
    }
#ifndef UNIX_SOCKET_UNAVAILABLE
	IP::finalize();
	IP::reset_backend();
	NetSocketUnix::cleanup();
#endif
}

Vector<String> OS_Unix::get_video_adapter_driver_info() { return Vector<String>(); }

String OS_Unix::get_stdin_string(int64_t p_buffer_size)
{
	Vector<uint8_t> data;
	data.resize(p_buffer_size);
	if (fgets((char*)data.ptrw(), data.size(), stdin)) {
		return String::utf8((char*)data.ptr()).replace("\r\n", "\n").rstrip("\n");
	}
	return String();
}

PackedByteArray OS_Unix::get_stdin_buffer(int64_t p_buffer_size)
{
	Vector<uint8_t> data;
	data.resize(p_buffer_size);
	size_t sz = fread((void*)data.ptrw(), 1, data.size(), stdin);
	if (sz > 0) {
		data.resize(sz);
		return data;
	}
	return PackedByteArray();
}

OS::StdHandleType OS_Unix::get_stdin_type()
{
	int h = fileno(stdin);
	if (h == -1) {
		return OS::STD_HANDLE_INVALID;
	}

	if (isatty(h)) {
		return OS::STD_HANDLE_CONSOLE;
	}
	struct stat statbuf;
	if (fstat(h, &statbuf) < 0) {
		return OS::STD_HANDLE_UNKNOWN;
	}
	if (S_ISFIFO(statbuf.st_mode)) {
		return OS::STD_HANDLE_PIPE;
	}
	else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		return OS::STD_HANDLE_FILE;
	}
	return OS::STD_HANDLE_UNKNOWN;
}

OS::StdHandleType OS_Unix::get_stdout_type()
{
	int h = fileno(stdout);
	if (h == -1) {
		return OS::STD_HANDLE_INVALID;
	}

	if (isatty(h)) {
		return OS::STD_HANDLE_CONSOLE;
	}
	struct stat statbuf;
	if (fstat(h, &statbuf) < 0) {
		return OS::STD_HANDLE_UNKNOWN;
	}
	if (S_ISFIFO(statbuf.st_mode)) {
		return OS::STD_HANDLE_PIPE;
	}
	else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		return OS::STD_HANDLE_FILE;
	}
	return OS::STD_HANDLE_UNKNOWN;
}

OS::StdHandleType OS_Unix::get_stderr_type()
{
	int h = fileno(stderr);
	if (h == -1) {
		return OS::STD_HANDLE_INVALID;
	}

	if (isatty(h)) {
		return OS::STD_HANDLE_CONSOLE;
	}
	struct stat statbuf;
	if (fstat(h, &statbuf) < 0) {
		return OS::STD_HANDLE_UNKNOWN;
	}
	if (S_ISFIFO(statbuf.st_mode)) {
		return OS::STD_HANDLE_PIPE;
	}
	else if (S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode)) {
		return OS::STD_HANDLE_FILE;
	}
	return OS::STD_HANDLE_UNKNOWN;
}

Error OS_Unix::get_entropy(uint8_t* r_buffer, int p_bytes)
{
#if defined(UNIX_GET_ENTROPY)
	int left = p_bytes;
	int ofs = 0;
	do {
		int chunk = MIN(left, 256);
		ERR_FAIL_COND_V(getentropy(r_buffer + ofs, chunk), FAILED);
		left -= chunk;
		ofs += chunk;
	} while (left > 0);
// Define this yourself if you don't want to fall back to /dev/urandom.
#elif !defined(NO_URANDOM)
	int r = open("/dev/urandom", O_RDONLY);
	ERR_FAIL_COND_V(r < 0, FAILED);
	int left = p_bytes;
	do {
		ssize_t ret = read(r, r_buffer, p_bytes);
		ERR_FAIL_COND_V(ret <= 0, FAILED);
		left -= ret;
	} while (left > 0);
#else
	return ERR_UNAVAILABLE;
#endif
	return OK;
}

String OS_Unix::get_name() { return "Unix"; }

String OS_Unix::get_distribution_name() { return ""; }

String OS_Unix::get_version() { return ""; }

String OS_Unix::get_temp_path() { return "/tmp"; }

double OS_Unix::get_unix_time()
{
	struct timeval tv_now;
	gettimeofday(&tv_now, nullptr);
	return (double)tv_now.tv_sec + double(tv_now.tv_usec) / 1000000;
}

OS::DateTime OS_Unix::get_datetime(bool p_utc)
{
	time_t t = time(nullptr);
	struct tm lt;
	if (p_utc) {
		gmtime_r(&t, &lt);
	}
	else {
		localtime_r(&t, &lt);
	}
	OS::DateTime ret;
	ret.year = 1900 + lt.tm_year;
	// Index starting at 1 to match OS_Unix::get_date
	//   and Windows SYSTEMTIME and tm_mon follows the typical structure
	//   of 0-11, noted here: http://www.cplusplus.com/reference/ctime/tm/
	ret.month = (Month)(lt.tm_mon + 1);
	ret.day = lt.tm_mday;
	ret.weekday = (Weekday)lt.tm_wday;
	ret.hour = lt.tm_hour;
	ret.minute = lt.tm_min;
	ret.second = lt.tm_sec;
	ret.dst = lt.tm_isdst;

	return ret;
}

OS::TimeZoneInfo OS_Unix::get_time_zone_info()
{
	time_t t = time(nullptr);
	struct tm lt;
	localtime_r(&t, &lt);
	char name[16];
	strftime(name, 16, "%Z", &lt);
	name[15] = 0;
	OS::TimeZoneInfo ret;
	ret.name = name;

	char bias_buf[16];
	strftime(bias_buf, 16, "%z", &lt);
	int bias;
	bias_buf[15] = 0;
	sscanf(bias_buf, "%d", &bias);

	// convert from ISO 8601 (1 minute=1, 1 hour=100) to minutes
	int hour = (int)bias / 100;
	int minutes = bias % 100;
	if (bias < 0) {
		ret.bias = hour * 60 - minutes;
	}
	else {
		ret.bias = hour * 60 + minutes;
	}

	return ret;
}

void OS_Unix::delay_usec(uint32_t p_usec)
{
	struct timespec requested = {
		static_cast<time_t>(p_usec / 1000000), (static_cast<long>(p_usec) % 1000000) * 1000};
	struct timespec remaining;
	while (nanosleep(&requested, &remaining) == -1 && errno == EINTR) {
		requested.tv_sec = remaining.tv_sec;
		requested.tv_nsec = remaining.tv_nsec;
	}
}

uint64_t OS_Unix::get_ticks_usec()
{
#if defined(__APPLE__)
	uint64_t longtime = mach_absolute_time() * _clock_scale;
#else
	// Unchecked return. Static analyzers might complain.
	// If _setup_clock() succeeded, we assume clock_gettime() works.
	struct timespec tv_now = {0, 0};
	clock_gettime(GODOT_CLOCK, &tv_now);
	uint64_t longtime = ((uint64_t)tv_now.tv_nsec / 1000L) + (uint64_t)tv_now.tv_sec * 1000000L;
#endif
	longtime -= _clock_start;

	return longtime;
}

#if !defined(__GLIBC__) && !defined(WEB_ENABLED)
void OS_Unix::_load_iconv()
{
#if defined(MACOS_ENABLED) || defined(IOS_ENABLED)
	String iconv_lib_aliases[] = {"/usr/lib/libiconv.2.dylib"};
	String iconv_func_aliases[] = {"iconv"};
	String charset_lib_aliases[] = {"/usr/lib/libcharset.1.dylib"};
#else
	String iconv_lib_aliases[] = {"", "libiconv.2.so", "libiconv.so"};
	String iconv_func_aliases[] = {"libiconv", "iconv", "bsd_iconv", "rpl_iconv"};
	String charset_lib_aliases[] = {"", "libcharset.1.so", "libcharset.so"};
#endif

	for (size_t i = 0; i < sizeof(iconv_lib_aliases) / sizeof(iconv_lib_aliases[0]); i++) {
		void* iconv_lib = iconv_lib_aliases[i].is_empty()
							  ? RTLD_NEXT
							  : dlopen(iconv_lib_aliases[i].utf8().get_data(), RTLD_NOW);
		if (iconv_lib) {
			for (size_t j = 0; j < sizeof(iconv_func_aliases) / sizeof(iconv_func_aliases[0]);
				 j++) {
				gd_iconv_open = (PIConvOpen)dlsym(
					iconv_lib, (iconv_func_aliases[j] + "_open").utf8().get_data());
				gd_iconv = (PIConv)dlsym(iconv_lib, (iconv_func_aliases[j]).utf8().get_data());
				gd_iconv_close = (PIConvClose)dlsym(
					iconv_lib, (iconv_func_aliases[j] + "_close").utf8().get_data());
				if (gd_iconv_open && gd_iconv && gd_iconv_close) {
					break;
				}
			}
			if (gd_iconv_open && gd_iconv && gd_iconv_close) {
				break;
			}
			if (!iconv_lib_aliases[i].is_empty()) {
				dlclose(iconv_lib);
			}
		}
	}

	for (size_t i = 0; i < sizeof(charset_lib_aliases) / sizeof(charset_lib_aliases[0]); i++) {
		void* cs_lib = charset_lib_aliases[i].is_empty()
						   ? RTLD_NEXT
						   : dlopen(charset_lib_aliases[i].utf8().get_data(), RTLD_NOW);
		if (cs_lib) {
			gd_locale_charset = (PIConvLocaleCharset)dlsym(cs_lib, "locale_charset");
			if (gd_locale_charset) {
				break;
			}
			if (!charset_lib_aliases[i].is_empty()) {
				dlclose(cs_lib);
			}
		}
	}
	_iconv_ok = gd_iconv_open && gd_iconv && gd_iconv_close && gd_locale_charset;
}
#endif

String OS_Unix::multibyte_to_string(const String& p_encoding, const PackedByteArray& p_array)
{
	ERR_FAIL_COND_V_MSG(!_iconv_ok, String(), "Conversion failed: Unable to load libiconv");

	LocalVector<char> chars;
#if defined(__GLIBC__) || defined(WEB_ENABLED)
	gd_iconv_t ctx = gd_iconv_open(
		"UTF-8", p_encoding.is_empty() ? nl_langinfo(CODESET) : p_encoding.utf8().get_data());
#else
	gd_iconv_t ctx = gd_iconv_open(
		"UTF-8", p_encoding.is_empty() ? gd_locale_charset() : p_encoding.utf8().get_data());
#endif
	ERR_FAIL_COND_V_MSG(ctx == (gd_iconv_t)(-1), String(), "Conversion failed: Unknown encoding");

	char* in_ptr = (char*)p_array.ptr();
	size_t in_size = p_array.size();

	chars.resize(in_size);
	char* out_ptr = (char*)chars.ptr();
	size_t out_size = chars.size();

	while (gd_iconv(ctx, &in_ptr, &in_size, &out_ptr, &out_size) == (size_t)-1) {
		if (errno != E2BIG) {
			gd_iconv_close(ctx);
			ERR_FAIL_V_MSG(String(), vformat("Conversion failed: %d - %s", errno, strerror(errno)));
		}
		int64_t rate = (chars.size()) / (p_array.size() - in_size);
		size_t oldpos = chars.size() - out_size;
		chars.resize(chars.size() + in_size * rate);
		out_ptr = (char*)chars.ptr() + oldpos;
		out_size = chars.size() - oldpos;
	}
	chars.resize(chars.size() - out_size);
	gd_iconv_close(ctx);

	return String::utf8((const char*)chars.ptr(), chars.size());
}

PackedByteArray OS_Unix::string_to_multibyte(const String& p_encoding, const String& p_string)
{
	ERR_FAIL_COND_V_MSG(
		!_iconv_ok, PackedByteArray(), "Conversion failed: Unable to load libiconv");

	CharString charstr = p_string.utf8();

	PackedByteArray ret;
#if defined(__GLIBC__) || defined(WEB_ENABLED)
	gd_iconv_t ctx = gd_iconv_open(
		p_encoding.is_empty() ? nl_langinfo(CODESET) : p_encoding.utf8().get_data(), "UTF-8");
#else
	gd_iconv_t ctx = gd_iconv_open(
		p_encoding.is_empty() ? gd_locale_charset() : p_encoding.utf8().get_data(), "UTF-8");
#endif
	ERR_FAIL_COND_V_MSG(
		ctx == (gd_iconv_t)(-1), PackedByteArray(), "Conversion failed: Unknown encoding");

	char* in_ptr = (char*)charstr.ptr();
	size_t in_size = charstr.size();

	ret.resize(in_size);
	char* out_ptr = (char*)ret.ptrw();
	size_t out_size = ret.size();

	while (gd_iconv(ctx, &in_ptr, &in_size, &out_ptr, &out_size) == (size_t)-1) {
		if (errno != E2BIG) {
			gd_iconv_close(ctx);
			ERR_FAIL_V_MSG(
				PackedByteArray(), vformat("Conversion failed: %d - %s", errno, strerror(errno)));
		}
		int64_t rate = (ret.size()) / (charstr.size() - in_size);
		size_t oldpos = ret.size() - out_size;
		ret.resize(ret.size() + in_size * rate);
		out_ptr = (char*)ret.ptrw() + oldpos;
		out_size = ret.size() - oldpos;
	}
	ret.resize(ret.size() - out_size);
	gd_iconv_close(ctx);

	return ret;
}

int OS_Unix::_wait_for_pid_completion(const pid_t p_pid, int* r_status, int p_options, pid_t* r_pid)
{
	while (true) {
		pid_t pid = waitpid(p_pid, r_status, p_options);
		if (pid != -1) {
			// When `p_options` has `WNOHANG`, 0 can be returned if the process is still running.
			if (r_pid) {
				*r_pid = pid;
			}
			return 0;
		}
		const int error = errno;
		if (error == EINTR) {
			// We're in a debugger, should call waitpid again.
			// See https://stackoverflow.com/a/45472920/730797.
			continue;
		}
		return error;
	}
}

bool OS_Unix::_check_pid_is_running(const pid_t p_pid, int* r_status)
{
	const ProcessInfo* pi = process_map->getptr(p_pid);

	if (pi && !pi->is_running) {
		// Can return cached value.
		if (r_status) {
			*r_status = pi->exit_code;
		}
		return false;
	}

	pid_t pid = -1;
	int status = 0;
	const int result = _wait_for_pid_completion(p_pid, &status, WNOHANG, &pid);
	if (result == 0 && pid == 0) {
		// Thread is still running.
		return true;
	}
	ERR_FAIL_COND_V_MSG(result != 0 && errno == ECHILD, false,
		vformat(
			"The process %d does not exist or is not a child of the calling process.", (int)p_pid));
	ERR_FAIL_COND_V_MSG(result != 0, false,
		vformat("waitpid for process %d failed with errno: %d", (int)p_pid, errno));

	// Thread exited normally.
	status = WIFEXITED(status) ? WEXITSTATUS(status) : status;

	if (pi) {
		pi->is_running = false;
		pi->exit_code = status;
	}

	if (r_status) {
		*r_status = status;
	}

	return false;
}

Error OS_Unix::execute(const String& p_path, const List<String>& p_arguments, String* r_pipe,
	int* r_exitcode, bool read_stderr, Mutex* p_pipe_mutex, bool p_open_console)
{
#ifdef __EMSCRIPTEN__
	// Don't compile this code at all to avoid undefined references.
	// Actual virtual call goes to OS_Web.
	ERR_FAIL_V(ERR_BUG);
#else
	if (r_pipe) {
		String command = "\"" + p_path + "\"";
		for (const String& arg : p_arguments) {
			command += String(" \"") + arg + "\"";
		}
		if (read_stderr) {
			command += " 2>&1"; // Include stderr
		}
		else {
			command += " 2>/dev/null"; // Silence stderr
		}

		FILE* f = popen(command.utf8().get_data(), "r");
		ERR_FAIL_NULL_V_MSG(f, ERR_CANT_OPEN, "Cannot create pipe from command: " + command + ".");
		char buf[65535];
		while (fgets(buf, 65535, f)) {
			if (p_pipe_mutex) {
				p_pipe_mutex->lock();
			}
			String pipe_out;
			if (pipe_out.append_utf8(buf) == OK) {
				(*r_pipe) += pipe_out;
			}
			else {
				(*r_pipe) += String(buf); // If not valid UTF-8 try decode as Latin-1
			}
			if (p_pipe_mutex) {
				p_pipe_mutex->unlock();
			}
		}
		int rv = pclose(f);

		if (r_exitcode) {
			*r_exitcode = WEXITSTATUS(rv);
		}
		return OK;
	}

	pid_t pid = fork();
	ERR_FAIL_COND_V(pid < 0, ERR_CANT_FORK);

	if (pid == 0) {
		// The child process
		Vector<CharString> cs;
		cs.push_back(p_path.utf8());
		for (const String& arg : p_arguments) {
			cs.push_back(arg.utf8());
		}

		Vector<char*> args;
		for (int i = 0; i < cs.size(); i++) {
			args.push_back((char*)cs[i].get_data());
		}
		args.push_back(0);

		execvp(p_path.utf8().get_data(), &args[0]);
		// The execvp() function only returns if an error occurs.
		fprintf(stderr, "Could not create child process: %s\n", p_path.utf8().get_data());
		raise(SIGKILL);
	}

	int status = 0;
	const int result = _wait_for_pid_completion(pid, &status, 0);
	if (r_exitcode) {
		*r_exitcode = WIFEXITED(status) ? WEXITSTATUS(status) : status;
	}
	return result ? FAILED : OK;
#endif
}

Error OS_Unix::create_process(const String& p_path, const List<String>& p_arguments,
	ProcessID* r_child_id, bool p_open_console)
{
#ifdef __EMSCRIPTEN__
	// Don't compile this code at all to avoid undefined references.
	// Actual virtual call goes to OS_Web.
	ERR_FAIL_V(ERR_BUG);
#else
	pid_t pid = fork();
	ERR_FAIL_COND_V(pid < 0, ERR_CANT_FORK);

	if (pid == 0) {
		// The new process
		// Create a new session-ID so parent won't wait for it.
		// This ensures the process won't go zombie at the end.
		setsid();

		Vector<CharString> cs;
		cs.push_back(p_path.utf8());
		for (const String& arg : p_arguments) {
			cs.push_back(arg.utf8());
		}

		Vector<char*> args;
		for (int i = 0; i < cs.size(); i++) {
			args.push_back((char*)cs[i].get_data());
		}
		args.push_back(0);

		execvp(p_path.utf8().get_data(), &args[0]);
		// The execvp() function only returns if an error occurs.
		fprintf(stderr, "Could not create child process: %s\n", p_path.utf8().get_data());
		raise(SIGKILL);
	}

	ProcessInfo pi;
	process_map_mutex.lock();
	process_map->insert(pid, pi);
	process_map_mutex.unlock();

	if (r_child_id) {
		*r_child_id = pid;
	}
	return OK;
#endif
}

Error OS_Unix::kill(const ProcessID& p_pid)
{
	int ret = ::kill(p_pid, SIGKILL);
	if (!ret) {
		// avoid zombie process
		int st;
		_wait_for_pid_completion(p_pid, &st, 0);
	}
	return ret ? ERR_INVALID_PARAMETER : OK;
}

int OS_Unix::get_process_id() { return getpid(); }

bool OS_Unix::is_process_running(const ProcessID& p_pid)
{
	MutexLock lock(process_map_mutex);
	return _check_pid_is_running(p_pid, nullptr);
}

int OS_Unix::get_process_exit_code(const ProcessID& p_pid)
{
	MutexLock lock(process_map_mutex);

	int exit_code = 0;
	if (_check_pid_is_running(p_pid, &exit_code)) {
		// Thread is still running
		return -1;
	}

	return exit_code;
}

String OS_Unix::get_locale()
{
	if (!has_environment("LANG")) {
		return "en";
	}

	String locale = get_environment("LANG");
	int tp = locale.find_char('.');
	if (tp != -1) {
		locale = locale.substr(0, tp);
	}
	return locale;
}

Error OS_Unix::open_dynamic_library(
	const String& p_path, void*& p_library_handle, OS::GDExtensionData* p_data)
{
	String path = p_path;

	if (FileAccess::exists(path) && path.is_relative_path()) {
		// dlopen expects a slash, in this case a leading ./ for it to be interpreted as a relative
		// path,
		//  otherwise it will end up searching various system directories for the lib instead and
		//  finally failing.
		path = "./" + path;
	}

	if (!FileAccess::exists(path)) {
		// This code exists so GDExtension can load .so files from within the executable path.
		path = get_executable_path().get_base_dir().path_join(p_path.get_file());
	}

	if (!FileAccess::exists(path)) {
		// This code exists so GDExtension can load .so files from a standard unix location.
		path =
			get_executable_path().get_base_dir().path_join("../lib").path_join(p_path.get_file());
	}

	ERR_FAIL_COND_V_MSG(!FileAccess::exists(path), ERR_FILE_NOT_FOUND,
		vformat("Can't open dynamic library, file not found: '%s'.", p_path));

	p_library_handle = dlopen(path.utf8().get_data(), GODOT_DLOPEN_MODE);
	ERR_FAIL_NULL_V_MSG(p_library_handle, ERR_CANT_OPEN,
		vformat("Can't open dynamic library: %s. Error: %s.", p_path, dlerror()));

	if (p_data != nullptr && p_data->r_resolved_path != nullptr) {
		*p_data->r_resolved_path = path;
	}

	return OK;
}

Error OS_Unix::close_dynamic_library(void* p_library_handle)
{
	if (dlclose(p_library_handle)) {
		return FAILED;
	}
	return OK;
}

Error OS_Unix::get_dynamic_library_symbol_handle(
	void* p_library_handle, const String& p_name, void*& p_symbol_handle, bool p_optional)
{
	const char* error;
	dlerror(); // Clear existing errors

	p_symbol_handle = dlsym(p_library_handle, p_name.utf8().get_data());

	error = dlerror();
	if (error != nullptr) {
		ERR_FAIL_COND_V_MSG(!p_optional, ERR_CANT_RESOLVE,
			"Can't resolve symbol " + p_name + ". Error: " + error + ".");

		return ERR_CANT_RESOLVE;
	}
	return OK;
}

Error OS_Unix::set_cwd(const String& p_cwd)
{
	if (chdir(p_cwd.utf8().get_data()) != 0) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

String OS_Unix::get_cwd()
{
	String dir;
	char real_current_dir_name[2048];
	ERR_FAIL_NULL_V(getcwd(real_current_dir_name, 2048), ".");
	if (dir.append_utf8(real_current_dir_name) != OK) {
		dir = real_current_dir_name;
	}
	return dir;
}

bool OS_Unix::has_environment(const String& p_var)
{
	return getenv(p_var.utf8().get_data()) != nullptr;
}

String OS_Unix::get_environment(const String& p_var)
{
	const char* val = getenv(p_var.utf8().get_data());
	if (val == nullptr) { // Not set; return empty string
		return "";
	}
	String s;
	if (s.append_utf8(val) == OK) {
		return s;
	}
	return String(val); // Not valid UTF-8, so return as-is
}

void OS_Unix::set_environment(const String& p_var, const String& p_value)
{
	ERR_FAIL_COND_MSG(p_var.is_empty() || p_var.contains_char('='),
		vformat("Invalid environment variable name '%s', cannot be empty or include '='.", p_var));
	int err = setenv(p_var.utf8().get_data(), p_value.utf8().get_data(), /* overwrite: */ 1);
	ERR_FAIL_COND_MSG(err != 0,
		vformat("Failed setting environment variable '%s', the system is out of memory.", p_var));
}

void OS_Unix::unset_environment(const String& p_var)
{
	ERR_FAIL_COND_MSG(p_var.is_empty() || p_var.contains_char('='),
		vformat("Invalid environment variable name '%s', cannot be empty or include '='.", p_var));
	unsetenv(p_var.utf8().get_data());
}

String OS_Unix::get_user_data_dir(const String& p_user_dir)
{
	return OS::get_data_path().path_join(p_user_dir);
}

String OS_Unix::get_executable_path()
{
#ifdef __linux__
	// fix for running from a symlink
	char buf[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
	String b;
	if (len > 0) {
		b.append_utf8(buf, len);
	}
	if (b.is_empty()) {
		WARN_PRINT("Couldn't get executable path from /proc/self/exe, using argv[0]");
		return OS::get_executable_path();
	}
	return b;
#elif defined(__OpenBSD__)
	char resolved_path[MAXPATHLEN];

	realpath(OS::get_executable_path().utf8().get_data(), resolved_path);

	return String(resolved_path);
#elif defined(__NetBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC_ARGS, -1, KERN_PROC_PATHNAME};
	char buf[MAXPATHLEN];
	size_t len = sizeof(buf);
	if (sysctl(mib, 4, buf, &len, nullptr, 0) != 0) {
		WARN_PRINT("Couldn't get executable path from sysctl");
		return OS::get_executable_path();
	}

	// NetBSD does not always return a normalized path. For example if argv[0] is "./a.out" then
	// executable path is "/home/netbsd/./a.out". Normalize with realpath:
	char resolved_path[MAXPATHLEN];

	realpath(buf, resolved_path);

	return String(resolved_path);
#elif defined(__FreeBSD__)
	int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
	char buf[MAXPATHLEN];
	size_t len = sizeof(buf);
	if (sysctl(mib, 4, buf, &len, nullptr, 0) != 0) {
		WARN_PRINT("Couldn't get executable path from sysctl");
		return OS::get_executable_path();
	}

	return String::utf8(buf);
#elif defined(__APPLE__)
	char temp_path[1];
	uint32_t buff_size = 1;
	_NSGetExecutablePath(temp_path, &buff_size);

	char* resolved_path = new char[buff_size + 1];

	if (_NSGetExecutablePath(resolved_path, &buff_size) == 1) {
		WARN_PRINT("MAXPATHLEN is too small");
	}

	String path = String::utf8(resolved_path);
	delete[] resolved_path;

	return path;
#else
	ERR_PRINT("Warning, don't know how to obtain executable path on this OS! Please override t
		his "
			  "function properly.");
	return OS::get_executable_path();
#endif
}

String OS_Unix::expand_path(const String& p_path)
{
	String path = p_path;

	if (path.begins_with("~/") || path == "~") {
		String home = get_environment("HOME");
		if (!home.is_empty()) {
			path = home + path.substr(1);
		}
	}

	return path;
}

UnixTerminalLogger::~UnixTerminalLogger() {}

void OS_Unix::initialize()
{
#if !defined(__GLIBC__) && !defined(WEB_ENABLED)
	_load_iconv();
#endif

	Vector<Logger*> loggers;
	loggers.push_back(memnew(UnixTerminalLogger));
	OS::_set_logger(memnew(CompositeLogger(loggers)));
}

#endif

#ifdef RTLD_DEEPBIND
#undef RTLD_DEEPBIND
#endif
#ifdef GODOT_DLOPEN_MODE
#undef GODOT_DLOPEN_MODE
#endif
#ifdef UNIX_GET_ENTROPY
#undef UNIX_GET_ENTROPY
#endif
#ifdef GODOT_CLOCK
#undef GODOT_CLOCK
#endif

// ============================================================================
// Merged from drivers/unix/dir_access_unix.cpp
// ============================================================================

#include "drivers/unix/os_unix.h"

#if defined(UNIX_ENABLED)

#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/templates/list.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/statfs.h>
#endif
#include <sys/statvfs.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#if __has_include(<mntent.h>)
#include <mntent.h>
#endif

String DirAccessUnix::fix_path(const String &p_path) const {
	return DirAccess::fix_path(p_path).simplify_path();
}

Error DirAccessUnix::list_dir_begin() {
	list_dir_end(); //close any previous dir opening!

	//char real_current_dir_name[2048]; //is this enough?!
	//getcwd(real_current_dir_name,2048);
	//chdir(current_path.utf8().get_data());
	dir_stream = opendir(current_dir.utf8().get_data());
	//chdir(real_current_dir_name);
	if (!dir_stream) {
		return ERR_CANT_OPEN; //error!
	}

	return OK;
}

bool DirAccessUnix::file_exists(String p_file) {
	GLOBAL_LOCK_FUNCTION

	if (p_file.is_relative_path()) {
		p_file = current_dir.path_join(p_file);
	}

	p_file = fix_path(p_file);

	struct stat flags = {};
	bool success = (stat(p_file.utf8().get_data(), &flags) == 0);

	if (success && S_ISDIR(flags.st_mode)) {
		success = false;
	}

	return success;
}

bool DirAccessUnix::dir_exists(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);

	struct stat flags = {};
	bool success = (stat(p_dir.utf8().get_data(), &flags) == 0);

	return (success && S_ISDIR(flags.st_mode));
}

bool DirAccessUnix::is_readable(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);
	return (access(p_dir.utf8().get_data(), R_OK) == 0);
}

bool DirAccessUnix::is_writable(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);
	return (access(p_dir.utf8().get_data(), W_OK) == 0);
}

uint64_t DirAccessUnix::get_modified_time(String p_file) {
	if (p_file.is_relative_path()) {
		p_file = current_dir.path_join(p_file);
	}

	p_file = fix_path(p_file);

	struct stat flags = {};
	bool success = (stat(p_file.utf8().get_data(), &flags) == 0);

	if (success) {
		return flags.st_mtime;
	} else {
		ERR_FAIL_V(0);
	}
	return 0;
}

String DirAccessUnix::get_next() {
	if (!dir_stream) {
		return "";
	}

	dirent *entry = readdir(dir_stream);

	if (entry == nullptr) {
		list_dir_end();
		return "";
	}

	String fname = fix_unicode_name(entry->d_name);

	// Look at d_type to determine if the entry is a directory, unless
	// its type is unknown (the file system does not support it) or if
	// the type is a link, in that case we want to resolve the link to
	// known if it points to a directory. stat() will resolve the link
	// for us.
	if (entry->d_type == DT_UNKNOWN || entry->d_type == DT_LNK) {
		String f = current_dir.path_join(fname);

		struct stat flags = {};
		if (stat(f.utf8().get_data(), &flags) == 0) {
			_cisdir = S_ISDIR(flags.st_mode);
		} else {
			_cisdir = false;
		}
	} else {
		_cisdir = (entry->d_type == DT_DIR);
	}

	_cishidden = is_hidden(fname);

	return fname;
}

bool DirAccessUnix::current_is_dir() const {
	return _cisdir;
}

bool DirAccessUnix::current_is_hidden() const {
	return _cishidden;
}

void DirAccessUnix::list_dir_end() {
	if (dir_stream) {
		closedir(dir_stream);
	}
	dir_stream = nullptr;
	_cisdir = false;
}

#if __has_include(<mntent.h>) && defined(LINUXBSD_ENABLED)
static bool _filter_drive(struct mntent *mnt) {
	// Ignore devices that don't point to /dev
	if (strncmp(mnt->mnt_fsname, "/dev", 4) != 0) {
		return false;
	}

	// Accept devices mounted at common locations
	if (strncmp(mnt->mnt_dir, "/media", 6) == 0 ||
			strncmp(mnt->mnt_dir, "/mnt", 4) == 0 ||
			strncmp(mnt->mnt_dir, "/home", 5) == 0 ||
			strncmp(mnt->mnt_dir, "/run/media", 10) == 0) {
		return true;
	}

	// Ignore everything else
	return false;
}
#endif

static void _get_drives(List<String> *list) {
	// Add root.
	list->push_back("/");

#if __has_include(<mntent.h>) && defined(LINUXBSD_ENABLED)
	// Check /etc/mtab for the list of mounted partitions.
	FILE *mtab = setmntent("/etc/mtab", "r");
	if (mtab) {
		struct mntent mnt;
		char strings[4096];

		while (getmntent_r(mtab, &mnt, strings, sizeof(strings))) {
			if (mnt.mnt_dir != nullptr && _filter_drive(&mnt)) {
				// Avoid duplicates
				String name = String::utf8(mnt.mnt_dir);
				if (!list->find(name)) {
					list->push_back(name);
				}
			}
		}

		endmntent(mtab);
	}
#endif

	// Add $HOME.
	const char *home = getenv("HOME");
	if (home) {
		// Only add if it's not a duplicate
		String home_name = String::utf8(home);
		if (!list->find(home_name)) {
			list->push_back(home_name);
		}

		// Check GTK+3 bookmarks for both XDG locations (Documents, Downloads, etc.)
		// and potential user-defined bookmarks.
		char path[1024];
		snprintf(path, 1024, "%s/.config/gtk-3.0/bookmarks", home);
		FILE *fd = fopen(path, "r");
		if (fd) {
			char string[1024];
			while (fgets(string, 1024, fd)) {
				// Parse only file:// links
				if (strncmp(string, "file://", 7) == 0) {
					// Strip any unwanted edges on the strings and push_back if it's not a duplicate.
					String fpath = String::utf8(string + 7).strip_edges().split_spaces()[0].uri_file_decode();
					if (!list->find(fpath)) {
						list->push_back(fpath);
					}
				}
			}

			fclose(fd);
		}

		// Add Desktop dir.
		String dpath = OS::get_system_dir(OS::SystemDir::SYSTEM_DIR_DESKTOP);
		if (dpath.length() > 0 && !list->find(dpath)) {
			list->push_back(dpath);
		}
	}

	list->sort();
}

int DirAccessUnix::get_drive_count() {
	List<String> list;
	_get_drives(&list);

	return list.size();
}

String DirAccessUnix::get_drive(int p_drive) {
	List<String> list;
	_get_drives(&list);

	ERR_FAIL_INDEX_V(p_drive, list.size(), "");

	return list.get(p_drive);
}

int DirAccessUnix::get_current_drive() {
	int drive = 0;
	int max_length = -1;
	const String path = get_current_dir().to_lower();
	for (int i = 0; i < get_drive_count(); i++) {
		const String d = get_drive(i).to_lower();
		if (max_length < d.length() && path.begins_with(d)) {
			max_length = d.length();
			drive = i;
		}
	}
	return drive;
}

bool DirAccessUnix::drives_are_shortcuts() {
	return true;
}

Error DirAccessUnix::make_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	if (p_dir.is_relative_path()) {
		p_dir = get_current_dir().path_join(p_dir);
	}

	p_dir = fix_path(p_dir);

	bool success = (mkdir(p_dir.utf8().get_data(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0);
	int err = errno;

	if (success) {
		return OK;
	}

	if (err == EEXIST) {
		return ERR_ALREADY_EXISTS;
	}

	return ERR_CANT_CREATE;
}

Error DirAccessUnix::change_dir(String p_dir) {
	GLOBAL_LOCK_FUNCTION

	p_dir = fix_path(p_dir);

	// prev_dir is the directory we are changing out of
	String prev_dir;
	char real_current_dir_name[2048];
	ERR_FAIL_NULL_V(getcwd(real_current_dir_name, 2048), ERR_BUG);
	if (prev_dir.append_utf8(real_current_dir_name) != OK) {
		prev_dir = real_current_dir_name; //no utf8, maybe latin?
	}

	// try_dir is the directory we are trying to change into
	String try_dir = "";
	if (p_dir.is_relative_path()) {
		String next_dir = current_dir.path_join(p_dir);
		next_dir = next_dir.simplify_path();
		try_dir = next_dir;
	} else {
		try_dir = p_dir;
	}

	bool worked = (chdir(try_dir.utf8().get_data()) == 0); // we can only give this utf8
	if (!worked) {
		return ERR_INVALID_PARAMETER;
	}

	String base = _get_root_path();
	if (!base.is_empty() && !try_dir.begins_with(base)) {
		ERR_FAIL_NULL_V(getcwd(real_current_dir_name, 2048), ERR_BUG);
		String new_dir;
		new_dir.append_utf8(real_current_dir_name);

		if (!new_dir.begins_with(base)) {
			try_dir = current_dir; //revert
		}
	}

	// the directory exists, so set current_dir to try_dir
	current_dir = try_dir;
	ERR_FAIL_COND_V(chdir(prev_dir.utf8().get_data()) != 0, ERR_BUG);
	return OK;
}

String DirAccessUnix::get_current_dir(bool p_include_drive) const {
	String base = _get_root_path();
	if (!base.is_empty()) {
		String bd = current_dir.replace_first(base, "");
		if (bd.begins_with("/")) {
			return _get_root_string() + bd.substr(1);
		} else {
			return _get_root_string() + bd;
		}
	}
	return current_dir;
}

Error DirAccessUnix::rename(String p_path, String p_new_path) {
	if (p_path.is_relative_path()) {
		p_path = get_current_dir().path_join(p_path);
	}

	p_path = fix_path(p_path);
	if (p_path.ends_with("/")) {
		p_path = p_path.left(-1);
	}

	if (p_new_path.is_relative_path()) {
		p_new_path = get_current_dir().path_join(p_new_path);
	}

	p_new_path = fix_path(p_new_path);
	if (p_new_path.ends_with("/")) {
		p_new_path = p_new_path.left(-1);
	}

	int res = ::rename(p_path.utf8().get_data(), p_new_path.utf8().get_data());
	if (res != 0 && errno == EXDEV) { // Cross-device move, use copy and remove.
		Error err = OK;
		err = copy(p_path, p_new_path);
		if (err != OK) {
			return err;
		}
		return remove(p_path);
	} else {
		return (res == 0) ? OK : FAILED;
	}
}

Error DirAccessUnix::remove(String p_path) {
	if (p_path.is_relative_path()) {
		p_path = get_current_dir().path_join(p_path);
	}

	p_path = fix_path(p_path);
	if (p_path.ends_with("/")) {
		p_path = p_path.left(-1);
	}

	struct stat flags = {};
	if (lstat(p_path.utf8().get_data(), &flags) != 0) {
		return FAILED;
	}

	int err;
	if (S_ISDIR(flags.st_mode) && !is_link(p_path)) {
		err = ::rmdir(p_path.utf8().get_data());
	} else {
		err = ::unlink(p_path.utf8().get_data());
	}
	if (err != 0) {
		return FAILED;
	}
	if (remove_notification_func != nullptr) {
		remove_notification_func(p_path);
	}
	return OK;
}

bool DirAccessUnix::is_link(String p_file) {
	if (p_file.is_relative_path()) {
		p_file = get_current_dir().path_join(p_file);
	}

	p_file = fix_path(p_file);
	if (p_file.ends_with("/")) {
		p_file = p_file.left(-1);
	}

	struct stat flags = {};
	if (lstat(p_file.utf8().get_data(), &flags) != 0) {
		return false;
	}

	return S_ISLNK(flags.st_mode);
}

String DirAccessUnix::read_link(String p_file) {
	if (p_file.is_relative_path()) {
		p_file = get_current_dir().path_join(p_file);
	}

	p_file = fix_path(p_file);
	if (p_file.ends_with("/")) {
		p_file = p_file.left(-1);
	}

	char buf[PATH_MAX];
	memset(buf, 0, PATH_MAX);
	ssize_t len = readlink(p_file.utf8().get_data(), buf, sizeof(buf));
	String link;
	if (len > 0) {
		link.append_utf8(buf, len);
	}
	return link;
}

Error DirAccessUnix::create_link(String p_source, String p_target) {
	if (p_target.is_relative_path()) {
		p_target = get_current_dir().path_join(p_target);
	}

	p_source = fix_path(p_source);
	p_target = fix_path(p_target);

	if (symlink(p_source.utf8().get_data(), p_target.utf8().get_data()) == 0) {
		return OK;
	} else {
		return FAILED;
	}
}

uint64_t DirAccessUnix::get_space_left() {
	struct statvfs vfs;
	if (statvfs(current_dir.utf8().get_data(), &vfs) != 0) {
		return 0;
	}

	return (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize;
}

String DirAccessUnix::get_filesystem_type() const {
#ifdef __linux__
	struct statfs fs;
	if (statfs(current_dir.utf8().get_data(), &fs) != 0) {
		return "";
	}
	switch (static_cast<unsigned int>(fs.f_type)) {
		case 0x0000adf5:
			return "ADFS";
		case 0x0000adff:
			return "AFFS";
		case 0x5346414f:
			return "AFS";
		case 0x00000187:
			return "AUTOFS";
		case 0x00c36400:
			return "CEPH";
		case 0x73757245:
			return "CODA";
		case 0x28cd3d45:
			return "CRAMFS";
		case 0x453dcd28:
			return "CRAMFS";
		case 0x64626720:
			return "DEBUGFS";
		case 0x73636673:
			return "SECURITYFS";
		case 0xf97cff8c:
			return "SELINUX";
		case 0x43415d53:
			return "SMACK";
		case 0x858458f6:
			return "RAMFS";
		case 0x01021994:
			return "TMPFS";
		case 0x958458f6:
			return "HUGETLBFS";
		case 0x73717368:
			return "SQUASHFS";
		case 0x0000f15f:
			return "ECRYPTFS";
		case 0x00414a53:
			return "EFS";
		case 0xe0f5e1e2:
			return "EROFS";
		case 0x0000ef53:
			return "EXTFS";
		case 0xabba1974:
			return "XENFS";
		case 0x9123683e:
			return "BTRFS";
		case 0x00003434:
			return "NILFS";
		case 0xf2f52010:
			return "F2FS";
		case 0xf995e849:
			return "HPFS";
		case 0x00009660:
			return "ISOFS";
		case 0x000072b6:
			return "JFFS2";
		case 0x58465342:
			return "XFS";
		case 0x6165676c:
			return "PSTOREFS";
		case 0xde5e81e4:
			return "EFIVARFS";
		case 0x00c0ffee:
			return "HOSTFS";
		case 0x794c7630:
			return "OVERLAYFS";
		case 0x65735546:
			return "FUSE";
		case 0xca451a4e:
			return "BCACHEFS";
		case 0x00004d44:
			return "FAT32";
		case 0x2011bab0:
			return "EXFAT";
		case 0x0000564c:
			return "NCP";
		case 0x00006969:
			return "NFS";
		case 0x7461636f:
			return "OCFS2";
		case 0x00009fa1:
			return "OPENPROM";
		case 0x0000002f:
			return "QNX4";
		case 0x68191122:
			return "QNX6";
		case 0x6b414653:
			return "AFS";
		case 0x52654973:
			return "REISERFS";
		case 0x0000517b:
			return "SMB";
		case 0xff534d42:
			return "CIFS";
		case 0x0027e0eb:
			return "CGROUP";
		case 0x63677270:
			return "CGROUP2";
		case 0x07655821:
			return "RDTGROUP";
		case 0x74726163:
			return "TRACEFS";
		case 0x01021997:
			return "V9FS";
		case 0x62646576:
			return "BDEVFS";
		case 0x64646178:
			return "DAXFS";
		case 0x42494e4d:
			return "BINFMTFS";
		case 0x00001cd1:
			return "DEVPTS";
		case 0x6c6f6f70:
			return "BINDERFS";
		case 0x0bad1dea:
			return "FUTEXFS";
		case 0x50495045:
			return "PIPEFS";
		case 0x00009fa0:
			return "PROC";
		case 0x534f434b:
			return "SOCKFS";
		case 0x62656572:
			return "SYSFS";
		case 0x00009fa2:
			return "USBDEVICE";
		case 0x11307854:
			return "MTD_INODE";
		case 0x09041934:
			return "ANON_INODE";
		case 0x73727279:
			return "BTRFS";
		case 0x6e736673:
			return "NSFS";
		case 0xcafe4a11:
			return "BPF_FS";
		case 0x5a3c69f0:
			return "AAFS";
		case 0x5a4f4653:
			return "ZONEFS";
		case 0x15013346:
			return "UDF";
		case 0x444d4142:
			return "DMA_BUF";
		case 0x454d444d:
			return "DEVMEM";
		case 0x5345434d:
			return "SECRETMEM";
		case 0x50494446:
			return "PID_FS";
		default:
			return "";
	}
#else
	return ""; //TODO this should be implemented
#endif
}

bool DirAccessUnix::is_hidden(const String &p_name) {
	return p_name != "." && p_name != ".." && p_name.begins_with(".");
}

bool DirAccessUnix::is_case_sensitive(const String &p_path) const {
#if defined(LINUXBSD_ENABLED)
	String f = p_path;
	if (!f.is_absolute_path()) {
		f = get_current_dir().path_join(f);
	}
	f = fix_path(f);

	int fd = ::open(f.utf8().get_data(), O_RDONLY | O_NONBLOCK);
	if (fd) {
		long flags = 0;
		if (ioctl(fd, _IOR('f', 1, long), &flags) >= 0) {
			::close(fd);
			return !(flags & 0x40000000 /* FS_CASEFOLD_FL */);
		}
		::close(fd);
	}
#endif
	return true;
}

bool DirAccessUnix::is_equivalent(const String &p_path_a, const String &p_path_b) const {
	String f1 = fix_path(p_path_a);
	struct stat st1 = {};
	int err = stat(f1.utf8().get_data(), &st1);
	if (err) {
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}

	String f2 = fix_path(p_path_b);
	struct stat st2 = {};
	err = stat(f2.utf8().get_data(), &st2);
	if (err) {
		return DirAccess::is_equivalent(p_path_a, p_path_b);
	}

	return (st1.st_dev == st2.st_dev) && (st1.st_ino == st2.st_ino);
}

DirAccessUnix::DirAccessUnix() {
	dir_stream = nullptr;
	_cisdir = false;

	/* determine drive count */

	// set current directory to an absolute path of the current directory
	char real_current_dir_name[2048];
	ERR_FAIL_NULL(getcwd(real_current_dir_name, 2048));
	current_dir.clear();
	if (current_dir.append_utf8(real_current_dir_name) != OK) {
		current_dir = real_current_dir_name;
	}

	change_dir(current_dir);
}

DirAccessUnix::RemoveNotificationFunc DirAccessUnix::remove_notification_func = nullptr;

DirAccessUnix::~DirAccessUnix() {
	list_dir_end();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/file_access_unix.cpp
// ============================================================================

#include "drivers/unix/os_unix.h"

#if defined(UNIX_ENABLED)

#include "core/string/ustring.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(WEB_ENABLED)
#include <sys/xattr.h>
#endif
#include <unistd.h>

#include <cerrno>
#include <cstdlib>

void FileAccessUnix::check_errors(bool p_write) const {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");

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

Error FileAccessUnix::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	path = fix_path(p_path);
	//printf("opening %s, %i\n", path.utf8().get_data(), Memory::get_static_mem_usage());

	ERR_FAIL_COND_V_MSG(f, ERR_ALREADY_IN_USE, "File is already in use.");
	const char *mode_string;

	if (p_mode_flags == READ) {
		mode_string = "rb";
	} else if (p_mode_flags == WRITE) {
		mode_string = "wb";
	} else if (p_mode_flags == READ_WRITE) {
		mode_string = "rb+";
	} else if (p_mode_flags == WRITE_READ) {
		mode_string = "wb+";
	} else {
		return ERR_INVALID_PARAMETER;
	}

	/* pretty much every implementation that uses fopen as primary
	   backend (unix-compatible mostly) supports utf8 encoding */

	//printf("opening %s as %s\n", p_path.utf8().get_data(), path.utf8().get_data());
	struct stat st = {};
	int err = stat(path.utf8().get_data(), &st);
	if (!err) {
		switch (st.st_mode & S_IFMT) {
			case S_IFLNK:
			case S_IFREG:
				break;
			default:
				return ERR_FILE_CANT_OPEN;
		}
	}

#if defined(TOOLS_ENABLED)
	if (p_mode_flags & READ) {
		String real_path = get_real_path();
		if (real_path != path) {
			// Don't warn on symlinks, since they can be used to simply share addons on multiple projects.
			if (real_path.to_lower() == path.to_lower()) {
				// The File system is case insensitive, but other platforms can be sensitive to it
				// To ease cross-platform development, we issue a warning if users try to access
				// a file using the wrong case (which *works* on Windows and macOS, but won't on other
				// platforms).
				WARN_PRINT(vformat("Case mismatch opening requested file '%s', stored as '%s' in the filesystem. This file will not open when exported to other case-sensitive platforms.", path, real_path));
			}
		}
	}
#endif

	if (is_backup_save_enabled() && (p_mode_flags == WRITE)) {
		// Set save path to the symlink target, not the link itself.
		String link;
		bool is_link = false;
		{
			CharString cs = path.utf8();
			struct stat lst = {};
			if (lstat(cs.get_data(), &lst) == 0) {
				is_link = S_ISLNK(lst.st_mode);
			}
			if (is_link) {
				char buf[PATH_MAX];
				memset(buf, 0, PATH_MAX);
				ssize_t len = readlink(cs.get_data(), buf, sizeof(buf));
				if (len > 0) {
					link.append_utf8(buf, len);
				}
				if (!link.is_absolute_path()) {
					link = path.get_base_dir().path_join(link);
				}
			}
		}
		save_path = is_link ? link : path;

		// Create a temporary file in the same directory as the target file.
		path = path + "-XXXXXX";
		CharString cs = path.utf8();
		int fd = mkstemp(cs.ptrw());
		if (fd == -1) {
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}

		struct stat file_stat = {};
		int error = stat(save_path.utf8().get_data(), &file_stat);
		if (!error) {
			fchmod(fd, file_stat.st_mode & 0xFFF); // Mask to remove file type
		} else {
			fchmod(fd, 0644); // Fallback permissions
		}

		path = String::utf8(cs.ptr());

		f = fdopen(fd, mode_string);
		if (f == nullptr) {
			// Delete temp file and close descriptor if open failed.
			::unlink(cs.ptr());
			::close(fd);
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}
	} else {
		f = fopen(path.utf8().get_data(), mode_string);
	}

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
	}

	// Set close on exec to avoid leaking it to subprocesses.
	int fd = fileno(f);

	if (fd != -1) {
		int opts = fcntl(fd, F_GETFD);
		fcntl(fd, F_SETFD, opts | FD_CLOEXEC);
	}

	last_error = OK;
	flags = p_mode_flags;
	return OK;
}

void FileAccessUnix::_close() {
	if (!f) {
		return;
	}

	fclose(f);
	f = nullptr;

	if (close_notification_func) {
		close_notification_func(path, flags);
	}

	if (!save_path.is_empty()) {
		int rename_error = rename(path.utf8().get_data(), save_path.utf8().get_data());

		if (rename_error && close_fail_notify) {
			close_fail_notify(save_path);
		}

		save_path = "";
		ERR_FAIL_COND(rename_error != 0);
	}
}

bool FileAccessUnix::is_open() const {
	return (f != nullptr);
}

String FileAccessUnix::get_path() const {
	return path_src;
}

String FileAccessUnix::get_path_absolute() const {
	return path;
}

#if defined(TOOLS_ENABLED)
String FileAccessUnix::get_real_path() const {
	char *resolved_path = ::realpath(path.utf8().get_data(), nullptr);

	if (!resolved_path) {
		return path;
	}

	String result;
	Error parse_ok = result.append_utf8(resolved_path);
	::free(resolved_path);

	if (parse_ok != OK) {
		return path;
	}

	return result.simplify_path();
}
#endif

void FileAccessUnix::seek(uint64_t p_position) {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");

	if (fseeko(f, p_position, SEEK_SET)) {
		check_errors();
	}
}

void FileAccessUnix::seek_end(int64_t p_position) {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");

	if (fseeko(f, p_position, SEEK_END)) {
		check_errors();
	}
}

uint64_t FileAccessUnix::get_position() const {
	ERR_FAIL_NULL_V_MSG(f, 0, "File must be opened before use.");

	int64_t pos = ftello(f);
	if (pos < 0) {
		check_errors();
		ERR_FAIL_V(0);
	}
	return pos;
}

uint64_t FileAccessUnix::get_length() const {
	ERR_FAIL_NULL_V_MSG(f, 0, "File must be opened before use.");

	int64_t pos = ftello(f);
	ERR_FAIL_COND_V(pos < 0, 0);
	ERR_FAIL_COND_V(fseeko(f, 0, SEEK_END), 0);
	int64_t size = ftello(f);
	ERR_FAIL_COND_V(size < 0, 0);
	ERR_FAIL_COND_V(fseeko(f, pos, SEEK_SET), 0);

	return size;
}

bool FileAccessUnix::eof_reached() const {
	return feof(f);
}

uint64_t FileAccessUnix::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_NULL_V_MSG(f, -1, "File must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	uint64_t read = fread(p_dst, 1, p_length, f);
	check_errors();

	return read;
}

Error FileAccessUnix::get_error() const {
	return last_error;
}

Error FileAccessUnix::resize(int64_t p_length) {
	ERR_FAIL_NULL_V_MSG(f, FAILED, "File must be opened before use.");
	int res = ::ftruncate(fileno(f), p_length);
	switch (res) {
		case 0:
			return OK;
		case EBADF:
			return ERR_FILE_CANT_OPEN;
		case EFBIG:
			return ERR_OUT_OF_MEMORY;
		case EINVAL:
			return ERR_INVALID_PARAMETER;
		default:
			return FAILED;
	}
}

void FileAccessUnix::flush() {
	ERR_FAIL_NULL_MSG(f, "File must be opened before use.");
	fflush(f);
}

bool FileAccessUnix::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_NULL_V_MSG(f, false, "File must be opened before use.");
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);
	bool res = fwrite(p_src, 1, p_length, f) == p_length;
	check_errors(true);
	return res;
}

bool FileAccessUnix::file_exists(const String &p_path) {
	struct stat st = {};
	const CharString filename_utf8 = fix_path(p_path).utf8();

	// Does the name exist at all?
	if (stat(filename_utf8.get_data(), &st)) {
		return false;
	}

	// See if we have access to the file
	if (access(filename_utf8.get_data(), F_OK)) {
		return false;
	}

	// See if this is a regular file
	switch (st.st_mode & S_IFMT) {
		case S_IFLNK:
		case S_IFREG:
			return true;
		default:
			return false;
	}
}

uint64_t FileAccessUnix::_get_modified_time(const String &p_file) {
	String file = fix_path(p_file);
	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);

	if (!err) {
		uint64_t modified_time = 0;
		if ((st.st_mode & S_IFMT) == S_IFLNK || (st.st_mode & S_IFMT) == S_IFREG || (st.st_mode & S_IFDIR) == S_IFDIR) {
			modified_time = st.st_mtime;
		}
#ifdef ANDROID_ENABLED
		// Workaround for GH-101007
		//FIXME: After saving, all timestamps (st_mtime, st_ctime, st_atime) are set to the same value.
		// After exporting or after some time, only 'modified_time' resets to a past timestamp.
		uint64_t created_time = st.st_ctime;
		if (modified_time < created_time) {
			modified_time = created_time;
		}
#endif
		return modified_time;
	} else {
		return 0;
	}
}

uint64_t FileAccessUnix::_get_access_time(const String &p_file) {
	String file = fix_path(p_file);
	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);

	if (!err) {
		if ((st.st_mode & S_IFMT) == S_IFLNK || (st.st_mode & S_IFMT) == S_IFREG || (st.st_mode & S_IFDIR) == S_IFDIR) {
			return st.st_atime;
		}
	}
	ERR_FAIL_V_MSG(0, "Failed to get access time for: " + p_file + "");
}

int64_t FileAccessUnix::_get_size(const String &p_file) {
	String file = fix_path(p_file);
	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);

	if (!err) {
		if ((st.st_mode & S_IFMT) == S_IFLNK || (st.st_mode & S_IFMT) == S_IFREG) {
			return st.st_size;
		}
	}
	ERR_FAIL_V_MSG(-1, "Failed to get size for: " + p_file + "");
}

uint32_t FileAccessUnix::_get_unix_permissions(const String &p_file) {
	String file = fix_path(p_file);
	struct stat status = {};
	int err = stat(file.utf8().get_data(), &status);

	if (!err) {
		return status.st_mode & 0xFFF; //only permissions
	} else {
		ERR_FAIL_V_MSG(0, "Failed to get unix permissions for: " + p_file + ".");
	}
}

Error FileAccessUnix::_set_unix_permissions(const String &p_file, uint32_t p_permissions) {
	String file = fix_path(p_file);

	int err = chmod(file.utf8().get_data(), p_permissions);
	if (!err) {
		return OK;
	}

	return FAILED;
}

bool FileAccessUnix::_get_hidden_attribute(const String &p_file) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, false, "Failed to get attributes for: " + p_file);

	return (st.st_flags & UF_HIDDEN);
#else
	return false;
#endif
}

Error FileAccessUnix::_set_hidden_attribute(const String &p_file, bool p_hidden) {
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to get attributes for: " + p_file);

	if (p_hidden) {
		err = chflags(file.utf8().get_data(), st.st_flags | UF_HIDDEN);
	} else {
		err = chflags(file.utf8().get_data(), st.st_flags & ~UF_HIDDEN);
	}
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to set attributes for: " + p_file);
	return OK;
#else
	return ERR_UNAVAILABLE;
#endif
}

bool FileAccessUnix::_get_read_only_attribute(const String &p_file) {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, false, "Failed to get attributes for: " + p_file);

	return st.st_flags & UF_IMMUTABLE;
#else
	return false;
#endif
}

Error FileAccessUnix::_set_read_only_attribute(const String &p_file, bool p_ro) {
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
	String file = fix_path(p_file);

	struct stat st = {};
	int err = stat(file.utf8().get_data(), &st);
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to get attributes for: " + p_file);

	if (p_ro) {
		err = chflags(file.utf8().get_data(), st.st_flags | UF_IMMUTABLE);
	} else {
		err = chflags(file.utf8().get_data(), st.st_flags & ~UF_IMMUTABLE);
	}
	ERR_FAIL_COND_V_MSG(err, FAILED, "Failed to set attributes for: " + p_file);
	return OK;
#else
	return ERR_UNAVAILABLE;
#endif
}

PackedByteArray FileAccessUnix::_get_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), PackedByteArray());

	PackedByteArray data;
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	CharString attr_name = p_attribute_name.utf8();
	ssize_t attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), nullptr, 0, 0, 0);
	if (attr_size <= 0) {
		return PackedByteArray();
	}

	data.resize(attr_size);
	attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), (void *)data.ptrw(), data.size(), 0, 0);
	ERR_FAIL_COND_V_MSG(attr_size != data.size(), PackedByteArray(), "Failed to set extended attributes for: " + p_file);
#else
	String file = fix_path(p_file);
	CharString attr_name = ("user." + p_attribute_name).utf8();
	ssize_t attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), nullptr, 0);
	if (attr_size <= 0) {
		return PackedByteArray();
	}

	data.resize(attr_size);
	attr_size = getxattr(file.utf8().get_data(), attr_name.get_data(), (void *)data.ptrw(), data.size());
	ERR_FAIL_COND_V_MSG(attr_size != data.size(), PackedByteArray(), "Failed to set extended attributes for: " + p_file);
#endif
	return data;
}

Error FileAccessUnix::_set_extended_attribute(const String &p_file, const String &p_attribute_name, const PackedByteArray &p_data) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	int err = setxattr(file.utf8().get_data(), p_attribute_name.utf8().get_data(), (const void *)p_data.ptr(), p_data.size(), 0, 0);
	if (err != 0) {
		return FAILED;
	}
#else
	String file = fix_path(p_file);
	int err = setxattr(file.utf8().get_data(), ("user." + p_attribute_name).utf8().get_data(), (const void *)p_data.ptr(), p_data.size(), 0);
	if (err != 0) {
		return FAILED;
	}
#endif
	return OK;
}

Error FileAccessUnix::_remove_extended_attribute(const String &p_file, const String &p_attribute_name) {
	ERR_FAIL_COND_V(p_attribute_name.is_empty(), FAILED);

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	int err = removexattr(file.utf8().get_data(), p_attribute_name.utf8().get_data(), 0);
	if (err != 0) {
		return FAILED;
	}
#else
	String file = fix_path(p_file);
	int err = removexattr(file.utf8().get_data(), ("user." + p_attribute_name).utf8().get_data());
	if (err != 0) {
		return FAILED;
	}
#endif
	return OK;
}

PackedStringArray FileAccessUnix::_get_extended_attributes_list(const String &p_file) {
	PackedStringArray ret;
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(WEB_ENABLED)
	// Not supported.
#elif defined(__APPLE__)
	String file = fix_path(p_file);
	size_t size = listxattr(file.utf8().get_data(), nullptr, 0, 0);
	if (size > 0) {
		PackedByteArray data;
		data.resize(size);
		listxattr(file.utf8().get_data(), (char *)data.ptrw(), data.size(), 0);
		int64_t start = 0;
		for (int64_t x = 0; x < data.size(); x++) {
			if (x != start && data[x] == 0) {
				ret.push_back(String::utf8((const char *)(data.ptr() + start), x - start));
				start = x + 1;
			}
		}
	}
#else
	String file = fix_path(p_file);
	size_t size = listxattr(file.utf8().get_data(), nullptr, 0);
	if (size > 0) {
		PackedByteArray data;
		data.resize(size);
		listxattr(file.utf8().get_data(), (char *)data.ptrw(), data.size());
		int64_t start = 0;
		for (int64_t x = 0; x < data.size(); x++) {
			if (x != start && data[x] == 0) {
				String name = String::utf8((const char *)(data.ptr() + start), x - start);
				if (name.begins_with("user.")) {
					ret.push_back(name.trim_prefix("user."));
				}
				start = x + 1;
			}
		}
	}
#endif
	return ret;
}

void FileAccessUnix::close() {
	_close();
}

FileAccessUnix::CloseNotificationFunc FileAccessUnix::close_notification_func = nullptr;

FileAccessUnix::~FileAccessUnix() {
	_close();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/file_access_unix_pipe.cpp
// ============================================================================


#if defined(UNIX_ENABLED)

#include "core/string/ustring.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>

#ifndef _GNU_SOURCE
typedef typeof(void(int)) *sighandler_t;
#endif

Error FileAccessUnixPipe::open_existing(int p_rfd, int p_wfd, bool p_blocking) {
	// Open pipe using handles created by pipe(fd) call in the OS.execute_with_pipe.
	_close();

	path_src = String();
	unlink_on_close = false;
	ERR_FAIL_COND_V_MSG(fd[0] >= 0 || fd[1] >= 0, ERR_ALREADY_IN_USE, "Pipe is already in use.");
	fd[0] = p_rfd;
	fd[1] = p_wfd;

	if (!p_blocking) {
		fcntl(fd[0], F_SETFL, fcntl(fd[0], F_GETFL) | O_NONBLOCK);
		fcntl(fd[1], F_SETFL, fcntl(fd[1], F_GETFL) | O_NONBLOCK);
	}

	last_error = OK;
	return OK;
}

Error FileAccessUnixPipe::open_internal(const String &p_path, int p_mode_flags) {
	_close();

	path_src = p_path;
	ERR_FAIL_COND_V_MSG(fd[0] >= 0 || fd[1] >= 0, ERR_ALREADY_IN_USE, "Pipe is already in use.");

	path = String("/tmp/") + p_path.replace("pipe://", "").replace_char('/', '_');
	const CharString path_utf8 = path.utf8();

	struct stat st = {};
	int err = stat(path_utf8.get_data(), &st);
	if (err) {
		if (mkfifo(path_utf8.get_data(), 0600) != 0) {
			last_error = ERR_FILE_CANT_OPEN;
			return last_error;
		}
		unlink_on_close = true;
	} else {
		ERR_FAIL_COND_V_MSG(!S_ISFIFO(st.st_mode), ERR_ALREADY_IN_USE, "Pipe name is already used by file.");
	}

	int f = ::open(path_utf8.get_data(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
	if (f < 0) {
		switch (errno) {
			case ENOENT: {
				last_error = ERR_FILE_NOT_FOUND;
			} break;
			default: {
				last_error = ERR_FILE_CANT_OPEN;
			} break;
		}
		return last_error;
	}

	// Set close on exec to avoid leaking it to subprocesses.
	fd[0] = f;
	fd[1] = f;

	last_error = OK;
	return OK;
}

void FileAccessUnixPipe::_close() {
	if (fd[0] < 0) {
		return;
	}

	if (fd[1] != fd[0]) {
		::close(fd[1]);
	}
	::close(fd[0]);
	fd[0] = -1;
	fd[1] = -1;

	if (unlink_on_close) {
		::unlink(path.utf8().ptr());
	}
	unlink_on_close = false;
}

bool FileAccessUnixPipe::is_open() const {
	return (fd[0] >= 0 || fd[1] >= 0);
}

String FileAccessUnixPipe::get_path() const {
	return path_src;
}

String FileAccessUnixPipe::get_path_absolute() const {
	return path_src;
}

uint64_t FileAccessUnixPipe::get_length() const {
	ERR_FAIL_COND_V_MSG(fd[0] < 0, 0, "Pipe must be opened before use.");

	int buf_rem = 0;
	ERR_FAIL_COND_V(ioctl(fd[0], FIONREAD, &buf_rem) != 0, 0);
	return buf_rem;
}

uint64_t FileAccessUnixPipe::get_buffer(uint8_t *p_dst, uint64_t p_length) const {
	ERR_FAIL_COND_V_MSG(fd[0] < 0, -1, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_dst && p_length > 0, -1);

	ssize_t read = ::read(fd[0], p_dst, p_length);
	if (read == -1) {
		last_error = ERR_FILE_CANT_READ;
		read = 0;
	} else if (read != (ssize_t)p_length) {
		last_error = ERR_FILE_CANT_READ;
	} else {
		last_error = OK;
	}
	return read;
}

Error FileAccessUnixPipe::get_error() const {
	return last_error;
}

bool FileAccessUnixPipe::store_buffer(const uint8_t *p_src, uint64_t p_length) {
	ERR_FAIL_COND_V_MSG(fd[1] < 0, false, "Pipe must be opened before use.");
	ERR_FAIL_COND_V(!p_src && p_length > 0, false);

	sighandler_t sig_pipe = signal(SIGPIPE, SIG_IGN);
	ssize_t ret = ::write(fd[1], p_src, p_length);
	signal(SIGPIPE, sig_pipe);

	if (ret != (ssize_t)p_length) {
		last_error = ERR_FILE_CANT_WRITE;
		return false;
	} else {
		last_error = OK;
		return true;
	}
}

void FileAccessUnixPipe::close() {
	_close();
}

FileAccessUnixPipe::~FileAccessUnixPipe() {
	_close();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/ip_unix.cpp
// ============================================================================

#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)


#include <netdb.h>

#ifdef ANDROID_ENABLED
// We could drop this file once we up our API level to 24,
// where the NDK's ifaddrs.h supports to needed getifaddrs.
#include <thirdparty/misc/ifaddrs-android.h>
#else
#ifdef __FreeBSD__
#include <sys/types.h>
#endif
#include <ifaddrs.h>
#endif

#include <arpa/inet.h>
#include <sys/socket.h>

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#include <net/if.h> // Order is important on OpenBSD, leave as last.

static IPAddress _unix_sockaddr2ip(struct sockaddr *p_addr) {
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

void IPUnix::_resolve_hostname(
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
		IPAddress ip = _unix_sockaddr2ip(next->ai_addr);
		if (ip.is_valid() && !r_addresses.find(ip)) {
			r_addresses.push_back(ip);
		}
		next = next->ai_next;
	} while (next);

	freeaddrinfo(result);
}

void IPUnix::get_local_interfaces(HashMap<String, IP::Interface_Info>* r_interfaces)
{
	struct ifaddrs* ifAddrStruct = nullptr;
	struct ifaddrs* ifa = nullptr;
	int family;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
		if (!ifa->ifa_addr) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family != AF_INET && family != AF_INET6) {
			continue;
		}

		HashMap<String, IP::Interface_Info>::Iterator E = r_interfaces->find(ifa->ifa_name);
		if (!E) {
			IP::Interface_Info info;
			info.name = ifa->ifa_name;
			info.name_friendly = ifa->ifa_name;
			info.index = String::num_uint64(if_nametoindex(ifa->ifa_name));
			E = r_interfaces->insert(ifa->ifa_name, info);
			ERR_CONTINUE(!E);
		}

		IP::Interface_Info& info = E->value;
		info.ip_addresses.push_front(_unix_sockaddr2ip(ifa->ifa_addr));
	}

	if (ifAddrStruct != nullptr) {
		freeifaddrs(ifAddrStruct);
	}
}

void IPUnix::make_default()
{
	IP::bind<IPUnix>();
}

#endif // UNIX_ENABLED

// ============================================================================
// Merged from drivers/unix/net_socket_unix.cpp
// ============================================================================

// Some proprietary Unix-derived platforms don't expose Unix sockets
// so this allows skipping this file to reimplement this API differently.
#if defined(UNIX_ENABLED) && !defined(UNIX_SOCKET_UNAVAILABLE)

#include "drivers/unix/os_unix.h"

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#ifdef WEB_ENABLED
#include <arpa/inet.h>
#endif

// BSD calls this flag IPV6_JOIN_GROUP
#if !defined(IPV6_ADD_MEMBERSHIP) && defined(IPV6_JOIN_GROUP)
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif
#if !defined(IPV6_DROP_MEMBERSHIP) && defined(IPV6_LEAVE_GROUP)
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif

size_t NetSocketUnix::_set_addr_storage(struct sockaddr_storage *p_addr, const IPAddress &p_ip, uint16_t p_port, IP::Type p_ip_type) {
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

socklen_t NetSocketUnix::_unix_set_sockaddr(struct sockaddr_un *p_addr, const CharString &p_path) {
	memset(p_addr, 0, sizeof(struct sockaddr_un));
	p_addr->sun_family = AF_UNIX;

	// Path must not exceed maximum path length for Unix domain socket
	size_t path_len = p_path.length();
	ERR_FAIL_COND_V(path_len >= sizeof(p_addr->sun_path) - 1, 0);

	// Regular file system socket
	memcpy(p_addr->sun_path, p_path.get_data(), path_len);
	p_addr->sun_path[path_len] = '\0';
	return sizeof(struct sockaddr_un);
}

void NetSocketUnix::_set_ip_port(struct sockaddr_storage *p_addr, IPAddress *r_ip, uint16_t *r_port) {
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

Ref<NetSocket> NetSocketUnix::_create_func() {
	return memnew(NetSocketUnix);
}

void NetSocketUnix::make_default() {
	_create = _create_func;
}

void NetSocketUnix::cleanup() {
}

NetSocketUnix::NetSocketUnix() {
}

NetSocketUnix::~NetSocketUnix() {
	close();
}

// Silence a warning reported in GH-27594.
// EAGAIN and EWOULDBLOCK have the same value on most platforms, but it's not guaranteed.
VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wlogical-op")

NetSocketUnix::NetError NetSocketUnix::_get_socket_error() const {
	if (errno == EISCONN) {
		return ERR_NET_IS_CONNECTED;
	}
	if (errno == EINPROGRESS || errno == EALREADY) {
		return ERR_NET_IN_PROGRESS;
	}
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		return ERR_NET_WOULD_BLOCK;
	}
	if (errno == EADDRINUSE || errno == EINVAL || errno == EADDRNOTAVAIL) {
		return ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE;
	}
	if (errno == EACCES) {
		return ERR_NET_UNAUTHORIZED;
	}
	if (errno == ENOBUFS) {
		return ERR_NET_BUFFER_TOO_SMALL;
	}
	print_verbose("Socket error: " + itos(errno) + ".");
	return ERR_NET_OTHER;
}

VLTR_GCC_WARNING_POP

bool NetSocketUnix::_can_use_ip(const IPAddress &p_ip, const bool p_for_bind) const {
	if (p_for_bind && !(p_ip.is_valid() || p_ip.is_wildcard())) {
		return false;
	} else if (!p_for_bind && !p_ip.is_valid()) {
		return false;
	}
	// Check if socket support this IP type.
	IP::Type type = p_ip.is_ipv4() ? IP::TYPE_IPV4 : IP::TYPE_IPV6;
	return !(_ip_type != IP::TYPE_ANY && !p_ip.is_wildcard() && _ip_type != type);
}

bool NetSocketUnix::_can_use_path(const CharString &p_path) const {
	// Path must not exceed maximum path length for Unix domain socket
	return !p_path.is_empty() && (size_t)p_path.length() < sizeof(((sockaddr_un *)nullptr)->sun_path);
}

_FORCE_INLINE_ Error NetSocketUnix::_change_multicast_group(IPAddress p_ip, String p_if_name, bool p_add) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != Family::INET, ERR_UNAVAILABLE);
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

void NetSocketUnix::_set_socket(int p_sock, IP::Type p_ip_type, bool p_is_stream) {
	_sock = p_sock;
	_ip_type = p_ip_type;
	_is_stream = p_is_stream;
	// Disable descriptor sharing with subprocesses.
	_set_close_exec_enabled(true);
}

void NetSocketUnix::_set_close_exec_enabled(bool p_enabled) {
	// Enable close on exec to avoid sharing with subprocesses. Off by default on Windows.
	int opts = fcntl(_sock, F_GETFD);
	fcntl(_sock, F_SETFD, opts | FD_CLOEXEC);
}

Error NetSocketUnix::_inet_open(Type p_sock_type, IP::Type &r_ip_type) {
	ERR_FAIL_COND_V(r_ip_type > IP::TYPE_ANY || r_ip_type < IP::TYPE_NONE, ERR_INVALID_PARAMETER);

#if defined(__OpenBSD__)
	// OpenBSD does not support dual stacking, fallback to IPv4 only.
	if (r_ip_type == IP::TYPE_ANY) {
		r_ip_type = IP::TYPE_IPV4;
	}
#endif

	int family = r_ip_type == IP::TYPE_IPV4 ? AF_INET : AF_INET6;
	int protocol = p_sock_type == TYPE_TCP ? IPPROTO_TCP : IPPROTO_UDP;
	int type = p_sock_type == TYPE_TCP ? SOCK_STREAM : SOCK_DGRAM;
	_sock = socket(family, type, protocol);

	if (_sock == -1 && r_ip_type == IP::TYPE_ANY) {
		// Careful here, changing the referenced parameter so the caller knows that we are using an IPv4 socket
		// in place of a dual stack one, and further calls to _set_sock_addr will work as expected.
		r_ip_type = IP::TYPE_IPV4;
		family = AF_INET;
		_sock = socket(family, type, protocol);
	}

	ERR_FAIL_COND_V(_sock == -1, FAILED);
	_ip_type = r_ip_type;
	_family = Family::INET;

	if (family == AF_INET6) {
		// Select IPv4 over IPv6 mapping.
		set_ipv6_only_enabled(r_ip_type != IP::TYPE_ANY);
	}

	if (protocol == IPPROTO_UDP) {
		// Make sure to disable broadcasting for UDP sockets.
		// Depending on the OS, this option might or might not be enabled by default. Let's normalize it.
		set_broadcasting_enabled(false);
	}

	_is_stream = p_sock_type == TYPE_TCP;

	// Disable descriptor sharing with subprocesses.
	_set_close_exec_enabled(true);

#if defined(SO_NOSIGPIPE)
	// Disable SIGPIPE (should only be relevant to stream sockets, but seems to affect UDP too on iOS).
	int par = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_NOSIGPIPE, &par, sizeof(int)) != 0) {
		print_verbose("Unable to turn off SIGPIPE on socket.");
	}
#endif
	return OK;
}

Error NetSocketUnix::_unix_open() {
	_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	ERR_FAIL_COND_V(_sock == -1, FAILED);

	_family = Family::UNIX;

	_set_close_exec_enabled(true);

#if defined(SO_NOSIGPIPE)
	// Disable SIGPIPE (should only be relevant to stream sockets, but seems to affect UDP too on iOS).
	int par = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_NOSIGPIPE, &par, sizeof(int)) != 0) {
		print_verbose("Unable to turn off SIGPIPE on socket.");
	}
#endif

	return OK;
}

Error NetSocketUnix::open(NetSocket::Family p_family, NetSocket::Type p_sock_type, IP::Type &r_ip_type) {
	ERR_FAIL_COND_V(is_open(), ERR_ALREADY_IN_USE);

	switch (p_family) {
		case Family::INET:
			return _inet_open(p_sock_type, r_ip_type);
		case Family::UNIX:
			return _unix_open();
		case Family::NONE:
		default:
			return ERR_INVALID_PARAMETER;
	}
}

void NetSocketUnix::close() {
	if (_sock != -1) {
		::close(_sock);

		if (_family == Family::UNIX) {
			if (_unlink_on_close) {
				::unlink(_unix_path.get_data());
				_unlink_on_close = false;
				_unix_path = CharString();
			}
		}
	}

	_sock = -1;
	_family = Family::NONE;
	_ip_type = IP::TYPE_NONE;
	_is_stream = false;
}

Error NetSocketUnix::_inet_bind(IPAddress p_addr, uint16_t p_port) {
	ERR_FAIL_COND_V(!_can_use_ip(p_addr, true), ERR_INVALID_PARAMETER);

	sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_addr, p_port, _ip_type);

	if (::bind(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		print_verbose("Failed to bind socket. Error: " + itos(err) + ".");
		close();
		return ERR_UNAVAILABLE;
	}

	return OK;
}

Error NetSocketUnix::_unix_bind(const CharString &p_path) {
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_INVALID_PARAMETER);

	struct sockaddr_un addr;
	socklen_t addr_size = _unix_set_sockaddr(&addr, p_path);
	ERR_FAIL_COND_V(addr_size == 0, ERR_INVALID_PARAMETER);

	// If the socket file exists, attempt to remove it.
	if (access(p_path.get_data(), F_OK) == 0) {
		// Check if it's a socket
		struct stat st;
		if (stat(p_path.get_data(), &st) == 0) {
			if (S_ISSOCK(st.st_mode)) {
				// It is a socket, try to remove it.
				if (unlink(p_path.get_data()) != 0) {
					// Failed to remove existing socket file.
					return FAILED;
				}
			} else {
				// It's not a socket, don't remove it.
				return ERR_ALREADY_EXISTS;
			}
		}
	}

	_unlink_on_close = true;

	if (::bind(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		print_verbose("Failed to bind socket. Error: " + itos(err) + ".");
		close();
		switch (err) {
			case ERR_NET_UNAUTHORIZED:
				return ERR_UNAUTHORIZED;
			default:
				return ERR_UNAVAILABLE;
		}
	}

	return OK;
}

Error NetSocketUnix::bind(NetSocket::Address p_addr) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != p_addr.get_family(), ERR_INVALID_PARAMETER);
	switch (p_addr.get_family()) {
		case Family::INET: {
			return _inet_bind(p_addr.ip(), p_addr.port());
		}
		case Family::UNIX: {
			_unix_path = p_addr.get_path();
			return _unix_bind(_unix_path);
		}
		case Family::NONE:
		default:
			return ERR_INVALID_PARAMETER;
	}
}

Error NetSocketUnix::listen(int p_max_pending) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	if (::listen(_sock, p_max_pending) != 0) {
		_get_socket_error();
		print_verbose("Failed to listen from socket.");
		close();
		return FAILED;
	}

	return OK;
}

Error NetSocketUnix::_inet_connect_to_host(IPAddress p_host, uint16_t p_port) {
	ERR_FAIL_COND_V(!_can_use_ip(p_host, false), ERR_INVALID_PARAMETER);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_host, p_port, _ip_type);

	if (::connect(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
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

Error NetSocketUnix::_unix_connect_to_host(const CharString &p_path) {
	ERR_FAIL_COND_V(!_can_use_path(p_path), ERR_INVALID_PARAMETER);

	struct sockaddr_un addr;
	socklen_t addr_size = _unix_set_sockaddr(&addr, p_path);
	ERR_FAIL_COND_V(addr_size == 0, ERR_INVALID_PARAMETER);

	if (::connect(_sock, (struct sockaddr *)&addr, addr_size) != 0) {
		NetError err = _get_socket_error();
		switch (err) {
			case ERR_NET_ADDRESS_INVALID_OR_UNAVAILABLE:
				return ERR_INVALID_PARAMETER;
			// Still waiting to connect, try again in a while.
			case ERR_NET_WOULD_BLOCK:
			case ERR_NET_IN_PROGRESS:
				return ERR_BUSY;
			case ERR_NET_UNAUTHORIZED:
				return ERR_UNAUTHORIZED;
			default:
				print_verbose("Connection to host failed.");
				close();
				return FAILED;
		}
	}

	return OK;
}

Error NetSocketUnix::connect_to_host(NetSocket::Address p_addr) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != p_addr.get_family(), ERR_INVALID_PARAMETER);

	switch (p_addr.get_family()) {
		case Family::INET:
			return _inet_connect_to_host(p_addr.ip(), p_addr.port());
		case Family::UNIX:
			return _unix_connect_to_host(p_addr.get_path());
		case Family::NONE:
		default:
			return ERR_INVALID_PARAMETER;
	}
}

Error NetSocketUnix::poll(PollType p_type, int p_timeout) const {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	struct pollfd pfd;
	pfd.fd = _sock;
	pfd.events = POLLIN;
	pfd.revents = 0;

	switch (p_type) {
		case POLL_TYPE_IN:
			pfd.events = POLLIN;
			break;
		case POLL_TYPE_OUT:
			pfd.events = POLLOUT;
			break;
		case POLL_TYPE_IN_OUT:
			pfd.events = POLLOUT | POLLIN;
	}

	int ret = ::poll(&pfd, 1, p_timeout);

	if (ret < 0 || pfd.revents & POLLERR) {
		_get_socket_error();
		print_verbose("Error when polling socket.");
		return FAILED;
	}

	if (ret == 0) {
		return ERR_BUSY;
	}

	return OK;
}

Error NetSocketUnix::recv(uint8_t *p_buffer, int p_len, int &r_read) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	r_read = ::recv(_sock, p_buffer, p_len, 0);

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

Error NetSocketUnix::recvfrom(uint8_t *p_buffer, int p_len, int &r_read, IPAddress &r_ip, uint16_t &r_port, bool p_peek) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != Family::INET, ERR_UNAVAILABLE);

	struct sockaddr_storage from;
	socklen_t len = sizeof(struct sockaddr_storage);
	memset(&from, 0, len);

	r_read = ::recvfrom(_sock, p_buffer, p_len, p_peek ? MSG_PEEK : 0, (struct sockaddr *)&from, &len);

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

Error NetSocketUnix::send(const uint8_t *p_buffer, int p_len, int &r_sent) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);

	int flags = 0;
#ifdef MSG_NOSIGNAL
	if (_is_stream || _family == Family::UNIX) {
		flags = MSG_NOSIGNAL;
	}
#endif
	r_sent = ::send(_sock, p_buffer, p_len, flags);

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

Error NetSocketUnix::sendto(const uint8_t *p_buffer, int p_len, int &r_sent, IPAddress p_ip, uint16_t p_port) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	ERR_FAIL_COND_V(_family != Family::INET, ERR_UNAVAILABLE);

	struct sockaddr_storage addr;
	size_t addr_size = _set_addr_storage(&addr, p_ip, p_port, _ip_type);
	r_sent = ::sendto(_sock, p_buffer, p_len, 0, (struct sockaddr *)&addr, addr_size);

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

Error NetSocketUnix::set_broadcasting_enabled(bool p_enabled) {
	ERR_FAIL_COND_V(!is_open(), ERR_UNCONFIGURED);
	// IPv6 has no broadcast support.
	if (_ip_type == IP::TYPE_IPV6) {
		return ERR_UNAVAILABLE;
	}

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_BROADCAST, &par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change broadcast setting.");
		return FAILED;
	}
	return OK;
}

void NetSocketUnix::set_blocking_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	int ret = 0;
	int opts = fcntl(_sock, F_GETFL);
	if (p_enabled) {
		ret = fcntl(_sock, F_SETFL, opts & ~O_NONBLOCK);
	} else {
		ret = fcntl(_sock, F_SETFL, opts | O_NONBLOCK);
	}

	if (ret != 0) {
		WARN_PRINT("Unable to change non-block mode.");
	}
}

void NetSocketUnix::set_ipv6_only_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	// This option is only available in IPv6 sockets.
	ERR_FAIL_COND(_ip_type == IP::TYPE_IPV4);

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_IPV6, IPV6_V6ONLY, &par, sizeof(int)) != 0) {
		WARN_PRINT("Unable to change IPv4 address mapping over IPv6 option.");
	}
}

void NetSocketUnix::set_tcp_no_delay_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());
	ERR_FAIL_COND(!_is_stream); // Not TCP.

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, &par, sizeof(int)) < 0) {
		WARN_PRINT("Unable to set TCP no delay option.");
	}
}

void NetSocketUnix::set_reuse_address_enabled(bool p_enabled) {
	ERR_FAIL_COND(!is_open());

	int par = p_enabled ? 1 : 0;
	if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &par, sizeof(int)) < 0) {
		WARN_PRINT("Unable to set socket REUSEADDR option.");
	}
}

bool NetSocketUnix::is_open() const {
	return _sock != -1;
}

int NetSocketUnix::get_available_bytes() const {
	ERR_FAIL_COND_V(!is_open(), -1);

	int len;
	int ret = ioctl(_sock, FIONREAD, &len);
	if (ret == -1) {
		_get_socket_error();
		print_verbose("Error when checking available bytes on socket.");
		return -1;
	}
	return len;
}

Error NetSocketUnix::_inet_get_socket_address(IPAddress *r_ip, uint16_t *r_port) const {
	struct sockaddr_storage saddr;
	socklen_t len = sizeof(saddr);
	if (getsockname(_sock, (struct sockaddr *)&saddr, &len) != 0) {
		_get_socket_error();
		print_verbose("Error when reading local socket address.");
		return FAILED;
	}
	_set_ip_port(&saddr, r_ip, r_port);
	return OK;
}

Error NetSocketUnix::get_socket_address(NetSocket::Address *r_addr) const {
	ERR_FAIL_COND_V(!is_open(), FAILED);
	switch (_family) {
		case Family::INET: {
			IPAddress ip;
			uint16_t port = 0;
			Error res = _inet_get_socket_address(&ip, &port);
			ERR_FAIL_COND_V(res != OK, res);
			if (r_addr) {
				Address addr(ip, port);
				*r_addr = addr;
			}
		} break;
		case Family::UNIX: {
			if (r_addr) {
				*r_addr = Address(_unix_path);
			}
		} break;
		case Family::NONE:
		default:
			return FAILED;
	}

	return OK;
}

Ref<NetSocket> NetSocketUnix::_inet_accept(IPAddress &r_ip, uint16_t &r_port) {
	struct sockaddr_storage their_addr;
	socklen_t size = sizeof(their_addr);
	int fd = ::accept(_sock, (struct sockaddr *)&their_addr, &size);
	if (fd == -1) {
		_get_socket_error();
		print_verbose("Error when accepting socket connection.");
		return Ref<NetSocket>();
	}

	_set_ip_port(&their_addr, &r_ip, &r_port);

	Ref<NetSocketUnix> ns;
	ns.instantiate();
	ns->_set_socket(fd, _ip_type, _is_stream);
	ns->set_blocking_enabled(false);
	return Ref<NetSocket>(ns);
}

Ref<NetSocket> NetSocketUnix::_unix_accept() {
	struct sockaddr_un addr;
	socklen_t addr_len = sizeof(addr);

	int fd = ::accept(_sock, (struct sockaddr *)&addr, &addr_len);
	if (fd == -1) {
		_get_socket_error();
		print_verbose("Error when accepting socket connection.");
		return Ref<NetSocket>();
	}

	Ref<NetSocketUnix> ret = memnew(NetSocketUnix);
	ret->_sock = fd;
	ret->_family = _family;
	ret->_unix_path = _unix_path;
	ret->set_blocking_enabled(false);
	return Ref<NetSocket>(ret);
}

Ref<NetSocket> NetSocketUnix::accept(NetSocket::Address &r_addr) {
	Ref<NetSocket> out;
	ERR_FAIL_COND_V(!is_open(), out);

	switch (_family) {
		case Family::INET: {
			IPAddress ip;
			uint16_t port;
			out = _inet_accept(ip, port);
			if (out.is_valid()) {
				r_addr = Address(ip, port);
			}
		} break;
		case Family::UNIX: {
			out = _unix_accept();
		} break;
		case Family::NONE:
		default:
			break;
	}
	return out;
}

Error NetSocketUnix::join_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, true);
}

Error NetSocketUnix::leave_multicast_group(const IPAddress &p_multi_address, const String &p_if_name) {
	return _change_multicast_group(p_multi_address, p_if_name, false);
}

#endif // UNIX_ENABLED && !UNIX_SOCKET_UNAVAILABLE

#ifdef IPV6_ADD_MEMBERSHIP
#undef IPV6_ADD_MEMBERSHIP
#endif
#ifdef IPV6_DROP_MEMBERSHIP
#undef IPV6_DROP_MEMBERSHIP
#endif

// ============================================================================
// Merged from drivers/unix/syslog_logger.cpp
// ============================================================================

#ifdef UNIX_ENABLED
// --- Inlined from drivers/unix/syslog_logger.h ---
#ifdef UNIX_ENABLED

#include "core/os/os.h"

class SyslogLogger : public Logger {
public:
	virtual void logv(const char *p_format, va_list p_list, bool p_err) _PRINTF_FORMAT_ATTRIBUTE_2_0;
	virtual void print_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, ErrorType p_type);

	virtual ~SyslogLogger();
};

#endif // UNIX_ENABLED

// --- End drivers/unix/syslog_logger.h ---

#include <syslog.h>

void SyslogLogger::logv(const char *p_format, va_list p_list, bool p_err) {
	if (!should_log(p_err)) {
		return;
	}

	vsyslog(p_err ? LOG_ERR : LOG_INFO, p_format, p_list);
}

void SyslogLogger::print_error(const char *p_function, const char *p_file, int p_line, const char *p_code, const char *p_rationale, ErrorType p_type) {
	if (!should_log(true)) {
		return;
	}

	const char *err_type = error_type_string(p_type);

	const char *err_details;
	if (p_rationale && *p_rationale) {
		err_details = p_rationale;
	} else {
		err_details = p_code;
	}

	syslog(p_type == ERR_WARNING ? LOG_WARNING : LOG_ERR, "**%s**: %s\n   At: %s:%i:%s() - %s", err_type, err_details, p_file, p_line, p_function, p_code);
}

SyslogLogger::~SyslogLogger() {
}

#endif

// ============================================================================
// Merged from drivers/unix/thread_posix.cpp
// ============================================================================

#if defined(UNIX_ENABLED)


#include "core/os/thread.h"
#include "core/string/ustring.h"

#if defined(PLATFORM_THREAD_OVERRIDE) && defined(__APPLE__)
void init_thread_posix() {
}
#else

#ifdef PTHREAD_BSD_SET_NAME
#include <pthread_np.h>
#endif

static Error _unix_thread_set_name(const String &p_name) {
#ifdef PTHREAD_NO_RENAME
	return ERR_UNAVAILABLE;

#else

#ifdef PTHREAD_RENAME_SELF

	// check if thread is the same as caller
	int err = pthread_setname_np(p_name.utf8().get_data());

#else

	pthread_t running_thread = pthread_self();
#ifdef PTHREAD_BSD_SET_NAME
	pthread_set_name_np(running_thread, p_name.utf8().get_data());
	int err = 0; // Open/FreeBSD ignore errors in this function
#elif defined(PTHREAD_NETBSD_SET_NAME)
	int err = pthread_setname_np(running_thread, "%s", const_cast<char *>(p_name.utf8().get_data()));
#else
	int err = pthread_setname_np(running_thread, p_name.utf8().get_data());
#endif // PTHREAD_BSD_SET_NAME

#endif // PTHREAD_RENAME_SELF

	return err == 0 ? OK : ERR_INVALID_PARAMETER;

#endif // PTHREAD_NO_RENAME
}

void init_thread_posix() {
	Thread::_set_platform_functions({ .set_name = _unix_thread_set_name });
}

#endif // PLATFORM_THREAD_OVERRIDE && __APPLE__

#endif // UNIX_ENABLED
#endif // defined(UNIX_ENABLED)
