/**************************************************************************/
/*  resource_uid.h                                                        */
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

#include "core/types.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"

class FileAccess;

typedef void (*ResourceUIDScanForUIDOnStartup)();

class ResourceUID final
{
public:
	typedef int64_t ID;
	constexpr const static ID INVALID_ID = -1;

	static String get_cache_file();

private:
	struct Cache
	{
		CharString cs;
		bool saved_to_cache = false;
	};

	struct Data
	{
		Mutex mutex;
		HashMap<ID, Cache> unique_ids; // Unique IDs and utf8 paths (less memory used).
		bool use_reverse_cache = false;
		HashMap<CharString, ID> reverse_cache; // Used at runtime.
		uint32_t cache_entries = 0;
		bool changed = false;
		bool cache_initialized = false;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	inline static ResourceUIDScanForUIDOnStartup scan_for_uid_on_startup = nullptr;

	static String id_to_text(ID p_id);
	static ID text_to_id(const String& p_text);

	static ID create_id();
	static ID create_id_for_path(const String& p_path);
	static bool has_id(ID p_id);
	static void add_id(ID p_id, const String& p_path);
	static void set_id(ID p_id, const String& p_path);
	static String get_id_path(ID p_id);
	static ID get_path_id(const String& p_path);
	static void remove_id(ID p_id);

	static String uid_to_path(const String& p_uid);
	static String path_to_uid(const String& p_path);
	static String ensure_path(const String& p_uid_or_path);

	static Error load_from_cache(bool p_reset);
	static Error save_to_cache();
	static Error update_cache();
	static String get_path_from_cache(Ref<FileAccess>& p_cache_file, const String& p_uid_string);
	static Vector<uint8_t> encode_binary_cache(const Vector<Pair<ID, String>>& p_entries);

	static void enable_reverse_cache()
	{
		if (data) {
			data->use_reverse_cache = true;
		}
	}

	static void clear();

	ResourceUID() = delete;
	ResourceUID(const ResourceUID&) = delete;
	ResourceUID& operator=(const ResourceUID&) = delete;
	~ResourceUID() = delete;
};
