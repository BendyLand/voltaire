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
#include "core/error/error_macros.h"
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
template <int Tag>
class SafeBinaryMutex {
	friend class MutexLock<SafeBinaryMutex<Tag>>;

	using StdMutexType = THREADING_NAMESPACE::mutex;

	mutable THREADING_NAMESPACE::mutex mutex;

	struct TLSData {
		mutable THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> lock;
		uint32_t count = 0;

		TLSData(SafeBinaryMutex<Tag> &p_mutex) :
				lock(p_mutex.mutex, THREADING_NAMESPACE::defer_lock) {}
	};
	static thread_local TLSData tls_data;

public:
	_ALWAYS_INLINE_ void lock() const {
		if (++tls_data.count == 1) {
			tls_data.lock.lock();
		}
	}

	_ALWAYS_INLINE_ void unlock() const {
		DEV_ASSERT(tls_data.count);
		if (--tls_data.count == 0) {
			tls_data.lock.unlock();
		}
	}

	_ALWAYS_INLINE_ THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> &_get_lock() const {
		return const_cast<THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> &>(tls_data.lock);
	}

	_ALWAYS_INLINE_ SafeBinaryMutex() {
	}

	_ALWAYS_INLINE_ ~SafeBinaryMutex() {
		DEV_ASSERT(!tls_data.count);
	}
};

template <int Tag>
class MutexLock<SafeBinaryMutex<Tag>> {
	friend class ConditionVariable;

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

	// TODO: Implement a `try_temp_relock` if needed (will also need a dummy method below).
};

VLTR_CLANG_WARNING_POP

#else // No threads.

template <int Tag>
class SafeBinaryMutex {
	struct TLSData {
		TLSData(SafeBinaryMutex<Tag> &p_mutex) {}
	};
	static thread_local TLSData tls_data;

public:
	void lock() const {}
	void unlock() const {}
};

template <int Tag>
class MutexLock<SafeBinaryMutex<Tag>> {
public:
	MutexLock(const SafeBinaryMutex<Tag> &p_mutex) {}

	void temp_relock() const {}
	void temp_unlock() const {}
};

#endif // THREADS_ENABLED

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

class ConditionVariable {
	mutable THREADING_NAMESPACE::condition_variable condition;

public:
	template <typename BinaryMutexT>
	_ALWAYS_INLINE_ void wait(const MutexLock<BinaryMutexT> &p_lock) const {
		condition.wait(p_lock._get_lock());
	}

	template <int Tag>
	_ALWAYS_INLINE_ void wait(const MutexLock<SafeBinaryMutex<Tag>> &p_lock) const {
		condition.wait(p_lock.mutex._get_lock());
	}

	_ALWAYS_INLINE_ void notify_one() const {
		condition.notify_one();
	}

	_ALWAYS_INLINE_ void notify_all() const {
		condition.notify_all();
	}
};

#else // No threads.

class ConditionVariable {
public:
	template <typename BinaryMutexT>
	void wait(const MutexLock<BinaryMutexT> &p_lock) const {}
	void notify_one() const {}
	void notify_all() const {}
};

#endif // THREADS_ENABLED

// --- Folded from core/templates/command_queue_mt.h ---
#include "core/os/mutex.h"
#include "core/templates/local_vector.h"
// --- Folded from core/templates/simple_type.h ---
#include <type_traits>

template <typename T>
using GetSimpleTypeT = typename std::remove_cv_t<std::remove_reference_t<T>>;

// --- End core/templates/simple_type.h ---
// --- Folded from core/templates/tuple.h ---
// Simple recursive Tuple type that has no runtime overhead.
//
// The compile-time recursion works as follows:
// Assume the following: Tuple<int, float> my_tuple(42, 3.14f);
// This expands to a class hierarchy that inherits from the previous step.
// So in this case this leads to:
//  - struct Tuple<int> : Tuple<float>   <--- This contains the int value.
//  - struct Tuple<float>                <--- This contains the float value.
// where each of the classes has a single field of the type for that step in the
// recursion. So: float value;  int value; etc.
//
// This works by splitting up the parameter pack for each step in the recursion minus the first.
// so the first step creates the "T value" from the first template parameter.
// any further template arguments end up in "Rest", which we then use to instantiate a new
// tuple, but now minus the first argument. To write this all out:
//
// Tuple<int, float>
// step 1: Tuple T = int, Rest = float. Results in a Tuple<int> : Tuple<float>
// step 2: Tuple T = float, no Rest. Results in a Tuple<float>
//
// tuple_get<I> works through a similar recursion, using the inheritance chain to walk to the right node.
// In order to tuple_get<1>(my_tuple), from the example tuple above:
//
// 1. We want tuple_get<1> to return the float, which is one level "up" from Tuple<int> : Tuple<float>,
//    (the real type of the Tuple "root").
// 2. Since index 1 > 0, it casts the tuple to its parent type (Tuple<float>). This works because
//    we cast to Tuple<Rest...> which in this case is just float.
// 3. Now we're looking for index 0 in Tuple<float>, which directly returns its value field. Note
//    how get<0> is a template specialization.
//
// At compile time, this gets fully resolved. The compiler sees get<1>(my_tuple) and:
// 1. Creates TupleGet<1, Tuple<int, float>>::tuple_get which contains the cast to Tuple<float>.
// 2. Creates TupleGet<0, Tuple<float>>::tuple_get which directly returns the value.
// 3. The compiler will then simply optimize all of this nonsense away and return the float directly.

#include "core/typedefs.h"

template <typename... Types>
struct Tuple;

template <>
struct Tuple<> {};

template <typename T, typename... Rest>
struct Tuple<T, Rest...> : Tuple<Rest...> {
	T value;

	Tuple() = default;

	template <typename F, typename... R>
	_FORCE_INLINE_ Tuple(F &&p_first, R &&...p_rest) :
			Tuple<Rest...>(std::forward<R>(p_rest)...),
			value(std::forward<F>(p_first)) {}
};

// Tuple is zero-constructible if and only if all constrained types are zero-constructible.
template <typename... Types>
struct is_zero_constructible<Tuple<Types...>> : std::conjunction<is_zero_constructible<Types>...> {};

template <size_t I, typename Tuple>
struct TupleGet;

template <typename First, typename... Rest>
struct TupleGet<0, Tuple<First, Rest...>> {
	_FORCE_INLINE_ static First &tuple_get(Tuple<First, Rest...> &p_tuple) {
		return p_tuple.value;
	}
};

// Rationale for using auto here is that the alternative is writing a
// helper struct to create an otherwise useless type. we would have to write
// a second recursive template chain like: TupleGetType<I, Tuple<First, Rest...>>::type
// just to recover the type in the most baroque way possible.

template <size_t I, typename First, typename... Rest>
struct TupleGet<I, Tuple<First, Rest...>> {
	_FORCE_INLINE_ static auto &tuple_get(Tuple<First, Rest...> &p_tuple) {
		return TupleGet<I - 1, Tuple<Rest...>>::tuple_get(static_cast<Tuple<Rest...> &>(p_tuple));
	}
};

template <size_t I, typename... Types>
_FORCE_INLINE_ auto &tuple_get(Tuple<Types...> &p_tuple) {
	return TupleGet<I, Tuple<Types...>>::tuple_get(p_tuple);
}

template <size_t I, typename... Types>
_FORCE_INLINE_ const auto &tuple_get(const Tuple<Types...> &p_tuple) {
	return TupleGet<I, Tuple<Types...>>::tuple_get(p_tuple);
}

// --- End core/templates/tuple.h ---
#include "core/typedefs.h"

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

	template <typename T, typename M, bool NeedsSync, typename... Args>
	struct Command : public CommandBase
	{
		T* instance;
		M method;
		Tuple<GetSimpleTypeT<Args>...> args;

		template <typename... FwdArgs>
		_FORCE_INLINE_ Command(T* p_instance, M p_method, FwdArgs&&... p_args)
			: CommandBase(NeedsSync), instance(p_instance), method(p_method),
			  args(std::forward<FwdArgs>(p_args)...)
		{
		}

		void call() override { call_impl(BuildIndexSequence<sizeof...(Args)>{}); }

	private:
		template <size_t... I> _FORCE_INLINE_ void call_impl(IndexSequence<I...>)
		{
			// Move out of the Tuple, this will be destroyed as soon as the call is complete.
			(instance->*method)(std::move(get<I>())...);
		}

		// This method exists so we can call it in the parameter pack expansion in call_impl.
		template <size_t I> _FORCE_INLINE_ auto& get() { return ::tuple_get<I>(args); }
	};

	// Separate class from Command so we can save the space of the ret pointer for commands that
	// don't return.
	template <typename T, typename M, typename R, typename... Args>
	struct CommandRet : public CommandBase
	{
		T* instance;
		M method;
		R* ret;
		Tuple<GetSimpleTypeT<Args>...> args;

		_FORCE_INLINE_ CommandRet(
			T* p_instance, M p_method, R* p_ret, GetSimpleTypeT<Args>... p_args)
			: CommandBase(true), instance(p_instance), method(p_method), ret(p_ret), args{p_args...}
		{
		}

		void call() override { *ret = call_impl(BuildIndexSequence<sizeof...(Args)>{}); }

	private:
		template <size_t... I> _FORCE_INLINE_ R call_impl(IndexSequence<I...>)
		{
			// Move out of the Tuple, this will be destroyed as soon as the call is complete.
			return (instance->*method)(std::move(get<I>())...);
		}

		// This method exists so we can call it in the parameter pack expansion in call_impl.
		template <size_t I> _FORCE_INLINE_ auto& get() { return ::tuple_get<I>(args); }
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
		// Standard command, no sync.
		using CommandType = Command<T, M, false, Args...>;
		static_assert(sizeof(CommandType) <= MAX_COMMAND_SIZE);
		_push_internal<CommandType, false>(p_instance, p_method, std::forward<Args>(p_args)...);
	}

	template <typename T, typename M, typename... Args>
	void push_and_sync(T* p_instance, M p_method, Args... p_args)
	{
		// Standard command, sync.
		using CommandType = Command<T, M, true, Args...>;
		static_assert(sizeof(CommandType) <= MAX_COMMAND_SIZE);
		_push_internal<CommandType, true>(p_instance, p_method, std::forward<Args>(p_args)...);
	}

	template <typename T, typename M, typename R, typename... Args>
	void push_and_ret(T* p_instance, M p_method, R* r_ret, Args... p_args)
	{
		// Command with return value, sync.
		using CommandType = CommandRet<T, M, R, Args...>;
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
