/**************************************************************************/
/*  resource_format_binary.cpp                                            */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access_compressed.h"
#include "core/io/missing_resource.h"
#include "core/version.h"
#include "resource_format_binary.h"
#include "scene/property_utils.h"
#include "scene/resources/packed_scene.h"

// #define print_bl(m_what) print_line(m_what)
#define print_bl(m_what) (void)(m_what)

enum
{
	// numbering must be different from variant, in case new variant types are added (variant must
	// be always contiguous for jumptable optimization)
	VARIANT_NIL = 1,
	VARIANT_BOOL = 2,
	VARIANT_INT = 3,
	VARIANT_FLOAT = 4,
	VARIANT_STRING = 5,
	VARIANT_VECTOR2 = 10,
	VARIANT_RECT2 = 11,
	VARIANT_VECTOR3 = 12,
	VARIANT_PLANE = 13,
	VARIANT_QUATERNION = 14,
	VARIANT_AABB = 15,
	VARIANT_BASIS = 16,
	VARIANT_TRANSFORM3D = 17,
	VARIANT_TRANSFORM2D = 18,
	VARIANT_COLOR = 20,
	VARIANT_NODE_PATH = 22,
	VARIANT_RID = 23,
	VARIANT_OBJECT = 24,
	VARIANT_INPUT_EVENT = 25,
	VARIANT_DICTIONARY = 26,
	VARIANT_ARRAY = 30,
	VARIANT_PACKED_BYTE_ARRAY = 31,
	VARIANT_PACKED_INT32_ARRAY = 32,
	VARIANT_PACKED_FLOAT32_ARRAY = 33,
	VARIANT_PACKED_STRING_ARRAY = 34,
	VARIANT_PACKED_VECTOR3_ARRAY = 35,
	VARIANT_PACKED_COLOR_ARRAY = 36,
	VARIANT_PACKED_VECTOR2_ARRAY = 37,
	VARIANT_INT64 = 40,
	VARIANT_DOUBLE = 41,
	VARIANT_CALLABLE = 42,
	VARIANT_SIGNAL = 43,
	VARIANT_STRING_NAME = 44,
	VARIANT_VECTOR2I = 45,
	VARIANT_RECT2I = 46,
	VARIANT_VECTOR3I = 47,
	VARIANT_PACKED_INT64_ARRAY = 48,
	VARIANT_PACKED_FLOAT64_ARRAY = 49,
	VARIANT_VECTOR4 = 50,
	VARIANT_VECTOR4I = 51,
	VARIANT_PROJECTION = 52,
	VARIANT_PACKED_VECTOR4_ARRAY = 53,
	OBJECT_EMPTY = 0,
	OBJECT_EXTERNAL_RESOURCE = 1,
	OBJECT_INTERNAL_RESOURCE = 2,
	OBJECT_EXTERNAL_RESOURCE_INDEX = 3,
	// Version 2: Added 64-bit support for float and int.
	// Version 3: Changed NodePath encoding.
	// Version 4: New string ID for ext/subresources, breaks forward compat.
	// Version 5: Ability to store script class in the header.
	// Version 6: Added PackedVector4Array Variant type.
	FORMAT_VERSION = 6,
	FORMAT_VERSION_CAN_RENAME_DEPS = 1,
	FORMAT_VERSION_NO_NODEPATH_PROPERTY = 3,
};

void ResourceLoaderBinary::_advance_padding(uint32_t p_len)
{
	uint32_t extra = 4 - (p_len % 4);
	if (extra < 4) {
		for (uint32_t i = 0; i < extra; i++) {
			f->get_8(); // pad to 32
		}
	}
}

static Error read_reals(real_t* r_dst, Ref<FileAccess>& r_file, size_t p_count)
{
	if (r_file->real_is_double) {
		if constexpr (sizeof(real_t) == 8) {
			// Ideal case with double-precision
			r_file->get_buffer((uint8_t*)r_dst, p_count * sizeof(double));
		}
		else if constexpr (sizeof(real_t) == 4) {
			// May be slower, but this is for compatibility. Eventually the data should be
			// converted.
			for (size_t i = 0; i < p_count; ++i) {
				r_dst[i] = r_file->get_double();
			}
		}
		else {
			ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "real_t size is neither 4 nor 8!");
		}
	}
	else {
		if constexpr (sizeof(real_t) == 4) {
			// Ideal case with float-precision
			r_file->get_buffer((uint8_t*)r_dst, p_count * sizeof(float));
		}
		else if constexpr (sizeof(real_t) == 8) {
			for (size_t i = 0; i < p_count; ++i) {
				r_dst[i] = r_file->get_float();
			}
		}
		else {
			ERR_FAIL_V_MSG(ERR_UNAVAILABLE, "real_t size is neither 4 nor 8!");
		}
	}
	return OK;
}

StringName ResourceLoaderBinary::_get_string()
{
	uint32_t id = f->get_32();
	if (id & 0x80000000) {
		uint32_t len = id & 0x7FFFFFFF;
		if ((int)len > str_buf.size()) {
			str_buf.resize(len);
		}
		if (len == 0) {
			return StringName();
		}
		f->get_buffer((uint8_t*)&str_buf[0], len);
		return String::utf8(&str_buf[0], len);
	}

	return string_map[id];
}

Ref<Resource> ResourceLoaderBinary::get_resource() { return resource; }

Error ResourceLoaderBinary::load()
{
	if (error != OK) {
		return error;
	}

	for (int i = 0; i < external_resources.size(); i++) {
		String path = external_resources[i].path;

		if (remaps.has(path)) {
			path = remaps[path];
		}

		if (!path.contains("://") && path.is_relative_path()) {
			// path is relative to file being loaded, so convert to a resource path
			path = ProjectSettings::get_singleton()->localize_path(
				path.get_base_dir().path_join(external_resources[i].path));
		}

		external_resources.write[i].path =
			path; // remap happens here, not on load because on load it can actually be used for
				  // filesystem dock resource remap
		external_resources.write[i].load_token =
			ResourceLoader::_load_start(path, external_resources[i].type,
				use_sub_threads ? ResourceLoader::LOAD_THREAD_DISTRIBUTE
								: ResourceLoader::LOAD_THREAD_FROM_CURRENT,
				cache_mode_for_external);
		if (external_resources[i].load_token.is_null()) {
			if (!ResourceLoader::get_abort_on_missing_resources()) {
				ResourceLoader::notify_dependency_error(
					local_path, path, external_resources[i].type);
			}
			else {
				error = ERR_FILE_MISSING_DEPENDENCIES;
				ERR_FAIL_V_MSG(error, vformat("Can't load dependency: '%s'.", path));
			}
		}
	}

	for (int i = 0; i < internal_resources.size(); i++) {
		bool main = i == (internal_resources.size() - 1);

		// maybe it is loaded already
		String path;
		String id;

		if (!main) {
			path = internal_resources[i].path;

			if (path.begins_with("local://")) {
				path = path.replace_first("local://", "");
				id = path;
				path = res_path + "::" + path;

				internal_resources.write[i].path = path; // Update path.
			}

			if (cache_mode == ResourceFormatLoader::CACHE_MODE_REUSE && ResourceCache::has(path)) {
				Ref<Resource> cached = ResourceCache::get_ref(path);
				if (cached.is_valid()) {
					// already loaded, don't do anything
					error = OK;
					internal_index_cache[path] = cached;
					continue;
				}
			}
		}
		else {
			if (cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE &&
				!ResourceCache::has(res_path)) {
				path = res_path;
			}
		}

		uint64_t offset = internal_resources[i].offset;

		f->seek(offset);

		String t = get_unicode_string();

		Ref<Resource> res;
		Resource* r = nullptr;

		Ref<MissingResource> missing_resource;

		if (main) {
			res = ResourceLoader::get_resource_ref_override(local_path);
			r = res.ptr();
		}
		if (!r) {
			if (cache_mode == ResourceFormatLoader::CACHE_MODE_REPLACE &&
				ResourceCache::has(path)) {
			}
			if (res.is_null()) {
				if (!r) {
					error = ERR_FILE_CORRUPT;
				}
				res = Ref<Resource>(r);
			}
		}

		if (r) {
			if (!path.is_empty()) {
				if (cache_mode != ResourceFormatLoader::CACHE_MODE_IGNORE) {
					r->set_path(path,
						cache_mode ==
							ResourceFormatLoader::
								CACHE_MODE_REPLACE); // If got here because the resource with same
													 // path has different type, replace it.
				}
				else {
					r->set_path_cache(path);
				}
			}
			r->set_scene_unique_id(id);
		}

		if (!main) {
			internal_index_cache[path] = res;
		}

		int pc = f->get_32();

		for (int j = 0; j < pc; j++) {
			StringName name = _get_string();

			if (name == StringName()) {
				error = ERR_FILE_CORRUPT;
				ERR_FAIL_V(ERR_FILE_CORRUPT);
			}

			if (error) {
				return error;
			}

			bool set_valid = true;
		}

		if (missing_resource.is_valid()) {
			missing_resource->set_recording_properties(false);
		}

		if (progress) {
			*progress = (i + 1) / float(internal_resources.size());
		}

		resource_cache.push_back(res);

		if (main) {
			f.unref();
			resource = res;
			resource->set_as_translation_remapped(translation_remapped);
			error = OK;
			return OK;
		}
	}

	return ERR_FILE_EOF;
}

void ResourceLoaderBinary::set_translation_remapped(bool p_remapped)
{
	translation_remapped = p_remapped;
}

static void save_ustring(Ref<FileAccess> r_file, const String& p_string)
{
	CharString utf8 = p_string.utf8();
	r_file->store_32(uint32_t(utf8.length() + 1));
	r_file->store_buffer((const uint8_t*)utf8.get_data(), utf8.length() + 1);
}

static String get_ustring(Ref<FileAccess> r_file)
{
	int len = r_file->get_32();
	Vector<char> str_buf;
	str_buf.resize(len);
	r_file->get_buffer((uint8_t*)&str_buf[0], len);
	return String::utf8(&str_buf[0], len);
}

String ResourceLoaderBinary::get_unicode_string()
{
	int len = f->get_32();
	if (len > str_buf.size()) {
		str_buf.resize(len);
	}
	if (len == 0) {
		return String();
	}
	f->get_buffer((uint8_t*)&str_buf[0], len);
	return String::utf8(&str_buf[0], len);
}

void ResourceLoaderBinary::get_classes_used(Ref<FileAccess> p_file, HashSet<StringName>* p_classes)
{
	open(p_file, false, true);
	if (error) {
		return;
	}

	for (const IntResource& res : internal_resources) {
		p_file->seek(res.offset);
		String t = get_unicode_string();
		if (!p_file->get_error() && t != String()) {
			p_classes->insert(t);
		}
	}
}

void ResourceLoaderBinary::get_dependencies(
	Ref<FileAccess> p_file, List<String>* p_dependencies, bool p_add_types)
{
	open(p_file, false, true);
	if (error) {
		return;
	}

	for (int i = 0; i < external_resources.size(); i++) {
		String dep;
		String fallback_path;

		if (external_resources[i].uid != ResourceUID::INVALID_ID) {
			dep = ResourceUID::get_singleton()->id_to_text(external_resources[i].uid);
			fallback_path =
				external_resources[i].path; // Used by Dependency Editor, in case uid path fails.
		}
		else {
			dep = external_resources[i].path;
		}

		if (p_add_types && !external_resources[i].type.is_empty()) {
			dep += "::" + external_resources[i].type;
		}
		if (!fallback_path.is_empty()) {
			if (!p_add_types) {
				dep += "::"; // Ensure that path comes third, even if there is no type.
			}
			dep += "::" + fallback_path;
		}

		p_dependencies->push_back(dep);
	}
}

void ResourceLoaderBinary::open(Ref<FileAccess> p_file, bool p_no_resources, bool p_keep_uuid_paths)
{
	error = OK;

	f = p_file;
	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		error = fac->open_after_magic(f);
		if (error != OK) {
			f.unref();
			ERR_FAIL_MSG(vformat("Failed to open binary resource file: '%s'.", local_path));
		}
		f = fac;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		error = ERR_FILE_UNRECOGNIZED;
		f.unref();
		ERR_FAIL_MSG(vformat("Unrecognized binary resource file: '%s'.", local_path));
	}

	bool big_endian = f->get_32();
	bool use_real64 = f->get_32();

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	ver_format = f->get_32();

	print_bl("big endian: " + itos(big_endian));
	print_bl("endian swap: " + itos(big_endian));
	print_bl("real64: " + itos(use_real64));
	print_bl("major: " + itos(ver_major));
	print_bl("minor: " + itos(ver_minor));
	print_bl("format: " + itos(ver_format));

	if (ver_format > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		f.unref();
		ERR_FAIL_MSG(
			vformat("File '%s' can't be loaded, as it uses a format version (%d) or engine version "
					"(%d.%d) which are not supported by your engine version (%s).",
				local_path, ver_format, ver_major, ver_minor, VLTR_VERSION_BRANCH));
	}

	type = get_unicode_string();

	print_bl("type: " + type);

	importmd_ofs = f->get_64();
	uint32_t flags = f->get_32();
	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_NAMED_SCENE_IDS) {
		using_named_scene_ids = true;
	}
	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_UIDS) {
		using_uids = true;
	}
	f->real_is_double =
		(flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_REAL_T_IS_DOUBLE) != 0;

	if (using_uids) {
		uid = ResourceUID::ID(f->get_64());
	}
	else {
		f->get_64(); // skip over uid field
		uid = ResourceUID::INVALID_ID;
	}

	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		script_class = get_unicode_string();
	}

	for (int i = 0; i < ResourceFormatSaverBinaryInstance::RESERVED_FIELDS; i++) {
		f->get_32(); // skip a few reserved fields
	}

	if (p_no_resources) {
		return;
	}

	uint32_t string_table_size = f->get_32();
	string_map.resize(string_table_size);
	for (uint32_t i = 0; i < string_table_size; i++) {
		StringName s = get_unicode_string();
		string_map.write[i] = s;
	}

	print_bl("strings: " + itos(string_table_size));

	uint32_t ext_resources_size = f->get_32();
	for (uint32_t i = 0; i < ext_resources_size; i++) {
		ExtResource er;
		er.type = get_unicode_string();
		er.path = get_unicode_string();
		if (using_uids) {
			er.uid = ResourceUID::ID(f->get_64());
			if (!p_keep_uuid_paths && er.uid != ResourceUID::INVALID_ID) {
				if (ResourceUID::get_singleton()->has_id(er.uid)) {
					// If a UID is found and the path is valid, it will be used, otherwise, it falls
					// back to the path.
					er.path = ResourceUID::get_singleton()->get_id_path(er.uid);
				}
				else {
#ifdef TOOLS_ENABLED
					// Silence a warning that can happen during the initial filesystem scan due to
					// cache being regenerated.
					if (ResourceLoader::get_resource_uid(res_path) != er.uid) {
						WARN_PRINT(vformat("'%s': In external resource #%d, invalid UID: '%s' - "
										   "using text path instead: '%s'.",
							res_path, i, ResourceUID::get_singleton()->id_to_text(er.uid),
							er.path));
					}
#else
					WARN_PRINT(vformat("'%s': In external resource #%d, invalid UID: '%s' - using "
									   "text path instead: '%s'.",
						res_path, i, ResourceUID::get_singleton()->id_to_text(er.uid), er.path));
#endif
				}
			}
		}

		external_resources.push_back(er);
	}

	print_bl("ext resources: " + itos(ext_resources_size));
	uint32_t int_resources_size = f->get_32();

	for (uint32_t i = 0; i < int_resources_size; i++) {
		IntResource ir;
		ir.path = get_unicode_string();
		ir.offset = f->get_64();
		internal_resources.push_back(ir);
	}

	print_bl("int resources: " + itos(int_resources_size));

	if (f->eof_reached()) {
		error = ERR_FILE_CORRUPT;
		f.unref();
		ERR_FAIL_MSG(vformat("Premature end of file (EOF): '%s'.", local_path));
	}
}

String ResourceLoaderBinary::recognize(Ref<FileAccess> p_file)
{
	error = OK;

	f = p_file;
	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		error = fac->open_after_magic(f);
		if (error != OK) {
			f.unref();
			return "";
		}
		f = fac;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		error = ERR_FILE_UNRECOGNIZED;
		f.unref();
		return "";
	}

	bool big_endian = f->get_32();
	f->get_32(); // use_real64

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	uint32_t ver_major = f->get_32();
	f->get_32(); // ver_minor
	uint32_t ver_fmt = f->get_32();

	if (ver_fmt > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		f.unref();
		return "";
	}

	return get_unicode_string();
}

String ResourceLoaderBinary::recognize_script_class(Ref<FileAccess> p_file)
{
	error = OK;

	f = p_file;
	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		error = fac->open_after_magic(f);
		if (error != OK) {
			f.unref();
			return "";
		}
		f = fac;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		error = ERR_FILE_UNRECOGNIZED;
		f.unref();
		return "";
	}

	bool big_endian = f->get_32();
	f->get_32(); // use_real64

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	uint32_t ver_major = f->get_32();
	f->get_32(); // ver_minor
	uint32_t ver_fmt = f->get_32();

	if (ver_fmt > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		f.unref();
		return "";
	}

	_ALLOW_DISCARD_ get_unicode_string(); // type

	f->get_64(); // Metadata offset
	uint32_t flags = f->get_32();
	f->get_64(); // UID

	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		return get_unicode_string();
	}
	else {
		return String();
	}
}

Ref<Resource> ResourceFormatLoaderBinary::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	switch (p_cache_mode) {
	case CACHE_MODE_IGNORE:
	case CACHE_MODE_REUSE:
	case CACHE_MODE_REPLACE:
		loader.cache_mode = p_cache_mode;
		loader.cache_mode_for_external = CACHE_MODE_REUSE;
		break;
	case CACHE_MODE_IGNORE_DEEP:
		loader.cache_mode = CACHE_MODE_IGNORE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	case CACHE_MODE_REPLACE_DEEP:
		loader.cache_mode = CACHE_MODE_REPLACE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	}
	loader.use_sub_threads = p_use_sub_threads;
	loader.progress = r_progress;
	String path = !p_original_path.is_empty() ? p_original_path : p_path;
	loader.local_path = ProjectSettings::get_singleton()->localize_path(path);
	loader.res_path = loader.local_path;
	loader.open(f);

	err = loader.load();

	if (r_error) {
		*r_error = err;
	}

	if (err) {
		return Ref<Resource>();
	}
	return loader.resource;
}

void ResourceFormatLoaderBinary::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}

	// res files not supported for GDExtension.
	if (p_type == "GDExtension") {
		return;
	}
}

void ResourceFormatLoaderBinary::get_recognized_extensions(List<String>* p_extensions) const {}

bool ResourceFormatLoaderBinary::handles_type(const String& p_type) const
{
	return true; // handles all
}

void ResourceFormatLoaderBinary::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(f.is_null(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.get_dependencies(f, p_dependencies, p_add_types);
}

Error ResourceFormatLoaderBinary::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Cannot open file '%s'.", p_path));

	Ref<FileAccess> fw;

	String local_path = p_path.get_base_dir();

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		Error err = fac->open_after_magic(f);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot open file '%s'.", p_path));
		f = fac;

		Ref<FileAccessCompressed> facw;
		facw.instantiate();
		facw->configure("RSCC");
		err = facw->open_internal(p_path + ".depren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			err, ERR_FILE_CORRUPT, vformat("Cannot create file '%s.depren'.", p_path));

		fw = facw;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not normal.
		ERR_FAIL_V_MSG(
			ERR_FILE_UNRECOGNIZED, vformat("Unrecognized binary resource file '%s'.", local_path));
	}
	else {
		fw = FileAccess::open(p_path + ".depren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			fw.is_null(), ERR_CANT_CREATE, vformat("Cannot create file '%s.depren'.", p_path));

		uint8_t magic[4] = {'R', 'S', 'R', 'C'};
		fw->store_buffer(magic, 4);
	}

	bool big_endian = f->get_32();
	bool use_real64 = f->get_32();

	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	fw->store_32(big_endian);
	fw->store_32(use_real64); // use real64
	fw->set_big_endian(big_endian != 0);

	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	uint32_t ver_format = f->get_32();

	if (ver_format < FORMAT_VERSION_CAN_RENAME_DEPS) {
		fw.unref();

		{
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			da->remove(p_path + ".depren");
		}

		// Use the old approach.

		WARN_PRINT(vformat(
			"This file is old, so it can't refactor dependencies, opening and resaving '%s'.",
			p_path));

		Error err;
		f = FileAccess::open(p_path, FileAccess::READ, &err);

		ERR_FAIL_COND_V_MSG(
			err != OK, ERR_FILE_CANT_OPEN, vformat("Cannot open file '%s'.", p_path));

		ResourceLoaderBinary loader;
		loader.local_path = ProjectSettings::get_singleton()->localize_path(p_path);
		loader.res_path = loader.local_path;
		loader.remaps = p_map;
		loader.open(f);

		err = loader.load();

		ERR_FAIL_COND_V(err != ERR_FILE_EOF, ERR_FILE_CORRUPT);
		Ref<Resource> res = loader.get_resource();
		ERR_FAIL_COND_V(res.is_null(), ERR_FILE_CORRUPT);

		return ResourceFormatSaverBinary::singleton->save(res, p_path);
	}

	if (ver_format > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED,
			vformat("File '%s' can't be loaded, as it uses a format version (%d) or engine version "
					"(%d.%d) which are not supported by your engine version (%s).",
				local_path, ver_format, ver_major, ver_minor, VLTR_VERSION_BRANCH));
	}

	// Since we're not actually converting the file contents, leave the version
	// numbers in the file untouched.
	fw->store_32(ver_major);
	fw->store_32(ver_minor);
	fw->store_32(ver_format);

	save_ustring(fw, get_ustring(f)); // type

	uint64_t md_ofs = f->get_position();
	uint64_t importmd_ofs = f->get_64();
	fw->store_64(0); // metadata offset

	uint32_t flags = f->get_32();
	bool using_uids = (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_UIDS);
	uint64_t uid_data = f->get_64();

	fw->store_32(flags);
	fw->store_64(uid_data);
	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		save_ustring(fw, get_ustring(f));
	}

	for (int i = 0; i < ResourceFormatSaverBinaryInstance::RESERVED_FIELDS; i++) {
		fw->store_32(0); // reserved
		f->get_32();
	}

	// string table
	uint32_t string_table_size = f->get_32();

	fw->store_32(string_table_size);

	for (uint32_t i = 0; i < string_table_size; i++) {
		String s = get_ustring(f);
		save_ustring(fw, s);
	}

	// external resources
	uint32_t ext_resources_size = f->get_32();
	fw->store_32(ext_resources_size);
	for (uint32_t i = 0; i < ext_resources_size; i++) {
		String type = get_ustring(f);
		String path = get_ustring(f);

		if (using_uids) {
			ResourceUID::ID uid = f->get_64();
			if (uid != ResourceUID::INVALID_ID) {
				if (ResourceUID::get_singleton()->has_id(uid)) {
					// If a UID is found and the path is valid, it will be used, otherwise, it falls
					// back to the path.
					path = ResourceUID::get_singleton()->get_id_path(uid);
				}
			}
		}

		bool relative = false;
		if (!path.begins_with("res://")) {
			path = local_path.path_join(path).simplify_path();
			relative = true;
		}

		if (p_map.has(path)) {
			String np = p_map[path];
			path = np;
		}

		String full_path = path;

		if (relative) {
			// restore relative
			path = local_path.path_to_file(path);
		}

		save_ustring(fw, type);
		save_ustring(fw, path);

		if (using_uids) {
			ResourceUID::ID uid = ResourceSaver::get_resource_id_for_path(full_path);
			fw->store_64(uint64_t(uid));
		}
	}

	int64_t size_diff = (int64_t)fw->get_position() - (int64_t)f->get_position();

	// internal resources
	uint32_t int_resources_size = f->get_32();
	fw->store_32(int_resources_size);

	for (uint32_t i = 0; i < int_resources_size; i++) {
		String path = get_ustring(f);
		uint64_t offset = f->get_64();
		save_ustring(fw, path);
		fw->store_64(offset + size_diff);
	}

	// rest of file
	uint8_t b = f->get_8();
	while (!f->eof_reached()) {
		fw->store_8(b);
		b = f->get_8();
	}
	f.unref();

	bool all_ok = fw->get_error() == OK;

	fw->seek(md_ofs);
	fw->store_64(importmd_ofs + size_diff);

	if (!all_ok) {
		return ERR_CANT_CREATE;
	}

	fw.unref();

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (da->exists(p_path + ".depren")) {
		da->remove(p_path);
		da->rename(p_path + ".depren", p_path);
	}
	return OK;
}

void ResourceFormatLoaderBinary::get_classes_used(
	const String& p_path, HashSet<StringName>* r_classes)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(f.is_null(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.get_classes_used(f, r_classes);

	if (loader.type != "PackedScene") {
		return;
	}

	// Fetch the nodes inside scene files.

	// Reopening is necessary, or errors will occur.
	f->reopen(p_path, FileAccess::READ);
	loader.open(f);
	ERR_FAIL_COND(loader.load() != OK);

	Ref<SceneState> state = Ref<PackedScene>(loader.get_resource())->get_state();
	for (int i = 0; i < state->get_node_count(); i++) {
		const StringName node_name = state->get_node_type(i);
		// Fetch the values of properties in the node.
	}
}

String ResourceFormatLoaderBinary::get_resource_type(const String& p_path) const
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	loader.res_path = loader.local_path;
	String r = loader.recognize(f);
	return r;
}

String ResourceFormatLoaderBinary::get_resource_script_class(const String& p_path) const
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	loader.res_path = loader.local_path;
	return loader.recognize_script_class(f);
}

ResourceUID::ID ResourceFormatLoaderBinary::get_resource_uid(const String& p_path) const
{
	String ext = p_path.get_extension().to_lower();
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ResourceUID::INVALID_ID; // could not read
	}

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.open(f, true);
	if (loader.error != OK) {
		return ResourceUID::INVALID_ID; // could not read
	}
	return loader.uid;
}

bool ResourceFormatLoaderBinary::has_custom_uid_support() const { return true; }

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////

void ResourceFormatSaverBinaryInstance::_pad_buffer(Ref<FileAccess> r_file, int p_bytes)
{
	int extra = 4 - (p_bytes % 4);
	if (extra < 4) {
		for (int i = 0; i < extra; i++) {
			r_file->store_8(0); // pad to 32
		}
	}
}

void ResourceFormatSaverBinaryInstance::save_unicode_string(
	Ref<FileAccess> r_file, const String& p_string, bool p_bit_on_len)
{
	CharString utf8 = p_string.utf8();
	if (p_bit_on_len) {
		r_file->store_32(uint32_t((utf8.length() + 1) | 0x80000000));
	}
	else {
		r_file->store_32(uint32_t(utf8.length() + 1));
	}
	r_file->store_buffer((const uint8_t*)utf8.get_data(), utf8.length() + 1);
}

int ResourceFormatSaverBinaryInstance::get_string_index(const String& p_string)
{
	StringName s = p_string;
	if (string_map.has(s)) {
		return string_map[s];
	}

	string_map[s] = strings.size();
	strings.push_back(s);
	return strings.size() - 1;
}

static String _resource_get_class(Ref<Resource> p_resource)
{
	Ref<MissingResource> missing_resource = p_resource;
	if (missing_resource.is_valid()) {
		return missing_resource->get_original_class();
	}
	return String();
}

Error ResourceFormatSaverBinaryInstance::save(
	const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags)
{
	Resource::seed_scene_unique_id(p_path.hash());

	Error err;
	Ref<FileAccess> f;
	if (p_flags & ResourceSaver::FLAG_COMPRESS) {
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		fac->configure("RSCC");
		f = fac;
		err = fac->open_internal(p_path, FileAccess::WRITE);
	}
	else {
		f = FileAccess::open(p_path, FileAccess::WRITE, &err);
	}

	ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot create file '%s'.", p_path));

	relative_paths = p_flags & ResourceSaver::FLAG_RELATIVE_PATHS;
	skip_editor = p_flags & ResourceSaver::FLAG_OMIT_EDITOR_PROPERTIES;
	bundle_resources = p_flags & ResourceSaver::FLAG_BUNDLE_RESOURCES;
	big_endian = p_flags & ResourceSaver::FLAG_SAVE_BIG_ENDIAN;
	takeover_paths = p_flags & ResourceSaver::FLAG_REPLACE_SUBRESOURCE_PATHS;

	if (!p_path.begins_with("res://")) {
		takeover_paths = false;
	}

	local_path = p_path.get_base_dir();
	path = ProjectSettings::get_singleton()->localize_path(p_path);

	if (!(p_flags & ResourceSaver::FLAG_COMPRESS)) {
		// save header compressed
		static const uint8_t header[4] = {'R', 'S', 'R', 'C'};
		f->store_buffer(header, 4);
	}

	if (big_endian) {
		f->store_32(1);
	}
	else {
		f->store_32(0);
	}
	f->store_32(0); // 64 bits file, false for now
	f->set_big_endian(big_endian);

	f->store_32(VLTR_VERSION_MAJOR);
	f->store_32(VLTR_VERSION_MINOR);
	f->store_32(FORMAT_VERSION);

	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	save_unicode_string(f, _resource_get_class(p_resource));
	f->store_64(0); // offset to import metadata

	String script_class;
	{
		uint32_t format_flags = FORMAT_FLAG_NAMED_SCENE_IDS | FORMAT_FLAG_UIDS;
#ifdef REAL_T_IS_DOUBLE
		format_flags |= FORMAT_FLAG_REAL_T_IS_DOUBLE;
#endif
		f->store_32(format_flags);
	}
	ResourceUID::ID uid = ResourceSaver::get_resource_id_for_path(p_path, true);
	f->store_64(uint64_t(uid));
	if (!script_class.is_empty()) {
		save_unicode_string(f, script_class);
	}

	for (int i = 0; i < ResourceFormatSaverBinaryInstance::RESERVED_FIELDS; i++) {
		f->store_32(0); // reserved
	}

	List<ResourceData> resources;

	{
		for (const Ref<Resource>& E : saved_resources) {
			ResourceData& rd = resources.push_back(ResourceData())->get();
			rd.type = _resource_get_class(E);
		}
	}

	f->store_32(uint32_t(strings.size())); // string table size
	for (int i = 0; i < strings.size(); i++) {
		save_unicode_string(f, strings[i]);
	}

	// save external resource table
	f->store_32(external_resources.size()); // amount of external resources
	Vector<Ref<Resource>> save_order;
	save_order.resize(external_resources.size());

	for (const KeyValue<Ref<Resource>, int>& E : external_resources) {
		save_order.write[E.value] = E.key;
	}

	for (int i = 0; i < save_order.size(); i++) {
		String res_path = save_order[i]->get_path();
		res_path = relative_paths ? local_path.path_to_file(res_path) : res_path;
		save_unicode_string(f, res_path);
		ResourceUID::ID ruid =
			ResourceSaver::get_resource_id_for_path(save_order[i]->get_path(), false);
		f->store_64(uint64_t(ruid));
	}
	// save internal resource table
	f->store_32(uint32_t(saved_resources.size())); // amount of internal resources
	Vector<uint64_t> ofs_pos;
	HashSet<String> used_unique_ids;

	for (Ref<Resource>& r : saved_resources) {
		if (r->is_built_in()) {
			if (!r->get_scene_unique_id().is_empty()) {
				if (used_unique_ids.has(r->get_scene_unique_id())) {
					r->set_scene_unique_id("");
				}
				else {
					used_unique_ids.insert(r->get_scene_unique_id());
				}
			}
		}
	}

	HashMap<Ref<Resource>, int> resource_map;
	int res_index = 0;
	for (Ref<Resource>& r : saved_resources) {
		if (r->is_built_in()) {
			if (r->get_scene_unique_id().is_empty()) {
				String new_id;

				while (true) {
					new_id = _resource_get_class(r) + "_" + Resource::generate_scene_unique_id();
					if (!used_unique_ids.has(new_id)) {
						break;
					}
				}

				r->set_scene_unique_id(new_id);
				used_unique_ids.insert(new_id);
			}

			save_unicode_string(f, "local://" + r->get_scene_unique_id());
			if (takeover_paths) {
				r->set_path(p_path + "::" + r->get_scene_unique_id(), true);
			}
		}
		else {
			save_unicode_string(f, r->get_path()); // actual external
		}
		ofs_pos.push_back(f->get_position());
		f->store_64(0); // offset in 64 bits
		resource_map[r] = res_index++;
	}

	Vector<uint64_t> ofs_table;

	// now actually save the resources
	for (const ResourceData& rd : resources) {
		ofs_table.push_back(f->get_position());
		save_unicode_string(f, rd.type);
		f->store_32(uint32_t(rd.properties.size()));

		for (const Property& p : rd.properties) {
			f->store_32(uint32_t(p.name_idx));
		}
	}

	for (int i = 0; i < ofs_table.size(); i++) {
		f->seek(ofs_pos[i]);
		f->store_64(ofs_table[i]);
	}

	f->seek_end();

	f->store_buffer((const uint8_t*)"RSRC", 4); // magic at end

	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Error ResourceFormatSaverBinaryInstance::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Cannot open file '%s'.", p_path));

	Ref<FileAccess> fw;

	local_path = p_path.get_base_dir();

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] == 'R' && header[1] == 'S' && header[2] == 'C' && header[3] == 'C') {
		// Compressed.
		Ref<FileAccessCompressed> fac;
		fac.instantiate();
		Error err = fac->open_after_magic(f);
		ERR_FAIL_COND_V_MSG(err != OK, err, vformat("Cannot open file '%s'.", p_path));
		f = fac;

		Ref<FileAccessCompressed> facw;
		facw.instantiate();
		facw->configure("RSCC");
		err = facw->open_internal(p_path + ".uidren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			err, ERR_FILE_CORRUPT, vformat("Cannot create file '%s.uidren'.", p_path));

		fw = facw;

	}
	else if (header[0] != 'R' || header[1] != 'S' || header[2] != 'R' || header[3] != 'C') {
		// Not a binary resource.
		return ERR_FILE_UNRECOGNIZED;
	}
	else {
		fw = FileAccess::open(p_path + ".uidren", FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(
			fw.is_null(), ERR_CANT_CREATE, vformat("Cannot create file '%s.uidren'.", p_path));

		uint8_t magich[4] = {'R', 'S', 'R', 'C'};
		fw->store_buffer(magich, 4);
	}

	big_endian = f->get_32();
	bool use_real64 = f->get_32();
	f->set_big_endian(big_endian != 0); // read big endian if saved as big endian

	fw->store_32(big_endian);
	fw->store_32(use_real64); // use real64
	fw->set_big_endian(big_endian != 0);

	uint32_t ver_major = f->get_32();
	uint32_t ver_minor = f->get_32();
	uint32_t ver_format = f->get_32();

	if (ver_format < FORMAT_VERSION_CAN_RENAME_DEPS) {
		fw.unref();

		{
			Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);
			da->remove(p_path + ".uidren");
		}

		// Use the old approach.

		WARN_PRINT(vformat(
			"This file is old, so it does not support UIDs, opening and resaving '%s'.", p_path));
		return ERR_UNAVAILABLE;
	}

	if (ver_format > FORMAT_VERSION || ver_major > VLTR_VERSION_MAJOR) {
		ERR_FAIL_V_MSG(ERR_FILE_UNRECOGNIZED,
			vformat("File '%s' can't be loaded, as it uses a format version (%d) or engine version "
					"(%d.%d) which are not supported by your engine version (%s).",
				local_path, ver_format, ver_major, ver_minor, VLTR_VERSION_BRANCH));
	}

	// Since we're not actually converting the file contents, leave the version
	// numbers in the file untouched.
	fw->store_32(ver_major);
	fw->store_32(ver_minor);
	fw->store_32(ver_format);

	save_ustring(fw, get_ustring(f)); // type

	fw->store_64(f->get_64()); // metadata offset

	uint32_t flags = f->get_32();
	flags |= ResourceFormatSaverBinaryInstance::FORMAT_FLAG_UIDS;
	f->get_64(); // Skip previous UID

	fw->store_32(flags);
	fw->store_64(uint64_t(p_uid));

	if (flags & ResourceFormatSaverBinaryInstance::FORMAT_FLAG_HAS_SCRIPT_CLASS) {
		save_ustring(fw, get_ustring(f));
	}

	// rest of file
	uint8_t b = f->get_8();
	while (!f->eof_reached()) {
		fw->store_8(b);
		b = f->get_8();
	}

	f.unref();

	bool all_ok = fw->get_error() == OK;

	if (!all_ok) {
		return ERR_CANT_CREATE;
	}

	fw.unref();

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	da->remove(p_path);
	da->rename(p_path + ".uidren", p_path);
	return OK;
}

Error ResourceFormatSaverBinary::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	ResourceFormatSaverBinaryInstance saver;
	return saver.save(local_path, p_resource, p_flags);
}

Error ResourceFormatSaverBinary::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String local_path = ProjectSettings::get_singleton()->localize_path(p_path);
	ResourceFormatSaverBinaryInstance saver;
	return saver.set_uid(local_path, p_uid);
}

bool ResourceFormatSaverBinary::recognize(const Ref<Resource>& p_resource) const
{
	return true; // all recognized
}

void ResourceFormatSaverBinary::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	String base = p_resource->get_base_extension().to_lower();
	p_extensions->push_back(base);
	if (base != "res") {
		p_extensions->push_back("res");
	}
}

ResourceFormatSaverBinary::ResourceFormatSaverBinary() { singleton = this; }


