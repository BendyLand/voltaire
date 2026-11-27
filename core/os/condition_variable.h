/**************************************************************************/
/*  condition_variable.h                                                  */
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

#include "core/os/mutex.h"
// --- Folded from core/os/safe_binary_mutex.h ---
#include "core/typedefs.h"
#include "core/os/mutex.h"
#include "core/typedefs.h"

#ifdef THREADS_ENABLED

VLTR_CLANG_WARNING_PUSH_AND_IGNORE("-Wundefined-var-template")

// A very special kind of mutex, used in scenarios where these
// requirements hold at the same time:
// - Must be used with a condition variable (only binary mutexes are suitable).
// - Must have recursive semnantics (or simulate, as this one does).
// The implementation keeps the lock count in TS. Therefore, only
// one object of each version of the template can exists; hence the Tag argument.
// Tags must be unique across the Godot codebase.
// Also, don't forget to declare the thread_local variable on each use.
VLTR_CLANG_WARNING_POP

#endif // THREADS_ENABLED

template <int Tag>
class SafeBinaryMutex {
	friend class MutexLock<SafeBinaryMutex<Tag>>;

#ifdef THREADS_ENABLED
	using StdMutexType = THREADING_NAMESPACE::mutex;
	mutable THREADING_NAMESPACE::mutex mutex;
#endif

	struct TLSData {
#ifdef THREADS_ENABLED
		mutable THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> lock;
		uint32_t count = 0;

		TLSData(SafeBinaryMutex<Tag> &p_mutex) :
				lock(p_mutex.mutex, THREADING_NAMESPACE::defer_lock) {}
#else
		TLSData(SafeBinaryMutex<Tag> &p_mutex) {}
#endif
	};
	static thread_local TLSData tls_data;

public:
	_ALWAYS_INLINE_ void lock() const {
#ifdef THREADS_ENABLED
		if (++tls_data.count == 1) {
			tls_data.lock.lock();
		}
#endif
	}

	_ALWAYS_INLINE_ void unlock() const {
#ifdef THREADS_ENABLED
		DEV_ASSERT(tls_data.count);
		if (--tls_data.count == 0) {
			tls_data.lock.unlock();
		}
#endif
	}

#ifdef THREADS_ENABLED
	_ALWAYS_INLINE_ THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> &_get_lock() const {
		return const_cast<THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> &>(tls_data.lock);
	}

	_ALWAYS_INLINE_ SafeBinaryMutex() {}

	_ALWAYS_INLINE_ ~SafeBinaryMutex() {
		DEV_ASSERT(!tls_data.count);
	}
#endif
};

template <int Tag>
class MutexLock<SafeBinaryMutex<Tag>> {
	friend class ConditionVariable;

#ifdef THREADS_ENABLED
	const SafeBinaryMutex<Tag> &mutex;

public:
	explicit MutexLock(const SafeBinaryMutex<Tag> &p_mutex) :
			mutex(p_mutex) {
		mutex.lock();
	}

	~MutexLock() {
		mutex.unlock();
	}

	_ALWAYS_INLINE_ void temp_relock() const {
		mutex.lock();
	}

	_ALWAYS_INLINE_ void temp_unlock() const {
		mutex.unlock();
	}
#else
public:
	MutexLock(const SafeBinaryMutex<Tag> &p_mutex) {}

	void temp_relock() const {}
	void temp_unlock() const {}
#endif
};

// --- End core/os/safe_binary_mutex.h ---

#ifdef THREADS_ENABLED

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.condition_variable.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <condition_variable>
#define THREADING_NAMESPACE std
#endif

// An object one or multiple threads can wait on a be notified by some other.
// Normally, you want to use a semaphore for such scenarios, but when the
// condition is something different than a count being greater than zero
// (which is the built-in logic in a semaphore) or you want to provide your
// own mutex to tie the wait-notify to some other behavior, you need to use this.

#endif // THREADS_ENABLED

class ConditionVariable {
#ifdef THREADS_ENABLED
	mutable THREADING_NAMESPACE::condition_variable condition;
#endif

public:
	template <typename BinaryMutexT>
	_ALWAYS_INLINE_ void wait(const MutexLock<BinaryMutexT> &p_lock) const {
#ifdef THREADS_ENABLED
		condition.wait(p_lock._get_lock());
#endif
	}

	template <int Tag>
	_ALWAYS_INLINE_ void wait(const MutexLock<SafeBinaryMutex<Tag>> &p_lock) const {
#ifdef THREADS_ENABLED
		condition.wait(p_lock.mutex._get_lock());
#endif
	}

	_ALWAYS_INLINE_ void notify_one() const {
#ifdef THREADS_ENABLED
		condition.notify_one();
#endif
	}

	_ALWAYS_INLINE_ void notify_all() const {
#ifdef THREADS_ENABLED
		condition.notify_all();
#endif
	}
};

// --- Folded from core/templates/command_queue_mt.h ---
#include "core/os/mutex.h"
#include "core/string/ustring.h"
// --- Folded from core/templates/simple_type.h ---
#include <type_traits>

template <typename T>
using GetSimpleTypeT = typename std::remove_cv_t<std::remove_reference_t<T>>;

// --- End core/templates/simple_type.h ---
#include "core/typedefs.h"
#include <tuple>

class CommandQueueMT
{
	static const size_t MAX_COMMAND_SIZE = 1024;

	struct CommandBase
	{
		bool sync = false;
		virtual void call() = 0;
		virtual ~CommandBase() = default;

		CommandBase(bool p_sync) : sync(p_sync) {}
	};

	template <typename T, typename M, bool NeedsSync, typename R, typename... Args>
	struct Command : public CommandBase
	{
		T* instance;
		M method;
		R* ret;
		std::tuple<GetSimpleTypeT<Args>...> args;

		template <typename... FwdArgs>
		_FORCE_INLINE_ Command(T* p_instance, M p_method, R* p_ret, FwdArgs&&... p_args)
			: CommandBase(NeedsSync), instance(p_instance), method(p_method), ret(p_ret),
			  args(std::forward<FwdArgs>(p_args)...)
		{
		}

		void call() override
		{
			if constexpr (std::is_void_v<R>) {
				std::apply([this](auto&&... p_a) { (instance->*method)(std::move(p_a)...); }, args);
			} else {
				*ret = std::apply([this](auto&&... p_a) -> R { return (instance->*method)(std::move(p_a)...); }, args);
			}
		}
	};

	/***** BASE *******/

	static const uint32_t DEFAULT_COMMAND_MEM_SIZE_KB = 64;

	inline static thread_local bool flushing = false;

	BinaryMutex mutex;
	LocalVector<uint8_t> command_mem;
	ConditionVariable sync_cond_var;
	uint32_t sync_head = 0;
	uint32_t sync_tail = 0;
	uint32_t sync_awaiters = 0;
	uint64_t flush_read_ptr = 0;
	std::atomic<bool> pending{false};

	template <typename T, typename... Args> _FORCE_INLINE_ void create_command(Args&&... p_args)
	{
		// alloc size is size+T+safeguard
		constexpr uint64_t alloc_size = ((sizeof(T) + 8U - 1U) & ~(8U - 1U));
		static_assert(alloc_size < UINT32_MAX, "Type too large to fit in the command queue.");

		uint64_t size = command_mem.size();
		command_mem.resize(size + alloc_size + sizeof(uint64_t));
		*(uint64_t*)&command_mem[size] = alloc_size;
		void* cmd = &command_mem[size + sizeof(uint64_t)];
		memnew_placement(cmd, T(std::forward<Args>(p_args)...));
		pending.store(true);
	}

	template <typename T, bool NeedsSync, typename... Args>
	_FORCE_INLINE_ void _push_internal(Args&&... p_args)
	{
		MutexLock mlock(mutex);
		create_command<T>(std::forward<Args>(p_args)...);

		if constexpr (NeedsSync) {
			sync_tail++;
			_wait_for_sync(mlock);
		}
	}

	_FORCE_INLINE_ void _prevent_sync_wraparound()
	{
		bool safe_to_reset = !sync_awaiters;
		bool already_sync_to_latest = sync_head == sync_tail;
		if (safe_to_reset && already_sync_to_latest) {
			sync_head = 0;
			sync_tail = 0;
		}
	}

	void _flush()
	{
		// Safeguard against trying to re-lock the binary mutex.
		if (flushing) {
			return;
		}

		flushing = true;

		MutexLock lock(mutex);

		if (unlikely(flush_read_ptr)) {
			// Another thread is flushing.
			lock.temp_unlock(); // Not really temp.
			sync();
			flushing = false;
			return;
		}

		alignas(uint64_t) char cmd_local_mem[MAX_COMMAND_SIZE];

		while (flush_read_ptr < command_mem.size()) {
			uint64_t size = *(uint64_t*)&command_mem[flush_read_ptr];
			flush_read_ptr += sizeof(uint64_t);

			// Protect against race condition between this thread
			// during the call to the command and other threads potentially
			// invalidating the pointer due to reallocs by relocating the object.
			CommandBase* cmd_original =
				reinterpret_cast<CommandBase*>(&command_mem[flush_read_ptr]);
			CommandBase* cmd_local = reinterpret_cast<CommandBase*>(cmd_local_mem);
			memcpy(cmd_local_mem, (char*)cmd_original, size);

			lock.temp_unlock();
			cmd_local->call();
			lock.temp_relock();

			if (unlikely(cmd_local->sync)) {
				sync_head++;
				lock.temp_unlock(); // Give an opportunity to awaiters right away.
				sync_cond_var.notify_all();
				lock.temp_relock();
			}

			cmd_local->~CommandBase();

			flush_read_ptr += size;
		}

		command_mem.clear();
		pending.store(false);
		flush_read_ptr = 0;

		_prevent_sync_wraparound();

		flushing = false;
	}

	_FORCE_INLINE_ void _wait_for_sync(MutexLock<BinaryMutex>& p_lock)
	{
		sync_awaiters++;
		uint32_t sync_head_goal = sync_tail;
		do {
			sync_cond_var.wait(p_lock);
		} while (sync_head < sync_head_goal);
		sync_awaiters--;
		_prevent_sync_wraparound();
	}

	void _no_op() {}

public:
	template <typename T, typename M, typename... Args>
	void push(T* p_instance, M p_method, Args&&... p_args)
	{
		using CommandType = Command<T, M, false, void, Args...>;
		static_assert(sizeof(CommandType) <= MAX_COMMAND_SIZE);
		_push_internal<CommandType, false>(
			p_instance, p_method, static_cast<void*>(nullptr), std::forward<Args>(p_args)...);
	}

	template <typename T, typename M, typename... Args>
	void push_and_sync(T* p_instance, M p_method, Args... p_args)
	{
		using CommandType = Command<T, M, true, void, Args...>;
		static_assert(sizeof(CommandType) <= MAX_COMMAND_SIZE);
		_push_internal<CommandType, true>(
			p_instance, p_method, static_cast<void*>(nullptr), std::forward<Args>(p_args)...);
	}

	template <typename T, typename M, typename R, typename... Args>
	void push_and_ret(T* p_instance, M p_method, R* r_ret, Args... p_args)
	{
		using CommandType = Command<T, M, true, R, Args...>;
		static_assert(sizeof(CommandType) <= MAX_COMMAND_SIZE);
		_push_internal<CommandType, true>(
			p_instance, p_method, r_ret, std::forward<Args>(p_args)...);
	}

	_FORCE_INLINE_ void flush_if_pending()
	{
		if (unlikely(pending.load())) {
			_flush();
		}
	}

	void flush_all() { _flush(); }

	void sync() { push_and_sync(this, &CommandQueueMT::_no_op); }

	void wait_and_flush() { _flush(); }

	CommandQueueMT() { command_mem.reserve(DEFAULT_COMMAND_MEM_SIZE_KB * 1024); }
};
