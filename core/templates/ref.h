#ifndef REF_H
#define REF_H

#include <atomic>
#include <type_traits>
#include <utility>
#include "core/os/memory.h"
#include "core/typedefs.h"

class RefCounted
{
	mutable std::atomic<uint32_t> _ref_count{0};
	mutable std::atomic<uint32_t> _ref_count_init{0};

public:
	_FORCE_INLINE_ bool reference() const
	{
		uint32_t count = _ref_count.fetch_add(1, std::memory_order_relaxed);
		return count > 0 || _ref_count_init.exchange(1, std::memory_order_acq_rel) == 0;
	}

	_FORCE_INLINE_ bool unreference() const
	{
		if (_ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
			memdelete(const_cast<RefCounted*>(this));
			return true;
		}
		return false;
	}

	_FORCE_INLINE_ uint32_t get_reference_count() const
	{
		return _ref_count.load(std::memory_order_relaxed);
	}

	RefCounted() = default;
	virtual ~RefCounted() = default;

	RefCounted(const RefCounted&) = delete;
	RefCounted& operator=(const RefCounted&) = delete;
};

template <typename T> class Ref
{
	T* data = nullptr;

	_FORCE_INLINE_ void ref(T* p_ptr)
	{
		if (p_ptr == data) {
			return;
		}
		unref();
		if (p_ptr && p_ptr->reference()) {
			data = p_ptr;
		}
	}

public:
	// --- Constructors ---
	_FORCE_INLINE_ Ref() = default;

	_FORCE_INLINE_ Ref(std::nullptr_t) {}

	_FORCE_INLINE_ Ref(T* p_ptr)
	{
		if (p_ptr) {
			ref(p_ptr);
		}
	}

	_FORCE_INLINE_ Ref(const Ref& p_from) { ref(p_from.data); }

	_FORCE_INLINE_ Ref(Ref&& p_from) noexcept : data(p_from.data) { p_from.data = nullptr; }

	// Universal converting copy constructor (Upcasts & Downcasts)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> &&
									(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>)>>
	_FORCE_INLINE_ Ref(const Ref<U>& p_from)
	{
		if constexpr (std::is_base_of_v<T, U>) {
			ref(p_from.ptr());
		}
		else if constexpr (std::is_base_of_v<U, T>) {
			ref(dynamic_cast<T*>(p_from.ptr()));
		}
	}

	// Converting move constructor (Upcast only)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ Ref(Ref<U>&& p_from) noexcept : data(p_from.data)
	{
		p_from.data = nullptr;
	}

	_FORCE_INLINE_ ~Ref() { unref(); }

	// --- Lifecycle & Mutators ---
	_FORCE_INLINE_ void unref()
	{
		if (data) {
			data->unreference();
			data = nullptr;
		}
	}

	template <typename... Args> _FORCE_INLINE_ void instantiate(Args&&... p_args)
	{
		ref(memnew(T(std::forward<Args>(p_args)...)));
	}

	// --- Assignment Operators ---
	_FORCE_INLINE_ Ref& operator=(const Ref& p_from)
	{
		if (this != &p_from) {
			ref(p_from.data);
		}
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(Ref&& p_from) noexcept
	{
		if (this != &p_from) {
			unref();
			data = p_from.data;
			p_from.data = nullptr;
		}
		return *this;
	}

	// Universal converting copy assignment (Upcasts & Downcasts)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> &&
									(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>)>>
	_FORCE_INLINE_ Ref& operator=(const Ref<U>& p_from)
	{
		if constexpr (std::is_base_of_v<T, U>) {
			ref(p_from.ptr());
		}
		else if constexpr (std::is_base_of_v<U, T>) {
			ref(dynamic_cast<T*>(p_from.ptr()));
		}
		return *this;
	}

	// Converting move assignment (Upcast only)
	template <typename U,
		typename = std::enable_if_t<!std::is_same_v<T, U> && std::is_base_of_v<T, U>>>
	_FORCE_INLINE_ Ref& operator=(Ref<U>&& p_from) noexcept
	{
		unref();
		data = p_from.data;
		p_from.data = nullptr;
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(T* p_ptr)
	{
		ref(p_ptr);
		return *this;
	}

	_FORCE_INLINE_ Ref& operator=(std::nullptr_t)
	{
		unref();
		return *this;
	}

	_FORCE_INLINE_ T* ptr() const { return data; }

	_FORCE_INLINE_ T* operator->() const { return data; }

	_FORCE_INLINE_ T& operator*() const { return *data; }

	_FORCE_INLINE_ explicit operator bool() const { return data != nullptr; }

	_FORCE_INLINE_ bool is_valid() const { return data != nullptr; }

	_FORCE_INLINE_ bool is_null() const { return data == nullptr; }

	_FORCE_INLINE_ bool operator==(const Ref& p_r) const { return data == p_r.data; }

	_FORCE_INLINE_ bool operator!=(const Ref& p_r) const { return data != p_r.data; }

	_FORCE_INLINE_ bool operator==(const T* p_ptr) const { return data == p_ptr; }

	_FORCE_INLINE_ bool operator!=(const T* p_ptr) const { return data != p_ptr; }

	_FORCE_INLINE_ bool operator<(const Ref& p_r) const { return data < p_r.data; }

	template <typename U> friend class Ref;
};

template <typename T, typename... Args> _FORCE_INLINE_ Ref<T> memnew_ref(Args&&... p_args)
{
	return Ref<T>(memnew(T(std::forward<Args>(p_args)...)));
}

#endif // REF_H


