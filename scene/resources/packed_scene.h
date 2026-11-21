/**************************************************************************/
/*  packed_scene.h                                                        */
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
#include "core/typedefs.h"
#include "scene/main/node.h"

class PackedScene;

class SceneState : public RefCounted
{
	Vector<String> names;
	Vector<NodePath> node_paths;
	Vector<PackedInt32Array> id_paths;
	mutable PackedInt32Array ids;
	Vector<NodePath> editable_instances;
	mutable HashMap<NodePath, int> node_path_cache;
	mutable HashMap<int, int> base_scene_node_remap;

	int base_scene_idx = -1;

	enum
	{
		NO_PARENT_SAVED = 0x7FFFFFFF,
		NAME_INDEX_BITS = 18,
		NAME_MASK = (1 << NAME_INDEX_BITS) - 1,
	};

	struct NodeData
	{
		int parent = 0;
		int owner = 0;
		int type = 0;
		int name = 0;
		int instance = 0;
		int index = 0;

		struct Property
		{
			int name = 0;
			int value = 0;
		};

		Vector<Property> properties;
		Vector<int> groups;
	};

	struct DeferredNodePathProperties
	{
		String property;
	};

	Vector<NodeData> nodes;

	struct ConnectionData
	{
		int from = 0;
		int to = 0;
		int signal = 0;
		int method = 0;
		int flags = 0;
		int unbinds = 0;
		Vector<int> binds;
	};

	Vector<ConnectionData> connections;

	String path;

	uint64_t last_modified_time = 0;

	static bool disable_placeholders;

	Vector<String> _get_node_groups(int p_idx) const;

	int _find_base_scene_node_remap_key(int p_idx) const;

	Node* _recover_node_path_index(Node* p_base, int p_idx) const;

#ifdef TOOLS_ENABLED
public:
	typedef void (*InstantiationWarningNotify)(const String& p_warning);

private:
	static InstantiationWarningNotify instantiation_warn_notify;
#endif

protected:

public:
	enum
	{
		FLAG_ID_IS_PATH = (1 << 30),
		TYPE_INSTANTIATED = 0x7FFFFFFF,
		FLAG_INSTANCE_IS_PLACEHOLDER = (1 << 30),
		FLAG_PATH_PROPERTY_IS_NODE = (1 << 30),
		FLAG_PROP_NAME_MASK = FLAG_PATH_PROPERTY_IS_NODE - 1,
		FLAG_MASK = (1 << 24) - 1,
	};

	enum GenEditState
	{
		GEN_EDIT_STATE_DISABLED,
		GEN_EDIT_STATE_INSTANCE,
		GEN_EDIT_STATE_MAIN,
		GEN_EDIT_STATE_MAIN_INHERITED,
	};

	struct PackState
	{
		Ref<SceneState> state;
		int node = -1;
	};

	static void set_disable_placeholders(bool p_disable);
	static Ref<Resource> get_remap_resource(const Ref<Resource>& p_resource,
		HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& remap_cache,
		const Ref<Resource>& p_fallback, Node* p_for_scene);

	int find_node_by_path(const NodePath& p_node) const;
	bool is_node_in_group(int p_node, const String& p_group) const;
	bool is_connection(
		int p_node, const String& p_signal, int p_to_node, const String& p_to_method) const;

	Error pack(Node* p_scene);

	void set_path(const String& p_path);
	String get_path() const;

	void clear();
	Error copy_from(const Ref<SceneState>& p_scene_state);

	bool can_instantiate() const;
	Node* instantiate(GenEditState p_edit_state) const;

	Ref<SceneState> get_base_scene_state() const;

	void update_instance_resource(String p_path, Ref<PackedScene> p_packed_scene);

	// unbuild API

	int get_node_count() const;
	String get_node_type(int p_idx) const;
	String get_node_name(int p_idx) const;
	NodePath get_node_path(int p_idx, bool p_for_parent = false) const;
	int32_t get_node_unique_id(int p_idx) const;
	PackedInt32Array get_node_id_path(int p_idx) const;
	PackedInt32Array get_node_parent_id_path(int p_idx) const;
	NodePath get_node_owner_path(int p_idx) const;
	PackedInt32Array get_node_owner_id_path(int p_idx) const;
	Ref<PackedScene> get_node_instance(int p_idx) const;
	String get_node_instance_placeholder(int p_idx) const;
	bool is_node_instance_placeholder(int p_idx) const;
	Vector<String> get_node_groups(int p_idx) const;
	int get_node_index(int p_idx) const;

	int get_node_property_count(int p_idx) const;
	String get_node_property_name(int p_idx, int p_prop) const;
	Vector<String> get_node_deferred_nodepath_properties(int p_idx) const;

	int get_connection_count() const;
	NodePath get_connection_source(int p_idx) const;
	String get_connection_signal(int p_idx) const;
	NodePath get_connection_target(int p_idx) const;
	String get_connection_method(int p_idx) const;

	PackedInt32Array get_connection_source_id_path(int p_idx) const;
	PackedInt32Array get_connection_target_id_path(int p_idx) const;

	int get_connection_flags(int p_idx) const;
	int get_connection_unbinds(int p_idx) const;

	bool has_connection(const NodePath& p_node_from, const String& p_signal,
		const NodePath& p_node_to, const String& p_method, bool p_no_inheritance = false);

	Vector<NodePath> get_editable_instances() const;
	Ref<Resource> get_sub_resource(const String& p_path);
	Vector<Ref<Resource>> get_sub_resources();

	// build API

	int add_name(const String& p_name);
	int add_node_path(const NodePath& p_path, const PackedInt32Array& p_uid_path);
	int add_node(int p_parent, int p_owner, int p_type, int p_name, int p_instance, int p_index,
		int32_t p_unique_id);
	void add_node_property(int p_node, int p_name, int p_value, bool p_deferred_node_path = false);
	void add_node_group(int p_node, int p_group);
	void set_base_scene(int p_idx);
	void add_connection(int p_from, int p_to, int p_signal, int p_method, int p_flags,
		int p_unbinds, const Vector<int>& p_binds);
	void add_editable_instance(const NodePath& p_path);

	bool remove_group_references(const String& p_name);
	bool rename_group_references(const String& p_old_name, const String& p_new_name);
	HashSet<String> get_all_groups();

	virtual void set_last_modified_time(uint64_t p_time) { last_modified_time = p_time; }

	uint64_t get_last_modified_time() const { return last_modified_time; }

	// Used when saving pointers (saves a path property instead).
	static String get_meta_pointer_property(const String& p_property);

#ifdef TOOLS_ENABLED
	static void set_instantiation_warning_notify_func(InstantiationWarningNotify p_warn_notify)
	{
		instantiation_warn_notify = p_warn_notify;
	}
#endif

	SceneState() = default;
};


class PackedScene : public Resource
{
	Ref<SceneState> state;

protected:
	virtual bool editor_can_reload_from_file() override
	{
		return false;
	} // this is handled by editor better

	virtual void reset_state() override;

public:
	enum GenEditState
	{
		GEN_EDIT_STATE_DISABLED,
		GEN_EDIT_STATE_INSTANCE,
		GEN_EDIT_STATE_MAIN,
		GEN_EDIT_STATE_MAIN_INHERITED,
	};

	Error pack(Node* p_scene);

	void clear();

	bool can_instantiate() const;
	Node* instantiate(GenEditState p_edit_state = GEN_EDIT_STATE_DISABLED) const;

	void recreate_state();
	void replace_state(Ref<SceneState> p_by);

	virtual void reload_from_file() override;

	virtual void set_path(const String& p_path, bool p_take_over = false) override;
	virtual void set_path_cache(const String& p_path) override;

	Ref<SceneState> get_state() const;

	PackedScene();
};

// --- Folded from scene/resources/resource_format_text.h ---
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/templates/rb_map.h"


class ResourceLoaderText
{
public:
	enum
	{
		// Version 2: Changed names for Basis, AABB, Vectors, etc.
		// Version 3: New string ID for ext/subresources, breaks forward compat.
		// Version 4: PackedByteArray can be base64 encoded, and PackedVector4Array was added.
		FORMAT_VERSION = 4,
		// For compat, save as version 3 if not using PackedVector4Array or no big PackedByteArray.
		FORMAT_VERSION_COMPAT = 3,
	};

private:
	bool translation_remapped = false;
	String local_path;
	String res_path;
	String error_text;

	Ref<FileAccess> f;

	struct ExtResource
	{
		Ref<ResourceLoader::LoadToken> load_token;
		String path;
		String type;
	};

	bool is_scene = false;
	int format_version;
	String res_type;

	bool ignore_resource_parsing = false;

	HashMap<String, ExtResource> ext_resources;
	HashMap<String, Ref<Resource>> int_resources;

	int resources_total = 0;
	int resource_current = 0;

	String resource_type;
	String script_class;

	ResourceFormatLoader::CacheMode cache_mode = ResourceFormatLoader::CACHE_MODE_REUSE;
	ResourceFormatLoader::CacheMode cache_mode_for_external =
		ResourceFormatLoader::CACHE_MODE_REUSE;

	bool use_sub_threads = false;
	float* progress = nullptr;

	mutable int lines = 0;

	ResourceUID::ID res_uid = ResourceUID::INVALID_ID;

	HashMap<String, String> remaps;

	void _count_resources();

	struct DummyReadData
	{
		bool no_placeholders = false;
		HashMap<Ref<Resource>, int> external_resources;
		HashMap<String, Ref<Resource>> rev_external_resources;
		HashMap<Ref<Resource>, int> resource_index_map;
		HashMap<String, Ref<Resource>> resource_map;
	};

	void _printerr();

	friend class ResourceFormatLoaderText;
	friend
 class ResourceFormatSaverText;

	Error error = OK;

	Ref<Resource> resource;

public:
	Ref<Resource> get_resource();
	Error load();
	Error set_uid(Ref<FileAccess> p_f, ResourceUID::ID p_uid);
	int get_stage() const;
	int get_stage_count() const;
	void set_translation_remapped(bool p_remapped);

	void open(Ref<FileAccess> p_f, bool p_skip_first_tag = false);
	String recognize(Ref<FileAccess> p_f);
	String recognize_script_class(Ref<FileAccess> p_f);
	ResourceUID::ID get_uid(Ref<FileAccess> p_f);
	void get_dependencies(Ref<FileAccess> p_f, List<String>* p_dependencies, bool p_add_types);
	Error rename_dependencies(
		Ref<FileAccess> p_f, const String& p_path, const HashMap<String, String>& p_map);
	Error get_classes_used(HashSet<String>* r_classes);

	ResourceLoaderText() = default;
};

class ResourceFormatLoaderText : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions_for_type(
		const String& p_type, List<String>* p_extensions) const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual void get_classes_used(const String& p_path, HashSet<String>* r_classes) override;

	virtual String get_resource_type(const String& p_path) const override;
	virtual String get_resource_script_class(const String& p_path) const override;
	virtual ResourceUID::ID get_resource_uid(const String& p_path) const override;
	virtual bool has_custom_uid_support() const override;
	virtual void get_dependencies(
		const String& p_path, List<String>* p_dependencies, bool p_add_types = false) override;
	virtual Error rename_dependencies(
		const String& p_path, const HashMap<String, String>& p_map) override;

	ResourceFormatLoaderText() = default;
};

class ResourceFormatSaverTextInstance
{
	String local_path;

	Ref<PackedScene> packed_scene;

	bool takeover_paths = false;
	bool relative_paths = false;
	bool bundle_resources = false;
	bool skip_editor = false;

	struct NonPersistentKey
	{ // for resource properties generated on the fly
		Ref<Resource> base;
		String property;

		bool operator<(const NonPersistentKey& p_key) const
		{
			return base == p_key.base ? property < p_key.property : base < p_key.base;
		}
	};

	HashSet<Ref<Resource>> resource_set;
	List<Ref<Resource>> saved_resources;
	HashMap<Ref<Resource>, String> external_resources;
	HashMap<Ref<Resource>, String> internal_resources;
	bool use_compat = true;

	struct ResourceSort
	{
		Ref<Resource> resource;
		String id;

		bool operator<(const ResourceSort& p_right) const
		{
			return id.naturalnocasecmp_to(p_right.id) < 0;
		}
	};

	static String _write_resources(void* ud, const Ref<Resource>& p_resource);
	String _write_resource(const Ref<Resource>& res);

public:
	Error save(const String& p_path, const Ref<Resource>& p_resource, uint32_t p_flags = 0);
};

class ResourceFormatSaverText : public ResourceFormatSaver
{
public:
	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual Error set_uid(const String& p_path, ResourceUID::ID p_uid) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceFormatSaverText() = default;
};



// --- End scene/resources/resource_format_text.h ---

// --- Folded from scene/resources/video_stream.h ---
#include "scene/resources/texture.h"

class VideoStreamPlayback : public Resource
{
public:
	typedef int (*AudioMixCallback)(void* p_udata, const float* p_data, int p_frames);
	struct TheoraData;

private:
	TheoraData* data = nullptr;

protected:
	AudioMixCallback mix_callback = nullptr;
	void* mix_udata = nullptr;
	mutable int _channel_count = 0; // Used only to assist with bounds checking in mix_audio.

	int mix_audio(int num_frames, PackedFloat32Array buffer = {}, int offset = 0);

public:
	VideoStreamPlayback();
	~VideoStreamPlayback();

	void stop();
	void play();

	bool is_playing() const;

	void set_paused(bool p_paused);
	bool is_paused() const;

	double get_length() const;

	double get_playback_position() const;
	void seek(double p_time);

	void set_file(const String& p_file);
	void set_audio_track(int p_idx);

	Ref<Texture2D> get_texture() const;
	void update(double p_delta);

	void set_mix_callback(AudioMixCallback p_callback, void* p_userdata);
	int get_channels() const;
	int get_mix_rate() const;
};

class VideoStream : public Resource
{
protected:
	String file;
	int audio_track = 0;

public:
	void set_file(const String& p_file);
	String get_file();

	void set_audio_track(int p_track);
	Ref<VideoStreamPlayback> instantiate_playback();

	VideoStream();
	~VideoStream();
};

// --- End scene/resources/video_stream.h ---
