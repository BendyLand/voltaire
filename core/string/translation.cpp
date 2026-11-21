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
#include "core/string/translation_server.h"

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
// --- Inlined from core/string/locales.h ---
// Windows has some weird locale identifiers which do not honor the ISO 639-1
// standardized nomenclature. Whenever those don't conflict with existing ISO
// identifiers, we override them.
//
// Reference:
// - https://msdn.microsoft.com/en-us/library/windows/desktop/ms693062(v=vs.85).aspx

static const char *locale_renames[][2] = {
	{ "in", "id" }, //  Indonesian
	{ "iw", "he" }, //  Hebrew
	{ "no", "nb" }, //  Norwegian Bokmål
	{ "C", "en" }, // Locale is not set, fallback to English.
	{ nullptr, nullptr }
};

// Additional script information to preferred scripts.
// Language code, script code, default country, supported countries.
// Reference:
// - https://lh.2xlibre.net/locales/
// - https://www.localeplanet.com/icu/index.html
// - https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f

static const char *locale_scripts[][4] = {
	{ "az", "Latn", "", "AZ" },
	{ "az", "Arab", "", "IR" },
	{ "bs", "Latn", "", "BA" },
	{ "ff", "Latn", "", "BF,CM,GH,GM,GN,GW,LR,MR,NE,NG,SL,SN" },
	{ "pa", "Arab", "PK", "PK" },
	{ "pa", "Guru", "IN", "IN" },
	{ "sd", "Arab", "PK", "PK" },
	{ "sd", "Deva", "IN", "IN" },
	{ "shi", "Tfng", "", "MA" },
	{ "sr", "Cyrl", "", "BA,RS,XK" },
	{ "sr", "Latn", "", "ME" },
	{ "uz", "Latn", "", "UZ" },
	{ "uz", "Arab", "AF", "AF" },
	{ "vai", "Vaii", "", "LR" },
	{ "yue", "Hans", "CN", "CN" },
	{ "yue", "Hant", "HK", "HK" },
	{ "zh", "Hans", "CN", "CN,SG" },
	{ "zh", "Hant", "TW", "HK,MO,TW" },
	{ nullptr, nullptr, nullptr, nullptr }
};

// Additional mapping for outdated, temporary or exceptionally reserved country codes.
// Reference:
// - https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
// - https://www.iso.org/obp/ui/#search/code/

static const char *country_renames[][2] = {
	{ "BU", "MM" }, // Burma, name changed to Myanmar.
	{ "KV", "XK" }, // Kosovo (temporary FIPS code to European Commission code), no official ISO code assigned.
	{ "TP", "TL" }, // East Timor, name changed to Timor-Leste.
	{ "UK", "GB" }, // United Kingdom, exceptionally reserved code.
	{ nullptr, nullptr }
};

// Country code, country name.
// Reference:
// - https://en.wikipedia.org/wiki/ISO_3166-1_alpha-2
// - https://www.iso.org/obp/ui/#search/code/

static const char *country_names[][2] = {
	{ "AC", "Ascension Island" }, // Exceptionally reserved.
	{ "AD", "Andorra" },
	{ "AE", "United Arab Emirates" },
	{ "AF", "Afghanistan" },
	{ "AG", "Antigua and Barbuda" },
	{ "AI", "Anguilla" },
	{ "AL", "Albania" },
	{ "AM", "Armenia" },
	{ "AN", "Netherlands Antilles" }, // Transitionally reserved, divided into BQ, CW and SX.
	{ "AO", "Angola" },
	{ "AQ", "Antarctica" },
	{ "AR", "Argentina" },
	{ "AS", "American Samoa" },
	{ "AT", "Austria" },
	{ "AU", "Australia" },
	{ "AW", "Aruba" },
	{ "AX", "Åland Islands" },
	{ "AZ", "Azerbaijan" },
	{ "BA", "Bosnia and Herzegovina" },
	{ "BB", "Barbados" },
	{ "BD", "Bangladesh" },
	{ "BE", "Belgium" },
	{ "BF", "Burkina Faso" },
	{ "BG", "Bulgaria" },
	{ "BH", "Bahrain" },
	{ "BI", "Burundi" },
	{ "BJ", "Benin" },
	{ "BL", "St. Barthélemy" },
	{ "BM", "Bermuda" },
	{ "BN", "Brunei" },
	{ "BO", "Bolivia" },
	{ "BQ", "Caribbean Netherlands" },
	{ "BR", "Brazil" },
	{ "BS", "Bahamas" },
	{ "BT", "Bhutan" },
	{ "BV", "Bouvet Island" },
	{ "BW", "Botswana" },
	{ "BY", "Belarus" },
	{ "BZ", "Belize" },
	{ "CA", "Canada" },
	{ "CC", "Cocos (Keeling) Islands" },
	{ "CD", "Congo - Kinshasa" },
	{ "CF", "Central African Republic" },
	{ "CG", "Congo - Brazzaville" },
	{ "CH", "Switzerland" },
	{ "CI", "Côte d'Ivoire" },
	{ "CK", "Cook Islands" },
	{ "CL", "Chile" },
	{ "CM", "Cameroon" },
	{ "CN", "China" },
	{ "CO", "Colombia" },
	{ "CP", "Clipperton Island" }, // Exceptionally reserved.
	{ "CR", "Costa Rica" },
	{ "CQ", "Island of Sark" }, // Exceptionally reserved.
	{ "CU", "Cuba" },
	{ "CV", "Cabo Verde" },
	{ "CW", "Curaçao" },
	{ "CX", "Christmas Island" },
	{ "CY", "Cyprus" },
	{ "CZ", "Czechia" },
	{ "DE", "Germany" },
	{ "DG", "Diego Garcia" }, // Exceptionally reserved.
	{ "DJ", "Djibouti" },
	{ "DK", "Denmark" },
	{ "DM", "Dominica" },
	{ "DO", "Dominican Republic" },
	{ "DZ", "Algeria" },
	{ "EA", "Ceuta and Melilla" }, // Exceptionally reserved.
	{ "EC", "Ecuador" },
	{ "EE", "Estonia" },
	{ "EG", "Egypt" },
	{ "EH", "Western Sahara" },
	{ "ER", "Eritrea" },
	{ "ES", "Spain" },
	{ "ET", "Ethiopia" },
	{ "EU", "European Union" }, // Exceptionally reserved.
	{ "EZ", "Eurozone" }, // Exceptionally reserved.
	{ "FI", "Finland" },
	{ "FJ", "Fiji" },
	{ "FK", "Falkland Islands" },
	{ "FM", "Micronesia" },
	{ "FO", "Faroe Islands" },
	{ "FR", "France" },
	{ "FX", "France, Metropolitan" }, // Exceptionally reserved.
	{ "GA", "Gabon" },
	{ "GB", "United Kingdom" },
	{ "GD", "Grenada" },
	{ "GE", "Georgia" },
	{ "GF", "French Guiana" },
	{ "GG", "Guernsey" },
	{ "GH", "Ghana" },
	{ "GI", "Gibraltar" },
	{ "GL", "Greenland" },
	{ "GM", "Gambia" },
	{ "GN", "Guinea" },
	{ "GP", "Guadeloupe" },
	{ "GQ", "Equatorial Guinea" },
	{ "GR", "Greece" },
	{ "GS", "South Georgia and South Sandwich Islands" },
	{ "GT", "Guatemala" },
	{ "GU", "Guam" },
	{ "GW", "Guinea-Bissau" },
	{ "GY", "Guyana" },
	{ "HK", "Hong Kong" },
	{ "HM", "Heard Island and McDonald Islands" },
	{ "HN", "Honduras" },
	{ "HR", "Croatia" },
	{ "HT", "Haiti" },
	{ "HU", "Hungary" },
	{ "IC", "Canary Islands" }, // Exceptionally reserved.
	{ "ID", "Indonesia" },
	{ "IE", "Ireland" },
	{ "IL", "Israel" },
	{ "IM", "Isle of Man" },
	{ "IN", "India" },
	{ "IO", "British Indian Ocean Territory" },
	{ "IQ", "Iraq" },
	{ "IR", "Iran" },
	{ "IS", "Iceland" },
	{ "IT", "Italy" },
	{ "JE", "Jersey" },
	{ "JM", "Jamaica" },
	{ "JO", "Jordan" },
	{ "JP", "Japan" },
	{ "KE", "Kenya" },
	{ "KG", "Kyrgyzstan" },
	{ "KH", "Cambodia" },
	{ "KI", "Kiribati" },
	{ "KM", "Comoros" },
	{ "KN", "St. Kitts and Nevis" },
	{ "KP", "North Korea" },
	{ "KR", "South Korea" },
	{ "KW", "Kuwait" },
	{ "KY", "Cayman Islands" },
	{ "KZ", "Kazakhstan" },
	{ "LA", "Laos" },
	{ "LB", "Lebanon" },
	{ "LC", "St. Lucia" },
	{ "LI", "Liechtenstein" },
	{ "LK", "Sri Lanka" },
	{ "LR", "Liberia" },
	{ "LS", "Lesotho" },
	{ "LT", "Lithuania" },
	{ "LU", "Luxembourg" },
	{ "LV", "Latvia" },
	{ "LY", "Libya" },
	{ "MA", "Morocco" },
	{ "MC", "Monaco" },
	{ "MD", "Moldova" },
	{ "ME", "Montenegro" },
	{ "MF", "St. Martin" },
	{ "MG", "Madagascar" },
	{ "MH", "Marshall Islands" },
	{ "MK", "North Macedonia" },
	{ "ML", "Mali" },
	{ "MM", "Myanmar" },
	{ "MN", "Mongolia" },
	{ "MO", "Macao" },
	{ "MP", "Northern Mariana Islands" },
	{ "MQ", "Martinique" },
	{ "MR", "Mauritania" },
	{ "MS", "Montserrat" },
	{ "MT", "Malta" },
	{ "MU", "Mauritius" },
	{ "MV", "Maldives" },
	{ "MW", "Malawi" },
	{ "MX", "Mexico" },
	{ "MY", "Malaysia" },
	{ "MZ", "Mozambique" },
	{ "NA", "Namibia" },
	{ "NC", "New Caledonia" },
	{ "NE", "Niger" },
	{ "NF", "Norfolk Island" },
	{ "NG", "Nigeria" },
	{ "NI", "Nicaragua" },
	{ "NL", "Netherlands" },
	{ "NO", "Norway" },
	{ "NP", "Nepal" },
	{ "NR", "Nauru" },
	{ "NU", "Niue" },
	{ "NZ", "New Zealand" },
	{ "OM", "Oman" },
	{ "PA", "Panama" },
	{ "PE", "Peru" },
	{ "PF", "French Polynesia" },
	{ "PG", "Papua New Guinea" },
	{ "PH", "Philippines" },
	{ "PK", "Pakistan" },
	{ "PL", "Poland" },
	{ "PM", "St. Pierre and Miquelon" },
	{ "PN", "Pitcairn Islands" },
	{ "PR", "Puerto Rico" },
	{ "PS", "Palestine" },
	{ "PT", "Portugal" },
	{ "PW", "Palau" },
	{ "PY", "Paraguay" },
	{ "QA", "Qatar" },
	{ "RE", "Réunion" },
	{ "RO", "Romania" },
	{ "RS", "Serbia" },
	{ "RU", "Russia" },
	{ "RW", "Rwanda" },
	{ "SA", "Saudi Arabia" },
	{ "SB", "Solomon Islands" },
	{ "SC", "Seychelles" },
	{ "SD", "Sudan" },
	{ "SE", "Sweden" },
	{ "SG", "Singapore" },
	{ "SH", "St. Helena, Ascension and Tristan da Cunha" },
	{ "SI", "Slovenia" },
	{ "SJ", "Svalbard and Jan Mayen" },
	{ "SK", "Slovakia" },
	{ "SL", "Sierra Leone" },
	{ "SM", "San Marino" },
	{ "SN", "Senegal" },
	{ "SO", "Somalia" },
	{ "SR", "Suriname" },
	{ "SS", "South Sudan" },
	{ "ST", "Sao Tome and Principe" },
	{ "SV", "El Salvador" },
	{ "SX", "Sint Maarten" },
	{ "SY", "Syria" },
	{ "SZ", "Eswatini" },
	{ "TA", "Tristan da Cunha" }, // Exceptionally reserved.
	{ "TC", "Turks and Caicos Islands" },
	{ "TD", "Chad" },
	{ "TF", "French Southern Territories" },
	{ "TG", "Togo" },
	{ "TH", "Thailand" },
	{ "TJ", "Tajikistan" },
	{ "TK", "Tokelau" },
	{ "TL", "Timor-Leste" },
	{ "TM", "Turkmenistan" },
	{ "TN", "Tunisia" },
	{ "TO", "Tonga" },
	{ "TR", "Turkey" },
	{ "TT", "Trinidad and Tobago" },
	{ "TV", "Tuvalu" },
	{ "TW", "Taiwan" },
	{ "TZ", "Tanzania" },
	{ "UA", "Ukraine" },
	{ "UG", "Uganda" },
	{ "UM", "U.S. Outlying Islands" },
	{ "US", "United States of America" },
	{ "UY", "Uruguay" },
	{ "UZ", "Uzbekistan" },
	{ "VA", "Holy See" },
	{ "VC", "St. Vincent and the Grenadines" },
	{ "VE", "Venezuela" },
	{ "VG", "British Virgin Islands" },
	{ "VI", "U.S. Virgin Islands" },
	{ "VN", "Viet Nam" },
	{ "VU", "Vanuatu" },
	{ "WF", "Wallis and Futuna" },
	{ "WS", "Samoa" },
	{ "XK", "Kosovo" }, // Temporary code, no official ISO code assigned.
	{ "YE", "Yemen" },
	{ "YT", "Mayotte" },
	{ "ZA", "South Africa" },
	{ "ZM", "Zambia" },
	{ "ZW", "Zimbabwe" },
	{ nullptr, nullptr }
};

// Languages code, language name.
// Reference:
// - https://en.wikipedia.org/wiki/List_of_ISO_639-1_codes
// - https://www.localeplanet.com/icu/index.html
// - https://lh.2xlibre.net/locales/

static const char *language_list[][2] = {
	{ "aa", "Afar" },
	{ "ab", "Abkhazian" },
	{ "ace", "Achinese" },
	{ "ach", "Acoli" },
	{ "ada", "Adangme" },
	{ "ady", "Adyghe" },
	{ "ae", "Avestan" },
	{ "aeb", "Tunisian Arabic" },
	{ "af", "Afrikaans" },
	{ "afh", "Afrihili" },
	{ "agq", "Aghem" },
	{ "ain", "Ainu" },
	{ "agr", "Aguaruna" },
	{ "ak", "Akan" },
	{ "akk", "Akkadian" },
	{ "akz", "Alabama" },
	{ "ale", "Aleut" },
	{ "aln", "Gheg Albanian" },
	{ "alt", "Southern Altai" },
	{ "am", "Amharic" },
	{ "an", "Aragonese" },
	{ "ang", "Old English" },
	{ "anp", "Angika" },
	{ "ar", "Arabic" },
	{ "arc", "Aramaic" },
	{ "arn", "Mapudungun" },
	{ "aro", "Araona" },
	{ "arp", "Arapaho" },
	{ "arq", "Algerian Arabic" },
	{ "ars", "Najdi Arabic" },
	{ "arw", "Arawak" },
	{ "ary", "Moroccan Arabic" },
	{ "arz", "Egyptian Arabic" },
	{ "as", "Assamese" },
	{ "asa", "Asu" },
	{ "ase", "American Sign Language" },
	{ "ast", "Asturian" },
	{ "av", "Avaric" },
	{ "avk", "Kotava" },
	{ "awa", "Awadhi" },
	{ "ayc", "Southern Aymara" },
	{ "ay", "Aymara" },
	{ "az", "Azerbaijani" },
	{ "ba", "Bashkir" },
	{ "bal", "Baluchi" },
	{ "ban", "Balinese" },
	{ "bar", "Bavarian" },
	{ "bas", "Bassa" },
	{ "bax", "Bamun" },
	{ "bbc", "Batak Toba" },
	{ "bbj", "Ghomala" },
	{ "be", "Belarusian" },
	{ "bej", "Beja" },
	{ "bem", "Bemba" },
	{ "ber", "Berber" },
	{ "bew", "Betawi" },
	{ "bez", "Bena" },
	{ "bfd", "Bafut" },
	{ "bfq", "Badaga" },
	{ "bg", "Bulgarian" },
	{ "bhb", "Bhili" },
	{ "bgn", "Western Balochi" },
	{ "bho", "Bhojpuri" },
	{ "bi", "Bislama" },
	{ "bik", "Bikol" },
	{ "bin", "Bini" },
	{ "bjn", "Banjar" },
	{ "bkm", "Kom" },
	{ "bla", "Siksika" },
	{ "bm", "Bambara" },
	{ "bn", "Bengali" },
	{ "bo", "Tibetan" },
	{ "bpy", "Bishnupriya" },
	{ "bqi", "Bakhtiari" },
	{ "br", "Breton" },
	{ "brh", "Brahui" },
	{ "brx", "Bodo" },
	{ "bs", "Bosnian" },
	{ "bss", "Akoose" },
	{ "bua", "Buriat" },
	{ "bug", "Buginese" },
	{ "bum", "Bulu" },
	{ "byn", "Bilin" },
	{ "byv", "Medumba" },
	{ "ca", "Catalan" },
	{ "cad", "Caddo" },
	{ "car", "Carib" },
	{ "cay", "Cayuga" },
	{ "cch", "Atsam" },
	{ "ccp", "Chakma" },
	{ "ce", "Chechen" },
	{ "ceb", "Cebuano" },
	{ "cgg", "Chiga" },
	{ "ch", "Chamorro" },
	{ "chb", "Chibcha" },
	{ "chg", "Chagatai" },
	{ "chk", "Chuukese" },
	{ "chm", "Mari" },
	{ "chn", "Chinook Jargon" },
	{ "cho", "Choctaw" },
	{ "chp", "Chipewyan" },
	{ "chr", "Cherokee" },
	{ "chy", "Cheyenne" },
	{ "cic", "Chickasaw" },
	{ "ckb", "Central Kurdish" },
	{ "cmn", "Mandarin Chinese" },
	{ "co", "Corsican" },
	{ "cop", "Coptic" },
	{ "cps", "Capiznon" },
	{ "cr", "Cree" },
	{ "crh", "Crimean Tatar" },
	{ "crs", "Seselwa Creole French" },
	{ "cs", "Czech" },
	{ "csb", "Kashubian" },
	{ "cu", "Church Slavic" },
	{ "cv", "Chuvash" },
	{ "cy", "Welsh" },
	{ "da", "Danish" },
	{ "dak", "Dakota" },
	{ "dar", "Dargwa" },
	{ "dav", "Taita" },
	{ "de", "German" },
	{ "del", "Delaware" },
	{ "den", "Slave" },
	{ "dgr", "Dogrib" },
	{ "din", "Dinka" },
	{ "dje", "Zarma" },
	{ "doi", "Dogri" },
	{ "dsb", "Lower Sorbian" },
	{ "dtp", "Central Dusun" },
	{ "dua", "Duala" },
	{ "dum", "Middle Dutch" },
	{ "dv", "Dhivehi" },
	{ "dyo", "Jola-Fonyi" },
	{ "dyu", "Dyula" },
	{ "dz", "Dzongkha" },
	{ "dzg", "Dazaga" },
	{ "ebu", "Embu" },
	{ "ee", "Ewe" },
	{ "efi", "Efik" },
	{ "egl", "Emilian" },
	{ "egy", "Ancient Egyptian" },
	{ "eka", "Ekajuk" },
	{ "el", "Greek" },
	{ "elx", "Elamite" },
	{ "en", "English" },
	{ "enm", "Middle English" },
	{ "eo", "Esperanto" },
	{ "es", "Spanish" },
	{ "esu", "Central Yupik" },
	{ "et", "Estonian" },
	{ "eu", "Basque" },
	{ "ewo", "Ewondo" },
	{ "ext", "Extremaduran" },
	{ "fa", "Persian" },
	{ "fan", "Fang" },
	{ "fat", "Fanti" },
	{ "ff", "Fulah" },
	{ "fi", "Finnish" },
	{ "fil", "Filipino" },
	{ "fit", "Tornedalen Finnish" },
	{ "fj", "Fijian" },
	{ "fo", "Faroese" },
	{ "fon", "Fon" },
	{ "fr", "French" },
	{ "frc", "Cajun French" },
	{ "frm", "Middle French" },
	{ "fro", "Old French" },
	{ "frp", "Arpitan" },
	{ "frr", "Northern Frisian" },
	{ "frs", "Eastern Frisian" },
	{ "fur", "Friulian" },
	{ "fy", "Western Frisian" },
	{ "ga", "Irish" },
	{ "gaa", "Ga" },
	{ "gag", "Gagauz" },
	{ "gan", "Gan Chinese" },
	{ "gay", "Gayo" },
	{ "gba", "Gbaya" },
	{ "gbz", "Zoroastrian Dari" },
	{ "gd", "Scottish Gaelic" },
	{ "gez", "Geez" },
	{ "gil", "Gilbertese" },
	{ "gl", "Galician" },
	{ "glk", "Gilaki" },
	{ "gmh", "Middle High German" },
	{ "gn", "Guarani" },
	{ "goh", "Old High German" },
	{ "gom", "Goan Konkani" },
	{ "gon", "Gondi" },
	{ "gor", "Gorontalo" },
	{ "got", "Gothic" },
	{ "grb", "Grebo" },
	{ "grc", "Ancient Greek" },
	{ "gsw", "Swiss German" },
	{ "gu", "Gujarati" },
	{ "guc", "Wayuu" },
	{ "gur", "Frafra" },
	{ "guz", "Gusii" },
	{ "gv", "Manx" },
	{ "gwi", "Gwichʼin" },
	{ "ha", "Hausa" },
	{ "hai", "Haida" },
	{ "hak", "Hakka Chinese" },
	{ "haw", "Hawaiian" },
	{ "he", "Hebrew" },
	{ "hi", "Hindi" },
	{ "hif", "Fiji Hindi" },
	{ "hil", "Hiligaynon" },
	{ "hit", "Hittite" },
	{ "hmn", "Hmong" },
	{ "ho", "Hiri Motu" },
	{ "hne", "Chhattisgarhi" },
	{ "hr", "Croatian" },
	{ "hsb", "Upper Sorbian" },
	{ "hsn", "Xiang Chinese" },
	{ "ht", "Haitian" },
	{ "hu", "Hungarian" },
	{ "hup", "Hupa" },
	{ "hus", "Huastec" },
	{ "hy", "Armenian" },
	{ "hz", "Herero" },
	{ "ia", "Interlingua" },
	{ "iba", "Iban" },
	{ "ibb", "Ibibio" },
	{ "id", "Indonesian" },
	{ "ie", "Interlingue" },
	{ "ig", "Igbo" },
	{ "ii", "Sichuan Yi" },
	{ "ik", "Inupiaq" },
	{ "ilo", "Iloko" },
	{ "inh", "Ingush" },
	{ "io", "Ido" },
	{ "is", "Icelandic" },
	{ "it", "Italian" },
	{ "iu", "Inuktitut" },
	{ "izh", "Ingrian" },
	{ "ja", "Japanese" },
	{ "jam", "Jamaican Creole English" },
	{ "jbo", "Lojban" },
	{ "jgo", "Ngomba" },
	{ "jmc", "Machame" },
	{ "jpr", "Judeo-Persian" },
	{ "jrb", "Judeo-Arabic" },
	{ "jut", "Jutish" },
	{ "jv", "Javanese" },
	{ "ka", "Georgian" },
	{ "kaa", "Kara-Kalpak" },
	{ "kab", "Kabyle" },
	{ "kac", "Kachin" },
	{ "kaj", "Jju" },
	{ "kam", "Kamba" },
	{ "kaw", "Kawi" },
	{ "kbd", "Kabardian" },
	{ "kbl", "Kanembu" },
	{ "kcg", "Tyap" },
	{ "kde", "Makonde" },
	{ "kea", "Kabuverdianu" },
	{ "ken", "Kenyang" },
	{ "kfo", "Koro" },
	{ "kg", "Kongo" },
	{ "kgp", "Kaingang" },
	{ "kha", "Khasi" },
	{ "kho", "Khotanese" },
	{ "khq", "Koyra Chiini" },
	{ "khw", "Khowar" },
	{ "ki", "Kikuyu" },
	{ "kiu", "Kirmanjki" },
	{ "kj", "Kuanyama" },
	{ "kk", "Kazakh" },
	{ "kkj", "Kako" },
	{ "kl", "Kalaallisut" },
	{ "kln", "Kalenjin" },
	{ "km", "Central Khmer" },
	{ "kmb", "Kimbundu" },
	{ "kn", "Kannada" },
	{ "ko", "Korean" },
	{ "koi", "Komi-Permyak" },
	{ "kok", "Konkani" },
	{ "kos", "Kosraean" },
	{ "kpe", "Kpelle" },
	{ "kr", "Kanuri" },
	{ "krc", "Karachay-Balkar" },
	{ "kri", "Krio" },
	{ "krj", "Kinaray-a" },
	{ "krl", "Karelian" },
	{ "kru", "Kurukh" },
	{ "ks", "Kashmiri" },
	{ "ksb", "Shambala" },
	{ "ksf", "Bafia" },
	{ "ksh", "Colognian" },
	{ "ku", "Kurdish" },
	{ "kum", "Kumyk" },
	{ "kut", "Kutenai" },
	{ "kv", "Komi" },
	{ "kw", "Cornish" },
	{ "ky", "Kirghiz" },
	{ "la", "Latin" },
	{ "lad", "Ladino" },
	{ "lag", "Langi" },
	{ "lah", "Lahnda" },
	{ "lam", "Lamba" },
	{ "lb", "Luxembourgish" },
	{ "lez", "Lezghian" },
	{ "lfn", "Lingua Franca Nova" },
	{ "lg", "Ganda" },
	{ "li", "Limburgan" },
	{ "lij", "Ligurian" },
	{ "liv", "Livonian" },
	{ "lkt", "Lakota" },
	{ "lmo", "Lombard" },
	{ "ln", "Lingala" },
	{ "lo", "Lao" },
	{ "lol", "Mongo" },
	{ "lou", "Louisiana Creole" },
	{ "loz", "Lozi" },
	{ "lrc", "Northern Luri" },
	{ "lt", "Lithuanian" },
	{ "ltg", "Latgalian" },
	{ "lu", "Luba-Katanga" },
	{ "lua", "Luba-Lulua" },
	{ "lui", "Luiseno" },
	{ "lun", "Lunda" },
	{ "luo", "Luo" },
	{ "lus", "Mizo" },
	{ "luy", "Luyia" },
	{ "lv", "Latvian" },
	{ "lzh", "Literary Chinese" },
	{ "lzz", "Laz" },
	{ "mad", "Madurese" },
	{ "maf", "Mafa" },
	{ "mag", "Magahi" },
	{ "mai", "Maithili" },
	{ "mak", "Makasar" },
	{ "man", "Mandingo" },
	{ "mas", "Masai" },
	{ "mde", "Maba" },
	{ "mdf", "Moksha" },
	{ "mdr", "Mandar" },
	{ "men", "Mende" },
	{ "mer", "Meru" },
	{ "mfe", "Morisyen" },
	{ "mg", "Malagasy" },
	{ "mga", "Middle Irish" },
	{ "mgh", "Makhuwa-Meetto" },
	{ "mgo", "Metaʼ" },
	{ "mh", "Marshallese" },
	{ "mhr", "Eastern Mari" },
	{ "mi", "Māori" },
	{ "mic", "Mi'kmaq" },
	{ "min", "Minangkabau" },
	{ "miq", "Mískito" },
	{ "mjw", "Karbi" },
	{ "mk", "Macedonian" },
	{ "ml", "Malayalam" },
	{ "mn", "Mongolian" },
	{ "mnc", "Manchu" },
	{ "mni", "Manipuri" },
	{ "mnw", "Mon" },
	{ "mos", "Mossi" },
	{ "moh", "Mohawk" },
	{ "mr", "Marathi" },
	{ "mrj", "Western Mari" },
	{ "ms", "Malay" },
	{ "mt", "Maltese" },
	{ "mua", "Mundang" },
	{ "mus", "Muscogee" },
	{ "mwl", "Mirandese" },
	{ "mwr", "Marwari" },
	{ "mwv", "Mentawai" },
	{ "my", "Burmese" },
	{ "mye", "Myene" },
	{ "myv", "Erzya" },
	{ "mzn", "Mazanderani" },
	{ "na", "Nauru" },
	{ "nah", "Nahuatl" },
	{ "nan", "Min Nan Chinese" },
	{ "nap", "Neapolitan" },
	{ "naq", "Nama" },
	{ "nan", "Min Nan Chinese" },
	{ "nb", "Norwegian Bokmål" },
	{ "nd", "North Ndebele" },
	{ "nds", "Low German" },
	{ "ne", "Nepali" },
	{ "new", "Newari" },
	{ "nhn", "Central Nahuatl" },
	{ "ng", "Ndonga" },
	{ "nia", "Nias" },
	{ "niu", "Niuean" },
	{ "njo", "Ao Naga" },
	{ "nl", "Dutch" },
	{ "nmg", "Kwasio" },
	{ "nn", "Norwegian Nynorsk" },
	{ "nnh", "Ngiemboon" },
	{ "nog", "Nogai" },
	{ "non", "Old Norse" },
	{ "nov", "Novial" },
	{ "nqo", "N'ko" },
	{ "nr", "South Ndebele" },
	{ "nso", "Pedi" },
	{ "nus", "Nuer" },
	{ "nv", "Navajo" },
	{ "nwc", "Classical Newari" },
	{ "ny", "Nyanja" },
	{ "nym", "Nyamwezi" },
	{ "nyn", "Nyankole" },
	{ "nyo", "Nyoro" },
	{ "nzi", "Nzima" },
	{ "oc", "Occitan" },
	{ "oj", "Ojibwa" },
	{ "om", "Oromo" },
	{ "or", "Odia" },
	{ "os", "Ossetic" },
	{ "osa", "Osage" },
	{ "ota", "Ottoman Turkish" },
	{ "pa", "Panjabi" },
	{ "pag", "Pangasinan" },
	{ "pal", "Pahlavi" },
	{ "pam", "Pampanga" },
	{ "pap", "Papiamento" },
	{ "pau", "Palauan" },
	{ "pcd", "Picard" },
	{ "pcm", "Nigerian Pidgin" },
	{ "pdc", "Pennsylvania German" },
	{ "pdt", "Plautdietsch" },
	{ "peo", "Old Persian" },
	{ "pfl", "Palatine German" },
	{ "phn", "Phoenician" },
	{ "pi", "Pali" },
	{ "pl", "Polish" },
	{ "pms", "Piedmontese" },
	{ "pnt", "Pontic" },
	{ "pon", "Pohnpeian" },
	{ "pr", "Pirate" },
	{ "prg", "Prussian" },
	{ "pro", "Old Provençal" },
	{ "prs", "Dari" },
	{ "ps", "Pushto" },
	{ "pt", "Portuguese" },
	{ "qu", "Quechua" },
	{ "quc", "K'iche" },
	{ "qug", "Chimborazo Highland Quichua" },
	{ "quy", "Ayacucho Quechua" },
	{ "quz", "Cusco Quechua" },
	{ "raj", "Rajasthani" },
	{ "rap", "Rapanui" },
	{ "rar", "Rarotongan" },
	{ "rgn", "Romagnol" },
	{ "rif", "Riffian" },
	{ "rm", "Romansh" },
	{ "rn", "Rundi" },
	{ "ro", "Romanian" },
	{ "rof", "Rombo" },
	{ "rom", "Romany" },
	{ "rtm", "Rotuman" },
	{ "ru", "Russian" },
	{ "rue", "Rusyn" },
	{ "rug", "Roviana" },
	{ "rup", "Aromanian" },
	{ "rw", "Kinyarwanda" },
	{ "rwk", "Rwa" },
	{ "sa", "Sanskrit" },
	{ "sad", "Sandawe" },
	{ "sah", "Sakha" },
	{ "sam", "Samaritan Aramaic" },
	{ "saq", "Samburu" },
	{ "sas", "Sasak" },
	{ "sat", "Santali" },
	{ "saz", "Saurashtra" },
	{ "sba", "Ngambay" },
	{ "sbp", "Sangu" },
	{ "sc", "Sardinian" },
	{ "scn", "Sicilian" },
	{ "sco", "Scots" },
	{ "sd", "Sindhi" },
	{ "sdc", "Sassarese Sardinian" },
	{ "sdh", "Southern Kurdish" },
	{ "se", "Northern Sami" },
	{ "see", "Seneca" },
	{ "seh", "Sena" },
	{ "sei", "Seri" },
	{ "sel", "Selkup" },
	{ "ses", "Koyraboro Senni" },
	{ "sg", "Sango" },
	{ "sga", "Old Irish" },
	{ "sgs", "Samogitian" },
	{ "sh", "Serbo-Croatian" },
	{ "shi", "Tachelhit" },
	{ "shn", "Shan" },
	{ "shs", "Shuswap" },
	{ "shu", "Chadian Arabic" },
	{ "si", "Sinhala" },
	{ "sid", "Sidamo" },
	{ "sk", "Slovak" },
	{ "sl", "Slovenian" },
	{ "sli", "Lower Silesian" },
	{ "sly", "Selayar" },
	{ "sm", "Samoan" },
	{ "sma", "Southern Sami" },
	{ "smj", "Lule Sami" },
	{ "smn", "Inari Sami" },
	{ "sms", "Skolt Sami" },
	{ "sn", "Shona" },
	{ "snk", "Soninke" },
	{ "so", "Somali" },
	{ "sog", "Sogdien" },
	{ "son", "Songhai" },
	{ "sq", "Albanian" },
	{ "sr", "Serbian" },
	{ "srn", "Sranan Tongo" },
	{ "srr", "Serer" },
	{ "ss", "Swati" },
	{ "ssy", "Saho" },
	{ "st", "Southern Sotho" },
	{ "stq", "Saterland Frisian" },
	{ "su", "Sundanese" },
	{ "suk", "Sukuma" },
	{ "sus", "Susu" },
	{ "sux", "Sumerian" },
	{ "sv", "Swedish" },
	{ "sw", "Swahili" },
	{ "swb", "Comorian" },
	{ "swc", "Congo Swahili" },
	{ "syc", "Classical Syriac" },
	{ "syr", "Syriac" },
	{ "szl", "Silesian" },
	{ "ta", "Tamil" },
	{ "tcy", "Tulu" },
	{ "te", "Telugu" },
	{ "tem", "Timne" },
	{ "teo", "Teso" },
	{ "ter", "Tereno" },
	{ "tet", "Tetum" },
	{ "tg", "Tajik" },
	{ "th", "Thai" },
	{ "the", "Chitwania Tharu" },
	{ "ti", "Tigrinya" },
	{ "tig", "Tigre" },
	{ "tiv", "Tiv" },
	{ "tk", "Turkmen" },
	{ "tkl", "Tokelau" },
	{ "tkr", "Tsakhur" },
	{ "tl", "Tagalog" },
	{ "tlh", "Klingon" },
	{ "tli", "Tlingit" },
	{ "tly", "Talysh" },
	{ "tmh", "Tamashek" },
	{ "tn", "Tswana" },
	{ "to", "Tongan" },
	{ "tok", "Toki Pona" },
	{ "tog", "Nyasa Tonga" },
	{ "tpi", "Tok Pisin" },
	{ "tr", "Turkish" },
	{ "tru", "Turoyo" },
	{ "trv", "Taroko" },
	{ "ts", "Tsonga" },
	{ "tsd", "Tsakonian" },
	{ "tsi", "Tsimshian" },
	{ "tt", "Tatar" },
	{ "ttt", "Muslim Tat" },
	{ "tum", "Tumbuka" },
	{ "tvl", "Tuvalu" },
	{ "tw", "Twi" },
	{ "twq", "Tasawaq" },
	{ "ty", "Tahitian" },
	{ "tyv", "Tuvinian" },
	{ "tzm", "Central Atlas Tamazight" },
	{ "udm", "Udmurt" },
	{ "ug", "Uyghur" },
	{ "uga", "Ugaritic" },
	{ "uk", "Ukrainian" },
	{ "umb", "Umbundu" },
	{ "unm", "Unami" },
	{ "ur", "Urdu" },
	{ "uz", "Uzbek" },
	{ "vai", "Vai" },
	{ "ve", "Venda" },
	{ "vec", "Venetian" },
	{ "vep", "Veps" },
	{ "vi", "Vietnamese" },
	{ "vls", "West Flemish" },
	{ "vmf", "Main-Franconian" },
	{ "vo", "Volapük" },
	{ "vot", "Votic" },
	{ "vro", "Võro" },
	{ "vun", "Vunjo" },
	{ "wa", "Walloon" },
	{ "wae", "Walser" },
	{ "wal", "Wolaytta" },
	{ "war", "Waray" },
	{ "was", "Washo" },
	{ "wbp", "Warlpiri" },
	{ "wo", "Wolof" },
	{ "wuu", "Wu Chinese" },
	{ "xal", "Kalmyk" },
	{ "xh", "Xhosa" },
	{ "xmf", "Mingrelian" },
	{ "xog", "Soga" },
	{ "yao", "Yao" },
	{ "yap", "Yapese" },
	{ "yav", "Yangben" },
	{ "ybb", "Yemba" },
	{ "yi", "Yiddish" },
	{ "yo", "Yoruba" },
	{ "yrl", "Nheengatu" },
	{ "yue", "Yue Chinese" },
	{ "yuw", "Papua New Guinea" },
	{ "za", "Zhuang" },
	{ "zap", "Zapotec" },
	{ "zbl", "Blissymbols" },
	{ "zea", "Zeelandic" },
	{ "zen", "Zenaga" },
	{ "zgh", "Standard Moroccan Tamazight" },
	{ "zh", "Chinese" },
	{ "zu", "Zulu" },
	{ "zun", "Zuni" },
	{ "zza", "Zaza" },
	{ nullptr, nullptr },
};

// Scripts used by languages.
// Reference:
// - https://www.unicode.org/cldr/charts/48/supplemental/languages_and_scripts.html

static const char *language_script_list[][2] = {
	{ "aa", "Latn" },
	{ "ab", "Cyrl" },
	{ "ace", "Latn" },
	{ "ach", "Latn" },
	{ "ada", "Latn" },
	{ "ady", "Cyrl" },
	{ "ae", "Avst" },
	{ "aeb", "Arab" },
	{ "af", "Latn" },
	{ "afh", "Latn" },
	{ "agq", "Latn" },
	{ "ain", "Kana Latn" },
	{ "agr", "Latn" },
	{ "ak", "Latn" },
	{ "akk", "Xsux" },
	{ "akz", "Latn" },
	{ "ale", "Latn" },
	{ "aln", "Latn" },
	{ "alt", "Cyrl" },
	{ "am", "Ethi" },
	{ "an", "Latn" },
	{ "ang", "Latn" },
	{ "anp", "Deva" },
	{ "ar", "Arab Syrc" },
	{ "arc", "Armi Nbat Palm" },
	{ "arn", "Latn" },
	{ "aro", "Latn" },
	{ "arp", "Latn" },
	{ "arq", "Arab" },
	{ "ars", "Arab" },
	{ "arw", "Latn" },
	{ "ary", "Arab" },
	{ "arz", "Arab" },
	{ "as", "Beng" },
	{ "asa", "Latn" },
	{ "ast", "Latn" },
	{ "av", "Cyrl" },
	{ "avk", "Latn" },
	{ "awa", "Deva" },
	{ "ayc", "Latn" },
	{ "ay", "Latn" },
	{ "az", "Latn Arab Cyrl" },
	{ "ba", "Cyrl" },
	{ "bal", "Arab Latn" },
	{ "ban", "Latn Bali" },
	{ "bar", "Latn" },
	{ "bas", "Latn" },
	{ "bax", "Bamu" },
	{ "bbc", "Latn Batk" },
	{ "bbj", "Latn" },
	{ "be", "Cyrl" },
	{ "bej", "Arab" },
	{ "bem", "Latn" },
	{ "ber", "Latn Arab Tfng" },
	{ "bew", "Latn" },
	{ "bez", "Latn" },
	{ "bfd", "Latn" },
	{ "bfq", "Taml" },
	{ "bg", "Cyrl" },
	{ "bhb", "Deva" },
	{ "bgn", "Arab" },
	{ "bho", "Deva" },
	{ "bi", "Latn" },
	{ "bik", "Latn" },
	{ "bin", "Latn" },
	{ "bjn", "Latn" },
	{ "bkm", "Latn" },
	{ "bla", "Latn" },
	{ "bm", "Latn Nkoo" },
	{ "bn", "Beng" },
	{ "bo", "Tibt" },
	{ "bpy", "Beng" },
	{ "bqi", "Arab" },
	{ "br", "Latn" },
	{ "brh", "Arab Latn" },
	{ "brx", "Deva" },
	{ "bs", "Latn Cyrl" },
	{ "bss", "Latn" },
	{ "bua", "Cyrl" },
	{ "bug", "Latn Bug" },
	{ "bum", "Latn" },
	{ "byn", "Ethi" },
	{ "byv", "Latn" },
	{ "ca", "Latn" },
	{ "cad", "Latn" },
	{ "car", "Latn" },
	{ "cay", "Latn" },
	{ "cch", "Latn" },
	{ "ccp", "Cakm Beng" },
	{ "ce", "Cyrl" },
	{ "ceb", "Latn" },
	{ "cgg", "Latn" },
	{ "ch", "Latn" },
	{ "chb", "Latn" },
	{ "chg", "Arab" },
	{ "chk", "Latn" },
	{ "chm", "Cyrl" },
	{ "chn", "Latn" },
	{ "cho", "Latn" },
	{ "chp", "Latn Cans" },
	{ "chr", "Cher" },
	{ "chy", "Latn" },
	{ "cic", "Latn" },
	{ "ckb", "Arab" },
	{ "cmn", "Hant Hans Hani" },
	{ "co", "Latn" },
	{ "cop", "Copt Arab Grek" },
	{ "cps", "Latn" },
	{ "cr", "Cans Latn" },
	{ "crh", "Cyrl" },
	{ "crs", "Latn" },
	{ "cs", "Latn" },
	{ "csb", "Latn" },
	{ "cu", "Cyrl" },
	{ "cv", "Cyrl" },
	{ "cy", "Latn" },
	{ "da", "Latn" },
	{ "dak", "Latn" },
	{ "dar", "Cyrl" },
	{ "dav", "Latn" },
	{ "de", "Latn Runr" },
	{ "del", "Latn" },
	{ "den", "Latn Cans" },
	{ "dgr", "Latn" },
	{ "din", "Latn" },
	{ "dje", "Latn" },
	{ "doi", "Deva Arab Takr" },
	{ "dsb", "Latn" },
	{ "dtp", "Latn" },
	{ "dua", "Latn" },
	{ "dum", "Latn" },
	{ "dv", "Thaa" },
	{ "dyo", "Latn Arab" },
	{ "dyu", "Latn" },
	{ "dz", "Tibt" },
	{ "dzg", "Latn" },
	{ "ebu", "Latn" },
	{ "ee", "Latn" },
	{ "efi", "Latn" },
	{ "egl", "Latn" },
	{ "egy", "Egyp" },
	{ "eka", "Latn" },
	{ "el", "Grek" },
	{ "elx", "Xsux" },
	{ "en", "Latn Dsrt Shaw" },
	{ "enm", "Latn" },
	{ "eo", "Latn" },
	{ "es", "Latn" },
	{ "esu", "Latn" },
	{ "et", "Latn" },
	{ "eu", "Latn" },
	{ "ewo", "Latn" },
	{ "ext", "Latn" },
	{ "fa", "Arab" },
	{ "fan", "Latn" },
	{ "fat", "Latn" },
	{ "ff", "Latn Adlm" },
	{ "fi", "Latn" },
	{ "fil", "Latn Tglg" },
	{ "fit", "Latn" },
	{ "fj", "Latn" },
	{ "fo", "Latn" },
	{ "fon", "Latn" },
	{ "fr", "Latn Dupl" },
	{ "frc", "Latn" },
	{ "frm", "Latn" },
	{ "fro", "Latn" },
	{ "frp", "Latn" },
	{ "frr", "Latn" },
	{ "frs", "Latn" },
	{ "fur", "Latn" },
	{ "fy", "Latn" },
	{ "ga", "Latn" },
	{ "gaa", "Latn" },
	{ "gag", "Latn Cyrl" },
	{ "gan", "Hans Hani" },
	{ "gay", "Latn" },
	{ "gba", "Latn" },
	{ "gbz", "Arab" },
	{ "gd", "Latn" },
	{ "gez", "Ethi" },
	{ "gil", "Latn" },
	{ "gl", "Latn" },
	{ "glk", "Arab" },
	{ "gmh", "Latn" },
	{ "gn", "Latn" },
	{ "goh", "Latn" },
	{ "gom", "Deav Latn Knda Mlym Arab" },
	{ "gon", "Deva Telu" },
	{ "gor", "Latn" },
	{ "got", "Goth" },
	{ "grb", "Latn" },
	{ "grc", "Grek" },
	{ "gsw", "Latn" },
	{ "gu", "Gujr" },
	{ "guc", "Latn" },
	{ "gur", "Latn" },
	{ "guz", "Latn" },
	{ "gv", "Latn" },
	{ "gwi", "Latn" },
	{ "ha", "Latn Arab" },
	{ "hai", "Latn" },
	{ "hak", "Hans Hant Hani" },
	{ "haw", "Latn" },
	{ "he", "Hebr" },
	{ "hi", "Deva Latn Mahj" },
	{ "hif", "Deva Latn" },
	{ "hil", "Latn" },
	{ "hit", "Xsux" },
	{ "hmn", "Latn Hmng" },
	{ "ho", "Latn" },
	{ "hne", "Deva" },
	{ "hr", "Latn" },
	{ "hsb", "Latn" },
	{ "hsn", "Hans Hani" },
	{ "ht", "Latn" },
	{ "hu", "Latn" },
	{ "hup", "Latn" },
	{ "hus", "Latn" },
	{ "hy", "Armn" },
	{ "hz", "Latn" },
	{ "ia", "Latn" },
	{ "iba", "Latn" },
	{ "ibb", "Latn" },
	{ "id", "Latn Arab" },
	{ "ie", "Latn" },
	{ "ig", "Latn" },
	{ "ii", "Yiii Latn" },
	{ "ik", "Latn" },
	{ "ilo", "Latn" },
	{ "inh", "Cyrl" },
	{ "io", "Latn" },
	{ "is", "Latn" },
	{ "it", "Latn" },
	{ "iu", "Cans Latn" },
	{ "izh", "Latn" },
	{ "ja", "Jpan Hani Hira Kana" },
	{ "jam", "Latn" },
	{ "jbo", "Latn" },
	{ "jgo", "Latn" },
	{ "jmc", "Latn" },
	{ "jpr", "Hebr" },
	{ "jrb", "Hebr" },
	{ "jut", "Latn" },
	{ "jv", "Latn Java" },
	{ "ka", "Geor" },
	{ "kaa", "Cyrl Latn" },
	{ "kab", "Latn" },
	{ "kac", "Latn" },
	{ "kaj", "Latn" },
	{ "kam", "Latn" },
	{ "kaw", "Bali Java Kawi" },
	{ "kbd", "Cyrl" },
	{ "kbl", "Arab" },
	{ "kcg", "Latn" },
	{ "kde", "Latn" },
	{ "kea", "Latn" },
	{ "ken", "Latn" },
	{ "kfo", "Latn" },
	{ "kg", "Latn" },
	{ "kgp", "Latn" },
	{ "kha", "Latn Beng" },
	{ "kho", "Brah Khar" },
	{ "khq", "Latn" },
	{ "khw", "Arab" },
	{ "ki", "Latn" },
	{ "kiu", "Latn" },
	{ "kj", "Latn" },
	{ "kk", "Cyrl Arab" },
	{ "kkj", "Latn" },
	{ "kl", "Latn" },
	{ "kln", "Latn" },
	{ "km", "Khmr" },
	{ "kmb", "Latn" },
	{ "kn", "Knda" },
	{ "ko", "Kore Hang Hani" },
	{ "koi", "Cyrl" },
	{ "kok", "Deva Latn" },
	{ "kos", "Latn" },
	{ "kpe", "Latn" },
	{ "kr", "Latn" },
	{ "krc", "Cyrl" },
	{ "kri", "Latn" },
	{ "krj", "Latn" },
	{ "krl", "Latn" },
	{ "kru", "Deva" },
	{ "ks", "Arab Deva" },
	{ "ksb", "Latn" },
	{ "ksf", "Latn" },
	{ "ksh", "Latn" },
	{ "ku", "Latn Arab Cyrl" },
	{ "kum", "Cyrl" },
	{ "kut", "Latn" },
	{ "kv", "Cyrl Perm" },
	{ "kw", "Latn" },
	{ "ky", "Cyrl Arab Latn" },
	{ "la", "Latn" },
	{ "lad", "Hebr" },
	{ "lag", "Latn" },
	{ "lah", "Arab" },
	{ "lam", "Latn" },
	{ "lb", "Latn" },
	{ "lez", "Cyrl Aghb" },
	{ "lfn", "Latn Cyrl" },
	{ "lg", "Latn" },
	{ "li", "Latn" },
	{ "lij", "Latn" },
	{ "liv", "Latn" },
	{ "lkt", "Latn" },
	{ "lmo", "Latn" },
	{ "ln", "Latn" },
	{ "lo", "Laoo" },
	{ "lol", "Latn" },
	{ "lou", "Latn" },
	{ "loz", "Latn" },
	{ "lrc", "Arab" },
	{ "lt", "Latn" },
	{ "ltg", "Latn" },
	{ "lu", "Latn" },
	{ "lua", "Latn" },
	{ "lui", "Latn" },
	{ "lun", "Latn" },
	{ "luo", "Latn" },
	{ "lus", "Beng" },
	{ "luy", "Latn" },
	{ "lv", "Latn" },
	{ "lzh", "Hant" },
	{ "lzz", "Latn Geor" },
	{ "mad", "Latn" },
	{ "maf", "Latn" },
	{ "mag", "Deva" },
	{ "mai", "Deva Tirh" },
	{ "mak", "Latn Bugi" },
	{ "man", "Latn Nkoo" },
	{ "mas", "Latn" },
	{ "mde", "Arab Latn" },
	{ "mdf", "Cyrl" },
	{ "mdr", "Latn Bugi" },
	{ "men", "Latn Mend" },
	{ "mer", "Latn" },
	{ "mfe", "Latn" },
	{ "mg", "Latn" },
	{ "mga", "Latn" },
	{ "mgh", "Latn" },
	{ "mgo", "Latn" },
	{ "mh", "Latn" },
	{ "mhr", "Cyrl" },
	{ "mi", "Latn" },
	{ "mic", "Latn" },
	{ "min", "Latn" },
	{ "miq", "Latn" },
	{ "mjw", "Latn" },
	{ "mk", "Cyrl" },
	{ "ml", "Mlym" },
	{ "mn", "Cyrl Mong Phag" },
	{ "mnc", "Mong" },
	{ "mni", "Beng Mtei" },
	{ "mnw", "Mymr" },
	{ "mos", "Latn" },
	{ "moh", "Latn" },
	{ "mr", "Deva Modi" },
	{ "mrj", "Cyrl" },
	{ "ms", "Latn Arab" },
	{ "mt", "Latn" },
	{ "mua", "Latn" },
	{ "mus", "Latn" },
	{ "mwl", "Latn" },
	{ "mwr", "Deva" },
	{ "mwv", "Latn" },
	{ "my", "Mymr" },
	{ "mye", "Latn" },
	{ "myv", "Cyrl" },
	{ "mzn", "Arab" },
	{ "na", "Latn" },
	{ "nah", "Latn" },
	{ "nan", "Hans Hant Hani" },
	{ "nap", "Latn" },
	{ "naq", "Latn" },
	{ "nan", "Latn Hans Hant Hani" },
	{ "nb", "Latn" },
	{ "nd", "Latn" },
	{ "nds", "Latn" },
	{ "ne", "Deva" },
	{ "new", "Deva" },
	{ "nhn", "Latn" },
	{ "ng", "Latn" },
	{ "nia", "Latn" },
	{ "niu", "Latn" },
	{ "njo", "Latn" },
	{ "nl", "Latn" },
	{ "nmg", "Latn" },
	{ "nn", "Latn" },
	{ "nnh", "Latn" },
	{ "nog", "Cyrl" },
	{ "non", "Runr" },
	{ "nov", "Latn" },
	{ "nqo", "Nkoo" },
	{ "nr", "Latn" },
	{ "nso", "Latn" },
	{ "nus", "Latn" },
	{ "nv", "Latn" },
	{ "nwc", "Ranj Newa Deva Sidd Brah" },
	{ "ny", "Latn" },
	{ "nym", "Latn" },
	{ "nyn", "Latn" },
	{ "nyo", "Latn" },
	{ "nzi", "Latn" },
	{ "oc", "Latn" },
	{ "oj", "Cans Latn" },
	{ "om", "Latn Ethi" },
	{ "or", "Orya" },
	{ "os", "Cyrl" },
	{ "osa", "Osge Latn" },
	{ "ota", "Arab" },
	{ "pa", "Guru Arab" },
	{ "pag", "Latn" },
	{ "pal", "Phli Phlp" },
	{ "pam", "Latn" },
	{ "pap", "Latn" },
	{ "pau", "Latn" },
	{ "pcd", "Latn" },
	{ "pcm", "Latn" },
	{ "pdc", "Latn" },
	{ "pdt", "Latn" },
	{ "peo", "Xpeo" },
	{ "pfl", "Latn" },
	{ "phn", "Phnx" },
	{ "pi", "Latn Deva Mymr Sinh Thai" },
	{ "pl", "Latn" },
	{ "pms", "Latn" },
	{ "pnt", "Grek Cyrl Latn" },
	{ "pon", "Latn" },
	{ "pr", "Latn" },
	{ "prg", "Latn" },
	{ "pro", "Latn" },
	{ "prs", "Arab" },
	{ "ps", "Arab" },
	{ "pt", "Latn" },
	{ "qu", "Latn" },
	{ "quc", "Latn" },
	{ "qug", "Latn" },
	{ "quy", "Latn" },
	{ "quz", "Latn" },
	{ "raj", "Deva" },
	{ "rap", "Latn" },
	{ "rar", "Latn" },
	{ "rgn", "Latn" },
	{ "rif", "Latn Tfng" },
	{ "rm", "Latn" },
	{ "rn", "Latn" },
	{ "ro", "Latn Cyrl" },
	{ "rof", "Latn" },
	{ "rom", "Latn Cyrl" },
	{ "rtm", "Latn" },
	{ "ru", "Cyrl" },
	{ "rue", "Cyrl" },
	{ "rug", "Latn" },
	{ "rup", "Latn" },
	{ "rw", "Latn" },
	{ "rwk", "Latn" },
	{ "sa", "Deva Gran Shrd Sidd Sinh" },
	{ "sad", "Latn" },
	{ "sah", "Cyrl" },
	{ "sam", "Samr Hebr" },
	{ "saq", "Latn" },
	{ "sas", "Latn" },
	{ "sat", "Olck Beng Deva Latn Orya" },
	{ "saz", "Saur" },
	{ "sba", "Latn" },
	{ "sbp", "Latn" },
	{ "sc", "Latn" },
	{ "scn", "Latn" },
	{ "sco", "Latn" },
	{ "sd", "Arab Deva Khoj Sind" },
	{ "sdc", "Latn" },
	{ "sdh", "Arab" },
	{ "se", "Latn Cyrl Arab" },
	{ "see", "Latn" },
	{ "seh", "Latn" },
	{ "sei", "Latn" },
	{ "sel", "Cyrl" },
	{ "ses", "Latn" },
	{ "sg", "Latn" },
	{ "sga", "Latn Ogam" },
	{ "sgs", "Latn" },
	{ "sh", "Latn Cyrl" },
	{ "shi", "Tfng Latn Arab" },
	{ "shn", "Mymr" },
	{ "shs", "Latn Dupl" },
	{ "shu", "Arab" },
	{ "si", "Sinh" },
	{ "sid", "Latn" },
	{ "sk", "Latn" },
	{ "sl", "Latn" },
	{ "sli", "Latn" },
	{ "sly", "Latn" },
	{ "sm", "Latn" },
	{ "sma", "Latn" },
	{ "smj", "Latn" },
	{ "smn", "Latn" },
	{ "sms", "Latn" },
	{ "sn", "Latn" },
	{ "snk", "Latn" },
	{ "so", "Latn Arab Osma" },
	{ "sog", "Sogd Syrc Mani" },
	{ "son", "Latn Arab" },
	{ "sq", "Latn Elba" },
	{ "sr", "Cyrl Latn" },
	{ "srn", "Latn" },
	{ "srr", "Latn" },
	{ "ss", "Latn" },
	{ "ssy", "Latn" },
	{ "st", "Latn" },
	{ "stq", "Latn" },
	{ "su", "Latn Sund" },
	{ "suk", "Latn" },
	{ "sus", "Latn Arab" },
	{ "sux", "Xsux" },
	{ "sv", "Latn" },
	{ "sw", "Latn" },
	{ "swb", "Arab Latn" },
	{ "swc", "Arab Latn" },
	{ "syc", "Syrc" },
	{ "syr", "Syrc" },
	{ "szl", "Latn" },
	{ "ta", "Taml" },
	{ "tcy", "Knda" },
	{ "te", "Telu" },
	{ "tem", "Latn" },
	{ "teo", "Latn" },
	{ "ter", "Latn" },
	{ "tet", "Latn" },
	{ "tg", "Cyrl Arab Latn" },
	{ "th", "Thai" },
	{ "the", "Deva" },
	{ "ti", "Ethi" },
	{ "tig", "Ethi" },
	{ "tiv", "Latn" },
	{ "tk", "Latn Arab Cyrl" },
	{ "tkl", "Latn" },
	{ "tkr", "Latn Cyrl" },
	{ "tl", "Latn Arab Blis" },
	{ "tlh", "Piqd Latn" },
	{ "tli", "Latn" },
	{ "tly", "Latn Arab Cyrl" },
	{ "tmh", "Latn" },
	{ "tn", "Latn" },
	{ "to", "Latn" },
	{ "tog", "Latn" },
	{ "tok", "Latn" },
	{ "tpi", "Latn" },
	{ "tr", "Latn Arab" },
	{ "tru", "Latn Syrc" },
	{ "trv", "Latn" },
	{ "ts", "Latn" },
	{ "tsd", "Grek" },
	{ "tsi", "Latn" },
	{ "tt", "Cyrl" },
	{ "ttt", "Latn Arab Cyrl" },
	{ "tum", "Latn" },
	{ "tvl", "Latn" },
	{ "tw", "Latn" },
	{ "twq", "Latn" },
	{ "ty", "Latn" },
	{ "tyv", "Cyrl" },
	{ "tzm", "Latn Tfng" },
	{ "udm", "Cyrl Latn" },
	{ "ug", "Arab Cyrl Latn" },
	{ "uga", "Ugar" },
	{ "uk", "Cyrl" },
	{ "umb", "Latn" },
	{ "unm", "Latn" },
	{ "ur", "Arab" },
	{ "uz", "Latn Cyrl Arab" },
	{ "vai", "Vaii Latn" },
	{ "ve", "Latn" },
	{ "vec", "Latn" },
	{ "vep", "Latn" },
	{ "vi", "Latn Hani" },
	{ "vls", "Latn" },
	{ "vmf", "Latn" },
	{ "vo", "Latn" },
	{ "vot", "Latn" },
	{ "vro", "Latn" },
	{ "vun", "Latn" },
	{ "wa", "Latn" },
	{ "wae", "Latn" },
	{ "wal", "Ethi" },
	{ "war", "Latn" },
	{ "was", "Latn" },
	{ "wbp", "Latn" },
	{ "wo", "Latn Arab" },
	{ "wuu", "Hans Hani" },
	{ "xal", "Cyrl" },
	{ "xh", "Latn" },
	{ "xmf", "Geor" },
	{ "xog", "Latn" },
	{ "yao", "Latn" },
	{ "yap", "Latn" },
	{ "yav", "Latn" },
	{ "ybb", "Latn" },
	{ "yi", "Hebr" },
	{ "yo", "Latn" },
	{ "yrl", "Latn" },
	{ "yue", "Hant Hans Hani" },
	{ "yuw", "Latn" },
	{ "za", "Latn Hans Hani" },
	{ "zap", "Latn" },
	{ "zbl", "Blis" },
	{ "zea", "Latn" },
	{ "zen", "Tfng" },
	{ "zgh", "Tfng" },
	{ "zh", "Hans Hant Hani Bopo Latn Phag" },
	{ "zu", "Latn" },
	{ "zun", "Latn" },
	{ "zza", "Latn" },
	{ nullptr, nullptr },
};

// Additional regional variants.
// Variant name, supported languages.

static const char *locale_variants[][2] = {
	{ "valencia", "ca" },
	{ "iqtelif", "tt" },
	{ "saaho", "aa" },
	{ "tradnl", "es" },
	{ nullptr, nullptr },
};

// Script names and codes (excludes typographic variants, special codes, reserved codes and aliases for combined scripts).
// Reference:
// - https://en.wikipedia.org/wiki/ISO_15924

static const char *script_list[][2] = {
	{ "Adlam", "Adlm" },
	{ "Afaka", "Afak" },
	{ "Caucasian Albanian", "Aghb" },
	{ "Ahom", "Ahom" },
	{ "Arabic", "Arab" },
	{ "Imperial Aramaic", "Armi" },
	{ "Armenian", "Armn" },
	{ "Avestan", "Avst" },
	{ "Balinese", "Bali" },
	{ "Bamum", "Bamu" },
	{ "Bassa Vah", "Bass" },
	{ "Batak", "Batk" },
	{ "Bengali", "Beng" },
	{ "Bhaiksuki", "Bhks" },
	{ "Blissymbols", "Blis" },
	{ "Bopomofo", "Bopo" },
	{ "Brahmi", "Brah" },
	{ "Braille", "Brai" },
	{ "Buginese", "Bugi" },
	{ "Buhid", "Buhd" },
	{ "Chakma", "Cakm" },
	{ "Unified Canadian Aboriginal", "Cans" },
	{ "Carian", "Cari" },
	{ "Cham", "Cham" },
	{ "Cherokee", "Cher" },
	{ "Chorasmian", "Chrs" },
	{ "Cirth", "Cirt" },
	{ "Coptic", "Copt" },
	{ "Cypro-Minoan", "Cpmn" },
	{ "Cypriot", "Cprt" },
	{ "Cyrillic", "Cyrl" },
	{ "Devanagari", "Deva" },
	{ "Dives Akuru", "Diak" },
	{ "Dogra", "Dogr" },
	{ "Deseret", "Dsrt" },
	{ "Duployan", "Dupl" },
	{ "Egyptian demotic", "Egyd" },
	{ "Egyptian hieratic", "Egyh" },
	{ "Egyptian hieroglyphs", "Egyp" },
	{ "Elbasan", "Elba" },
	{ "Elymaic", "Elym" },
	{ "Ethiopic", "Ethi" },
	{ "Khutsuri", "Geok" },
	{ "Georgian", "Geor" },
	{ "Glagolitic", "Glag" },
	{ "Gunjala Gondi", "Gong" },
	{ "Masaram Gondi", "Gonm" },
	{ "Gothic", "Goth" },
	{ "Grantha", "Gran" },
	{ "Greek", "Grek" },
	{ "Gujarati", "Gujr" },
	{ "Gurmukhi", "Guru" },
	{ "Hangul", "Hang" },
	{ "Han", "Hani" },
	{ "Hanunoo", "Hano" },
	{ "Simplified Han", "Hans" },
	{ "Traditional Han", "Hant" },
	{ "Hatran", "Hatr" },
	{ "Hebrew", "Hebr" },
	{ "Hiragana", "Hira" },
	{ "Anatolian Hieroglyphs", "Hluw" },
	{ "Pahawh Hmong", "Hmng" },
	{ "Nyiakeng Puachue Hmong", "Hmnp" },
	{ "Old Hungarian", "Hung" },
	{ "Indus", "Inds" },
	{ "Old Italic", "Ital" },
	{ "Javanese", "Java" },
	{ "Jurchen", "Jurc" },
	{ "Kayah Li", "Kali" },
	{ "Katakana", "Kana" },
	{ "Kawi", "Kawi" },
	{ "Kharoshthi", "Khar" },
	{ "Khmer", "Khmr" },
	{ "Khojki", "Khoj" },
	{ "Khitan large script", "Kitl" },
	{ "Khitan small script", "Kits" },
	{ "Kannada", "Knda" },
	{ "Kpelle", "Kpel" },
	{ "Kaithi", "Kthi" },
	{ "Tai Tham", "Lana" },
	{ "Lao", "Laoo" },
	{ "Latin", "Latn" },
	{ "Leke", "Leke" },
	{ "Lepcha", "Lepc" },
	{ "Limbu", "Limb" },
	{ "Linear A", "Lina" },
	{ "Linear B", "Linb" },
	{ "Lisu", "Lisu" },
	{ "Loma", "Loma" },
	{ "Lycian", "Lyci" },
	{ "Lydian", "Lydi" },
	{ "Mahajani", "Mahj" },
	{ "Makasar", "Maka" },
	{ "Mandaic", "Mand" },
	{ "Manichaean", "Mani" },
	{ "Marchen", "Marc" },
	{ "Mayan Hieroglyphs", "Maya" },
	{ "Medefaidrin", "Medf" },
	{ "Mende Kikakui", "Mend" },
	{ "Meroitic Cursive", "Merc" },
	{ "Meroitic Hieroglyphs", "Mero" },
	{ "Malayalam", "Mlym" },
	{ "Modi", "Modi" },
	{ "Mongolian", "Mong" },
	{ "Moon", "Moon" },
	{ "Mro", "Mroo" },
	{ "Meitei Mayek", "Mtei" },
	{ "Multani", "Mult" },
	{ "Myanmar / Burmese", "Mymr" },
	{ "Nag Mundari", "Nagm" },
	{ "Nandinagari", "Nand" },
	{ "Old North Arabian", "Narb" },
	{ "Nabataean", "Nbat" },
	{ "Newa", "Newa" },
	{ "Naxi Dongba", "Nkdb" },
	{ "Nakhi Geba", "Nkgb" },
	{ "N'ko", "Nkoo" },
	{ "Nüshu", "Nshu" },
	{ "Ogham", "Ogam" },
	{ "Ol Chiki", "Olck" },
	{ "Old Turkic", "Orkh" },
	{ "Oriya", "Orya" },
	{ "Osage", "Osge" },
	{ "Osmanya", "Osma" },
	{ "Old Uyghur", "Ougr" },
	{ "Palmyrene", "Palm" },
	{ "Pau Cin Hau", "Pauc" },
	{ "Proto-Cuneiform", "Pcun" },
	{ "Proto-Elamite", "Pelm" },
	{ "Old Permic", "Perm" },
	{ "Phags-pa", "Phag" },
	{ "Inscriptional Pahlavi", "Phli" },
	{ "Psalter Pahlavi", "Phlp" },
	{ "Book Pahlavi", "Phlv" },
	{ "Phoenician", "Phnx" },
	{ "Klingon", "Piqd" },
	{ "Miao", "Plrd" },
	{ "Inscriptional Parthian", "Prti" },
	{ "Proto-Sinaitic", "Psin" },
	{ "Ranjana", "Ranj" },
	{ "Rejang", "Rjng" },
	{ "Hanifi Rohingya", "Rohg" },
	{ "Rongorongo", "Roro" },
	{ "Runic", "Runr" },
	{ "Samaritan", "Samr" },
	{ "Sarati", "Sara" },
	{ "Old South Arabian", "Sarb" },
	{ "Saurashtra", "Saur" },
	{ "SignWriting", "Sgnw" },
	{ "Shavian", "Shaw" },
	{ "Sharada", "Shrd" },
	{ "Shuishu", "Shui" },
	{ "Siddham", "Sidd" },
	{ "Khudawadi", "Sind" },
	{ "Sinhala", "Sinh" },
	{ "Sogdian", "Sogd" },
	{ "Old Sogdian", "Sogo" },
	{ "Sora Sompeng", "Sora" },
	{ "Soyombo", "Soyo" },
	{ "Sundanese", "Sund" },
	{ "Syloti Nagri", "Sylo" },
	{ "Syriac", "Syrc" },
	{ "Tagbanwa", "Tagb" },
	{ "Takri", "Takr" },
	{ "Tai Le", "Tale" },
	{ "New Tai Lue", "Talu" },
	{ "Tamil", "Taml" },
	{ "Tangut", "Tang" },
	{ "Tai Viet", "Tavt" },
	{ "Telugu", "Telu" },
	{ "Tengwar", "Teng" },
	{ "Tifinagh", "Tfng" },
	{ "Tagalog", "Tglg" },
	{ "Thaana", "Thaa" },
	{ "Thai", "Thai" },
	{ "Tibetan", "Tibt" },
	{ "Tirhuta", "Tirh" },
	{ "Tangsa", "Tnsa" },
	{ "Toto", "Toto" },
	{ "Ugaritic", "Ugar" },
	{ "Vai", "Vaii" },
	{ "Visible Speech", "Visp" },
	{ "Vithkuqi", "Vith" },
	{ "Warang Citi", "Wara" },
	{ "Wancho", "Wcho" },
	{ "Woleai", "Wole" },
	{ "Old Persian", "Xpeo" },
	{ "Cuneiform", "Xsux" },
	{ "Yezidi", "Yezi" },
	{ "Yi", "Yiii" },
	{ "Zanabazar Square", "Zanb" },
	{ nullptr, nullptr }
};

// Plural rules.
// Reference:
// - https://cgit.git.savannah.gnu.org/cgit/gettext.git/tree/gettext-tools/src/plural-table.c
// - https://github.com/unicode-org/cldr/blob/release-47/common/supplemental/plurals.xml
static const char *plural_rules[][2] = {
	{ "bm bo dz hnj id ig ii in ja jbo jv jw kde kea km ko lkt lo ms my nqo osa root sah ses sg su th to tpi vi wo yo yue zh", "nplurals=1; plural=0;" },
	{ "af an asa ast az bal bem bez bg brx ca ce cgg chr ckb da de dv ee el en eo es et eu fi fo fur fy gl gsw ha haw he hu ia io it jgo ji jmc ka kaj kcg kk kkj kl ks ksb ku ky lb lg lij mas mgo ml mn mr nah nb nd ne nl nn nnh no nr ny nyn om or os pap ps pt rm rof rwk saq sc sd sdh seh sn so sq ss ssy st sv sw syr ta te teo tig tk tn tr ts ug ur uz ve vo vun wae xh xog yi", "nplurals=2; plural=(n != 1);" },
	{ "ak bho csw ff fr guw hy kab ln mg nso pa pt_BR si ti wa", "nplurals=2; plural=(n > 1);" },
	{ "is mk", "nplurals=2; plural=(n%10==1 && n%100!=11);" },
	{ "tzm", "nplurals=2; plural=(n<=1 || (n>=11 && n<=99));" },
	{ "am as bn doi fa gu hi kn pcm zu", "nplurals=2; plural=(n==0 || n==1);" },
	{ "ceb fil tl", "nplurals=2; plural=(n==1 || n==2 || n==3 || (n%10!=4 && n%10!=6 && n%10!=9));" },
	{ "prg", "nplurals=3; plural=(n%10==0 || (n%100>=11 && n%100<=19) ? 0 : n%10==1 && n%100!=11 ? 1 : 2);" },
	{ "lv", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2);" },
	{ "lt", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2);" },
	{ "be hr ru sr uk", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
	{ "bs sh", "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<12 || n%100>14) ? 1 : 2);" },
	{ "lag", "nplurals=3; plural=(n==0 ? 0 : (n==0 || n==1) && n!=0 ? 1 : 2);" },
	{ "blo ksh", "nplurals=3; plural=(n==0 ? 0 : n==1 ? 1 : 2);" },
	{ "shi", "nplurals=3; plural=(n==0 || n==1 ? 0 : n>=2 && n<=10 ? 1 : 2);" },
	{ "pl", "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
	{ "mo", "nplurals=3; plural=(n==1 ? 0 : n==0 || (n!=1 && n%100>=1 && n%100<=19) ? 1 : 2);" },
	{ "iu iw naq sat se sma smi smj smn sms", "nplurals=3; plural=(n==1 ? 0 : n==2 ? 1 : 2);" },
	{ "cs sk", "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" },
	{ "ro", "nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;" },
	{ "ga", "nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;" },
	{ "sl", "nplurals=4; plural=(n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3);" },
	{ "dsb hsb", "nplurals=4; plural=(n%100==1 ? 0 : n%100==2 ? 1 : n%100>=3 && n%100<=4 ? 2 : 3);" },
	{ "gv", "nplurals=4; plural=(n%10==1 ? 0 : n%10==2 ? 1 : n%100==0 || n%100==20 || n%100==40 || n%100==60 || n%100==80 ? 2 : 3);" },
	{ "gd", "nplurals=4; plural=(n==1 || n==11 ? 0 : n==2 || n==12 ? 1 : (n>=3 && n<=10) || (n>=13 && n<=19) ? 2 : 3);" },
	{ "br", "nplurals=5; plural=(n%10==1 && n%100!=11 && n%100!=71 && n%100!=91 ? 0 : n%10==2 && n%100!=12 && n%100!=72 && n%100!=92 ? 1 : ((n%10>=3 && n%10<=4) || n%10==9) && (n%100<10 || n%100>19) && (n%100<70 || n%100>79) && (n%100<90 || n%100>99) ? 2 : n!=0 && n%1000000==0 ? 3 : 4);" },
	{ "mt", "nplurals=5; plural=(n==1 ? 0 : n==2 ? 1 : n==0 || (n%100>=3 && n%100<=10) ? 2 : n%100>=11 && n%100<=19 ? 3 : 4);" },
	{ "kw", "nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n%100==2 || n%100==22 || n%100==42 || n%100==62 || n%100==82 || (n%1000==0 && ((n%100000>=1000 && n%100000<=20000) || n%100000==40000 || n%100000==60000 || n%100000==80000)) || (n!=0 && n%1000000==100000) ? 2 : n%100==3 || n%100==23 || n%100==43 || n%100==63 || n%100==83 ? 3 : n!=1 && (n%100==1 || n%100==21 || n%100==41 || n%100==61 || n%100==81) ? 4 : 5);" },
	{ "ar ars", "nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5);" },
	{ "cy", "nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n==3 ? 3 : n==6 ? 4 : 5);" },
	{ nullptr, nullptr }
};

static const struct NumSystemData {
	const char *locales;
	const char32_t *digits;
	const char32_t *percent_sign;
	const char32_t *exp_l;
	const char32_t *exp_u;
} num_system_data[] = {
	// Eastern Arabic numerals.
	{ "ar ar_AE ar_BH ar_DJ ar_EG ar_ER ar_IL ar_IQ ar_JO ar_KM ar_KW ar_LB ar_MR ar_OM ar_PS ar_QA ar_SA ar_SD ar_SO ar_SS ar_SY ar_TD ar_YE ckb ckb_IQ ckb_IR sd sd_PK sd_Arab sd_Arab_PK", U"٠١٢٣٤٥٦٧٨٩٫", U"٪", U"اس", U"اس" },
	// Persian and Urdu numerals.
	{ "fa fa_AF fa_IR ks ks_IN ks_Arab ks_Arab_IN lrc lrc_IQ lrc_IR mzn mzn_IR pa_PK pa_Arab pa_Arab_PK ps ps_AF ps_PK ur_IN uz_AF uz_Arab uz_Arab_AF", U"۰۱۲۳۴۵۶۷۸۹٫", U"٪", U"اس", U"اس" },
	// Bengali numerals.
	{ "as as_IN bn bn_BD bn_IN mni mni_IN mni_Beng mni_Beng_IN", U"০১২৩৪৫৬৭৮৯.", U"%", U"e", U"E" },
	// Devanagari numerals.
	{ "mr mr_IN ne ne_IN ne_NP sa sa_IN", U"०१२३४५६७८९.", U"%", U"e", U"E" },
	// Dzongkha numerals.
	{ "dz dz_BT", U"༠༡༢༣༤༥༦༧༨༩.", U"%", U"e", U"E" },
	// Santali numerals.
	{ "sat sat_IN sat_Olck sat_Olck_IN", U"᱐᱑᱒᱓᱔᱕᱖᱗᱘᱙.", U"%", U"e", U"E" },
	// Burmese numerals.
	{ "my my_MM", U"၀၁၂၃၄၅၆၇၈၉.", U"%", U"e", U"E" },
	// Chakma numerals.
	{ "ccp ccp_BD ccp_IN", U"𑄶𑄷𑄸𑄹𑄺𑄻𑄼𑄽𑄾𑄿.", U"%", U"e", U"E" },
	// Adlam numerals.
	{ "ff ff_Adlm_BF ff_Adlm_CM ff_Adlm_GH ff_Adlm_GM ff_Adlm_GN ff_Adlm_GW ff_Adlm_LR ff_Adlm_MR ff_Adlm_NE ff_Adlm_NG ff_Adlm_SL ff_Adlm_SN", U"𞥐𞥑𞥒𞥓𞥔𞥕𞥖𞥗𞥘𞥙.", U"%", U"𞤉", U"𞤉" },
	// End sentinel.
	{ nullptr, nullptr, nullptr, nullptr, nullptr }
};

// --- End core/string/locales.h ---
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

// ============================================================================
// Merged from core/string/node_path.cpp
// ============================================================================

#include "core/types.h"
#include "node_path.h"

void NodePath::_update_hash_cache() const
{
	String path = get_concatenated_names();
	String subpath = get_concatenated_subnames();
	uint32_t hash =
		HashMapHasherDefault::hash(Pair<const String&, const String&>(path, subpath));
	data->hash_cache = is_absolute() ? hash : ~hash;
	data->hash_cache_valid = true;
}

void NodePath::prepend_period()
{
	if (data->path.size() && data->path[0] != ".") {
		_copy_on_write();
		data->path.insert(0, ".");
		data->concatenated_path = String();
		data->hash_cache_valid = false;
	}
}

bool NodePath::is_absolute() const
{
	if (!data) {
		return false;
	}

	return data->absolute;
}

int NodePath::get_name_count() const
{
	if (!data) {
		return 0;
	}

	return data->path.size();
}

String NodePath::get_name(int p_idx) const
{
	ERR_FAIL_NULL_V(data, String());
	ERR_FAIL_INDEX_V(p_idx, data->path.size(), String());
	return data->path[p_idx];
}

int NodePath::get_subname_count() const
{
	if (!data) {
		return 0;
	}

	return data->subpath.size();
}

String NodePath::get_subname(int p_idx) const
{
	ERR_FAIL_NULL_V(data, String());
	ERR_FAIL_INDEX_V(p_idx, data->subpath.size(), String());
	return data->subpath[p_idx];
}

int NodePath::get_total_name_count() const
{
	if (!data) {
		return 0;
	}

	return data->path.size() + data->subpath.size();
}

void NodePath::unref()
{
	if (data && data->refcount.unref()) {
		memdelete(data);
	}
	data = nullptr;
}

bool NodePath::operator==(const NodePath& p_path) const
{
	if (data == p_path.data) {
		return true;
	}

	if (!data || !p_path.data) {
		return false;
	}

	if (data->hash_cache_valid && p_path.data->hash_cache_valid) {
		if (data->hash_cache != p_path.data->hash_cache) {
			return false;
		}
	}

	if (data->absolute != p_path.data->absolute) {
		return false;
	}

	int path_size = data->path.size();

	if (path_size != p_path.data->path.size()) {
		return false;
	}

	int subpath_size = data->subpath.size();

	if (subpath_size != p_path.data->subpath.size()) {
		return false;
	}

	const String* l_path_ptr = data->path.ptr();
	const String* r_path_ptr = p_path.data->path.ptr();

	for (int i = 0; i < path_size; i++) {
		if (l_path_ptr[i] != r_path_ptr[i]) {
			return false;
		}
	}

	const String* l_subpath_ptr = data->subpath.ptr();
	const String* r_subpath_ptr = p_path.data->subpath.ptr();

	for (int i = 0; i < subpath_size; i++) {
		if (l_subpath_ptr[i] != r_subpath_ptr[i]) {
			return false;
		}
	}

	return true;
}

bool NodePath::operator!=(const NodePath& p_path) const { return (!(*this == p_path)); }

void NodePath::operator=(const NodePath& p_path)
{
	if (this == &p_path) {
		return;
	}

	unref();

	if (p_path.data && p_path.data->refcount.ref()) {
		data = p_path.data;
	}
}

void NodePath::_copy_on_write()
{
	if (!data) {
		return; // No data; nothing to do.
	}

	if (data->refcount.get() == 1) {
		return; // Already the only owner of data.
	}

	// Make a copy.
	Data* new_data = memnew(Data);
	new_data->refcount.init();
	new_data->path = data->path;
	new_data->subpath = data->subpath;
	new_data->concatenated_path = data->concatenated_path;
	new_data->concatenated_subpath = data->concatenated_subpath;
	new_data->absolute = data->absolute;
	new_data->hash_cache_valid = data->hash_cache_valid;
	new_data->hash_cache = data->hash_cache;
	unref();
	data = new_data;
}

NodePath::operator String() const
{
	if (!data) {
		return String();
	}

	String ret;
	if (data->absolute) {
		ret = "/";
	}

	ret += get_concatenated_names();

	String subpath = get_concatenated_subnames();
	if (!subpath.is_empty()) {
		ret += ":" + subpath;
	}

	return ret;
}

Vector<String> NodePath::get_names() const
{
	if (data) {
		return data->path;
	}
	return Vector<String>();
}

Vector<String> NodePath::get_subnames() const
{
	if (data) {
		return data->subpath;
	}
	return Vector<String>();
}

String NodePath::get_concatenated_names() const
{
	ERR_FAIL_NULL_V(data, String());

	if (data->concatenated_path.is_empty()) {
		int pc = data->path.size();
		String concatenated;
		const String* sn = data->path.ptr();
		for (int i = 0; i < pc; i++) {
			if (i > 0) {
				concatenated += "/";
			}
			concatenated += sn[i];
		}
		data->concatenated_path = concatenated;
	}
	return data->concatenated_path;
}

String NodePath::get_concatenated_subnames() const
{
	ERR_FAIL_NULL_V(data, String());

	if (data->concatenated_subpath.is_empty()) {
		int spc = data->subpath.size();
		String concatenated;
		const String* ssn = data->subpath.ptr();
		for (int i = 0; i < spc; i++) {
			if (i > 0) {
				concatenated += ":";
			}
			concatenated += ssn[i];
		}
		data->concatenated_subpath = concatenated;
	}
	return data->concatenated_subpath;
}

NodePath NodePath::slice(int p_begin, int p_end) const
{
	const int name_count = get_name_count();
	const int total_count = get_total_name_count();

	int begin = CLAMP(p_begin, -total_count, total_count);
	if (begin < 0) {
		begin += total_count;
	}
	int end = CLAMP(p_end, -total_count, total_count);
	if (end < 0) {
		end += total_count;
	}
	const int sub_begin = MAX(begin - name_count, 0);
	const int sub_end = MAX(end - name_count, 0);

	const Vector<String> names = get_names().slice(begin, end);
	const Vector<String> sub_names = get_subnames().slice(sub_begin, sub_end);
	const bool absolute = is_absolute() && (begin == 0);
	return NodePath(names, sub_names, absolute);
}

NodePath NodePath::rel_path_to(const NodePath& p_np) const
{
	ERR_FAIL_COND_V(!is_absolute(), NodePath());
	ERR_FAIL_COND_V(!p_np.is_absolute(), NodePath());

	Vector<String> src_dirs = get_names();
	Vector<String> dst_dirs = p_np.get_names();

	// find common parent
	int common_parent = 0;

	while (true) {
		if (src_dirs.size() == common_parent) {
			break;
		}
		if (dst_dirs.size() == common_parent) {
			break;
		}
		if (src_dirs[common_parent] != dst_dirs[common_parent]) {
			break;
		}
		common_parent++;
	}

	common_parent--;

	Vector<String> relpath;
	relpath.resize(src_dirs.size() + dst_dirs.size() + 1);

	String* relpath_ptr = relpath.ptrw();

	int path_size = 0;
	String back_str("..");
	for (int i = common_parent + 1; i < src_dirs.size(); i++) {
		relpath_ptr[path_size++] = back_str;
	}

	for (int i = common_parent + 1; i < dst_dirs.size(); i++) {
		relpath_ptr[path_size++] = dst_dirs[i];
	}

	if (path_size == 0) {
		relpath_ptr[path_size++] = ".";
	}

	relpath.resize(path_size);

	return NodePath(relpath, p_np.get_subnames(), false);
}

NodePath NodePath::get_as_property_path() const
{
	if (!data || !data->path.size()) {
		return *this;
	}
	else {
		Vector<String> new_path = data->subpath;

		String initial_subname = data->path[0];

		for (int i = 1; i < data->path.size(); i++) {
			initial_subname += "/" + data->path[i];
		}
		new_path.insert(0, initial_subname);

		return NodePath(Vector<String>(), new_path, false);
	}
}

bool NodePath::is_empty() const { return !data; }

void NodePath::simplify()
{
	if (!data) {
		return;
	}
	_copy_on_write();
	for (int i = 0; i < data->path.size(); i++) {
		if (data->path.size() == 1) {
			break;
		}
		if (data->path[i] == ".") {
			data->path.remove_at(i);
			i--;
		}
		else if (i > 0 && data->path[i] == ".." && data->path[i - 1] != "." &&
				   data->path[i - 1] != "..") {
			// remove both
			data->path.remove_at(i - 1);
			data->path.remove_at(i - 1);
			i -= 2;
			if (data->path.is_empty()) {
				data->path.push_back(".");
				break;
			}
		}
	}
	data->concatenated_path = String();
	data->hash_cache_valid = false;
}

NodePath NodePath::simplified() const
{
	NodePath np = *this;
	np.simplify();
	return np;
}

NodePath::NodePath(const Vector<String>& p_path, bool p_absolute)
{
	if (p_path.is_empty() && !p_absolute) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = p_absolute;
	data->path = p_path;
	data->hash_cache_valid = false;
}

NodePath::NodePath(
	const Vector<String>& p_path, const Vector<String>& p_subpath, bool p_absolute)
{
	if (p_path.is_empty() && p_subpath.is_empty() && !p_absolute) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = p_absolute;
	data->path = p_path;
	data->subpath = p_subpath;
	data->hash_cache_valid = false;
}

NodePath::NodePath(const NodePath& p_path)
{
	if (p_path.data && p_path.data->refcount.ref()) {
		data = p_path.data;
	}
}

NodePath::NodePath(const String& p_path)
{
	if (p_path.length() == 0) {
		return;
	}

	String path = p_path;
	Vector<String> subpath;

	bool absolute = (path[0] == '/');
	bool last_is_slash = true;
	int slices = 0;
	int subpath_pos = path.find_char(':');

	if (subpath_pos != -1) {
		int from = subpath_pos + 1;

		for (int i = from; i <= path.length(); i++) {
			if (path[i] == ':' || path[i] == 0) {
				String str = path.substr(from, i - from);
				if (str.is_empty()) {
					if (path[i] == 0) {
						continue; // Allow end-of-path :
					}

					ERR_FAIL_MSG(vformat("Invalid NodePath '%s'.", p_path));
				}
				subpath.push_back(str);

				from = i + 1;
			}
		}

		path = path.substr(0, subpath_pos);
	}

	for (int i = (int)absolute; i < path.length(); i++) {
		if (path[i] == '/') {
			last_is_slash = true;
		}
		else {
			if (last_is_slash) {
				slices++;
			}

			last_is_slash = false;
		}
	}

	if (slices == 0 && !absolute && !subpath.size()) {
		return;
	}

	data = memnew(Data);
	data->refcount.init();
	data->absolute = absolute;
	data->subpath = subpath;
	data->hash_cache_valid = false;

	if (slices == 0) {
		return;
	}
	data->path.resize(slices);
	last_is_slash = true;
	int from = (int)absolute;
	int slice = 0;

	for (int i = (int)absolute; i < path.length() + 1; i++) {
		if (path[i] == '/' || path[i] == 0) {
			if (!last_is_slash) {
				String name = path.substr(from, i - from);
				ERR_FAIL_INDEX(slice, data->path.size());
				data->path.write[slice++] = name;
			}
			from = i + 1;
			last_is_slash = true;
		}
		else {
			last_is_slash = false;
		}
	}
}

NodePath::~NodePath() { unref(); }

// ==========================================================================
// print_string.cpp
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

#include <cstdio>
#include "core/core_globals.h"
#include "core/os/os.h"
#include "print_string.h"

static PrintHandlerList* print_handler_list = nullptr;
static thread_local bool is_printing = false;

static void __print_fallback(const String& p_string, bool p_err, bool p_reentrance)
{
	if (p_reentrance) {
		fprintf(p_err ? stderr : stdout,
			"While attempting to print an error, another error was printed:\n");
	}

	fprintf(p_err ? stderr : stdout, "%s\n", p_string.utf8().get_data());
}

void add_print_handler(PrintHandlerList* p_handler)
{
	_global_lock();
	p_handler->next = print_handler_list;
	print_handler_list = p_handler;
	_global_unlock();
}

void remove_print_handler(const PrintHandlerList* p_handler)
{
	_global_lock();

	PrintHandlerList* prev = nullptr;
	PrintHandlerList* l = print_handler_list;

	while (l) {
		if (l == p_handler) {
			if (prev) {
				prev->next = l->next;
			}
			else {
				print_handler_list = l->next;
			}
			break;
		}
		prev = l;
		l = l->next;
	}
	// OS::print("print handler list is %p\n",print_handler_list);

	_global_unlock();
	ERR_FAIL_NULL(l);
}

void __print_line(const String& p_string)
{
	if (!CoreGlobals::print_line_enabled) {
		return;
	}

	if (!CoreGlobals::print_ready) {
		__print_fallback(p_string, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(p_string, false, true);
		return;
	}

	is_printing = true;

	OS::print("%s\n", p_string.utf8().get_data());

	_global_lock();
	PrintHandlerList* l = print_handler_list;
	while (l) {
		l->printfunc(l->userdata, p_string, false, false);
		l = l->next;
	}

	_global_unlock();

	is_printing = false;
}

void __print_line_rich(const String& p_string)
{
	if (!CoreGlobals::print_line_enabled) {
		return;
	}

	// Convert a subset of BBCode tags to ANSI escape codes for correct display in the terminal.
	// Support of those ANSI escape codes varies across terminal emulators,
	// especially for italic and strikethrough.

	String output;
	int pos = 0;
	bool in_named_url = false;
	while (pos <= p_string.length()) {
		int brk_pos = p_string.find_char('[', pos);

		if (brk_pos < 0) {
			brk_pos = p_string.length();
		}

		String txt = brk_pos > pos ? p_string.substr(pos, brk_pos - pos) : "";
		if (brk_pos == p_string.length()) {
			output += txt;
			break;
		}

		int brk_end = p_string.find_char(']', brk_pos + 1);

		if (brk_end == -1) {
			txt += p_string.substr(brk_pos);
			output += txt;
			break;
		}
		pos = brk_end + 1;
		output += txt;

		String tag = p_string.substr(brk_pos + 1, brk_end - brk_pos - 1);
		if (tag == "b") {
			output += "\u001b[1m";
		}
		else if (tag == "/b") {
			output += "\u001b[22m";
		}
		else if (tag == "i") {
			output += "\u001b[3m";
		}
		else if (tag == "/i") {
			output += "\u001b[23m";
		}
		else if (tag == "u") {
			output += "\u001b[4m";
		}
		else if (tag == "/u") {
			output += "\u001b[24m";
		}
		else if (tag == "s") {
			output += "\u001b[9m";
		}
		else if (tag == "/s") {
			output += "\u001b[29m";
		}
		else if (tag == "indent") {
			output += "    ";
		}
		else if (tag == "/indent") {
			output += "";
		}
		else if (tag == "code") {
			output += "\u001b[2m";
		}
		else if (tag == "/code") {
			output += "\u001b[22m";
		}
		else if (tag.begins_with("url=")) {
			// Support named URLs using OSC 8 escape codes:
			// <https://gist.github.com/egmontkob/eb114294efbcd5adb1944c9f3cb5feda>
			in_named_url = true;
			const String url_link = tag.substr(strlen("url="));
			output += vformat("\u001b]8;;%s\u001b\\", url_link);
		}
		else if (tag == "url") {
			output += "";
		}
		else if (tag == "/url") {
			if (in_named_url) {
				output += "\u001b]8;;\u001b\\";
				in_named_url = false;
			}
			else {
				// While it's legal to close an URL that was never opened using OSC 8 escape codes,
				// it would result in the code being printed to unsupported terminal emulators
				// when using unnamed URLs.
				output += "";
			}
		}
		else if (tag == "center") {
			output += "\n\t\t\t";
		}
		else if (tag == "/center") {
			output += "";
		}
		else if (tag == "right") {
			output += "\n\t\t\t\t\t\t";
		}
		else if (tag == "/right") {
			output += "";
		}
		else if (tag.begins_with("color=")) {
			String color_name = tag.trim_prefix("color=");
			if (color_name == "black") {
				output += "\u001b[30m";
			}
			else if (color_name == "red") {
				output += "\u001b[91m";
			}
			else if (color_name == "green") {
				output += "\u001b[92m";
			}
			else if (color_name == "lime") {
				output += "\u001b[92m";
			}
			else if (color_name == "yellow") {
				output += "\u001b[93m";
			}
			else if (color_name == "blue") {
				output += "\u001b[94m";
			}
			else if (color_name == "magenta") {
				output += "\u001b[95m";
			}
			else if (color_name == "pink") {
				output += "\u001b[38;5;218m";
			}
			else if (color_name == "purple") {
				output += "\u001b[38;5;98m";
			}
			else if (color_name == "cyan") {
				output += "\u001b[96m";
			}
			else if (color_name == "white") {
				output += "\u001b[97m";
			}
			else if (color_name == "orange") {
				output += "\u001b[38;5;208m";
			}
			else if (color_name == "gray") {
				output += "\u001b[90m";
			}
			else {
				Color c = Color::from_string(color_name, Color());
				output += vformat("\u001b[38;2;%d;%d;%dm", c.r * 255, c.g * 255, c.b * 255);
			}
		}
		else if (tag == "/color") {
			output += "\u001b[39m";
		}
		else if (tag.begins_with("bgcolor=")) {
			String color_name = tag.trim_prefix("bgcolor=");
			if (color_name == "black") {
				output += "\u001b[40m";
			}
			else if (color_name == "red") {
				output += "\u001b[101m";
			}
			else if (color_name == "green") {
				output += "\u001b[102m";
			}
			else if (color_name == "lime") {
				output += "\u001b[102m";
			}
			else if (color_name == "yellow") {
				output += "\u001b[103m";
			}
			else if (color_name == "blue") {
				output += "\u001b[104m";
			}
			else if (color_name == "magenta") {
				output += "\u001b[105m";
			}
			else if (color_name == "pink") {
				output += "\u001b[48;5;218m";
			}
			else if (color_name == "purple") {
				output += "\u001b[48;5;98m";
			}
			else if (color_name == "cyan") {
				output += "\u001b[106m";
			}
			else if (color_name == "white") {
				output += "\u001b[107m";
			}
			else if (color_name == "orange") {
				output += "\u001b[48;5;208m";
			}
			else if (color_name == "gray") {
				output += "\u001b[100m";
			}
			else {
				Color c = Color::from_string(color_name, Color());
				output += vformat("\u001b[48;2;%d;%d;%dm", c.r * 255, c.g * 255, c.b * 255);
			}
		}
		else if (tag == "/bgcolor") {
			output += "\u001b[49m";
		}
		else if (tag.begins_with("fgcolor=")) {
			String color_name = tag.trim_prefix("fgcolor=");
			if (color_name == "black") {
				output += "\u001b[30;40m";
			}
			else if (color_name == "red") {
				output += "\u001b[91;101m";
			}
			else if (color_name == "green") {
				output += "\u001b[92;102m";
			}
			else if (color_name == "lime") {
				output += "\u001b[92;102m";
			}
			else if (color_name == "yellow") {
				output += "\u001b[93;103m";
			}
			else if (color_name == "blue") {
				output += "\u001b[94;104m";
			}
			else if (color_name == "magenta") {
				output += "\u001b[95;105m";
			}
			else if (color_name == "pink") {
				output += "\u001b[38;5;218;48;5;218m";
			}
			else if (color_name == "purple") {
				output += "\u001b[38;5;98;48;5;98m";
			}
			else if (color_name == "cyan") {
				output += "\u001b[96;106m";
			}
			else if (color_name == "white") {
				output += "\u001b[97;107m";
			}
			else if (color_name == "orange") {
				output += "\u001b[38;5;208;48;5;208m";
			}
			else if (color_name == "gray") {
				output += "\u001b[90;100m";
			}
			else {
				Color c = Color::from_string(color_name, Color());
				output += vformat("\u001b[38;2;%d;%d;%d;48;2;%d;%d;%dm", c.r * 255, c.g * 255,
					c.b * 255, c.r * 255, c.g * 255, c.b * 255);
			}
		}
		else if (tag == "/fgcolor") {
			output += "\u001b[39;49m";
		}
		else {
			output += "[";
			pos = brk_pos + 1;
		}
	}
	output += "\u001b[0m"; // Reset.

	if (!CoreGlobals::print_ready) {
		__print_fallback(output, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(output, false, true);
		return;
	}

	is_printing = true;

	OS::print_rich("%s\n", output.utf8().get_data());

	_global_lock();
	PrintHandlerList* l = print_handler_list;
	while (l) {
		l->printfunc(l->userdata, p_string, false, true);
		l = l->next;
	}

	_global_unlock();

	is_printing = false;
}

void print_raw(const String& p_string)
{
	if (!CoreGlobals::print_ready) {
		__print_fallback(p_string, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(p_string, true, true);
		return;
	}

	is_printing = true;

	OS::print("%s", p_string.utf8().get_data());

	is_printing = false;
}

void print_error(const String& p_string)
{
	if (!CoreGlobals::print_error_enabled) {
		return;
	}

	if (!CoreGlobals::print_ready) {
		__print_fallback(p_string, false, false);
		return;
	}

	if (is_printing) {
		__print_fallback(p_string, true, true);
		return;
	}

	is_printing = true;

	OS::printerr("%s\n", p_string.utf8().get_data());

	_global_lock();
	PrintHandlerList* l = print_handler_list;
	while (l) {
		l->printfunc(l->userdata, p_string, true, false);
		l = l->next;
	}

	_global_unlock();

	is_printing = false;
}

bool is_print_verbose_enabled() { return OS::is_stdout_verbose(); }

// ==========================================================================
// string_builder.cpp
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

#include "string_builder.h"

StringBuilder &StringBuilder::append(const String &p_string) {
	if (p_string.is_empty()) {
		return *this;
	}

	strings.push_back(p_string);
	appended_strings.push_back(-1);

	string_length += p_string.length();

	return *this;
}

StringBuilder &StringBuilder::append(const char *p_cstring) {
	int32_t len = strlen(p_cstring);

	c_strings.push_back(p_cstring);
	appended_strings.push_back(len);

	string_length += len;

	return *this;
}

String StringBuilder::as_string() const {
	if (string_length == 0) {
		return "";
	}

	String string;
	string.resize_uninitialized(string_length + 1);
	char32_t *buffer = string.ptrw();

	int current_position = 0;

	int godot_string_elem = 0;
	int c_string_elem = 0;

	for (uint32_t i = 0; i < appended_strings.size(); i++) {
		const int32_t str_len = appended_strings[i];

		if (str_len == -1) {
			// Godot string
			const String &s = strings[godot_string_elem];

			memcpy(buffer + current_position, s.ptr(), s.length() * sizeof(char32_t));

			current_position += s.length();

			godot_string_elem++;
		} else {
			const char *s = c_strings[c_string_elem];

			for (int32_t j = 0; j < str_len; j++) {
				buffer[current_position + j] = s[j];
			}

			current_position += str_len;

			c_string_elem++;
		}
	}
	buffer[current_position] = 0;

	return string;
}

// ==========================================================================
// fuzzy_search.cpp
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

#include "core/string/translation_server.h"

static const String boundary_chars = "/\\-_. ";

static bool _is_valid_interval(const Vector2i& p_interval)
{
	// Empty intervals are represented as (-1, -1).
	return p_interval.x >= 0 && p_interval.y >= p_interval.x;
}

static Vector2i _extend_interval(const Vector2i& p_a, const Vector2i& p_b)
{
	if (!_is_valid_interval(p_a)) {
		return p_b;
	}
	if (!_is_valid_interval(p_b)) {
		return p_a;
	}
	return Vector2i(MIN(p_a.x, p_b.x), MAX(p_a.y, p_b.y));
}

static bool _is_word_boundary(const String& p_str, int p_index)
{
	if (p_index == -1 || p_index == p_str.size()) {
		return true;
	}
	return boundary_chars.find_char(p_str[p_index]) != -1;
}

bool FuzzySearchToken::try_exact_match(
	FuzzyTokenMatch& p_match, const String& p_target, int p_offset) const
{
	p_match.token_idx = idx;
	p_match.token_length = string.length();
	int match_idx = p_target.find(string, p_offset);
	if (match_idx == -1) {
		return false;
	}
	p_match.add_substring(match_idx, string.length());
	return true;
}

bool FuzzySearchToken::try_fuzzy_match(
	FuzzyTokenMatch& p_match, const String& p_target, int p_offset, int p_miss_budget) const
{
	p_match.token_idx = idx;
	p_match.token_length = string.length();
	int run_start = -1;
	int run_len = 0;

	// Search for the subsequence p_token in p_target starting from p_offset, recording each
	// substring for later scoring and display.
	for (int i = 0; i < string.length(); i++) {
		int new_offset = p_target.find_char(string[i], p_offset);
		if (new_offset < 0) {
			p_miss_budget--;
			if (p_miss_budget < 0) {
				return false;
			}
		}
		else {
			if (run_start == -1 || p_offset != new_offset) {
				if (run_start != -1) {
					p_match.add_substring(run_start, run_len);
				}
				run_start = new_offset;
				run_len = 1;
			}
			else {
				run_len += 1;
			}
			p_offset = new_offset + 1;
		}
	}

	if (run_start != -1) {
		p_match.add_substring(run_start, run_len);
	}

	return true;
}

void FuzzyTokenMatch::add_substring(int p_substring_start, int p_substring_length)
{
	substrings.append(Vector2i(p_substring_start, p_substring_length));
	matched_length += p_substring_length;
	Vector2i substring_interval = {p_substring_start, p_substring_start + p_substring_length - 1};
	interval = _extend_interval(interval, substring_interval);
}

bool FuzzyTokenMatch::intersects(const Vector2i& p_other_interval) const
{
	if (!_is_valid_interval(interval) || !_is_valid_interval(p_other_interval)) {
		return false;
	}
	return interval.y >= p_other_interval.x && interval.x <= p_other_interval.y;
}

bool FuzzySearchMatch::_can_add_token_match(const FuzzyTokenMatch& p_match) const
{
	if (p_match.get_miss_count() > miss_budget) {
		return false;
	}

	if (p_match.intersects(match_interval)) {
		if (token_matches.size() == 1) {
			return false;
		}
		for (const FuzzyTokenMatch& existing_match : token_matches) {
			if (existing_match.intersects(p_match.interval)) {
				return false;
			}
		}
	}

	return true;
}

bool FuzzyTokenMatch::is_case_insensitive(const String& p_original, const String& p_adjusted) const
{
	for (const Vector2i& substr : substrings) {
		const int end = substr.x + substr.y;
		for (int i = substr.x; i < end; i++) {
			if (p_original[i] != p_adjusted[i]) {
				return true;
			}
		}
	}
	return false;
}

void FuzzySearchMatch::_score_token_match(FuzzyTokenMatch& p_match, bool p_case_insensitive) const
{
	// This can always be tweaked more. The intuition is that exact matches should almost always
	// be prioritized over broken up matches, and other criteria more or less act as tie breakers.

	p_match.score = -20 * p_match.get_miss_count() - (p_case_insensitive ? 3 : 0);

	for (const Vector2i& substring : p_match.substrings) {
		// Score longer substrings higher than short substrings.
		int substring_score = substring.y * substring.y;
		// Score matches deeper in path higher than shallower matches
		if (substring.x > dir_index) {
			substring_score *= 2;
		}
		// Score matches on a word boundary higher than matches within a word
		if (_is_word_boundary(target, substring.x - 1) ||
			_is_word_boundary(target, substring.x + substring.y)) {
			substring_score += 4;
		}
		// Score exact query matches higher than non-compact subsequence matches
		if (substring.y == p_match.token_length) {
			substring_score += 100;
		}
		p_match.score += substring_score;
	}
}

void FuzzySearchMatch::_maybe_apply_token_order_score_bonus()
{
	// This adds a small bonus to results which match tokens in the same order they appear in the
	// query.
	if (token_matches.is_empty()) {
		return;
	}

	int* token_range_starts = (int*)alloca(sizeof(int) * token_matches.size());

	for (const FuzzyTokenMatch& match : token_matches) {
		token_range_starts[match.token_idx] = match.interval.x;
	}

	for (int i = 1; i < token_matches.size(); i++) {
		// Individual tokens can match without a range if the missed-character budget allows for it.
		// If the i'th token matches in this manner, skip ahead so we check neither (i-1, i) nor (i,
		// i+1). It's safe that this skips i=0 since any valid start will be > -1.
		if (token_range_starts[i] == -1) {
			i++;
			continue;
		}
		if (token_range_starts[i - 1] > token_range_starts[i]) {
			return;
		}
	}

	score += 1;
}

void FuzzySearchMatch::_add_token_match(const FuzzyTokenMatch& p_match)
{
	score += p_match.score;
	match_interval = _extend_interval(match_interval, p_match.interval);
	miss_budget -= p_match.get_miss_count();
	token_matches.append(p_match);
}

static void remove_low_scores(Vector<Ref<FuzzySearchMatch>>& p_results, float p_cull_score)
{
	// Removes all results with score < p_cull_score in-place.
	int i = 0;
	int j = p_results.size() - 1;
	Ref<FuzzySearchMatch>* results = p_results.ptrw();

	while (true) {
		// Advances i to an element to remove and j to an element to keep.
		while (j >= i && results[j]->get_score() < p_cull_score) {
			j--;
		}
		while (i < j && results[i]->get_score() >= p_cull_score) {
			i++;
		}
		if (i >= j) {
			break;
		}
		results[i++] = results[j--];
	}

	p_results.resize(j + 1);
}

Vector<FuzzySearchToken> FuzzySearch::_get_tokens(const String& p_query) const
{
	Vector<FuzzySearchToken> tokens;

	for (const String& string : p_query.split(" ", false)) {
		tokens.append({
			static_cast<int>(tokens.size()),
			case_sensitive ? string : string.to_lower(),
		});
	}

	struct TokenComparator
	{
		bool operator()(const FuzzySearchToken& p_left, const FuzzySearchToken& p_right) const
		{
			if (p_left.string.length() == p_right.string.length()) {
				return p_left.idx < p_right.idx;
			}
			return p_left.string.length() > p_right.string.length();
		}
	};

	// Prioritize matching longer tokens before shorter ones since match overlaps are not accepted.
	tokens.sort_custom<TokenComparator>();
	return tokens;
}

void FuzzySearch::_sort_and_filter(Vector<Ref<FuzzySearchMatch>>& p_results) const
{
	if (p_results.is_empty()) {
		return;
	}

	if (filter_low_scores) {
		float avg_score = 0;
		float max_score = 0;

		for (const Ref<FuzzySearchMatch>& result : p_results) {
			avg_score += result->get_score();
			max_score = MAX(max_score, result->get_score());
		}

		avg_score /= p_results.size();
		float cull_score = MIN(filter_cutoff, Math::lerp(avg_score, max_score, filter_factor));
		remove_low_scores(p_results, cull_score);
	}

	struct FuzzySearchResultComparator
	{
		bool operator()(
			const Ref<FuzzySearchMatch>& p_lhs, const Ref<FuzzySearchMatch>& p_rhs) const
		{
			// Sort on (score, length, alphanumeric) to ensure consistent ordering.
			if (p_lhs->score == p_rhs->score) {
				if (p_lhs->target.length() == p_rhs->target.length()) {
					return p_lhs->target < p_rhs->target;
				}
				return p_lhs->target.length() < p_rhs->target.length();
			}
			return p_lhs->score > p_rhs->score;
		}
	};

	SortArray<Ref<FuzzySearchMatch>, FuzzySearchResultComparator> sorter;

	if (p_results.size() > max_results) {
		sorter.partial_sort(0, p_results.size(), max_results, p_results.ptrw());
		p_results.resize(max_results);
	}
	else {
		sorter.sort(p_results.ptrw(), p_results.size());
	}
}

void FuzzySearch::set_case_sensitive(bool p_case_sensitive) { case_sensitive = p_case_sensitive; }

bool FuzzySearch::_search_tokens(const Vector<FuzzySearchToken>& p_tokens, const String& p_target,
	Ref<FuzzySearchMatch>& r_result) const
{
	r_result->target = p_target;
	r_result->dir_index = p_target.rfind_char('/');
	r_result->miss_budget = max_misses;
	r_result->token_matches.reserve(p_tokens.size());

	String adjusted_target = case_sensitive ? p_target : p_target.to_lower();

	// For each token, eagerly generate subsequences starting from index 0 and keep the best scoring
	// one which does not conflict with prior token matches. This is not ensured to find the highest
	// scoring combination of matches, or necessarily the highest scoring single subsequence, as it
	// only considers eager subsequences for a given index, and likewise eagerly finds matches for
	// each token in sequence.
	for (const FuzzySearchToken& token : p_tokens) {
		FuzzyTokenMatch best_match;
		int offset = start_offset;

		while (true) {
			FuzzyTokenMatch match;
			if (exact_token_matches) {
				if (!token.try_exact_match(match, adjusted_target, offset)) {
					break;
				}
			}
			else {
				if (!token.try_fuzzy_match(match, adjusted_target, offset, r_result->miss_budget)) {
					break;
				}
			}
			if (r_result->_can_add_token_match(match)) {
				r_result->_score_token_match(
					match, match.is_case_insensitive(p_target, adjusted_target));
				if (best_match.token_idx == -1 || best_match.score < match.score) {
					best_match = match;
				}
			}
			if (_is_valid_interval(match.interval)) {
				offset = match.interval.x + 1;
			}
			else {
				break;
			}
		}

		if (best_match.token_idx == -1) {
			return false;
		}

		r_result->_add_token_match(best_match);
	}

	if (r_result->match_interval.x == -1) {
		// Reject matches which rely entirely on misses.
		return false;
	}

	r_result->_maybe_apply_token_order_score_bonus();
	return true;
}

Ref<FuzzySearchMatch> FuzzySearch::search(const String& p_query, const String& p_target) const
{
	Ref<FuzzySearchMatch> result;
	result.instantiate();
	if (_search_tokens(_get_tokens(p_query), p_target, result)) {
		return result;
	}
	return nullptr;
}

Vector<Ref<FuzzySearchMatch>> FuzzySearch::search_all(
	const String& p_query, const PackedStringArray& p_targets) const
{
	Vector<Ref<FuzzySearchMatch>> results;
	const Vector<FuzzySearchToken> tokens = _get_tokens(p_query);

	for (int i = 0; i < p_targets.size(); i++) {
		Ref<FuzzySearchMatch> result;
		result.instantiate();
		result->original_index = i;
		if (_search_tokens(tokens, p_targets[i], result)) {
			results.append(result);
		}
	}

	_sort_and_filter(results);
	return results;
}
