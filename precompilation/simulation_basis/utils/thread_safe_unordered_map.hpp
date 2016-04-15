/**
 * \file thread_safe_unordered_map.hpp
 * \brief Implements thread safe hash tables (utils::thread_safe_unordered_map).
 */

#ifndef THREAD_SAFE_UNORDERED_MAP_HPP_
#define THREAD_SAFE_UNORDERED_MAP_HPP_

#include <unordered_map>    // for the underlying container
#include <initializer_list> // std::initializer_list
#include <memory>           // std::allocator
#include <shared_mutex>     // std::shared_mutex
#include <utility>          // std::move, std::make_pair


namespace utils {

	/**
	 * \brief thread_safe_unordered_map is a class intended to be a thread-safe
	 * interface to std::unordered_map.
	 *
	 * \author Maverick Chardet
	 *
	 * \details All the methods that have the same name as the ones of
	 * std::unordered_map act the same way (but in a thread-safe fashion),
     * except insert, emplace and erase which act the same but take integer
     * positions instead of iterators to describe the location in the container.
	 *
	 * The iterator-related methods have been removed because inherently not
	 * thread-safe. Likewise, "at" and "operator[]" have been replaced with the
	 * get and set methods.
	 *
	 * However, it is possible to get a reference to the underlying
     * unordered_map thanks to the "raw" method.
	 *
	 * Finally, the "unique_lock" and "shared_lock" methods allow to get a lock
	 * object to control the underlying mutex: this allows to use raw access in
	 * a safe way. This allows for instance to perform a lot of operations (ex:
	 * iterating the whole unordered_map) without having to lock at each access.
     * This also allows to use the underlying unordered_map's reference-access
     * methods.
	 *
	 */
	template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class Allocator = std::allocator<std::pair<const Key, T>>>
	class thread_safe_unordered_map { // Named the STL way

	public:
		// Types
		typedef std::unordered_map<Key, T, Hash, KeyEqual, Allocator> unordered_map_type;

        typedef typename unordered_map_type::key_type key_type;
        typedef typename unordered_map_type::mapped_type mapped_type;
        typedef typename unordered_map_type::value_type value_type;
        typedef typename unordered_map_type::size_type size_type;
        typedef typename unordered_map_type::difference_type difference_type;
        typedef typename unordered_map_type::hasher hasher;
        typedef typename unordered_map_type::key_equal key_equal;
        typedef typename unordered_map_type::allocator_type allocator_type;
        typedef typename unordered_map_type::reference reference;
        typedef typename unordered_map_type::const_reference const_reference;
        typedef typename unordered_map_type::pointer pointer;
        typedef typename unordered_map_type::const_pointer const_pointer;

		typedef std::shared_timed_mutex shared_mutex_type;
		typedef std::unique_lock<shared_mutex_type> unique_lock_type;
		typedef std::shared_lock<shared_mutex_type> shared_lock_type;


		// Constructors

		/// See std::unordered_map constructor documentation.
		thread_safe_unordered_map () :
		map_{}
		{

		}
		/// See std::unordered_map constructor documentation.
		thread_safe_unordered_map (const Allocator& alloc) :
			map_{alloc}
		{

		}
		/// See std::unordered_map constructor documentation.
		thread_safe_unordered_map (size_type bucket_count) :
			map_{bucket_count}
		{

		}

		// TODO: Implement the other constructors
		// TODO: Implement operator=


		// get_allocator

		/// See std::unordered_map::get_allocator documentation.
		allocator_type get_allocator() const { return map_.get_allocator(); }


		// Capacity

		/// See std::unordered_map::empty documentation.
		bool empty () const {
			return map_.empty();
		}

		/// See std::unordered_map::size documentation.
		size_type size () const {
			return map_.size();
		}

		/// See std::unordered_map::max_size documentation.
		size_type max_size () const {
			return map_.max_size();
		}


		// Modifiers

		/// See std::unordered_map::clear documentation (unique_lock access).
		void clear () {
			unique_lock_type ulock(mut_);
			map_.clear();
		}

		/** TODO: doc */
		bool insert (const value_type& value) {
			unique_lock_type ulock(mut_);
			return map_.insert(value).second;
		}

		/** TODO: doc */
		template <class P>
		bool insert (P&& value) {
			unique_lock_type ulock(mut_);
			return map_.insert(value).second;
		}

		void insert (std::initializer_list<value_type> ilist) {
			unique_lock_type ulock(mut_);
			map_.insert(ilist);
		}

		/** Constructs a new T with arguments args at the pos-th position, i.e:
		  * before the pos-th element (unique_lock access). */
		template <class... Args>
		void emplace (Args&&... args) {
			unique_lock_type ulock(mut_);
			map_.emplace(std::move(args...));
		}

		/// Removes the element at the pos-th position (unique_lock access).
		void erase (const key_type& key) {
			unique_lock_type ulock(mut_);
			map_.erase(key);
		}

		/** See std::unordered_map::swap documentation (unique_lock access and
		  * unique_lock access for "other"). */
		void swap (thread_safe_unordered_map& other) {
			unique_lock_type ulock1(mut_, std::defer_lock);
			unique_lock_type ulock2(other.mut_, std::defer_lock);
			bool first_locked = ulock1.try_lock();
			ulock2.lock();
			if (!first_locked) {
				ulock1.lock();
			}
			map_.swap(other.map_);
		}


		// Thread-safe element access

		/** TODO: documentation (shared_lock
		  * access). */
		T get (const Key& key) const {
			shared_lock_type slock(mut_);
			return map_.at(key);
		}

		/// TODO: documentation (unique_lock access).
		void set (const Key& key, const T& val) {
			unique_lock_type ulock(mut_);
			auto it = map_.find(key);
			if (it != map_.end()) {
				it->second = val;
			} else {
				map_.insert(std::make_pair(key, val));
			}
		}

		//TODO: R-value reference set?


		/// See std::unordered_map::count documentation (shared_lock access).
		size_type count (const Key& key) const {
			shared_lock_type slock(mut_);
			return map_.count(key);
		}


		/// TODO documentation
		std::pair<T, bool> get_if_exists (const Key& key) const {
			std::pair<T, bool> prt;
			shared_lock_type slock(mut_);
			auto it = map_.find(key);
			if (it != map_.end()) {
				prt.first = it->second;
				prt.second = true;
			} else {
				prt.second = false;
			}
			return prt;
		}


		// TODO: Bucket interface, hash policy, observers


		// Raw access

		/// Returns a reference to the underlying std::unordered_map.
		unordered_map_type& raw () {
			return map_;
		}

		/// Returns a const reference to the underlying std::unordered_map.
		const unordered_map_type& raw () const {
			return map_;
		}

		/** Returns a unique_lock object owning the mutex of the
		  * thread_safe_unordered_map.
		  * Warning: never use locking methods between a manual lock and a
		  * manual unlock. */
		unique_lock_type unique_lock (bool locked=true) const {;
			if (locked) return unique_lock_type(mut_);
			else return unique_lock_type(mut_, std::defer_lock);
		}

		/** Returns a shared_lock object owning the mutex of the
		  * thread_safe_unordered_map.
		  * Warning: never use locking methods between a manual lock and a
		  * manual unlock. */
		shared_lock_type shared_lock (bool locked=true) const {;
			if (locked) return shared_lock_type(mut_);
			else return shared_lock_type(mut_, std::defer_lock);
		}


	private:
        unordered_map_type map_;
		mutable shared_mutex_type mut_;
	};

}

#endif
