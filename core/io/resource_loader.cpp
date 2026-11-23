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

#include "core/io/resource_loader.h"

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
#include "core/io/resource_loader.h"
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


// --- Merged from core/io/image.cpp ---
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/image_loader.h"
#include "core/io/resource_loader.h"
#include "core/math/math_funcs.h"
#include "core/templates/hash_map.h"
#include "core/io/image.h"

#ifndef DISABLE_DEPRECATED

static Image::CompressProfile _astc_format_to_compress_profile(Image::ASTCFormat p_format)
{
	switch (p_format) {
	case Image::ASTC_FORMAT_4x4:
		return Image::COMPRESS_PROFILE_MAX_QUALITY;
	case Image::ASTC_FORMAT_8x8:
		return Image::COMPRESS_PROFILE_MAX_COMPRESSION;
	default:
		return Image::COMPRESS_PROFILE_AUTOMATIC;
	}
}

Error Image::_compress_bind_compat_115003(
	CompressMode p_mode, CompressSource p_source, ASTCFormat p_format)
{
	return compress(p_mode, p_source, _astc_format_to_compress_profile(p_format));
}

Error Image::_compress_from_channels_bind_compat_115003(
	CompressMode p_mode, UsedChannels p_channels, ASTCFormat p_format)
{
	return compress_from_channels(p_mode, p_channels, _astc_format_to_compress_profile(p_format));
}

Vector<uint8_t> Image::_save_exr_to_buffer_bind_compat_117800(bool p_grayscale) const
{
	return save_exr_to_buffer(p_grayscale);
}

Error Image::_save_exr_bind_compat_117800(const String& p_path, bool p_grayscale) const
{
	return save_exr(p_path, p_grayscale);
}

void Image::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

const char* Image::format_names[Image::FORMAT_MAX] = {
	"Lum8",
	"LumAlpha8",
	"Red8",
	"RedGreen",
	"RGB8",
	"RGBA8",
	"RGBA4444",
	"RGBA5551", // Actually RGB565, kept as RGBA5551 for compatibility.
	"RFloat",
	"RGFloat",
	"RGBFloat",
	"RGBAFloat",
	"RHalf",
	"RGHalf",
	"RGBHalf",
	"RGBAHalf",
	"RGBE9995",
	"DXT1 RGB8",
	"DXT3 RGBA8",
	"DXT5 RGBA8",
	"RGTC Red8",
	"RGTC RedGreen8",
	"BPTC_RGBA",
	"BPTC_RGBF",
	"BPTC_RGBFU",
	"ETC",
	"ETC2_R11",
	"ETC2_R11S",
	"ETC2_RG11",
	"ETC2_RG11S",
	"ETC2_RGB8",
	"ETC2_RGBA8",
	"ETC2_RGB8A1",
	"ETC2_RA_AS_RG",
	"FORMAT_DXT5_RA_AS_RG",
	"ASTC_4x4",
	"ASTC_4x4_HDR",
	"ASTC_8x8",
	"ASTC_8x8_HDR",
	"R16",
	"RG16",
	"RGB16",
	"RGBA16",
	"R16Int",
	"RG16Int",
	"RGB16Int",
	"RGBA16Int",
	"ASTC_6x6",
	"ASTC_6x6_HDR",
};

// External VRAM compression function pointers.

void (*Image::_image_compress_bc_func)(Image*, Image::UsedChannels) = nullptr;
void (*Image::_image_compress_bptc_func)(Image*, Image::UsedChannels, Image::BPTCFormat) = nullptr;
void (*Image::_image_compress_etc1_func)(Image*) = nullptr;
void (*Image::_image_compress_etc2_func)(Image*, Image::UsedChannels) = nullptr;
void (*Image::_image_compress_astc_func)(
	Image*, Image::UsedChannels, Image::CompressProfile) = nullptr;

Error (*Image::_image_compress_bptc_rd_func)(
	Image*, Image::UsedChannels, Image::BPTCFormat) = nullptr;
Error (*Image::_image_compress_bc_rd_func)(Image*, Image::UsedChannels) = nullptr;

// External VRAM decompression function pointers.

void (*Image::_image_decompress_bc)(Image*) = nullptr;
void (*Image::_image_decompress_bptc)(Image*) = nullptr;
void (*Image::_image_decompress_etc1)(Image*) = nullptr;
void (*Image::_image_decompress_etc2)(Image*) = nullptr;
void (*Image::_image_decompress_astc)(Image*) = nullptr;

// External packer function pointers.

Vector<uint8_t> (*Image::webp_lossy_packer)(const Ref<Image>&, float) = nullptr;
Vector<uint8_t> (*Image::webp_lossless_packer)(const Ref<Image>&) = nullptr;
Vector<uint8_t> (*Image::png_packer)(const Ref<Image>&) = nullptr;
Vector<uint8_t> (*Image::basis_universal_packer)(
	const Ref<Image>&, Image::UsedChannels, const BasisUniversalPackerParams&) = nullptr;

Ref<Image> (*Image::webp_unpacker)(const Vector<uint8_t>&) = nullptr;
Ref<Image> (*Image::png_unpacker)(const Vector<uint8_t>&) = nullptr;
Ref<Image> (*Image::basis_universal_unpacker)(const Vector<uint8_t>&) = nullptr;
Ref<Image> (*Image::basis_universal_unpacker_ptr)(const uint8_t*, int) = nullptr;

void Image::_put_pixelb(
	int p_x, int p_y, uint32_t p_pixel_size, uint8_t* p_data, const uint8_t* p_pixel)
{
	uint32_t ofs = (p_y * width + p_x) * p_pixel_size;
	memcpy(p_data + ofs, p_pixel, p_pixel_size);
}

void Image::_get_pixelb(
	int p_x, int p_y, uint32_t p_pixel_size, const uint8_t* p_data, uint8_t* p_pixel)
{
	uint32_t ofs = (p_y * width + p_x) * p_pixel_size;
	memcpy(p_pixel, p_data + ofs, p_pixel_size);
}

int Image::get_format_pixel_size(Format p_format)
{
	switch (p_format) {
	case FORMAT_L8:
		return 1;
	case FORMAT_LA8:
		return 2;
	case FORMAT_R8:
		return 1;
	case FORMAT_RG8:
		return 2;
	case FORMAT_RGB8:
		return 3;
	case FORMAT_RGBA8:
		return 4;
	case FORMAT_RGBA4444:
		return 2;
	case FORMAT_RGB565:
		return 2;
	case FORMAT_RF:
		return 4;
	case FORMAT_RGF:
		return 8;
	case FORMAT_RGBF:
		return 12;
	case FORMAT_RGBAF:
		return 16;
	case FORMAT_RH:
		return 2;
	case FORMAT_RGH:
		return 4;
	case FORMAT_RGBH:
		return 6;
	case FORMAT_RGBAH:
		return 8;
	case FORMAT_RGBE9995:
		return 4;
	case FORMAT_DXT1:
		return 1;
	case FORMAT_DXT3:
		return 1;
	case FORMAT_DXT5:
		return 1;
	case FORMAT_RGTC_R:
		return 1;
	case FORMAT_RGTC_RG:
		return 1;
	case FORMAT_BPTC_RGBA:
		return 1;
	case FORMAT_BPTC_RGBF:
		return 1;
	case FORMAT_BPTC_RGBFU:
		return 1;
	case FORMAT_ETC:
		return 1;
	case FORMAT_ETC2_R11:
		return 1;
	case FORMAT_ETC2_R11S:
		return 1;
	case FORMAT_ETC2_RG11:
		return 1;
	case FORMAT_ETC2_RG11S:
		return 1;
	case FORMAT_ETC2_RGB8:
		return 1;
	case FORMAT_ETC2_RGBA8:
		return 1;
	case FORMAT_ETC2_RGB8A1:
		return 1;
	case FORMAT_ETC2_RA_AS_RG:
		return 1;
	case FORMAT_DXT5_RA_AS_RG:
		return 1;
	case FORMAT_ASTC_4x4:
		return 1;
	case FORMAT_ASTC_4x4_HDR:
		return 1;
	case FORMAT_ASTC_6x6:
		return 1;
	case FORMAT_ASTC_6x6_HDR:
		return 1;
	case FORMAT_ASTC_8x8:
		return 1;
	case FORMAT_ASTC_8x8_HDR:
		return 1;
	case FORMAT_R16:
		return 2;
	case FORMAT_RG16:
		return 4;
	case FORMAT_RGB16:
		return 6;
	case FORMAT_RGBA16:
		return 8;
	case FORMAT_R16I:
		return 2;
	case FORMAT_RG16I:
		return 4;
	case FORMAT_RGB16I:
		return 6;
	case FORMAT_RGBA16I:
		return 8;
	case FORMAT_MAX: {
	}
	}
	return 0;
}

uint64_t Image::get_format_pixels_shifted(Format p_format, uint64_t p_pixels)
{
	switch (p_format) {
	case FORMAT_ASTC_8x8:
	case FORMAT_ASTC_8x8_HDR:
		return p_pixels >> 2;
	case FORMAT_ASTC_6x6:
	case FORMAT_ASTC_6x6_HDR:
		return (p_pixels * 4) / 9;
	case FORMAT_DXT1:
	case FORMAT_RGTC_R:
	case FORMAT_ETC:
	case FORMAT_ETC2_R11:
	case FORMAT_ETC2_R11S:
	case FORMAT_ETC2_RGB8:
	case FORMAT_ETC2_RGB8A1:
		return p_pixels >> 1;
	default:
		return p_pixels;
	}
}

void Image::get_format_min_pixel_size(Format p_format, int& r_w, int& r_h)
{
	switch (p_format) {
	case FORMAT_DXT1:
	case FORMAT_DXT3:
	case FORMAT_DXT5:
	case FORMAT_RGTC_R:
	case FORMAT_RGTC_RG: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ETC: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_BPTC_RGBA:
	case FORMAT_BPTC_RGBF:
	case FORMAT_BPTC_RGBFU: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ETC2_R11:
	case FORMAT_ETC2_R11S:
	case FORMAT_ETC2_RG11:
	case FORMAT_ETC2_RG11S:
	case FORMAT_ETC2_RGB8:
	case FORMAT_ETC2_RGBA8:
	case FORMAT_ETC2_RGB8A1:
	case FORMAT_ETC2_RA_AS_RG:
	case FORMAT_DXT5_RA_AS_RG: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ASTC_4x4:
	case FORMAT_ASTC_4x4_HDR: {
		r_w = 4;
		r_h = 4;
	} break;
	case FORMAT_ASTC_6x6:
	case FORMAT_ASTC_6x6_HDR: {
		r_w = 6;
		r_h = 6;
	} break;
	case FORMAT_ASTC_8x8:
	case FORMAT_ASTC_8x8_HDR: {
		r_w = 8;
		r_h = 8;
	} break;
	default: {
		r_w = 1;
		r_h = 1;
	} break;
	}
}

int Image::get_format_block_size(Format p_format)
{
	switch (p_format) {
	case FORMAT_DXT1:
	case FORMAT_DXT3:
	case FORMAT_DXT5:
	case FORMAT_RGTC_R:
	case FORMAT_RGTC_RG: {
		return 4;
	}
	case FORMAT_ETC: {
		return 4;
	}
	case FORMAT_BPTC_RGBA:
	case FORMAT_BPTC_RGBF:
	case FORMAT_BPTC_RGBFU: {
		return 4;
	}
	case FORMAT_ETC2_R11:
	case FORMAT_ETC2_R11S:
	case FORMAT_ETC2_RG11:
	case FORMAT_ETC2_RG11S:
	case FORMAT_ETC2_RGB8:
	case FORMAT_ETC2_RGBA8:
	case FORMAT_ETC2_RGB8A1:
	case FORMAT_ETC2_RA_AS_RG:
	case FORMAT_DXT5_RA_AS_RG: {
		return 4;
	}
	case FORMAT_ASTC_4x4:
	case FORMAT_ASTC_4x4_HDR: {
		return 4;
	}
	case FORMAT_ASTC_6x6:
	case FORMAT_ASTC_6x6_HDR: {
		return 6;
	}
	case FORMAT_ASTC_8x8:
	case FORMAT_ASTC_8x8_HDR: {
		return 8;
	}
	default: {
	}
	}

	return 1;
}

void Image::_get_mipmap_offset_and_size(
	int p_mipmap, int64_t& r_offset, int& r_width, int& r_height) const
{
	int w = width;
	int h = height;
	int64_t ofs = 0;

	int pixel_size = get_format_pixel_size(format);
	int block = get_format_block_size(format);
	int minw, minh;
	get_format_min_pixel_size(format, minw, minh);

	for (int i = 0; i < p_mipmap; i++) {
		int bw = w % block != 0 ? w + (block - w % block) : w;
		int bh = h % block != 0 ? h + (block - h % block) : h;

		int64_t s = bw * bh;

		s *= pixel_size;
		s = get_format_pixels_shifted(format, s);
		ofs += s;
		w = MAX(minw, w >> 1);
		h = MAX(minh, h >> 1);
	}

	r_offset = ofs;
	r_width = w;
	r_height = h;
}

int64_t Image::get_mipmap_offset(int p_mipmap) const
{
	ERR_FAIL_INDEX_V(p_mipmap, get_mipmap_count() + 1, -1);

	int64_t ofs;
	int w, h;
	_get_mipmap_offset_and_size(p_mipmap, ofs, w, h);
	return ofs;
}

void Image::get_mipmap_offset_and_size(int p_mipmap, int64_t& r_ofs, int64_t& r_size) const
{
	int64_t ofs;
	int w, h;
	_get_mipmap_offset_and_size(p_mipmap, ofs, w, h);
	int64_t ofs2;
	_get_mipmap_offset_and_size(p_mipmap + 1, ofs2, w, h);
	r_ofs = ofs;
	r_size = ofs2 - ofs;
}

void Image::get_mipmap_offset_size_and_dimensions(
	int p_mipmap, int64_t& r_ofs, int64_t& r_size, int& p_w, int& p_h) const
{
	int64_t ofs;
	_get_mipmap_offset_and_size(p_mipmap, ofs, p_w, p_h);
	int64_t ofs2;
	int w2, h2;
	_get_mipmap_offset_and_size(p_mipmap + 1, ofs2, w2, h2);
	r_ofs = ofs;
	r_size = ofs2 - ofs;
}

Image::Image3DValidateError Image::validate_3d_image(Image::Format p_format, int p_width,
	int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_images)
{
	int w = p_width;
	int h = p_height;
	int d = p_depth;

	int arr_ofs = 0;

	while (true) {
		for (int i = 0; i < d; i++) {
			int idx = i + arr_ofs;
			if (idx >= p_images.size()) {
				return VALIDATE_3D_ERR_MISSING_IMAGES;
			}
			if (p_images[idx].is_null() || p_images[idx]->is_empty()) {
				return VALIDATE_3D_ERR_IMAGE_EMPTY;
			}
			if (p_images[idx]->get_format() != p_format) {
				return VALIDATE_3D_ERR_IMAGE_FORMAT_MISMATCH;
			}
			if (p_images[idx]->get_width() != w || p_images[idx]->get_height() != h) {
				return VALIDATE_3D_ERR_IMAGE_SIZE_MISMATCH;
			}
			if (p_images[idx]->has_mipmaps()) {
				return VALIDATE_3D_ERR_IMAGE_HAS_MIPMAPS;
			}
		}

		arr_ofs += d;

		if (!p_mipmaps) {
			break;
		}

		if (w == 1 && h == 1 && d == 1) {
			break;
		}

		w = MAX(1, w >> 1);
		h = MAX(1, h >> 1);
		d = MAX(1, d >> 1);
	}

	if (arr_ofs != p_images.size()) {
		return VALIDATE_3D_ERR_EXTRA_IMAGES;
	}

	return VALIDATE_3D_OK;
}

String Image::get_3d_image_validation_error_text(Image3DValidateError p_error)
{
	switch (p_error) {
	case VALIDATE_3D_OK: {
		return "Ok";
	} break;
	case VALIDATE_3D_ERR_IMAGE_EMPTY: {
		return "Empty Image found";
	} break;
	case VALIDATE_3D_ERR_MISSING_IMAGES: {
		return "Missing Images";
	} break;
	case VALIDATE_3D_ERR_EXTRA_IMAGES: {
		return "Too many Images";
	} break;
	case VALIDATE_3D_ERR_IMAGE_SIZE_MISMATCH: {
		return "Image size mismatch";
	} break;
	case VALIDATE_3D_ERR_IMAGE_FORMAT_MISMATCH: {
		return "Image format mismatch";
	} break;
	case VALIDATE_3D_ERR_IMAGE_HAS_MIPMAPS: {
		return "Image has included mipmaps";
	} break;
	}
	return String();
}

int Image::get_width() const { return width; }

int Image::get_height() const { return height; }

Size2i Image::get_size() const { return Size2i(width, height); }

bool Image::has_mipmaps() const { return mipmaps; }

int Image::get_mipmap_count() const
{
	if (mipmaps) {
		return get_image_required_mipmaps(width, height, format);
	}
	else {
		return 0;
	}
}

// Using template generates perfectly optimized code due to constant expression reduction and unused
// variable removal present in all compilers.
template <uint32_t read_bytes, bool read_alpha, uint32_t write_bytes, bool write_alpha,
	bool read_gray, bool write_gray>
static void _convert(int p_width, int p_height, const uint8_t* p_src, uint8_t* p_dst)
{
	constexpr uint32_t max_bytes = MAX(read_bytes, write_bytes);

	for (int y = 0; y < p_height; y++) {
		for (int x = 0; x < p_width; x++) {
			const uint8_t* rofs = &p_src[((y * p_width) + x) * (read_bytes + (read_alpha ? 1 : 0))];
			uint8_t* wofs = &p_dst[((y * p_width) + x) * (write_bytes + (write_alpha ? 1 : 0))];

			uint8_t rgba[4] = {0, 0, 0, 255};

			if constexpr (read_gray) {
				rgba[0] = rofs[0];
				rgba[1] = rofs[0];
				rgba[2] = rofs[0];
			}
			else {
				for (uint32_t i = 0; i < max_bytes; i++) {
					rgba[i] = (i < read_bytes) ? rofs[i] : 0;
				}
			}

			if constexpr (read_alpha || write_alpha) {
				rgba[3] = read_alpha ? rofs[read_bytes] : 255;
			}

			if constexpr (write_gray) {
				// REC.709
				const uint8_t luminance =
					(13938U * rgba[0] + 46869U * rgba[1] + 4729U * rgba[2] + 32768U) >> 16U;
				wofs[0] = luminance;
			}
			else {
				for (uint32_t i = 0; i < write_bytes; i++) {
					wofs[i] = rgba[i];
				}
			}

			if constexpr (write_alpha) {
				wofs[write_bytes] = rgba[3];
			}
		}
	}
}

template <typename T, uint32_t read_channels, uint32_t write_channels, T def_zero, T def_one>
static void _convert_fast(int p_width, int p_height, const T* p_src, T* p_dst)
{
	uint32_t dst_count = 0;
	uint32_t src_count = 0;

	const int resolution = p_width * p_height;

	for (int i = 0; i < resolution; i++) {
		memcpy(
			p_dst + dst_count, p_src + src_count, MIN(read_channels, write_channels) * sizeof(T));

		if constexpr (write_channels > read_channels) {
			const T def_value[4] = {def_zero, def_zero, def_zero, def_one};
			memcpy(p_dst + dst_count + read_channels, &def_value[read_channels],
				(write_channels - read_channels) * sizeof(T));
		}

		dst_count += write_channels;
		src_count += read_channels;
	}
}

static bool _are_formats_compatible(Image::Format p_format0, Image::Format p_format1)
{
	if (p_format0 <= Image::FORMAT_RGBA8 && p_format1 <= Image::FORMAT_RGBA8) {
		return true;
	}
	else if (p_format0 <= Image::FORMAT_RGBAH && p_format0 >= Image::FORMAT_RH &&
			   p_format1 <= Image::FORMAT_RGBAH && p_format1 >= Image::FORMAT_RH) {
		return true;
	}
	else if (p_format0 <= Image::FORMAT_RGBAF && p_format0 >= Image::FORMAT_RF &&
			   p_format1 <= Image::FORMAT_RGBAF && p_format1 >= Image::FORMAT_RF) {
		return true;
	}

	return false;
}

void Image::convert(Format p_new_format)
{
	ERR_FAIL_INDEX_MSG(p_new_format, FORMAT_MAX,
		vformat("The Image format specified (%d) is out of range. See Image's Format enum.",
			p_new_format));
	ERR_FAIL_COND_MSG(
		Image::is_format_compressed(format) || Image::is_format_compressed(p_new_format),
		"Cannot convert to (or from) compressed formats. Use compress() and decompress() instead.");

	if (data.is_empty() || p_new_format == format) {
		return;
	}

	// Includes the main image.
	const int mipmap_count = get_mipmap_count() + 1;

	if (!_are_formats_compatible(format, p_new_format)) {
		// Use put/set pixel which is slower but works with non-byte formats.
		Image new_img(width, height, mipmaps, p_new_format);

		for (int mip = 0; mip < mipmap_count; mip++) {
			int64_t src_mip_ofs, dst_mip_ofs;
			int w, h;
			_get_mipmap_offset_and_size(mip, src_mip_ofs, w, h);
			new_img._get_mipmap_offset_and_size(mip, dst_mip_ofs, w, h);

			uint8_t* dst_mip_ptr = new_img.ptrw() + dst_mip_ofs;
			const uint8_t* src_mip_ptr = ptr() + src_mip_ofs;

			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					uint32_t mip_ofs = y * w + x;
					new_img._set_color_at_ofs(
						dst_mip_ptr, mip_ofs, _get_color_at_ofs(src_mip_ptr, mip_ofs));
				}
			}
		}

		_copy_internals_from(new_img);
		return;
	}

	// Convert the formats in an optimized way by removing/adding color channels if necessary.
	Image new_img(width, height, mipmaps, p_new_format);

	const int conversion_type = format | p_new_format << 8;

	for (int mip = 0; mip < mipmap_count; mip++) {
		int64_t mip_offset = 0;
		int64_t mip_size = 0;
		int mip_width = 0;
		int mip_height = 0;
		get_mipmap_offset_size_and_dimensions(mip, mip_offset, mip_size, mip_width, mip_height);

		const uint8_t* rptr = data.ptr() + mip_offset;
		uint8_t* wptr = new_img.data.ptrw() + new_img.get_mipmap_offset(mip);

		switch (conversion_type) {
		case FORMAT_L8 | (FORMAT_LA8 << 8):
			_convert<1, false, 1, true, true, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_R8 << 8):
			_convert<1, false, 1, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_RG8 << 8):
			_convert<1, false, 2, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_RGB8 << 8):
			_convert<1, false, 3, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_L8 | (FORMAT_RGBA8 << 8):
			_convert<1, false, 3, true, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_L8 << 8):
			_convert<1, true, 1, false, true, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_R8 << 8):
			_convert<1, true, 1, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_RG8 << 8):
			_convert<1, true, 2, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_RGB8 << 8):
			_convert<1, true, 3, false, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_LA8 | (FORMAT_RGBA8 << 8):
			_convert<1, true, 3, true, true, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_L8 << 8):
			_convert<1, false, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_LA8 << 8):
			_convert<1, false, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_RG8 << 8):
			_convert<1, false, 2, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_RGB8 << 8):
			_convert<1, false, 3, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_R8 | (FORMAT_RGBA8 << 8):
			_convert<1, false, 3, true, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_L8 << 8):
			_convert<2, false, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_LA8 << 8):
			_convert<2, false, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_R8 << 8):
			_convert<2, false, 1, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_RGB8 << 8):
			_convert<2, false, 3, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RG8 | (FORMAT_RGBA8 << 8):
			_convert<2, false, 3, true, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_L8 << 8):
			_convert<3, false, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_LA8 << 8):
			_convert<3, false, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_R8 << 8):
			_convert<3, false, 1, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_RG8 << 8):
			_convert<3, false, 2, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGB8 | (FORMAT_RGBA8 << 8):
			_convert<3, false, 3, true, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_L8 << 8):
			_convert<3, true, 1, false, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_LA8 << 8):
			_convert<3, true, 1, true, false, true>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_R8 << 8):
			_convert<3, true, 1, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_RG8 << 8):
			_convert<3, true, 2, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RGBA8 | (FORMAT_RGB8 << 8):
			_convert<3, true, 3, false, false, false>(mip_width, mip_height, rptr, wptr);
			break;
		case FORMAT_RH | (FORMAT_RGH << 8):
			_convert_fast<uint16_t, 1, 2, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RH | (FORMAT_RGBH << 8):
			_convert_fast<uint16_t, 1, 3, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RH | (FORMAT_RGBAH << 8):
			_convert_fast<uint16_t, 1, 4, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGH | (FORMAT_RH << 8):
			_convert_fast<uint16_t, 2, 1, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGH | (FORMAT_RGBH << 8):
			_convert_fast<uint16_t, 2, 3, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGH | (FORMAT_RGBAH << 8):
			_convert_fast<uint16_t, 2, 4, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBH | (FORMAT_RH << 8):
			_convert_fast<uint16_t, 3, 1, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBH | (FORMAT_RGH << 8):
			_convert_fast<uint16_t, 3, 2, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBH | (FORMAT_RGBAH << 8):
			_convert_fast<uint16_t, 3, 4, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBAH | (FORMAT_RH << 8):
			_convert_fast<uint16_t, 4, 1, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBAH | (FORMAT_RGH << 8):
			_convert_fast<uint16_t, 4, 2, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBAH | (FORMAT_RGBH << 8):
			_convert_fast<uint16_t, 4, 3, 0x0000, 0x3C00>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RF | (FORMAT_RGF << 8):
			_convert_fast<uint32_t, 1, 2, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RF | (FORMAT_RGBF << 8):
			_convert_fast<uint32_t, 1, 3, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RF | (FORMAT_RGBAF << 8):
			_convert_fast<uint32_t, 1, 4, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGF | (FORMAT_RF << 8):
			_convert_fast<uint32_t, 2, 1, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGF | (FORMAT_RGBF << 8):
			_convert_fast<uint32_t, 2, 3, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGF | (FORMAT_RGBAF << 8):
			_convert_fast<uint32_t, 2, 4, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBF | (FORMAT_RF << 8):
			_convert_fast<uint32_t, 3, 1, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBF | (FORMAT_RGF << 8):
			_convert_fast<uint32_t, 3, 2, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBF | (FORMAT_RGBAF << 8):
			_convert_fast<uint32_t, 3, 4, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBAF | (FORMAT_RF << 8):
			_convert_fast<uint32_t, 4, 1, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBAF | (FORMAT_RGF << 8):
			_convert_fast<uint32_t, 4, 2, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_RGBAF | (FORMAT_RGBF << 8):
			_convert_fast<uint32_t, 4, 3, 0x00000000, 0x3F800000>(
				mip_width, mip_height, (const uint32_t*)rptr, (uint32_t*)wptr);
			break;
		case FORMAT_R16 | (FORMAT_RG16 << 8):
			_convert_fast<uint16_t, 1, 2, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16 | (FORMAT_RGB16 << 8):
			_convert_fast<uint16_t, 1, 3, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16 | (FORMAT_RGBA16 << 8):
			_convert_fast<uint16_t, 1, 4, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16 | (FORMAT_R16 << 8):
			_convert_fast<uint16_t, 2, 1, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16 | (FORMAT_RGB16 << 8):
			_convert_fast<uint16_t, 2, 3, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16 | (FORMAT_RGBA16 << 8):
			_convert_fast<uint16_t, 2, 4, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16 | (FORMAT_R16 << 8):
			_convert_fast<uint16_t, 3, 1, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16 | (FORMAT_RG16 << 8):
			_convert_fast<uint16_t, 3, 2, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16 | (FORMAT_RGBA16 << 8):
			_convert_fast<uint16_t, 3, 4, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16 | (FORMAT_R16 << 8):
			_convert_fast<uint16_t, 4, 1, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16 | (FORMAT_RG16 << 8):
			_convert_fast<uint16_t, 4, 2, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16 | (FORMAT_RGB16 << 8):
			_convert_fast<uint16_t, 4, 3, 0x0000, 0xFFFF>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16I | (FORMAT_RG16I << 8):
			_convert_fast<uint16_t, 1, 2, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16I | (FORMAT_RGB16I << 8):
			_convert_fast<uint16_t, 1, 3, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_R16I | (FORMAT_RGBA16I << 8):
			_convert_fast<uint16_t, 1, 4, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16I | (FORMAT_R16I << 8):
			_convert_fast<uint16_t, 2, 1, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16I | (FORMAT_RGB16I << 8):
			_convert_fast<uint16_t, 2, 3, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RG16I | (FORMAT_RGBA16I << 8):
			_convert_fast<uint16_t, 2, 4, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16I | (FORMAT_R16I << 8):
			_convert_fast<uint16_t, 3, 1, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16I | (FORMAT_RG16I << 8):
			_convert_fast<uint16_t, 3, 2, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGB16I | (FORMAT_RGBA16I << 8):
			_convert_fast<uint16_t, 3, 4, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16I | (FORMAT_R16I << 8):
			_convert_fast<uint16_t, 4, 1, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16I | (FORMAT_RG16I << 8):
			_convert_fast<uint16_t, 4, 2, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		case FORMAT_RGBA16I | (FORMAT_RGB16I << 8):
			_convert_fast<uint16_t, 4, 3, 0x0000, 0x0001>(
				mip_width, mip_height, (const uint16_t*)rptr, (uint16_t*)wptr);
			break;
		}
	}

	_copy_internals_from(new_img);
}

Image::Format Image::get_format() const { return format; }

enum ImageScaleType
{
	IMAGE_SCALING_INT,
	IMAGE_SCALING_FLOAT,
};

static double _bicubic_interp_kernel(double p_x)
{
	p_x = Math::abs(p_x);

	double bc = 0;

	if (p_x <= 1) {
		bc = (1.5 * p_x - 2.5) * p_x * p_x + 1;
	}
	else if (p_x < 2) {
		bc = ((-0.5 * p_x + 2.5) * p_x - 4) * p_x + 2;
	}

	return bc;
}

template <int CC, typename T, ImageScaleType TYPE>
static void _scale_cubic(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	// get source image size
	int width = p_src_width;
	int height = p_src_height;
	double xfac = (double)width / p_dst_width;
	double yfac = (double)height / p_dst_height;
	// coordinates of source points and coefficients
	double ox, oy, dx, dy;
	int ox1, oy1, ox2, oy2;
	// destination pixel values
	// width and height decreased by 1
	int ymax = height - 1;
	int xmax = width - 1;
	// temporary pointer

	for (uint32_t y = 0; y < p_dst_height; y++) {
		// Y coordinates
		oy = (double)(y + 0.5) * yfac - 0.5;
		oy1 = (int)oy;
		dy = oy - (double)oy1;

		for (uint32_t x = 0; x < p_dst_width; x++) {
			// X coordinates
			ox = (double)(x + 0.5) * xfac - 0.5;
			ox1 = (int)ox;
			dx = ox - (double)ox1;

			// initial pixel value

			T* __restrict dst = ((T*)p_dst) + (y * p_dst_width + x) * CC;

			double color[CC] = {};

			for (int n = -1; n < 3; n++) {
				// get Y coefficient
				[[maybe_unused]] double k1 = _bicubic_interp_kernel(dy - (double)n);

				oy2 = oy1 + n;
				if (oy2 < 0) {
					oy2 = 0;
				}
				if (oy2 > ymax) {
					oy2 = ymax;
				}

				for (int m = -1; m < 3; m++) {
					// get X coefficient
					[[maybe_unused]] double k2 = k1 * _bicubic_interp_kernel((double)m - dx);

					ox2 = ox1 + m;
					if (ox2 < 0) {
						ox2 = 0;
					}
					if (ox2 > xmax) {
						ox2 = xmax;
					}

					// get pixel of original image
					const T* __restrict p = ((T*)p_src) + (oy2 * p_src_width + ox2) * CC;

					for (int i = 0; i < CC; i++) {
						if constexpr (sizeof(T) == 2 && TYPE == IMAGE_SCALING_FLOAT) { // half float
							color[i] = Math::half_to_float(p[i]);
						}
						else {
							color[i] += p[i] * k2;
						}
					}
				}
			}

			for (int i = 0; i < CC; i++) {
				if constexpr (sizeof(T) == 1) { // byte
					dst[i] = CLAMP(Math::fast_ftoi(color[i]), 0, 255);
				}
				else if constexpr (sizeof(T) == 2) {
					if constexpr (TYPE == IMAGE_SCALING_FLOAT) {
						dst[i] = Math::make_half_float(color[i]); // half float
					}
					else {
						dst[i] = CLAMP(Math::fast_ftoi(color[i]), 0, 65535); // uint16
					}
				}
				else {
					dst[i] = color[i];
				}
			}
		}
	}
}

template <int CC, typename T, ImageScaleType TYPE>
static void _scale_bilinear(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	constexpr uint32_t FRAC_BITS = 8;
	constexpr uint32_t FRAC_LEN = (1 << FRAC_BITS);
	constexpr uint32_t FRAC_HALF = (FRAC_LEN >> 1);
	constexpr uint32_t FRAC_MASK = FRAC_LEN - 1;

	for (uint32_t i = 0; i < p_dst_height; i++) {
		// Add 0.5 in order to interpolate based on pixel center
		uint32_t src_yofs_up_fp = (i + 0.5) * p_src_height * FRAC_LEN / p_dst_height;
		// Calculate nearest src pixel center above current, and truncate to get y index
		uint32_t src_yofs_up =
			src_yofs_up_fp >= FRAC_HALF ? (src_yofs_up_fp - FRAC_HALF) >> FRAC_BITS : 0;
		uint32_t src_yofs_down = (src_yofs_up_fp + FRAC_HALF) >> FRAC_BITS;
		if (src_yofs_down >= p_src_height) {
			src_yofs_down = p_src_height - 1;
		}
		// Calculate distance to pixel center of src_yofs_up
		uint32_t src_yofs_frac = src_yofs_up_fp & FRAC_MASK;
		src_yofs_frac =
			src_yofs_frac >= FRAC_HALF ? src_yofs_frac - FRAC_HALF : src_yofs_frac + FRAC_HALF;

		uint32_t y_ofs_up = src_yofs_up * p_src_width * CC;
		uint32_t y_ofs_down = src_yofs_down * p_src_width * CC;

		for (uint32_t j = 0; j < p_dst_width; j++) {
			uint32_t src_xofs_left_fp = (j + 0.5) * p_src_width * FRAC_LEN / p_dst_width;
			uint32_t src_xofs_left =
				src_xofs_left_fp >= FRAC_HALF ? (src_xofs_left_fp - FRAC_HALF) >> FRAC_BITS : 0;
			uint32_t src_xofs_right = (src_xofs_left_fp + FRAC_HALF) >> FRAC_BITS;
			if (src_xofs_right >= p_src_width) {
				src_xofs_right = p_src_width - 1;
			}
			uint32_t src_xofs_frac = src_xofs_left_fp & FRAC_MASK;
			src_xofs_frac =
				src_xofs_frac >= FRAC_HALF ? src_xofs_frac - FRAC_HALF : src_xofs_frac + FRAC_HALF;

			src_xofs_left *= CC;
			src_xofs_right *= CC;

			for (uint32_t l = 0; l < CC; l++) {
				if constexpr (sizeof(T) == 1) { // uint8
					uint32_t p00 = p_src[y_ofs_up + src_xofs_left + l] << FRAC_BITS;
					uint32_t p10 = p_src[y_ofs_up + src_xofs_right + l] << FRAC_BITS;
					uint32_t p01 = p_src[y_ofs_down + src_xofs_left + l] << FRAC_BITS;
					uint32_t p11 = p_src[y_ofs_down + src_xofs_right + l] << FRAC_BITS;

					uint32_t interp_up = p00 + (((p10 - p00) * src_xofs_frac) >> FRAC_BITS);
					uint32_t interp_down = p01 + (((p11 - p01) * src_xofs_frac) >> FRAC_BITS);
					uint32_t interp =
						interp_up + (((interp_down - interp_up) * src_yofs_frac) >> FRAC_BITS);
					interp >>= FRAC_BITS;
					p_dst[i * p_dst_width * CC + j * CC + l] = uint8_t(interp);
				}
				else if constexpr (sizeof(T) == 2) {
					if constexpr (TYPE == IMAGE_SCALING_FLOAT) { // half float
						float xofs_frac = float(src_xofs_frac) / (1 << FRAC_BITS);
						float yofs_frac = float(src_yofs_frac) / (1 << FRAC_BITS);
						const T* src = ((const T*)p_src);
						T* dst = ((T*)p_dst);

						float p00 = Math::half_to_float(src[y_ofs_up + src_xofs_left + l]);
						float p10 = Math::half_to_float(src[y_ofs_up + src_xofs_right + l]);
						float p01 = Math::half_to_float(src[y_ofs_down + src_xofs_left + l]);
						float p11 = Math::half_to_float(src[y_ofs_down + src_xofs_right + l]);

						float interp_up = p00 + (p10 - p00) * xofs_frac;
						float interp_down = p01 + (p11 - p01) * xofs_frac;
						float interp = interp_up + ((interp_down - interp_up) * yofs_frac);

						dst[i * p_dst_width * CC + j * CC + l] = Math::make_half_float(interp);
					}
					else { // uint16
						float xofs_frac = float(src_xofs_frac) / (1 << FRAC_BITS);
						float yofs_frac = float(src_yofs_frac) / (1 << FRAC_BITS);
						const T* src = ((const T*)p_src);
						T* dst = ((T*)p_dst);

						float p00 = src[y_ofs_up + src_xofs_left + l];
						float p10 = src[y_ofs_up + src_xofs_right + l];
						float p01 = src[y_ofs_down + src_xofs_left + l];
						float p11 = src[y_ofs_down + src_xofs_right + l];

						float interp_up = p00 + (p10 - p00) * xofs_frac;
						float interp_down = p01 + (p11 - p01) * xofs_frac;
						float interp = interp_up + ((interp_down - interp_up) * yofs_frac);

						dst[i * p_dst_width * CC + j * CC + l] = uint16_t(interp);
					}
				}
				else if constexpr (sizeof(T) == 4) { // float

					float xofs_frac = float(src_xofs_frac) / (1 << FRAC_BITS);
					float yofs_frac = float(src_yofs_frac) / (1 << FRAC_BITS);
					const T* src = ((const T*)p_src);
					T* dst = ((T*)p_dst);

					float p00 = src[y_ofs_up + src_xofs_left + l];
					float p10 = src[y_ofs_up + src_xofs_right + l];
					float p01 = src[y_ofs_down + src_xofs_left + l];
					float p11 = src[y_ofs_down + src_xofs_right + l];

					float interp_up = p00 + (p10 - p00) * xofs_frac;
					float interp_down = p01 + (p11 - p01) * xofs_frac;
					float interp = interp_up + ((interp_down - interp_up) * yofs_frac);

					dst[i * p_dst_width * CC + j * CC + l] = interp;
				}
			}
		}
	}
}

template <int CC, typename T>
static void _scale_nearest(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	for (uint32_t i = 0; i < p_dst_height; i++) {
		uint32_t src_yofs = (i + 0.5) * p_src_height / p_dst_height;
		uint32_t y_ofs = src_yofs * p_src_width * CC;

		for (uint32_t j = 0; j < p_dst_width; j++) {
			uint32_t src_xofs = (j + 0.5) * p_src_width / p_dst_width;
			src_xofs *= CC;

			for (uint32_t l = 0; l < CC; l++) {
				const T* src = ((const T*)p_src);
				T* dst = ((T*)p_dst);

				T p = src[y_ofs + src_xofs + l];
				dst[i * p_dst_width * CC + j * CC + l] = p;
			}
		}
	}
}

#define LANCZOS_TYPE 3

static float _lanczos(float p_x)
{
	return Math::abs(p_x) >= LANCZOS_TYPE ? 0 : Math::sincn(p_x) * Math::sincn(p_x / LANCZOS_TYPE);
}

template <int CC, typename T, ImageScaleType TYPE>
static void _scale_lanczos(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst,
	uint32_t p_src_width, uint32_t p_src_height, uint32_t p_dst_width, uint32_t p_dst_height)
{
	int32_t src_width = p_src_width;
	int32_t src_height = p_src_height;
	int32_t dst_height = p_dst_height;
	int32_t dst_width = p_dst_width;

	uint32_t buffer_size = src_height * dst_width * CC;
	float* buffer = memnew_arr(float, buffer_size); // Store the first pass in a buffer

	{ // FIRST PASS (horizontal)

		float x_scale = float(src_width) / float(dst_width);

		float scale_factor = MAX(x_scale, 1); // A larger kernel is required only when downscaling
		int32_t half_kernel = LANCZOS_TYPE * scale_factor;

		float* kernel = memnew_arr(float, half_kernel * 2);

		for (int32_t buffer_x = 0; buffer_x < dst_width; buffer_x++) {
			// The corresponding point on the source image
			float src_x =
				(buffer_x + 0.5f) * x_scale; // Offset by 0.5 so it uses the pixel's center
			int32_t start_x = MAX(0, int32_t(src_x) - half_kernel + 1);
			int32_t end_x = MIN(src_width - 1, int32_t(src_x) + half_kernel);

			// Create the kernel used by all the pixels of the column
			for (int32_t target_x = start_x; target_x <= end_x; target_x++) {
				kernel[target_x - start_x] = _lanczos((target_x + 0.5f - src_x) / scale_factor);
			}

			for (int32_t buffer_y = 0; buffer_y < src_height; buffer_y++) {
				float pixel[CC] = {0};
				float weight = 0;

				for (int32_t target_x = start_x; target_x <= end_x; target_x++) {
					float lanczos_val = kernel[target_x - start_x];
					weight += lanczos_val;

					const T* __restrict src_data =
						((const T*)p_src) + (buffer_y * src_width + target_x) * CC;

					for (uint32_t i = 0; i < CC; i++) {
						if constexpr (sizeof(T) == 2 && TYPE == IMAGE_SCALING_FLOAT) { // half float
							pixel[i] += Math::half_to_float(src_data[i]) * lanczos_val;
						}
						else {
							pixel[i] += src_data[i] * lanczos_val;
						}
					}
				}

				float* dst_data = ((float*)buffer) + (buffer_y * dst_width + buffer_x) * CC;

				for (uint32_t i = 0; i < CC; i++) {
					dst_data[i] = pixel[i] / weight; // Normalize the sum of all the samples
				}
			}
		}

		memdelete_arr(kernel);
	} // End of first pass

	{ // SECOND PASS (vertical + result)

		float y_scale = float(src_height) / float(dst_height);

		float scale_factor = MAX(y_scale, 1);
		int32_t half_kernel = LANCZOS_TYPE * scale_factor;

		float* kernel = memnew_arr(float, half_kernel * 2);

		for (int32_t dst_y = 0; dst_y < dst_height; dst_y++) {
			float buffer_y = (dst_y + 0.5f) * y_scale;
			int32_t start_y = MAX(0, int32_t(buffer_y) - half_kernel + 1);
			int32_t end_y = MIN(src_height - 1, int32_t(buffer_y) + half_kernel);

			for (int32_t target_y = start_y; target_y <= end_y; target_y++) {
				kernel[target_y - start_y] = _lanczos((target_y + 0.5f - buffer_y) / scale_factor);
			}

			for (int32_t dst_x = 0; dst_x < dst_width; dst_x++) {
				float pixel[CC] = {0};
				float weight = 0;

				for (int32_t target_y = start_y; target_y <= end_y; target_y++) {
					float lanczos_val = kernel[target_y - start_y];
					weight += lanczos_val;

					float* buffer_data = ((float*)buffer) + (target_y * dst_width + dst_x) * CC;

					for (uint32_t i = 0; i < CC; i++) {
						pixel[i] += buffer_data[i] * lanczos_val;
					}
				}

				T* dst_data = ((T*)p_dst) + (dst_y * dst_width + dst_x) * CC;

				for (uint32_t i = 0; i < CC; i++) {
					pixel[i] /= weight;

					if constexpr (sizeof(T) == 1) { // byte
						dst_data[i] = CLAMP(Math::fast_ftoi(pixel[i]), 0, 255);
					}
					else if constexpr (sizeof(T) == 2) {
						if constexpr (TYPE == IMAGE_SCALING_FLOAT) { // half float
							dst_data[i] = Math::make_half_float(pixel[i]);
						}
						else { // uint16
							dst_data[i] = CLAMP(Math::fast_ftoi(pixel[i]), 0, 65535);
						}

					}
					else { // float
						dst_data[i] = pixel[i];
					}
				}
			}
		}

		memdelete_arr(kernel);
	} // End of second pass

	memdelete_arr(buffer);
}

static void _overlay(const uint8_t* __restrict p_src, uint8_t* __restrict p_dst, float p_alpha,
	uint32_t p_width, uint32_t p_height, uint32_t p_pixel_size)
{
	uint16_t alpha = MIN((uint16_t)(p_alpha * 256.0f), 256);

	for (uint32_t i = 0; i < p_width * p_height * p_pixel_size; i++) {
		p_dst[i] = (p_dst[i] * (256 - alpha) + p_src[i] * alpha) >> 8;
	}
}

bool Image::is_size_po2() const
{
	return Math::is_power_of_2(width) && Math::is_power_of_2(height);
}

void Image::resize_to_po2(bool p_square, Interpolation p_interpolation)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot resize in compressed image formats.");

	int w = Math::next_power_of_2((uint32_t)width);
	int h = Math::next_power_of_2((uint32_t)height);
	if (p_square) {
		w = h = MAX(w, h);
	}

	if (w == width && h == height) {
		if (!p_square || w == h) {
			return; // nothing to do
		}
	}

	resize(w, h, p_interpolation);
}

void Image::resize(int p_width, int p_height, Interpolation p_interpolation)
{
	ERR_FAIL_COND_MSG(
		data.is_empty(), "Cannot resize image before creating it, use set_data() first.");
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot resize in compressed image formats.");
	ERR_FAIL_COND_MSG(p_width <= 0, "Image width must be greater than 0.");
	ERR_FAIL_COND_MSG(p_height <= 0, "Image height must be greater than 0.");
	ERR_FAIL_COND_MSG(
		p_width > MAX_WIDTH, vformat("Image width cannot be greater than %d pixels.", MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_height > MAX_HEIGHT,
		vformat("Image height cannot be greater than %d pixels.", MAX_HEIGHT));
	ERR_FAIL_COND_MSG(p_width * p_height > MAX_PIXELS,
		vformat("Too many pixels for image, maximum is %d pixels.", MAX_PIXELS));

	if (p_width == width && p_height == height) {
		return;
	}

	// Convert the image to 'standard' RGB(A) formats that may be resized.
	Format original_format = format;
	if (original_format == FORMAT_RGB565 || original_format == FORMAT_RGBA4444) {
		convert(FORMAT_RGBA8);
	}
	else if (original_format == FORMAT_RGBE9995) {
		convert(FORMAT_RGBH);
	}

	Image dst(p_width, p_height, false, format);

	// Setup mipmap-aware scaling
	bool mipmap_aware =
		p_interpolation == INTERPOLATE_TRILINEAR /* || p_interpolation == INTERPOLATE_TRICUBIC */;

	Image dst2;
	int mip1 = 0;
	int mip2 = 0;
	float mip1_weight = 0;
	if (mipmap_aware) {
		float avg_scale = ((float)p_width / width + (float)p_height / height) * 0.5f;
		if (avg_scale >= 1.0f) {
			mipmap_aware = false;
		}
		else {
			float level = Math::log(1.0f / avg_scale) / Math::log(2.0f);
			mip1 = CLAMP((int)Math::floor(level), 0, get_mipmap_count());
			mip2 = CLAMP((int)Math::ceil(level), 0, get_mipmap_count());
			mip1_weight = 1.0f - (level - mip1);
		}
	}
	bool interpolate_mipmaps = mipmap_aware && mip1 != mip2;
	if (interpolate_mipmaps) {
		dst2.initialize_data(p_width, p_height, false, format);
	}

	bool had_mipmaps = mipmaps;
	if (interpolate_mipmaps && !had_mipmaps) {
		generate_mipmaps();
	}
	// --

	const uint8_t* r = data.ptr();
	const unsigned char* r_ptr = r;

	uint8_t* w = dst.data.ptrw();
	unsigned char* w_ptr = w;

	switch (p_interpolation) {
	case INTERPOLATE_NEAREST: {
		if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
			switch (get_format_pixel_size(format)) {
			case 1:
				_scale_nearest<1, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 2:
				_scale_nearest<2, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 3:
				_scale_nearest<3, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_nearest<4, uint8_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
			switch (get_format_pixel_size(format)) {
			case 4:
				_scale_nearest<1, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_nearest<2, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 12:
				_scale_nearest<3, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 16:
				_scale_nearest<4, float>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}

		}
		else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_nearest<1, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_nearest<2, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_nearest<3, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_nearest<4, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_nearest<1, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_nearest<2, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_nearest<3, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_nearest<4, uint16_t>(r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}

	} break;
	case INTERPOLATE_BILINEAR:
	case INTERPOLATE_TRILINEAR: {
		for (int i = 0; i < 2; ++i) {
			int src_width;
			int src_height;
			const unsigned char* src_ptr;

			if (!mipmap_aware) {
				if (i == 0) {
					// Standard behavior
					src_width = width;
					src_height = height;
					src_ptr = r_ptr;
				}
				else {
					// No need for a second iteration
					break;
				}
			}
			else {
				if (i == 0) {
					// Read from the first mipmap that will be interpolated
					// (if both levels are the same, we will not interpolate, but at least we'll
					// sample from the right level)
					int64_t offs;
					_get_mipmap_offset_and_size(mip1, offs, src_width, src_height);
					src_ptr = r_ptr + offs;
				}
				else if (!interpolate_mipmaps) {
					// No need generate a second image
					break;
				}
				else {
					// Switch to read from the second mipmap that will be interpolated
					int64_t offs;
					_get_mipmap_offset_and_size(mip2, offs, src_width, src_height);
					src_ptr = r_ptr + offs;
					// Switch to write to the second destination image
					w = dst2.data.ptrw();
					w_ptr = w;
				}
			}

			if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
				switch (get_format_pixel_size(format)) {
				case 1:
					_scale_bilinear<1, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 2:
					_scale_bilinear<2, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 3:
					_scale_bilinear<3, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 4:
					_scale_bilinear<4, uint8_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
			else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
				switch (get_format_pixel_size(format)) {
				case 4:
					_scale_bilinear<1, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 8:
					_scale_bilinear<2, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 12:
					_scale_bilinear<3, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 16:
					_scale_bilinear<4, float, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
			else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
				switch (get_format_pixel_size(format)) {
				case 2:
					_scale_bilinear<1, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 4:
					_scale_bilinear<2, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 6:
					_scale_bilinear<3, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 8:
					_scale_bilinear<4, uint16_t, IMAGE_SCALING_FLOAT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
			else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
				switch (get_format_pixel_size(format)) {
				case 2:
					_scale_bilinear<1, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 4:
					_scale_bilinear<2, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 6:
					_scale_bilinear<3, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				case 8:
					_scale_bilinear<4, uint16_t, IMAGE_SCALING_INT>(
						src_ptr, w_ptr, src_width, src_height, p_width, p_height);
					break;
				}
			}
		}

		if (interpolate_mipmaps) {
			// Switch to read again from the first scaled mipmap to overlay it over the second
			r = dst.data.ptr();
			_overlay(r, w, mip1_weight, p_width, p_height, get_format_pixel_size(format));
		}

	} break;
	case INTERPOLATE_CUBIC: {
		if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
			switch (get_format_pixel_size(format)) {
			case 1:
				_scale_cubic<1, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 2:
				_scale_cubic<2, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 3:
				_scale_cubic<3, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_cubic<4, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
			switch (get_format_pixel_size(format)) {
			case 4:
				_scale_cubic<1, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_cubic<2, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 12:
				_scale_cubic<3, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 16:
				_scale_cubic<4, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_cubic<1, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_cubic<2, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_cubic<3, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_cubic<4, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_cubic<1, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_cubic<2, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_cubic<3, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_cubic<4, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
	} break;
	case INTERPOLATE_LANCZOS: {
		if (format >= FORMAT_L8 && format <= FORMAT_RGBA8) {
			switch (get_format_pixel_size(format)) {
			case 1:
				_scale_lanczos<1, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 2:
				_scale_lanczos<2, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 3:
				_scale_lanczos<3, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_lanczos<4, uint8_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RF && format <= FORMAT_RGBAF) {
			switch (get_format_pixel_size(format)) {
			case 4:
				_scale_lanczos<1, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_lanczos<2, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 12:
				_scale_lanczos<3, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 16:
				_scale_lanczos<4, float, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_RH && format <= FORMAT_RGBAH) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_lanczos<1, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_lanczos<2, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_lanczos<3, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_lanczos<4, uint16_t, IMAGE_SCALING_FLOAT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
		else if (format >= FORMAT_R16 && format <= FORMAT_RGBA16I) {
			switch (get_format_pixel_size(format)) {
			case 2:
				_scale_lanczos<1, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 4:
				_scale_lanczos<2, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 6:
				_scale_lanczos<3, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			case 8:
				_scale_lanczos<4, uint16_t, IMAGE_SCALING_INT>(
					r_ptr, w_ptr, width, height, p_width, p_height);
				break;
			}
		}
	} break;
	}

	if (interpolate_mipmaps) {
		dst._copy_internals_from(dst2);
	}

	if (had_mipmaps) {
		dst.generate_mipmaps();
	}

	_copy_internals_from(dst);

	// Reconvert the image to its original format.
	if (original_format != format) {
		convert(original_format);
	}
}

void Image::crop_from_point(int p_x, int p_y, int p_width, int p_height)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot crop in compressed image formats.");
	ERR_FAIL_COND_MSG(p_x < 0, "Start x position cannot be smaller than 0.");
	ERR_FAIL_COND_MSG(p_y < 0, "Start y position cannot be smaller than 0.");
	ERR_FAIL_COND_MSG(p_width <= 0, "Width of image must be greater than 0.");
	ERR_FAIL_COND_MSG(p_height <= 0, "Height of image must be greater than 0.");
	ERR_FAIL_COND_MSG(
		p_x + p_width > MAX_WIDTH, vformat("End x position cannot be greater than %d.", MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_y + p_height > MAX_HEIGHT,
		vformat("End y position cannot be greater than %d.", MAX_HEIGHT));

	/* to save memory, cropping should be done in-place, however, since this function
	   will most likely either not be used much, or in critical areas, for now it won't, because
	   it's a waste of time. */

	if (p_width == width && p_height == height && p_x == 0 && p_y == 0) {
		return;
	}

	uint8_t pdata[16]; // largest is 16
	uint32_t pixel_size = get_format_pixel_size(format);

	Image dst(p_width, p_height, false, format);

	{
		const uint8_t* r = data.ptr();
		uint8_t* w = dst.data.ptrw();

		int m_h = p_y + p_height;
		int m_w = p_x + p_width;
		for (int y = p_y; y < m_h; y++) {
			for (int x = p_x; x < m_w; x++) {
				if ((x >= width || y >= height)) {
					for (uint32_t i = 0; i < pixel_size; i++) {
						pdata[i] = 0;
					}
				}
				else {
					_get_pixelb(x, y, pixel_size, r, pdata);
				}

				dst._put_pixelb(x - p_x, y - p_y, pixel_size, w, pdata);
			}
		}
	}

	if (has_mipmaps()) {
		dst.generate_mipmaps();
	}
	_copy_internals_from(dst);
}

void Image::crop(int p_width, int p_height) { crop_from_point(0, 0, p_width, p_height); }

void Image::rotate_90(ClockDirection p_direction)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot rotate in compressed image formats.");
	ERR_FAIL_COND_MSG(width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", width));
	ERR_FAIL_COND_MSG(height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", height));

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	// In-place 90 degrees rotation by following the permutation cycles.
	{
		// Explanation by example (clockwise):
		//
		//  abc      da
		//  def  ->  eb
		//           fc
		//
		// In memory:
		//  012345    012345
		//  abcdef -> daebfc
		//
		// Permutation cycles:
		//  (0  --a-->  1  --b-->  3  --d-->  0)
		//  (2  --c-->  5  --f-->  4  --e-->  2)
		//
		// Applying cycles (backwards):
		//  0->s  s=a (store)
		//  3->0  abcdef -> dbcdef
		//  1->3  dbcdef -> dbcbef
		//  s->1  dbcbef -> dacbef
		//
		//  2->s  s=c
		//  4->2  dacbef -> daebef
		//  5->4  daebef -> daebff
		//  s->5  daebff -> daebfc

		const int w = width;
		const int h = height;
		const int size = w * h;

		uint8_t* data_ptr = data.ptrw();
		uint32_t pixel_size = get_format_pixel_size(format);

		uint8_t single_pixel_buffer[16];

#define PREV_INDEX_IN_CYCLE(index)                                                                 \
	(p_direction == CLOCKWISE) ? ((h - 1 - (index % h)) * w + (index / h))                         \
							   : ((index % h) * w + (w - 1 - (index / h)))

		if (w == h) { // Square case, 4-length cycles only (plus irrelevant thus skipped 1-length
					  // cycle in the middle for odd-sized squares).
			for (int y = 0; y < h / 2; y++) {
				for (int x = 0; x < (w + 1) / 2; x++) {
					int current = y * w + x;
					memcpy(single_pixel_buffer, data_ptr + current * pixel_size, pixel_size);
					for (int i = 0; i < 3; i++) {
						int prev = PREV_INDEX_IN_CYCLE(current);
						memcpy(data_ptr + current * pixel_size, data_ptr + prev * pixel_size,
							pixel_size);
						current = prev;
					}
					memcpy(data_ptr + current * pixel_size, single_pixel_buffer, pixel_size);
				}
			}
		}
		else { // Rectangular case (w != h), kinda unpredictable cycles.
			int permuted_pixels_count = 0;

			for (int i = 0; i < size; i++) {
				int prev = PREV_INDEX_IN_CYCLE(i);
				if (prev == i) {
					// 1-length cycle, pixel remains at the same index.
					permuted_pixels_count++;
					continue;
				}

				// Check whether we already processed this cycle.
				// We iterate over it and if we'll find an index smaller than `i` then we already
				// processed this cycle because we always start at the smallest index in the cycle.
				// TODO: Improve this naive approach, can be done better.
				while (prev > i) {
					prev = PREV_INDEX_IN_CYCLE(prev);
				}
				if (prev < i) {
					continue;
				}

				// Save the in-cycle pixel with the smallest index (`i`).
				memcpy(single_pixel_buffer, data_ptr + i * pixel_size, pixel_size);

				// Overwrite pixels one by one by the preceding pixel in the cycle.
				int current = i;
				prev = PREV_INDEX_IN_CYCLE(current);
				while (prev != i) {
					memcpy(
						data_ptr + current * pixel_size, data_ptr + prev * pixel_size, pixel_size);
					permuted_pixels_count++;

					current = prev;
					prev = PREV_INDEX_IN_CYCLE(current);
				};

				// Overwrite the remaining pixel in the cycle by the saved pixel with the smallest
				// index.
				memcpy(data_ptr + current * pixel_size, single_pixel_buffer, pixel_size);
				permuted_pixels_count++;

				if (permuted_pixels_count == size) {
					break;
				}
			}

			width = h;
			height = w;
		}

#undef PREV_INDEX_IN_CYCLE
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

void Image::rotate_180()
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot rotate in compressed image formats.");
	ERR_FAIL_COND_MSG(width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", width));
	ERR_FAIL_COND_MSG(height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", height));

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	{
		uint8_t* data_ptr = data.ptrw();
		uint32_t pixel_size = get_format_pixel_size(format);

		uint8_t single_pixel_buffer[16];

		uint8_t* from_begin_ptr = data_ptr;
		uint8_t* from_end_ptr = data_ptr + (width * height - 1) * pixel_size;

		while (from_begin_ptr < from_end_ptr) {
			memcpy(single_pixel_buffer, from_begin_ptr, pixel_size);
			memcpy(from_begin_ptr, from_end_ptr, pixel_size);
			memcpy(from_end_ptr, single_pixel_buffer, pixel_size);

			from_begin_ptr += pixel_size;
			from_end_ptr -= pixel_size;
		}
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

void Image::flip_y()
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot flip_y in compressed image formats.");

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	{
		uint8_t* w = data.ptrw();
		uint8_t up[16];
		uint8_t down[16];
		uint32_t pixel_size = get_format_pixel_size(format);

		for (int y = 0; y < height / 2; y++) {
			for (int x = 0; x < width; x++) {
				_get_pixelb(x, y, pixel_size, w, up);
				_get_pixelb(x, height - y - 1, pixel_size, w, down);

				_put_pixelb(x, height - y - 1, pixel_size, w, up);
				_put_pixelb(x, y, pixel_size, w, down);
			}
		}
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

void Image::flip_x()
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot flip_x in compressed image formats.");

	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	{
		uint8_t* w = data.ptrw();
		uint8_t up[16];
		uint8_t down[16];
		uint32_t pixel_size = get_format_pixel_size(format);

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width / 2; x++) {
				_get_pixelb(x, y, pixel_size, w, up);
				_get_pixelb(width - x - 1, y, pixel_size, w, down);

				_put_pixelb(width - x - 1, y, pixel_size, w, up);
				_put_pixelb(x, y, pixel_size, w, down);
			}
		}
	}

	if (used_mipmaps) {
		generate_mipmaps();
	}
}

// Get mipmap size and offset.
int64_t Image::_get_dst_image_size(int p_width, int p_height, Format p_format, int& r_mipmaps,
	int p_mipmaps, int* r_mm_width, int* r_mm_height)
{
	// Data offset in mipmaps (including the original texture).
	int64_t size = 0;

	int w = p_width;
	int h = p_height;

	// Current mipmap index in the loop below. p_mipmaps is the target mipmap index.
	// In this function, mipmap 0 represents the first mipmap instead of the original texture.
	int mm = 0;

	int pixsize = get_format_pixel_size(p_format);
	int block = get_format_block_size(p_format);

	// Technically, you can still compress up to 1 px no matter the format, so commenting this.
	// int minw, minh;
	// get_format_min_pixel_size(p_format, minw, minh);
	int minw = 1, minh = 1;

	while (true) {
		int bw = w % block != 0 ? w + (block - w % block) : w;
		int bh = h % block != 0 ? h + (block - h % block
) : h;

		int64_t s = bw * bh;

		s *= pixsize;
		s = get_format_pixels_shifted(p_format, s);

		size += s;

		if (p_mipmaps >= 0) {
			w = MAX(minw, w >> 1);
			h = MAX(minh, h >> 1);
		}
		else {
			if (w == minw && h == minh) {
				break;
			}
			w = MAX(minw, w >> 1);
			h = MAX(minh, h >> 1);
		}

		// Set mipmap size.
		if (r_mm_width) {
			*r_mm_width = w;
		}
		if (r_mm_height) {
			*r_mm_height = h;
		}

		// Reach target mipmap.
		if (p_mipmaps >= 0 && mm == p_mipmaps) {
			break;
		}

		mm++;
	}

	r_mipmaps = mm;
	return size;
}

template <typename Component, int CC, bool renormalize,
	void (*average_func)(
		Component&, const Component&, const Component&, const Component&, const Component&),
	void (*renormalize_func)(Component*)>
static void _generate_po2_mipmap(
	const Component* p_src, Component* p_dst, uint32_t p_width, uint32_t p_height)
{
	// Fast power of 2 mipmap generation.
	uint32_t dst_w = MAX(p_width >> 1, 1u);
	uint32_t dst_h = MAX(p_height >> 1, 1u);

	int right_step = (p_width == 1) ? 0 : CC;
	int down_step = (p_height == 1) ? 0 : (p_width * CC);

	for (uint32_t i = 0; i < dst_h; i++) {
		const Component* rup_ptr = &p_src[i * 2 * down_step];
		const Component* rdown_ptr = rup_ptr + down_step;
		Component* dst_ptr = &p_dst[i * dst_w * CC];
		uint32_t count = dst_w;

		while (count) {
			count--;
			for (int j = 0; j < CC; j++) {
				average_func(dst_ptr[j], rup_ptr[j], rup_ptr[j + right_step], rdown_ptr[j],
					rdown_ptr[j + right_step]);
			}

			if constexpr (renormalize) {
				renormalize_func(dst_ptr);
			}

			dst_ptr += CC;
			rup_ptr += right_step * 2;
			rdown_ptr += right_step * 2;
		}
	}
}

void Image::_generate_mipmap_from_format(Image::Format p_format, const uint8_t* p_src,
	uint8_t* p_dst, uint32_t p_width, uint32_t p_height, bool p_renormalize)
{
	const float* src_float = reinterpret_cast<const float*>(p_src);
	float* dst_float = reinterpret_cast<float*>(p_dst);

	const uint16_t* src_u16 = reinterpret_cast<const uint16_t*>(p_src);
	uint16_t* dst_u16 = reinterpret_cast<uint16_t*>(p_dst);

	const uint32_t* src_u32 = reinterpret_cast<const uint32_t*>(p_src);
	uint32_t* dst_u32 = reinterpret_cast<uint32_t*>(p_dst);

	switch (p_format) {
	case Image::FORMAT_L8:
	case Image::FORMAT_R8:
		_generate_po2_mipmap<uint8_t, 1, false, Image::average_4_uint8, Image::renormalize_uint8>(
			p_src, p_dst, p_width, p_height);
		break;
	case Image::FORMAT_LA8:
		_generate_po2_mipmap<uint8_t, 2, false, Image::average_4_uint8, Image::renormalize_uint8>(
			p_src, p_dst, p_width, p_height);
		break;
	case Image::FORMAT_RG8:
		_generate_po2_mipmap<uint8_t, 2, false, Image::average_4_uint8, Image::renormalize_uint8>(
			p_src, p_dst, p_width, p_height);
		break;
	case Image::FORMAT_RGB8: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint8_t, 3, true, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint8_t, 3, false, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBA8: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint8_t, 4, true, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint8_t, 4, false, Image::average_4_uint8,
				Image::renormalize_uint8>(p_src, p_dst, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBA4444: {
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_rgba4444, nullptr>(
			src_u16, dst_u16, p_width, p_height);
	} break;
	case Image::FORMAT_RGB565: {
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_rgb565, nullptr>(
			src_u16, dst_u16, p_width, p_height);
	} break;
	case Image::FORMAT_RF:
		_generate_po2_mipmap<float, 1, false, Image::average_4_float, Image::renormalize_float>(
			src_float, dst_float, p_width, p_height);
		break;
	case Image::FORMAT_RGF:
		_generate_po2_mipmap<float, 2, false, Image::average_4_float, Image::renormalize_float>(
			src_float, dst_float, p_width, p_height);
		break;
	case Image::FORMAT_RGBF: {
		if (p_renormalize) {
			_generate_po2_mipmap<float, 3, true, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<float, 3, false, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBAF: {
		if (p_renormalize) {
			_generate_po2_mipmap<float, 4, true, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<float, 4, false, Image::average_4_float, Image::renormalize_float>(
				src_float, dst_float, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RH:
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_half, Image::renormalize_half>(
			src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RGH:
		_generate_po2_mipmap<uint16_t, 2, false, Image::average_4_half, Image::renormalize_half>(
			src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RGBH: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 3, true, Image::average_4_half, Image::renormalize_half>(
				src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 3, false, Image::average_4_half,
				Image::renormalize_half>(src_u16, dst_u16, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBAH: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 4, true, Image::average_4_half, Image::renormalize_half>(
				src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 4, false, Image::average_4_half,
				Image::renormalize_half>(src_u16, dst_u16, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBE9995:
		_generate_po2_mipmap<uint32_t, 1, false, Image::average_4_rgbe9995, nullptr>(
			src_u32, dst_u32, p_width, p_height);
		break;
	case Image::FORMAT_R16:
	case Image::FORMAT_R16I:
		_generate_po2_mipmap<uint16_t, 1, false, Image::average_4_uint16,
			Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RG16:
	case Image::FORMAT_RG16I:
		_generate_po2_mipmap<uint16_t, 2, false, Image::average_4_uint16,
			Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		break;
	case Image::FORMAT_RGB16:
	case Image::FORMAT_RGB16I: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 3, true, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 3, false, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
	} break;
	case Image::FORMAT_RGBA16:
	case Image::FORMAT_RGBA16I: {
		if (p_renormalize) {
			_generate_po2_mipmap<uint16_t, 4, true, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
		else {
			_generate_po2_mipmap<uint16_t, 4, false, Image::average_4_uint16,
				Image::renormalize_uint16>(src_u16, dst_u16, p_width, p_height);
		}
	} break;

	default:
		return;
	}
}

void Image::shrink_x2()
{
	ERR_FAIL_COND(data.is_empty());
	Vector<uint8_t> new_data;

	if (mipmaps) {
		// Just use the lower mipmap as base and copy all.
		int64_t ofs = get_mipmap_offset(1);
		int64_t new_size = data.size() - ofs;

		new_data.resize(new_size);
		ERR_FAIL_COND(new_data.is_empty());

		memcpy(new_data.ptrw(), data.ptr() + ofs, new_size);
	}
	else {
		// Generate a mipmap and replace the original.
		ERR_FAIL_COND(is_compressed());

		new_data.resize((width / 2) * (height / 2) * get_format_pixel_size(format));
		ERR_FAIL_COND(data.is_empty() || new_data.is_empty());

		_generate_mipmap_from_format(format, data.ptr(), new_data.ptrw(), width, height, false);
	}

	width = MAX(width / 2, 1);
	height = MAX(height / 2, 1);
	data = new_data;
}

void Image::normalize()
{
	bool used_mipmaps = has_mipmaps();
	if (used_mipmaps) {
		clear_mipmaps();
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			Color c = get_pixel(x, y);
			Vector3 v(c.r * 2.0 - 1.0, c.g * 2.0 - 1.0, c.b * 2.0 - 1.0);
			v.normalize();
			c.r = v.x * 0.5 + 0.5;
			c.g = v.y * 0.5 + 0.5;
			c.b = v.z * 0.5 + 0.5;
			set_pixel(x, y, c);
		}
	}

	if (used_mipmaps) {
		generate_mipmaps(true);
	}
}

Error Image::generate_mipmaps(bool p_renormalize)
{
	ERR_FAIL_COND_V_MSG(
		is_compressed(), ERR_UNAVAILABLE, "Cannot generate mipmaps from compressed image formats.");
	ERR_FAIL_COND_V_MSG(width == 0 || height == 0, ERR_UNCONFIGURED,
		"Cannot generate mipmaps with width or height equal to 0.");

	int gen_mipmap_count;

	int64_t size = _get_dst_image_size(width, height, format, gen_mipmap_count);
	data.resize(size);
	uint8_t* wp = data.ptrw();

	int prev_ofs = 0;
	int prev_h = height;
	int prev_w = width;

	for (int i = 1; i <= gen_mipmap_count; i++) {
		int64_t ofs;
		int w, h;
		_get_mipmap_offset_and_size(i, ofs, w, h);

		_generate_mipmap_from_format(
			format, wp + prev_ofs, wp + ofs, prev_w, prev_h, p_renormalize);

		prev_ofs = ofs;
		prev_w = w;
		prev_h = h;
	}

	mipmaps = true;

	return OK;
}

void Image::clear_mipmaps()
{
	if (!mipmaps) {
		return;
	}

	if (is_empty()) {
		return;
	}

	int64_t ofs;
	int w, h;
	_get_mipmap_offset_and_size(1, ofs, w, h);
	data.resize(ofs);

	mipmaps = false;
}

bool Image::is_empty() const { return (data.is_empty()); }

const Vector<uint8_t>& Image::get_data() const { return data; }

Ref<Image> Image::create_empty(int p_width, int p_height, bool p_use_mipmaps, Format p_format)
{
	Ref<Image> image;
	image.instantiate();
	image->initialize_data(p_width, p_height, p_use_mipmaps, p_format);
	return image;
}

Ref<Image> Image::create_from_data(
	int p_width, int p_height, bool p_use_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	Ref<Image> image;
	image.instantiate();
	image->initialize_data(p_width, p_height, p_use_mipmaps, p_format, p_data);
	return image;
}

void Image::set_data(
	int p_width, int p_height, bool p_use_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	initialize_data(p_width, p_height, p_use_mipmaps, p_format, p_data);
}

void Image::initialize_data(int p_width, int p_height, bool p_use_mipmaps, Format p_format)
{
	ERR_FAIL_COND_MSG(p_width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", p_width));
	ERR_FAIL_COND_MSG(p_height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", p_height));
	ERR_FAIL_COND_MSG(p_width > MAX_WIDTH,
		vformat("The Image width specified (%d pixels) cannot be greater than %d pixels.", p_width,
			MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_height > MAX_HEIGHT,
		vformat("The Image height specified (%d pixels) cannot be greater than %d pixels.",
			p_height, MAX_HEIGHT));
	ERR_FAIL_COND_MSG(p_width * p_height > MAX_PIXELS,
		vformat("Too many pixels for Image. Maximum is %dx%d = %d pixels.", MAX_WIDTH, MAX_HEIGHT,
			MAX_PIXELS));
	ERR_FAIL_INDEX_MSG(p_format, FORMAT_MAX,
		vformat(
			"The Image format specified (%d) is out of range. See Image's Format enum.", p_format));

	int mm = 0;
	int64_t size = _get_dst_image_size(p_width, p_height, p_format, mm, p_use_mipmaps ? -1 : 0);
	data.resize(size);

	{
		uint8_t* w = data.ptrw();
		memset(w, 0, size);
	}

	width = p_width;
	height = p_height;
	mipmaps = p_use_mipmaps;
	format = p_format;
}

void Image::initialize_data(
	int p_width, int p_height, bool p_use_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND_MSG(p_width <= 0,
		vformat("The Image width specified (%d pixels) must be greater than 0 pixels.", p_width));
	ERR_FAIL_COND_MSG(p_height <= 0,
		vformat("The Image height specified (%d pixels) must be greater than 0 pixels.", p_height));
	ERR_FAIL_COND_MSG(p_width > MAX_WIDTH,
		vformat("The Image width specified (%d pixels) cannot be greater than %d pixels.", p_width,
			MAX_WIDTH));
	ERR_FAIL_COND_MSG(p_height > MAX_HEIGHT,
		vformat("The Image height specified (%d pixels) cannot be greater than %d pixels.",
			p_height, MAX_HEIGHT));
	ERR_FAIL_COND_MSG(p_width * p_height > MAX_PIXELS,
		vformat("Too many pixels for Image. Maximum is %dx%d = %d pixels.", MAX_WIDTH, MAX_HEIGHT,
			MAX_PIXELS));
	ERR_FAIL_INDEX_MSG(p_format, FORMAT_MAX,
		vformat(
			"The Image format specified (%d) is out of range. See Image's Format enum.", p_format));

	int mm;
	int64_t size = _get_dst_image_size(p_width, p_height, p_format, mm, p_use_mipmaps ? -1 : 0);

	if (unlikely(p_data.size() != size)) {
		String description_mipmaps = get_format_name(p_format) + " ";
		if (p_use_mipmaps) {
			const int num_mipmaps = get_image_required_mipmaps(p_width, p_height, p_format);
			if (num_mipmaps != 1) {
				description_mipmaps += vformat("with %d mipmaps", num_mipmaps);
			}
			else {
				description_mipmaps += "with 1 mipmap";
			}
		}
		else {
			description_mipmaps += "without mipmaps";
		}
		const String description = vformat("%dx%dx%d (%s)", p_width, p_height,
			get_format_pixel_size(p_format), description_mipmaps);
		ERR_FAIL_MSG(vformat("Expected Image data size of %s = %d bytes, got %d bytes instead.",
			description, size, p_data.size()));
	}

	height = p_height;
	width = p_width;
	format = p_format;
	data = p_data;

	mipmaps = p_use_mipmaps;
}

void Image::initialize_data(const char** p_xpm)
{
	int size_width = 0;
	int size_height = 0;
	int pixelchars = 0;
	mipmaps = false;
	bool has_alpha = false;

	enum Status
	{
		READING_HEADER,
		READING_COLORS,
		READING_PIXELS,
		DONE
	};

	Status status = READING_HEADER;
	int line = 0;

	HashMap<String, Color> colormap;
	int colormap_size = 0;
	uint32_t pixel_size = 0;
	uint8_t* data_write = nullptr;

	while (status != DONE) {
		const char* line_ptr = p_xpm[line];

		switch (status) {
		case READING_HEADER: {
			String line_str = line_ptr;
			line_str = line_str.replace_char('\t', ' ');

			size_width = line_str.get_slicec(' ', 0).to_int();
			size_height = line_str.get_slicec(' ', 1).to_int();
			colormap_size = line_str.get_slicec(' ', 2).to_int();
			pixelchars = line_str.get_slicec(' ', 3).to_int();
			ERR_FAIL_COND(colormap_size > 32766);
			ERR_FAIL_COND(pixelchars > 5);
			ERR_FAIL_COND(size_width > 32767);
			ERR_FAIL_COND(size_height > 32767);
			status = READING_COLORS;
		} break;
		case READING_COLORS: {
			String colorstring;
			for (int i = 0; i < pixelchars; i++) {
				colorstring += *line_ptr;
				line_ptr++;
			}
			// skip spaces
			while (*line_ptr == ' ' || *line_ptr == '\t' || *line_ptr == 0) {
				if (*line_ptr == 0) {
					break;
				}
				line_ptr++;
			}
			if (*line_ptr == 'c') {
				line_ptr++;
				while (*line_ptr == ' ' || *line_ptr == '\t' || *line_ptr == 0) {
					if (*line_ptr == 0) {
						break;
					}
					line_ptr++;
				}

				if (*line_ptr == '#') {
					line_ptr++;
					uint8_t col_r = 0;
					uint8_t col_g = 0;
					uint8_t col_b = 0;
					// uint8_t col_a=255;

					for (int i = 0; i < 6; i++) {
						char v = line_ptr[i];

						if (is_digit(v)) {
							v -= '0';
						}
						else if (v >= 'A' && v <= 'F') {
							v = (v - 'A') + 10;
						}
						else if (v >= 'a' && v <= 'f') {
							v = (v - 'a') + 10;
						}
						else {
							break;
						}

						switch (i) {
						case 0:
							col_r = v << 4;
							break;
						case 1:
							col_r |= v;
							break;
						case 2:
							col_g = v << 4;
							break;
						case 3:
							col_g |= v;
							break;
						case 4:
							col_b = v << 4;
							break;
						case 5:
							col_b |= v;
							break;
						}
					}

					// magenta mask
					if (col_r == 255 && col_g == 0 && col_b == 255) {
						colormap[colorstring] = Color(0, 0, 0, 0);
						has_alpha = true;
					}
					else {
						colormap[colorstring] =
							Color(col_r / 255.0, col_g / 255.0, col_b / 255.0, 1.0);
					}
				}
			}
			if (line == colormap_size) {
				status = READING_PIXELS;
				initialize_data(
					size_width, size_height, false, has_alpha ? FORMAT_RGBA8 : FORMAT_RGB8);
				data_write = data.ptrw();
				pixel_size = has_alpha ? 4 : 3;
			}
		} break;
		case READING_PIXELS: {
			int y = line - colormap_size - 1;
			for (int x = 0; x < size_width; x++) {
				char pixelstr[6] = {0, 0, 0, 0, 0, 0};
				for (int i = 0; i < pixelchars; i++) {
					pixelstr[i] = line_ptr[x * pixelchars + i];
				}

				Color* colorptr = colormap.getptr(pixelstr);
				ERR_FAIL_NULL(colorptr);
				uint8_t pixel[4];
				for (uint32_t i = 0; i < pixel_size; i++) {
					pixel[i] = CLAMP((*colorptr)[i] * 255, 0, 255);
				}
				_put_pixelb(x, y, pixel_size, data_write, pixel);
			}

			if (y == (size_height - 1)) {
				status = DONE;
			}
		} break;
		default: {
		}
		}

		line++;
	}
}

#define DETECT_ALPHA_MAX_THRESHOLD 254
#define DETECT_ALPHA_MIN_THRESHOLD 2

#define DETECT_ALPHA(m_value)                                                                      \
	{                                                                                              \
		uint8_t value = m_value;                                                                   \
		if (value < DETECT_ALPHA_MIN_THRESHOLD)                                                    \
			bit = true;                                                                            \
		else if (value < DETECT_ALPHA_MAX_THRESHOLD) {                                             \
			detected = true;                                                                       \
			break;                                                                                 \
		}                                                                                          \
	}

#define DETECT_NON_ALPHA(m_value)                                                                  \
	{                                                                                              \
		uint8_t value = m_value;                                                                   \
		if (value > 0) {                                                                           \
			detected = true;                                                                       \
			break;                                                                                 \
		}                                                                                          \
	}

bool Image::is_invisible() const
{
	int w, h;
	int64_t len;
	_get_mipmap_offset_and_size(1, len, w, h);

	if (len == 0) {
		return true;
	}

	switch (format) {
	case FORMAT_LA8: {
		const int pixel_count = len / 2;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i] & 0xFF00) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBA8: {
		const int pixel_count = len / 4;
		const uint32_t* pixeldata = reinterpret_cast<const uint32_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i] & 0xFF000000) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBA4444: {
		const int pixel_count = len / 2;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i] & 0x000F) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBAH: {
		// The alpha mask accounts for the sign bit.
		const int pixel_count = len / 8;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i * 4 + 3] & 0x7FFF) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBAF: {
		// The alpha mask accounts for the sign bit.
		const int pixel_count = len / 16;
		const uint32_t* pixeldata = reinterpret_cast<const uint32_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if ((pixeldata[i * 4 + 3] & 0x7FFFFFFF) != 0) {
				return false;
			}
		}
	} break;
	case FORMAT_RGBA16:
	case FORMAT_RGBA16I: {
		const int pixel_count = len / 8;
		const uint16_t* pixeldata = reinterpret_cast<const uint16_t*>(data.ptr());

		for (int i = 0; i < pixel_count; i++) {
			if (pixeldata[i * 4 + 3] != 0) {
				return false;
			}
		}
	} break;
	default: {
		// Formats that are compressed or don't support alpha channels are presumed to be visible.
		return false;
	}
	}

	// Every pixel has been checked, the image is invisible.
	return true;
}

Image::AlphaMode Image::detect_alpha() const
{
	int64_t len = data.size();

	if (len == 0) {
		return ALPHA_NONE;
	}

	int w, h;
	_get_mipmap_offset_and_size(1, len, w, h);

	const uint8_t* r = data.ptr();
	const unsigned char* data_ptr = r;

	bool bit = false;
	bool detected = false;

	switch (format) {
	case FORMAT_LA8: {
		for (int i = 0; i < (len >> 1); i++) {
			DETECT_ALPHA(data_ptr[(i << 1) + 1]);
		}

	} break;
	case FORMAT_RGBA8: {
		for (int i = 0; i < (len >> 2); i++) {
			DETECT_ALPHA(data_ptr[(i << 2) + 3])
		}

	} break;
	case FORMAT_DXT3:
	case FORMAT_DXT5: {
		detected = true;
	} break;
	default: {
	}
	}

	if (detected) {
		return ALPHA_BLEND;
	}
	else if (bit) {
		return ALPHA_BIT;
	}
	else {
		return ALPHA_NONE;
	}
}

Error Image::load(const String& p_path)
{
	String path = ResourceUID::ensure_path(p_path);
#ifdef DEBUG_ENABLED
	if (path.begins_with("res://") && ResourceLoader::exists(path)) {
		WARN_PRINT(vformat(
			"Loaded resource as image file, this will not work on export: '%s'. Instead, import "
			"the image file as an Image resource and load it normally as a resource.",
			path));
	}
#endif
	return ImageLoader::load_image(path, this);
}

Ref<Image> Image::load_from_file(const String& p_path)
{
	String path = ResourceUID::ensure_path(p_path);
#ifdef DEBUG_ENABLED
	if (path.begins_with("res://") && ResourceLoader::exists(path)) {
		WARN_PRINT(vformat(
			"Loaded resource as image file, this will not work on export: '%s'. Instead, import "
			"the image file as an Image resource and load it normally as a resource.",
			path));
	}
#endif
	Ref<Image> image;
	image.instantiate();
	Error err = ImageLoader::load_image(path, image);
	if (err != OK) {
		ERR_FAIL_V_MSG(Ref<Image>(), vformat("Failed to load image. Error %d", err));
	}
	return image;
}

Error Image::save_png(const String& p_path) const
{
	if (save_png_func == nullptr) {
		return ERR_UNAVAILABLE;
	}

	return save_png_func(p_path, Ref<Image>((Image*)this));
}

Error Image::save_jpg(const String& p_path, float p_quality) const
{
	if (save_jpg_func == nullptr) {
		return ERR_UNAVAILABLE;
	}

	return save_jpg_func(p_path, Ref<Image>((Image*)this), p_quality);
}

Vector<uint8_t> Image::save_png_to_buffer() const
{
	if (save_png_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}

	return save_png_buffer_func(Ref<Image>((Image*)this));
}

Vector<uint8_t> Image::save_jpg_to_buffer(float p_quality) const
{
	if (save_jpg_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}

	return save_jpg_buffer_func(Ref<Image>((Image*)this), p_quality);
}

Error Image::save_exr(
	const String& p_path, bool p_grayscale, bool p_color_image, float p_max_value) const
{
	if (save_exr_func == nullptr) {
		return ERR_UNAVAILABLE;
	}
	return save_exr_func(p_path, Ref<Image>((Image*)this), p_grayscale, p_color_image, p_max_value);
}

Vector<uint8_t> Image::save_exr_to_buffer(
	bool p_grayscale, bool p_color_image, float p_max_value) const
{
	if (save_exr_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}
	return save_exr_buffer_func(Ref<Image>((Image*)this), p_grayscale, p_color_image, p_max_value);
}

Error Image::save_dds(const String& p_path) const
{
	if (save_dds_func == nullptr) {
		return ERR_UNAVAILABLE;
	}

	return save_dds_func(p_path, Ref<Image>((Image*)this));
}

Vector<uint8_t> Image::save_dds_to_buffer() const
{
	if (save_dds_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}
	return save_dds_buffer_func(Ref<Image>((Image*)this));
}

Error Image::save_webp(const String& p_path, const bool p_lossy, const float p_quality) const
{
	if (save_webp_func == nullptr) {
		return ERR_UNAVAILABLE;
	}
	ERR_FAIL_COND_V_MSG(p_lossy && !(0.0f <= p_quality && p_quality <= 1.0f), ERR_INVALID_PARAMETER,
		vformat("The WebP lossy quality was set to %f, which is not valid. WebP lossy quality must "
				"be between 0.0 and 1.0 (inclusive).",
			p_quality));

	return save_webp_func(p_path, Ref<Image>((Image*)this), p_lossy, p_quality);
}

Vector<uint8_t> Image::save_webp_to_buffer(const bool p_lossy, const float p_quality) const
{
	if (save_webp_buffer_func == nullptr) {
		return Vector<uint8_t>();
	}
	ERR_FAIL_COND_V_MSG(p_lossy && !(0.0f <= p_quality && p_quality <= 1.0f), Vector<uint8_t>(),
		vformat("The WebP lossy quality was set to %f, which is not valid. WebP lossy quality must "
				"be between 0.0 and 1.0 (inclusive).",
			p_quality));

	return save_webp_buffer_func(Ref<Image>((Image*)this), p_lossy, p_quality);
}

int64_t Image::get_image_data_size(int p_width, int p_height, Format p_format, bool p_mipmaps)
{
	int mm;
	return _get_dst_image_size(p_width, p_height, p_format, mm, p_mipmaps ? -1 : 0);
}

int Image::get_image_required_mipmaps(int p_width, int p_height, Format p_format)
{
	int mm;
	_get_dst_image_size(p_width, p_height, p_format, mm, -1);
	return mm;
}

Size2i Image::get_image_mipmap_size(int p_width, int p_height, Format p_format, int p_mipmap)
{
	int mm;
	Size2i ret;
	_get_dst_image_size(p_width, p_height, p_format, mm, p_mipmap, &ret.x, &ret.y);
	return ret;
}

int64_t Image::get_image_mipmap_offset(int p_width, int p_height, Format p_format, int p_mipmap)
{
	if (p_mipmap <= 0) {
		return 0;
	}
	int mm;
	return _get_dst_image_size(p_width, p_height, p_format, mm, p_mipmap - 1);
}

int64_t Image::get_image_mipmap_offset_and_dimensions(
	int p_width, int p_height, Format p_format, int p_mipmap, int& r_w, int& r_h)
{
	if (p_mipmap <= 0) {
		r_w = p_width;
		r_h = p_height;
		return 0;
	}
	int mm;
	return _get_dst_image_size(p_width, p_height, p_format, mm, p_mipmap - 1, &r_w, &r_h);
}

bool Image::is_compressed() const { return is_format_compressed(format); }

bool Image::is_format_compressed(Format p_format)
{
	return (p_format >= FORMAT_DXT1 && p_format <= FORMAT_ASTC_8x8_HDR) ||
		   (p_format >= FORMAT_ASTC_6x6 && p_format <= FORMAT_ASTC_6x6_HDR);
}

Error Image::decompress()
{
	if (((format >= FORMAT_DXT1 && format <= FORMAT_RGTC_RG) || (format == FORMAT_DXT5_RA_AS_RG)) &&
		_image_decompress_bc) {
		_image_decompress_bc(this);
	}
	else if (format >= FORMAT_BPTC_RGBA && format <= FORMAT_BPTC_RGBFU &&
			   _image_decompress_bptc) {
		_image_decompress_bptc(this);
	}
	else if (format == FORMAT_ETC && _image_decompress_etc1) {
		_image_decompress_etc1(this);
	}
	else if (format >= FORMAT_ETC2_R11 && format <= FORMAT_ETC2_RA_AS_RG &&
			   _image_decompress_etc2) {
		_image_decompress_etc2(this);
	}
	else if (((format >= FORMAT_ASTC_4x4 && format <= FORMAT_ASTC_8x8_HDR) ||
				   (format >= FORMAT_ASTC_6x6 && format <= FORMAT_ASTC_6x6_HDR)) &&
			   _image_decompress_astc) {
		_image_decompress_astc(this);
	}
	else {
		return ERR_UNAVAILABLE;
	}
	return OK;
}

bool Image::can_decompress(const String& p_format_tag)
{
	if (p_format_tag == "astc") {
		return _image_decompress_astc != nullptr;
	}
	else if (p_format_tag == "bptc") {
		return _image_decompress_bptc != nullptr;
	}
	else if (p_format_tag == "etc2") {
		return _image_decompress_etc2 != nullptr;
	}
	else if (p_format_tag == "s3tc") {
		return _image_decompress_bc != nullptr;
	}
	return false;
}

Error Image::compress(CompressMode p_mode, CompressSource p_source, CompressProfile p_profile)
{
	ERR_FAIL_INDEX_V_MSG(p_mode, COMPRESS_MAX, ERR_INVALID_PARAMETER, "Invalid compress mode.");
	ERR_FAIL_INDEX_V_MSG(
		p_source, COMPRESS_SOURCE_MAX, ERR_INVALID_PARAMETER, "Invalid compress source.");
	return compress_from_channels(p_mode, detect_used_channels(p_source), p_profile);
}

Error Image::compress_from_channels(
	CompressMode p_mode, UsedChannels p_channels, CompressProfile p_profile)
{
	return _compress_from_channels(p_mode, p_channels, p_profile, BPTC_DETECT);
}

Error Image::_compress_from_channels(CompressMode p_mode, UsedChannels p_channels,
	CompressProfile p_profile, BPTCFormat p_bptc_format)
{
	ERR_FAIL_COND_V(data.is_empty(), ERR_INVALID_DATA);

	// RenderingDevice only.
	switch (p_mode) {
	case COMPRESS_S3TC: {
		ERR_FAIL_NULL_V(_image_compress_bc_func, ERR_UNAVAILABLE);
		_image_compress_bc_func(this, p_channels);
	} break;
	case COMPRESS_ETC: {
		ERR_FAIL_NULL_V(_image_compress_etc1_func, ERR_UNAVAILABLE);
		_image_compress_etc1_func(this);
	} break;
	case COMPRESS_ETC2: {
		ERR_FAIL_NULL_V(_image_compress_etc2_func, ERR_UNAVAILABLE);
		_image_compress_etc2_func(this, p_channels);
	} break;
	case COMPRESS_BPTC: {
		ERR_FAIL_NULL_V(_image_compress_bptc_func, ERR_UNAVAILABLE);
		_image_compress_bptc_func(this, p_channels, p_bptc_format);
	} break;
	case COMPRESS_ASTC: {
		ERR_FAIL_NULL_V(_image_compress_astc_func, ERR_UNAVAILABLE);
		_image_compress_astc_func(this, p_channels, p_profile);
	} break;
	case COMPRESS_MAX: {
		ERR_FAIL_V(ERR_INVALID_PARAMETER);
	} break;
	}

	return OK;
}

Image::Image(const char** p_xpm)
{
	width = 0;
	height = 0;
	mipmaps = false;
	format = FORMAT_L8;

	initialize_data(p_xpm);
}

Image::Image(int p_width, int p_height, bool p_use_mipmaps, Format p_format)
{
	width = 0;
	height = 0;
	mipmaps = p_use_mipmaps;
	format = FORMAT_L8;

	initialize_data(p_width, p_height, p_use_mipmaps, p_format);
}

Image::Image(
	int p_width, int p_height, bool p_mipmaps, Format p_format, const Vector<uint8_t>& p_data)
{
	width = 0;
	height = 0;
	mipmaps = p_mipmaps;
	format = FORMAT_L8;

	initialize_data(p_width, p_height, p_mipmaps, p_format, p_data);
}

Rect2i Image::get_used_rect() const
{
	if (format != FORMAT_LA8 && format != FORMAT_RGBA8 && format != FORMAT_RGBAF &&
		format != FORMAT_RGBAH && format != FORMAT_RGBA4444 && format != FORMAT_RGB565) {
		return Rect2i(0, 0, width, height);
	}

	int len = data.size();

	if (len == 0) {
		return Rect2i();
	}

	int minx = 0xFFFFFF, miny = 0xFFFFFFF;
	int maxx = -1, maxy = -1;
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			if (!(get_pixel(i, j).a > 0)) {
				continue;
			}
			if (i > maxx) {
				maxx = i;
			}
			if (j > maxy) {
				maxy = j;
			}
			if (i < minx) {
				minx = i;
			}
			if (j < miny) {
				miny = j;
			}
		}
	}

	if (maxx == -1) {
		return Rect2i();
	}
	else {
		return Rect2i(minx, miny, maxx - minx + 1, maxy - miny + 1);
	}
}

Ref<Image> Image::get_region(const Rect2i& p_region) const
{
	Ref<Image> img = memnew(Image(p_region.size.x, p_region.size.y, mipmaps, format));
	img->blit_rect(Ref<Image>((Image*)this), p_region, Point2i(0, 0));
	return img;
}

void Image::_get_clipped_src_and_dest_rects(const Ref<Image>& p_src, const Rect2i& p_src_rect,
	const Point2i& p_dest, Rect2i& r_clipped_src_rect, Rect2i& r_clipped_dest_rect) const
{
	r_clipped_dest_rect.position = p_dest;
	r_clipped_src_rect = p_src_rect;

	if (r_clipped_src_rect.position.x < 0) {
		r_clipped_dest_rect.position.x -= r_clipped_src_rect.position.x;
		r_clipped_src_rect.size.x += r_clipped_src_rect.position.x;
		r_clipped_src_rect.position.x = 0;
	}
	if (r_clipped_src_rect.position.y < 0) {
		r_clipped_dest_rect.position.y -= r_clipped_src_rect.position.y;
		r_clipped_src_rect.size.y += r_clipped_src_rect.position.y;
		r_clipped_src_rect.position.y = 0;
	}

	if (r_clipped_dest_rect.position.x < 0) {
		r_clipped_src_rect.position.x -= r_clipped_dest_rect.position.x;
		r_clipped_src_rect.size.x += r_clipped_dest_rect.position.x;
		r_clipped_dest_rect.position.x = 0;
	}
	if (r_clipped_dest_rect.position.y < 0) {
		r_clipped_src_rect.position.y -= r_clipped_dest_rect.position.y;
		r_clipped_src_rect.size.y += r_clipped_dest_rect.position.y;
		r_clipped_dest_rect.position.y = 0;
	}

	r_clipped_src_rect.size.x =
		MAX(0, MIN(r_clipped_src_rect.size.x, MIN(p_src->width - r_clipped_src_rect.position.x,
												  width - r_clipped_dest_rect.position.x)));
	r_clipped_src_rect.size.y =
		MAX(0, MIN(r_clipped_src_rect.size.y, MIN(p_src->height - r_clipped_src_rect.position.y,
												  height - r_clipped_dest_rect.position.y)));

	r_clipped_dest_rect.size.x = r_clipped_src_rect.size.x;
	r_clipped_dest_rect.size.y = r_clipped_src_rect.size.y;
}

void Image::blit_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(p_src.is_null(), "Cannot blit_rect an image: invalid source Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(format != p_src->format);
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot blit_rect in compressed image formats.");

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	uint8_t* wp = data.ptrw();
	uint8_t* dst_data_ptr = wp;

	const uint8_t* rp = p_src->data.ptr();
	const uint8_t* src_data_ptr = rp;

	int pixel_size = get_format_pixel_size(format);

	// If the rect width is equivalent for both src and dst and the x offset is 0, we can blit in a
	// single memcpy. Else, we do a per-line copy.
	if ((dest_rect.size.x == p_src->width) && (p_src->width == width) &&
		(dest_rect.position.x == 0) && (src_rect.position.x == 0)) {
		const uint8_t* src = &src_data_ptr[(src_rect.position.y * p_src->width) * pixel_size];
		uint8_t* dst = &dst_data_ptr[(dest_rect.position.y * width) * pixel_size];
		memcpy(dst, src, width * dest_rect.size.y * pixel_size);
	}
	else {
		for (int i = 0; i < dest_rect.size.y; i++) {
			const uint8_t* src =
				&src_data_ptr[((src_rect.position.y + i) * p_src->width + src_rect.position.x) *
							  pixel_size];
			uint8_t* dst =
				&dst_data_ptr[((dest_rect.position.y + i) * width + dest_rect.position.x) *
							  pixel_size];
			memcpy(dst, src, pixel_size * dest_rect.size.x);
		}
	}
}

void Image::blit_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask,
	const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(
		p_src.is_null(), "Cannot blit_rect_mask an image: invalid source Image object.");
	ERR_FAIL_COND_MSG(
		p_mask.is_null(), "Cannot blit_rect_mask an image: invalid mask Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	int maskdsize = p_mask->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(maskdsize == 0);
	ERR_FAIL_COND_MSG(
		p_src->width != p_mask->width, "Source image width is different from mask width.");
	ERR_FAIL_COND_MSG(
		p_src->height != p_mask->height, "Source image height is different from mask height.");
	ERR_FAIL_COND(format != p_src->format);

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	uint8_t* wp = data.ptrw();
	uint8_t* dst_data_ptr = wp;

	const uint8_t* rp = p_src->data.ptr();
	const uint8_t* src_data_ptr = rp;

	int pixel_size = get_format_pixel_size(format);

	Ref<Image> msk = p_mask;

	for (int i = 0; i < dest_rect.size.y; i++) {
		for (int j = 0; j < dest_rect.size.x; j++) {
			int src_x = src_rect.position.x + j;
			int src_y = src_rect.position.y + i;

			if (msk->get_pixel(src_x, src_y).a != 0) {
				int dst_x = dest_rect.position.x + j;
				int dst_y = dest_rect.position.y + i;

				const uint8_t* src = &src_data_ptr[(src_y * p_src->width + src_x) * pixel_size];
				uint8_t* dst = &dst_data_ptr[(dst_y * width + dst_x) * pixel_size];

				for (int k = 0; k < pixel_size; k++) {
					dst[k] = src[k];
				}
			}
		}
	}
}

void Image::blend_rect(const Ref<Image>& p_src, const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(p_src.is_null(), "Cannot blend_rect an image: invalid source Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(format != p_src->format);

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	Ref<Image> img = p_src;

	for (int i = 0; i < dest_rect.size.y; i++) {
		for (int j = 0; j < dest_rect.size.x; j++) {
			int src_x = src_rect.position.x + j;
			int src_y = src_rect.position.y + i;

			int dst_x = dest_rect.position.x + j;
			int dst_y = dest_rect.position.y + i;

			Color sc = img->get_pixel(src_x, src_y);
			if (sc.a != 0) {
				Color dc = get_pixel(dst_x, dst_y);
				dc = dc.blend(sc);
				set_pixel(dst_x, dst_y, dc);
			}
		}
	}
}

void Image::blend_rect_mask(const Ref<Image>& p_src, const Ref<Image>& p_mask,
	const Rect2i& p_src_rect, const Point2i& p_dest)
{
	ERR_FAIL_COND_MSG(
		p_src.is_null(), "Cannot blend_rect_mask an image: invalid source Image object.");
	ERR_FAIL_COND_MSG(
		p_mask.is_null(), "Cannot blend_rect_mask an image: invalid mask Image object.");
	int dsize = data.size();
	int srcdsize = p_src->data.size();
	int maskdsize = p_mask->data.size();
	ERR_FAIL_COND(dsize == 0);
	ERR_FAIL_COND(srcdsize == 0);
	ERR_FAIL_COND(maskdsize == 0);
	ERR_FAIL_COND_MSG(
		p_src->width != p_mask->width, "Source image width is different from mask width.");
	ERR_FAIL_COND_MSG(
		p_src->height != p_mask->height, "Source image height is different from mask height.");
	ERR_FAIL_COND(format != p_src->format);

	Rect2i src_rect;
	Rect2i dest_rect;
	_get_clipped_src_and_dest_rects(p_src, p_src_rect, p_dest, src_rect, dest_rect);
	if (!src_rect.has_area() || !dest_rect.has_area()) {
		return;
	}

	Ref<Image> img = p_src;
	Ref<Image> msk = p_mask;

	for (int i = 0; i < dest_rect.size.y; i++) {
		for (int j = 0; j < dest_rect.size.x; j++) {
			int src_x = src_rect.position.x + j;
			int src_y = src_rect.position.y + i;

			// If the mask's pixel is transparent then we skip it
			// Color c = msk->get_pixel(src_x, src_y);
			// if (c.a == 0) continue;
			if (msk->get_pixel(src_x, src_y).a != 0) {
				int dst_x = dest_rect.position.x + j;
				int dst_y = dest_rect.position.y + i;

				Color sc = img->get_pixel(src_x, src_y);
				if (sc.a != 0) {
					Color dc = get_pixel(dst_x, dst_y);
					dc = dc.blend(sc);
					set_pixel(dst_x, dst_y, dc);
				}
			}
		}
	}
}

// Repeats `p_pixel` `p_count` times in consecutive memory.
// Results in the original pixel and `p_count - 1` subsequent copies of it.
void Image::_repeat_pixel_over_subsequent_memory(uint8_t* p_pixel, int p_pixel_size, int p_count)
{
	int offset = 1;
	for (int stride = 1; offset + stride <= p_count; stride *= 2) {
		memcpy(p_pixel + offset * p_pixel_size, p_pixel, stride * p_pixel_size);
		offset += stride;
	}
	if (offset < p_count) {
		memcpy(p_pixel + offset * p_pixel_size, p_pixel, (p_count - offset) * p_pixel_size);
	}
}

void Image::fill(const Color& p_color)
{
	if (data.is_empty()) {
		return;
	}
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot fill in compressed image formats.");

	uint8_t* dst_data_ptr = data.ptrw();
	int pixel_size = get_format_pixel_size(format);
	int64_t pixel_count = data.size() / pixel_size;

	// Put first pixel with the format-aware API.
	_set_color_at_ofs(dst_data_ptr, 0, p_color);

	_repeat_pixel_over_subsequent_memory(dst_data_ptr, pixel_size, pixel_count);
}

void Image::fill_rect(const Rect2i& p_rect, const Color& p_color)
{
	if (data.is_empty()) {
		return;
	}
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot fill rect in compressed image formats.");

	Rect2i r = Rect2i(0, 0, width, height).intersection(p_rect.abs());
	if (!r.has_area()) {
		return;
	}

	uint8_t* dst_data_ptr = data.ptrw();

	int pixel_size = get_format_pixel_size(format);

	// Put first pixel with the format-aware API.
	uint8_t* rect_first_pixel_ptr =
		&dst_data_ptr[(r.position.y * width + r.position.x) * pixel_size];
	_set_color_at_ofs(rect_first_pixel_ptr, 0, p_color);

	if (r.size.x == width) {
		// No need to fill rows separately.
		_repeat_pixel_over_subsequent_memory(rect_first_pixel_ptr, pixel_size, width * r.size.y);
	}
	else {
		_repeat_pixel_over_subsequent_memory(rect_first_pixel_ptr, pixel_size, r.size.x);
		for (int y = 1; y < r.size.y; y++) {
			memcpy(rect_first_pixel_ptr + y * width * pixel_size, rect_first_pixel_ptr,
				r.size.x * pixel_size);
		}
	}
}

Color Image::get_pixelv(const Point2i& p_point) const { return get_pixel(p_point.x, p_point.y); }

void Image::_copy_internals_from(const Image& p_image)
{
	format = p_image.format;
	width = p_image.width;
	height = p_image.height;
	mipmaps = p_image.mipmaps;
	data = p_image.data;
}

_FORCE_INLINE_ Color color_from_rgba4444(uint16_t p_col)
{
	float r = ((p_col >> 12) & 0xF) / 15.0;
	float g = ((p_col >> 8) & 0xF) / 15.0;
	float b = ((p_col >> 4) & 0xF) / 15.0;
	float a = (p_col & 0xF) / 15.0;
	return Color(r, g, b, a);
}

_FORCE_INLINE_ uint16_t color_to_rgba4444(Color p_col)
{
	uint16_t rgba = 0;

	rgba = uint16_t(CLAMP(p_col.r * 15.0, 0, 15)) << 12;
	rgba |= uint16_t(CLAMP(p_col.g * 15.0, 0, 15)) << 8;
	rgba |= uint16_t(CLAMP(p_col.b * 15.0, 0, 15)) << 4;
	rgba |= uint16_t(CLAMP(p_col.a * 15.0, 0, 15));

	return rgba;
}

_FORCE_INLINE_ Color color_from_rgb565(uint16_t p_col)
{
	float r = ((p_col >> 11) & 0x1F) / 31.0;
	float g = ((p_col >> 5) & 0x3F) / 63.0;
	float b = (p_col & 0x1F) / 31.0;
	return Color(r, g, b, 1.0);
}

_FORCE_INLINE_ uint16_t color_to_rgb565(Color p_col)
{
	uint16_t rgba = 0;

	rgba = uint16_t(CLAMP(p_col.r * 31.0, 0, 31)) << 11;
	rgba |= uint16_t(CLAMP(p_col.g * 63.0, 0, 63)) << 5;
	rgba |= uint16_t(CLAMP(p_col.b * 31.0, 0, 31));

	return rgba;
}

Color Image::_get_color_at_ofs(const uint8_t* p_ptr, uint32_t p_ofs) const
{
	switch (format) {
	case FORMAT_L8: {
		float l = p_ptr[p_ofs] / 255.0;
		return Color(l, l, l, 1);
	}
	case FORMAT_LA8: {
		float l = p_ptr[p_ofs * 2 + 0] / 255.0;
		float a = p_ptr[p_ofs * 2 + 1] / 255.0;
		return Color(l, l, l, a);
	}
	case FORMAT_R8: {
		float r = p_ptr[p_ofs] / 255.0;
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RG8: {
		float r = p_ptr[p_ofs * 2 + 0] / 255.0;
		float g = p_ptr[p_ofs * 2 + 1] / 255.0;
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGB8: {
		float r = p_ptr[p_ofs * 3 + 0] / 255.0;
		float g = p_ptr[p_ofs * 3 + 1] / 255.0;
		float b = p_ptr[p_ofs * 3 + 2] / 255.0;
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBA8: {
		float r = p_ptr[p_ofs * 4 + 0] / 255.0;
		float g = p_ptr[p_ofs * 4 + 1] / 255.0;
		float b = p_ptr[p_ofs * 4 + 2] / 255.0;
		float a = p_ptr[p_ofs * 4 + 3] / 255.0;
		return Color(r, g, b, a);
	}
	case FORMAT_RGBA4444: {
		return color_from_rgba4444(((uint16_t*)p_ptr)[p_ofs]);
	}
	case FORMAT_RGB565: {
		return color_from_rgb565(((uint16_t*)p_ptr)[p_ofs]);
	}
	case FORMAT_RF: {
		float r = ((float*)p_ptr)[p_ofs];
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RGF: {
		float r = ((float*)p_ptr)[p_ofs * 2 + 0];
		float g = ((float*)p_ptr)[p_ofs * 2 + 1];
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGBF: {
		float r = ((float*)p_ptr)[p_ofs * 3 + 0];
		float g = ((float*)p_ptr)[p_ofs * 3 + 1];
		float b = ((float*)p_ptr)[p_ofs * 3 + 2];
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBAF: {
		float r = ((float*)p_ptr)[p_ofs * 4 + 0];
		float g = ((float*)p_ptr)[p_ofs * 4 + 1];
		float b = ((float*)p_ptr)[p_ofs * 4 + 2];
		float a = ((float*)p_ptr)[p_ofs * 4 + 3];
		return Color(r, g, b, a);
	}
	case FORMAT_RH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs];
		return Color(Math::half_to_float(r), 0, 0, 1);
	}
	case FORMAT_RGH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 2 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 2 + 1];
		return Color(Math::half_to_float(r), Math::half_to_float(g), 0, 1);
	}
	case FORMAT_RGBH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 3 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 3 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 3 + 2];
		return Color(Math::half_to_float(r), Math::half_to_float(g), Math::half_to_float(b), 1);
	}
	case FORMAT_RGBAH: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 4 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 4 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 4 + 2];
		uint16_t a = ((uint16_t*)p_ptr)[p_ofs * 4 + 3];
		return Color(Math::half_to_float(r), Math::half_to_float(g), Math::half_to_float(b),
			Math::half_to_float(a));
	}
	case FORMAT_RGBE9995: {
		return Color::from_rgbe9995(((uint32_t*)p_ptr)[p_ofs]);
	}
	case FORMAT_R16: {
		float r = ((uint16_t*)p_ptr)[p_ofs] / 65535.0f;
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RG16: {
		float r = ((uint16_t*)p_ptr)[p_ofs * 2 + 0] / 65535.0f;
		float g = ((uint16_t*)p_ptr)[p_ofs * 2 + 1] / 65535.0f;
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGB16: {
		float r = ((uint16_t*)p_ptr)[p_ofs * 3 + 0] / 65535.0f;
		float g = ((uint16_t*)p_ptr)[p_ofs * 3 + 1] / 65535.0f;
		float b = ((uint16_t*)p_ptr)[p_ofs * 3 + 2] / 65535.0f;
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBA16: {
		float r = ((uint16_t*)p_ptr)[p_ofs * 4 + 0] / 65535.0f;
		float g = ((uint16_t*)p_ptr)[p_ofs * 4 + 1] / 65535.0f;
		float b = ((uint16_t*)p_ptr)[p_ofs * 4 + 2] / 65535.0f;
		float a = ((uint16_t*)p_ptr)[p_ofs * 4 + 3] / 65535.0f;
		return Color(r, g, b, a);
	}
	case FORMAT_R16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs];
		return Color(r, 0, 0, 1);
	}
	case FORMAT_RG16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 2 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 2 + 1];
		return Color(r, g, 0, 1);
	}
	case FORMAT_RGB16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 3 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 3 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 3 + 2];
		return Color(r, g, b, 1);
	}
	case FORMAT_RGBA16I: {
		uint16_t r = ((uint16_t*)p_ptr)[p_ofs * 4 + 0];
		uint16_t g = ((uint16_t*)p_ptr)[p_ofs * 4 + 1];
		uint16_t b = ((uint16_t*)p_ptr)[p_ofs * 4 + 2];
		uint16_t a = ((uint16_t*)p_ptr)[p_ofs * 4 + 3];
		return Color(r, g, b, a);
	}

	default: {
		ERR_FAIL_V_MSG(Color(), "Can't get_pixel() on compressed image, sorry.");
	}
	}
}

void Image::_set_color_at_ofs(uint8_t* r_ptr, uint32_t p_ofs, const Color& p_color)
{
	switch (format) {
	case FORMAT_L8: {
		r_ptr[p_ofs] = uint8_t(CLAMP(p_color.get_v() * 255.0, 0, 255));
	} break;
	case FORMAT_LA8: {
		r_ptr[p_ofs * 2 + 0] = uint8_t(CLAMP(p_color.get_v() * 255.0, 0, 255));
		r_ptr[p_ofs * 2 + 1] = uint8_t(CLAMP(p_color.a * 255.0, 0, 255));
	} break;
	case FORMAT_R8: {
		r_ptr[p_ofs] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
	} break;
	case FORMAT_RG8: {
		r_ptr[p_ofs * 2 + 0] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
		r_ptr[p_ofs * 2 + 1] = uint8_t(CLAMP(p_color.g * 255.0, 0, 255));
	} break;
	case FORMAT_RGB8: {
		r_ptr[p_ofs * 3 + 0] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
		r_ptr[p_ofs * 3 + 1] = uint8_t(CLAMP(p_color.g * 255.0, 0, 255));
		r_ptr[p_ofs * 3 + 2] = uint8_t(CLAMP(p_color.b * 255.0, 0, 255));
	} break;
	case FORMAT_RGBA8: {
		r_ptr[p_ofs * 4 + 0] = uint8_t(CLAMP(p_color.r * 255.0, 0, 255));
		r_ptr[p_ofs * 4 + 1] = uint8_t(CLAMP(p_color.g * 255.0, 0, 255));
		r_ptr[p_ofs * 4 + 2] = uint8_t(CLAMP(p_color.b * 255.0, 0, 255));
		r_ptr[p_ofs * 4 + 3] = uint8_t(CLAMP(p_color.a * 255.0, 0, 255));
	} break;
	case FORMAT_RGBA4444: {
		((uint16_t*)r_ptr)[p_ofs] = color_to_rgba4444(p_color);
	} break;
	case FORMAT_RGB565: {
		((uint16_t*)r_ptr)[p_ofs] = color_to_rgb565(p_color);
	} break;
	case FORMAT_RF: {
		((float*)r_ptr)[p_ofs] = p_color.r;
	} break;
	case FORMAT_RGF: {
		((float*)r_ptr)[p_ofs * 2 + 0] = p_color.r;
		((float*)r_ptr)[p_ofs * 2 + 1] = p_color.g;
	} break;
	case FORMAT_RGBF: {
		((float*)r_ptr)[p_ofs * 3 + 0] = p_color.r;
		((float*)r_ptr)[p_ofs * 3 + 1] = p_color.g;
		((float*)r_ptr)[p_ofs * 3 + 2] = p_color.b;
	} break;
	case FORMAT_RGBAF: {
		((float*)r_ptr)[p_ofs * 4 + 0] = p_color.r;
		((float*)r_ptr)[p_ofs * 4 + 1] = p_color.g;
		((float*)r_ptr)[p_ofs * 4 + 2] = p_color.b;
		((float*)r_ptr)[p_ofs * 4 + 3] = p_color.a;
	} break;
	case FORMAT_RH: {
		((uint16_t*)r_ptr)[p_ofs] = Math::make_half_float(p_color.r);
	} break;
	case FORMAT_RGH: {
		((uint16_t*)r_ptr)[p_ofs * 2 + 0] = Math::make_half_float(p_color.r);
		((uint16_t*)r_ptr)[p_ofs * 2 + 1] = Math::make_half_float(p_color.g);
	} break;
	case FORMAT_RGBH: {
		((uint16_t*)r_ptr)[p_ofs * 3 + 0] = Math::make_half_float(p_color.r);
		((uint16_t*)r_ptr)[p_ofs * 3 + 1] = Math::make_half_float(p_color.g);
		((uint16_t*)r_ptr)[p_ofs * 3 + 2] = Math::make_half_float(p_color.b);
	} break;
	case FORMAT_RGBAH: {
		((uint16_t*)r_ptr)[p_ofs * 4 + 0] = Math::make_half_float(p_color.r);
		((uint16_t*)r_ptr)[p_ofs * 4 + 1] = Math::make_half_float(p_color.g);
		((uint16_t*)r_ptr)[p_ofs * 4 + 2] = Math::make_half_float(p_color.b);
		((uint16_t*)r_ptr)[p_ofs * 4 + 3] = Math::make_half_float(p_color.a);
	} break;
	case FORMAT_RGBE9995: {
		((uint32_t*)r_ptr)[p_ofs] = p_color.to_rgbe9995();
	} break;
	case FORMAT_R16: {
		((uint16_t*)r_ptr)[p_ofs] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
	} break;
	case FORMAT_RG16: {
		((uint16_t*)r_ptr)[p_ofs * 2 + 0] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 2 + 1] = uint16_t(CLAMP(p_color.g * 65535.0, 0, 65535));
	} break;
	case FORMAT_RGB16: {
		((uint16_t*)r_ptr)[p_ofs * 3 + 0] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 1] = uint16_t(CLAMP(p_color.g * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 2] = uint16_t(CLAMP(p_color.b * 65535.0, 0, 65535));
	} break;
	case FORMAT_RGBA16: {
		((uint16_t*)r_ptr)[p_ofs * 4 + 0] = uint16_t(CLAMP(p_color.r * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 1] = uint16_t(CLAMP(p_color.g * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 2] = uint16_t(CLAMP(p_color.b * 65535.0, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 3] = uint16_t(CLAMP(p_color.a * 65535.0, 0, 65535));
	} break;
	case FORMAT_R16I: {
		((uint16_t*)r_ptr)[p_ofs] = uint16_t(CLAMP(p_color.r, 0, 65535));
	} break;
	case FORMAT_RG16I: {
		((uint16_t*)r_ptr)[p_ofs * 2 + 0] = uint16_t(CLAMP(p_color.r, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 2 + 1] = uint16_t(CLAMP(p_color.g, 0, 65535));
	} break;
	case FORMAT_RGB16I: {
		((uint16_t*)r_ptr)[p_ofs * 3 + 0] = uint16_t(CLAMP(p_color.r, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 1] = uint16_t(CLAMP(p_color.g, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 3 + 2] = uint16_t(CLAMP(p_color.b, 0, 65535));
	} break;
	case FORMAT_RGBA16I: {
		((uint16_t*)r_ptr)[p_ofs * 4 + 0] = uint16_t(CLAMP(p_color.r, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 1] = uint16_t(CLAMP(p_color.g, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 2] = uint16_t(CLAMP(p_color.b, 0, 65535));
		((uint16_t*)r_ptr)[p_ofs * 4 + 3] = uint16_t(CLAMP(p_color.a, 0, 65535));
	} break;

	default: {
		ERR_FAIL_MSG("Can't set_pixel() on compressed image, sorry.");
	}
	}
}

Color Image::get_pixel(int p_x, int p_y) const
{
#ifdef DEBUG_ENABLED
	ERR_FAIL_INDEX_V(p_x, width, Color());
	ERR_FAIL_INDEX_V(p_y, height, Color());
#endif

	uint32_t ofs = p_y * width + p_x;
	return _get_color_at_ofs(data.ptr(), ofs);
}

void Image::set_pixelv(const Point2i& p_point, const Color& p_color)
{
	set_pixel(p_point.x, p_point.y, p_color);
}

void Image::set_pixel(int p_x, int p_y, const Color& p_color)
{
#ifdef DEBUG_ENABLED
	ERR_FAIL_INDEX(p_x, width);
	ERR_FAIL_INDEX(p_y, height);
#endif

	uint32_t ofs = p_y * width + p_x;
	_set_color_at_ofs(data.ptrw(), ofs, p_color);
}

const uint8_t* Image::ptr() const { return data.ptr(); }

uint8_t* Image::ptrw() { return data.ptrw(); }

int64_t Image::get_data_size() const { return data.size(); }

void Image::adjust_bcs(float p_brightness, float p_contrast, float p_saturation)
{
	ERR_FAIL_COND_MSG(is_compressed(), "Cannot adjust_bcs in compressed image formats.");

	uint8_t* w = data.ptrw();
	uint32_t pixel_size = get_format_pixel_size(format);
	uint32_t pixel_count = data.size() / pixel_size;

	for (uint32_t i = 0; i < pixel_count; i++) {
		Color c = _get_color_at_ofs(w, i);
		Vector3 rgb(c.r, c.g, c.b);

		rgb *= p_brightness;
		rgb = Vector3(0.5, 0.5, 0.5).lerp(rgb, p_contrast);
		float center = (rgb.x + rgb.y + rgb.z) / 3.0;
		rgb = Vector3(center, center, center).lerp(rgb, p_saturation);
		c.r = rgb.x;
		c.g = rgb.y;
		c.b = rgb.z;
		_set_color_at_ofs(w, i, c);
	}
}

Image::UsedChannels Image::detect_used_channels(CompressSource p_source) const
{
	ERR_FAIL_COND_V(data.is_empty(), USED_CHANNELS_RGBA);
	ERR_FAIL_COND_V(is_compressed(), USED_CHANNELS_RGBA);

	if (p_source == COMPRESS_SOURCE_NORMAL) {
		return USED_CHANNELS_RG; // Normal maps only use RG channels.
	}

	if (format == FORMAT_L8) {
		return USED_CHANNELS_L; // Grayscale only cannot have any channel less.
	}
	else if (format == FORMAT_R8 || format == FORMAT_RH || format == FORMAT_RF ||
			   format == FORMAT_R16 || format == FORMAT_R16I) {
		return USED_CHANNELS_R; // Red only cannot have any channel less.
	}

	const bool supports_alpha = format == FORMAT_RGBA8 || format == FORMAT_RGBA4444 ||
								format == FORMAT_RGBAH || format == FORMAT_RGBAF ||
								format == FORMAT_RGBA16 || format == FORMAT_RGBA16I;
	bool r = false, g = false, b = false, a = false, c = false;

	const uint8_t* data_ptr = data.ptr();
	const uint32_t data_total = width * height;

	for (uint32_t i = 0; i < data_total; i++) {
		Color col = _get_color_at_ofs(data_ptr, i);

		if (!r && col.r > 0.001) {
			r = true;
		}
		if (!g && col.g > 0.001) {
			g = true;
		}
		if (!b && col.b > 0.001) {
			b = true;
		}
		if (!a && col.a < 0.999) {
			a = true;
		}

		if (col.r != col.b || col.r != col.g || col.b != col.g) {
			c = true; // The image is not grayscale.
		}

		if (r && g && b && c) {
			// All channels are used, no need to continue.
			if (!supports_alpha) {
				break;
			}
			else if (a) {
				break;
			}
		}
	}

	UsedChannels used_channels;

	if (!c) {
		// Uniform RGB (grayscale).
		if (a) {
			used_channels = USED_CHANNELS_LA;
		}
		else {
			used_channels = USED_CHANNELS_L;
		}
	}
	else {
		// Colored image.
		if (a) {
			used_channels = USED_CHANNELS_RGBA;
		}
		else if (b) {
			used_channels = USED_CHANNELS_RGB;
		}
		else if (g) {
			used_channels = USED_CHANNELS_RG;
		}
		else {
			used_channels = USED_CHANNELS_R;
		}
	}

	if (p_source == COMPRESS_SOURCE_SRGB &&
		(used_channels == USED_CHANNELS_R || used_channels == USED_CHANNELS_RG)) {
		used_channels = USED_CHANNELS_RGB; // R and RG do not support SRGB.
	}

	return used_channels;
}

void Image::optimize_channels()
{
	switch (detect_used_channels()) {
	case USED_CHANNELS_L:
		convert(FORMAT_L8);
		break;
	case USED_CHANNELS_LA:
		convert(FORMAT_LA8);
		break;
	case USED_CHANNELS_R:
		convert(FORMAT_R8);
		break;
	case USED_CHANNELS_RG:
		convert(FORMAT_RG8);
		break;
	case USED_CHANNELS_RGB:
		convert(FORMAT_RGB8);
		break;
	case USED_CHANNELS_RGBA:
		convert(FORMAT_RGBA8);
		break;
	}
}

void Image::normal_map_to_xy()
{
	convert(Image::FORMAT_RGBA8);

	{
		int len = data.size() / 4;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i << 2) + 3] = data_ptr[(i << 2) + 0]; // x to w
			data_ptr[(i << 2) + 0] = data_ptr[(i << 2) + 1]; // y to xz
			data_ptr[(i << 2) + 2] = data_ptr[(i << 2) + 1];
		}
	}

	convert(Image::FORMAT_LA8);
}

Ref<Image> Image::rgbe_to_srgb()
{
	if (data.is_empty()) {
		return Ref<Image>();
	}

	ERR_FAIL_COND_V(format != FORMAT_RGBE9995, Ref<Image>());

	Ref<Image> new_image = create_empty(width, height, false, Image::FORMAT_RGB8);

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			new_image->set_pixel(col, row, get_pixel(col, row).linear_to_srgb());
		}
	}

	if (has_mipmaps()) {
		new_image->generate_mipmaps();
	}

	return new_image;
}

Ref<Image> Image::get_image_from_mipmap(int p_mipmap) const
{
	int64_t ofs, size;
	int w, h;
	get_mipmap_offset_size_and_dimensions(p_mipmap, ofs, size, w, h);

	Vector<uint8_t> new_data;
	new_data.resize(size);

	{
		uint8_t* wr = new_data.ptrw();
		const uint8_t* rd = data.ptr();
		memcpy(wr, rd + ofs, size);
	}

	Ref<Image> image;
	image.instantiate();
	image->width = w;
	image->height = h;
	image->format = format;
	image->data = new_data;

	image->mipmaps = false;
	return image;
}

void Image::bump_map_to_normal_map(float p_bump_scale)
{
	ERR_FAIL_COND(is_compressed());
	clear_mipmaps();
	convert(Image::FORMAT_RF);

	Vector<uint8_t> result_image; // rgba output
	result_image.resize(width * height * 4);

	{
		const uint8_t* rp = data.ptr();
		uint8_t* wp = result_image.ptrw();

		ERR_FAIL_NULL(rp);

		unsigned char* write_ptr = wp;
		float* read_ptr = (float*)rp;

		for (int ty = 0; ty < height; ty++) {
			int py = ty + 1;
			if (py >= height) {
				py -= height;
			}

			for (int tx = 0; tx < width; tx++) {
				int px = tx + 1;
				if (px >= width) {
					px -= width;
				}
				float here = read_ptr[ty * width + tx];
				float to_right = read_ptr[ty * width + px];
				float above = read_ptr[py * width + tx];
				Vector3 up = Vector3(0, 1, (here - above) * p_bump_scale);
				Vector3 across = Vector3(1, 0, (to_right - here) * p_bump_scale);

				Vector3 normal = across.cross(up);
				normal.normalize();

				write_ptr[((ty * width + tx) << 2) + 0] = (127.5 + normal.x * 127.5);
				write_ptr[((ty * width + tx) << 2) + 1] = (127.5 + normal.y * 127.5);
				write_ptr[((ty * width + tx) << 2) + 2] = (127.5 + normal.z * 127.5);
				write_ptr[((ty * width + tx) << 2) + 3] = 255;
			}
		}
	}
	format = FORMAT_RGBA8;
	data = result_image;
}

bool Image::detect_signed(bool p_include_mips) const
{
	ERR_FAIL_COND_V(is_compressed(), false);

	if (format >= Image::FORMAT_RH && format <= Image::FORMAT_RGBAH) {
		const uint16_t* img_data = reinterpret_cast<const uint16_t*>(data.ptr());
		const uint64_t img_size = p_include_mips
									  ? (data.size() / 2)
									  : (width * height * get_format_pixel_size(format) / 2);

		for (uint64_t i = 0; i < img_size; i++) {
			if ((img_data[i] & 0x8000) != 0 && (img_data[i] & 0x7fff) != 0) {
				return true;
			}
		}

	}
	else if (format >= Image::FORMAT_RF && format <= Image::FORMAT_RGBAF) {
		const uint32_t* img_data = reinterpret_cast<const uint32_t*>(data.ptr());
		const uint64_t img_size = p_include_mips
									  ? (data.size() / 4)
									  : (width * height * get_format_pixel_size(format) / 4);

		for (uint64_t i = 0; i < img_size; i++) {
			if ((img_data[i] & 0x80000000) != 0 && (img_data[i] & 0x7fffffff) != 0) {
				return true;
			}
		}
	}

	return false;
}

void Image::srgb_to_linear()
{
	if (data.is_empty()) {
		return;
	}

	static const uint8_t srgb2lin[256] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7,
		8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 16,
		17, 17, 18, 18, 19, 19, 20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 29, 29,
		30, 31, 31, 32, 33, 33, 34, 35, 36, 36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47,
		47, 48, 49, 50, 51, 52, 53, 54, 55, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68,
		70, 71, 72, 73, 74, 75, 76, 77, 78, 80, 81, 82, 83, 84, 85, 87, 88, 89, 90, 92, 93, 94, 95,
		97, 98, 99, 101, 102, 103, 105, 106, 107, 109, 110, 112, 113, 114, 116, 117, 119, 120, 122,
		123, 125, 126, 128, 129, 131, 132, 134, 135, 137, 139, 140, 142, 144, 145, 147, 148, 150,
		152, 153, 155, 157, 159, 160, 162, 164, 166, 167, 169, 171, 173, 175, 176, 178, 180, 182,
		184, 186, 188, 190, 192, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213, 215, 218,
		220, 222, 224, 226, 228, 230, 232, 235, 237, 239, 241, 243, 245, 248, 250, 252, 255};

	ERR_FAIL_COND(format != FORMAT_RGB8 && format != FORMAT_RGBA8);

	if (format == FORMAT_RGBA8) {
		int len = data.size() / 4;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i << 2) + 0] = srgb2lin[data_ptr[(i << 2) + 0]];
			data_ptr[(i << 2) + 1] = srgb2lin[data_ptr[(i << 2) + 1]];
			data_ptr[(i << 2) + 2] = srgb2lin[data_ptr[(i << 2) + 2]];
		}

	}
	else if (format == FORMAT_RGB8) {
		int len = data.size() / 3;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i * 3) + 0] = srgb2lin[data_ptr[(i * 3) + 0]];
			data_ptr[(i * 3) + 1] = srgb2lin[data_ptr[(i * 3) + 1]];
			data_ptr[(i * 3) + 2] = srgb2lin[data_ptr[(i * 3) + 2]];
		}
	}
}

void Image::linear_to_srgb()
{
	if (data.is_empty()) {
		return;
	}

	static const uint8_t lin2srgb[256] = {0, 12, 21, 28, 33, 38, 42, 46, 49, 52, 55, 58, 61, 63, 66,
		68, 70, 73, 75, 77, 79, 81, 82, 84, 86, 88, 89, 91, 93, 94, 96, 97, 99, 100, 102, 103, 104,
		106, 107, 109, 110, 111, 112, 114, 115, 116, 117, 118, 120, 121, 122, 123, 124, 125, 126,
		127, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 142, 143, 144,
		145, 146, 147, 148, 149, 150, 151, 151, 152, 153, 154, 155, 156, 157, 157, 158, 159, 160,
		161, 161, 162, 163, 164, 165, 165, 166, 167, 168, 168, 169, 170, 171, 171, 172, 173, 174,
		174, 175, 176, 176, 177, 178, 179, 179, 180, 181, 181, 182, 183, 183, 184, 185, 185, 186,
		187, 187, 188, 189, 189, 190, 191, 191, 192, 193, 193, 194, 194, 195, 196, 196, 197, 197,
		198, 199, 199, 200, 201, 201, 202, 202, 203, 204, 204, 205, 205, 206, 206, 207, 208,
 208,
		209, 209, 210, 210, 211, 212, 212, 213, 213, 214, 214, 215, 215, 216, 217, 217, 218, 218,
		219, 219, 220, 220, 221, 221, 222, 222, 223, 223, 224, 224, 225, 226, 226, 227, 227, 228,
		228, 229, 229, 230, 230, 231, 231, 232, 232, 233, 233, 234, 234, 235, 235, 236, 236, 237,
		237, 237, 238, 238, 239, 239, 240, 240, 241, 241, 242, 242, 243, 243, 244, 244, 245, 245,
		245, 246, 246, 247, 247, 248, 248, 249, 249, 250, 250, 251, 251, 251, 252, 252, 253, 253,
		254, 254, 255};

	ERR_FAIL_COND(format != FORMAT_RGB8 && format != FORMAT_RGBA8);

	if (format == FORMAT_RGBA8) {
		int len = data.size() / 4;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i << 2) + 0] = lin2srgb[data_ptr[(i << 2) + 0]];
			data_ptr[(i << 2) + 1] = lin2srgb[data_ptr[(i << 2) + 1]];
			data_ptr[(i << 2) + 2] = lin2srgb[data_ptr[(i << 2) + 2]];
		}

	}
	else if (format == FORMAT_RGB8) {
		int len = data.size() / 3;
		uint8_t* data_ptr = data.ptrw();

		for (int i = 0; i < len; i++) {
			data_ptr[(i * 3) + 0] = lin2srgb[data_ptr[(i * 3) + 0]];
			data_ptr[(i * 3) + 1] = lin2srgb[data_ptr[(i * 3) + 1]];
			data_ptr[(i * 3) + 2] = lin2srgb[data_ptr[(i * 3) + 2]];
		}
	}
}

void Image::premultiply_alpha()
{
	if (data.is_empty()) {
		return;
	}

	if (format != FORMAT_RGBA8) {
		return; // not needed
	}

	uint8_t* data_ptr = data.ptrw();

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			uint8_t* ptr = &data_ptr[(i * width + j) * 4];

			ptr[0] = (uint16_t(ptr[0]) * uint16_t(ptr[3]) + 255U) >> 8;
			ptr[1] = (uint16_t(ptr[1]) * uint16_t(ptr[3]) + 255U) >> 8;
			ptr[2] = (uint16_t(ptr[2]) * uint16_t(ptr[3]) + 255U) >> 8;
		}
	}
}

void Image::fix_alpha_edges()
{
	if (data.is_empty()) {
		return;
	}

	if (format != FORMAT_RGBA8) {
		return; // not needed
	}

	Vector<uint8_t> dcopy = data;
	const uint8_t* srcptr = dcopy.ptr();

	uint8_t* data_ptr = data.ptrw();

	const int max_radius = 4;
	const int alpha_threshold = 20;
	const int max_dist = 0x7FFFFFFF;

	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			const uint8_t* rptr = &srcptr[(i * width + j) * 4];
			uint8_t* wptr = &data_ptr[(i * width + j) * 4];

			if (rptr[3] >= alpha_threshold) {
				continue;
			}

			int closest_dist = max_dist;
			uint8_t closest_color[3] = {0};

			int from_x = MAX(0, j - max_radius);
			int to_x = MIN(width - 1, j + max_radius);
			int from_y = MAX(0, i - max_radius);
			int to_y = MIN(height - 1, i + max_radius);

			for (int k = from_y; k <= to_y; k++) {
				for (int l = from_x; l <= to_x; l++) {
					int dy = i - k;
					int dx = j - l;
					int dist = dy * dy + dx * dx;
					if (dist >= closest_dist) {
						continue;
					}

					const uint8_t* rp2 = &srcptr[(k * width + l) << 2];

					if (rp2[3] < alpha_threshold) {
						continue;
					}

					closest_dist = dist;
					closest_color[0] = rp2[0];
					closest_color[1] = rp2[1];
					closest_color[2] = rp2[2];
				}
			}

			if (closest_dist != max_dist) {
				wptr[0] = closest_color[0];
				wptr[1] = closest_color[1];
				wptr[2] = closest_color[2];
			}
		}
	}
}

String Image::get_format_name(Format p_format)
{
	ERR_FAIL_INDEX_V(p_format, FORMAT_MAX, String());
	return format_names[p_format];
}

uint32_t Image::get_format_component_mask(Format p_format)
{
	const uint32_t r = 1;
	const uint32_t rg = 3;
	const uint32_t rgb = 7;
	const uint32_t rgba = 15;

	switch (p_format) {
	case FORMAT_L8:
		return rgb;
	case FORMAT_LA8:
		return rgba;
	case FORMAT_R8:
		return r;
	case FORMAT_RG8:
		return rg;
	case FORMAT_RGB8:
		return rgb;
	case FORMAT_RGBA8:
		return rgba;
	case FORMAT_RGBA4444:
		return rgba;
	case FORMAT_RGB565:
		return rgb;
	case FORMAT_RF:
		return r;
	case FORMAT_RGF:
		return rg;
	case FORMAT_RGBF:
		return rgb;
	case FORMAT_RGBAF:
		return rgba;
	case FORMAT_RH:
		return r;
	case FORMAT_RGH:
		return rg;
	case FORMAT_RGBH:
		return rgb;
	case FORMAT_RGBAH:
		return rgba;
	case FORMAT_RGBE9995:
		return rgb;
	case FORMAT_DXT1:
		return rgb;
	case FORMAT_DXT3:
		return rgba;
	case FORMAT_DXT5:
		return rgba;
	case FORMAT_RGTC_R:
		return r;
	case FORMAT_RGTC_RG:
		return rg;
	case FORMAT_BPTC_RGBA:
		return rgba;
	case FORMAT_BPTC_RGBF:
		return rgb;
	case FORMAT_BPTC_RGBFU:
		return rgb;
	case FORMAT_ETC:
		return rgb;
	case FORMAT_ETC2_R11:
		return r;
	case FORMAT_ETC2_R11S:
		return r;
	case FORMAT_ETC2_RG11:
		return rg;
	case FORMAT_ETC2_RG11S:
		return rg;
	case FORMAT_ETC2_RGB8:
		return rgb;
	case FORMAT_ETC2_RGBA8:
		return rgba;
	case FORMAT_ETC2_RGB8A1:
		return rgba;
	case FORMAT_ETC2_RA_AS_RG:
		return rg;
	case FORMAT_DXT5_RA_AS_RG:
		return rg;
	case FORMAT_ASTC_4x4:
		return rgba;
	case FORMAT_ASTC_4x4_HDR:
		return rgba;
	case FORMAT_ASTC_6x6:
		return rgba;
	case FORMAT_ASTC_6x6_HDR:
		return rgba;
	case FORMAT_ASTC_8x8:
		return rgba;
	case FORMAT_ASTC_8x8_HDR:
		return rgba;
	case FORMAT_R16:
		return r;
	case FORMAT_RG16:
		return rg;
	case FORMAT_RGB16:
		return rgb;
	case FORMAT_RGBA16:
		return rgba;
	case FORMAT_R16I:
		return r;
	case FORMAT_RG16I:
		return rg;
	case FORMAT_RGB16I:
		return rgb;
	case FORMAT_RGBA16I:
		return rgba;
	default:
		ERR_PRINT("Unhandled format.");
		return rgba;
	}
}

Error Image::load_png_from_buffer(const Vector<uint8_t>& p_array)
{
	return _load_from_buffer(p_array, _png_mem_loader_func);
}

Error Image::load_jpg_from_buffer(const Vector<uint8_t>& p_array)
{
	return _load_from_buffer(p_array, _jpg_mem_loader_func);
}

Error Image::load_exr_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_exr_mem_loader_func, ERR_UNAVAILABLE,
		"The TinyEXR module isn't enabled. Recompile the Godot editor or export template binary "
		"with the `tinyexr_export_templates=yes` SCons option.");
	return _load_from_buffer(p_array, _exr_mem_loader_func);
}

Error Image::load_webp_from_buffer(const Vector<uint8_t>& p_array)
{
	return _load_from_buffer(p_array, _webp_mem_loader_func);
}

Error Image::load_tga_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_tga_mem_loader_func, ERR_UNAVAILABLE,
		"The TGA module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_tga_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _tga_mem_loader_func);
}

Error Image::load_bmp_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_bmp_mem_loader_func, ERR_UNAVAILABLE,
		"The BMP module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_bmp_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _bmp_mem_loader_func);
}

Error Image::load_dds_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_dds_mem_loader_func, ERR_UNAVAILABLE,
		"The DDS module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_dds_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _dds_mem_loader_func);
}

Error Image::load_svg_from_buffer(const Vector<uint8_t>& p_array, float p_scale)
{
	ERR_FAIL_NULL_V_MSG(_svg_scalable_mem_loader_func, ERR_UNAVAILABLE,
		"The SVG module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_svg_enabled=yes` SCons option.");

	int buffer_size = p_array.size();

	ERR_FAIL_COND_V(buffer_size == 0, ERR_INVALID_PARAMETER);

	Ref<Image> image = _svg_scalable_mem_loader_func(p_array.ptr(), buffer_size, p_scale);
	ERR_FAIL_COND_V(image.is_null(), ERR_PARSE_ERROR);

	copy_internals_from(image);

	return OK;
}

Error Image::load_svg_from_string(const String& p_svg_str, float p_scale)
{
	return load_svg_from_buffer(p_svg_str.to_utf8_buffer(), p_scale);
}

Error Image::load_ktx_from_buffer(const Vector<uint8_t>& p_array)
{
	ERR_FAIL_NULL_V_MSG(_ktx_mem_loader_func, ERR_UNAVAILABLE,
		"The KTX module isn't enabled. Recompile the Godot editor or export template binary with "
		"the `module_ktx_enabled=yes` SCons option.");
	return _load_from_buffer(p_array, _ktx_mem_loader_func);
}

void Image::convert_rg_to_ra_rgba8()
{
	ERR_FAIL_COND(format != FORMAT_RGBA8);
	ERR_FAIL_COND(data.is_empty());

	int s = data.size();
	uint8_t* w = data.ptrw();
	for (int i = 0; i < s; i += 4) {
		w[i + 3] = w[i + 1];
		w[i + 1] = 0;
		w[i + 2] = 0;
	}
}

void Image::convert_ra_rgba8_to_rg()
{
	ERR_FAIL_COND(format != FORMAT_RGBA8);
	ERR_FAIL_COND(data.is_empty());

	int s = data.size();
	uint8_t* w = data.ptrw();
	for (int i = 0; i < s; i += 4) {
		w[i + 1] = w[i + 3];
		w[i + 2] = 0;
		w[i + 3] = 255;
	}
}

void Image::convert_rgba8_to_bgra8()
{
	ERR_FAIL_COND(format != FORMAT_RGBA8);
	ERR_FAIL_COND(data.is_empty());

	int s = data.size();
	uint8_t* w = data.ptrw();
	for (int i = 0; i < s; i += 4) {
		uint8_t r = w[i];
		w[i] = w[i + 2]; // Swap R to B
		w[i + 2] = r;	 // Swap B to R
	}
}

Error Image::_load_from_buffer(const Vector<uint8_t>& p_array, ImageMemLoadFunc p_loader)
{
	int buffer_size = p_array.size();

	ERR_FAIL_COND_V(buffer_size == 0, ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_loader, ERR_INVALID_PARAMETER);

	const uint8_t* r = p_array.ptr();

	Ref<Image> image = p_loader(r, buffer_size);
	ERR_FAIL_COND_V(image.is_null(), ERR_PARSE_ERROR);

	copy_internals_from(image);

	return OK;
}

void Image::average_4_uint8(
	uint8_t& p_out, const uint8_t& p_a, const uint8_t& p_b, const uint8_t& p_c, const uint8_t& p_d)
{
	p_out = static_cast<uint8_t>((p_a + p_b + p_c + p_d + 2) >> 2);
}

void Image::average_4_float(
	float& p_out, const float& p_a, const float& p_b, const float& p_c, const float& p_d)
{
	p_out = (p_a + p_b + p_c + p_d) * 0.25f;
}

void Image::average_4_half(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = Math::make_half_float((Math::half_to_float(p_a) + Math::half_to_float(p_b) +
									  Math::half_to_float(p_c) + Math::half_to_float(p_d)) *
								  0.25f);
}

void Image::average_4_rgbe9995(uint32_t& p_out, const uint32_t& p_a, const uint32_t& p_b,
	const uint32_t& p_c, const uint32_t& p_d)
{
	p_out = ((Color::from_rgbe9995(p_a) + Color::from_rgbe9995(p_b) + Color::from_rgbe9995(p_c) +
				 Color::from_rgbe9995(p_d)) *
			 0.25f)
				.to_rgbe9995();
}

void Image::average_4_uint16(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = static_cast<uint16_t>((p_a + p_b + p_c + p_d + 2) >> 2);
}

void Image::average_4_rgba4444(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = color_to_rgba4444((color_from_rgba4444(p_a) + color_from_rgba4444(p_b) +
								  color_from_rgba4444(p_c) + color_from_rgba4444(p_d)) *
							  0.25f);
}

void Image::average_4_rgb565(uint16_t& p_out, const uint16_t& p_a, const uint16_t& p_b,
	const uint16_t& p_c, const uint16_t& p_d)
{
	p_out = color_to_rgb565((color_from_rgb565(p_a) + color_from_rgb565(p_b) +
								color_from_rgb565(p_c) + color_from_rgb565(p_d)) *
							0.25f);
}

void Image::renormalize_uint8(uint8_t* p_rgb)
{
	Vector3 n(p_rgb[0] / 255.0, p_rgb[1] / 255.0, p_rgb[2] / 255.0);
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	n *= 255;
	p_rgb[0] = CLAMP(int(Math::round(n.x)), 0, 255);
	p_rgb[1] = CLAMP(int(Math::round(n.y)), 0, 255);
	p_rgb[2] = CLAMP(int(Math::round(n.z)), 0, 255);
}

void Image::renormalize_float(float* p_rgb)
{
	Vector3 n(p_rgb[0], p_rgb[1], p_rgb[2]);
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	p_rgb[0] = n.x;
	p_rgb[1] = n.y;
	p_rgb[2] = n.z;
}

void Image::renormalize_half(uint16_t* p_rgb)
{
	Vector3 n(Math::half_to_float(p_rgb[0]), Math::half_to_float(p_rgb[1]),
		Math::half_to_float(p_rgb[2]));
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	p_rgb[0] = Math::make_half_float(n.x);
	p_rgb[1] = Math::make_half_float(n.y);
	p_rgb[2] = Math::make_half_float(n.z);
}

void Image::renormalize_uint16(uint16_t* p_rgb)
{
	Vector3 n(p_rgb[0] / 65535.0, p_rgb[1] / 65535.0, p_rgb[2] / 65535.0);
	n *= 2.0;
	n -= Vector3(1, 1, 1);
	n.normalize();
	n += Vector3(1, 1, 1);
	n *= 0.5;
	n *= 65535;
	p_rgb[0] = CLAMP(int(Math::round(n.x)), 0, 65535);
	p_rgb[1] = CLAMP(int(Math::round(n.y)), 0, 65535);
	p_rgb[2] = CLAMP(int(Math::round
(n.z)), 0, 65535);
}

Image::Image(const uint8_t* p_mem_png_jpg, int p_len)
{
	width = 0;
	height = 0;
	mipmaps = false;
	format = FORMAT_L8;

	if (_png_mem_loader_func) {
		copy_internals_from(_png_mem_loader_func(p_mem_png_jpg, p_len));
	}

	if (is_empty() && _jpg_mem_loader_func) {
		copy_internals_from(_jpg_mem_loader_func(p_mem_png_jpg, p_len));
	}

	if (is_empty() && _webp_mem_loader_func) {
		copy_internals_from(_webp_mem_loader_func(p_mem_png_jpg, p_len));
	}
}

void Image::set_as_black() { memset(data.ptrw(), 0, data.size()); }

void Image::copy_internals_from(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_MSG(p_image.is_null(), "Cannot copy image internals: invalid Image object.");
	format = p_image->format;
	width = p_image->width;
	height = p_image->height;
	mipmaps = p_image->mipmaps;
	data = p_image->data;
}
