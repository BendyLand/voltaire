/**************************************************************************/
/*  translation_server.cpp                                                */
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
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/locales.h"
#include "translation_server.compat.inc"
#include "translation_server.h"

void TranslationServer::init_locale_info()
{
	// Init locale info.
	language_map.clear();
	int idx = 0;
	while (language_list[idx][0] != nullptr) {
		language_map[language_list[idx][0]] = String::utf8(language_list[idx][1]);
		idx++;
	}

	// Init locale-script map.
	locale_script_info.clear();
	idx = 0;
	while (locale_scripts[idx][0] != nullptr) {
		LocaleScriptInfo info;
		info.name = locale_scripts[idx][0];
		info.script = locale_scripts[idx][1];
		info.default_country = locale_scripts[idx][2];
		Vector<String> supported_countries = String(locale_scripts[idx][3]).split(",", false);
		for (int i = 0; i < supported_countries.size(); i++) {
			info.supported_countries.insert(supported_countries[i]);
		}
		locale_script_info.push_back(info);
		idx++;
	}

	// Init supported script list.
	script_map.clear();
	idx = 0;
	while (script_list[idx][0] != nullptr) {
		script_map[script_list[idx][1]] = String::utf8(script_list[idx][0]);
		idx++;
	}

	// Init regional variant map.
	variant_map.clear();
	idx = 0;
	while (locale_variants[idx][0] != nullptr) {
		variant_map[locale_variants[idx][0]] = locale_variants[idx][1];
		idx++;
	}

	// Init locale renames.
	locale_rename_map.clear();
	idx = 0;
	while (locale_renames[idx][0] != nullptr) {
		if (!String(locale_renames[idx][1]).is_empty()) {
			locale_rename_map[locale_renames[idx][0]] = locale_renames[idx][1];
		}
		idx++;
	}

	// Init locale scripts.
	language_script_map.clear();
	idx = 0;
	while (language_script_list[idx][0] != nullptr) {
		HashSet<String>& set = language_script_map[language_script_list[idx][0]];
		Vector<String> scripts = String(language_script_list[idx][1]).split(" ");
		for (const String& s : scripts) {
			set.insert(s);
		}
		idx++;
	}

	// Init country names.
	country_name_map.clear();
	idx = 0;
	while (country_names[idx][0] != nullptr) {
		country_name_map[String(country_names[idx][0])] = String::utf8(country_names[idx][1]);
		idx++;
	}

	// Init country renames.
	country_rename_map.clear();
	idx = 0;
	while (country_renames[idx][0] != nullptr) {
		if (!String(country_renames[idx][1]).is_empty()) {
			country_rename_map[country_renames[idx][0]] = country_renames[idx][1];
		}
		idx++;
	}

	// Init plural rules.
	plural_rules_map.clear();
	idx = 0;
	while (plural_rules[idx][0] != nullptr) {
		const Vector<String> rule_locs = String(plural_rules[idx][0]).split(" ");
		const String rule = String(plural_rules[idx][1]);
		for (const String& l : rule_locs) {
			plural_rules_map[l] = rule;
		}
		idx++;
	}

	// Init number systems.
	num_system_map.clear();
	idx = 0;
	while (num_system_data[idx].locales != nullptr) {
		const NumSystemData& nsd = num_system_data[idx];

		// These fields must not be empty.
		DEV_ASSERT(nsd.percent_sign && nsd.percent_sign[0] != '\0');
		DEV_ASSERT(nsd.digits && nsd.digits[0] != '\0');
		DEV_ASSERT(nsd.exp_l && nsd.exp_l[0] != '\0');
		DEV_ASSERT(nsd.exp_u && nsd.exp_u[0] != '\0');
		DEV_ASSERT(strlen(nsd.digits) == 11);

		const Vector<String> locales = String(nsd.locales).split(" ");
		for (const String& l : locales) {
			num_system_map[l] = idx;
		}
		idx++;
	}
}

TranslationServer::Locale::operator String() const
{
	String out = language;
	if (!script.is_empty()) {
		out = out + "_" + script;
	}
	if (!country.is_empty()) {
		out = out + "_" + country;
	}
	if (!variant.is_empty()) {
		out = out + "_" + variant;
	}
	return out;
}

TranslationServer::Locale::Locale(
	const String& p_locale, bool p_add_defaults)
{
	// Replaces '-' with '_' for macOS style locales.
	String univ_locale = p_locale.replace_char('-', '_');

	// Extract locale elements.
	Vector<String> locale_elements = univ_locale.get_slicec('@', 0).split("_");
	language = locale_elements[0];
	if (locale_elements.size() >= 2) {
		if (locale_elements[1].length() == 4 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_lower_case(locale_elements[1][1]) &&
			is_ascii_lower_case(locale_elements[1][2]) &&
			is_ascii_lower_case(locale_elements[1][3])) {
			script = locale_elements[1];
		}
		if (locale_elements[1].length() == 2 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_upper_case(locale_elements[1][1])) {
			country = locale_elements[1];
		}
	}
	if (locale_elements.size() >= 3) {
		if (locale_elements[2].length() == 2 && is_ascii_upper_case(locale_elements[2][0]) &&
			is_ascii_upper_case(locale_elements[2][1])) {
			country = locale_elements[2];
		}
		else if (TranslationServer::variant_map.has(locale_elements[2].to_lower()) &&
				   TranslationServer::variant_map[locale_elements[2].to_lower()] == language) {
			variant = locale_elements[2].to_lower();
		}
	}
	if (locale_elements.size() >= 4) {
		if (TranslationServer::variant_map.has(locale_elements[3].to_lower()) &&
			TranslationServer::variant_map[locale_elements[3].to_lower()] == language) {
			variant = locale_elements[3].to_lower();
		}
	}

	// Try extract script and variant from the extra part.
	Vector<String> script_extra = univ_locale.get_slicec('@', 1).split(";");
	for (int i = 0; i < script_extra.size(); i++) {
		if (script_extra[i].to_lower() == "cyrillic") {
			script = "Cyrl";
			break;
		}
		else if (script_extra[i].to_lower() == "latin") {
			script = "Latn";
			break;
		}
		else if (script_extra[i].to_lower() == "devanagari") {
			script = "Deva";
			break;
		}
		else if (TranslationServer::variant_map.has(script_extra[i].to_lower()) &&
				   TranslationServer::variant_map[script_extra[i].to_lower()] == language) {
			variant = script_extra[i].to_lower();
		}
	}

	// Handles known non-ISO language names used e.g. on Windows.
	if (TranslationServer::locale_rename_map.has(language)) {
		language = TranslationServer::locale_rename_map[language];
	}

	// Handle country renames.
	if (TranslationServer::country_rename_map.has(country)) {
		country = TranslationServer::country_rename_map[country];
	}

	// Remove unsupported script codes.
	if (!TranslationServer::script_map.has(script)) {
		script = "";
	}

	// Add script code base on language and country codes for some ambiguous cases.
	if (p_add_defaults) {
		if (script.is_empty()) {
			for (int i = 0; i < TranslationServer::locale_script_info.size(); i++) {
				const LocaleScriptInfo& info = TranslationServer::locale_script_info[i];
				if (info.name == language) {
					if (country.is_empty() || info.supported_countries.has(country)) {
						script = info.script;
						break;
					}
				}
			}
		}
		if (!script.is_empty() && country.is_empty()) {
			// Add conntry code based on script for some ambiguous cases.
			for (int i = 0; i < TranslationServer::locale_script_info.size(); i++) {
				const LocaleScriptInfo& info = TranslationServer::locale_script_info[i];
				if (info.name == language && info.script == script) {
					country = info.default_country;
					break;
				}
			}
		}
	}
}

String TranslationServer::format_number(const String& p_string, const String& p_locale)
{
	ERR_FAIL_COND_V(p_locale.is_empty(), p_string);
	if (!num_system_map.has(p_locale)) {
		return p_string;
	}

	int index = num_system_map[p_locale];
	const NumSystemData& nsd = num_system_data[index];

	String res = p_string;
	res = res.replace("e", nsd.exp_l);
	res = res.replace("E", nsd.exp_u);
	char32_t* res_data = res.ptrw();
	for (int j = 0; j < res.length(); j++) {
		if (res_data[j] >= 0x30 && res_data[j] <= 0x39) {
			res_data[j] = nsd.digits[res_data[j] - 0x30];
		}
		else if (res_data[j] == '.' || res_data[j] == ',') {
			res_data[j] = nsd.digits[10];
		}
	}
	return res;
}

String TranslationServer::parse_number(const String& p_string, const String& p_locale)
{
	ERR_FAIL_COND_V(p_locale.is_empty(), p_string);
	if (!num_system_map.has(p_locale)) {
		return p_string;
	}

	int index = num_system_map[p_locale];
	const NumSystemData& nsd = num_system_data[index];

	String res = p_string;
	res = res.replace(nsd.exp_l, "e");
	res = res.replace(nsd.exp_u, "E");
	char32_t* res_data = res.ptrw();
	for (int j = 0; j < res.length(); j++) {
		if (res_data[j] == nsd.digits[10]) {
			res_data[j] = '.';
		}
		else {
			for (int k = 0; k < 10; k++) {
				if (res_data[j] == nsd.digits[k]) {
					res_data[j] = 0x30 + k;
				}
			}
		}
	}
	return res;
}

String TranslationServer::get_percent_sign(const String& p_locale)
{
	ERR_FAIL_COND_V(p_locale.is_empty(), "%");
	if (!num_system_map.has(p_locale)) {
		return "%";
	}

	int index = num_system_map[p_locale];
	return num_system_data[index].percent_sign;
}

String TranslationServer::standardize_locale(const String& p_locale, bool p_add_defaults)
{
	return Locale(p_locale, p_add_defaults).operator String();
}

int TranslationServer::compare_locales(const String& p_locale_a, const String& p_locale_b)
{
	if (p_locale_a == p_locale_b) {
		// Exact match.
		return 10;
	}

	const String cache_key = p_locale_a + "|" + p_locale_b;
	const int* cached_result = data->locale_compare_cache.getptr(cache_key);
	if (cached_result) {
		return *cached_result;
	}

	Locale locale_a = Locale(p_locale_a, true);
	Locale locale_b = Locale(p_locale_b, true);

	if (locale_a == locale_b) {
		// Exact match.
		data->locale_compare_cache.insert(cache_key, 10);
		return 10;
	}

	if (locale_a.language != locale_b.language) {
		// No match.
		data->locale_compare_cache.insert(cache_key, 0);
		return 0;
	}

	// Matching language, both locales have extra parts. Compare the
	// remaining elements. If both elements are non-empty, check the
	// match to increase or decrease the score. If either element or
	// both are empty, leave the score as is.
	int score = 5;
	if (!locale_a.script.is_empty() && !locale_b.script.is_empty()) {
		if (locale_a.script == locale_b.script) {
			score++;
		}
		else {
			score--;
		}
	}
	if (!locale_a.country.is_empty() && !locale_b.country.is_empty()) {
		if (locale_a.country == locale_b.country) {
			score++;
		}
		else {
			score--;
		}
	}
	if (!locale_a.variant.is_empty() && !locale_b.variant.is_empty()) {
		if (locale_a.variant == locale_b.variant) {
			score++;
		}
		else {
			score--;
		}
	}

	data->locale_compare_cache.insert(cache_key, score);
	return score;
}

String TranslationServer::get_locale_name(const String& p_locale)
{
	String lang_name, script_name, country_name;
	Vector<String> locale_elements = standardize_locale(p_locale).split("_");
	lang_name = locale_elements[0];
	if (locale_elements.size() >= 2) {
		if (locale_elements[1].length() == 4 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_lower_case(locale_elements[1][1]) &&
			is_ascii_lower_case(locale_elements[1][2]) &&
			is_ascii_lower_case(locale_elements[1][3])) {
			script_name = locale_elements[1];
		}
		if (locale_elements[1].length() == 2 && is_ascii_upper_case(locale_elements[1][0]) &&
			is_ascii_upper_case(locale_elements[1][1])) {
			country_name = locale_elements[1];
		}
	}
	if (locale_elements.size() >= 3) {
		if (locale_elements[2].length() == 2 && is_ascii_upper_case(locale_elements[2][0]) &&
			is_ascii_upper_case(locale_elements[2][1])) {
			country_name = locale_elements[2];
		}
	}

	String name = get_language_name(lang_name);
	if (!script_name.is_empty()) {
		name = name + " (" + get_script_name(script_name) + ")";
	}
	if (!country_name.is_empty()) {
		name = name + ", " + get_country_name(country_name);
	}
	return name;
}

String TranslationServer::get_plural_rules(const String& p_locale)
{
	const String* rule = plural_rules_map.getptr(p_locale);
	if (rule) {
		return *rule;
	}

	Locale l = Locale(p_locale, false);
	if (!l.country.is_empty()) {
		rule = plural_rules_map.getptr(l.language + "_" + l.country);
		if (rule) {
			return *rule;
		}
	}
	rule = plural_rules_map.getptr(l.language);
	if (rule) {
		return *rule;
	}
	return String();
}

Vector<String> TranslationServer::get_all_languages()
{
	Vector<String> languages;

	for (const KeyValue<String, String>& E : language_map) {
		languages.push_back(E.key);
	}

	return languages;
}

String TranslationServer::get_language_name(const String& p_language)
{
	if (language_map.has(p_language)) {
		return language_map[p_language];
	}
	else {
		return p_language;
	}
}

Vector<String> TranslationServer::get_all_scripts()
{
	Vector<String> scripts;

	for (const KeyValue<String, String>& E : script_map) {
		scripts.push_back(E.key);
	}

	return scripts;
}

String TranslationServer::get_script_name(const String& p_script)
{
	if (script_map.has(p_script)) {
		return script_map[p_script];
	}
	else {
		return p_script;
	}
}

Vector<String> TranslationServer::get_all_countries()
{
	Vector<String> countries;

	for (const KeyValue<String, String>& E : country_name_map) {
		countries.push_back(E.key);
	}

	return countries;
}

String TranslationServer::get_country_name(const String& p_country)
{
	if (country_name_map.has(p_country)) {
		return country_name_map[p_country];
	}
	else {
		return p_country;
	}
}

void TranslationServer::set_locale(const String& p_locale)
{
	ERR_FAIL_COND_MSG(p_locale.is_empty(), "Locale cannot be an empty string.");

	String new_locale = standardize_locale(p_locale);
	if (data->locale == new_locale) {
		return;
	}

	data->locale = new_locale;
	ResourceLoader::reload_translation_remaps();
}

String TranslationServer::get_locale() { return data->locale; }

void TranslationServer::set_fallback_allowed(const bool& p_allow) { data->allow_fallback = p_allow; }

bool TranslationServer::is_fallback_allowed() { return data->allow_fallback; }

void TranslationServer::set_fallback_locale(const String& p_locale) { data->fallback = p_locale; }

String TranslationServer::get_fallback_locale() { return data->fallback; }

bool TranslationServer::is_script_suppored_by_locale(
	const String& p_locale, const String& p_script)
{
	Locale l = Locale(p_locale, true);
	if (l.script == p_script) {
		return true;
	}
	if (!language_script_map.has(l.language)) {
		return false;
	}
	return language_script_map[l.language].has(p_script);
}

PackedStringArray TranslationServer::get_loaded_locales()
{
	return data->main_domain->get_loaded_locales();
}

void TranslationServer::add_translation(const Ref<Translation>& p_translation)
{
	data->main_domain->add_translation(p_translation);
}

void TranslationServer::remove_translation(const Ref<Translation>& p_translation)
{
	data->main_domain->remove_translation(p_translation);
}

#ifndef DISABLE_DEPRECATED
Ref<Translation> TranslationServer::get_translation_object(const String& p_locale)
{
	return data->main_domain->get_translation_object(p_locale);
}
#endif

bool TranslationServer::has_translation(const Ref<Translation>& p_translation)
{
	return data->main_domain->has_translation(p_translation);
}

bool TranslationServer::has_translation_for_locale(const String& p_locale, bool p_exact)
{
	return data->main_domain->has_translation_for_locale(p_locale, p_exact);
}

void TranslationServer::clear() { data->main_domain->clear(); }

String TranslationServer::translate(
	const String& p_message, const String& p_context)
{
	return data->main_domain->translate(p_message, p_context);
}

String TranslationServer::translate_plural(const String& p_message,
	const String& p_message_plural, int p_n, const String& p_context)
{
	return data->main_domain->translate_plural(p_message, p_message_plural, p_n, p_context);
}

bool TranslationServer::has_domain(const String& p_domain)
{
	if (p_domain == String()) {
		return true;
	}
	return data->custom_domains.has(p_domain);
}

Ref<TranslationDomain> TranslationServer::get_or_add_domain(const String& p_domain)
{
	if (p_domain == String()) {
		return data->main_domain;
	}
	const Ref<TranslationDomain>* domain = data->custom_domains.getptr(p_domain);
	if (domain) {
		if (domain->is_valid()) {
			return *domain;
		}
		ERR_PRINT("Bug (please report): Found invalid translation domain.");
	}
	Ref<TranslationDomain> new_domain = memnew(TranslationDomain);
	data->custom_domains[p_domain] = new_domain;
	return new_domain;
}

void TranslationServer::remove_domain(const String& p_domain)
{
	ERR_FAIL_COND_MSG(p_domain == String(), "Cannot remove main translation domain.");
	data->custom_domains.erase(p_domain);
}

String TranslationServer::get_tool_locale()
{
#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint() ||
		Engine::is_project_manager_hint()) {
		if (data->editor_domain->has_translation_for_locale(data->locale, true)) {
			return data->locale;
		}
		return "en";
	}
#endif

	Ref<Translation> res;
	int best_score = 0;

	for (const Ref<Translation>& E : data->main_domain->get_translations()) {
		int score = TranslationServer::compare_locales(data->locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			res = E;
			best_score = score;
			if (score == 10) {
				return data->locale; // Exact match.
			}
		}
	}
	return res.is_valid() ? res->get_locale() : data->fallback;
}

bool TranslationServer::is_pseudolocalization_enabled()
{
	return data->main_domain->is_pseudolocalization_enabled();
}

void TranslationServer::set_pseudolocalization_enabled(bool p_enabled)
{
	data->main_domain->set_pseudolocalization_enabled(p_enabled);

	ResourceLoader::reload_translation_remaps();
}

String TranslationServer::pseudolocalize(const String& p_message)
{
	return data->main_domain->pseudolocalize(p_message);
}


void TranslationServer::load_project_translations(Ref<TranslationDomain> p_domain)
{
	DEV_ASSERT(p_domain.is_valid());

	p_domain->clear();
	const String prop = "internationalization/locale/translations";
	if (!ProjectSettings::has_setting(prop)) {
		return;
	}
}
void TranslationServer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) {}

void TranslationServer::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);

	data->main_domain.instantiate();

#ifdef TOOLS_ENABLED
	data->editor_domain = get_or_add_domain("godot.editor");
	data->property_domain = get_or_add_domain("godot.properties");
	data->doc_domain = get_or_add_domain("godot.documentation");
#endif // TOOLS_ENABLED

	init_locale_info();
}

void TranslationServer::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void TranslationServer::setup() {}

