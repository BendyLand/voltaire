#ifndef MEM_SHARED_PTR_H
#define MEM_SHARED_PTR_H

#include <memory>
#include <utility>
#include "core/os/memory.h"

template <typename T> struct MemDeleter
{
	void operator()(T* p) const
	{
		if (p) {
			memdelete(p);
		}
	}
};

template <typename T> using mem_shared_ptr = std::shared_ptr<T>;

template <typename T, typename... Args> mem_shared_ptr<T> mem_make_shared(Args&&... p_args)
{
	return mem_shared_ptr<T>(memnew(T(std::forward<Args>(p_args)...)), MemDeleter<T>());
}

#endif // MEM_SHARED_PTR_H


