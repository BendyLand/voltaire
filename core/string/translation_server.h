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

// --- Inlined/Folded from core/string/translation.h ---
#include "core/io/resource.h"

class PluralRules;

class Translation : public Resource
{
public:
	struct MessageKey
	{
		String msgctxt;
		String msgid;

		// Required to use this struct as a key in HashMap.
		static uint32_t hash(const MessageKey& p_key)
		{
			uint32_t h = hash_murmur3_one_32(HashMapHasherDefault::hash(p_key.msgctxt));
			return hash_fmix32(hash_murmur3_one_32(HashMapHasherDefault::hash(p_key.msgid), h));
		}

		bool operator==(const MessageKey& p_key) const
		{
			return msgctxt == p_key.msgctxt && msgid == p_key.msgid;
		}
	};

private:
	String locale = "en";

	HashMap<MessageKey, Vector<String>, MessageKey> translation_map;

	// Compressed perfect-hash lookup tables (populated by `generate()`).
	// `hash_table[hash(0, text)]` produces a `bucket_table` index or 0xFFFFFFFF if not found.
	Vector<int> hash_table;

	// Continuous `Bucket`s in a flat layout.
	Vector<int> bucket_table;

	// Data for translated strings, UTF-8 encoded, either compressed or uncompressed.
	Vector<uint8_t> strings;

	struct Bucket
	{
		// Number of `Elem` objects at `elem`.
		int size;

		// Use `hash(func, text)` to generate the unique `Elem::key` in this bucket.
		uint32_t func;

		struct Elem
		{
			// Unique key for the text.
			uint32_t key;

			// Used to index into `strings`.
			uint32_t str_offset;

			// The string is not compressed if `comp_size` equals `uncomp_size`.
			uint32_t comp_size;
			uint32_t uncomp_size;
		};

		Elem elem[1];
	};

	_FORCE_INLINE_ uint32_t hash(uint32_t p_d, const char* p_str) const
	{
		if (p_d == 0) {
			p_d = 0x1000193;
		}
		while (*p_str) {
			p_d = (p_d * 0x1000193) ^ static_cast<uint8_t>(*p_str);
			p_str++;
		}

		return p_d;
	}

	mutable PluralRules* plural_rules_cache = nullptr;
	String plural_rules_override;

	Vector<String> _get_message_list() const;
	PluralRules* _get_plural_rules() const;

public:
	void set_locale(const String& p_locale);

	_FORCE_INLINE_ String get_locale() const { return locale; }

	void add_message(const String& p_src_text, const String& p_xlated_text,
		const String& p_context = "");
	void add_plural_message(const String& p_src_text,
		const Vector<String>& p_plural_xlated_texts, const String& p_context = "");
	String get_message(const String& p_src_text, const String& p_context = "") const;
	String get_plural_message(const String& p_src_text,
		const String& p_plural_text, int p_n, const String& p_context = "") const;
	void erase_message(const String& p_src_text, const String& p_context = "");
	void get_message_list(List<String>* r_messages) const;
	int get_message_count() const;
	Vector<String> get_translated_message_list() const;

	bool generate(const Ref<Translation>& p_from);

	void set_plural_rules_override(const String& p_rules);
	String get_plural_rules_override() const;

	// This method is not exposed to scripting intentionally. It is only used by TranslationLoaderPO
	// and tests.
	int get_nplurals() const;

	~Translation();
};
// --- Folded from core/string/translation_domain.h ---
#include "core/types.h"

class Translation;

class TranslationDomain : public RefCounted
{

	struct PseudolocalizationConfig
	{
		bool enabled = false;
		bool accents_enabled = true;
		bool double_vowels_enabled = false;
		bool fake_bidi_enabled = false;
		bool override_enabled = false;
		bool skip_placeholders_enabled = true;
		float expansion_ratio = 0.0;
		String prefix = "[";
		String suffix = "]";
	};

	bool enabled = true;

	String locale_override;
	HashSet<Ref<Translation>> translations;
	PseudolocalizationConfig pseudolocalization;

	String _get_override_string(const String& p_message) const;
	String _double_vowels(const String& p_message) const;
	String _replace_with_accented_string(const String& p_message) const;
	String _wrap_with_fakebidi_characters(const String& p_message) const;
	String _add_padding(const String& p_message, int p_length) const;
	const char32_t* _get_accented_version(char32_t p_character) const;
	bool _is_placeholder(const String& p_message, int p_index) const;

protected:

public:
	// Methods in this section are not intended for scripting.
	String get_message_from_translations(
		const String& p_locale, const String& p_message, const String& p_context) const;
	String get_message_from_translations(const String& p_locale, const String& p_message,
		const String& p_message_plural, int p_n, const String& p_context) const;
	PackedStringArray get_loaded_locales() const;

public:
#ifndef DISABLE_DEPRECATED
	Ref<Translation> get_translation_object(const String& p_locale) const;
#endif

	void add_translation(const Ref<Translation>& p_translation);
	void remove_translation(const Ref<Translation>& p_translation);
	void clear();

	bool has_translation(const Ref<Translation>& p_translation) const;
	const HashSet<Ref<Translation>> get_translations() const;
	HashSet<Ref<Translation>> find_translations(const String& p_locale, bool p_exact) const;
	bool has_translation_for_locale(const String& p_locale, bool p_exact) const;

	String translate(const String& p_message, const String& p_context) const;
	String translate_plural(const String& p_message, const String& p_message_plural,
		int p_n, const String& p_context) const;

	String get_locale_override() const;
	void set_locale_override(const String& p_locale);

	bool is_enabled() const;
	void set_enabled(bool p_enabled);

	bool is_pseudolocalization_enabled() const;
	void set_pseudolocalization_enabled(bool p_enabled);
	bool is_pseudolocalization_accents_enabled() const;
	void set_pseudolocalization_accents_enabled(bool p_enabled);
	bool is_pseudolocalization_double_vowels_enabled() const;
	void set_pseudolocalization_double_vowels_enabled(bool p_enabled);
	bool is_pseudolocalization_fake_bidi_enabled() const;
	void set_pseudolocalization_fake_bidi_enabled(bool p_enabled);
	bool is_pseudolocalization_override_enabled() const;
	void set_pseudolocalization_override_enabled(bool p_enabled);
	bool is_pseudolocalization_skip_placeholders_enabled() const;
	void set_pseudolocalization_skip_placeholders_enabled(bool p_enabled);
	float get_pseudolocalization_expansion_ratio() const;
	void set_pseudolocalization_expansion_ratio(float p_ratio);
	String get_pseudolocalization_prefix() const;
	void set_pseudolocalization_prefix(const String& p_prefix);
	String get_pseudolocalization_suffix() const;
	void set_pseudolocalization_suffix(const String& p_suffix);

	String pseudolocalize(const String& p_message) const;
};



// --- End core/string/translation_domain.h ---

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

// --- Folded from core/string/plural_rules.h ---
#include "core/templates/hash_map.h"
#include "core/types.h"

class Expression;

class PluralRules
{
	mutable LRUCache<int, int> cache;

	// These two fields are initialized in the constructor.
	const int nplurals;
	const String plural;

	// Cache temporary variables related to `evaluate()` to make it faster.
	class EQNode : public RefCounted
	{
	public:
		String regex;
		Ref<EQNode> left;
		Ref<EQNode> right;
	};

	Ref<EQNode> equi_tests;
	Ref<Expression> expr;

	int _find_unquoted(const String& p_src, char32_t p_chr) const;
	void _cache_plural_tests(const String& p_plural_rule, Ref<EQNode>& p_node);

	PluralRules(int p_nplurals, const String& p_plural);

public:
	int evaluate(int p_n) const;

	int get_nplurals() const { return nplurals; }

	String get_plural() const { return plural; }

	static PluralRules* parse(const String& p_rules);
};



// --- End core/string/plural_rules.h ---

// --- Folded from core/string/fuzzy_search.h ---
#include "core/types.h"

class FuzzyTokenMatch;

struct FuzzySearchToken
{
	int idx = -1;
	String string;

	bool try_exact_match(FuzzyTokenMatch& p_match, const String& p_target, int p_offset) const;
	bool try_fuzzy_match(
		FuzzyTokenMatch& p_match, const String& p_target, int p_offset, int p_miss_budget) const;
};

class FuzzyTokenMatch
{
	friend struct FuzzySearchToken;
	friend class FuzzySearchMatch;
	friend class FuzzySearch;

	int matched_length = 0;
	int token_length = 0;
	int token_idx = -1;
	Vector2i interval = Vector2i(-1, -1); // x and y are both inclusive indices.

	void add_substring(int p_substring_start, int p_substring_length);
	bool intersects(const Vector2i& p_other_interval) const;
	bool is_case_insensitive(const String& p_original, const String& p_adjusted) const;

	int get_miss_count() const { return token_length - matched_length; }

public:
	int score = 0;
	Vector<Vector2i> substrings; // x is start index, y is length.
};

class FuzzySearchMatch : public RefCounted
{
	friend class FuzzySearch;

	String target;
	int score = 0;
	int original_index = -1;
	int dir_index = -1;
	Vector<FuzzyTokenMatch> token_matches;
	int miss_budget = 0;
	Vector2i match_interval = Vector2i(-1, -1);

	bool _can_add_token_match(const FuzzyTokenMatch& p_match) const;
	void _score_token_match(FuzzyTokenMatch& p_match, bool p_case_insensitive) const;
	void _add_token_match(const FuzzyTokenMatch& p_match);
	void _maybe_apply_token_order_score_bonus();

protected:

public:
	void set_target(const String& p_target) { target = p_target; }

	String get_target() const { return target; }

	void set_score(int p_score) { score = p_score; }

	int get_score() const { return score; }

	void set_original_index(int p_original_index) { original_index = p_original_index; }

	int get_original_index() const { return original_index; }

	void set_dir_index(int p_dir_index) { dir_index = p_dir_index; }

	int get_dir_index() const { return dir_index; }

	Vector<FuzzyTokenMatch> get_token_matches() { return token_matches; }

	Vector<Vector2i> get_matched_substrings() const;
};

class FuzzySearch : public RefCounted
{
	int start_offset = 0;
	int max_results = 100;
	int max_misses = 2;
	bool exact_token_matches = false;
	bool case_sensitive = false;
	bool filter_low_scores = true;
	float filter_factor = 0.1f;
	float filter_cutoff = 30.0f;

	Vector<FuzzySearchToken> _get_tokens(const String& p_query) const;
	void _sort_and_filter(Vector<Ref<FuzzySearchMatch>>& p_results) const;
	bool _search_tokens(const Vector<FuzzySearchToken>& p_tokens, const String& p_target,
		Ref<FuzzySearchMatch>& r_result) const;

protected:

public:
	void set_start_offset(int p_offset)
	{
		ERR_FAIL_COND(p_offset < 0);
		start_offset = p_offset;
	}

	int get_start_offset() const { return start_offset; }

	void set_max_results(int p_max_results)
	{
		ERR_FAIL_COND(p_max_results <= 0);
		max_results = p_max_results;
	}

	int get_max_results() const { return max_results; }

	void set_max_misses(int p_max_misses)
	{
		ERR_FAIL_COND(p_max_misses < 0);
		max_misses = p_max_misses;
	}

	int get_max_misses() const { return max_misses; }

	void set_use_exact_tokens(bool p_use_exact_tokens) { exact_token_matches = p_use_exact_tokens; }

	bool get_use_exact_tokens() const { return exact_token_matches; }

	void set_case_sensitive(bool p_case_sensitive);

	bool get_case_sensitive() const { return case_sensitive; }

	void set_filter_low_scores(bool p_filter_low_scores)
	{
		filter_low_scores = p_filter_low_scores;
	}

	bool get_filter_low_scores() const { return filter_low_scores; }

	void set_filter_factor(float p_filter_factor)
	{
		ERR_FAIL_COND_MSG(p_filter_factor < 0.0f || p_filter_factor > 1.0f,
			"filter_factor should be in the range [0, 1]");
		filter_factor = p_filter_factor;
	}

	float get_filter_factor() const { return filter_factor; }

	void set_filter_cutoff(float p_filter_cutoff) { filter_cutoff = p_filter_cutoff; }

	float get_filter_cutoff() const { return filter_cutoff; }

	Ref<FuzzySearchMatch> search(const String& p_query, const String& p_target) const;
	Vector<Ref<FuzzySearchMatch>> search_all(
		const String& p_query, const PackedStringArray& p_targets) const;
};



// --- End core/string/fuzzy_search.h ---
