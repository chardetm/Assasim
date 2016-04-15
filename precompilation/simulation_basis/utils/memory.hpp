/**
 * \file memory.hpp
 * \brief Implements memory-related stuff.
 * \author Maverick Chardet
 */

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include <memory>   // std::allocator
#include <utility>  // std::move
#include <cstddef>  // size_t
#include <cstdlib>  // malloc

namespace utils {
	/// Allocates the memory for type T with malloc and constructs an element of
	/// type T with arguments args for constructor. The memory has to be freed
	/// with free.
	template <class T, class... Args>
	T* malloc_construct(Args&&... args) {
		T* ptr = static_cast<T*>(malloc(sizeof(T)));
		std::allocator<T> alloc;
		alloc.construct(ptr, std::forward<Args>(args)...);
		return ptr;
	}
	
	/// Used internally.
	struct free_delete {
	    void operator()(void* x) { free(x); }
	};
	
	/// Works exactly like unique_ptr<T> but uses free to release memory.
	template <class T>
	using unique_malloc_ptr = std::unique_ptr<T,free_delete>;
	
	/// Works exactly like make_unique
	template <class T, class... Args>
	unique_malloc_ptr<T> make_unique_malloc (Args&&... args) {
		return unique_malloc_ptr<T>(malloc_construct<T>(std::forward<Args>(args)...));
	}
}

#endif