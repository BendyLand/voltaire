/**************************************************************************/
/*  resource_importer.h                                                   */
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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"

class ResourceImporter;
class ResourceFormatImporter;

typedef Ref<Resource> (*ResourceFormatImporterLoadOnStartup)(
	const String& p_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	ResourceFormatLoader::CacheMode p_cache_mode);

class ResourceFormatImporter final
{
public:
	using CacheMode = ResourceFormatLoader::CacheMode;

private:
	struct PathAndType
	{
		String path;
		String type;
		String importer;
		String group_file;
		ResourceUID::ID uid = ResourceUID::INVALID_ID;
	};

	static Error _get_path_and_type(const String& p_path, PathAndType& r_path_and_type, bool p_load,
		bool* r_valid = nullptr);

	// need them to stay in order to compute the settings hash
	struct SortImporterByName
	{
		bool operator()(const Ref<ResourceImporter>& p_a, const Ref<ResourceImporter>& p_b) const;
	};

	struct Data
	{
		Vector<Ref<ResourceImporter>> importers;
	};

	static inline Data* data = nullptr;

public:
	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE);
	static Ref<Resource> load_internal(const String& p_path, Error* r_error, bool p_use_sub_threads,
		float* r_progress, CacheMode p_cache_mode, bool p_silence_errors);
	static void get_recognized_extensions(List<String>* p_extensions);
	static void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions);
	static bool recognize_path(
		const String& p_path, const String& p_for_type = String());
	static bool handles_type(const String& p_type);
	static String get_resource_type(const String& p_path);
	static String get_resource_script_class(const String& p_path);
	static ResourceUID::ID get_resource_uid(const String& p_path);
	static bool has_custom_uid_support();
	static bool is_import_valid(const String& p_path);
	static void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false);

	static bool is_imported(const String& p_path) { return recognize_path(p_path); }

	static String get_import_group_file(const String& p_path);
	static void get_classes_used(const String& p_path, HashSet<String>* r_classes);
	static bool exists(const String& p_path);

	static void get_build_dependencies(const String& p_path, HashSet<String>* r_dependencies);

	static int get_import_order(const String& p_path);

	static Error get_import_order_threads_and_importer(
		const String& p_path, int& r_order, bool& r_can_threads, String& r_importer);

	static String get_internal_resource_path(const String& p_path);
	static void get_internal_resource_path_list(const String& p_path, List<String>* r_paths);

	static void add_importer(const Ref<ResourceImporter>& p_importer, bool p_first_priority = false);

	static void remove_importer(const Ref<ResourceImporter>& p_importer)
	{
		if (data) {
			data->importers.erase(p_importer);
		}
	}

	static Ref<ResourceImporter> get_importer_by_name(const String& p_name);
	static Ref<ResourceImporter> get_importer_by_file(const String& p_file);
	static void get_importers_for_file(const String& p_file, List<Ref<ResourceImporter>>* r_importers);
	static void get_importers(List<Ref<ResourceImporter>>* r_importers);

	static bool are_import_settings_valid(const String& p_path);
	static String get_import_settings_hash();

	static String get_import_base_path(const String& p_for_file);
	static Error get_resource_import_info(const String& p_path, String& r_type, ResourceUID::ID& r_uid,
		String& r_import_group_file);

	ResourceFormatImporter() = delete;
	ResourceFormatImporter(const ResourceFormatImporter&) = delete;
	ResourceFormatImporter& operator=(const ResourceFormatImporter&) = delete;
	~ResourceFormatImporter() = delete;
};

class ResourceFormatImporterLoader : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override
	{
		return ResourceFormatImporter::load(
			p_path, p_original_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
	}
	virtual void get_recognized_extensions(List<String>* p_extensions) const override
	{
		ResourceFormatImporter::get_recognized_extensions(p_extensions);
	}
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override
	{
		ResourceFormatImporter::get_recognized_extensions_for_type(p_type, p_extensions);
	}
	virtual bool recognize_path(
		const String& p_path, const String& p_for_type = String()) const override
	{
		return ResourceFormatImporter::recognize_path(p_path, p_for_type);
	}
	virtual bool handles_type(const String& p_type) const override
	{
		return ResourceFormatImporter::handles_type(p_type);
	}
	virtual String get_resource_type(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_type(p_path);
	}
	virtual String get_resource_script_class(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_script_class(p_path);
	}
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override
	{
		return ResourceFormatImporter::get_resource_uid(p_path);
	}
	virtual bool has_custom_uid_support() const override
	{
		return ResourceFormatImporter::has_custom_uid_support();
	}
	virtual bool is_import_valid(const String& p_path) const override
	{
		return ResourceFormatImporter::is_import_valid(p_path);
	}
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override
	{
		ResourceFormatImporter::get_dependencies(p_path, p_dependencies, p_add_types);
	}
	virtual bool is_imported(const String& p_path) const override
	{
		return ResourceFormatImporter::is_imported(p_path);
	}
	virtual String get_import_group_file(const String& p_path) const override
	{
		return ResourceFormatImporter::get_import_group_file(p_path);
	}
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override
	{
		ResourceFormatImporter::get_classes_used(p_path, r_classes);
	}
	virtual bool exists(const String& p_path) const override
	{
		return ResourceFormatImporter::exists(p_path);
	}
	virtual int get_import_order(const String& p_path) const override
	{
		return ResourceFormatImporter::get_import_order(p_path);
	}
};

class ResourceImporter : public RefCounted
{
protected:

public:
	static inline ResourceFormatImporterLoadOnStartup load_on_startup = nullptr;

	virtual String get_importer_name() const = 0;
	virtual String get_visible_name() const = 0;
	virtual void get_recognized_extensions(List<String>* p_extensions) const = 0;
	virtual String get_save_extension() const = 0;
	virtual String get_resource_type() const = 0;

	virtual float get_priority() const { return 1.0; }

	virtual int get_import_order() const { return IMPORT_ORDER_DEFAULT; }

	virtual int get_format_version() const { return 0; }

	struct ImportOption
	{
		ImportOption() {}
	};

	enum ImportOrder
	{
		IMPORT_ORDER_DEFAULT = 0,
		IMPORT_ORDER_SCENE = 100,
	};

	virtual bool has_advanced_options() const { return false; }

	virtual void show_advanced_options(const String& p_path) {}

	virtual int get_preset_count() const { return 0; }

	virtual String get_preset_name(int p_idx) const { return String(); }

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const = 0;

	virtual String get_option_group_file() const { return String(); }

	virtual bool can_import_threaded() const { return false; }

	virtual void import_threaded_begin() {}

	virtual void import_threaded_end() {}

	virtual String get_import_settings_string() const { return String(); }

	virtual void get_build_dependencies(
		const String& p_path, HashSet<String>* r_build_dependencies);
};

class ResourceFormatImporterSaver : public ResourceFormatSaver
{
public:
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
};


