/**************************************************************************/
/*  mutex.h                                                               */
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

#include "core/typedefs.h"

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.mutex.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <mutex>
#define THREADING_NAMESPACE std
#endif

template <typename MutexT>
class MutexLock;

template <typename StdMutexT>
class MutexImpl {
	friend class MutexLock<MutexImpl<StdMutexT>>;

	using StdMutexType = StdMutexT;

	mutable StdMutexT mutex;

public:
#ifdef THREADS_ENABLED
	_ALWAYS_INLINE_ void lock() const {
		mutex.lock();
	}

	_ALWAYS_INLINE_ void unlock() const {
		mutex.unlock();
	}

	_ALWAYS_INLINE_ bool try_lock() const {
		return mutex.try_lock();
	}
#else
	void lock() const {}
	void unlock() const {}
	bool try_lock() const { return true; }
#endif
};

template <typename MutexT>
class [[nodiscard]] MutexLock {
#ifdef THREADS_ENABLED
	mutable THREADING_NAMESPACE::unique_lock<typename MutexT::StdMutexType> lock;

public:
	explicit MutexLock(const MutexT &p_mutex) :
			lock(p_mutex.mutex) {}

	// Clarification: all the funny syntax is needed so this function exists only for binary mutexes.
	template <typename T = MutexT>
	_ALWAYS_INLINE_ THREADING_NAMESPACE::unique_lock<THREADING_NAMESPACE::mutex> &_get_lock(
			typename std::enable_if<std::is_same<T, THREADING_NAMESPACE::mutex>::value> * = nullptr) const {
		return lock;
	}

	_ALWAYS_INLINE_ void temp_relock() const {
		lock.lock();
	}

	_ALWAYS_INLINE_ void temp_unlock() const {
		lock.unlock();
	}
#else
public:
	explicit MutexLock(const MutexT &p_mutex) {}

	void temp_relock() const {}
	void temp_unlock() const {}
#endif
};

using Mutex = MutexImpl<THREADING_NAMESPACE::recursive_mutex>; // Recursive, for general use
using BinaryMutex = MutexImpl<THREADING_NAMESPACE::mutex>; // Non-recursive, handle with care

#ifdef THREADS_ENABLED
extern template class MutexImpl<THREADING_NAMESPACE::recursive_mutex>;
extern template class MutexImpl<THREADING_NAMESPACE::mutex>;
extern template class MutexLock<MutexImpl<THREADING_NAMESPACE::recursive_mutex>>;
extern template class MutexLock<MutexImpl<THREADING_NAMESPACE::mutex>>;
#endif // THREADS_ENABLED

// --- Folded from core/os/spin_lock.h ---
// --- Folded from core/os/thread.h ---
#include "platform/linuxbsd/platform_config.h" // IWYU pragma: keep. Can override the implementation.

// Define PLATFORM_THREAD_OVERRIDE in your platform's `platform_config.h`
// to use a custom Thread implementation defined in `platform/[your_platform]/platform_thread.h`.
// Overriding the Thread implementation is required in some proprietary platforms.

#ifdef PLATFORM_THREAD_OVERRIDE
#include "core/os/memory.h"
#include "core/typedefs.h"
#include <pthread.h>
#include <new>
#elif defined(THREADS_ENABLED)
#include "core/os/memory.h"
#include "core/typedefs.h"
#include <new>
#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.thread.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <thread>
#define THREADING_NAMESPACE std
#endif
#else
#include "core/typedefs.h"
#endif

class String;

class Thread {
public:
	typedef void (*Callback)(void *p_userdata);
	typedef uint64_t ID;

	enum : ID {
		UNASSIGNED_ID = 0,
		MAIN_ID = 1
	};

	enum Priority {
		PRIORITY_LOW,
		PRIORITY_NORMAL,
		PRIORITY_HIGH
	};

	struct Settings {
		Priority priority;
		Settings() { priority = PRIORITY_NORMAL; }
	};

#ifndef PLATFORM_THREAD_OVERRIDE
	struct PlatformFunctions {
		Error (*set_name)(const String &) = nullptr;
		void (*set_priority)(Thread::Priority) = nullptr;
		void (*init)() = nullptr;
		void (*wrapper)(Thread::Callback, void *) = nullptr;
		void (*term)() = nullptr;
	};
#endif

#if defined(PLATFORM_THREAD_OVERRIDE)
#if defined(__cpp_lib_hardware_interference_size)
	VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Winterference-size")
	static constexpr size_t CACHE_LINE_BYTES = std::hardware_destructive_interference_size;
	VLTR_GCC_WARNING_POP
#else
	static constexpr size_t CACHE_LINE_BYTES = 128;
#endif
#elif defined(THREADS_ENABLED)
#if defined(__cpp_lib_hardware_interference_size) && !defined(ANDROID_ENABLED)
	VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Winterference-size")
	static constexpr size_t CACHE_LINE_BYTES = std::hardware_destructive_interference_size;
	VLTR_GCC_WARNING_POP
#elif defined(__GCC_DESTRUCTIVE_SIZE) && !defined(ANDROID_ENABLED)
	static constexpr size_t CACHE_LINE_BYTES = __GCC_DESTRUCTIVE_SIZE;
#else
	static constexpr size_t CACHE_LINE_BYTES = 128;
#endif
#else
	static constexpr size_t CACHE_LINE_BYTES = sizeof(void *);
#endif

private:
	friend class Main;

#ifdef PLATFORM_THREAD_OVERRIDE
	ID id = UNASSIGNED_ID;
	pthread_t pthread;

	static inline SafeFlag is_main_thread_assigned{ false };
	static SafeNumeric<uint64_t> id_counter;
	static thread_local ID caller_id;

	static void *thread_callback(void *p_data);
#elif defined(THREADS_ENABLED)
	static PlatformFunctions platform_functions;

	ID id = UNASSIGNED_ID;

	static inline SafeFlag is_main_thread_assigned{ false };
	static SafeNumeric<uint64_t> id_counter;
	static thread_local ID caller_id;
	THREADING_NAMESPACE::thread thread;

	static void callback(ID p_caller_id, const Settings &p_settings, Thread::Callback p_callback, void *p_userdata);
#else
	static PlatformFunctions platform_functions;
#endif

public:
#ifndef PLATFORM_THREAD_OVERRIDE
	static void _set_platform_functions(const PlatformFunctions &p_functions);
#endif

#ifdef PLATFORM_THREAD_OVERRIDE
	_FORCE_INLINE_ static void yield() { pthread_yield_np(); }

	_FORCE_INLINE_ ID get_id() const { return id; }
	_FORCE_INLINE_ static ID get_caller_id() { return caller_id; }
	_FORCE_INLINE_ static ID get_main_id() { return MAIN_ID; }

	_FORCE_INLINE_ static bool is_main_thread() { return caller_id == MAIN_ID; }
	static void make_main_thread();
	static void release_main_thread();

	static Error set_name(const String &p_name);

	ID start(Thread::Callback p_callback, void *p_user, const Settings &p_settings = Settings());
	bool is_started() const { return id != UNASSIGNED_ID; }
	void wait_to_finish();

	Thread() = default;
	~Thread();
#elif defined(THREADS_ENABLED)
	_FORCE_INLINE_ static void yield() { std::this_thread::yield(); }

	_FORCE_INLINE_ ID get_id() const { return id; }
	_FORCE_INLINE_ static ID get_caller_id() { return caller_id; }
	_FORCE_INLINE_ static ID get_main_id() { return MAIN_ID; }

	_FORCE_INLINE_ static bool is_main_thread() { return caller_id == MAIN_ID; }
	static void make_main_thread();
	static void release_main_thread();

	static Error set_name(const String &p_name);

	ID start(Thread::Callback p_callback, void *p_user, const Settings &p_settings = Settings());
	bool is_started() const;
	void wait_to_finish();

	~Thread();
#else
	_FORCE_INLINE_ static void yield() {}

	_FORCE_INLINE_ ID get_id() const { return 0; }
	_FORCE_INLINE_ static ID get_caller_id() { return MAIN_ID; }
	_FORCE_INLINE_ static ID get_main_id() { return MAIN_ID; }

	_FORCE_INLINE_ static bool is_main_thread() { return true; }
	static void make_main_thread() {}
	static void release_main_thread() {}

	static Error set_name(const String &p_name) { return ERR_UNAVAILABLE; }

	void start(Thread::Callback p_callback, void *p_user, const Settings &p_settings = Settings()) {}
	bool is_started() const { return false; }
	void wait_to_finish() {}
#endif
};
// --- End core/os/thread.h ---
#include "core/typedefs.h"

#ifdef THREADS_ENABLED
#ifdef _MSC_VER
#include <intrin.h>
#endif
#if defined(__APPLE__)
#include <os/lock.h>
#else
#include <atomic>

_ALWAYS_INLINE_ static void _cpu_pause() {
#if defined(_MSC_VER)
#if defined(_M_ARM) || defined(_M_ARM64)
	__yield();
#elif defined(_M_IX86) || defined(_M_X64)
	_mm_pause();
#endif
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
	__builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__)
	asm volatile("yield");
#elif defined(__powerpc__)
	asm volatile("or 27,27,27");
#elif defined(__riscv)
	asm volatile(".insn i 0x0F, 0, x0, x0, 0x010");
#endif
#endif
}

static_assert(std::atomic_bool::is_always_lock_free);
#endif // __APPLE__
#endif // THREADS_ENABLED

class SpinLock {
#ifdef THREADS_ENABLED
	union {
#if defined(__APPLE__)
		mutable os_unfair_lock _lock = OS_UNFAIR_LOCK_INIT;
#elif __cplusplus >= 202002L
		mutable std::atomic<bool> locked = false;
#else
		mutable std::atomic<bool> locked = ATOMIC_VAR_INIT(false);
#endif
		char aligner[Thread::CACHE_LINE_BYTES];
	};
#endif

public:
	_ALWAYS_INLINE_ void lock() const {
#ifdef THREADS_ENABLED
#if defined(__APPLE__)
		os_unfair_lock_lock(&_lock);
#else
		while (true) {
			bool expected = false;
			if (locked.compare_exchange_weak(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
				break;
			}
			do {
				_cpu_pause();
			} while (locked.load(std::memory_order_relaxed));
		}
#endif
#endif
	}

	_ALWAYS_INLINE_ void unlock() const {
#ifdef THREADS_ENABLED
#if defined(__APPLE__)
		os_unfair_lock_unlock(&_lock);
#else
		locked.store(false, std::memory_order_release);
#endif
#endif
	}
};

// --- Folded from core/os/rw_lock.h ---
#include "core/typedefs.h"

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.shared_mutex.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <shared_mutex>
#define THREADING_NAMESPACE std
#endif

class RWLock {
	mutable THREADING_NAMESPACE::shared_mutex mutex;

public:
	// Lock the RWLock, block if locked by someone else.
	_ALWAYS_INLINE_ void read_lock() const {
		mutex.lock_shared();
	}

	// Unlock the RWLock, let other threads continue.
	_ALWAYS_INLINE_ void read_unlock() const {
		mutex.unlock_shared();
	}

	// Attempt to lock the RWLock for reading. True on success, false means it can't lock.
	_ALWAYS_INLINE_ bool read_try_lock() const {
		return mutex.try_lock_shared();
	}

	// Lock the RWLock, block if locked by someone else.
	_ALWAYS_INLINE_ void write_lock() {
		mutex.lock();
	}

	// Unlock the RWLock, let other threads continue.
	_ALWAYS_INLINE_ void write_unlock() {
		mutex.unlock();
	}

	// Attempt to lock the RWLock for writing. True on success, false means it can't lock.
	_ALWAYS_INLINE_ bool write_try_lock() {
		return mutex.try_lock();
	}
};

class RWLockRead {
	const RWLock &lock;

public:
	_ALWAYS_INLINE_ RWLockRead(const RWLock &p_lock) :
			lock(p_lock) {
		lock.read_lock();
	}
	_ALWAYS_INLINE_ ~RWLockRead() {
		lock.read_unlock();
	}
};

class RWLockWrite {
	RWLock &lock;

public:
	_ALWAYS_INLINE_ RWLockWrite(RWLock &p_lock) :
			lock(p_lock) {
		lock.write_lock();
	}
	_ALWAYS_INLINE_ ~RWLockWrite() {
		lock.write_unlock();
	}
};

// --- Folded from core/os/semaphore.h ---
#include "core/typedefs.h"

#ifdef THREADS_ENABLED
#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.condition_variable.h>
#include <thirdparty/mingw-std-threads/mingw.mutex.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <condition_variable>
#include <mutex>
#define THREADING_NAMESPACE std
#endif
#endif // THREADS_ENABLED

class Semaphore {
#ifdef THREADS_ENABLED
private:
	mutable THREADING_NAMESPACE::mutex mutex;
	mutable THREADING_NAMESPACE::condition_variable condition;
	mutable uint32_t count = 0; // Initialized as locked.
#ifdef DEBUG_ENABLED
	mutable uint32_t awaiters = 0;
#endif
#endif

public:
	_ALWAYS_INLINE_ void post(uint32_t p_count = 1) const {
#ifdef THREADS_ENABLED
		std::lock_guard lock(mutex);
		count += p_count;
		for (uint32_t i = 0; i < p_count; ++i) {
			condition.notify_one();
		}
#endif
	}

	_ALWAYS_INLINE_ void wait() const {
#ifdef THREADS_ENABLED
		THREADING_NAMESPACE::unique_lock lock(mutex);
#ifdef DEBUG_ENABLED
		++awaiters;
#endif
		while (!count) { // Handle spurious wake-ups.
			condition.wait(lock);
		}
		--count;
#ifdef DEBUG_ENABLED
		--awaiters;
#endif
#endif
	}

	_ALWAYS_INLINE_ bool try_wait() const {
#ifdef THREADS_ENABLED
		std::lock_guard lock(mutex);
		if (count) {
			count--;
			return true;
		} else {
			return false;
		}
#else
		return true;
#endif
	}

#if defined(THREADS_ENABLED) && defined(DEBUG_ENABLED)
	~Semaphore() {
		std::lock_guard lock(mutex);
		if (awaiters) {
			WARN_PRINT(
					"A Semaphore object is being destroyed while one or more threads are still waiting on it.\n"
					"Please call post() on it as necessary to prevent such a situation and so ensure correct cleanup.");
			new (&condition) THREADING_NAMESPACE::condition_variable();
		}
	}
#endif
};
