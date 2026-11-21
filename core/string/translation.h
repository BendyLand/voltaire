/**************************************************************************/
/*  translation.h                                                         */
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
