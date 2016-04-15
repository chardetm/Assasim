/**
 * \file thread_safe_vector.hpp
 * \brief Implements thread safe vectors (utils::thread_safe_vector).
 * \author Maverick Chardet
 */

#ifndef THREAD_SAFE_VECTOR_HPP_
#define THREAD_SAFE_VECTOR_HPP_

#include <vector>           // for the underlying container
#include <initializer_list> // std::initializer_list
#include <memory>           // std::allocator
#include <shared_mutex>     // std::shared_mutex
#include <utility>          // std::move


namespace utils {

	/**
	 * \class thread_safe_vector
	 *
	 * \brief thread_safe_vector is a class intended to be a thread-safe
	 * interface to std::vector.
	 *
	 * \details All the methods that have the same name as the ones of
	 * std::vector act the same way (but in a thread-safe fashion), except
	 * insert, emplace and erase which act the same but take integer positions
	 * instead of iterators to describe the location in the container.
	 *
	 * The iterator-related methods have been removed because inherently not
	 * thread-safe. Likewise, "at" and "operator[]" have been replaced with the
	 * get and set methods.
	 *
	 * However, it is possible to get a reference to the underlying vector
	 * thanks to the "raw" method. It is also possible to get a pointer to the
	 * memory managed by the underlying vector thanks to the "data" method.
	 *
	 * Finally, the "unique_lock" and "shared_lock" methods allow to get a lock
	 * object to control the underlying mutex: this allows to use raw access in
	 * a safe way. This allows for instance to perform a lot of operations (ex:
	 * iterating the whole vector) without having to lock at each access. This
	 * also allows to use the underlying vector's reference-access methods.
	 *
	 */
	template <class T, class Allocator = std::allocator<T>>
	class thread_safe_vector { // Named the STL way

	public:
		// Types
		typedef std::vector<T, Allocator> vector_type;

		typedef typename vector_type::value_type value_type;
		typedef typename vector_type::allocator_type allocator_type;
		typedef typename vector_type::size_type size_type;
		typedef typename vector_type::difference_type difference_type;
		typedef typename vector_type::reference reference;
		typedef typename vector_type::const_reference const_reference;
		typedef typename vector_type::pointer pointer;
		typedef typename vector_type::const_pointer const_pointer;

		typedef std::shared_timed_mutex shared_mutex_type;
		typedef std::unique_lock<shared_mutex_type> unique_lock_type;
		typedef std::shared_lock<shared_mutex_type> shared_lock_type;

	private:
		typedef typename vector_type::const_iterator const_iterator;

	public:

		// Constructors

		/// See std::vector constructor documentation.
		thread_safe_vector () :
			vect_{}
		{

		}
		/// See std::vector constructor documentation.
		explicit thread_safe_vector (const Allocator& alloc) :
			vect_{alloc}
		{

		}

		/// See std::vector constructor documentation.
		thread_safe_vector (size_type count, const T& value, const Allocator& alloc = Allocator()) :
			vect_{count, value, alloc}
		{}

		/// See std::vector constructor documentation.
		explicit thread_safe_vector (size_type count, const Allocator& alloc = Allocator()) :
			vect_{count, alloc}
		{}

		/// See std::vector constructor documentation.
		template <class InputIt>
		thread_safe_vector (InputIt first, InputIt last, const Allocator& alloc = Allocator()) :
			vect_{first, last, alloc}
		{}

		/** See std::vector constructor documentation (shared_lock access to
		  * "other"). */
		thread_safe_vector (const thread_safe_vector& other) {
			shared_lock_type slock(other.mut_);
			vect_(other.vect_);
		}

		/** See std::vector constructor documentation (shared_lock access to
		  * "other"). */
		thread_safe_vector (const thread_safe_vector& other, const Allocator& alloc) {
			shared_lock_type slock(other.mut_);
			vect_(other.vect_, alloc);
		}

		/** See std::vector constructor documentation (shared_lock access to
		  * "other"). */
		thread_safe_vector (thread_safe_vector&& other) {
			shared_lock_type slock(other.mut_);
			vect_(std::move(other.vect_));
		}

		/** See std::vector constructor documentation (shared_lock access to
		  * "other"). */
		thread_safe_vector (thread_safe_vector&& other, const Allocator& alloc) {
			shared_lock_type slock(other.mut_);
			vect_(std::move(other.vect_), alloc);
		}

		/// See std::vector constructor documentation.
		thread_safe_vector (std::initializer_list<T> init, const Allocator& alloc = Allocator()) :
			vect_{init, alloc}
		{}


		// operator=

		/** See std::vector::operator= documentation (unique_lock access,
		    shared_lock acccess to "other"). */
		thread_safe_vector& operator= (const thread_safe_vector &other) {
			unique_lock_type ulock1(mut_, std::defer_lock);
			shared_lock_type slock2(other.mut_, std::defer_lock);
			bool first_locked = ulock1.try_lock();
			slock2.lock();
			if (!first_locked) {
				ulock1.lock();
			}
			vect_ = other.vect_;
			return *this;
		}

		/** See std::vector::operator= documentation (unique_lock access,
		  * shared_lock acccess to "other"). */
		thread_safe_vector& operator= (thread_safe_vector &&other) {
			unique_lock_type ulock1(mut_, std::defer_lock);
			shared_lock_type slock2(other.mut_, std::defer_lock);
			bool first_locked = ulock1.try_lock();
			slock2.lock();
			if (!first_locked) {
				ulock1.lock();
			}
			vect_ = std::move(other.vect_);
			return *this;
		}

		/// See std::vector::operator= documentation.
		thread_safe_vector& operator= (std::initializer_list<T> ilist) {
			unique_lock_type ulock(mut_);
			vect_ = ilist;
			return *this;
		}


		// assign

		/// See std::vector::assign documentation (unique_lock access).
		void assign (size_type count, const T& value) {
			unique_lock_type ulock(mut_);
			vect_.assign(count, value);
		}

		/// See std::vector::assign documentation (unique_lock access).
		template <class InputIt>
		void assign (InputIt first, InputIt last) {
			unique_lock_type ulock(mut_);
			vect_.assign(first, last);
		}

		/// See std::vector::assign documentation (unique_lock access).
		void assign (std::initializer_list<T> ilist) {
			unique_lock_type ulock(mut_);
			vect_.assign(ilist);
		}


		// get_allocator

		/// See std::vector::get_allocator documentation.
		allocator_type get_allocator() const {
			return vect_.get_allocator();
		}


		// Thread-safe element access

		/** Returns the pos-th element of the vector by value (shared_lock
		  * access). */
		T get (size_type pos) const {
			shared_lock_type slock(mut_);
			return vect_.at(pos);
		}

		/// Sets the pos-th element of the vector (unique_lock access).
		void set (size_type pos, const T& val) {
			unique_lock_type ulock(mut_);
			vect_.at(pos) = val;
		}

		/// Sets the pos-th element of the vector (unique_lock access).
		void set (size_type pos, T&& val) {
			unique_lock_type ulock(mut_);
			vect_.at(pos) = std::move(val);
		}


		// Capacity

		/// See std::vector::empty documentation.
		bool empty () const {
			return vect_.empty();
		}

		/// See std::vector::size documentation.
		size_type size () const {
			return vect_.size();
		}

		/// See std::vector::max_size documentation.
		size_type max_size () const {
			return vect_.max_size();
		}

		/// See std::vector::reserve documentation (unique_lock access).
		void reserve (size_type new_cap) {
			unique_lock_type ulock(mut_);
			vect_.reserve(new_cap);
		}

		/// See std::vector::capacity documentation.
		size_type capacity () const {
			return vect_.capacity();
		}

		/// See std::vector::shrink_to_fit documentation (unique_lock access).
		void shrink_to_fit () {
			unique_lock_type ulock(mut_);
			vect_.shrink_to_fit();
		}


		// Modifiers

		/// See std::vector::clear documentation (unique_lock access).
		void clear () {
			unique_lock_type ulock(mut_);
			vect_.clear();
		}

		/** Inserts value at the pos-th position, i.e: before the pos-th element
		  * (unique_lock access). */
		void insert_before (size_type pos, const T& value) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.insert(it, value);
		}
		/** Inserts value at the pos-th position, i.e: before the pos-th element
		  * (unique_lock access). */
		void insert_before (size_type pos, T&& value) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.insert(it, std::move(value));
		}
		/** Inserts count copies of value at the pos-th position, i.e: before
		  * the pos-th element (unique_lock access). */
		void insert_before (size_type pos, size_type count, const T& value) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.insert(it, count, value);
		}
		/** Inserts the elements in the range [first, last) at the pos-th
		  * position, i.e: before the pos-th element (unique_lock access). */
		template <class InputIt>
		void insert_before (size_type pos, InputIt first, InputIt last) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.insert(it, first, last);
		}
		/** Inserts the elements in ilist at the pos-th position, i.e: before
		  * the pos-th element (unique_lock access). */
		void insert_before (size_type pos, std::initializer_list<T> ilist) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.insert(it, ilist);
		}

		/** Constructs a new T with arguments args at the pos-th position, i.e:
		  * before the pos-th element (unique_lock access). */
		template <class... Args>
		void emplace_before (size_type pos, Args&&... args) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.emplace(it, std::move(args...));
		}

		/// Removes the element at the pos-th position (unique_lock access).
		void erase (size_type pos) {
			unique_lock_type ulock(mut_);
			const_iterator it = vect_.cbegin();
			it+=pos;
			vect_.erase(it);
		}

		/** Removes the elements in the range [first, last) (unique_lock
		  * access). */
		void erase (size_type first, size_type last) {
			unique_lock_type ulock(mut_);
			const_iterator it_first = vect_.cbegin();
			const_iterator it_last = it_first+last;
			it_first+=first;
			vect_.erase(first, last);
		}

		/// See std::vector::push_back documentation (unique_lock access).
		void push_back (const T& value) {
			unique_lock_type ulock(mut_);
			vect_.push_back(value);
		}

		/// See std::vector::push_back documentation (unique_lock access).
		void push_back (T&& value) {
			unique_lock_type ulock(mut_);
			vect_.push_back(std::move(value));
		}

		/// See std::vector::emplace_back documentation (unique_lock access).
		template <class... Args>
		void emplace_back (Args&&... args) {
			unique_lock_type ulock(mut_);
			vect_.emplace_back(std::move(args...));
		}

		/// See std::vector::pop_back documentation (unique_lock access).
		void pop_back () {
			unique_lock_type ulock(mut_);
			vect_.pop_back();
		}

		/// See std::vector::resize documentation (unique_lock access).
		void resize (size_type count) {
			unique_lock_type ulock(mut_);
			vect_.resize(count);
		}

		/// See std::vector::resize documentation (unique_lock access).
		void resize (size_type count, const value_type& value) {
			unique_lock_type ulock(mut_);
			vect_.resize(count, value);
		}

		/** See std::vector::swap documentation (unique_lock access and
		  * unique_lock access for "other"). */
		void swap (thread_safe_vector& other) {
			unique_lock_type ulock1(mut_, std::defer_lock);
			unique_lock_type ulock2(other.mut_, std::defer_lock);
			bool first_locked = ulock1.try_lock();
			ulock2.lock();
			if (!first_locked) {
				ulock1.lock();
			}
			vect_.swap(other.vect_);
		}


		// Raw access

		/// Returns a reference to the underlying std::vector.
		vector_type& raw () {
			return vect_;
		}

		/// Returns a const reference to the underlying std::vector.
		const vector_type& raw () const {
			return vect_;
		}

		/** Returns a unique_lock object owning the mutex of the
		  * thread_safe_vector.
		  * Warning: never use locking methods between a manual lock and a
		  * manual unlock. */
		unique_lock_type unique_lock (bool locked=true) const {;
			if (locked) return unique_lock_type(mut_);
			else return unique_lock_type(mut_, std::defer_lock);
		}

		/** Returns a shared_lock object owning the mutex of the
		  * thread_safe_vector.
		  * Warning: never use locking methods between a manual lock and a
		  * manual unlock. */
		shared_lock_type shared_lock (bool locked=true) const {;
			if (locked) return shared_lock_type(mut_);
			else return shared_lock_type(mut_, std::defer_lock);
		}

		/// See std::vector::data documentation.
		T* data() {
			return vect_.data();
		}

		/// See std::vector::data documentation.
		const T* data() const {
			return vect_.data();
		}


	private:
		vector_type vect_;
		mutable shared_mutex_type mut_;
	};

}

#endif
