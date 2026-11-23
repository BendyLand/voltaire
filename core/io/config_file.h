/**************************************************************************/
/*  config_file.h                                                         */
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

#include "core/io/file_access.h"
#include "core/templates/hash_map.h"

class ConfigFile : public RefCounted
{
	Error _internal_load(const String& p_path, Ref<FileAccess> p_file);
	Error _internal_save(Ref<FileAccess> p_file);

	Error _parse(const String& p_path);

public:
	bool has_section(const String& p_section) const;
	bool has_section_key(const String& p_section, const String& p_key) const;

	Vector<String> get_sections() const;
	Vector<String> get_section_keys(const String& p_section) const;

	void erase_section(const String& p_section);
	void erase_section_key(const String& p_section, const String& p_key);

	Error save(const String& p_path);
	Error load(const String& p_path);
	Error parse(const String& p_data);

	String encode_to_text() const; // used by exporter

	void clear();

	Error load_encrypted(const String& p_path, const Vector<uint8_t>& p_key);
	Error load_encrypted_pass(const String& p_path, const String& p_pass);

	Error save_encrypted(const String& p_path, const Vector<uint8_t>& p_key);
	Error save_encrypted_pass(const String& p_path, const String& p_pass);
};

// --- Folded from core/io/json_resource_format.h ---
#include "core/io/resource_loader.h"
#include "core/io/resource_uid.h"

class ResourceFormatLoaderJSON : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	// Treat JSON as a text file, do not generate a `*.json.uid` file.
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceUID::INVALID_ID;
	}

	virtual bool has_custom_uid_support() const override { return true; }
};

class ResourceFormatSaverJSON : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
};



// --- End core/io/json_resource_format.h ---

// --- Folded from core/io/plist.h ---
// Property list file format (application/x-plist) parser, property list ASN-1 serialization.

#include "core/io/file_access.h"

class PListNode;

class PList : public RefCounted
{
	friend class PListNode;

public:
	enum PLNodeType
	{
		PL_NODE_TYPE_NIL,
		PL_NODE_TYPE_STRING,
		PL_NODE_TYPE_ARRAY,
		PL_NODE_TYPE_DICT,
		PL_NODE_TYPE_BOOLEAN,
		PL_NODE_TYPE_INTEGER,
		PL_NODE_TYPE_REAL,
		PL_NODE_TYPE_DATA,
		PL_NODE_TYPE_DATE,
	};

private:
	struct PListTrailer
	{
		uint8_t offset_size;
		uint8_t ref_size;
		uint64_t object_num;
		uint64_t root_offset_idx;
		uint64_t offset_table_start;
	};

	PListTrailer trailer;
	Ref<PListNode> root;

	uint64_t read_bplist_var_size_int(Ref<FileAccess> p_file, uint8_t p_size);
	Ref<PListNode> read_bplist_obj(Ref<FileAccess> p_file, uint64_t p_offset_idx);

public:
	PList();
	PList(const String& p_string);

	bool load_file(const String& p_filename);
	bool load_string(const String& p_string, String& r_err_out);

	Vector<uint8_t> save_asn1() const;
	String save_text() const;

	Ref<PListNode> get_root();
};

/*************************************************************************/

class PListNode : public RefCounted
{
	static int _asn1_size_len(uint8_t p_len_octets);

public:
	PList::PLNodeType data_type = PList::PLNodeType::PL_NODE_TYPE_NIL;

	CharString data_string;
	Vector<Ref<PListNode>> data_array;
	HashMap<String, Ref<PListNode>> data_dict;

	union
	{
		int64_t data_int;
		bool data_bool;
		double data_real;
	};

	PList::PLNodeType get_type() const;

	static Ref<PListNode> new_array();
	static Ref<PListNode> new_dict();
	static Ref<PListNode> new_string(const String& p_string);
	static Ref<PListNode> new_data(const String& p_string);
	static Ref<PListNode> new_date(const String& p_string);
	static Ref<PListNode> new_bool(bool p_bool);
	static Ref<PListNode> new_int(int64_t p_int);
	static Ref<PListNode> new_real(double p_real);

	bool push_subnode(const Ref<PListNode>& p_node, const String& p_key = "");

	size_t get_asn1_size(uint8_t p_len_octets) const;

	void store_asn1_size(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const;
	bool store_asn1(Vector<uint8_t>& p_stream, uint8_t p_len_octets) const;
	void store_text(String& p_stream, uint8_t p_indent) const;
};



// --- End core/io/plist.h ---

// --- Folded from core/io/translation_loader_po.h ---
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"

class TranslationLoaderPO : public ResourceFormatLoader
{
public:
	static Ref<Resource> load_translation(Ref<FileAccess> r_file, Error* r_error = nullptr);
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	// Treat translations as text/binary files, do not generate a `*.{po,mo}.uid` file.
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceUID::INVALID_ID;
	}

	virtual bool has_custom_uid_support() const override { return true; }
};



// --- End core/io/translation_loader_po.h ---

// --- Folded from core/io/json.h ---
#include "core/io/resource.h"

class JSON : public Resource
{
	enum TokenType
	{
		TK_CURLY_BRACKET_OPEN,
		TK_CURLY_BRACKET_CLOSE,
		TK_BRACKET_OPEN,
		TK_BRACKET_CLOSE,
		TK_IDENTIFIER,
		TK_STRING,
		TK_NUMBER,
		TK_COLON,
		TK_COMMA,
		TK_EOF,
		TK_MAX
	};

	enum Expecting
	{
		EXPECT_OBJECT,
		EXPECT_OBJECT_KEY,
		EXPECT_COLON,
		EXPECT_OBJECT_VALUE,
	};

	struct Token
	{
		TokenType type;
	};

	String text;
	String err_str;
	int err_line = 0;

	static const char* tk_name[];

	static void _add_indent(String& r_result, const String& p_indent, int p_size);
	static Error _get_token(const char32_t* p_str, int& r_index, int p_len, Token& r_token,
		int& r_line, String& r_err_str);

protected:

public:
	Error parse(const String& p_json_string, bool p_keep_text = false);
	String get_parsed_text() const;

	_FORCE_INLINE_ int get_error_line() const { return err_line; }

	_FORCE_INLINE_ String get_error_message() const { return err_str; }
};
// --- End core/io/json.h ---

// --- Folded from core/io/xml_parser.h ---
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/types.h"

/*
  Based on irrXML (see their zlib license). Added mainly for compatibility with their Collada
  loader.
*/

class XMLParser : public RefCounted
{
public:
	//! Enumeration of all supported source text file formats
	enum SourceFormat
	{
		SOURCE_ASCII,
		SOURCE_UTF8,
		SOURCE_UTF16_BE,
		SOURCE_UTF16_LE,
		SOURCE_UTF32_BE,
		SOURCE_UTF32_LE
	};

	enum NodeType
	{
		NODE_NONE,
		NODE_ELEMENT,
		NODE_ELEMENT_END,
		NODE_TEXT,
		NODE_COMMENT,
		NODE_CDATA,
		NODE_UNKNOWN
	};

private:
	char* data_copy = nullptr;
	const char* data = nullptr;
	const char* P = nullptr;
	uint64_t length = 0;
	uint64_t current_line = 0;
	String node_name;
	bool node_empty = false;
	NodeType node_type = NODE_NONE;
	uint64_t node_offset = 0;

	struct Attribute
	{
		String name;
		String value;
	};

	Vector<Attribute> attributes;

	bool _set_text(const char* p_start, const char* p_end);
	void _parse_closing_xml_element();
	void _ignore_definition();
	bool _parse_cdata();
	void _parse_comment();
	void _parse_opening_xml_element();
	void _parse_current_node();

	_FORCE_INLINE_ void next_char()
	{
		if (*P == '\n') {
			current_line++;
		}
		P++;
	}


public:
	Error read();
	NodeType get_node_type();
	String get_node_name() const;
	String get_node_data() const;
	uint64_t get_node_offset() const;
	int get_attribute_count() const;
	String get_attribute_name(int p_idx) const;
	String get_attribute_value(int p_idx) const;
	bool has_attribute(const String& p_name) const;
	String get_named_attribute_value(const String& p_name) const;
	String get_named_attribute_value_safe(
		const String& p_name) const; // do not print error if doesn't exist
	bool is_empty() const;
	int get_current_line() const;

	void skip_section();
	Error seek(uint64_t p_pos);

	Error open(const String& p_path);
	Error open_buffer(const Vector<uint8_t>& p_buffer);
	Error _open_buffer(const uint8_t* p_buffer, size_t p_size);

	void close();

	~XMLParser();
};
// --- End core/io/xml_parser.h ---
