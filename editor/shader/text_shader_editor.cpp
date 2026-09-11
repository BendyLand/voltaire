/**************************************************************************/
/*  text_shader_editor.cpp                                                */
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
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/os.h"
#include "core/version_generated.gen.h"
#include "editor/docks/inspector_dock.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/scene/material_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "modules/regex/regex.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/split_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/sky.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_server.h"
#include "servers/rendering/shader_preprocessor.h"
#include "servers/rendering/shader_types.h"
#include "text_shader_editor.h"

/*** SHADER SYNTAX HIGHLIGHTER ****/

void GDShaderSyntaxHighlighter::add_disabled_branch_region(const Point2i& p_region)
{
	ERR_FAIL_COND(p_region.x < 0);
	ERR_FAIL_COND(p_region.y < 0);

	for (int i = 0; i < disabled_branch_regions.size(); i++) {
		ERR_FAIL_COND_MSG(disabled_branch_regions[i].x == p_region.x,
			"Branch region with a start line '" + itos(p_region.x) + "' already exists.");
	}

	Point2i disabled_branch_region;
	disabled_branch_region.x = p_region.x;
	disabled_branch_region.y = p_region.y;
	disabled_branch_regions.push_back(disabled_branch_region);

	clear_highlighting_cache();
}

void GDShaderSyntaxHighlighter::clear_disabled_branch_regions()
{
	disabled_branch_regions.clear();
	clear_highlighting_cache();
}

void GDShaderSyntaxHighlighter::set_disabled_branch_color(const Color& p_color)
{
	disabled_branch_color = p_color;
	clear_highlighting_cache();
}

/*** SHADER PREVIEW LINE LAYER ****/

void TextShaderPreviewLineLayer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		line_color = Color(EditorNode::get_singleton()->get_editor_theme()->get_color(
							   SceneStringName(font_color), EditorStringName(Editor)),
			0.7);
	} break;
	case NOTIFICATION_DRAW: {
		int total_gutter_width = code_editor->get_line_start_margin();
		for (int i = 0; i < code_editor->get_gutter_count() - 1; i++) {
			if (code_editor->is_gutter_drawn(i)) {
				total_gutter_width += code_editor->get_gutter_width(i);
			}
		}

		const Rect2i visible_rect = scroll_container->get_global_rect();
		RenderingServer::get_singleton()->canvas_item_set_custom_rect(get_canvas_item(), true,
			Rect2(get_global_transform().affine_inverse().xform(Vector2(visible_rect.position)),
				visible_rect.size + Vector2(code_editor->get_total_gutter_width(), 0)));
		RenderingServer::get_singleton()->canvas_item_set_clip(get_canvas_item(), true);

		int current_caret_line = code_editor->get_caret_line();
		int idx = 0;
		for (const KeyValue<int, TextShaderPreview*>& E : *previews) {
			idx++;

			const Control* panel_container = E.value->get_panel_container();
			float alpha = (E.key == current_caret_line)
							  ? 0.2
							  : (E.value->is_hovered() ? 0.15 : (idx % 2 == 0 ? 0.1 : 0.05));
			const Color polygon_color = Color(line_color, alpha);
			E.value->update_panel_color(polygon_color);

			Point2i end_pos = code_editor->get_pos_at_line_column(E.key, 0);
			if (end_pos.x == -1) {
				continue;
			}

			Point2 preview_size = panel_container->get_size();
			const Point2 preview_bottom = panel_container->get_global_position() + preview_size;
			const Point2 preview_top = preview_bottom - Point2(0, preview_size.y);
			const Point2 line_bottom = code_editor->get_global_position() + Point2(0, end_pos.y);
			const Point2 line_top = line_bottom - Point2(0, code_editor->get_line_height());
			const Point2 line_bottom_gutter = line_bottom + Point2(total_gutter_width, 0);
			const Point2 line_top_gutter = line_top + Point2(total_gutter_width, 0);

			draw_polygon({preview_top, line_top, line_top_gutter, line_bottom_gutter, line_bottom,
							 preview_bottom},
				{polygon_color, polygon_color, polygon_color, polygon_color, polygon_color,
					polygon_color});
		}
	} break;
	}
}

void TextShaderPreviewLineLayer::set_previews(HashMap<int, TextShaderPreview*>& p_previews)
{
	previews = &p_previews;
}

void TextShaderPreviewLineLayer::set_scroll_container(ScrollContainer* p_scroll_container)
{
	scroll_container = p_scroll_container;
}

TextShaderPreviewLineLayer::TextShaderPreviewLineLayer() { set_as_top_level(true); }

/***  SHADER PREVIEW ****/

class SquareMarginContainer : public MarginContainer
{
public:
	Size2 get_minimum_size() const override
	{
		Size2 ms = MarginContainer::get_minimum_size();
		float side = MAX(get_size().x, ms.y);
		return Size2(ms.x, side);
	}
};

HashMap<String, String> TextShaderPreview::spatial_assignments = {
	{"bool", "ALBEDO = vec3(float(%s)); ALPHA = 1.0;"},
	{"int", "ALBEDO = vec3(float(%s)); ALPHA = 1.0;"},
	{"float", "ALBEDO = vec3(%s); ALPHA = 1.0;"},
	{"vec2", "ALBEDO = vec3(%s.rg, 0.0); ALPHA = 1.0;"},
	{"vec3", "ALBEDO = %s; ALPHA = 1.0;"},
	{"vec4", "vec4 __sp_v4 = %s; ALBEDO = __sp_v4.rgb; ALPHA = __sp_v4.a;"},
};

HashMap<String, String> TextShaderPreview::canvas_assignments = {
	{"bool", "COLOR = vec4(vec3(float(%s)), 1.0);"},
	{"int", "COLOR = vec4(vec3(float(%s)), 1.0);"},
	{"float", "COLOR = vec4(vec3(%s), 1.0);"},
	{"vec2", "COLOR = vec4(%s, 0.0, 1.0);"},
	{"vec3", "COLOR = vec4(%s, 1.0);"},
	{"vec4", "COLOR = %s;"},
};

HashMap<String, String> TextShaderPreview::builtin_spatial_types = {
	{"NORMAL_MAP_DEPTH", "float"},
	{"DEPTH", "float"},
	{"ALPHA", "float"},
	{"ALPHA_SCISSOR_THRESHOLD", "float"},
	{"ALPHA_HASH_SCALE", "float"},
	{"ALPHA_ANTIALIASING_EDGE", "float"},
	{"PREMUL_ALPHA_FACTOR", "float"},
	{"METALLIC", "float"},
	{"SPECULAR", "float"},
	{"ROUGHNESS", "float"},
	{"RIM", "float"},
	{"RIM_TINT", "float"},
	{"CLEARCOAT", "float"},
	{"CLEARCOAT_ROUGHNESS", "float"},
	{"ANISOTROPY", "float"},
	{"SSS_STRENGTH", "float"},
	{"SSS_TRANSMITTANCE_DEPTH", "float"},
	{"SSS_TRANSMITTANCE_BOOST", "float"},
	{"AO", "float"},
	{"AO_LIGHT_AFFECT", "float"},

	{"ALPHA_TEXTURE_COORDINATE", "vec2"},
	{"ANISOTROPY_FLOW", "vec2"},

	{"NORMAL", "vec3"},
	{"NORMAL_MAP", "vec3"},
	{"LIGHT_VERTEX", "vec3"},
	{"TANGENT", "vec3"},
	{"BINORMAL", "vec3"},
	{"ALBEDO", "vec3"},
	{"BACKLIGHT", "vec3"},
	{"EMISSION", "vec3"},
	{"BENT_NORMAL_MAP", "vec3"},

	{"FOG", "vec4"},
	{"RADIANCE", "vec4"},
	{"IRRADIANCE", "vec4"},
	{"SSS_TRANSMITTANCE_COLOR", "vec4"},
};

HashMap<String, String> TextShaderPreview::builtin_canvas_types = {
	{"NORMAL_MAP_DEPTH", "float"},

	{"SHADOW_VERTEX", "vec2"},
	{"VERTEX", "vec2"},

	{"NORMAL", "vec3"},
	{"NORMAL_MAP", "vec3"},
	{"LIGHT_VERTEX", "vec3"},

	{"COLOR", "vec4"},
};

void TextShaderPreview::_on_hover_enter()
{
	hovered = true;
	delete_button->show();
	goto_button->show();

	DisplayServer::get_singleton()->cursor_set_shape(
		DisplayServerEnums::CURSOR_ARROW); // Since MaterialEditor doesn't set cursor.
}

void TextShaderPreview::_on_hover_exit()
{
	hovered = false;
	delete_button->hide();
	goto_button->hide();
}

String TextShaderPreview::_get_enclosing_function(
	const PackedStringArray& p_lines, int p_line) const
{
	int brace_stack = 0;

	Ref<RegEx> regex;
	regex.instantiate();
	regex->compile(R"(void\s+(\w+)\s*\()");

	for (int i = p_line; i >= 0; i--) {
		// Strip comments and trailing whitespace.
		String clean_line = p_lines[i].split("//")[0].strip_edges();
		if (clean_line.is_empty()) {
			continue;
		}

		brace_stack += clean_line.count("}");
		brace_stack -= clean_line.count("{");
	}

	return String(); // Global scope.
}

bool TextShaderPreview::_is_inside_loop(const PackedStringArray& p_lines, int p_line) const
{
	int brace_stack = 0;

	Ref<RegEx> loop_regex;
	loop_regex.instantiate();
	loop_regex->compile(R"(\b(for|while|do)\b)");

	Ref<RegEx> func_regex;
	func_regex.instantiate();
	func_regex->compile(R"(\b(?!for\b|while\b|do\b|if\b|else\b|return\b|switch\b)\w+\s+\w+\s*\()");

	for (int i = p_line; i >= 0; i--) {
		String clean_line = p_lines[i].split("//")[0].strip_edges();
		if (clean_line.is_empty()) {
			continue;
		}

		brace_stack += clean_line.count("}");
		brace_stack -= clean_line.count("{");

		if (brace_stack < 0) {
			if (loop_regex->search(clean_line).is_valid()) {
				return true;
			}
			if (func_regex->search(clean_line).is_valid()) {
				return false;
			}

			brace_stack = 0;
		}
	}

	return false;
}

void TextShaderPreview::_show_error(const String& p_error)
{
	surface->edit(Ref<Material>(), env);
	error_label->set_text(p_error);
	surface_container->hide();
	error_container->show();
}

void TextShaderPreview::show_shader_compile_error()
{
	_show_error(TTRC("Shader must be compiled correctly."));
}

void TextShaderPreview::recompile(const String& p_code)
{
	set_shader_code(p_code, line, in_comment);
}

void TextShaderPreview::sync_shader_parameters()
{
	if (shader_material->get_shader().is_null()) {
		return;
	}
	const Ref<ShaderMaterial> src_mat = _get_source_material();
	if (src_mat.is_valid()) {
		_sync_shader_parameters(src_mat, shader_material);
	}
	else {
		_reset_shader_parameters(shader_material);
	}
}

void TextShaderPreview::update_panel_color(const Color& p_color)
{
	panel_style->set_bg_color(p_color);
}

PanelContainer* TextShaderPreview::get_panel_container() const { return panel; }

Control* TextShaderPreview::get_hover_control() const { return surface_hover; }

void TextShaderPreview::set_shader_code(const String& p_code, int p_line, bool p_in_comment)
{
	line = p_line;
	in_comment = p_in_comment;
	goto_button->set_text(itos(line + 1));

	String shader_type = ShaderLanguage::get_shader_type(p_code);
	bool mode_3d = shader_type == "spatial";

	if (shader_type != "canvas_item" && !mode_3d) {
		_show_error(TTRC("For previews, shader type must be CanvasItem or Spatial."));
		return;
	}

	const PackedStringArray lines = p_code.split("\n");
	String enclosing_function = _get_enclosing_function(lines, p_line);

	if (enclosing_function != "fragment") {
		_show_error(TTRC("Previewed line must be inside fragment() function."));
		return;
	}

	if (_is_inside_loop(lines, p_line)) {
		_show_error(TTRC("Preview not supported inside loops."));
		return;
	}

	String var_name;
	int start;
	int end;

	if (in_comment || !_find_statement(lines, p_line, var_name, start, end)) {
		_show_error(TTRC("Previewed line must contain an assignment."));
		return;
	}

	String type = _find_var_type(lines, var_name, end, mode_3d);

	// All code before assignment stays as it was.
	PackedStringArray truncated_lines = lines.slice(0, end + 1);

	String injection;
	HashMap<String, String>& assignments = mode_3d ? spatial_assignments : canvas_assignments;
	if (!assignments.has(type)) {
		_show_error(TTRC("Preview only available for bool, int, float, vec2, vec3, vec4."));
		return;
	}
	injection = assignments[type].replace("%s", var_name);
	truncated_lines.append(injection);

	String full_truncated_text = "\n";
	full_truncated_text = full_truncated_text.join(truncated_lines);

	int open_braces = full_truncated_text.count("{");
	int closed_braces = full_truncated_text.count("}");
	int needed_closures = open_braces - closed_braces;

	for (int i = 0; i < needed_closures; i++) {
		full_truncated_text += "\n}";
	}

	Ref<Shader> shader;
	shader.instantiate();
	shader->set_code(full_truncated_text);
	shader_material->set_shader(shader);

	const Ref<ShaderMaterial> src_mat = _get_source_material();
	if (src_mat.is_valid()) {
		_sync_shader_parameters(src_mat, shader_material);
	}
	else {
		_reset_shader_parameters(shader_material);
	}

	error_container->hide();
	surface_container->show();
	surface->edit(shader_material.ptr(), env);
	surface->show(); // Edit may have called hide() earlier on failed compilation.
}

/*** SHADER SCRIPT EDITOR ****/

static bool saved_warnings_enabled = false;
static bool saved_treat_warning_as_errors = false;
static HashMap<ShaderWarning::Code, bool> saved_warnings;
static uint32_t saved_warning_flags = 0U;

void ShaderTextEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		get_text_editor()->add_theme_color_override(
			"breakpoint_color", EditorNode::get_singleton()->get_editor_theme()->get_color(
									SceneStringName(font_color), EditorStringName(Editor)));
		get_text_editor()->add_theme_icon_override(
			"breakpoint", get_editor_theme_icon(SNAME("GuiVisibilityVisible")).ptr());

		if (is_visible_in_tree()) {
			_load_theme_settings();
			if (warnings.size() > 0 && last_compile_result == OK) {
				warnings_panel->clear();
				_update_warning_panel();
			}
		}
	} break;
	}
}

Ref<Shader> ShaderTextEditor::get_edited_shader() const { return shader; }

Ref<ShaderInclude> ShaderTextEditor::get_edited_shader_include() const { return shader_inc; }

void ShaderTextEditor::set_edited_shader(const Ref<Shader>& p_shader)
{
	set_edited_shader(p_shader, p_shader->get_code());
}

void ShaderTextEditor::set_edited_shader_include(const Ref<ShaderInclude>& p_shader_inc)
{
	set_edited_shader_include(p_shader_inc, p_shader_inc->get_code());
}

void ShaderTextEditor::_shader_changed()
{
	// This function is used for dependencies (include changing changes main shader and forces it to
	// revalidate)
	if (block_shader_changed) {
		return;
	}
	dependencies_version++;
	_validate_script();
}

void ShaderTextEditor::goto_shader_preview(int p_line) { goto_line_centered(p_line); }

void ShaderTextEditor::clear_previews()
{
	for (KeyValue<int, TextShaderPreview*> pair : previews) {
		pair.value->queue_free();
	}
	previews.clear();
}

void ShaderTextEditor::redraw_preview_lines() { preview_line_layer->queue_redraw(); }

void ShaderTextEditor::recompile_previews()
{
	for (KeyValue<int, TextShaderPreview*>& E : previews) {
		E.value->recompile(get_text_editor()->get_text());
	}
}

void ShaderTextEditor::update_parameters()
{
	for (KeyValue<int, TextShaderPreview*>& E : previews) {
		E.value->sync_shader_parameters();
	}
}

TextShaderPreviewLineLayer* ShaderTextEditor::get_preview_line_layer() const
{
	return preview_line_layer;
}

TextShaderPreview* ShaderTextEditor::get_preview(int p_line) const
{
	if (previews.has(p_line)) {
		return previews[p_line];
	}
	return nullptr;
}

void ShaderTextEditor::remove_shader_preview(int p_line)
{
	get_text_editor()->set_line_as_breakpoint(p_line, false);
}

void ShaderTextEditor::set_preview_box(Control* p_box) { preview_box = p_box; }

void ShaderTextEditor::reload_text()
{
	ERR_FAIL_COND(shader.is_null() && shader_inc.is_null());

	String code;
	if (shader.is_valid()) {
		code = shader->get_code();
	}
	else {
		code = shader_inc->get_code();
	}

	CodeEdit* te = get_text_editor();
	int column = te->get_caret_column();
	int row = te->get_caret_line();
	int h = te->get_h_scroll();
	int v = te->get_v_scroll();

	te->set_text(code);
	te->set_caret_line(row);
	te->set_caret_column(column);
	te->set_h_scroll(h);
	te->set_v_scroll(v);

	te->tag_saved_version();

	update_line_and_column();
}

void ShaderTextEditor::set_warnings_panel(RichTextLabel* p_warnings_panel)
{
	warnings_panel = p_warnings_panel;
}

void ShaderTextEditor::_check_shader_mode()
{
	String type = ShaderLanguage::get_shader_type(get_text_editor()->get_text());

	Shader::Mode mode;

	if (type == "canvas_item") {
		mode = Shader::MODE_CANVAS_ITEM;
	}
	else if (type == "particles") {
		mode = Shader::MODE_PARTICLES;
	}
	else if (type == "sky") {
		mode = Shader::MODE_SKY;
	}
	else if (type == "fog") {
		mode = Shader::MODE_FOG;
	}
	else if (type == "texture_blit") {
		mode = Shader::MODE_TEXTURE_BLIT;
	}
	else {
		mode = Shader::MODE_SPATIAL;
	}

	if (shader->get_mode() != mode) {
		set_block_shader_changed(true);
		shader->set_code(get_text_editor()->get_text());
		set_block_shader_changed(false);
		_load_theme_settings();
	}
}

static ShaderLanguage::DataType _get_global_shader_uniform_type(const StringName& p_variable)
{
	RSE::GlobalShaderParameterType gvt =
		RS::get_singleton()->global_shader_parameter_get_type(p_variable);
	return (ShaderLanguage::DataType)RS::global_shader_uniform_type_get_shader_datatype(gvt);
}

static String complete_from_path;

/*** SCRIPT EDITOR ******/

void TextShaderEditor::_prepare_edit_menu()
{
	const CodeEdit* tx = code_editor->get_text_editor();
	PopupMenu* popup = edit_menu->get_popup();
	popup->set_item_disabled(popup->get_item_index(EDIT_UNDO), !tx->has_undo());
	popup->set_item_disabled(popup->get_item_index(EDIT_REDO), !tx->has_redo());
}

void TextShaderEditor::_notification(int p_what)
{
	switch (p_what) {
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		if (EditorThemeManager::is_generated_theme_outdated() ||
			EditorSettings::get_singleton()->check_changed_settings_in_group(
				"interface/editor/fonts") ||
			EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor")) {
			_apply_editor_settings();
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (is_visible_in_tree() && preview_timer->is_inside_tree()) {
			preview_timer->start();
		}
	} break;

	case NOTIFICATION_RESIZED: {
		preview_timer->start();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		site_search->set_button_icon(get_editor_theme_icon(SNAME("ExternalLink")));

		Ref<StyleBoxFlat> tab_style = get_theme_stylebox(SNAME("tab_selected"), "TabBar");
		Ref<StyleBoxFlat> preview_style = memnew(StyleBoxFlat);
		preview_style->set_bg_color(
			get_theme_color(SNAME("dark_color_1"), EditorStringName(Editor)));
		preview_style->set_corner_radius_all(tab_style->get_corner_radius(CORNER_TOP_LEFT));
		preview_panel->add_theme_style_override(SceneStringName(panel), preview_style.ptr());

		update_params_btn->set_button_icon(get_editor_theme_icon(SNAME("Reload")));
		remove_all_btn->set_button_icon(get_editor_theme_icon(SNAME("Remove")));
	} break;

	case NOTIFICATION_APPLICATION_FOCUS_IN: {
		_check_for_external_edit();
	} break;
	}
}

void TextShaderEditor::_show_warnings_panel(bool p_show) { warnings_panel->set_visible(p_show); }

void TextShaderEditor::goto_line_selection(int p_line, int p_begin, int p_end)
{
	code_editor->goto_line_selection(p_line, p_begin, p_end);
}

void TextShaderEditor::_project_settings_changed() { _update_warnings(true); }

void TextShaderEditor::_focus_preview_line(int p_line)
{
	code_editor->goto_line_centered(p_line);

	TextShaderPreview* preview = code_editor->get_preview(p_line);
	if (preview) {
		preview_sbox->ensure_control_visible(preview);
	}
	preview_timer->start();
}

void TextShaderEditor::_reload()
{
	if (shader.is_valid()) {
		_reload_shader_from_disk();
	}
	else if (shader_inc.is_valid()) {
		_reload_shader_include_from_disk();
	}
}

void TextShaderEditor::edit_shader(const Ref<Shader>& p_shader)
{
	if (p_shader.is_null() || !p_shader->is_text_shader()) {
		return;
	}

	if (shader == p_shader) {
		return;
	}

	shader = p_shader;
	shader_inc = Ref<ShaderInclude>();

	code_editor->set_edited_shader(shader);
}

void TextShaderEditor::edit_shader_include(const Ref<ShaderInclude>& p_shader_inc)
{
	if (p_shader_inc.is_null()) {
		return;
	}

	if (shader_inc == p_shader_inc) {
		return;
	}

	shader_inc = p_shader_inc;
	shader = Ref<Shader>();

	code_editor->set_edited_shader_include(p_shader_inc);
}

void TextShaderEditor::use_menu_bar(MenuButton* p_file_menu)
{
	p_file_menu->set_switch_on_hover(true);
	menu_bar_hbox->add_child(p_file_menu);
	menu_bar_hbox->move_child(p_file_menu, 0);
}

void TextShaderEditor::save_external_data(const String& p_str)
{
	if (shader.is_null() && shader_inc.is_null()) {
		disk_changed->hide();
		return;
	}

	if (trim_trailing_whitespace_on_save) {
		trim_trailing_whitespace();
	}

	if (trim_final_newlines_on_save) {
		trim_final_newlines();
	}

	apply_shaders();

	Ref<Shader> edited_shader = code_editor->get_edited_shader();
	if (edited_shader.is_valid()) {
		ResourceSaver::save(edited_shader.ptr());
	}
	if (shader.is_valid() && shader != edited_shader) {
		ResourceSaver::save(shader.ptr());
	}

	Ref<ShaderInclude> edited_shader_inc = code_editor->get_edited_shader_include();
	if (edited_shader_inc.is_valid()) {
		ResourceSaver::save(edited_shader_inc.ptr());
	}
	if (shader_inc.is_valid() && shader_inc != edited_shader_inc) {
		ResourceSaver::save(shader_inc.ptr());
	}
	code_editor->get_text_editor()->tag_saved_version();

	disk_changed->hide();
}

void TextShaderEditor::trim_trailing_whitespace() { code_editor->trim_trailing_whitespace(); }

void TextShaderEditor::trim_final_newlines() { code_editor->trim_final_newlines(); }

void TextShaderEditor::set_toggle_list_control(Control* p_toggle_list_control)
{
	code_editor->set_toggle_list_control(p_toggle_list_control);
}

void TextShaderEditor::update_toggle_files_button() { code_editor->update_toggle_files_button(); }

void TextShaderEditor::validate_script() { code_editor->_validate_script(); }

bool TextShaderEditor::is_unsaved() const
{
	return code_editor->get_text_editor()->get_saved_version() !=
		   code_editor->get_text_editor()->get_version();
}

void TextShaderEditor::tag_saved_version() { code_editor->get_text_editor()->tag_saved_version(); }

void TextShaderEditor::_update_shader_previews()
{
	pending_update_shader_previews = false;

	const CodeEdit* ce = code_editor->get_text_editor();
	code_editor->clear_previews();
	bool found = false;

	for (int i = 0; i < ce->get_line_count(); i++) {
		if (ce->is_line_breakpointed(i)) {
			found = true;
			code_editor->toggle_shader_preview(i);
		}
	}

	if (!found) {
		preview_box->hide();
		return;
	}
	preview_box->show();

	preview_timer->start();
}

void TextShaderEditor::_make_context_menu(bool p_selection, Vector2 p_position)
{
	context_menu->clear();
	if (DisplayServer::get_singleton()->has_feature(
			DisplayServerEnums::FEATURE_EMOJI_AND_SYMBOL_PICKER)) {
		context_menu->add_item(TTRC("Emoji & Symbols"), EDIT_EMOJI_AND_SYMBOL);
		context_menu->add_separator();
	}
	if (p_selection) {
		context_menu->add_shortcut(ED_GET_SHORTCUT("ui_cut"), EDIT_CUT);
		context_menu->add_shortcut(ED_GET_SHORTCUT("ui_copy"), EDIT_COPY);
	}

	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_paste"), EDIT_PASTE);
	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_text_select_all"), EDIT_SELECT_ALL);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_undo"), EDIT_UNDO);
	context_menu->add_shortcut(ED_GET_SHORTCUT("ui_redo"), EDIT_REDO);

	context_menu->add_separator();
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/indent"), EDIT_INDENT);
	context_menu->add_shortcut(ED_GET_SHORTCUT("script_text_editor/unindent"), EDIT_UNINDENT);
	context_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/toggle_comment"), EDIT_TOGGLE_COMMENT);
	context_menu->add_shortcut(
		ED_GET_SHORTCUT("script_text_editor/toggle_bookmark"), BOOKMARK_TOGGLE);

	context_menu->set_item_disabled(
		context_menu->get_item_index(EDIT_UNDO), !code_editor->get_text_editor()->has_undo());
	context_menu->set_item_disabled(
		context_menu->get_item_index(EDIT_REDO), !code_editor->get_text_editor()->has_redo());

	context_menu->set_position(get_screen_position() + p_position);
	context_menu->reset_size();
	context_menu->popup();
}

void TextShaderEditor::register_editor()
{
	ED_SHORTCUT("shader_text_editor/toggle_shader_preview", TTRC("Toggle Shader Preview"), Key::F9);
	ED_SHORTCUT_OVERRIDE("shader_text_editor/toggle_shader_preview", "macos",
		KeyModifierMask::META | KeyModifierMask::SHIFT | Key::B);

	ED_SHORTCUT("shader_text_editor/remove_all_shader_previews", TTRC("Remove All Shader Previews"),
		KeyModifierMask::CMD_OR_CTRL | KeyModifierMask::SHIFT | Key::F9);
	// Using Control for these shortcuts even on macOS because Command+Comma is taken for opening
	// Editor Settings.
	ED_SHORTCUT("shader_text_editor/goto_next_shader_preview", TTRC("Go to Next Shader Preview"),
		KeyModifierMask::CTRL | Key::PERIOD);
	ED_SHORTCUT("shader_text_editor/goto_previous_shader_preview",
		TTRC("Go to Previous Shader Preview"), KeyModifierMask::CTRL | Key::COMMA);
}


