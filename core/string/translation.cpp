/**************************************************************************/
/*  translation.cpp                                                       */
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

#include "core/os/thread.h"
#include "core/string/plural_rules.h"
#include "core/string/translation_server.h"
#include "core/templates/pair.h"
#include "core/types.h"
#include "translation.h"

extern "C"
{
#include <thirdparty/misc/smaz.h>
}

struct CompressedString
{
	int orig_len = 0;
	CharString compressed;
	int offset = 0;
};

void _check_for_incompatibility(const String& p_msgctxt, const String& p_msgid)
{
	// Gettext PO and MO files use an empty untranslated string without context
	// to store metadata.
	if (p_msgctxt.is_empty() && p_msgid.is_empty()) {
		WARN_PRINT("Both context and the untranslated string are empty. This may cause issues with "
				   "the translation system and external tools.");
	}

	// The EOT character (0x04) is used as a separator between context and
	// untranslated string in the MO file format. This convention is also used
	// by `get_message_list()`.
	//
	// It's unusual to have this character in the context or untranslated
	// string. But it doesn't do any harm as long as you are aware of this when
	// using the relevant APIs and tools.
	if (p_msgctxt.contains_char(0x04)) {
		WARN_PRINT(vformat("Found EOT character (0x04) within context '%s'. This may cause issues "
						   "with the translation system and external tools.",
			p_msgctxt));
	}
	if (p_msgid.contains_char(0x04)) {
		WARN_PRINT(vformat("Found EOT character (0x04) within untranslated string '%s'. This may "
						   "cause issues with the translation system and external tools.",
			p_msgid));
	}
}

bool Translation::generate(const Ref<Translation>& p_from)
{
	// This method compresses a Translation instance.
	// Right now, it doesn't handle context or plurals.
#ifdef TOOLS_ENABLED
	ERR_FAIL_COND_V(p_from.is_null(), false);

	List<String> keys;
	{
		List<String> raw_keys;
		p_from->get_message_list(&raw_keys);

		for (const String& key : raw_keys) {
			const String key_str = key;
			int p = key_str.find_char(0x04);
			if (p == -1) {
				keys.push_back(key);
			}
			else {
				const String& msgctxt = key_str.substr(0, p);
				const String& msgid = key_str.substr(p + 1);
				WARN_PRINT(vformat("Optimized translation does not support context, ignoring "
								   "message '%s' with context '%s'.",
					msgid, msgctxt));
			}
		}
	}

	int size = Math::larger_prime(keys.size());

	Vector<Vector<Pair<int, CharString>>> buckets;
	Vector<HashMap<uint32_t, int>> table;
	Vector<uint32_t> hfunc_table;
	Vector<CompressedString> compressed;

	table.resize(size);
	hfunc_table.resize(size);
	buckets.resize(size);
	compressed.resize(keys.size());

	int idx = 0;
	int total_compression_size = 0;

	for (const String& E : keys) {
		// hash string
		CharString cs = E.utf8();
		uint32_t h = hash(0, cs.get_data());
		Pair<int, CharString> p;
		p.first = idx;
		p.second = cs;
		buckets.write[h % size].push_back(p);

		// compress string
		CharString src_s = p_from->get_message(E).utf8();
		CompressedString ps;
		ps.orig_len = src_s.size();
		ps.offset = total_compression_size;

		if (ps.orig_len != 0) {
			CharString dst_s;
			dst_s.resize_uninitialized(src_s.size());
			int ret = smaz_compress(src_s.get_data(), src_s.size(), dst_s.ptrw(), src_s.size());
			if (ret >= src_s.size()) {
				// if compressed is larger than original, just use original
				ps.orig_len = src_s.size();
				ps.compressed = src_s;
			}
			else {
				dst_s.resize_uninitialized(ret);
				ps.compressed = dst_s;
			}
		}
		else {
			ps.orig_len = 1;
			ps.compressed.resize_uninitialized(1);
			ps.compressed[0] = 0;
		}

		compressed.write[idx] = ps;
		total_compression_size += ps.compressed.size();
		idx++;
	}

	int bucket_table_size = 0;

	for (int i = 0; i < size; i++) {
		const Vector<Pair<int, CharString>>& b = buckets[i];
		HashMap<uint32_t, int>& t = table.write[i];

		if (b.is_empty()) {
			continue;
		}

		int d = 1;
		int item = 0;

		while (item < b.size()) {
			uint32_t slot = hash(d, b[item].second.get_data());
			if (t.has(slot)) {
				item = 0;
				d++;
				t.clear();
			}
			else {
				t[slot] = b[item].first;
				item++;
			}
		}

		hfunc_table.write[i] = d;
		bucket_table_size += 2 + b.size() * 4;
	}

	ERR_FAIL_COND_V(bucket_table_size == 0, false);

	String from_locale = p_from->get_locale();

	hash_table.resize(size);
	bucket_table.resize(bucket_table_size);

	int* htwb = hash_table.ptrw();
	int* btwb = bucket_table.ptrw();

	uint32_t* htw = (uint32_t*)&htwb[0];
	uint32_t* btw = (uint32_t*)&btwb[0];

	int btindex = 0;

	for (int i = 0; i < size; i++) {
		const HashMap<uint32_t, int>& t = table[i];
		if (t.is_empty()) {
			htw[i] = 0xFFFFFFFF; // nothing
			continue;
		}

		htw[i] = btindex;
		btw[btindex++] = t.size();
		btw[btindex++] = hfunc_table[i];

		for (const KeyValue<uint32_t, int>& E : t) {
			btw[btindex++] = E.key;
			btw[btindex++] = compressed[E.value].offset;
			btw[btindex++] = compressed[E.value].compressed.size();
			btw[btindex++] = compressed[E.value].orig_len;
		}
	}

	strings.resize(total_compression_size);
	uint8_t* cw = strings.ptrw();

	for (int i = 0; i < compressed.size(); i++) {
		memcpy(&cw[compressed[i].offset], compressed[i].compressed.get_data(),
			compressed[i].compressed.size());
	}

	ERR_FAIL_COND_V(btindex != bucket_table_size, false);
	translation_map.clear();
	set_locale(from_locale);

	return true;
#else
	return false;
#endif
}

Vector<String> Translation::_get_message_list() const
{
	if (bucket_table.size() > 0) {
		WARN_PRINT_ONCE("Optimized translation does not store the message texts to be translated.");
		return {};
	}

	List<String> msgstrs;
	get_message_list(&msgstrs);

	Vector<String> keys;
	keys.resize(msgstrs.size());
	int idx = 0;
	for (const String& msgstr : msgstrs) {
		keys.write[idx++] = msgstr;
	}
	return keys;
}

Vector<String> Translation::get_translated_message_list() const
{
	if (bucket_table.size() > 0) {
		Vector<String> msgs;

		const int* htr = hash_table.ptr();
		const uint32_t* htptr = (const uint32_t*)&htr[0];
		const int* btr = bucket_table.ptr();
		const uint32_t* btptr = (const uint32_t*)&btr[0];
		const uint8_t* sr = strings.ptr();
		const char* sptr = (const char*)&sr[0];

		for (int i = 0; i < hash_table.size(); i++) {
			uint32_t p = htptr[i];
			if (p != 0xFFFFFFFF) {
				const Bucket& bucket = *(const Bucket*)&btptr[p];
				for (int j = 0; j < bucket.size; j++) {
					if (bucket.elem[j].comp_size == bucket.elem[j].uncomp_size) {
						String rstr = String::utf8(
							&sptr[bucket.elem[j].str_offset], bucket.elem[j].uncomp_size);
						msgs.push_back(rstr);
					}
					else {
						CharString uncomp;
						uncomp.resize_uninitialized(bucket.elem[j].uncomp_size + 1);
						smaz_decompress(&sptr[bucket.elem[j].str_offset], bucket.elem[j].comp_size,
							uncomp.ptrw(), bucket.elem[j].uncomp_size);
						String rstr = String::utf8(uncomp.get_data());
						msgs.push_back(rstr);
					}
				}
			}
		}
		return msgs;
	}

	Vector<String> msgstrs;
	for (const KeyValue<MessageKey, Vector<String>>& E : translation_map) {
		for (const String& msgstr : E.value) {
			msgstrs.push_back(msgstr);
		}
	}
	return msgstrs;
}

void Translation::set_locale(const String& p_locale)
{
	locale = TranslationServer::standardize_locale(p_locale);

	if (plural_rules_cache && plural_rules_override.is_empty()) {
		memdelete(plural_rules_cache);
		plural_rules_cache = nullptr;
	}
}

void Translation::add_message(
	const String& p_src_text, const String& p_xlated_text, const String& p_context)
{
	_check_for_incompatibility(p_context, p_src_text);
	translation_map[{p_context, p_src_text}] = {p_xlated_text};
}

void Translation::add_plural_message(const String& p_src_text,
	const Vector<String>& p_plural_xlated_texts, const String& p_context)
{
	ERR_FAIL_COND_MSG(p_plural_xlated_texts.is_empty(),
		"Parameter vector p_plural_xlated_texts passed in is empty.");

	Vector<String> msgstrs;
	msgstrs.resize(p_plural_xlated_texts.size());
	for (int i = 0; i < p_plural_xlated_texts.size(); i++) {
		msgstrs.write[i] = p_plural_xlated_texts[i];
	}

	_check_for_incompatibility(p_context, p_src_text);
	translation_map[{p_context, p_src_text}] = msgstrs;
}

String Translation::get_message(const String& p_src_text, const String& p_context) const
{
	if (bucket_table.size() > 0) {
		// p_context passed in is ignored in compressed mode.
		int htsize = hash_table.size();

		if (htsize == 0) {
			return String();
		}

		CharString str = p_src_text.utf8();
		uint32_t h = hash(0, str.get_data());

		const int* htr = hash_table.ptr();
		const uint32_t* htptr = (const uint32_t*)&htr[0];
		const int* btr = bucket_table.ptr();
		const uint32_t* btptr = (const uint32_t*)&btr[0];
		const uint8_t* sr = strings.ptr();
		const char* sptr = (const char*)&sr[0];

		uint32_t p = htptr[h % htsize];

		if (p == 0xFFFFFFFF) {
			return String(); // nothing
		}

		const Bucket& bucket = *(const Bucket*)&btptr[p];

		h = hash(bucket.func, str.get_data());

		int idx = -1;

		for (int i = 0; i < bucket.size; i++) {
			if (bucket.elem[i].key == h) {
				idx = i;
				break;
			}
		}

		if (idx == -1) {
			return String();
		}

		if (bucket.elem[idx].comp_size == bucket.elem[idx].uncomp_size) {
			return String::utf8(&sptr[bucket.elem[idx].str_offset], bucket.elem[idx].uncomp_size);
		}
		else {
			CharString uncomp;
			uncomp.resize_uninitialized(bucket.elem[idx].uncomp_size + 1);
			smaz_decompress(&sptr[bucket.elem[idx].str_offset], bucket.elem[idx].comp_size,
				uncomp.ptrw(), bucket.elem[idx].uncomp_size);
			return String::utf8(uncomp.get_data());
		}
	}

	const Vector<String>* msgstrs = translation_map.getptr({p_context, p_src_text});
	if (msgstrs == nullptr) {
		return String();
	}

	DEV_ASSERT(!msgstrs->is_empty()); // Should be prevented when adding messages.
	return msgstrs->get(0);
}

String Translation::get_plural_message(const String& p_src_text,
	const String& p_plural_text, int p_n, const String& p_context) const
{
	if (bucket_table.size() > 0) {
		// The use of plurals translation is not yet supported in compressed mode.
		return get_message(p_src_text, p_context);
	}

	ERR_FAIL_COND_V_MSG(p_n < 0, String(),
		"N passed into translation to get a plural message should not be negative. For negative "
		"numbers, use singular translation please. Search \"gettext PO Plural Forms\" online for "
		"details on translating negative numbers.");

	const Vector<String>* msgstrs = translation_map.getptr({p_context, p_src_text});
	if (msgstrs == nullptr) {
		return String();
	}

	const int index = _get_plural_rules()->evaluate(p_n);
	ERR_FAIL_INDEX_V_MSG(index, msgstrs->size(), String(),
		"Plural index returned or number of plural translations is not valid.");
	return msgstrs->get(index);
}

void Translation::erase_message(const String& p_src_text, const String& p_context)
{
	translation_map.erase({p_context, p_src_text});
}

void Translation::get_message_list(List<String>* r_messages) const
{
	if (bucket_table.size() > 0) {
		WARN_PRINT_ONCE("Optimized translation does not store the message texts to be translated.");
		return;
	}

	for (const KeyValue<MessageKey, Vector<String>>& E : translation_map) {
		if (E.key.msgctxt.is_empty()) {
			r_messages->push_back(E.key.msgid);
		}
		else {
			// Separated by the EOT character. Compatible with the MO file format.
			r_messages->push_back(vformat("%s\x04%s", E.key.msgctxt, E.key.msgid));
		}
	}
}

int Translation::get_message_count() const
{
	if (bucket_table.size() > 0) {
		WARN_PRINT_ONCE("Optimized translation does not store the message texts to be translated.");
		return 0;
	}

	return translation_map.size();
}

PluralRules* Translation::_get_plural_rules() const
{
	if (plural_rules_cache) {
		return plural_rules_cache;
	}

	if (!plural_rules_override.is_empty()) {
		plural_rules_cache = PluralRules::parse(plural_rules_override);
	}

	if (!plural_rules_cache) {
		// Locale's default plural rules.
		const String& default_rule = TranslationServer::get_plural_rules(locale);
		if (!default_rule.is_empty()) {
			plural_rules_cache = PluralRules::parse(default_rule);
		}

		// Use English plural rules as a fallback.
		if (!plural_rules_cache) {
			plural_rules_cache = PluralRules::parse("nplurals=2; plural=(n != 1);");
		}
	}

	DEV_ASSERT(plural_rules_cache != nullptr);
	return plural_rules_cache;
}

void Translation::set_plural_rules_override(const String& p_rules)
{
	plural_rules_override = p_rules;
	if (plural_rules_cache) {
		memdelete(plural_rules_cache);
		plural_rules_cache = nullptr;
	}
}

String Translation::get_plural_rules_override() const { return plural_rules_override; }

int Translation::get_nplurals() const { return _get_plural_rules()->get_nplurals(); }

Translation::~Translation()
{
	if (plural_rules_cache) {
		memdelete(plural_rules_cache);
		plural_rules_cache = nullptr;
	}
}

// ==========================================================================
// plural_rules.cpp
// ==========================================================================

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

#include "core/math/expression.h"
#include "plural_rules.h"

int PluralRules::_find_unquoted(const String& p_src, char32_t p_chr) const
{
	const int len = p_src.length();
	if (len == 0) {
		return -1;
	}

	const char32_t* src = p_src.get_data();
	bool in_quote = false;
	for (int i = 0; i < len; i++) {
		if (in_quote) {
			if (src[i] == ')') {
				in_quote = false;
			}
		}
		else {
			if (src[i] == '(') {
				in_quote = true;
			}
			else if (src[i] == p_chr) {
				return i;
			}
		}
	}

	return -1;
}

void PluralRules::_cache_plural_tests(const String& p_plural_rule, Ref<EQNode>& p_node)
{
	// Some examples of p_plural_rule passed in can have the form:
	// "n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5"
	// (Arabic) "n >= 2" (French) // When evaluating the last, especially careful with this one. "n
	// != 1" (English)

	String rule = p_plural_rule;
	if (rule.begins_with("(") && rule.ends_with(")")) {
		int bcount = 0;
		for (int i = 1; i < rule.length() - 1 && bcount >= 0; i++) {
			if (rule[i] == '(') {
				bcount++;
			}
			else if (rule[i] == ')') {
				bcount--;
			}
		}
		if (bcount == 0) {
			rule = rule.substr(1, rule.length() - 2);
		}
	}

	int first_ques_mark = _find_unquoted(rule, '?');
	int first_colon = _find_unquoted(rule, ':');

	if (first_ques_mark == -1) {
		p_node->regex = rule.strip_edges();
		return;
	}

	p_node->regex = rule.substr(0, first_ques_mark).strip_edges();

	p_node->left.instantiate();
	_cache_plural_tests(
		rule.substr(first_ques_mark + 1, first_colon - first_ques_mark - 1).strip_edges(),
		p_node->left);
	p_node->right.instantiate();
	_cache_plural_tests(rule.substr(first_colon + 1).strip_edges(), p_node->right);
}

PluralRules::PluralRules(int p_nplurals, const String& p_plural)
	: nplurals(p_nplurals), plural(p_plural)
{
	equi_tests.instantiate();
	_cache_plural_tests(plural, equi_tests);

	expr.instantiate();
}

int PluralRules::evaluate(int p_n) const { return 0; }

PluralRules* PluralRules::parse(const String& p_rules)
{
	// `p_rules` should be in the format "nplurals=<N>; plural=<Expression>;".

	const int nplurals_eq = p_rules.find_char('=');
	ERR_FAIL_COND_V_MSG(nplurals_eq == -1, nullptr,
		"Invalid plural rules format. Missing equal sign for `nplurals`.");

	const int nplurals_semi_col = p_rules.find_char(';', nplurals_eq);
	ERR_FAIL_COND_V_MSG(nplurals_semi_col == -1, nullptr,
		"Invalid plural rules format. Missing semicolon for `nplurals`.");

	const String nplurals_str =
		p_rules.substr(nplurals_eq + 1, nplurals_semi_col - (nplurals_eq + 1)).strip_edges();
	ERR_FAIL_COND_V_MSG(!nplurals_str.is_valid_int(), nullptr,
		"Invalid plural rules format. `nplurals` should be an integer.");

	const int nplurals = nplurals_str.to_int();
	ERR_FAIL_COND_V_MSG(
		nplurals < 1, nullptr, "Invalid plural rules format. `nplurals` should be at least 1.");

	const int expression_eq = p_rules.find_char('=', nplurals_semi_col + 1);
	ERR_FAIL_COND_V_MSG(expression_eq == -1, nullptr,
		"Invalid plural rules format. Missing equal sign for `plural`.");

	int expression_end = p_rules.rfind_char(';');
	if (expression_end == -1) {
		WARN_PRINT("Invalid plural rules format. Missing semicolon at the end of `plural` "
				   "expression. Assuming ends at the end of the string.");
		expression_end = p_rules.length();
	}

	const int expression_start = expression_eq + 1;
	ERR_FAIL_COND_V_MSG(expression_end <= expression_start, nullptr,
		"Invalid plural rules format. `plural` expression is empty.");

	const String& plural =
		p_rules.substr(expression_start, expression_end - expression_start).strip_edges();
	return memnew(PluralRules(nplurals, plural));
}

// ==========================================================================
// translation_domain.cpp
// ==========================================================================

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

#include "core/string/translation.h"
#include "core/string/translation_server.h"
#include "translation_domain.h"

struct _character_accent_pair
{
	const char32_t character;
	const char32_t* accented_character;
};

static _character_accent_pair _character_to_accented[] = {
	{'A', U"Å"},
	{'B', U"ß"},
	{'C', U"Ç"},
	{'D', U"Ð"},
	{'E', U"É"},
	{'F', U"F́"},
	{'G', U"Ĝ"},
	{'H', U"Ĥ"},
	{'I', U"Ĩ"},
	{'J', U"Ĵ"},
	{'K', U"ĸ"},
	{'L', U"Ł"},
	{'M', U"Ḿ"},
	{'N', U"й"},
	{'O', U"Ö"},
	{'P', U"Ṕ"},
	{'Q', U"Q́"},
	{'R', U"Ř"},
	{'S', U"Ŝ"},
	{'T', U"Ŧ"},
	{'U', U"Ũ"},
	{'V', U"Ṽ"},
	{'W', U"Ŵ"},
	{'X', U"X́"},
	{'Y', U"Ÿ"},
	{'Z', U"Ž"},
	{'a', U"á"},
	{'b', U"ḅ"},
	{'c', U"ć"},
	{'d', U"d́"},
	{'e', U"é"},
	{'f', U"f́"},
	{'g', U"ǵ"},
	{'h', U"h̀"},
	{'i', U"í"},
	{'j', U"ǰ"},
	{'k', U"ḱ"},
	{'l', U"ł"},
	{'m', U"m̀"},
	{'n', U"ή"},
	{'o', U"ô"},
	{'p', U"ṕ"},
	{'q', U"q́"},
	{'r', U"ŕ"},
	{'s', U"š"},
	{'t', U"ŧ"},
	{'u', U"ü"},
	{'v', U"ṽ"},
	{'w', U"ŵ"},
	{'x', U"x́"},
	{'y', U"ý"},
	{'z', U"ź"},
};

String TranslationDomain::_get_override_string(const String& p_message) const
{
	String res;
	for (int i = 0; i < p_message.length(); i++) {
		if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += p_message[i];
			res += p_message[i + 1];
			i++;
			continue;
		}
		res += '*';
	}
	return res;
}

String TranslationDomain::_double_vowels(const String& p_message) const
{
	String res;
	for (int i = 0; i < p_message.length(); i++) {
		if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += p_message[i];
			res += p_message[i + 1];
			i++;
			continue;
		}
		res += p_message[i];
		if (p_message[i] == 'a' || p_message[i] == 'e' || p_message[i] == 'i' ||
			p_message[i] == 'o' || p_message[i] == 'u' || p_message[i] == 'A' ||
			p_message[i] == 'E' || p_message[i] == 'I' || p_message[i] == 'O' ||
			p_message[i] == 'U') {
			res += p_message[i];
		}
	}
	return res;
}

String TranslationDomain::_replace_with_accented_string(const String& p_message) const
{
	String res;
	for (int i = 0; i < p_message.length(); i++) {
		if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += p_message[i];
			res += p_message[i + 1];
			i++;
			continue;
		}
		const char32_t* accented = _get_accented_version(p_message[i]);
		if (accented) {
			res += accented;
		}
		else {
			res += p_message[i];
		}
	}
	return res;
}

String TranslationDomain::_wrap_with_fakebidi_characters(const String& p_message) const
{
	String res;
	char32_t fakebidiprefix = U'\u202e';
	char32_t fakebidisuffix = U'\u202c';
	res += fakebidiprefix;
	// The fake bidi unicode gets popped at every newline so pushing it back at every newline.
	for (int i = 0; i < p_message.length(); i++) {
		if (p_message[i] == '\n') {
			res += fakebidisuffix;
			res += p_message[i];
			res += fakebidiprefix;
		}
		else if (pseudolocalization.skip_placeholders_enabled && _is_placeholder(p_message, i)) {
			res += fakebidisuffix;
			res += p_message[i];
			res += p_message[i + 1];
			res += fakebidiprefix;
			i++;
		}
		else {
			res += p_message[i];
		}
	}
	res += fakebidisuffix;
	return res;
}

String TranslationDomain::_add_padding(const String& p_message, int p_length) const
{
	String underscores = String("_").repeat(p_length * pseudolocalization.expansion_ratio / 2);
	String prefix = pseudolocalization.prefix + underscores;
	String suffix = underscores + pseudolocalization.suffix;

	return prefix + p_message + suffix;
}

const char32_t* TranslationDomain::_get_accented_version(char32_t p_character) const
{
	if (!is_ascii_alphabet_char(p_character)) {
		return nullptr;
	}

	for (unsigned int i = 0; i < std_size(_character_to_accented); i++) {
		if (_character_to_accented[i].character == p_character) {
			return _character_to_accented[i].accented_character;
		}
	}

	return nullptr;
}

bool TranslationDomain::_is_placeholder(const String& p_message, int p_index) const
{
	return p_index < p_message.length() - 1 && p_message[p_index] == '%' &&
		   (p_message[p_index + 1] == 's' || p_message[p_index + 1] == 'c' ||
			   p_message[p_index + 1] == 'd' || p_message[p_index + 1] == 'o' ||
			   p_message[p_index + 1] == 'x' || p_message[p_index + 1] == 'X' ||
			   p_message[p_index + 1] == 'f');
}

String TranslationDomain::get_message_from_translations(
	const String& p_locale, const String& p_message, const String& p_context) const
{
	String res;
	int best_score = 0;

	for (const Ref<Translation>& E : translations) {
		int score = TranslationServer::compare_locales(p_locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			const String r = E->get_message(p_message, p_context);
			if (r.is_empty()) {
				continue;
			}
			res = r;
			best_score = score;
			if (score == 10) {
				break; // Exact match, skip the rest.
			}
		}
	}

	return res;
}

String TranslationDomain::get_message_from_translations(const String& p_locale,
	const String& p_message, const String& p_message_plural, int p_n,
	const String& p_context) const
{
	String res;
	int best_score = 0;

	for (const Ref<Translation>& E : translations) {
		int score = TranslationServer::compare_locales(p_locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			const String r = E->get_plural_message(p_message, p_message_plural, p_n, p_context);
			if (r.is_empty()) {
				continue;
			}
			res = r;
			best_score = score;
			if (score == 10) {
				break; // Exact match, skip the rest.
			}
		}
	}

	return res;
}

PackedStringArray TranslationDomain::get_loaded_locales() const
{
	PackedStringArray locales;
	for (const Ref<Translation>& E : translations) {
		const String& locale = E->get_locale();
		if (!locales.has(locale)) {
			locales.push_back(locale);
		}
	}
	return locales;
}

#ifndef DISABLE_DEPRECATED
Ref<Translation> TranslationDomain::get_translation_object(const String& p_locale) const
{
	Ref<Translation> res;
	int best_score = 0;

	for (const Ref<Translation>& E : translations) {
		int score = TranslationServer::compare_locales(p_locale, E->get_locale());
		if (score > 0 && score >= best_score) {
			res = E;
			best_score = score;
			if (score == 10) {
				break; // Exact match, skip the rest.
			}
		}
	}
	return res;
}
#endif

void TranslationDomain::add_translation(const Ref<Translation>& p_translation)
{
	ERR_FAIL_COND_MSG(p_translation.is_null(), "Invalid translation provided.");
	translations.insert(p_translation);
}

void TranslationDomain::remove_translation(const Ref<Translation>& p_translation)
{
	translations.erase(p_translation);
}

void TranslationDomain::clear() { translations.clear(); }

const HashSet<Ref<Translation>> TranslationDomain::get_translations() const
{
	return HashSet<Ref<Translation>>(translations);
}

HashSet<Ref<Translation>> TranslationDomain::find_translations(
	const String& p_locale, bool p_exact) const
{
	HashSet<Ref<Translation>> res;
	if (p_exact) {
		for (const Ref<Translation>& E : translations) {
			if (E->get_locale() == p_locale) {
				res.insert(E);
			}
		}
	}
	else {
		for (const Ref<Translation>& E : translations) {
			if (TranslationServer::compare_locales(p_locale, E->get_locale()) >
				0) {
				res.insert(E);
			}
		}
	}
	return res;
}

bool TranslationDomain::has_translation(const Ref<Translation>& p_translation) const
{
	return translations.has(p_translation);
}

bool TranslationDomain::has_translation_for_locale(const String& p_locale, bool p_exact) const
{
	if (p_exact) {
		for (const Ref<Translation>& E : translations) {
			if (E->get_locale() == p_locale) {
				return true;
			}
		}
	}
	else {
		for (const Ref<Translation>& E : translations) {
			if (TranslationServer::compare_locales(p_locale, E->get_locale()) >
				0) {
				return true;
			}
		}
	}
	return false;
}

String TranslationDomain::translate(
	const String& p_message, const String& p_context) const
{
	if (!enabled) {
		return p_message;
	}

	const String& locale = locale_override.is_empty()
							   ? TranslationServer::get_locale()
							   : locale_override;
	String res = get_message_from_translations(locale, p_message, p_context);

	const bool& allow_fallback = TranslationServer::is_fallback_allowed();
	const String& fallback = TranslationServer::get_fallback_locale();
	if (res.is_empty() && allow_fallback && fallback.length() >= 2) {
		res = get_message_from_translations(fallback, p_message, p_context);
	}

	if (res.is_empty()) {
		return pseudolocalization.enabled ? pseudolocalize(p_message) : p_message;
	}
	return pseudolocalization.enabled ? pseudolocalize(res) : res;
}

String TranslationDomain::translate_plural(const String& p_message,
	const String& p_message_plural, int p_n, const String& p_context) const
{
	if (!enabled) {
		return p_n == 1 ? p_message : p_message_plural;
	}

	const String& locale = locale_override.is_empty()
							   ? TranslationServer::get_locale()
							   : locale_override;
	String res =
		get_message_from_translations(locale, p_message, p_message_plural, p_n, p_context);

	const bool& allow_fallback = TranslationServer::is_fallback_allowed();
	const String& fallback = TranslationServer::get_fallback_locale();
	if (res.is_empty() && allow_fallback && fallback.length() >= 2) {
		res = get_message_from_translations(fallback, p_message, p_message_plural, p_n, p_context);
	}

	if (res.is_empty()) {
		if (p_n == 1) {
			return p_message;
		}
		return p_message_plural;
	}
	return res;
}

String TranslationDomain::get_locale_override() const { return locale_override; }

void TranslationDomain::set_locale_override(const String& p_locale)
{
	locale_override = p_locale.is_empty()
						  ? p_locale
						  : TranslationServer::standardize_locale(p_locale);
}

bool TranslationDomain::is_enabled() const { return enabled; }

void TranslationDomain::set_enabled(bool p_enabled) { enabled = p_enabled; }

bool TranslationDomain::is_pseudolocalization_enabled() const { return pseudolocalization.enabled; }

void TranslationDomain::set_pseudolocalization_enabled(bool p_enabled)
{
	pseudolocalization.enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_accents_enabled() const
{
	return pseudolocalization.accents_enabled;
}

void TranslationDomain::set_pseudolocalization_accents_enabled(bool p_enabled)
{
	pseudolocalization.accents_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_double_vowels_enabled() const
{
	return pseudolocalization.double_vowels_enabled;
}

void TranslationDomain::set_pseudolocalization_double_vowels_enabled(bool p_enabled)
{
	pseudolocalization.double_vowels_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_fake_bidi_enabled() const
{
	return pseudolocalization.fake_bidi_enabled;
}

void TranslationDomain::set_pseudolocalization_fake_bidi_enabled(bool p_enabled)
{
	pseudolocalization.fake_bidi_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_override_enabled() const
{
	return pseudolocalization.override_enabled;
}

void TranslationDomain::set_pseudolocalization_override_enabled(bool p_enabled)
{
	pseudolocalization.override_enabled = p_enabled;
}

bool TranslationDomain::is_pseudolocalization_skip_placeholders_enabled() const
{
	return pseudolocalization.skip_placeholders_enabled;
}

void TranslationDomain::set_pseudolocalization_skip_placeholders_enabled(bool p_enabled)
{
	pseudolocalization.skip_placeholders_enabled = p_enabled;
}

float TranslationDomain::get_pseudolocalization_expansion_ratio() const
{
	return pseudolocalization.expansion_ratio;
}

void TranslationDomain::set_pseudolocalization_expansion_ratio(float p_ratio)
{
	pseudolocalization.expansion_ratio = p_ratio;
}

String TranslationDomain::get_pseudolocalization_prefix() const
{
	return pseudolocalization.prefix;
}

void TranslationDomain::set_pseudolocalization_prefix(const String& p_prefix)
{
	pseudolocalization.prefix = p_prefix;
}

String TranslationDomain::get_pseudolocalization_suffix() const
{
	return pseudolocalization.suffix;
}

void TranslationDomain::set_pseudolocalization_suffix(const String& p_suffix)
{
	pseudolocalization.suffix = p_suffix;
}

String TranslationDomain::pseudolocalize(const String& p_message) const
{
	if (p_message.is_empty()) {
		return p_message;
	}

	String message = p_message;
	int length = message.length();
	if (pseudolocalization.override_enabled) {
		message = _get_override_string(message);
	}

	if (pseudolocalization.double_vowels_enabled) {
		message = _double_vowels(message);
	}

	if (pseudolocalization.accents_enabled) {
		message = _replace_with_accented_string(message);
	}

	if (pseudolocalization.fake_bidi_enabled) {
		message = _wrap_with_fakebidi_characters(message);
	}

	return _add_padding(message, length);
}

// ==========================================================================
// translation_server.cpp
// ==========================================================================

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
#include "translation_server.h"

// --- translation_server.compat.inc ---
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

#ifndef DISABLE_DEPRECATED


String TranslationServer::_standardize_locale_bind_compat_98972(const String& p_locale)
{
	return standardize_locale(p_locale, false);
}

void TranslationServer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED


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
