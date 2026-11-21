/**************************************************************************/
/*  theme_db.cpp                                                          */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "scene/gui/control.h"
#include "scene/main/node.h"
#include "scene/main/window.h"
#include "scene/resources/font.h"
#include "scene/resources/style_box.h"
#include "scene/resources/texture.h"
#include "scene/theme/default_theme.h"
#include "servers/rendering/renderer.h"
#include "servers/text/text_server.h"
#include "theme_db.h"

void ThemeDB::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void ThemeDB::finalize()
{
	if (!data) {
		return;
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->project_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
	memdelete(data);
	data = nullptr;
}

void ThemeDB::initialize_theme()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::initialize_theme_noproject()
{
	if (!data) {
		return;
	}
	if (Renderer::data) {
		make_default_theme(1.0, Ref<Font>());
	}
	_init_default_theme_context();
}

void ThemeDB::finalize_theme()
{
	if (!data) {
		return;
	}
	if (!Renderer::data) {
		WARN_PRINT("Finalizing theme when there is no Renderer is an error; check the order "
				   "of operations.");
	}
	_finalize_theme_contexts();
	data->default_theme.unref();
	data->fallback_font.unref();
	data->fallback_icon.unref();
	data->fallback_stylebox.unref();
}

// Global Theme resources.

void ThemeDB::set_default_theme(const Ref<Theme>& p_default)
{
	if (data) {
		data->default_theme = p_default;
	}
}

Ref<Theme> ThemeDB::get_default_theme() { return data ? data->default_theme : Ref<Theme>(); }

void ThemeDB::set_project_theme(const Ref<Theme>& p_project_default)
{
	if (data) {
		data->project_theme = p_project_default;
	}
}

Ref<Theme> ThemeDB::get_project_theme() { return data ? data->project_theme : Ref<Theme>(); }

void ThemeDB::set_fallback_base_scale(float p_base_scale)
{
	if (data) {
		data->fallback_base_scale = p_base_scale;
	}
}

float ThemeDB::get_fallback_base_scale() { return data ? data->fallback_base_scale : 1.0f; }

void ThemeDB::set_fallback_font(const Ref<Font>& p_font)
{
	if (data) {
		data->fallback_font = p_font;
	}
}

Ref<Font> ThemeDB::get_fallback_font() { return data ? data->fallback_font : Ref<Font>(); }

void ThemeDB::set_fallback_font_size(int p_font_size)
{
	if (data) {
		data->fallback_font_size = p_font_size;
	}
}

int ThemeDB::get_fallback_font_size() { return data ? data->fallback_font_size : 16; }

void ThemeDB::set_fallback_icon(const Ref<Texture2D>& p_icon)
{
	if (data) {
		data->fallback_icon = p_icon;
	}
}

Ref<Texture2D> ThemeDB::get_fallback_icon()
{
	return data ? data->fallback_icon : Ref<Texture2D>();
}

void ThemeDB::set_fallback_stylebox(const Ref<StyleBox>& p_stylebox)
{
	if (data) {
		data->fallback_stylebox = p_stylebox;
	}
}

Ref<StyleBox> ThemeDB::get_fallback_stylebox()
{
	return data ? data->fallback_stylebox : Ref<StyleBox>();
}

void ThemeDB::get_native_type_dependencies(const String& p_base_type, Vector<String>& r_result)
{
	(void)r_result;
	if (p_base_type == String()) {
		return;
	}
}

void ThemeDB::_init_default_theme_context()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
	}
	data->default_theme_context = memnew(ThemeContext);
	Vector<Ref<Theme>> themes;

#ifdef TOOLS_ENABLED
	if (!Engine::is_editor_hint()) {
		themes.push_back(data->project_theme);
	}
#else
	themes.push_back(data->project_theme);
#endif
	themes.push_back(data->default_theme);
	data->default_theme_context->set_themes(themes);
}

void ThemeDB::_finalize_theme_contexts()
{
	if (!data) {
		return;
	}
	if (data->default_theme_context) {
		memdelete(data->default_theme_context);
		data->default_theme_context = nullptr;
	}
	while (data->theme_contexts.size()) {
		HashMap<Node*, ThemeContext*>::Iterator E = data->theme_contexts.begin();
		memdelete(E->value);
		data->theme_contexts.remove(E);
	}
}

ThemeContext* ThemeDB::create_theme_context(Node* p_node, Vector<Ref<Theme>>& p_themes)
{
	(void)p_node;
	(void)p_themes;
	return nullptr;
}

void ThemeDB::destroy_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return;
	}
	memdelete(data->theme_contexts[p_node]);
	data->theme_contexts.erase(p_node);
}

ThemeContext* ThemeDB::get_theme_context(Node* p_node)
{
	if (!data || !data->theme_contexts.has(p_node)) {
		return nullptr;
	}
	return data->theme_contexts[p_node];
}

ThemeContext* ThemeDB::get_default_theme_context()
{
	return data ? data->default_theme_context : nullptr;
}

ThemeContext* ThemeDB::get_nearest_theme_context(Node* p_for_node)
{
	if (!data || !p_for_node) {
		return nullptr;
	}
	ERR_FAIL_COND_V(!p_for_node->is_inside_tree(), nullptr);
	Node* parent_node = p_for_node->get_parent();
	while (parent_node) {
		if (data->theme_contexts.has(parent_node)) {
			return data->theme_contexts[parent_node];
		}
		parent_node = parent_node->get_parent();
	}
	return nullptr;
}

// Theme item binding.

void ThemeDB::bind_class_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::bind_class_external_item(Theme::DataType p_data_type, const String& p_class_name,
	const String& p_prop_name, const String& p_item_name, const String& p_type_name,
	ThemeItemSetter p_setter)
{
	if (!data) {
		return;
	}
	ERR_FAIL_COND_MSG(data->theme_item_binds[p_class_name].has(p_prop_name),
		vformat("Failed to bind theme item '%s' in class '%s': already bound", p_prop_name,
			p_class_name));
	ThemeItemBind bind;
	bind.data_type = p_data_type;
	bind.class_name = p_class_name;
	bind.item_name = p_item_name;
	bind.type_name = p_type_name;
	bind.external = true;
	bind.setter = p_setter;
	data->theme_item_binds[p_class_name][p_prop_name] = bind;
	data->theme_item_binds_list[p_class_name].push_back(bind);
}

void ThemeDB::update_class_instance_items(Node* p_instance) { (void)p_instance; }

void ThemeDB::get_class_items(const String& p_class_name, List<ThemeItemBind>* r_list,
	bool p_include_inherited, Theme::DataType p_filter_type)
{
	if (!data) {
		return;
	}
	List<String> class_hierarchy;
	String class_name = p_class_name;
	while (class_name != String()) {
		class_hierarchy.push_front(class_name);
	}
	HashSet<String> inherited_props;
	for (const String& theme_type : class_hierarchy) {
		HashMap<String, List<ThemeItemBind>>::Iterator E =
			data->theme_item_binds_list.find(theme_type);
		if (E) {
			for (const ThemeItemBind& F : E->value) {
				if (p_filter_type != Theme::DATA_TYPE_MAX && F.data_type != p_filter_type) {
					continue;
				}
				if (inherited_props.has(F.item_name)) {
					continue;
				}
				if (F.external || F.class_name != p_class_name) {
					inherited_props.insert(F.item_name);
					if (!p_include_inherited) {
						continue;
					}
				}
				r_list->push_back(F);
			}
		}
	}
}

void ThemeDB::_sort_theme_items()
{
	if (!data) {
		return;
	}
	for (KeyValue<String, List<ThemeDB::ThemeItemBind>>& E : data->theme_item_binds_list) {
		E.value.sort_custom<ThemeItemBind::SortByType>();
	}
}

const Vector<Ref<Theme>> ThemeContext::get_themes() const { return themes; }

Ref<Theme> ThemeContext::get_fallback_theme() const
{
	if (themes.is_empty()) {
		return ThemeDB::get_default_theme();
	}
	return themes[themes.size() - 1];
}

// --- theme_owner.cpp ---

#include "scene/gui/control.h"
#include "scene/main/window.h"
#include "scene/theme/theme_db.h"
#include "theme_owner.h"

Node* ThemeOwner::_get_next_owner_node(Node* p_from_node) const
{
	Node n = Node();
	return &n;
}

Ref<Theme> ThemeOwner::_get_owner_node_theme(Node* p_owner_node) const
{
	Ref<Theme> t = memnew(Theme);
	return t;
}

ThemeContext* ThemeOwner::_get_active_owner_context() const
{
	if (owner_context) {
		return owner_context;
	}

	return ThemeDB::get_default_theme_context();
}

bool ThemeOwner::has_theme_item_in_types(
	Theme::DataType p_data_type, const String& p_name, const Vector<String>& p_theme_types)
{
	ERR_FAIL_COND_V_MSG(
		p_theme_types.is_empty(), false, "At least one theme type must be specified.");

	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered.
	Node* current_owner = owner_node;

	while (current_owner) {
		// For each theme resource check the theme types provided and see if p_name exists with any
		// of them.
		for (const String& E : p_theme_types) {
			Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

			if (owner_theme.is_valid() && owner_theme->has_theme_item(p_data_type, p_name, E)) {
				return true;
			}
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			for (const String& E : p_theme_types) {
				if (theme->has_theme_item(p_data_type, p_name, E)) {
					return true;
				}
			}
		}
	}

	// Finally, if no match exists, return false.
	return false;
}

float ThemeOwner::get_theme_default_base_scale()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_base_scale()) {
			return owner_theme->get_default_base_scale();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_base_scale()) {
				return theme->get_default_base_scale();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_base_scale();
}

Ref<Font> ThemeOwner::get_theme_default_font()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font()) {
			return owner_theme->get_default_font();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font()) {
				return theme->get_default_font();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_font();
}

int ThemeOwner::get_theme_default_font_size()
{
	// First, look through each control or window node in the branch, until no valid parent can be
	// found. Only nodes with a theme resource attached are considered. For each theme resource see
	// if their assigned theme has the default value defined and valid.
	Node* current_owner = owner_node;

	while (current_owner) {
		Ref<Theme> owner_theme = _get_owner_node_theme(current_owner);

		if (owner_theme.is_valid() && owner_theme->has_default_font_size()) {
			return owner_theme->get_default_font_size();
		}

		current_owner = _get_next_owner_node(current_owner);
	}

	// Second, check global themes from the appropriate context.
	ThemeContext* global_context = _get_active_owner_context();
	for (const Ref<Theme>& theme : global_context->get_themes()) {
		if (theme.is_valid()) {
			if (theme->has_default_font_size()) {
				return theme->get_default_font_size();
			}
		}
	}

	// Finally, if no match exists, return the universal default.
	return ThemeDB::get_fallback_font_size();
}



void ThemeOwner::set_owner_node(Node*) {}

void ThemeOwner::set_owner_context(ThemeContext*, bool) {}

void ThemeOwner::propagate_theme_changed(Node*, Node*, bool, bool) {}

void ThemeContext::set_themes(Vector<Ref<Theme>>&) {}

// --- default_theme.cpp ---

#include "core/io/image.h"
#include "default_theme.h"
#include "scene/resources/font.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box.h"
#include "scene/resources/theme.h"
#include "scene/scene_string_names.h"
#include "scene/theme/default_theme_icons.gen.h"
#include "scene/theme/theme_db.h"
#include "servers/text/text_server.h"

#ifdef BROTLI_ENABLED
#include "scene/theme/default_font.gen.h"
#endif

static const int default_font_size = 16;

static float scale = 1.0;

static const int default_margin = 4;
static const int default_corner_radius = 3;

static Ref<StyleBox> make_flat_stylebox(Color p_color, float p_margin_left = default_margin,
	float p_margin_top = default_margin, float p_margin_right = default_margin,
	float p_margin_bottom = default_margin, int p_corner_radius = default_corner_radius,
	bool p_draw_center = true, int p_border_width = 0)
{
	Ref<StyleBox> style(memnew(StyleBox(StyleBox::STYLE_FLAT)));
	style->set_bg_color(p_color);
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));

	style->set_corner_radius_all(Math::round(p_corner_radius * scale));
	style->set_anti_aliased(true);
	// Adjust level of detail based on the corners' effective sizes.
	style->set_corner_detail(MIN(Math::ceil(1.5 * p_corner_radius), 6) * scale);

	style->set_draw_center(p_draw_center);
	style->set_border_width_all(Math::round(p_border_width * scale));

	return style;
}

static Ref<StyleBox> sb_expand(
	Ref<StyleBox> p_sbox, float p_left, float p_top, float p_right, float p_bottom)
{
	p_sbox->set_expand_margin(SIDE_LEFT, Math::round(p_left * scale));
	p_sbox->set_expand_margin(SIDE_TOP, Math::round(p_top * scale));
	p_sbox->set_expand_margin(SIDE_RIGHT, Math::round(p_right * scale));
	p_sbox->set_expand_margin(SIDE_BOTTOM, Math::round(p_bottom * scale));
	return p_sbox;
}

static Ref<StyleBox> make_empty_stylebox(float p_margin_left = -1, float p_margin_top = -1,
	float p_margin_right = -1, float p_margin_bottom = -1)
{
	Ref<StyleBox> style(memnew(StyleBox(StyleBox::STYLE_EMPTY)));
	style->set_content_margin_individual(Math::round(p_margin_left * scale),
		Math::round(p_margin_top * scale), Math::round(p_margin_right * scale),
		Math::round(p_margin_bottom * scale));
	return style;
}

void make_default_theme(float p_scale, Ref<Font> p_font,
	TextServer::SubpixelPositioning p_font_subpixel, TextServer::Hinting p_font_hinting,
	TextServer::FontAntialiasing p_font_antialiasing, bool p_font_msdf,
	bool p_font_generate_mipmaps)
{
	Ref<Theme> t;
	t.instantiate();
	Ref<StyleBox> default_style;
	Ref<Texture2D> default_icon;
	Ref<Font> default_font;
	Ref<FontVariation> bold_font;
	Ref<FontVariation> bold_italics_font;
	Ref<FontVariation> italics_font;
	float default_scale = CLAMP(p_scale, 0.5, 8.0);
	if (p_font.is_valid()) {
		// Use the custom font defined in the Project Settings.
		default_font = p_font;
	}
	else {
		// Use the default DynamicFont (separate from the editor font).
		// The default DynamicFont is chosen to have a small file size since it's
		// embedded in both editor and export template binaries.
		Ref<FontFile> dynamic_font;
		dynamic_font.instantiate();
#ifdef BROTLI_ENABLED
		dynamic_font->set_data_ptr(_font_OpenSans_SemiBold, _font_OpenSans_SemiBold_size);
		dynamic_font->set_subpixel_positioning(p_font_subpixel);
		dynamic_font->set_hinting(p_font_hinting);
		dynamic_font->set_antialiasing(p_font_antialiasing);
		dynamic_font->set_multichannel_signed_distance_field(p_font_msdf);
		dynamic_font->set_generate_mipmaps(p_font_generate_mipmaps);
#endif
		default_font = dynamic_font;
	}
	if (default_font.is_valid()) {
		bold_font.instantiate();
		bold_font->set_base_font(default_font);
		bold_font->set_variation_embolden(1.2);
		bold_italics_font.instantiate();
		bold_italics_font->set_base_font(default_font);
		bold_italics_font->set_variation_embolden(1.2);
		bold_italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
		italics_font.instantiate();
		italics_font->set_base_font(default_font);
		italics_font->set_variation_transform(Transform2D(1.0, 0.2, 0.0, 1.0, 0.0, 0.0));
	}
	fill_default_theme(t, default_font, bold_font, bold_italics_font, italics_font, default_icon,
		default_style, default_scale);
	ThemeDB::set_default_theme(t);
	ThemeDB::set_fallback_base_scale(default_scale);
	ThemeDB::set_fallback_icon(default_icon);
	ThemeDB::set_fallback_stylebox(default_style);
	ThemeDB::set_fallback_font(default_font);
	ThemeDB::set_fallback_font_size(default_font_size * default_scale);
}

void fill_default_theme(Ref<Theme>& r_theme, const Ref<Font>& p_font, const Ref<Font>& p_bold_font,
	const Ref<Font>& p_bold_italics_font, const Ref<Font>& p_italics_font, Ref<Texture2D>& r_icons,
	Ref<StyleBox>& r_style, float p_scale)
{
}
