/**************************************************************************/
/*  resource_loader.cpp                                                   */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_importer.h"
#include "core/os/condition_variable.h"
#include "core/os/os.h"
#include "core/os/safe_binary_mutex.h"
#include "core/string/print_string.h"
#include "core/string/translation_server.h"
#include "core/templates/rb_set.h"
#include "resource_loader.h"
#include "servers/rendering/renderer.h"

#ifdef DEBUG_LOAD_THREADED
#define print_lt(m_text) print_line(m_text)
#else
#define print_lt(m_text)
#endif

Ref<ResourceFormatLoader> ResourceLoader::loader[ResourceLoader::MAX_LOADERS];

int ResourceLoader::loader_count = 0;

bool ResourceFormatLoader::recognize_path(const String& p_path, const String& p_for_type) const
{
	List<String> extensions;
	if (p_for_type.is_empty()) {
		get_recognized_extensions(&extensions);
	}
	else {
		get_recognized_extensions_for_type(p_for_type, &extensions);
	}

	for (const String& E : extensions) {
		const String ext = !E.begins_with(".") ? "." + E : E;
		if (p_path.right(ext.length()).nocasecmp_to(ext) == 0) {
			return true;
		}
	}

	return false;
}

void ResourceFormatLoader::get_classes_used(const String& p_path, HashSet<String>* r_classes)
{
	Vector<String> ret;
	for (int i = 0; i < ret.size(); i++) {
		r_classes->insert(ret[i]);
	}

	String res = get_resource_type(p_path);
	if (!res.is_empty()) {
		r_classes->insert(res);
	}
}

ResourceUID::ID ResourceFormatLoader::get_resource_uid(const String& p_path) const
{
	int64_t uid = ResourceUID::INVALID_ID;
	Ref<FileAccess> file = FileAccess::open(p_path + ".uid", FileAccess::READ);
	if (file.is_valid()) {
		uid = ResourceUID::text_to_id(file->get_line());
	}
	return uid;
}

void ResourceFormatLoader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty() || handles_type(p_type)) {
		get_recognized_extensions(p_extensions);
	}
}

void ResourceLoader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions)
{
	for (int i = 0; i < loader_count; i++) {
		loader[i]->get_recognized_extensions_for_type(p_type, p_extensions);
	}
}

bool ResourceFormatLoader::exists(const String& p_path) const
{
	return FileAccess::exists(p_path); // By default just check file.
}

void ResourceFormatLoader::get_recognized_extensions(List<String>* p_extensions) const
{
	PackedStringArray exts;
	const String* r = exts.ptr();
	for (int i = 0; i < exts.size(); ++i) {
		p_extensions->push_back(r[i]);
	}
}

Error ResourceFormatLoader::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	return OK;
}

void ResourceFormatLoader::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	PackedStringArray deps;
	const String* r = deps.ptr();
	for (int i = 0; i < deps.size(); ++i) {
		p_dependencies->push_back(r[i]);
	}
}

void ResourceLoader::LoadToken::clear() {}

ResourceLoader::LoadToken::~LoadToken() { clear(); }

Ref<Resource> ResourceLoader::_load(const String& p_path, const String& p_original_path,
	const String& p_type_hint, CacheMode p_cache_mode, Error* r_error, bool p_use_sub_threads,
	float* r_progress)
{
	const String& original_path = p_original_path.is_empty() ? p_path : p_original_path;
	load_nesting++;

	print_verbose(vformat("Loading resource: %s remapped: %s", p_path, _path_remap(p_path)));

	// Try all loaders and pick the first match for the type hint
	bool found = false;
	Ref<Resource> res;
	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(p_path, p_type_hint)) {
			continue;
		}
		found = true;
		res = loader[i]->load(
			p_path, original_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
		if (res.is_valid()) {
			break;
		}
	}

	res_ref_overrides.erase(load_nesting);
	load_nesting--;

	if (res.is_valid()) {
		return res;
	}
	else {
		print_verbose(vformat("Failed loading resource: %s", p_path));
	}

#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint()) {
		if (ResourceFormatImporter::get_importer_by_file(p_path).is_valid()) {
			// The format is known to the editor, but the file hasn't been imported
			// (otherwise, ResourceFormatImporter would have been found as a suitable loader).
			found = true;
			if (r_error) {
				*r_error = ERR_FILE_NOT_FOUND;
			}
		}
	}
#endif

	ERR_FAIL_COND_V_MSG(found, Ref<Resource>(), vformat("Failed loading resource: %s.", p_path));

#ifdef TOOLS_ENABLED
	Ref<FileAccess> file_check = FileAccess::create(FileAccess::ACCESS_RESOURCES);
	if (!file_check->file_exists(p_path)) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		ERR_FAIL_V_MSG(
			Ref<Resource>(), vformat("Resource file not found: %s (expected type: %s)", p_path,
								 !p_type_hint.is_empty() ? p_type_hint : "unknown"));
	}
#endif

	if (r_error) {
		*r_error = ERR_FILE_UNRECOGNIZED;
	}
	ERR_FAIL_V_MSG(Ref<Resource>(), vformat("No loader found for resource: %s (expected type: %s)",
										p_path, !p_type_hint.is_empty() ? p_type_hint : "unknown"));
}

// This implementation must allow re-entrancy for a task that started awaiting in a deeper stack
// frame. The load task token must be manually re-referenced before this is called, which includes
// threaded runs.

String ResourceLoader::_validate_local_path(const String& p_path)
{
	ResourceUID::ID uid = ResourceUID::text_to_id(p_path);
	if (uid != ResourceUID::INVALID_ID) {
		return ResourceUID::get_id_path(uid);
	}
	else if (p_path.is_relative_path()) {
		return ("res://" + p_path).simplify_path();
	}
	else {
		return ProjectSettings::localize_path(p_path);
	}
}

Error ResourceLoader::load_threaded_request(
	const String& p_path, const String& p_type_hint, bool p_use_sub_threads, CacheMode p_cache_mode)
{
	Ref<ResourceLoader::LoadToken> token = _load_start(p_path, p_type_hint,
		p_use_sub_threads ? LOAD_THREAD_DISTRIBUTE : LOAD_THREAD_SPAWN_SINGLE, p_cache_mode, true);
	return token.is_valid() ? OK : FAILED;
}

ResourceLoader::LoadToken* ResourceLoader::_load_threaded_request_reuse_user_token(
	const String& p_path)
{
	HashMap<String, LoadToken*>::Iterator E = user_load_tokens.find(p_path);
	if (E) {
		print_verbose("load_threaded_request(): Another threaded load for resource path '" +
					  p_path + "' has been initiated. Not an error.");
		LoadToken* token = E->value;
		token->user_rc++;
		return token;
	}
	else {
		return nullptr;
	}
}

void ResourceLoader::_load_threaded_request_setup_user_token(
	LoadToken* p_token, const String& p_path)
{
	p_token->user_path = p_path;
	p_token->reference(); // Extra RC until all user requests have been gotten.
	p_token->user_rc = 1;
	user_load_tokens[p_path] = p_token;
	print_lt("REQUEST: user load tokens: " + itos(user_load_tokens.size()));
}

void ResourceLoader::resource_changed_emit(Resource* p_source) {}

Ref<Resource> ResourceLoader::load(
	const String& p_path, const String& p_type_hint, CacheMode p_cache_mode, Error* r_error)
{
	if (r_error) {
		*r_error = OK;
	}

	LoadThreadMode thread_mode = LOAD_THREAD_FROM_CURRENT;
	Ref<LoadToken> load_token = _load_start(p_path, p_type_hint, thread_mode, p_cache_mode);
	if (load_token.is_null()) {
		if (r_error) {
			*r_error = FAILED;
		}
		return Ref<Resource>();
	}

	Ref<Resource> res = _load_complete(*load_token.ptr(), r_error);
	return res;
}

Ref<ResourceLoader::LoadToken> ResourceLoader::_load_start(const String& p_path,
	const String& p_type_hint, LoadThreadMode p_thread_mode, CacheMode p_cache_mode,
	bool p_for_user)
{
	String local_path = _validate_local_path(p_path);
	ERR_FAIL_COND_V(local_path.is_empty(), Ref<ResourceLoader::LoadToken>());

	bool ignoring_cache =
		p_cache_mode == CACHE_MODE_IGNORE || p_cache_mode == CACHE_MODE_IGNORE_DEEP;

	Ref<LoadToken> load_token;
	bool must_not_register = false;
	ThreadLoadTask* load_task_ptr = nullptr;
	{
		MutexLock thread_load_lock(thread_load_mutex);

		if (p_for_user) {
			LoadToken* existing_token = _load_threaded_request_reuse_user_token(p_path);
			if (existing_token) {
				return Ref<LoadToken>(existing_token);
			}
		}

		if (!ignoring_cache && thread_load_tasks.has(local_path)) {
			load_token = Ref<LoadToken>(thread_load_tasks[local_path].load_token);
			if (load_token.is_valid()) {
				if (p_for_user) {
					// Load task exists, with no user tokens at the moment.
					// Let's "attach" to it.
					_load_threaded_request_setup_user_token(load_token.ptr(), p_path);
				}
				return load_token;
			}
			else {
				// The token is dying (reached 0 on another thread).
				// Ensure it's killed now so the path can be safely reused right away.
				thread_load_tasks[local_path].load_token->clear();
			}
		}

		load_token.instantiate();
		load_token->local_path = local_path;
		if (p_for_user) {
			_load_threaded_request_setup_user_token(load_token.ptr(), p_path);
		}

		// create load task
		{
			ThreadLoadTask load_task;

			load_task.load_token = load_token.ptr();
			load_task.local_path = local_path;
			load_task.type_hint = p_type_hint;
			load_task.cache_mode = p_cache_mode;
			load_task.use_sub_threads = p_thread_mode == LOAD_THREAD_DISTRIBUTE;
			if (p_cache_mode == CACHE_MODE_REUSE) {
				Ref<Resource> existing = ResourceCache::get_ref(local_path);
				if (existing.is_valid()) {
					// referencing is fine
					load_task.resource = existing;
					load_task.status = THREAD_LOAD_LOADED;
					load_task.progress = 1.0;
					DEV_ASSERT(!thread_load_tasks.has(local_path));
					thread_load_tasks[local_path] = load_task;
					return load_token;
				}
			}

			// Task hierarchy
			if (curr_load_task) {
				load_task.parent_task = curr_load_task;
				curr_load_task->sub_tasks.insert(load_task.local_path);
			}

			// If we want to ignore cache, but there's another task loading it, we can't add this
			// one to the map.
			must_not_register = ignoring_cache && thread_load_tasks.has(local_path);
			if (must_not_register) {
				load_token->task_if_unregistered = memnew(ThreadLoadTask(load_task));
				load_task_ptr = load_token->task_if_unregistered;
			}
			else {
				DEV_ASSERT(!thread_load_tasks.has(local_path));
				HashMap<String, ResourceLoader::ThreadLoadTask>::Iterator E =
					thread_load_tasks.insert(local_path, load_task);
				load_task_ptr = &E->value;
			}
		}

		// It's important to keep the token alive because until the load completes,
		// which includes before the thread start, it may happen that no one is grabbing
		// the token anymore so it's released.
		load_task_ptr->load_token->reference();

		if (p_thread_mode == LOAD_THREAD_FROM_CURRENT) {
			load_task_ptr->thread_id = Thread::get_caller_id();
		}
	} // MutexLock(thread_load_mutex).

	if (p_thread_mode == LOAD_THREAD_FROM_CURRENT) {
		_run_load_task(load_task_ptr);
	}

	return load_token;
}

float ResourceLoader::_dependency_get_progress(const String& p_path)
{
	if (thread_load_tasks.has(p_path)) {
		ThreadLoadTask& load_task = thread_load_tasks[p_path];
		if (load_task.in_progress_check) {
			// Given the fact that any resource loaded when an outer stack frame is
			// loading another one is considered a dependency of it, for progress
			// tracking purposes, a cycle can happen if even if the original resource
			// graphs involved have none. For instance, preload() can cause this.
			return load_task.max_reported_progress;
		}
		load_task.in_progress_check = true;
		float current_progress = 0.0;
		int dep_count = load_task.sub_tasks.size();
		if (dep_count > 0) {
			for (const String& E : load_task.sub_tasks) {
				current_progress += _dependency_get_progress(E);
			}
			current_progress /= float(dep_count);
			current_progress *= 0.5;
			current_progress += load_task.progress * 0.5;
		}
		else {
			current_progress = load_task.progress;
		}
		load_task.max_reported_progress = MAX(load_task.max_reported_progress, current_progress);
		load_task.in_progress_check = false;
		return load_task.max_reported_progress;
	}
	else {
		return 1.0; // assume finished loading it so it no longer exists
	}
}

ResourceLoader::ThreadLoadStatus ResourceLoader::load_threaded_get_status(
	const String& p_path, float* r_progress)
{
	bool ensure_progress = false;
	ThreadLoadStatus status = THREAD_LOAD_IN_PROGRESS;
	{
		MutexLock thread_load_lock(thread_load_mutex);

		if (!user_load_tokens.has(p_path)) {
			print_verbose("load_threaded_get_status(): No threaded load for resource path '" +
						  p_path +
						  "' has been initiated or its result has already been collected.");
			return THREAD_LOAD_INVALID_RESOURCE;
		}

		String local_path = _validate_local_path(p_path);
		LoadToken* load_token = user_load_tokens[p_path];
		ThreadLoadTask* load_task_ptr;

		if (load_token->task_if_unregistered) {
			load_task_ptr = load_token->task_if_unregistered;
		}
		else {
			ERR_FAIL_COND_V_MSG(!thread_load_tasks.has(local_path), THREAD_LOAD_INVALID_RESOURCE,
				"Bug in ResourceLoader logic, please report.");
			load_task_ptr = &thread_load_tasks[local_path];
		}

		status = load_task_ptr->status;
		if (r_progress) {
			*r_progress = _dependency_get_progress(local_path);
		}

		// Support userland polling in a loop on the main thread.
		if (Thread::is_main_thread() && status == THREAD_LOAD_IN_PROGRESS) {
			uint64_t frame = Engine::get_process_frames();
			if (frame == load_task_ptr->last_progress_check_main_thread_frame) {
				ensure_progress = true;
			}
			else {
				load_task_ptr->last_progress_check_main_thread_frame = frame;
			}
		}
	}

	if (ensure_progress) {
		_ensure_load_progress();
	}

	return status;
}

Ref<Resource> ResourceLoader::load_threaded_get(const String& p_path, Error* r_error)
{
	if (r_error) {
		*r_error = OK;
	}

	Ref<Resource> res;
	{
		MutexLock thread_load_lock(thread_load_mutex);

		if (!user_load_tokens.has(p_path)) {
			print_verbose("load_threaded_get(): No threaded load for resource path '" + p_path +
						  "' has been initiated or its result has already been collected.");
			if (r_error) {
				*r_error = ERR_INVALID_PARAMETER;
			}
			return Ref<Resource>();
		}

		LoadToken* load_token = user_load_tokens[p_path];
		DEV_ASSERT(load_token->user_rc >= 1);

		// Support userland requesting on the main thread before the load is reported to be
		// complete.
		if (Thread::is_main_thread() && !load_token->local_path.is_empty()) {
			ThreadLoadTask* load_task_ptr;

			if (load_token->task_if_unregistered) {
				load_task_ptr = load_token->task_if_unregistered;
			}
			else {
				if (!thread_load_tasks.has(load_token->local_path)) {
					print_error("Bug in ResourceLoader logic, please report.");
					if (r_error) {
						*r_error = ERR_BUG;
					}
					return Ref<Resource>();
				}

				load_task_ptr = &thread_load_tasks[load_token->local_path];
			}

			while (load_task_ptr->status == THREAD_LOAD_IN_PROGRESS) {
				thread_load_lock.temp_unlock();
				bool exit = !_ensure_load_progress();
				OS::delay_usec(1000);

				thread_load_lock.temp_relock();
				if (exit) {
					break;
				}
			}
		}

		res = _load_complete_inner(*load_token, r_error, thread_load_lock);

		load_token->user_rc--;
		if (load_token->user_rc == 0) {
			load_token->user_path.clear();
			user_load_tokens.erase(p_path);
			if (load_token->unreference()) {
				memdelete(load_token);
				load_token = nullptr;
			}
		}
	}

	print_lt("GET: user load tokens: " + itos(user_load_tokens.size()));

	return res;
}

Ref<Resource> ResourceLoader::_load_complete(LoadToken& p_load_token, Error* r_error)
{
	MutexLock thread_load_lock(thread_load_mutex);
	return _load_complete_inner(p_load_token, r_error, thread_load_lock);
}

void ResourceLoader::set_is_import_thread(bool p_import_thread) { import_thread = p_import_thread; }

void ResourceLoader::notify_dependency_error(
	const String& p_path, const String& p_dependency, const String& p_type)
{
	if (dep_err_notify) {
		if (Thread::get_caller_id() == Thread::get_main_id()) {
			dep_err_notify(p_path, p_dependency, p_type);
		}
	}
}

Ref<Resource> ResourceLoader::_load_complete_inner(LoadToken& p_load_token, Error* r_error,
	MutexLock<SafeBinaryMutex<BINARY_MUTEX_TAG>>& p_thread_load_lock)
{
	if (r_error) {
		*r_error = OK;
	}

	ThreadLoadTask* load_task_ptr = nullptr;
	if (p_load_token.task_if_unregistered) {
		load_task_ptr = p_load_token.task_if_unregistered;
	}
	else {
		if (!thread_load_tasks.has(p_load_token.local_path)) {
			if (r_error) {
				*r_error = ERR_BUG;
			}
			ERR_FAIL_V_MSG(Ref<Resource>(), "Bug in ResourceLoader logic, please report.");
		}

		ThreadLoadTask& load_task = thread_load_tasks[p_load_token.local_path];

		if (load_task.status == THREAD_LOAD_IN_PROGRESS) {
			DEV_ASSERT((load_task.task_id == 0) != (load_task.thread_id == 0));
			if (load_task.need_wait) {
				// Loading thread is main or user thread.
				if (!load_task.cond_var) {
					load_task.cond_var = memnew(ConditionVariable);
				}
				load_task.awaiters_count++;
				do {
					load_task.cond_var->wait(p_thread_load_lock);
					DEV_ASSERT(thread_load_tasks.has(p_load_token.local_path) &&
							   p_load_token.get_reference_count());
				} while (load_task.need_wait);
				load_task.awaiters_count--;
				if (load_task.awaiters_count == 0) {
					memdelete(load_task.cond_var);
					load_task.cond_var = nullptr;
				}

				DEV_ASSERT(load_task.status == THREAD_LOAD_FAILED ||
						   load_task.status == THREAD_LOAD_LOADED);
			}
		}

		if (cleaning_tasks) {
			load_task.resource = Ref<Resource>();
			load_task.error = FAILED;
		}

		load_task_ptr = &load_task;
	}

	Ref<Resource> resource = load_task_ptr->resource;
	if (r_error) {
		*r_error = load_task_ptr->error;
	}

	if (resource.is_valid()) {
		if (load_task_ptr->parent_task) {
			if (!load_task_ptr->connections_propagated) {
				// A task awaiting another => Let the awaiter accumulate the resource changed
				// connections.
				DEV_ASSERT(load_task_ptr->parent_task != load_task_ptr);
				for (const ThreadLoadTask::ResourceChangedConnection& rcc :
					load_task_ptr->resource_changed_connections) {
					load_task_ptr->parent_task->resource_changed_connections.push_back(rcc);
				}
				load_task_ptr->connections_propagated = true;
			}
		}
		else {
			p_thread_load_lock.temp_unlock();

			// A leaf task being awaited => Propagate the resource changed connections.
			if (Thread::is_main_thread()) {
				// On the main thread it's safe to migrate the connections to the standard signal
				// mechanism.
				for (const ThreadLoadTask::ResourceChangedConnection& rcc :
					load_task_ptr->resource_changed_connections) {
				}
			}
			else {
				// On non-main threads, we have to queue and call it done when processed.
				if (!load_task_ptr->resource_changed_connections.is_empty()) {
					for (const ThreadLoadTask::ResourceChangedConnection& rcc :
						load_task_ptr->resource_changed_connections) {
					}
				}
			}

			p_thread_load_lock.temp_relock();
		}
	}

	return resource;
}

bool ResourceLoader::_ensure_load_progress()
{
	// Some servers may need a new engine iteration to allow the load to progress.
	// Since the only known one is the rendering server (in single thread mode), let's keep it
	// simple and just sync it. This may be refactored in the future to support other servers and
	// have less coupling.
	if (OS::is_separate_thread_rendering_enabled()) {
		return false; // Not needed.
	}
	Renderer::sync();
	return true;
}

Ref<Resource> ResourceLoader::ensure_resource_ref_override_for_outer_load(
	const String& p_path, const String& p_res_type)
{
	ERR_FAIL_COND_V(
		load_nesting == 0, Ref<Resource>()); // It makes no sense to use this from nesting level 0.
	const String& local_path = _validate_local_path(p_path);
	HashMap<String, Ref<Resource>>& overrides = res_ref_overrides[load_nesting - 1];
	HashMap<String, Ref<Resource>>::Iterator E = overrides.find(local_path);
	if (E) {
		return E->value;
	}
	else {
		return nullptr;
	}
}

Ref<Resource> ResourceLoader::get_resource_ref_override(const String& p_path)
{
	DEV_ASSERT(p_path == _validate_local_path(p_path));
	HashMap<int, HashMap<String, Ref<Resource>>>::Iterator E = res_ref_overrides.find(load_nesting);
	if (!E) {
		return nullptr;
	}
	HashMap<String, Ref<Resource>>::Iterator F = E->value.find(p_path);
	if (!F) {
		return nullptr;
	}

	return F->value;
}

bool ResourceLoader::exists(const String& p_path, const String& p_type_hint)
{
	String local_path = _validate_local_path(p_path);

	if (ResourceCache::has(local_path)) {
		return true; // If cached, it probably exists
	}

	String path = _path_remap(local_path);

	// Try all loaders and pick the first match for the type hint
	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(path, p_type_hint)) {
			continue;
		}

		if (loader[i]->exists(path)) {
			return true;
		}
	}

	return false;
}

void ResourceLoader::add_resource_format_loader(
	Ref<ResourceFormatLoader> p_format_loader, bool p_at_front)
{
	ERR_FAIL_COND(p_format_loader.is_null());
	ERR_FAIL_COND(loader_count >= MAX_LOADERS);

	if (p_at_front) {
		for (int i = loader_count; i > 0; i--) {
			loader[i] = loader[i - 1];
		}
		loader[0] = p_format_loader;
		loader_count++;
	}
	else {
		loader[loader_count++] = p_format_loader;
	}
}

void ResourceLoader::remove_resource_format_loader(Ref<ResourceFormatLoader> p_format_loader)
{
	ERR_FAIL_COND(p_format_loader.is_null());

	// Find loader
	int i = 0;
	for (; i < loader_count; ++i) {
		if (loader[i] == p_format_loader) {
			break;
		}
	}

	ERR_FAIL_COND(i >= loader_count); // Not found

	// Shift next loaders up
	for (; i < loader_count - 1; ++i) {
		loader[i] = loader[i + 1];
	}
	loader[loader_count - 1].unref();
	--loader_count;
}

String ResourceLoader::get_import_group_file(const String& p_path)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->get_import_group_file(p_path);
	}

	return String(); // not found
}

bool ResourceLoader::is_import_valid(const String& p_path)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->is_import_valid(p_path);
	}

	return false; // not found
}

bool ResourceLoader::is_imported(const String& p_path)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->is_imported(p_path);
	}

	return false; // not found
}

void ResourceLoader::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		loader[i]->get_dependencies(local_path, p_dependencies, p_add_types);
	}
}

Error ResourceLoader::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	String local_path = _path_remap(_validate_local_path(p_path));

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->rename_dependencies(local_path, p_map);
	}

	return OK; // ??
}

void ResourceLoader::get_classes_used(const String& p_path, HashSet<String>* r_classes)
{
	String local_path = _validate_local_path(p_path);

	for (int i = 0; i < loader_count; i++) {
		if (!loader[i]->recognize_path(local_path)) {
			continue;
		}

		return loader[i]->get_classes_used(p_path, r_classes);
	}
}

String ResourceLoader::get_resource_type(const String& p_path)
{
	String local_path = _validate_local_path(p_path);

	for (int i = 0; i < loader_count; i++) {
		String result = loader[i]->get_resource_type(local_path);
		if (!result.is_empty()) {
			return result;
		}
	}

	return "";
}

String ResourceLoader::get_resource_script_class(const String& p_path)
{
	String local_path = _validate_local_path(p_path);

	for (int i = 0; i < loader_count; i++) {
		String result = loader[i]->get_resource_script_class(local_path);
		if (!result.is_empty()) {
			return result;
		}
	}

	return "";
}

ResourceUID::ID ResourceLoader::get_resource_uid(const String& p_path)
{
	const String local_path = _validate_local_path(p_path);
	if (!Engine::is_editor_hint()) {
		return ResourceUID::get_path_id(local_path);
	}

	for (int i = 0; i < loader_count; i++) {
		ResourceUID::ID id = loader[i]->get_resource_uid(local_path);
		if (id != ResourceUID::INVALID_ID) {
			return id;
		}
	}

	return ResourceUID::INVALID_ID;
}

bool ResourceLoader::should_create_uid_file(const String& p_path)
{
	const String local_path = _validate_local_path(p_path);
	if (FileAccess::exists(local_path + ".uid")) {
		return false;
	}

	for (int i = 0; i < loader_count; i++) {
		if (loader[i]->recognize_path(local_path)) {
			return !loader[i]->has_custom_uid_support();
		}
	}
	return false;
}

String ResourceLoader::_path_remap(const String& p_path, bool* r_translation_remapped)
{
	String new_path = p_path;

	if (translation_remaps.has(p_path)) {
		// translation_remaps has the following format:
		//   { "res://path.png": PackedStringArray( "res://path-ru.png:ru", "res://path-de.png:de" )
		//   }

		// To find the path of the remapped resource, we extract the locale name after
		// the last ':' to match the project locale.

		// An extra remap may still be necessary afterwards due to the text -> binary converter on
		// export.

		String locale = TranslationServer::get_locale();
		ERR_FAIL_COND_V_MSG(locale.length() < 2, p_path,
			vformat(
				"Could not remap path '%s' for translation as configured locale '%s' is invalid.",
				p_path, locale));

		Vector<String>& res_remaps = *translation_remaps.getptr(new_path);

		int best_score = 0;
		for (int i = 0; i < res_remaps.size(); i++) {
			int split = res_remaps[i].rfind_char(':');
			if (split == -1) {
				continue;
			}
			String l = res_remaps[i].substr(split + 1).strip_edges();
			int score = TranslationServer::compare_locales(locale, l);
			if (score > 0 && score >= best_score) {
				new_path = res_remaps[i].left(split);
				best_score = score;
				if (score == 10) {
					break; // Exact match, skip the rest.
				}
			}
		}

		if (r_translation_remapped) {
			*r_translation_remapped = true;
		}

		// Fallback to p_path if new_path does not exist.
		if (!FileAccess::exists(new_path + ".import") && !FileAccess::exists(new_path + ".remap") &&
			!FileAccess::exists(new_path)) {
			WARN_PRINT(vformat(
				"Translation remap '%s' does not exist. Falling back to '%s'.", new_path, p_path));
			new_path = p_path;
		}
	}

	// Usually, there's no remap file and FileAccess::exists() is faster than FileAccess::open().
	new_path = ResourceUID::ensure_path(new_path);

	return new_path;
}

String ResourceLoader::import_remap(const String& p_path)
{
	if (ResourceFormatImporter::recognize_path(p_path)) {
		return ResourceFormatImporter::get_internal_resource_path(p_path);
	}

	return p_path;
}

String ResourceLoader::path_remap(const String& p_path) { return _path_remap(p_path); }

void ResourceLoader::reload_translation_remaps()
{
	List<Resource*> to_reload;

	{
		MutexLock lock(ResourceCache::lock);
		SelfList<Resource>* E = remapped_list.first();

		while (E) {
			to_reload.push_back(E->self());
			E = E->next();
		}
	}

	// now just make sure to not delete any of these resources while changing locale..
	while (to_reload.front()) {
		to_reload.front()->get()->reload_from_file();
		to_reload.pop_front();
	}
}

void ResourceLoader::clear_translation_remaps()
{
	translation_remaps.clear();
	while (remapped_list.first() != nullptr) {
		remapped_list.remove(remapped_list.first());
	}
}

void ResourceLoader::clear_thread_load_tasks()
{
	// Bring the thing down as quickly as possible without causing deadlocks or leaks.

	MutexLock thread_load_lock(thread_load_mutex);
	cleaning_tasks = true;

	while (true) {
		bool none_running = true;
		if (thread_load_tasks.size()) {
			for (KeyValue<String, ResourceLoader::ThreadLoadTask>& E : thread_load_tasks) {
				if (E.value.status == THREAD_LOAD_IN_PROGRESS) {
					if (E.value.cond_var && E.value.need_wait) {
						E.value.cond_var->notify_all();
					}
					E.value.need_wait = false;
					none_running = false;
				}
			}
		}
		if (none_running) {
			break;
		}

		thread_load_lock.temp_unlock();

		OS::delay_usec(1000);

		thread_load_lock.temp_relock();
	}

	while (user_load_tokens.begin()) {
		LoadToken* user_token = user_load_tokens.begin()->value;
		user_load_tokens.remove(user_load_tokens.begin());
		DEV_ASSERT(user_token->user_rc > 0 && !user_token->user_path.is_empty());
		user_token->user_path.clear();
		user_token->user_rc = 0;
		user_token->unreference();
	}

	thread_load_tasks.clear();
	thread_waiting_on.clear();
	// yielders is already guaranteed to be empty now

	cleaning_tasks = false;
}

void ResourceLoader::set_load_callback(ResourceLoadedCallback p_callback)
{
	_loaded_callback = p_callback;
}

ResourceLoadedCallback ResourceLoader::_loaded_callback = nullptr;

void ResourceLoader::set_create_missing_resources_if_class_unavailable(bool p_enable)
{
	create_missing_resources_if_class_unavailable = p_enable;
}

void ResourceLoader::remove_custom_loaders()
{
	Vector<Ref<ResourceFormatLoader>> custom_loaders;
	for (int i = 0; i < custom_loaders.size(); ++i) {
		remove_resource_format_loader(custom_loaders[i]);
	}
}

bool ResourceLoader::is_cleaning_tasks()
{
	MutexLock lock(thread_load_mutex);
	return cleaning_tasks;
}

Vector<String> ResourceLoader::list_directory(const String& p_directory)
{
	RBSet<String> files_found;
	Ref<DirAccess> dir = DirAccess::open(p_directory);
	if (dir.is_null()) {
		return Vector<String>();
	}

	Error err = dir->list_dir_begin();
	if (err != OK) {
		return Vector<String>();
	}

	String d = dir->get_next();
	while (!d.is_empty()) {
		bool recognized = false;
		if (dir->current_is_dir()) {
			if (d != "." && d != "..") {
				d += "/";
				recognized = true;
			}
		}
		else {
			if (d.ends_with(".import") || d.ends_with(".remap") || d.ends_with(".uid")) {
				d = d.substr(0, d.rfind_char('.'));
			}

			if (d.ends_with(".gdc")) {
				d = d.substr(0, d.rfind_char('.'));
				d += ".gd";
			}

			const String full_path = p_directory.path_join(d);
			// Try all loaders and pick the first match for the type hint.
			for (int i = 0; i < loader_count; i++) {
				if (loader[i]->recognize_path(full_path)) {
					recognized = true;
					break;
				}
			}
		}

		if (recognized) {
			files_found.insert(d);
		}
		d = dir->get_next();
	}

	Vector<String> ret;
	for (const String& f : files_found) {
		ret.push_back(f);
	}

	return ret;
}

void ResourceLoader::initialize() {}

void ResourceLoader::finalize() {}

ResourceLoadErrorNotify ResourceLoader::err_notify = nullptr;
DependencyErrorNotify ResourceLoader::dep_err_notify = nullptr;

bool ResourceLoader::create_missing_resources_if_class_unavailable = false;
bool ResourceLoader::abort_on_missing_resource = true;
bool ResourceLoader::timestamp_on_load = false;

thread_local bool ResourceLoader::import_thread = false;
thread_local int ResourceLoader::load_nesting = 0;
thread_local HashMap<int, HashMap<String, Ref<Resource>>> ResourceLoader::res_ref_overrides;
thread_local ResourceLoader::ThreadLoadTask* ResourceLoader::curr_load_task = nullptr;

SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG>& _get_res_loader_mutex()
{
	return ResourceLoader::thread_load_mutex;
}

template <>
thread_local SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG>::TLSData
	SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG>::tls_data(_get_res_loader_mutex());
SafeBinaryMutex<ResourceLoader::BINARY_MUTEX_TAG> ResourceLoader::thread_load_mutex;
HashMap<String, ResourceLoader::ThreadLoadTask> ResourceLoader::thread_load_tasks;
HashMap<int, String> ResourceLoader::thread_waiting_on;
LocalVector<int> ResourceLoader::yielders;

bool ResourceLoader::cleaning_tasks = false;

HashMap<String, ResourceLoader::LoadToken*> ResourceLoader::user_load_tokens;

SelfList<Resource>::List ResourceLoader::remapped_list;
HashMap<String, Vector<String>> ResourceLoader::translation_remaps;

ResourceLoaderImport ResourceLoader::import = nullptr;

bool ResourceFormatLoader::handles_type(const String& p_type) const { return false; }

String ResourceFormatLoader::get_resource_type(const String& p_path) const { return String(); }

String ResourceFormatLoader::get_resource_script_class(const String& p_path) const
{
	return String();
}

bool ResourceFormatLoader::has_custom_uid_support() const { return false; }

void ResourceLoader::_run_load_task(void* p_userdata) {}

void ResourceLoader::load_translation_remaps() {}


Ref<Resource> ResourceFormatLoader::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress,
	CacheMode p_cache_mode)
{
	Ref<Resource> r = memnew(Resource);
	return r;
}
#ifdef print_lt
#undef print_lt
#endif

// ==========================================================================
// resource_saver.cpp
// ==========================================================================

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

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "resource_saver.h"

Ref<ResourceFormatSaver> ResourceSaver::saver[MAX_SAVERS];

int ResourceSaver::saver_count = 0;
bool ResourceSaver::timestamp_on_save = false;
ResourceSavedCallback ResourceSaver::save_callback = nullptr;
ResourceSaverGetResourceIDForPath ResourceSaver::save_get_id_for_path = nullptr;

Error ResourceFormatSaver::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Error err = ERR_METHOD_NOT_FOUND;
	return err;
}

Error ResourceFormatSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	Error err = ERR_FILE_UNRECOGNIZED;
	return err;
}

bool ResourceFormatSaver::recognize(const Ref<Resource>& p_resource) const
{
	bool success = false;
	return success;
}

void ResourceFormatSaver::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	PackedStringArray exts;
	const String* r = exts.ptr();
	for (int i = 0; i < exts.size(); ++i) {
		p_extensions->push_back(r[i]);
	}
}

bool ResourceFormatSaver::recognize_path(
	const Ref<Resource>& p_resource, const String& p_path) const
{
	String extension = p_path.get_extension();
	List<String> extensions;
	get_recognized_extensions(p_resource, &extensions);
	for (const String& E : extensions) {
		if (E.nocasecmp_to(extension) == 0) {
			return true;
		}
	}
	return false;
}

Error ResourceSaver::save(Resource* rp_resource, const String& p_path, uint32_t p_flags)
{
	String path = p_path;
	if (path.is_empty()) {
		path = rp_resource->get_path();
	}
	ERR_FAIL_COND_V_MSG(path.is_empty(), ERR_INVALID_PARAMETER,
		"Can't save resource to empty path. Provide non-empty path or a Resource with non-empty "
		"resource_path.");
	Error err = ERR_FILE_UNRECOGNIZED;
	for (int i = 0; i < saver_count; i++) {
		if (!saver[i]->recognize(rp_resource)) {
			continue;
		}

		if (!saver[i]->recognize_path(rp_resource, path)) {
			continue;
		}

		String old_path = rp_resource->get_path();

		String local_path = ProjectSettings::localize_path(path);

		if (p_flags & FLAG_CHANGE_PATH) {
			rp_resource->set_path(local_path);
		}

		err = saver[i]->save(rp_resource, path, p_flags);

		if (err == OK) {
			if (p_flags & FLAG_CHANGE_PATH) {
				rp_resource->set_path(old_path);
			}

			if (save_callback && path.begins_with("res://")) {
				save_callback(rp_resource, path);
			}

			return OK;
		}
	}

	return err;
}

Error ResourceSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String path = p_path;

	ERR_FAIL_COND_V_MSG(path.is_empty(), ERR_INVALID_PARAMETER,
		"Can't update UID to empty path. Provide non-empty path.");

	Error err = ERR_FILE_UNRECOGNIZED;

	for (int i = 0; i < saver_count; i++) {
		err = saver[i]->set_uid(path, p_uid);
		if (err == OK) {
			break;
		}
	}

	return err;
}

void ResourceSaver::set_save_callback(ResourceSavedCallback p_callback)
{
	save_callback = p_callback;
}

void ResourceSaver::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions)
{
	ERR_FAIL_COND_MSG(p_resource.is_null(), "It's not a reference to a valid Resource object.");
	for (int i = 0; i < saver_count; i++) {
		saver[i]->get_recognized_extensions(p_resource, p_extensions);
	}
}

void ResourceSaver::add_resource_format_saver(
	Ref<ResourceFormatSaver> p_format_saver, bool p_at_front)
{
	ERR_FAIL_COND_MSG(
		p_format_saver.is_null(), "It's not a reference to a valid ResourceFormatSaver object.");
	ERR_FAIL_COND(saver_count >= MAX_SAVERS);

	if (p_at_front) {
		for (int i = saver_count; i > 0; i--) {
			saver[i] = saver[i - 1];
		}
		saver[0] = p_format_saver;
		saver_count++;
	}
	else {
		saver[saver_count++] = p_format_saver;
	}
}

void ResourceSaver::remove_resource_format_saver(Ref<ResourceFormatSaver> p_format_saver)
{
	ERR_FAIL_COND_MSG(
		p_format_saver.is_null(), "It's not a reference to a valid ResourceFormatSaver object.");

	// Find saver
	int i = 0;
	for (; i < saver_count; ++i) {
		if (saver[i] == p_format_saver) {
			break;
		}
	}

	ERR_FAIL_COND(i >= saver_count); // Not found

	// Shift next savers up
	for (; i < saver_count - 1; ++i) {
		saver[i] = saver[i + 1];
	}
	saver[saver_count - 1].unref();
	--saver_count;
}

bool ResourceSaver::add_custom_resource_format_saver(const String& p_script_path)
{
	if (_find_custom_resource_format_saver(p_script_path).is_valid()) {
		return false;
	}

	Ref<Resource> res = ResourceLoader::load(p_script_path);
	ERR_FAIL_COND_V(res.is_null(), false);

	return true;
}

void ResourceSaver::remove_custom_savers()
{
	Vector<Ref<ResourceFormatSaver>> custom_savers;
	for (int i = 0; i < custom_savers.size(); ++i) {
		remove_resource_format_saver(custom_savers[i]);
	}
}

ResourceUID::ID ResourceSaver::get_resource_id_for_path(const String& p_path, bool p_generate)
{
	if (save_get_id_for_path) {
		return save_get_id_for_path(p_path, p_generate);
	}
	return ResourceUID::INVALID_ID;
}

void ResourceSaver::set_get_resource_id_for_path(ResourceSaverGetResourceIDForPath p_callback)
{
	save_get_id_for_path = p_callback;
}

Ref<ResourceFormatSaver> ResourceSaver::_find_custom_resource_format_saver(const String& p_path)
{
	Ref<ResourceFormatSaver> rfs = memnew(ResourceFormatSaver);
	return rfs;
}

// ==========================================================================
// resource_uid.cpp
// ==========================================================================

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

#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/math/random_pcg.h"
#include "resource_uid.h"

// These constants are off by 1, causing the 'z' and '9' characters never to be used.
// This cannot be fixed without breaking compatibility; see GH-83843.
static constexpr uint32_t uid_char_count = ('z' - 'a');
static constexpr uint32_t uid_base = uid_char_count + ('9' - '0');

String ResourceUID::get_cache_file()
{
	return ProjectSettings::get_project_data_path().path_join("uid_cache.bin");
}

static constexpr uint8_t uuid_characters[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
	'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', '0', '1', '2', '3', '4',
	'5', '6', '7', '8'};
static constexpr uint32_t uuid_characters_element_count = std_size(uuid_characters);
static constexpr uint8_t max_uuid_number_length =
	13; // Max 0x7FFFFFFFFFFFFFFF (uid://d4n4ub6itg400) size is 13 characters.

String ResourceUID::id_to_text(ID p_id)
{
	if (p_id < 0) {
		return "uid://<invalid>";
	}

	char32_t tmp[max_uuid_number_length];
	uint32_t tmp_size = 0;
	do {
		uint32_t c = p_id % uuid_characters_element_count;
		tmp[tmp_size] = uuid_characters[c];
		p_id /= uuid_characters_element_count;
		++tmp_size;
	} while (p_id);

	// tmp_size + uid:// (6) + 1 for null.
	String txt;
	txt.resize_uninitialized(tmp_size + 7);

	char32_t* p = txt.ptrw();
	p[0] = 'u';
	p[1] = 'i';
	p[2] = 'd';
	p[3] = ':';
	p[4] = '/';
	p[5] = '/';
	uint32_t size = 6;

	// The above loop give the number backward, recopy it in the string in the correct order.
	for (uint32_t i = 0; i < tmp_size; ++i) {
		p[size++] = tmp[tmp_size - i - 1];
	}

	p[size] = 0;

	return txt;
}

ResourceUID::ID ResourceUID::text_to_id(const String& p_text)
{
	if (!p_text.begins_with("uid://") || p_text == "uid://<invalid>") {
		return INVALID_ID;
	}

	uint32_t l = p_text.length();
	uint64_t uid = 0;
	for (uint32_t i = 6; i < l; i++) {
		uid *= uid_base;
		uint32_t c = p_text[i];
		if (is_ascii_lower_case(c)) {
			uid += c - 'a';
		}
		else if (is_digit(c)) {
			uid += c - '0' + uid_char_count;
		}
		else {
			return INVALID_ID;
		}
	}
	return ID(uid & 0x7FFFFFFFFFFFFFFF);
}

ResourceUID::ID ResourceUID::create_id()
{
	while (true) {
		ID id = INVALID_ID;
		MutexLock lock(data->mutex);
		Error err = CryptoCore::generate_random((uint8_t*)&id, sizeof(id));
		ERR_FAIL_COND_V(err != OK, INVALID_ID);
		id &= 0x7FFFFFFFFFFFFFFF;
		bool exists = data->unique_ids.has(id);
		if (!exists) {
			return id;
		}
	}
}

ResourceUID::ID ResourceUID::create_id_for_path(const String& p_path)
{
	ID id = INVALID_ID;
	RandomPCG rng;
	rng.seed(
		p_path.hash64() * p_path.to_lower().hash64() * FileAccess::get_md5(p_path).hash64());

	while (true) {
		int64_t num1 = rng.rand();
		int64_t num2 = ((int64_t)rng.rand()) << 32;
		id = (num1 | num2) & 0x7FFFFFFFFFFFFFFF;

		MutexLock lock(data->mutex);
		if (!data->unique_ids.has(id)) {
			break;
		}
	}
	return id;
}

bool ResourceUID::has_id(ID p_id)
{
	MutexLock l(data->mutex);
	return data->unique_ids.has(p_id);
}

void ResourceUID::add_id(ID p_id, const String& p_path)
{
	MutexLock l(data->mutex);
	ERR_FAIL_COND(data->unique_ids.has(p_id));
	Cache c;
	c.cs = p_path.utf8();
	data->unique_ids[p_id] = c;
	if (data->use_reverse_cache) {
		data->reverse_cache[c.cs] = p_id;
	}
	data->changed = true;
	// The cache was never loaded (probably does not exist), so assume that first ID initializes it.
	data->cache_initialized = true;
}

void ResourceUID::set_id(ID p_id, const String& p_path)
{
	MutexLock l(data->mutex);
	ERR_FAIL_COND(!data->unique_ids.has(p_id));
	CharString cs = p_path.utf8();
	const char* update_ptr = cs.ptr();
	const char* cached_ptr = data->unique_ids[p_id].cs.ptr();
	if (update_ptr == nullptr && cached_ptr == nullptr) {
		return; // Both are empty strings.
	}
	if ((update_ptr == nullptr) != (cached_ptr == nullptr) || strcmp(update_ptr, cached_ptr) != 0) {
		data->unique_ids[p_id].cs = cs;
		data->unique_ids[p_id].saved_to_cache = false; // changed
		if (data->use_reverse_cache) {
			data->reverse_cache[cs] = p_id;
		}
		data->changed = true;
	}
}

String ResourceUID::get_id_path(ID p_id)
{
	ERR_FAIL_COND_V_MSG(p_id == INVALID_ID, String(), "Invalid UID.");
	MutexLock l(data->mutex);
	const ResourceUID::Cache* cache = data->unique_ids.getptr(p_id);

#if TOOLS_ENABLED
	// On startup, the scan_for_uid_on_startup callback should be set and will
	// execute EditorFileSystem::scan_for_uid, which scans all project files
	// to reload the UID cache before the first scan.
	// Note: EditorFileSystem::scan_for_uid sets scan_for_uid_on_startup to nullptr
	//       once the first scan_for_uid is complete.
	if (!cache && scan_for_uid_on_startup) {
		scan_for_uid_on_startup();
		cache = data->unique_ids.getptr(p_id);
	}
#endif

	if (unlikely(!cache)) {
		if (data->cache_initialized) {
			ERR_PRINT(vformat("Unrecognized UID: \"%s\".", id_to_text(p_id)));
		}
		return String();
	}
	const CharString& cs = cache->cs;
	return String::utf8(cs.ptr());
}

ResourceUID::ID ResourceUID::get_path_id(const String& p_path)
{
	const ID* id = data->reverse_cache.getptr(p_path.utf8());
	if (id) {
		return *id;
	}
	return INVALID_ID;
}

void ResourceUID::remove_id(ID p_id)
{
	MutexLock l(data->mutex);
	ERR_FAIL_COND(!data->unique_ids.has(p_id));
	if (data->use_reverse_cache) {
		data->reverse_cache.erase(data->unique_ids[p_id].cs);
	}
	data->unique_ids.erase(p_id);
}

String ResourceUID::uid_to_path(const String& p_uid)
{
	return get_id_path(text_to_id(p_uid));
}

String ResourceUID::path_to_uid(const String& p_path)
{
	const ID id = ResourceLoader::get_resource_uid(p_path);
	if (id == INVALID_ID) {
		return p_path;
	}
	else {
		return id_to_text(id);
	}
}

String ResourceUID::ensure_path(const String& p_uid_or_path)
{
	if (p_uid_or_path.begins_with("uid://")) {
		return uid_to_path(p_uid_or_path);
	}
	return p_uid_or_path;
}

Vector<uint8_t> ResourceUID::encode_binary_cache(const Vector<Pair<ID, String>>& p_entries)
{
	Ref<StreamPeerBuffer> buffer;
	buffer.instantiate();
	buffer->put_u32(p_entries.size());

	for (const Pair<ID, String>& entry : p_entries) {
		buffer->put_u64(uint64_t(entry.first));
		CharString cs = entry.second.utf8();
		buffer->put_u32(cs.length());
		buffer->put_data((const uint8_t*)cs.ptr(), cs.length());
	}

	return buffer->get_data_array();
}

Error ResourceUID::save_to_cache()
{
	String cache_file = get_cache_file();
	if (!FileAccess::exists(cache_file)) {
		Ref<DirAccess> d = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		d->make_dir_recursive(String(cache_file).get_base_dir()); // ensure base dir exists
	}

	Ref<FileAccess> f = FileAccess::open(cache_file, FileAccess::WRITE);
	if (f.is_null()) {
		return ERR_CANT_OPEN;
	}

	MutexLock l(data->mutex);

	Vector<Pair<ID, String>> entries;
	entries.reserve(data->unique_ids.size());
	data->cache_entries = 0;

	for (KeyValue<ID, Cache>& E : data->unique_ids) {
		entries.push_back(
			Pair<ID, String>(E.key, String::utf8(E.value.cs.ptr(), E.value.cs.length())));
		E.value.saved_to_cache = true;
		data->cache_entries++;
	}

	Vector<uint8_t> encoded_data = encode_binary_cache(entries);
	f->store_buffer(encoded_data.ptr(), encoded_data.size());

	data->changed = false;
	return OK;
}

Error ResourceUID::load_from_cache(bool p_reset)
{
	Ref<FileAccess> f = FileAccess::open(get_cache_file(), FileAccess::READ);
	if (f.is_null()) {
		return ERR_CANT_OPEN;
	}

	MutexLock l(data->mutex);
	if (p_reset) {
		if (data->use_reverse_cache) {
			data->reverse_cache.clear();
		}
		data->unique_ids.clear();
	}

	uint32_t entry_count = f->get_32();
	for (uint32_t i = 0; i < entry_count; i++) {
		int64_t id = f->get_64();
		int32_t len = f->get_32();
		Cache c;
		c.cs.resize_uninitialized(len + 1);
		ERR_FAIL_COND_V(c.cs.size() != len + 1, ERR_FILE_CORRUPT); // Out of memory.
		c.cs[len] = 0;
		int32_t rl = f->get_buffer((uint8_t*)c.cs.ptrw(), len);
		ERR_FAIL_COND_V(rl != len, ERR_FILE_CORRUPT);

		c.saved_to_cache = true;
		data->unique_ids[id] = c;
		if (data->use_reverse_cache) {
			data->reverse_cache[c.cs] = id;
		}
	}

	data->cache_entries = entry_count;
	data->changed = false;
	data->cache_initialized = true;
	return OK;
}

Error ResourceUID::update_cache()
{
	if (!data->changed) {
		return OK;
	}

	if (data->cache_entries == 0) {
		return save_to_cache();
	}
	MutexLock l(data->mutex);

	Ref<FileAccess> f;
	for (KeyValue<ID, Cache>& E : data->unique_ids) {
		if (!E.value.saved_to_cache) {
			if (f.is_null()) {
				f = FileAccess::open(get_cache_file(), FileAccess::READ_WRITE); // Append.
				if (f.is_null()) {
					return ERR_CANT_OPEN;
				}
				f->seek_end();
			}
			f->store_64(uint64_t(E.key));
			uint32_t s = E.value.cs.length();
			f->store_32(s);
			f->store_buffer((const uint8_t*)E.value.cs.ptr(), s);
			E.value.saved_to_cache = true;
			data->cache_entries++;
		}
	}

	if (f.is_valid()) {
		f->seek(0);
		f->store_32(data->cache_entries); // update amount of entries
	}

	data->changed = false;

	return OK;
}

String ResourceUID::get_path_from_cache(Ref<FileAccess>& p_cache_file, const String& p_uid_string)
{
	const int64_t uid_from_string = text_to_id(p_uid_string);
	if (uid_from_string != INVALID_ID) {
		const uint32_t entry_count = p_cache_file->get_32();
		for (uint32_t i = 0; i < entry_count; i++) {
			int64_t id = p_cache_file->get_64();
			int32_t len = p_cache_file->get_32();

			if (id == uid_from_string) {
				CharString cs;
				cs.resize_uninitialized(len + 1);
				ERR_FAIL_COND_V(cs.size() != len + 1, String());
				cs[len] = 0;
				int32_t rl = p_cache_file->get_buffer((uint8_t*)cs.ptrw(), len);
				ERR_FAIL_COND_V(rl != len, String());
				return String::utf8(cs.get_data());
			}
			else {
				p_cache_file->seek(p_cache_file->get_position() + len);
				ERR_FAIL_COND_V(p_cache_file->eof_reached(), String());
			}
		}
	}
	return String();
}

void ResourceUID::clear()
{
	data->cache_entries = 0;
	if (data->use_reverse_cache) {
		data->reverse_cache.clear();
	}
	data->unique_ids.clear();
	data->changed = false;
}

void ResourceUID::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void ResourceUID::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

// ==========================================================================
// missing_resource.cpp
// ==========================================================================

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

#include "missing_resource.h"

void MissingResource::set_original_class(const String& p_class) { original_class = p_class; }

String MissingResource::get_original_class() const { return original_class; }

void MissingResource::set_recording_properties(bool p_enable) { recording_properties = p_enable; }

bool MissingResource::is_recording_properties() const { return recording_properties; }

String MissingResource::get_save_class() const { return original_class; }

// ==========================================================================
// image_loader.cpp
// ==========================================================================

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

#include "image_loader.h"

bool ImageFormatLoader::recognize(const String& p_extension) const
{
	List<String> extensions;
	get_recognized_extensions(&extensions);
	for (const String& E : extensions) {
		if (E.nocasecmp_to(p_extension) == 0) {
			return true;
		}
	}

	return false;
}

Error ImageFormatLoaderExtension::load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess,
	uint32_t p_flags, float p_scale)
{
	Error err = ERR_UNAVAILABLE;
	return err;
}

void ImageFormatLoaderExtension::add_format_loader() { ImageLoader::add_image_format_loader(this); }

void ImageFormatLoaderExtension::remove_format_loader()
{
	ImageLoader::remove_image_format_loader(this);
}

Error ImageLoader::load_image(const String& p_file, Ref<Image> p_image, Ref<FileAccess> p_custom,
	uint32_t p_flags, float p_scale)
{
	ERR_FAIL_COND_V_MSG(
		p_image.is_null(), ERR_INVALID_PARAMETER, "Can't load an image: invalid Image object.");
	const String file = ResourceUID::ensure_path(p_file);

	Ref<FileAccess> f = p_custom;
	if (f.is_null()) {
		Error err;
		f = FileAccess::open(file, FileAccess::READ, &err);
		ERR_FAIL_COND_V_MSG(f.is_null(), err, vformat("Error opening file '%s'.", file));
	}

	String extension = file.get_extension();

	for (int i = 0; i < loader.size(); i++) {
		if (!loader[i]->recognize(extension)) {
			continue;
		}
		Error err = loader.write[i]->load_image(p_image, f, p_flags, p_scale);
		if (err != OK) {
			ERR_PRINT(vformat("Error loading image: '%s'.", file));
		}

		if (err != ERR_FILE_UNRECOGNIZED) {
			return err;
		}
	}

	return
ERR_FILE_UNRECOGNIZED;
}

void ImageLoader::get_recognized_extensions(List<String>* p_extensions)
{
	for (int i = 0; i < loader.size(); i++) {
		loader[i]->get_recognized_extensions(p_extensions);
	}
}

Ref<ImageFormatLoader> ImageLoader::recognize(const String& p_extension)
{
	for (int i = 0; i < loader.size(); i++) {
		if (loader[i]->recognize(p_extension)) {
			return loader[i];
		}
	}

	return nullptr;
}

void ImageLoader::add_image_format_loader(Ref<ImageFormatLoader> p_loader)
{
	loader.push_back(p_loader);
}

void ImageLoader::remove_image_format_loader(Ref<ImageFormatLoader> p_loader)
{
	loader.erase(p_loader);
}

void ImageLoader::cleanup()
{
	while (loader.size()) {
		remove_image_format_loader(loader[0]);
	}
}

void ImageFormatLoaderExtension::get_recognized_extensions(List<String>* p_extensions) const {}

// ==========================================================================
// image_resource_format.cpp
// ==========================================================================

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

#include "image_resource_format.h"

Ref<Resource> ResourceFormatLoaderImage::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		if (r_error) {
			*r_error = ERR_CANT_OPEN;
		}
		return Ref<Resource>();
	}

	uint8_t header[4] = { 0, 0, 0, 0 };
	f->get_buffer(header, 4);

	bool unrecognized = header[0] != 'G' || header[1] != 'D' || header[2] != 'I' || header[3] != 'M';
	if (unrecognized) {
		if (r_error) {
			*r_error = ERR_FILE_UNRECOGNIZED;
		}
		ERR_FAIL_V(Ref<Resource>());
	}

	String extension = f->get_pascal_string();

	int idx = -1;

	for (int i = 0; i < ImageLoader::loader.size(); i++) {
		if (ImageLoader::loader[i]->recognize(extension)) {
			idx = i;
			break;
		}
	}

	if (idx == -1) {
		if (r_error) {
			*r_error = ERR_FILE_UNRECOGNIZED;
		}
		ERR_FAIL_V(Ref<Resource>());
	}

	Ref<Image> image;
	image.instantiate();

	Error err = ImageLoader::loader.write[idx]->load_image(image, f);

	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}
		return Ref<Resource>();
	}

	if (r_error) {
		*r_error = OK;
	}

	return image;
}

void ResourceFormatLoaderImage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("image");
}

bool ResourceFormatLoaderImage::handles_type(const String &p_type) const {
	return p_type == "Image";
}

String ResourceFormatLoaderImage::get_resource_type(const String &p_path) const {
	return p_path.get_extension().to_lower() == "image" ? "Image" : String();
}
