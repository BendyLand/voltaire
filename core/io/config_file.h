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
#include "core/io/resource_saver.h"
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
