/**************************************************************************/
/*  editor_help.cpp                                                       */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/core_constants.h"
#include "core/input/input.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/os/keyboard.h"
#include "core/os/os.h"
#include "core/string/string_builder.h"
#include "core/version.h"
#include "editor/doc/doc_data_compressed.gen.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/file_system/editor_paths.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/editor_property_name_processor.h"
#include "editor/script/script_editor_navigation_marker.h"
#include "editor/script/script_editor_plugin.h"
#include "editor/script/syntax_highlighters.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor_help.h"
#include "modules/modules_enabled.gen.h" // For gdscript, mono.
#include "scene/gui/line_edit.h"
#include "scene/resources/syntax_highlighter.h"
#include "servers/display/display_server.h"

// For syntax highlighting.
#ifdef MODULE_MONO_ENABLED
#include "modules/mono/csharp_script.h"
#endif

#include "modules/regex/regex.h"

#define CONTRIBUTE_URL                                                                             \
	"https://contributing.godotengine.org/en/latest/documentation/class_reference.html"

#ifdef MODULE_MONO_ENABLED
// Sync with the types mentioned in
// https://docs.godotengine.org/en/stable/tutorials/scripting/c_sharp/c_sharp_differences.html
const Vector<String> classes_with_csharp_differences = {
	"@GlobalScope",
	"String",
	"NodePath",
	"Signal",
	"Callable",
	"RID",
	"Basis",
	"Transform2D",
	"Transform3D",
	"Rect2",
	"Rect2i",
	"AABB",
	"Quaternion",
	"Projection",
	"Color",
	"Array",
	"Dictionary",
	"PackedByteArray",
	"PackedColorArray",
	"PackedFloat32Array",
	"PackedFloat64Array",
	"PackedInt32Array",
	"PackedInt64Array",
	"PackedStringArray",
	"PackedVector2Array",
	"PackedVector3Array",
	"PackedVector4Array",
	"Variant",
};
#endif

const Vector<String> packed_array_types = {
	"PackedByteArray",
	"PackedColorArray",
	"PackedFloat32Array",
	"PackedFloat64Array",
	"PackedInt32Array",
	"PackedInt64Array",
	"PackedStringArray",
	"PackedVector2Array",
	"PackedVector3Array",
	"PackedVector4Array",
};

static const char32_t nbsp_chr = 160;
static const String nbsp = String::chr(nbsp_chr);
static const String nbsp_equal_nbsp = nbsp + "=" + nbsp;
static const String colon_nbsp = ":" + nbsp;

static const char32_t wj_chr = 8288;
static const String cr_wj = "\r" + String::chr(wj_chr);

static String _fix_newlines(const String& p_string)
{
	// `\n` starts a new paragraph, `\r` just adds a break to existing one.
	// Add a non-printable character "WORD JOINER" so that multi-breaks work correctly
	// (`RichTextLabel` bug?).
	return p_string.replace("\n", cr_wj);
}

static String _fix_selection(const String& p_string)
{
	return p_string.replace_char(nbsp_chr, ' ').remove_char(wj_chr);
}

static String _fix_constant(const String& p_constant)
{
	if (p_constant.strip_edges() == "4294967295") {
		return "0xFFFFFFFF";
	}

	if (p_constant.strip_edges() == "2147483647") {
		return "0x7FFFFFFF";
	}

	if (p_constant.strip_edges() == "1048575") {
		return "0xFFFFF";
	}

	return p_constant;
}

static void _add_qualifiers_to_rt(const String& p_qualifiers, RichTextLabel* p_rt)
{
	for (const String& qualifier : p_qualifiers.split_spaces()) {
		String hint;
		if (qualifier == "vararg") {
			hint = TTR("This method supports a variable number of arguments.");
		}
		else if (qualifier == "virtual") {
			hint = TTR("This method is called by the engine.\nIt can be overridden to customize "
					   "built-in behavior.");
		}
		else if (qualifier == "required") {
			hint = TTR("This method is required to be overridden when extending its base class.");
		}
		else if (qualifier == "const") {
			hint =
				TTR("This method has no side effects.\nIt does not modify the object in any way.");
		}
		else if (qualifier == "static") {
			hint = TTR("This method does not need an instance to be called.\nIt can be called "
					   "directly using the class name.");
		}
		else if (qualifier == "abstract") {
			hint = TTR("This method must be implemented to complete the abstract class.");
		}

		p_rt->add_text(" ");
		if (hint.is_empty()) {
			p_rt->add_text(qualifier);
		}
	}
}

// Removes unnecessary prefix from `p_class_specifier` when within the `p_edited_class` context.
static String _contextualize_class_specifier(
	const String& p_class_specifier, const String& p_edited_class)
{
	// If this is a completely different context than the current class, then keep full path.
	if (!p_class_specifier.begins_with(p_edited_class)) {
		return p_class_specifier;
	}

	// Here equal `length()` and `begins_with()` from above implies `p_class_specifier ==
	// p_edited_class`.
	if (p_class_specifier.length() == p_edited_class.length()) {
		int rfind = p_class_specifier.rfind_char('.');
		if (rfind == -1) { // Single identifier.
			return p_class_specifier;
		}
		// Multiple specifiers: keep last one only.
		return p_class_specifier.substr(rfind + 1);
	}

	// They share a _name_ prefix but not a _class specifier_ prefix, e.g. `Tree` and `TreeItem`.
	// `begins_with()` and `length()`s being different implies `p_class_specifier.length() >
	// p_edited_class.length()` so this is safe.
	if (p_class_specifier[p_edited_class.length()] != '.') {
		return p_class_specifier;
	}

	// Remove class specifier prefix.
	return p_class_specifier.substr(p_edited_class.length() + 1);
}

void EditorHelp::_search(bool p_search_previous)
{
	if (p_search_previous) {
		find_bar->search_prev();
	}
	else {
		find_bar->search_next();
	}
}

void EditorHelp::_class_desc_select(const String& p_select)
{
	if (p_select.begins_with("$")) { // Enum.
		const String link = p_select.substr(1);

		String enum_class_name;
		String enum_name;
		if (CoreConstants::is_global_enum(link)) {
			enum_class_name = "@GlobalScope";
			enum_name = link;
		}
		else {
			const int dot_pos = link.rfind_char('.');
			if (dot_pos >= 0) {
				enum_class_name = link.left(dot_pos);
				enum_name = link.substr(dot_pos + 1);
			}
			else {
				enum_class_name = edited_class;
				enum_name = link;
			}
		}
	}
	else if (p_select.begins_with("@")) { // Member.
		const int tag_end = p_select.find_char(' ');
		const String tag = p_select.substr(1, tag_end - 1);
		const String link = p_select.substr(tag_end + 1).lstrip(" ");

		String topic;
		const HashMap<String, int>* table = nullptr;

		if (tag == "method") {
			topic = "class_method";
			table = &method_line;
		}
		else if (tag == "constructor") {
			topic = "class_method";
			table = &method_line;
		}
		else if (tag == "operator") {
			topic = "class_method";
			table = &method_line;
		}
		else if (tag == "member") {
			topic = "class_property";
			table = &property_line;
		}
		else if (tag == "enum") {
			topic = "class_enum";
			table = &enum_line;
		}
		else if (tag == "signal") {
			topic = "class_signal";
			table = &signal_line;
		}
		else if (tag == "constant") {
			topic = "class_constant";
			table = &constant_line;
		}
		else if (tag == "annotation") {
			topic = "class_annotation";
			table = &annotation_line;
		}
		else if (tag == "theme_item") {
			topic = "class_theme_item";
			table = &theme_property_line;
		}
		else {
			return;
		}

		// Case order is important here to correctly handle edge cases like `Variant.Type` in
		// `@GlobalScope`.
		if (table->has(link)) {
			// Found in the current page.
			ScriptEditorNavigationMarker::get_singleton()->locate_begin();
			if (class_desc->is_finished()) {
				_class_desc_scroll_to_paragraph((*table)[link], _need_save_new_history());
			}
			else {
				scroll_to = (*table)[link];
				need_save_new_history = _need_save_new_history();
			}
			ScriptEditorNavigationMarker::get_singleton()->locate_end();
		}
	}
	else if (p_select.begins_with("http:") || p_select.begins_with("https:")) {
		OS::get_singleton()->shell_open(p_select);
	}
	else if (p_select.begins_with("^")) { // Copy button.
		DisplayServer::get_singleton()->clipboard_set(p_select.substr(1));
		EditorToaster::get_singleton()->popup_str(
			TTR("Code snippet copied to clipboard."), EditorToaster::SEVERITY_INFO);
	}
}

void EditorHelp::_class_desc_input(const Ref<InputEvent>& p_input) {}

void EditorHelp::_class_desc_resized(bool p_force_update_theme)
{
	// Add extra horizontal margins for better readability.
	// The margins increase as the width of the editor help container increases.
	real_t char_width =
		theme_cache.doc_code_font->get_char_size('x', theme_cache.doc_code_font_size).width;
	const int new_display_margin =
		MAX(30 * EDSCALE, get_parent_anchorable_rect().size.width - char_width * 120 * EDSCALE) *
		0.5;
	if (display_margin != new_display_margin || p_force_update_theme) {
		display_margin = new_display_margin;

		Ref<StyleBox> class_desc_stylebox = theme_cache.background_style->duplicate();
		class_desc_stylebox->set_content_margin(SIDE_LEFT, display_margin);
		class_desc_stylebox->set_content_margin(SIDE_RIGHT, display_margin);
		class_desc->add_theme_style_override("focused", class_desc_stylebox.ptr());
	}
}

// Macros for assigning the deprecated/experimental marks to class members in overview.
#define DEPRECATED_DOC_TAG                                                                         \
	class_desc->push_font(theme_cache.doc_bold_font);                                              \
	class_desc->push_color(get_theme_color(SNAME("error_color"), EditorStringName(Editor)));       \
	Ref<Texture2D> error_icon = get_editor_theme_icon(SNAME("StatusError"));                       \
	class_desc->add_image(error_icon, error_icon->get_width(), error_icon->get_height());          \
	class_desc->add_text(String::chr(160) + TTR("Deprecated"));                                    \
	class_desc->pop();                                                                             \
	class_desc->pop();

#define EXPERIMENTAL_DOC_TAG                                                                       \
	class_desc->push_font(theme_cache.doc_bold_font);                                              \
	class_desc->push_color(get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));     \
	Ref<Texture2D> warning_icon = get_editor_theme_icon(SNAME("NodeWarning"));                     \
	class_desc->add_image(warning_icon, warning_icon->get_width(), warning_icon->get_height());    \
	class_desc->add_text(String::chr(160) + TTR("Experimental"));                                  \
	class_desc->pop();                                                                             \
	class_desc->pop();

// Macros for displaying the deprecated/experimental info in class member descriptions.
#define DEPRECATED_DOC_MSG(m_message, m_default_message)                                           \
	Ref<Texture2D> error_icon = get_editor_theme_icon(SNAME("StatusError"));                       \
	class_desc->add_image(error_icon, error_icon->get_width(), error_icon->get_height());          \
	class_desc->add_text(nbsp);                                                                    \
	class_desc->push_color(get_theme_color(SNAME("error_color"), EditorStringName(Editor)));       \
	class_desc->push_font(theme_cache.doc_bold_font);                                              \
	class_desc->add_text(TTR("Deprecated:"));                                                      \
	class_desc->pop();                                                                             \
	class_desc->pop();                                                                             \
	class_desc->add_text(" ");                                                                     \
	if ((m_message).is_empty()) {                                                                  \
		class_desc->add_text(m_default_message);                                                   \
	} \
	else {                                                                                       \
		_add_text(m_message);                                                                      \
	}

#define EXPERIMENTAL_DOC_MSG(m_message, m_default_message)                                         \
	Ref<Texture2D> warning_icon = get_editor_theme_icon(SNAME("NodeWarning"));                     \
	class_desc->add_image(warning_icon, warning_icon->get_width(), warning_icon->get_height());    \
	class_desc->add_text(nbsp);                                                                    \
	class_desc->push_color(get_theme_color(SNAME("warning_color"), EditorStringName(Editor)));     \
	class_desc->push_font(theme_cache.doc_bold_font);                                              \
	class_desc->add_text(TTR("Experimental:"));                                                    \
	class_desc->pop();                                                                             \
	class_desc->pop();                                                                             \
	class_desc->add_text(" ");                                                                     \
	if ((m_message).is_empty()) {                                                                  \
		class_desc->add_text(m_default_message);                                                   \
	} \
	else {                                                                                       \
		_add_text(m_message);                                                                      \
	}

void EditorHelp::_add_bulletpoint()
{
	static const char32_t prefix[3] = {0x25CF /* filled circle */, ' ', 0};
	class_desc->add_text(String(prefix));
}

bool EditorHelp::_need_save_new_history() const
{
	return !ScriptEditorNavigationMarker::get_singleton()->is_initializing() &&
		   ScriptEditorNavigationMarker::get_singleton()->is_locating();
}

void EditorHelp::_wait_for_thread(Thread& p_thread)
{
	if (p_thread.is_started()) {
		p_thread.wait_to_finish();
	}
}

String EditorHelp::get_cache_full_path()
{
	return EditorPaths::get_singleton()->get_cache_dir().path_join(
		vformat("editor_doc_cache-%d.%d.res", VLTR_VERSION_MAJOR, VLTR_VERSION_MINOR));
}

String EditorHelp::get_script_doc_cache_full_path()
{
	return EditorPaths::get_singleton()->get_project_settings_dir().path_join(
		"editor_script_doc_cache.res");
}

DocTools* EditorHelp::get_doc_data()
{
	_wait_for_thread();
	return doc;
}

void EditorHelp::remove_doc(const String& p_class_name)
{
	if (!_script_docs_loaded.is_set()) {
		_docs_to_remove.push_back(p_class_name);
		return;
	}

	DocTools* dt = get_doc_data();
	if (dt->has_doc(p_class_name)) {
		dt->remove_doc(p_class_name);
	}
}

void EditorHelp::remove_script_doc_by_path(const String& p_path)
{
	if (!_script_docs_loaded.is_set()) {
		_docs_to_remove_by_path.push_back(p_path);
		return;
	}
	get_doc_data()->remove_script_doc_by_path(p_path);
}

void EditorHelp::load_xml_buffer(const uint8_t* p_buffer, int p_size)
{
	if (!ext_doc) {
		ext_doc = memnew(DocTools);
	}

	ext_doc->load_xml(p_buffer, p_size);

	if (doc) {
		doc->load_xml(p_buffer, p_size);
	}
}

void EditorHelp::remove_class(const String& p_class)
{
	if (ext_doc && ext_doc->has_doc(p_class)) {
		ext_doc->remove_doc(p_class);
	}

	if (doc && doc->has_doc(p_class)) {
		remove_doc(p_class);
	}
}

void EditorHelp::_gen_extensions_docs()
{
	doc->generate((
		DocTools::GENERATE_FLAG_SKIP_BASIC_TYPES | DocTools::GENERATE_FLAG_EXTENSION_CLASSES_ONLY));

	// Append extra doc data, as it gets overridden by the generation step.
	if (ext_doc) {
		doc->merge_from(*ext_doc);
	}
}

static void _load_script_doc_cache(bool p_changes) { EditorHelp::load_script_doc_cache(); }

void EditorHelp::load_script_doc_cache()
{
	if (!ProjectSettings::get_singleton()->is_project_loaded()) {
		print_verbose("Skipping loading script doc cache since no project is open.");
		return;
	}

	if (EditorNode::is_cmdline_mode()) {
		return;
	}

	_wait_for_thread();

	if (!ResourceLoader::exists(get_script_doc_cache_full_path())) {
		print_verbose("Script documentation cache not found. Regenerating it may take a while for "
					  "projects with many scripts.");
		regenerate_script_doc_cache();
		return;
	}

	if (EditorFileSystem::get_singleton()->is_scanning()) {
		// This is assuming EditorFileSystem is performing first scan. We must wait until it is
		// done.
		return;
	}

	worker_thread.start(_load_script_doc_cache_thread, nullptr);
}

void EditorHelp::_process_postponed_docs()
{
	for (const String& class_name : _docs_to_remove) {
		doc->remove_doc(class_name);
	}
	for (const String& path : _docs_to_remove_by_path) {
		doc->remove_script_doc_by_path(path);
	}
	_docs_to_remove.clear();
	_docs_to_remove_by_path.clear();
}

void EditorHelp::_load_script_doc_cache_thread(void* p_udata)
{
	ERR_FAIL_COND_MSG(!ProjectSettings::get_singleton()->is_project_loaded(),
		"Error: cannot load script doc cache without a project.");
	ERR_FAIL_COND_MSG(!ResourceLoader::exists(get_script_doc_cache_full_path()),
		"Error: cannot load script doc cache from inexistent file.");

	Ref<Resource> script_doc_cache_res = ResourceLoader::load(
		get_script_doc_cache_full_path(), "", ResourceFormatLoader::CACHE_MODE_IGNORE);
	if (script_doc_cache_res.is_null()) {
		print_verbose("Script doc cache is corrupted. Regenerating it instead.");
		_delete_script_doc_cache();
		return;
	}

	// Protect from race condition in other threads reading / this thread writing to
	// _docs_to_add/remove/etc.
	_script_docs_loaded.set();

	// Deal with docs likely added from EditorFileSystem's scans while the cache was loading in
	// EditorHelp::worker_thread.
	_process_postponed_docs();

	// Always delete the doc cache after successful load since most uses of editor will change a
	// script, invalidating cache.
	_delete_script_doc_cache();
}

// Helper method to deal with "sources_changed" signal having a parameter.
static void _regenerate_script_doc_cache(bool p_changes)
{
	EditorHelp::regenerate_script_doc_cache();
}

void EditorHelp::regenerate_script_doc_cache()
{
	if (EditorFileSystem::get_singleton()->is_scanning()) {
		// Wait until EditorFileSystem scanning is complete to use updated filesystem structure.
		return;
	}

	_wait_for_thread(worker_thread);
	_wait_for_thread(loader_thread);
	loader_thread.start(
		_regen_script_doc_thread, EditorFileSystem::get_singleton()->get_filesystem());
}

// Runs on worker_thread since it writes to DocData.
void EditorHelp::_finish_regen_script_doc_thread(void* p_udata)
{
	loader_thread.wait_to_finish();
	_process_postponed_docs();
	_script_docs_loaded.set();

	OS::get_singleton()->benchmark_end_measure("EditorHelp", "Generate Script Documentation");
}

// Runs on loader_thread since _reload_scripts_documentation calls ResourceLoader::load().
// Avoids deadlocks of worker_thread needing main thread for load task dispatching, but main thread
// waiting on worker_thread.
void EditorHelp::_regen_script_doc_thread(void* p_udata)
{
	OS::get_singleton()->benchmark_begin_measure("EditorHelp", "Generate Script Documentation");

	EditorFileSystemDirectory* dir = static_cast<EditorFileSystemDirectory*>(p_udata);
	_script_docs_loaded.set_to(false);

	// Ignore changes from filesystem scan since script docs will be now.
	_docs_to_remove.clear();
	_docs_to_remove_by_path.clear();

	_reload_scripts_documentation(dir);

	// All ResourceLoader::load() calls are done, so we can no longer deadlock with main thread.
	// Switch to back to worker_thread from loader_thread to resynchronize access to DocData.
	worker_thread.start(_finish_regen_script_doc_thread, nullptr);
}

void EditorHelp::_reload_scripts_documentation(EditorFileSystemDirectory* p_dir)
{
	// Recursively force compile all scripts, which should generate their documentation.
	for (int i = 0; i < p_dir->get_subdir_count(); i++) {
		_reload_scripts_documentation(p_dir->get_subdir(i));
	}
}

void EditorHelp::_delete_script_doc_cache()
{
	if (FileAccess::exists(get_script_doc_cache_full_path())) {
		DirAccess::remove_file_or_error(
			ProjectSettings::get_singleton()->globalize_path(get_script_doc_cache_full_path()));
	}
}

void EditorHelp::generate_doc(bool p_use_cache, bool p_use_script_cache)
{
	doc_generation_count++;
	OS::get_singleton()->benchmark_begin_measure(
		"EditorHelp", vformat("Generate Documentation (Run %d)", doc_generation_count));

	// In case not the first attempt.
	_wait_for_thread();

	if (!doc) {
		doc = memnew(DocTools);
	}

	if (doc_version_hash.is_empty()) {
		_compute_doc_version_hash();
	}

	if (p_use_cache && FileAccess::exists(get_cache_full_path())) {
		worker_thread.start(_load_doc_thread, (void*)p_use_script_cache);
	}
	else {
		print_verbose("Regenerating editor help cache");
		doc->generate();
		worker_thread.start(_gen_doc_thread, (void*)p_use_script_cache);
	}
}

void EditorHelp::_toggle_files_pressed()
{
	ScriptEditor::get_singleton()->toggle_files_panel();
	update_toggle_files_button();
}

void EditorHelp::_notification(int p_what)
{
	switch (p_what) {
	case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
		bool need_update = false;
		if (EditorSettings::get_singleton()->check_changed_settings_in_group("text_editor/help")) {
			need_update = true;
		}
#if defined(MODULE_GDSCRIPT_ENABLED) || defined(MODULE_MONO_ENABLED)
		if (!need_update && EditorSettings::get_singleton()->check_changed_settings_in_group(
								"text_editor/theme/highlighting")) {
			need_update = true;
		}
#endif
		if (!need_update) {
			break;
		}
		[[fallthrough]];
	}
	case NOTIFICATION_READY: {
		_wait_for_thread();
		_update_doc();
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		if (is_inside_tree()) {
			if (is_visible_in_tree()) {
				_update_doc();
			}
			else {
				update_pending = true;
			}

			_class_desc_resized(true);
		}
		update_toggle_files_button();
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		if (update_pending && is_visible_in_tree()) {
			_update_doc();
		}
		update_toggle_files_button();
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (!is_ready()) {
			break;
		}

		if (is_visible_in_tree()) {
			_update_doc();
		}
		else {
			update_pending = true;
		}
		[[fallthrough]];
	}
	case NOTIFICATION_LAYOUT_DIRECTION_CHANGED: {
		update_toggle_files_button();
	} break;
	}
}

void EditorHelp::cleanup_doc()
{
	_wait_for_thread();
	memdelete(doc);
	doc = nullptr;
}

Vector<Pair<String, int>> EditorHelp::get_sections()
{
	_wait_for_thread();
	Vector<Pair<String, int>> sections;

	for (int i = 0; i < section_line.size(); i++) {
		sections.push_back(Pair<String, int>(section_line[i].first, i));
	}
	return sections;
}

void EditorHelp::scroll_to_section(int p_section_index)
{
	_wait_for_thread();
	ScriptEditorNavigationMarker::get_singleton()->locate_begin();
	int line = section_line[p_section_index].second;
	if (class_desc->is_finished()) {
		_class_desc_scroll_to_paragraph(line, _need_save_new_history());
	}
	else {
		scroll_to = line;
		need_save_new_history = _need_save_new_history();
	}
	ScriptEditorNavigationMarker::get_singleton()->locate_end();
}

String EditorHelp::get_class() { return edited_class; }

void EditorHelp::search_again(bool p_search_previous) { _search(p_search_previous); }

int EditorHelp::get_scroll() const { return class_desc->get_v_scroll_bar()->get_value(); }

void EditorHelp::set_scroll(int p_scroll) { class_desc->get_v_scroll_bar()->set_value(p_scroll); }

void EditorHelpBit::_go_to_url(const String& p_what)
{
	Vector<String> parts;
	{
		int from = 0;
		int buffer_start = 0;
		while (true) {
			const int pos = p_what.find_char(':', from);
			if (pos < 0) {
				parts.push_back(p_what.substr(buffer_start));
				break;
			}

			if (pos + 1 < p_what.length() && p_what[pos + 1] == ':') {
				// `::` used in built-in scripts.
				from = pos + 2;
			}
			else {
				parts.push_back(p_what.substr(buffer_start, pos - buffer_start));
				from = pos + 1;
				buffer_start = from;
			}
		}
	}

	const String what = parts[0]; // `parts` is always non-empty.
	const String clss = (parts.size() > 1) ? parts[1].to_lower() : String();
	const String name =
		(parts.size() > 2) ? parts[2].to_lower().replace_chars("/_", '-') : String();

	String section = "";
	if (what == "class_desc") {
		section = "#description";
	}
	else if (what == "class_signal") {
		section = vformat("#class-%s-signal-%s", clss, name);
	}
	else if (what == "class_method" || what == "class_method_desc") {
		section = vformat("#class-%s-method-%s", clss, name);
	}
	else if (what == "class_property") {
		section = vformat("#class-%s-property-%s", clss, name);
	}
	else if (what == "class_enum") {
		section = vformat("#enum-%s-%s", clss, name);
	}
	else if (what == "class_theme_item") {
		section = vformat("#class-%s-theme-%s", clss, name);
	}
	else if (what == "class_constant") {
		section = vformat("#class-%s-constant-%s", clss, name);
	}
	else if (what == "class_annotation") {
		section = vformat("#%s", clss);
	}

	String doc_url = clss.is_empty()
						 ? String(VLTR_VERSION_DOCS_URL "/")
						 : vformat(VLTR_VERSION_DOCS_URL "/classes/class_%s.html%s", clss, section);
	OS::get_singleton()->shell_open(doc_url);
}

void EditorHelpBit::_go_to_help(const String& p_what)
{
	if (ScriptEditor::get_singleton()) {
		EditorNode::get_singleton()->get_editor_main_screen()->select(
			EditorMainScreen::EDITOR_SCRIPT);
		ScriptEditor::get_singleton()->goto_help(p_what);
	}
	else {
		_go_to_url(p_what);
	}
}

void EditorHelpBit::_meta_clicked(const String& p_select)
{
	if (p_select.begins_with("$")) { // Enum.
		const String link = p_select.substr(1);

		String enum_class_name;
		String enum_name;
		if (CoreConstants::is_global_enum(link)) {
			enum_class_name = "@GlobalScope";
			enum_name = link;
		}
		else {
			const int dot_pos = link.rfind_char('.');
			if (dot_pos >= 0) {
				enum_class_name = link.left(dot_pos);
				enum_name = link.substr(dot_pos + 1);
			}
			else {
				enum_class_name = symbol_class_name;
				enum_name = link;
			}
		}

		_go_to_help("class_enum:" + enum_class_name + ":" + enum_name);
	}
	else if (p_select.begins_with("#")) { // Class.
		_go_to_help("class_name:" + p_select.substr(1));
	}
	else if (p_select.begins_with("@")) { // Member.
		const int tag_end = p_select.find_char(' ');
		const String tag = p_select.substr(1, tag_end - 1);
		const String link = p_select.substr(tag_end + 1).lstrip(" ");

		String topic;
		if (tag == "method") {
			topic = "class_method";
		}
		else if (tag == "constructor") {
			topic = "class_method";
		}
		else if (tag == "operator") {
			topic = "class_method";
		}
		else if (tag == "member") {
			topic = "class_property";
		}
		else if (tag == "enum") {
			topic = "class_enum";
		}
		else if (tag == "signal") {
			topic = "class_signal";
		}
		else if (tag == "constant") {
			topic = "class_constant";
		}
		else if (tag == "annotation") {
			topic = "class_annotation";
		}
		else if (tag == "theme_item") {
			topic = "class_theme_item";
		}
		else {
			return;
		}

		if (topic == "class_enum") {
			const String enum_link = link.trim_prefix("@GlobalScope.");
			if (CoreConstants::is_global_enum(enum_link)) {
				_go_to_help(topic + ":@GlobalScope:" + enum_link);
				return;
			}
		}
		else if (topic == "class_constant") {
			if (CoreConstants::is_global_constant(link)) {
				_go_to_help(topic + ":@GlobalScope:" + link);
				return;
			}
		}

		if (link.contains_char('.')) {
			const int class_end = link.rfind_char('.');
			_go_to_help(topic + ":" + link.left(class_end) + ":" + link.substr(class_end + 1));
		}
		else {
			_go_to_help(topic + ":" + symbol_class_name + ":" + link);
		}
	}
	else if (p_select.begins_with("open-file:")) {
		String path =
			ProjectSettings::get_singleton()->globalize_path(p_select.trim_prefix("open-file:"));
		OS::get_singleton()->shell_show_in_file_manager(path, true);
	}
	else if (p_select.begins_with("http:") || p_select.begins_with("https:")) {
		OS::get_singleton()->shell_open(p_select);
	}
	else if (p_select.begins_with("^")) { // Copy button.
		DisplayServer::get_singleton()->clipboard_set(p_select.substr(1));
		EditorToaster::get_singleton()->popup_str(
			TTR("Code snippet copied to clipboard."), EditorToaster::SEVERITY_INFO);
	}
}

String EditorHelpBit::get_as_plain_text(const String& p_symbol, const String& p_prologue)
{
	const PackedStringArray slices = p_symbol.split("|", true, 3);
	ERR_FAIL_COND_V_MSG(slices.size() < 3, String(),
		R"(Invalid doc id: The expected format is "item_type|class_name|item_name[|item_data]".)");

	const String& item_type = slices[0];
	const String& class_name = slices[1];
	const String& item_name = slices[2];

	HelpData new_help_data = HelpData();

	if (item_type == "class") {
		new_help_data = _get_class_help_data(class_name);
	}
	else if (item_type == "enum") {
		new_help_data = _get_enum_help_data(class_name, item_name);
	}
	else if (item_type == "constant") {
		new_help_data = _get_constant_help_data(class_name, item_name);
	}
	else if (item_type == "property") {
		new_help_data = _get_property_help_data(class_name, item_name);
		// Add copy note to built-in properties returning `Packed*Array`.
	}
	else if (item_type == "internal_property") {
		new_help_data.description = TTR("This property can only be set in the Inspector.");
	}
	else if (item_type == "theme_item") {
		new_help_data = _get_theme_item_help_data(class_name, item_name);
	}
	else if (item_type == "method") {
		new_help_data = _get_method_help_data(class_name, item_name);
	}
	else if (item_type == "signal") {
		new_help_data = _get_signal_help_data(class_name, item_name);
	}
	else if (item_type == "annotation") {
		new_help_data = _get_annotation_help_data(class_name, item_name);
	}

	if (!p_prologue.is_empty()) {
		if (new_help_data.description.is_empty()) {
			new_help_data.description = p_prologue;
		}
		else {
			new_help_data.description = p_prologue + "\n" + new_help_data.description;
		}
	}

	if (new_help_data.description.is_empty() && item_type != "resource") {
		new_help_data.description = TTR("No description available.");
	}

	String bbcode = new_help_data.description;
	if (!new_help_data.deprecated_message.is_empty()) {
		bbcode += "\n" + new_help_data.deprecated_message;
	}
	if (!new_help_data.experimental_message.is_empty()) {
		bbcode += "\n" + new_help_data.experimental_message;
	}

	if (bbcode.is_empty()) {
		return String();
	}

	DocTools* doc = EditorHelp::get_doc_data();

	StringBuilder output;

	List<String> tag_stack;
	int pos = 0;
	while (pos < bbcode.length()) {
		int brk_pos = bbcode.find_char('[', pos);

		if (brk_pos < 0) {
			brk_pos = bbcode.length();
		}

		if (brk_pos > pos) {
			String text = bbcode.substr(pos, brk_pos - pos);
			output.append(text);
		}

		if (brk_pos == bbcode.length()) {
			// Nothing else to add.
			break;
		}

		int brk_end = bbcode.find_char(']', brk_pos + 1);

		if (brk_end == -1) {
			String text = bbcode.substr(brk_pos);
			output.append(text);
			break;
		}

		String tag = bbcode.substr(brk_pos + 1, brk_end - brk_pos - 1);

		if (tag.begins_with("/")) {
			bool tag_ok = tag_stack.size() && tag_stack.front()->get() == tag.substr(1);

			if (!tag_ok) {
				output.append("]");
				pos = brk_pos + 1;
				continue;
			}

			tag_stack.pop_front();
			pos = brk_end + 1;
		}
		else if (tag.begins_with("method ") || tag.begins_with("constructor ") ||
				   tag.begins_with("operator ") || tag.begins_with("member ") ||
				   tag.begins_with("signal ") || tag.begins_with("enum ") ||
				   tag.begins_with("constant ") || tag.begins_with("theme_item ") ||
				   tag.begins_with("param ")) {
			int tag_end = tag.find_char(' ');
			String link_tag = tag.left(tag_end);
			String link_target = tag.substr(tag_end + 1).lstrip(" ");
			if (link_tag == "param") {
				link_tag = "parameter";
			}
			else if (link_tag == "theme_item") {
				link_tag = "theme item";
			}
			output.append(link_tag + " '" + link_target + "'");
			pos = brk_end + 1;
		}
		else if (tag == "codeblocks") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "codeblock" || tag.begins_with("codeblock ")) {
			pos = brk_end + 1;
			tag_stack.push_front("codeblock");
		}
		else if (tag.begins_with("gdscript")) {
			pos = brk_end + 1;
			output.append("GDScript:\n");
			tag_stack.push_front("gdscript");
		}
		else if (tag.begins_with("csharp")) {
			pos = brk_end + 1;
			output.append("C#:\n");
			tag_stack.push_front("csharp");
		}
		else if (tag == "b") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "i") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "code" || tag.begins_with("code ")) {
			pos = brk_end + 1;
			tag_stack.push_front("code");
		}
		else if (tag == "kbd") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "center") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "br") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "u") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "s") {
			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag == "url") {
			int end = bbcode.find_char('[', brk_end);
			if (end == -1) {
				end = bbcode.length();
			}
			String url = bbcode.substr(brk_end + 1, end - brk_end - 1);
			output.append(url);

			pos = brk_end + 1;
			tag_stack.push_front(tag);
		}
		else if (tag.begins_with("url=")) {
			pos = brk_end + 1;
			tag_stack.push_front("url");
		}
		else if (tag == "img") {
			int end = bbcode.find_char('[', brk_end);
			if (end == -1) {
				end = bbcode.length();
			}
			String image = bbcode.substr(brk_end + 1, end - brk_end - 1);

			output.append("(Image: ");
			output.append(image);
			output.append(")");

			pos = end;
			tag_stack.push_front(tag);
		}
		else if (tag.begins_with("color=")) {
			pos = brk_end + 1;
			tag_stack.push_front("color");
		}
		else if (tag.begins_with("font=")) {
			pos = brk_end + 1;
			tag_stack.push_front("font");
		}
		else {
			output.append("[");
			pos = brk_pos + 1;
		}
	}

	return output.as_string();
}

void EditorHelpBit::set_content_height_limits(float p_min, float p_max)
{
	ERR_FAIL_COND(p_min > p_max);
	content_min_height = p_min;
	content_max_height = p_max;

	if (is_inside_tree()) {
		update_content_height();
	}
}

void EditorHelpBit::update_content_height()
{
	float content_height = content->get_content_height();
	content->set_custom_minimum_size(Size2(content->get_custom_minimum_size().x,
		CLAMP(content_height, content_min_height, content_max_height)));
}

bool EditorHelpBitTooltip::_is_tooltip_visible = false;

void EditorHelpBitTooltip::_target_gui_input(const Ref<InputEvent>& p_event)
{
	// Only scrolling is not checked in `NOTIFICATION_INTERNAL_PROCESS`.
	const Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		switch (mb->get_button_index()) {
		case MouseButton::WHEEL_UP:
		case MouseButton::WHEEL_DOWN:
		case MouseButton::WHEEL_LEFT:
		case MouseButton::WHEEL_RIGHT:
			queue_free();
			break;
		default:
			break;
		}
	}

	const Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed()) {
		queue_free();
	}
}

void EditorHelpBitTooltip::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE:
		_is_tooltip_visible = true;
		_enter_tree_time = OS::get_singleton()->get_ticks_msec();
		break;
	case NOTIFICATION_EXIT_TREE:
		_is_tooltip_visible = false;
		break;
	case NOTIFICATION_WM_MOUSE_ENTER:
		_is_mouse_inside_tooltip = true;
		timer->stop();
		break;
	}
}

EditorHelpHighlighter* EditorHelpHighlighter::singleton = nullptr;

void EditorHelpHighlighter::create_singleton()
{
	ERR_FAIL_COND(singleton != nullptr);
	singleton = memnew(EditorHelpHighlighter);
}

void EditorHelpHighlighter::free_singleton()
{
	ERR_FAIL_NULL(singleton);
	memdelete(singleton);
	singleton = nullptr;
}

EditorHelpHighlighter* EditorHelpHighlighter::get_singleton() { return singleton; }

EditorHelpHighlighter::HighlightData EditorHelpHighlighter::_get_highlight_data(
	Language p_language, const String& p_source, bool p_use_cache)
{
	switch (p_language) {
	case LANGUAGE_GDSCRIPT:
#ifndef MODULE_GDSCRIPT_ENABLED
		ERR_FAIL_V_MSG(HighlightData(), "GDScript module is disabled.");
#endif
		break;
	case LANGUAGE_CSHARP:
#ifndef MODULE_MONO_ENABLED
		ERR_FAIL_V_MSG(HighlightData(), "Mono module is disabled.");
#endif
		break;
	default:
		ERR_FAIL_V_MSG(HighlightData(), "Invalid parameter \"p_language\".");
	}

	if (p_use_cache) {
		const HashMap<String, HighlightData>::ConstIterator E =
			highlight_data_caches[p_language].find(p_source);
		if (E) {
			return E->value;
		}
	}

	text_edits[p_language]->set_text(p_source);
	highlighters[p_language]->_update_cache();

	HighlightData result;

	int source_offset = 0;
	int result_index = 0;
	for (int i = 0; i < text_edits[p_language]->get_line_count(); i++) {
		int prev_column = -1;
		source_offset += text_edits[p_language]->get_line(i).length() + 1; // Plus newline.
	}

	if (p_use_cache) {
		highlight_data_caches[p_language][p_source] = result;
	}

	return result;
}

void EditorHelpHighlighter::reset_cache()
{
#ifdef MODULE_GDSCRIPT_ENABLED
	highlight_data_caches[LANGUAGE_GDSCRIPT].clear();
	text_edits[LANGUAGE_GDSCRIPT]->add_theme_color_override(
		SceneStringName(font_color), text_color);
#endif

#ifdef MODULE_MONO_ENABLED
	highlight_data_caches[LANGUAGE_CSHARP].clear();
	text_edits[LANGUAGE_CSHARP]->add_theme_color_override(SceneStringName(font_color), text_color);
#endif
}

EditorHelpHighlighter::EditorHelpHighlighter()
{
#ifdef MODULE_MONO_E
	NABLED
	TextEdit* csharp_text_edit = memnew(TextEdit);
	csharp_text_edit->add_theme_color_override(SceneStringName(font_color), text_color);

	// See G
H-89610.
	// Ref<CSharpScript> csharp;
	// csharp.instantiate();

	Ref<EditorStandardSyntaxHighlighter> csharp_highlighter;
	csharp_highlighter.instantiate();
	csharp_highlighter->set_text_edit(csharp_text_edit);
	// csharp_highlighter->_set_edited_resource(csharp);
	csharp_highlighter->_set_script_language(CSharpLanguage::get_singleton());

	text_edits[LANGUAGE_CSHARP] = csharp_text_edit;
	// scripts[LANGUAGE_CSHARP] = csharp;
	highlighters[LANGUAGE_CSHARP] = csharp_highlighter;
#endif
}

EditorHelpHighlighter::~EditorHelpHighlighter()
{
#ifdef MODULE_GDSCRIPT_ENABLED
	memdelete(text_edits[LANGUAGE_GDSCRIPT]);
#endif

#ifdef MODULE_MONO_ENABLED
	memdelete(text_edits[LANGUAGE_CSHARP]);
#endif
}

void FindBar::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED: {
		set_process_input(is_visible_in_tree());
	} break;
	}
}

void FindBar::set_rich_text_label(RichTextLabel* p_rich_text_label)
{
	rich_text_label = p_rich_text_label;
}

bool FindBar::search_next() { return _search(); }

bool FindBar::search_prev() { return _search(true); }

bool FindBar::_search(bool p_search_previous)
{
	String stext = search_text->get_text();
	bool keep = prev_search == stext;
	bool ret = rich_text_label->search(stext, keep, p_search_previous);

	prev_search = stext;
	if (!keep) {
		results_count_to_current = 0;
	}

	if (ret) {
		_update_results_count(p_search_previous);
	}
	else {
		results_count = 0;
		results_count_to_current = 0;
	}

	if (results_count == 1) {
		rich_text_label->scroll_to_selection();
	}

	_update_matches_label();

	return ret;
}

void FindBar::_update_results_count(bool p_search_previous)
{
	results_count = 0;

	String searched = search_text->get_text();
	if (searched.is_empty()) {
		return;
	}

	String full_text = rich_text_label->get_parsed_text();

	int from_pos = 0;

	while (true) {
		int pos = full_text.findn(searched, from_pos);
		if (pos == -1) {
			break;
		}

		results_count++;
		from_pos = pos + searched.length();
	}

	results_count_to_current += (p_search_previous) ? -1 : 1;
	if (results_count_to_current > results_count) {
		results_count_to_current = results_count_to_current - results_count;
	}
	else if (results_count_to_current <= 0) {
		results_count_to_current = results_count;
	}
}

void FindBar::_search_text_changed(const String& p_text) { search_next(); }

void FindBar::_search_text_submitted(const String& p_text)
{
	if (Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
		search_prev();
	}
	else {
		search_next();
	}
}

void EditorHelp::init_gdext_pointers() {}


