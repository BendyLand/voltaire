#ifndef MEM_UNIQUE_PTR_H
#define MEM_UNIQUE_PTR_H

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

template <typename T> using mem_unique_ptr = std::unique_ptr<T, MemDeleter<T>>;

template <typename T, typename... Args> mem_unique_ptr<T> mem_make_unique(Args&&... p_args)
{
	return mem_unique_ptr<T>(memnew(T(std::forward<Args>(p_args)...)));
}

#endif // MEM_UNIQUE_PTR_H

