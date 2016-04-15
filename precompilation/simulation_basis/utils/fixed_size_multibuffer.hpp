/**
 * \file fixed_size_multibuffer.hpp
 * \brief Implements the class fixed_size_multibuffer.
 * \author Maverick Chardet
 */

#ifndef FIXED_SIZE_MULTIBUFFER_HPP_
#define FIXED_SIZE_MULTIBUFFER_HPP_

#include <vector>    // for default template argument
#include <cstdint>   // uint8_t
#include <stdexcept> // std::out_of_range exception


namespace utils {


	/**
	 * \class fixed_size_multibuffer
	 *
	 * \brief fixed_size_multibuffer is a class intended to wrap low-level
	 * pointers manipulations when a big buffer is needed to store several
	 * instances of a class or structure of type T and/or ones that inherit T.
	 *
	 * \details This class allows to store elements similarly to a vector but
	 * with some free bytes between each element. To be precise, the elements
	 * are considered to be of a size given to the constructor (or to
	 * "set_buffers_sizes" or "rebuild") which is inteded to be greated than
	 * their actual size. This is not checked and is left at the discretion of
	 * the user. When the actual size of the elements is less than the declared
	 * size, this leads to "gaps" in the underlying container.
	 *
	 * There are several ways to access the elements: by reference (const or
	 * not), by typed pointers (const or not) and by void pointers (const or
	 * not). It is also possible to get a pointer to the underlying buffer or
	 * a reference to the container (of type Container in the template) that
	 * manages it.
	 *
	 * By default, the Container is a std::vector<uint8_t>. A custom Container
	 * type must implement the required functions and declare the types used
	 * in the implementation. Moreover, the size of the managed type is intended
	 * to be 1 byte. In all other cases, the behavior is undefined.
	 *
	 */
	template <class T, class Container=std::vector<uint8_t>>
	class fixed_size_multibuffer { // Named the STL way

	public:
		// Types
		typedef Container container_type;
		typedef T value_type;
		typedef typename container_type::size_type size_type;
		typedef typename container_type::difference_type difference_type;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;


		// Constructors

		/// Constructs an empty and not yet usable multibuffer. To be able to
		/// use it, call rebuild().
		explicit fixed_size_multibuffer () :
			element_size_{0}, nb_elements_{0}
		{};

		/// Constructs an empty multibuffer with virtual size "elem_size".
		fixed_size_multibuffer (size_type elem_size) :
			element_size_{elem_size}, nb_elements_{0}
		{};

		/// Constructs a multibuffer with virtual size "elem_size" that is able
		/// to contain "nb_elem" elements.
		fixed_size_multibuffer (size_type elem_size, size_type nb_elem) :
			element_size_{elem_size}, nb_elements_{nb_elem}
		{
			container_ = Container(nb_elem*elem_size);
		};


		/// Returns the number of elements that can be stored in the buffer.
		size_type size () const {
			return nb_elements_;
		}

		/// Returns the number of bytes managed by the underlying container.
		size_type size_raw () const {
			return container_.size();
		}

		/// Returns the maximum number of elements that can possibly be stored
		/// in the buffer after a resize().
		size_type max_size () const {
			if (element_size_ == 0) return container_.max_size();
			return container_.max_size()/element_size_;
		}

		/// Changes the number of elements that can be stored in the buffer.
		/// Warning: if decreasing the size, be careful not to erase pointers
		/// that point to allocated memory before freeing it if necessary, this
		/// could lead to memory leaks.
		void resize (size_type new_size) {
			container_.resize(element_size_*new_size);
			nb_elements_ = new_size;
		}

		/// Changes the number of elements that can be stored in the buffer to
		/// 0.
		/// Warning: if decreasing the size, be careful not to erase pointers
		/// that point to allocated memory before freeing it if necessary, this
		/// could lead to memory leaks.
		void clear () {
			container_.clear();
			nb_elements_ = 0;
		}

		/// Returns the maximum size that the buffer can take without
		/// reallocation.
		size_type capacity () const {
			if (element_size_ == 0) return container_.capacity();
			return container_.capacity()/element_size_;
		}

		/// Returns the virtual size of an element.
		size_type buffers_sizes () const {
			return element_size_;
		}

		/// Sets the new virtual size of the elements. Be careful: if the new
		/// size is different, don't try to access elements stored before the
		/// the call of this function after the call.
		void set_buffers_sizes (size_type new_size) {
			container_.resize(new_size*nb_elements_);
			element_size_ = new_size;
		}

		/// Allows to change the virtual size of the elements and the number of
		/// elements at the same time.
		void rebuild (size_type elem_size, size_type nb_elem) {
			container_.resize(elem_size*nb_elem);
			element_size_ = elem_size;
			nb_elements_ = nb_elem;
		}

		/// Returns true if the container cannot contain at least one element.
		bool empty () const {
			return container_.size() < element_size_;
		}

		/// Allows to reserve some space so that a resize with a lower value
		/// will not cause any reallocation.
		void reserve (size_type new_size) {
			container_.reserve(element_size_*new_size);
		}

		/// Asks to free the unused memory. The implementation of the underlying
		/// container defines the behavior of this function.
		void shrink_to_fit () {
			container_.shrink_to_fit();
		}


		/// Gives access to the n-th element by reference of type T.
		T& operator[] (size_type n) {
			return reinterpret_cast<T&>(*pointer_to(n));
		}
		/// Gives access to the n-th element by const reference of type T.
		const T& operator[] (size_type n) const {
			return reinterpret_cast<const T&>(*pointer_to(n));
		}

		/// Gives access to the n-th element by reference of type T, checks the
		/// bounds.
		/// \throws std::out_of_range if outside the bounds.
		T& at (size_type n) {
			if (n >= nb_elements_) throw std::out_of_range("utils::fixed_size_multibuffer::at: out_of_range.");
			return reinterpret_cast<T&>(*pointer_to(n));
		}
		/// Gives access to the n-th element by const reference of type T,
		/// checks the bounds.
		/// \throws std::out_of_range if outside the bounds.
		const T& at (size_type n) const {
			if (n >= nb_elements_) throw std::out_of_range("utils::fixed_size_multibuffer::at: out_of_range.");
			return reinterpret_cast<const T&>(*pointer_to(n));
		}

		/// Gives access to the first element by reference of type T.
		/// \throws std::out_of_range if the buffer is empty.
		T& front () {
			if (nb_elements_ == 0) throw std::out_of_range("utils::fixed_size_multibuffer::front: out_of_range.");
			return reinterpret_cast<T&>(*pointer_to(0));
		}
		/// Gives access to the first element by const reference of type T.
		/// \throws std::out_of_range if the buffer is empty.
		const T& front () const {
			if (nb_elements_ == 0) throw std::out_of_range("utils::fixed_size_multibuffer::front: out_of_range.");
			return reinterpret_cast<const T&>(*pointer_to(0));
		}

		/// Gives access to the last element by reference of type T.
		/// \throws std::out_of_range if the buffer is empty.
		T& back () {
			if (nb_elements_ == 0) throw std::out_of_range("utils::fixed_size_multibuffer::front: out_of_range.");
			return reinterpret_cast<T&>(*pointer_to(nb_elements_-1));
		}
		/// Gives access to the last element by const reference of type T.
		/// \throws std::out_of_range if the buffer is empty.
		const T& back () const {
			if (nb_elements_ == 0) throw std::out_of_range("utils::fixed_size_multibuffer::front: out_of_range.");
			return reinterpret_cast<const T&>(*pointer_to(nb_elements_-1));
		}

		/// Gives access to the n-th element by pointer of type T, checks the
		/// bounds.
		/// \throws std::out_of_range if outside the bounds.
		T* pointer_to (size_type n) {
			if (n >= nb_elements_) throw std::out_of_range("utils::fixed_size_multibuffer::pointer_to: out_of_range.");
			return reinterpret_cast<T*>(container_.data()+element_size_*n);
		}
		/// Gives access to the n-th element by const pointer of type T, checks
		/// the bounds.
		/// \throws std::out_of_range if outside the bounds.
		const T* pointer_to (size_type n) const {
			if (n >= nb_elements_) throw std::out_of_range("utils::fixed_size_multibuffer::pointer_to: out_of_range.");
			return reinterpret_cast<const T*>(container_.data()+element_size_*n);
		}

		/// Gives access to the n-th element by void pointer, checks the bounds.
		/// \throws std::out_of_range if outside the bounds.
		void* void_pointer_to (size_type n) {
			if (n >= nb_elements_) throw std::out_of_range("utils::fixed_size_multibuffer::pointer_to: out_of_range.");
			return reinterpret_cast<void*>(container_.data()+element_size_*n);
		}
		/// Gives access to the n-th element by const void pointer, checks the
		/// bounds.
		/// \throws std::out_of_range if outside the bounds.
		const void* void_pointer_to (size_type n) const {
			if (n >= nb_elements_) throw std::out_of_range("utils::fixed_size_multibuffer::pointer_to: out_of_range.");
			return reinterpret_cast<const void*>(container_.data()+element_size_*n);
		}

		/// Returns a void pointer to the underlying chunk of memory managed by
		/// the container.
		void* data () {
			return container_.data();
		}
		/// Returns a const void pointer to the underlying chunk of memory
		/// managed by the container.
		const void* data () const {
			return container_.data();
		}

		/// Returns a reference to the container managing the memory.
		container_type& raw () {
			return container_;
		}
		/// Returns a const reference to the container managing the memory.
		const container_type& raw () const {
			return container_;
		}


	private:
		Container container_;
		size_type element_size_;
		size_type nb_elements_;
	};
}

#endif
