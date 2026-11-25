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

#ifdef THREADS_ENABLED

template <typename MutexT>
class MutexLock;

template <typename StdMutexT>
class MutexImpl {
	friend class MutexLock<MutexImpl<StdMutexT>>;

	using StdMutexType = StdMutexT;

	mutable StdMutexT mutex;

public:
	_ALWAYS_INLINE_ void lock() const {
		mutex.lock();
	}

	_ALWAYS_INLINE_ void unlock() const {
		mutex.unlock();
	}

	_ALWAYS_INLINE_ bool try_lock() const {
		return mutex.try_lock();
	}
};

template <typename MutexT>
class [[nodiscard]] MutexLock {
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

	// TODO: Implement a `try_temp_relock` if needed (will also need a dummy method below).
};

using Mutex = MutexImpl<THREADING_NAMESPACE::recursive_mutex>; // Recursive, for general use
using BinaryMutex = MutexImpl<THREADING_NAMESPACE::mutex>; // Non-recursive, handle with care

extern template class MutexImpl<THREADING_NAMESPACE::recursive_mutex>;
extern template class MutexImpl<THREADING_NAMESPACE::mutex>;
extern template class MutexLock<MutexImpl<THREADING_NAMESPACE::recursive_mutex>>;
extern template class MutexLock<MutexImpl<THREADING_NAMESPACE::mutex>>;

#else // No threads.

class MutexImpl {
	mutable THREADING_NAMESPACE::mutex mutex;

public:
	void lock() const {}
	void unlock() const {}
	bool try_lock() const { return true; }
};

template <typename MutexT>
class [[nodiscard]] MutexLock {
public:
	MutexLock(const MutexT &p_mutex) {}

	void temp_relock() const {}
	void temp_unlock() const {}
};

using Mutex = MutexImpl;
using BinaryMutex = MutexImpl;

#endif // THREADS_ENABLED

// --- Folded from core/os/spin_lock.h ---
// --- Folded from core/os/thread.h ---
#include "platform/linuxbsd/platform_config.h" // IWYU pragma: keep. Can override the implementation.

// Define PLATFORM_THREAD_OVERRIDE in your platform's `platform_config.h`
// to use a custom Thread implementation defined in `platform/[your_platform]/platform_thread.h`.
// Overriding the Thread implementation is required in some proprietary platforms.

#ifdef PLATFORM_THREAD_OVERRIDE

// --- Folded from drivers/apple/thread_apple.h ---
#include "core/os/memory.h"
#include "core/typedefs.h"

#include <pthread.h>

#include <new> // For hardware interference size

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

#if defined(__cpp_lib_hardware_interference_size)
	VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Winterference-size")
	static constexpr size_t CACHE_LINE_BYTES = std::hardware_destructive_interference_size;
	VLTR_GCC_WARNING_POP
#else
	// At a negligible memory cost, we use a conservatively high value.
	static constexpr size_t CACHE_LINE_BYTES = 128;
#endif

private:
	friend class Main;

	ID id = UNASSIGNED_ID;
	pthread_t pthread;

	static inline SafeFlag is_main_thread_assigned{ false };
	static SafeNumeric<uint64_t> id_counter;
	static thread_local ID caller_id;

	static void *thread_callback(void *p_data);

public:
	_FORCE_INLINE_ static void yield() { pthread_yield_np(); }

	_FORCE_INLINE_ ID get_id() const { return id; }
	// get the ID of the caller thread
	_FORCE_INLINE_ static ID get_caller_id() {
		return caller_id;
	}
	// get the ID of the main thread
	_FORCE_INLINE_ static ID get_main_id() { return MAIN_ID; }

	_FORCE_INLINE_ static bool is_main_thread() { return caller_id == MAIN_ID; }
	static void make_main_thread();
	static void release_main_thread();

	static Error set_name(const String &p_name);

	ID start(Thread::Callback p_callback, void *p_user, const Settings &p_settings = Settings());
	bool is_started() const { return id != UNASSIGNED_ID; }
	/// Waits until thread is finished, and deallocates it.
	void wait_to_finish();

	Thread() = default;
	~Thread();
};
// --- End drivers/apple/thread_apple.h ---

#else

#include "core/typedefs.h"

#ifdef THREADS_ENABLED

#include "core/os/memory.h"

#include <new> // IWYU pragma: keep // For hardware interference size.

#ifdef MINGW_ENABLED
#define MINGW_STDTHREAD_REDUNDANCY_WARNING
#include <thirdparty/mingw-std-threads/mingw.thread.h>
#define THREADING_NAMESPACE mingw_stdthread
#else
#include <thread>
#define THREADING_NAMESPACE std
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

	struct PlatformFunctions {
		Error (*set_name)(const String &) = nullptr;
		void (*set_priority)(Thread::Priority) = nullptr;
		void (*init)() = nullptr;
		void (*wrapper)(Thread::Callback, void *) = nullptr;
		void (*term)() = nullptr;
	};

#if defined(__cpp_lib_hardware_interference_size) && !defined(ANDROID_ENABLED) // This would be OK with NDK >= 26.
	VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Winterference-size")
	static constexpr size_t CACHE_LINE_BYTES = std::hardware_destructive_interference_size;
	VLTR_GCC_WARNING_POP
#elif defined(__GCC_DESTRUCTIVE_SIZE) && !defined(ANDROID_ENABLED)
	static constexpr size_t CACHE_LINE_BYTES = __GCC_DESTRUCTIVE_SIZE;
#else
	// At a negligible memory cost, we use a conservatively high value.
	static constexpr size_t CACHE_LINE_BYTES = 128;
#endif

private:
	friend class Main;

	static PlatformFunctions platform_functions;

	ID id = UNASSIGNED_ID;

	static inline SafeFlag is_main_thread_assigned{ false };
	static SafeNumeric<uint64_t> id_counter;
	static thread_local ID caller_id;
	THREADING_NAMESPACE::thread thread;

	static void callback(ID p_caller_id, const Settings &p_settings, Thread::Callback p_callback, void *p_userdata);

public:
	static void _set_platform_functions(const PlatformFunctions &p_functions);

	_FORCE_INLINE_ static void yield() { std::this_thread::yield(); }

	_FORCE_INLINE_ ID get_id() const { return id; }
	// get the ID of the caller thread
	_FORCE_INLINE_ static ID get_caller_id() {
		return caller_id;
	}
	// get the ID of the main thread
	_FORCE_INLINE_ static ID get_main_id() { return MAIN_ID; }

	_FORCE_INLINE_ static bool is_main_thread() { return caller_id == MAIN_ID; } // Gain a tiny bit of perf here because there is no need to validate caller_id here, because only main thread will be set as 1.
	static void make_main_thread();
	static void release_main_thread();

	static Error set_name(const String &p_name);

	ID start(Thread::Callback p_callback, void *p_user, const Settings &p_settings = Settings());
	bool is_started() const;
	///< waits until thread is finished, and deallocates it.
	void wait_to_finish();

	~Thread();
};

#else // No threads.

class String;

class Thread {
public:
	typedef void (*Callback)(void *p_userdata);

	typedef uint64_t ID;

	static constexpr size_t CACHE_LINE_BYTES = sizeof(void *);

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

	struct PlatformFunctions {
		Error (*set_name)(const String &) = nullptr;
		void (*set_priority)(Thread::Priority) = nullptr;
		void (*init)() = nullptr;
		void (*wrapper)(Thread::Callback, void *) = nullptr;
		void (*term)() = nullptr;
	};

private:
	friend class Main;

	static PlatformFunctions platform_functions;

public:
	static void _set_platform_functions(const PlatformFunctions &p_functions);

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
};

#endif // THREADS_ENABLED

#endif // PLATFORM_THREAD_OVERRIDE
// --- End core/os/thread.h ---
#include "core/typedefs.h"

#ifdef THREADS_ENABLED

// Note the implementations below avoid false sharing by ensuring their
// sizes match the assumed cache line. We can't use align attributes
// because these objects may end up unaligned in semi-tightly packed arrays.

#ifdef _MSC_VER
#include <intrin.h>
#endif

#if defined(__APPLE__)

#include <os/lock.h>

class SpinLock {
	union {
		mutable os_unfair_lock _lock = OS_UNFAIR_LOCK_INIT;
		char aligner[Thread::CACHE_LINE_BYTES];
	};

public:
	_ALWAYS_INLINE_ void lock() const {
		os_unfair_lock_lock(&_lock);
	}

	_ALWAYS_INLINE_ void unlock() const {
		os_unfair_lock_unlock(&_lock);
	}
};

#else // __APPLE__

#include <atomic>

_ALWAYS_INLINE_ static void _cpu_pause() {
#if defined(_MSC_VER)
// ----- MSVC.
#if defined(_M_ARM) || defined(_M_ARM64) // ARM.
	__yield();
#elif defined(_M_IX86) || defined(_M_X64) // x86.
	_mm_pause();
#endif
#elif defined(__GNUC__) || defined(__clang__)
// ----- GCC/Clang.
#if defined(__i386__) || defined(__x86_64__) // x86.
	__builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__) // ARM.
	asm volatile("yield");
#elif defined(__powerpc__) // PowerPC.
	asm volatile("or 27,27,27");
#elif defined(__riscv) // RISC-V.
	asm volatile(".insn i 0x0F, 0, x0, x0, 0x010");
#endif
#endif
}

static_assert(std::atomic_bool::is_always_lock_free);

class SpinLock {
	union {
#if __cplusplus >= 202002L
		mutable std::atomic<bool> locked = false;
#else
		mutable std::atomic<bool> locked = ATOMIC_VAR_INIT(false);
#endif
		char aligner[Thread::CACHE_LINE_BYTES];
	};

public:
	_ALWAYS_INLINE_ void lock() const {
		while (true) {
			bool expected = false;
			if (locked.compare_exchange_weak(expected, true, std::memory_order_acquire, std::memory_order_relaxed)) {
				break;
			}
			do {
				_cpu_pause();
			} while (locked.load(std::memory_order_relaxed));
		}
	}

	_ALWAYS_INLINE_ void unlock() const {
		locked.store(false, std::memory_order_release);
	}
};

#endif // __APPLE__

#else // THREADS_ENABLED

class SpinLock {
public:
	void lock() const {}
	void unlock() const {}
};

#endif // THREADS_ENABLED

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

#ifdef DEBUG_ENABLED
#include "core/typedefs.h"
#endif

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

class Semaphore {
private:
	mutable THREADING_NAMESPACE::mutex mutex;
	mutable THREADING_NAMESPACE::condition_variable condition;
	mutable uint32_t count = 0; // Initialized as locked.
#ifdef DEBUG_ENABLED
	mutable uint32_t awaiters = 0;
#endif

public:
	_ALWAYS_INLINE_ void post(uint32_t p_count = 1) const {
		std::lock_guard lock(mutex);
		count += p_count;
		for (uint32_t i = 0; i < p_count; ++i) {
			condition.notify_one();
		}
	}

	_ALWAYS_INLINE_ void wait() const {
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
	}

	_ALWAYS_INLINE_ bool try_wait() const {
		std::lock_guard lock(mutex);
		if (count) {
			count--;
			return true;
		} else {
			return false;
		}
	}

#ifdef DEBUG_ENABLED
	~Semaphore() {
		// Destroying an std::condition_variable when not all threads waiting on it have been notified
		// invokes undefined behavior (e.g., it may be nicely destroyed or it may be awaited forever.)
		// That means other threads could still be running the body of std::condition_variable::wait()
		// but already past the safety checkpoint. That's the case for instance if that function is already
		// waiting to lock again.
		//
		// We will make the rule a bit more restrictive and simpler to understand at the same time: there
		// should not be any threads at any stage of the waiting by the time the semaphore is destroyed.
		//
		// We do so because of the following reasons:
		// - We have the guideline that threads must be awaited (i.e., completed), so the waiting thread
		//   must be completely done by the time the thread controlling it finally destroys the semaphore.
		//   Therefore, only a coding mistake could make the program run into such a attempt at premature
		//   destruction of the semaphore.
		// - In scripting, given that Semaphores are wrapped by RefCounted classes, in general it can't
		//   happen that a thread is trying to destroy a Semaphore while another is still doing whatever with
		//   it, so the simplification is mostly transparent to script writers.
		// - The redefined rule can be checked for failure to meet it, which is what this implementation does.
		//   This is useful to detect a few cases of potential misuse; namely:
		//   a) In scripting:
		//      * The coder is naughtily dealing with the reference count causing a semaphore to die prematurely.
		//      * The coder is letting the project reach its termination without having cleanly finished threads
		//        that await on semaphores (or at least, let the usual semaphore-controlled loop exit).
		//   b) In the native side, where Semaphore is not a ref-counted beast and certain coding mistakes can
		//      lead to its premature destruction as well.
		//
		// Let's let users know they are doing it wrong, but apply a, somewhat hacky, countermeasure against UB
		// in debug builds.
		std::lock_guard lock(mutex);
		if (awaiters) {
			WARN_PRINT(
					"A Semaphore object is being destroyed while one or more threads are still waiting on it.\n"
					"Please call post() on it as necessary to prevent such a situation and so ensure correct cleanup.");
			// And now, the hacky countermeasure (i.e., leak the condition variable).
			new (&condition) THREADING_NAMESPACE::condition_variable();
		}
	}
#endif
};

#else // No threads.

class Semaphore {
public:
	void post(uint32_t p_count = 1) const {}
	void wait() const {}
	bool try_wait() const {
		return true;
	}
};

#endif // THREADS_ENABLED
