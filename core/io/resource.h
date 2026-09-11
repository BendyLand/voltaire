/**************************************************************************/
/*  resource.h                                                            */
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

#pragma once

#include "core/io/resource_uid.h" // IWYU pragma: export. Make available to all resources.
#include "core/templates/self_list.h"
#include "core/types.h"

class Node;
class RWLock;

#define RES_BASE_EXTENSION(m_ext)                                                                  \
public:                                                                                            \
	static void register_custom_data_to_otdb()                                                     \
	{                                                                                              \
		Resource::_add_resource_base_extension_to_classdb(m_ext, get_class_static());              \
	}                                                                                              \
                                                                                                   \
	virtual String get_base_extension() const override                                             \
	{                                                                                              \
		return m_ext;                                                                              \
	}                                                                                              \
                                                                                                   \
private:

class Resource : public RefCounted
{
public:
	static void register_custom_data_to_otdb();

	virtual String get_base_extension() const { return "res"; }

protected:
	static void _add_resource_base_extension_to_classdb(
		const String& p_extension, const String& p_class);

private:
	friend class ResourceCache;

	String name;
	String path_cache;
	String scene_unique_id;

#ifdef TOOLS_ENABLED
	uint64_t last_modified_time = 0;
	uint64_t import_last_modified_time = 0;
	String import_path;
#endif

	enum EmitChangedState
	{
		EMIT_CHANGED_UNBLOCKED,
		EMIT_CHANGED_BLOCKED,
		EMIT_CHANGED_BLOCKED_PENDING_EMIT,
	};

	EmitChangedState emit_changed_state = EMIT_CHANGED_UNBLOCKED;
	bool local_to_scene = false;
	friend class SceneState;
	Node* local_scene = nullptr;

	SelfList<Resource> remapped_list;

	using DuplicateRemapCacheT = HashMap<Ref<Resource>, Ref<Resource>>;
	static thread_local inline DuplicateRemapCacheT* thread_duplicate_remap_cache = nullptr;
	static thread_local inline bool thread_duplicate_remap_cache_needs_deallocation = true;

protected:
	virtual void _resource_path_changed();
	static void _bind_methods();

	void _block_emit_changed();
	void _unblock_emit_changed();

	void _set_path(const String& p_path);
	void _take_over_path(const String& p_path);

	virtual void reset_local_to_scene();

	virtual Ref<Resource> _duplicate(bool p_subresources = false) const;
	virtual String _to_string();

public:
	static Node* (*_get_local_scene_func)();		// Used by the editor.
	static void (*_update_configuration_warning)(); // Used by the editor.

	void update_configuration_warning();
	virtual bool editor_can_reload_from_file();
	virtual void reset_state(); // For resources that store state in non-exposed properties, such as
								// via _validate_property or _get_property_list, this function must
								// be implemented to clear them.
	virtual Error copy_from(const Ref<Resource>& p_resource);
	virtual void reload_from_file();

	void emit_changed();
	void set_name(const String& p_name);
	String get_name() const;

	virtual void set_path(const String& p_path, bool p_take_over = false);
	String get_path() const;
	virtual void set_path_cache(
		const String& p_path); // Set raw path without involving resource cache.

	_FORCE_INLINE_ bool is_built_in() const
	{
		return path_cache.is_empty() || path_cache.contains("::") ||
			   path_cache.begins_with("local://");
	}

	static void seed_scene_unique_id(uint32_t p_seed);
	static String generate_scene_unique_id();
	void set_scene_unique_id(const String& p_id);
	String get_scene_unique_id() const;

	Ref<Resource> duplicate(bool p_deep = false) const;
	void configure_for_local_scene(
		Node* p_for_scene, HashMap<Ref<Resource>, Ref<Resource>>& p_remap_cache);

	void set_local_to_scene(bool p_enable);
	bool is_local_to_scene() const;
	virtual void setup_local_to_scene();

	Node* get_local_scene() const;

	void set_as_translation_remapped(bool p_remapped);

	virtual RID get_rid() const; // Some resources may offer conversion to RID.

	// Helps keep IDs the same when loading/saving scenes. An empty ID clears the entry, and an
	// empty ID is returned when not found.
	static void set_resource_id_for_path(
		const String& p_referrer_path, const String& p_resource_path, const String& p_id);

	void set_id_for_path(const String& p_referrer_path, const String& p_id)
	{
		set_resource_id_for_path(p_referrer_path, get_path(), p_id);
	}

	String get_id_for_path(const String& p_referrer_path) const;

	Resource();
	~Resource();
};

class ResourceCache
{
	friend class Resource;
	friend class ResourceLoader; // Need the lock.
	static Mutex lock;
	static HashMap<String, Resource*> resources;
#ifdef TOOLS_ENABLED
	static HashMap<String, HashMap<String, String>>
		resource_path_cache; // Each tscn has a set of resource paths and IDs.
	static RWLock path_cache_lock;
#endif // TOOLS_ENABLED
	friend void unregister_core_types();
	static void clear();
	friend void register_core_types();

public:
	static bool has(const String& p_path);
	static Ref<Resource> get_ref(const String& p_path);
	static void get_cached_resources(List<Ref<Resource>>* p_resources);
	static int get_cached_resource_count();
};


