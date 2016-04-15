/**
 * \file custom_heap.hpp
 * \brief Implements the class custom_heap.
 * \author Maverick Chardet
 */

#ifndef CUSTOM_HEAP_HPP_
#define CUSTOM_HEAP_HPP_

#include <cstdlib>   // malloc
#include <cstddef>   // ptrdiff_t
#include <cstdint>   // uint8_t
#include <stdexcept> // std exceptions

// TODO: Documentation

namespace utils {


	/**
	 * \class custom_heap
	 *
	 * \brief TODO: Implements a custom heap to avoid repeated malloc.
	 *
	 * \details TODO: This class implements a heap that allows to store values
	 * and free them all at a time. The memory itself is not freed and will be
	 * used for the future memory allocations. this allows to effectively call
	 * malloc very few times.
	 *
	 */
	class custom_heap { // Named the STL way

	public:
		// Types
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;


		// Constructors
		custom_heap () : data_{nullptr}, size_{0}, capacity_{0} {}

		explicit custom_heap (size_type capacity) : size_{0} {
			data_ = malloc(capacity);
			if (data_ == nullptr) throw std::runtime_error("Malloc failed");
			capacity_ = capacity;
		}

		~custom_heap () {
			if (data_ != nullptr) free(data_);
		}

		void* allocate(size_type size) {
			if (size_+size <= capacity_) {
				void* toReturn = (uint8_t*)data_+size_;
				size_ += size;
				return toReturn;
			} else {
				if (data_ == nullptr) {
					data_ = malloc(size);
					if (data_ == nullptr) throw std::runtime_error("Malloc failed");
					capacity_ = size;
					size_ = size;
					return data_;
				} else {
					size_type new_capacity = capacity_*2;
					if (size_+size > new_capacity) new_capacity = size_+size;
					data_ = realloc(data_, new_capacity);
					if (data_ == nullptr) {
						size_ = 0;
						capacity_ = 0;
						throw std::runtime_error("Realloc failed");
					}
					capacity_ = new_capacity;
					void* toReturn = (uint8_t*)data_+size_;
					size_ += size;
					return toReturn;
				}
			}
		}

		void reserve(size_type size) {
			if (size == 0 || size <= capacity_)
				return;

			if (data_ == nullptr) {
				data_ = malloc(size);
				if (data_ == nullptr) throw std::runtime_error("Malloc failed");
				capacity_ = size;

			} else {
				size_type new_capacity = capacity_*2;
				if (size > new_capacity) new_capacity = size;
				data_ = realloc(data_, new_capacity);
				if (data_ == nullptr) {
					size_ = 0;
					capacity_ = 0;
					throw std::runtime_error("Realloc failed");
				}
				capacity_ = new_capacity;
			}
		}

		void clear() {
			size_ = 0;
		}

		void shrink_to_fit() {
			if (data_ == nullptr) return;
			if (size_ == 0) {
				free(data_);
				return;
			}
			data_ = realloc(data_, size_);
			if (data_ == nullptr) {
				size_ = 0;
				capacity_ = 0;
				throw std::runtime_error("Realloc failed");
			}
			capacity_ = size_;
		}

		void* data() {
			return data_;
		}

		const void* data() const {
			return data_;
		}

		size_type size() const {
			return size_;
		}

		size_type capacity() const {
			return capacity_;
		}

	private:
		void* data_;
		size_type size_;
		size_type capacity_;
	};
}

#endif
