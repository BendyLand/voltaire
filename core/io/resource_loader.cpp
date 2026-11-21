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
#include "core/os/condition_variable.h"
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

#include "core/io/resource_loader.h"

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

// ============================================================================
// Merged from core/io/resource.cpp
// ============================================================================

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
		OS::DateTime dt = OS::get_datetime();
		uint32_t hash = hash_murmur3_one_32(OS::get_ticks_usec());
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

Ref<Resource> Resource::duplicate(bool p_deep) const
{
	Ref<Resource> res = memnew(Resource);
	return res;
}

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

Ref<Resource> Resource::_duplicate(bool p_subresources) const
{
	Ref<Resource> r = memnew(Resource);
	return r;
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
		if (OS::is_stdout_verbose()) {
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

void Resource::reload_from_file() {}

void Resource::setup_local_to_scene() {}


// ============================================================================
// Merged from core/io/resource_format_binary.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/missing_resource.h"
#include "core/version.h"
#include "core/io/resource_loader.h"
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

String ResourceLoaderBinary::_get_string()
{
	uint32_t id = f->get_32();
	if (id & 0x80000000) {
		uint32_t len = id & 0x7FFFFFFF;
		if ((int)len > str_buf.size()) {
			str_buf.resize(len);
		}
		if (len == 0) {
			return String();
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
			path = ProjectSettings::localize_path(
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
			String name = _get_string();

			if (name == String()) {
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

void ResourceLoaderBinary::get_classes_used(Ref<FileAccess> p_file, HashSet<String>* p_classes)
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
			dep = ResourceUID::id_to_text(external_resources[i].uid);
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
		String s = get_unicode_string();
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
				if (ResourceUID::has_id(er.uid)) {
					// If a UID is found and the path is valid, it will be used, otherwise, it falls
					// back to the path.
					er.path = ResourceUID::get_id_path(er.uid);
				}
				else {
#ifdef TOOLS_ENABLED
					// Silence a warning that can happen during the initial filesystem scan due to
					// cache being regenerated.
					if (ResourceLoader::get_resource_uid(res_path) != er.uid) {
						WARN_PRINT(vformat("'%s': In external resource #%d, invalid UID: '%s' - "
										   "using text path instead: '%s'.",
							res_path, i, ResourceUID::id_to_text(er.uid),
							er.path));
					}
#else
					WARN_PRINT(vformat("'%s': In external resource #%d, invalid UID: '%s' - using "
									   "text path instead: '%s'.",
						res_path, i, ResourceUID::id_to_text(er.uid), er.path));
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
	loader.local_path = ProjectSettings::localize_path(path);
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
	loader.local_path = ProjectSettings::localize_path(p_path);
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
		loader.local_path = ProjectSettings::localize_path(p_path);
		loader.res_path = loader.local_path;
		loader.remaps = p_map;
		loader.open(f);

		err = loader.load();

		ERR_FAIL_COND_V(err != ERR_FILE_EOF, ERR_FILE_CORRUPT);
		Ref<Resource> res = loader.get_resource();
		ERR_FAIL_COND_V(res.is_null(), ERR_FILE_CORRUPT);

		ResourceFormatSaverBinaryInstance saver;
		return saver.save(ProjectSettings::localize_path(p_path), res);
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
				if (ResourceUID::has_id(uid)) {
					// If a UID is found and the path is valid, it will be used, otherwise, it falls
					// back to the path.
					path = ResourceUID::get_id_path(uid);
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
	const String& p_path, HashSet<String>* r_classes)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_MSG(f.is_null(), vformat("Cannot open file '%s'.", p_path));

	ResourceLoaderBinary loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
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
		const String node_name = state->get_node_type(i);
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
	loader.local_path = ProjectSettings::localize_path(p_path);
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
	loader.local_path = ProjectSettings::localize_path(p_path);
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
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.open(f, true);
	if (loader.error != OK) {
		return ResourceUID::INVALID_ID; // could not read
	}
	return loader.uid;
}

bool ResourceFormatLoaderBinary::has_custom_uid_support() const { return true; }

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
	String s = p_string;
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
	path = ProjectSettings::localize_path(p_path);

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
	String local_path = ProjectSettings::localize_path(p_path);
	ResourceFormatSaverBinaryInstance saver;
	return saver.save(local_path, p_resource, p_flags);
}

Error ResourceFormatSaverBinary::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String local_path = ProjectSettings::localize_path(p_path);
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




// ============================================================================
// Merged from core/io/resource_importer.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "resource_importer.h"

bool ResourceFormatImporter::SortImporterByName::operator()(
	const Ref<ResourceImporter>& p_a, const Ref<ResourceImporter>& p_b) const
{
	return p_a->get_importer_name() < p_b->get_importer_name();
}

Error ResourceFormatImporter::_get_path_and_type(
	const String& p_path, PathAndType& r_path_and_type, bool p_load, bool* r_valid)
{
	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path + ".import", FileAccess::READ, &err);

	if (f.is_null()) {
		if (r_valid) {
			*r_valid = false;
		}
		return err;
	}

	if (r_valid) {
		*r_valid = true;
	}

	int lines = 0;
	String error_text;
	bool path_found = false; // First match must have priority.

	String decomp_path;
	bool decomp_path_found = false;
	while (true) {
		if (err == ERR_FILE_EOF) {
			if (p_load && !path_found && decomp_path_found) {
				print_verbose(vformat("No natively supported texture format found for %s, using "
									  "decompressable format %s.",
					p_path, decomp_path));
				r_path_and_type.path = decomp_path;
			}

			return OK;
		}
		else if (err != OK) {
			ERR_PRINT(vformat("ResourceFormatImporter::load - %s.import:%d error: %s.", p_path,
				lines, error_text));
			return err;
		}
	}

	if (p_load && !path_found && decomp_path_found) {
		print_verbose(vformat(
			"No natively supported texture format found for %s, using decompressable format %s.",
			p_path, decomp_path));
		r_path_and_type.path = decomp_path;
		return OK;
	}

	if (r_path_and_type.type.is_empty()) {
		return ERR_FILE_CORRUPT;
	}
	if (r_path_and_type.path.is_empty()) {
		// Some importers may not write files to the .godot folder, so the path can be empty.
		if (r_path_and_type.importer.is_empty()) {
			return ERR_FILE_CORRUPT;
		}

		// It's only invalid if the extension for the importer is not empty.
		Ref<ResourceImporter> importer = get_importer_by_name(r_path_and_type.importer);
		if (importer.is_null() || !importer->get_save_extension().is_empty()) {
			return ERR_FILE_CORRUPT;
		}
	}
	return OK;
}

Ref<Resource> ResourceFormatImporter::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
#ifdef TOOLS_ENABLED
	// When loading a resource on startup, we use the load_on_startup callback,
	// which executes the loading in the EditorFileSystem. It can reimport
	// the resource and retry the load, allowing the resource to be loaded
	// even if it is not yet imported.
	if (ResourceImporter::load_on_startup != nullptr) {
		return ResourceImporter::load_on_startup(
			p_path, r_error, p_use_sub_threads, r_progress, p_cache_mode);
	}
#endif

	return load_internal(p_path, r_error, p_use_sub_threads, r_progress, p_cache_mode, false);
}

Ref<Resource> ResourceFormatImporter::load_internal(const String& p_path, Error* r_error,
	bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode, bool p_silence_errors)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, true);

	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}

		return Ref<Resource>();
	}

	if (p_silence_errors) {
		// Note: Some importers do not create files in the .godot folder, so we need to check if the
		// path is empty.
		if (!pat.path.is_empty() && !FileAccess::exists(pat.path)) {
			return Ref<Resource>();
		}
	}

	Ref<Resource> res = ResourceLoader::_load(
		pat.path, p_path, pat.type, p_cache_mode, r_error, p_use_sub_threads, r_progress);

	return res;
}

void ResourceFormatImporter::get_recognized_extensions(List<String>* p_extensions)
{
	HashSet<String> found;

	for (int i = 0; i < data->importers.size(); i++) {
		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (!found.has(F)) {
				p_extensions->push_back(F);
				found.insert(F);
			}
		}
	}
}

void ResourceFormatImporter::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions)
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}

	HashSet<String> found;

	for (int i = 0; i < data->importers.size(); i++) {
		String res_type = data->importers[i]->get_resource_type();
		if (res_type.is_empty()) {
			continue;
		}

		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (!found.has(F)) {
				p_extensions->push_back(F);
				found.insert(F);
			}
		}
	}
}

bool ResourceFormatImporter::exists(const String& p_path)
{
	return FileAccess::exists(p_path + ".import");
}

bool ResourceFormatImporter::recognize_path(const String& p_path, const String& p_for_type)
{
	return FileAccess::exists(p_path + ".import");
}

Error ResourceFormatImporter::get_import_order_threads_and_importer(
	const String& p_path, int& r_order, bool& r_can_threads, String& r_importer)
{
	r_order = 0;
	r_importer = "";

	r_can_threads = false;
	Ref<ResourceImporter> importer;

	if (FileAccess::exists(p_path + ".import")) {
		PathAndType pat;
		Error err = _get_path_and_type(p_path, pat, false);

		if (err == OK) {
			importer = get_importer_by_name(pat.importer);
		}
	}
	else {
		importer = get_importer_by_file(p_path);
	}

	if (importer.is_valid()) {
		r_order = importer->get_import_order();
		r_importer = importer->get_importer_name();
		r_can_threads = importer->can_import_threaded();
		return OK;
	}
	else {
		return ERR_INVALID_PARAMETER;
	}
}

int ResourceFormatImporter::get_import_order(const String& p_path)
{
	Ref<ResourceImporter> importer;

	if (FileAccess::exists(p_path + ".import")) {
		PathAndType pat;
		Error err = _get_path_and_type(p_path, pat, false);

		if (err == OK) {
			importer = get_importer_by_name(pat.importer);
		}
	}
	else {
		importer = get_importer_by_file(p_path);
	}

	if (importer.is_valid()) {
		return importer->get_import_order();
	}

	return 0;
}

bool ResourceFormatImporter::handles_type(const String& p_type)
{
	for (int i = 0; i < data->importers.size(); i++) {
		String res_type = data->importers[i]->get_resource_type();
		if (res_type.is_empty()) {
			continue;
		}
	}

	return true;
}

String ResourceFormatImporter::get_internal_resource_path(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return String();
	}

	return pat.path;
}

String ResourceFormatImporter::get_import_group_file(const String& p_path)
{
	bool valid = true;
	PathAndType pat;
	_get_path_and_type(p_path, pat, false, &valid);
	return valid ? pat.group_file : String();
}

bool ResourceFormatImporter::is_import_valid(const String& p_path)
{
	bool valid = true;
	PathAndType pat;
	_get_path_and_type(p_path, pat, false, &valid);
	return valid;
}

String ResourceFormatImporter::get_resource_type(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return "";
	}

	return pat.type;
}

String ResourceFormatImporter::get_resource_script_class(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return String();
	}

	return ResourceLoader::get_resource_script_class(pat.path);
}

ResourceUID::ID ResourceFormatImporter::get_resource_uid(const String& p_path)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return ResourceUID::INVALID_ID;
	}

	return pat.uid;
}

bool ResourceFormatImporter::has_custom_uid_support() { return true; }

Error ResourceFormatImporter::get_resource_import_info(const String& p_path, String& r_type,
	ResourceUID::ID& r_uid, String& r_import_group_file)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err == OK) {
		r_type = pat.type;
		r_uid = pat.uid;
		r_import_group_file = pat.group_file;
	}
	else {
		r_type = "";
		r_uid = ResourceUID::INVALID_ID;
		r_import_group_file = "";
	}

	return err;
}

void ResourceFormatImporter::get_classes_used(const String& p_path, HashSet<String>* r_classes)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return;
	}

	ResourceLoader::get_classes_used(pat.path, r_classes);
}

void ResourceFormatImporter::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	PathAndType pat;
	Error err = _get_path_and_type(p_path, pat, false);

	if (err != OK) {
		return;
	}

	ResourceLoader::get_dependencies(pat.path, p_dependencies, p_add_types);
}

void ResourceFormatImporter::get_build_dependencies(
	const String& p_path, HashSet<String>* r_dependencies)
{
	if (!exists(p_path)) {
		return;
	}

	List<Ref<ResourceImporter>> valid_importers;
	get_importers_for_file(p_path, &valid_importers);
	for (Ref<ResourceImporter> importer : valid_importers) {
		importer->get_build_dependencies(p_path, r_dependencies);
	}
}

Ref<ResourceImporter> ResourceFormatImporter::get_importer_by_name(const String& p_name)
{
	for (int i = 0; i < data->importers.size(); i++) {
		if (data->importers[i]->get_importer_name() == p_name) {
			return data->importers[i];
		}
	}

	return Ref<ResourceImporter>();
}

void ResourceFormatImporter::add_importer(
	const Ref<ResourceImporter>& p_importer, bool p_first_priority)
{
	ERR_FAIL_COND(p_importer.is_null());
	if (p_first_priority) {
		data->importers.insert(0, p_importer);
	}
	else {
		data->importers.push_back(p_importer);
	}
}

void ResourceFormatImporter::get_importers_for_file(
	const String& p_file, List<Ref<ResourceImporter>>* r_importers)
{
	for (int i = 0; i < data->importers.size(); i++) {
		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (p_file.right(F.length()).nocasecmp_to(F) == 0) {
				r_importers->push_back(data->importers[i]);
				break;
			}
		}
	}
}

void ResourceFormatImporter::get_importers(List<Ref<ResourceImporter>>* r_importers)
{
	for (int i = 0; i < data->importers.size(); i++) {
		r_importers->push_back(data->importers[i]);
	}
}

Ref<ResourceImporter> ResourceFormatImporter::get_importer_by_file(const String& p_file)
{
	Ref<ResourceImporter> importer;
	float priority = 0;

	for (int i = 0; i < data->importers.size(); i++) {
		List<String> local_exts;
		data->importers[i]->get_recognized_extensions(&local_exts);
		for (const String& F : local_exts) {
			if (p_file.right(F.length()).nocasecmp_to(F) == 0 &&
				data->importers[i]->get_priority() > priority) {
				importer = data->importers[i];
				priority = data->importers[i]->get_priority();
				break;
			}
		}
	}

	return importer;
}

String ResourceFormatImporter::get_import_base_path(const String& p_for_file)
{
	return ProjectSettings::get_imported_files_path().path_join(
		p_for_file.get_file() + "-" + p_for_file.md5_text());
}

bool ResourceFormatImporter::are_import_settings_valid(const String& p_path)
{
	bool valid = true;
	PathAndType pat;
	_get_path_and_type(p_path, pat, false, &valid);

	if (!valid) {
		return false;
	}

	return true;
}

String ResourceFormatImporter::get_import_settings_hash()
{
	Vector<Ref<ResourceImporter>> sorted_importers = data->importers;

	sorted_importers.sort_custom<SortImporterByName>();

	String hash;
	for (int i = 0; i < sorted_importers.size(); i++) {
		hash += ":" + sorted_importers[i]->get_importer_name() + ":" +
				sorted_importers[i]->get_import_settings_string();
	}
	return hash.md5_text();
}

void ResourceFormatImporter::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void ResourceFormatImporter::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void ResourceImporter::get_build_dependencies(const String& p_path, HashSet<String>* r_dependencies)
{
	Vector<String> ret;
	for (int i = 0; i < ret.size(); i++) {
		r_dependencies->insert(ret[i]);
	}
}

Error ResourceFormatImporterSaver::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	Ref<ConfigFile> cf;
	cf.instantiate();
	Error err = cf->load(p_path + ".import");
	if (err != OK) {
		return err;
	}
	cf->save(p_path + ".import");

	return OK;
}



void ResourceFormatImporter::get_internal_resource_path_list(String const&, List<String, DefaultAllocator>*) {}
