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
