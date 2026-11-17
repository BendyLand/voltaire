/**************************************************************************/
/*  translation_server.h                                                  */
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

#pragma once

#include "core/string/translation.h"
#include "core/string/translation_domain.h"

class TranslationServer final
{
	struct Data
	{
		String locale = "en";
		bool allow_fallback = true;
		String fallback;

		Ref<TranslationDomain> main_domain;
#ifdef TOOLS_ENABLED
		Ref<TranslationDomain> editor_domain;
		Ref<TranslationDomain> property_domain;
		Ref<TranslationDomain> doc_domain;
#endif // TOOLS_ENABLED
		HashMap<String, Ref<TranslationDomain>> custom_domains;

		HashMap<String, int> locale_compare_cache;
	};

	static inline Data* data = nullptr;

#ifndef DISABLE_DEPRECATED
	static String _standardize_locale_bind_compat_98972(const String& p_locale);
	static void _bind_compatibility_methods();
#endif

	struct LocaleScriptInfo
	{
		String name;
		String script;
		String default_country;
		HashSet<String> supported_countries;
	};

	static inline Vector<LocaleScriptInfo> locale_script_info;

	struct Locale
	{
		String language;
		String script;
		String country;
		String variant;

		bool operator==(const Locale& p_locale) const
		{
			return (p_locale.language == language) && (p_locale.script == script) &&
				   (p_locale.country == country) && (p_locale.variant == variant);
		}

		explicit operator String() const;

		Locale(const String& p_locale, bool p_add_defaults);
	};

	static inline HashMap<String, String> language_map;
	static inline HashMap<String, String> script_map;
	static inline HashMap<String, String> locale_rename_map;
	static inline HashMap<String, String> country_name_map;
	static inline HashMap<String, String> country_rename_map;
	static inline HashMap<String, String> variant_map;
	static inline HashMap<String, String> plural_rules_map;
	static inline HashMap<String, int> num_system_map;
	static inline HashMap<String, HashSet<String>> language_script_map;

	static void init_locale_info();

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	// Built-in domain accessors. For engine code only, user code should use `get_or_add_domain()`
	// instead.
	static Ref<TranslationDomain> get_main_domain() { return data ? data->main_domain : Ref<TranslationDomain>(); }
#ifdef TOOLS_ENABLED
	static Ref<TranslationDomain> get_editor_domain() { return data ? data->editor_domain : Ref<TranslationDomain>(); }

	static Ref<TranslationDomain> get_property_domain() { return data ? data->property_domain : Ref<TranslationDomain>(); }

	static Ref<TranslationDomain> get_doc_domain() { return data ? data->doc_domain : Ref<TranslationDomain>(); }
#endif // TOOLS_ENABLED

	static void set_locale(const String& p_locale);
	static String get_locale();
	static void set_fallback_allowed(const bool& p_allow);
	static bool is_fallback_allowed();
	static void set_fallback_locale(const String& p_locale);
	static String get_fallback_locale();

#ifndef DISABLE_DEPRECATED
	static Ref<Translation> get_translation_object(const String& p_locale);
#endif

	static bool has_translation(const Ref<Translation>& p_translation);
	static bool has_translation_for_locale(const String& p_locale, bool p_exact);

	static Vector<String> get_all_languages();
	static String get_language_name(const String& p_language);

	static Vector<String> get_all_scripts();
	static String get_script_name(const String& p_script);

	static Vector<String> get_all_countries();
	static String get_country_name(const String& p_country);

	static String get_locale_name(const String& p_locale);
	static String get_plural_rules(const String& p_locale);

	static bool is_script_suppored_by_locale(const String& p_locale, const String& p_script);

	static PackedStringArray get_loaded_locales();

	static void add_translation(const Ref<Translation>& p_translation);
	static void remove_translation(const Ref<Translation>& p_translation);

	static String translate(const String& p_message, const String& p_context = "");
	static String translate_plural(const String& p_message, const String& p_message_plural,
		int p_n, const String& p_context = "");

	static String pseudolocalize(const String& p_message);

	static bool is_pseudolocalization_enabled();
	static void set_pseudolocalization_enabled(bool p_enabled);
	static void reload_pseudolocalization();

	static String format_number(const String& p_string, const String& p_locale);
	static String parse_number(const String& p_string, const String& p_locale);
	static String get_percent_sign(const String& p_locale);

	static String standardize_locale(const String& p_locale, bool p_add_defaults = false);

	static int compare_locales(const String& p_locale_a, const String& p_locale_b);

	static String get_tool_locale();

	static bool has_domain(const String& p_domain);
	static Ref<TranslationDomain> get_or_add_domain(const String& p_domain);
	static void remove_domain(const String& p_domain);

	static void setup();

	static void clear();

	static void load_project_translations(Ref<TranslationDomain> p_domain);

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif // TOOLS_ENABLED

	TranslationServer() = delete;
	TranslationServer(const TranslationServer&) = delete;
	TranslationServer& operator=(const TranslationServer&) = delete;
	~TranslationServer() = delete;
};
