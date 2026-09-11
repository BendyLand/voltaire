/**************************************************************************/
/*  resource.cpp                                                          */
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

#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/math/random_pcg.h"
#include "core/os/os.h"
#include "resource.h"
#include "scene/main/node.h" //only so casting works

void Resource::register_custom_data_to_otdb()
{
}

void Resource::_add_resource_base_extension_to_classdb(
	const String& p_extension, const String& p_class)
{
}

void Resource::emit_changed()
{
	if (emit_changed_state != EMIT_CHANGED_UNBLOCKED) {
		emit_changed_state = EMIT_CHANGED_BLOCKED_PENDING_EMIT;
		return;
	}
	if (ResourceLoader::is_within_load() && !Thread::is_main_thread()) {
		ResourceLoader::resource_changed_emit(this);
		return;
	}
}

void Resource::_block_emit_changed()
{
	if (emit_changed_state == EMIT_CHANGED_UNBLOCKED) {
		emit_changed_state = EMIT_CHANGED_BLOCKED;
	}
}

void Resource::_unblock_emit_changed()
{
	bool emit = (emit_changed_state == EMIT_CHANGED_BLOCKED_PENDING_EMIT);
	emit_changed_state = EMIT_CHANGED_UNBLOCKED;
	if (emit) {
		emit_changed();
	}
}

void Resource::_resource_path_changed() {}

void Resource::set_path(const String& p_path, bool p_take_over)
{
	if (path_cache == p_path) {
		return;
	}

	if (p_path.is_empty()) {
		p_take_over = false; // Can't take over an empty path
	}

	{
		MutexLock lock(ResourceCache::lock);

		if (!path_cache.is_empty()) {
			ResourceCache::resources.erase(path_cache);
		}

		path_cache = "";

		Ref<Resource> existing = ResourceCache::get_ref(p_path);

		if (existing.is_valid()) {
			if (p_take_over) {
				existing->path_cache = String();
				ResourceCache::resources.erase(p_path);
			}
			else {
				ERR_FAIL_MSG(vformat("Another resource is loaded from path '%s' (possible cyclic "
									 "resource inclusion).",
					p_path));
			}
		}

		path_cache = p_path;

		if (!path_cache.is_empty()) {
			ResourceCache::resources[path_cache] = this;
		}
	}

	_resource_path_changed();
}

String Resource::get_path() const { return path_cache; }

void Resource::set_path_cache(const String& p_path) { path_cache = p_path; }

static thread_local RandomPCG unique_id_gen = RandomPCG(0);

void Resource::seed_scene_unique_id(uint32_t p_seed) { unique_id_gen.seed(p_seed); }

String Resource::generate_scene_unique_id()
{
	// Generate a unique enough hash, but still user-readable.
	// If it's not unique it does not matter because the saver will try again.
	if (unique_id_gen.get_seed() == 0) {
		OS::DateTime dt = OS::get_singleton()->get_datetime();
		uint32_t hash = hash_murmur3_one_32(OS::get_singleton()->get_ticks_usec());
		hash = hash_murmur3_one_32(dt.year, hash);
		hash = hash_murmur3_one_32(dt.month, hash);
		hash = hash_murmur3_one_32(dt.day, hash);
		hash = hash_murmur3_one_32(dt.hour, hash);
		hash = hash_murmur3_one_32(dt.minute, hash);
		hash = hash_murmur3_one_32(dt.second, hash);
		hash = hash_murmur3_one_32(Math::rand(), hash);
		unique_id_gen.seed(hash);
	}

	uint32_t random_num = unique_id_gen.rand();

	static constexpr uint32_t characters = 5;
	static constexpr uint32_t char_count = ('z' - 'a');
	static constexpr uint32_t base = char_count + ('9' - '0');
	String id;
	id.resize_uninitialized(characters + 1);
	char32_t* ptr = id.ptrw();
	for (uint32_t i = 0; i < characters; i++) {
		uint32_t c = random_num % base;
		if (c < char_count) {
			ptr[i] = ('a' + c);
		}
		else {
			ptr[i] = ('0' + (c - char_count));
		}
		random_num /= base;
	}
	ptr[characters] = '\0';

	return id;
}

void Resource::set_scene_unique_id(const String& p_id)
{
	bool is_valid = true;
	for (int i = 0; i < p_id.length(); i++) {
		if (!is_ascii_identifier_char(p_id[i])) {
			is_valid = false;
			scene_unique_id = Resource::generate_scene_unique_id();
			break;
		}
	}

	ERR_FAIL_COND_MSG(
		!is_valid, "The scene unique ID must contain only letters, numbers, and underscores.");
	scene_unique_id = p_id;
}

String Resource::get_scene_unique_id() const { return scene_unique_id; }

void Resource::set_name(const String& p_name)
{
	name = p_name;
	emit_changed();
}

String Resource::get_name() const { return name; }

void Resource::update_configuration_warning()
{
	if (_update_configuration_warning) {
		_update_configuration_warning();
	}
}

bool Resource::editor_can_reload_from_file()
{
	return true; // by default yes
}

Error Resource::copy_from(const Ref<Resource>& p_resource)
{
	ERR_FAIL_COND_V(p_resource.is_null(), ERR_INVALID_PARAMETER);
	_block_emit_changed();
	reset_state(); // May want to reset state.
	_unblock_emit_changed();
	return OK;
}

void Resource::_set_path(const String& p_path) { set_path(p_path, false); }

void Resource::_take_over_path(const String& p_path) { set_path(p_path, true); }

void Resource::set_local_to_scene(bool p_enable) { local_to_scene = p_enable; }

bool Resource::is_local_to_scene() const { return local_to_scene; }

Node* Resource::get_local_scene() const
{
	if (local_scene) {
		return local_scene;
	}

	if (_get_local_scene_func) {
		return _get_local_scene_func();
	}

	return nullptr;
}

void Resource::reset_local_to_scene()
{
	// Restores the state as if setup_local_to_scene() hadn't been called.
}

String Resource::_to_string()
{
	return (name.is_empty() ? "" : String(name) + " ") + "(" + path_cache +
		   ")";
}

Node* (*Resource::_get_local_scene_func)() = nullptr;
void (*Resource::_update_configuration_warning)() = nullptr;

void Resource::set_as_translation_remapped(bool p_remapped)
{
	if (remapped_list.in_list() == p_remapped) {
		return;
	}

	MutexLock lock(ResourceCache::lock);

	if (p_remapped) {
		ResourceLoader::remapped_list.add(&remapped_list);
	}
	else {
		ResourceLoader::remapped_list.remove(&remapped_list);
	}
}

String Resource::get_id_for_path(const String& p_referrer_path) const
{
	return "";
}

void Resource::_bind_methods() {}

Resource::~Resource()
{
	if (unlikely(path_cache.is_empty())) {
		return;
	}

	MutexLock lock(ResourceCache::lock);
	// Only unregister from the cache if this is the actual resource listed there.
	// (Other resources can have the same value in `path_cache` if loaded with `CACHE_IGNORE`.)
	HashMap<String, Resource*>::Iterator E = ResourceCache::resources.find(path_cache);
	if (likely(E && E->value == this)) {
		ResourceCache::resources.remove(E);
	}
}

HashMap<String, Resource*> ResourceCache::resources;
#ifdef TOOLS_ENABLED
HashMap<String, HashMap<String, String>> ResourceCache::resource_path_cache;
#endif

Mutex ResourceCache::lock;

void ResourceCache::clear()
{
	if (!resources.is_empty()) {
		if (OS::get_singleton()->is_stdout_verbose()) {
			ERR_PRINT(vformat("%d resources still in use at exit.", resources.size()));
			for (const KeyValue<String, Resource*>& E : resources) {
				__print_line(vformat("Resource still in use: %s", E.key));
			}
		}
		else {
			ERR_PRINT(vformat("%d resources still in use at exit (run with --verbose for details).",
				resources.size()));
		}
	}

	resources.clear();
}

bool ResourceCache::has(const String& p_path)
{
	Resource** res = nullptr;

	{
		MutexLock mutex_lock(lock);

		res = resources.getptr(p_path);

		if (res && (*res)->get_reference_count() == 0) {
			// This resource is in the process of being deleted, ignore its existence.
			(*res)->path_cache = String();
			resources.erase(p_path);
			res = nullptr;
		}
	}

	if (!res) {
		return false;
	}

	return true;
}

Ref<Resource> ResourceCache::get_ref(const String& p_path)
{
	Ref<Resource> ref;
	{
		MutexLock mutex_lock(lock);
		Resource** res = resources.getptr(p_path);

		if (res) {
			ref = Ref<Resource>(*res);
		}

		if (res && ref.is_null()) {
			// This resource is in the process of being deleted, ignore its existence
			(*res)->path_cache = String();
			resources.erase(p_path);
			res = nullptr;
		}
	}

	return ref;
}

void ResourceCache::get_cached_resources(List<Ref<Resource>>* p_resources)
{
	MutexLock mutex_lock(lock);

	LocalVector<String> to_remove;

	for (KeyValue<String, Resource*>& E : resources) {
		Ref<Resource> ref = Ref<Resource>(E.value);

		if (ref.is_null()) {
			// This resource is in the process of being deleted, ignore its existence
			E.value->path_cache = String();
			to_remove.push_back(E.key);
			continue;
		}

		p_resources->push_back(ref);
	}

	for (const String& E : to_remove) {
		resources.erase(E);
	}
}

int ResourceCache::get_cached_resource_count()
{
	MutexLock mutex_lock(lock);
	return resources.size();
}

void Resource::reset_state()
{
	// stub
}

RID Resource::get_rid() const { return RID(); }


