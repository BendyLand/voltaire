/**************************************************************************/
/*  hash_map.h                                                            */
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

#include <initializer_list>
#include "core/os/memory.h"
#include "core/templates/span.h" // IWYU pragma: keep. `WARN_VERBOSE` macro.
#include "core/templates/hashfuncs.h"
#include "core/templates/list.h"

/**
 * A HashMap implementation that uses open addressing with Robin Hood hashing.
 * Robin Hood hashing swaps out entries that have a smaller probing distance
 * than the to-be-inserted entry, that evens out the average probing distance
 * and enables faster lookups. Backward shift deletion is employed to further
 * improve the performance and to avoid infinite loops in rare cases.
 *
 * Keys and values are stored in a double linked list by insertion order. This
 * has a slight performance overhead on lookup, which can be mostly compensated
 * using a paged allocator if required.
 *
 * The assignment operator copy the pairs from one map to the other.
 */

template <typename TKey, typename TValue> struct HashMapElement
{
	HashMapElement* next = nullptr;
	HashMapElement* prev = nullptr;
	KeyValue<TKey, TValue> data;

	HashMapElement() {}

	HashMapElement(const TKey& p_key, const TValue& p_value) : data(p_key, p_value) {}
};

template <typename TKey, typename TValue, typename Hasher = HashMapHasherDefault,
	typename Comparator = HashMapComparatorDefault<TKey>,
	typename Allocator = DefaultTypedAllocator<HashMapElement<TKey, TValue>>>
class _WARN_UNUSED_ HashMap : private Allocator
{
public:
	static constexpr uint32_t MIN_CAPACITY_INDEX = 2; // Use a prime.
	static constexpr float MAX_OCCUPANCY = 0.75;
	static constexpr uint32_t EMPTY_HASH = 0;
	using KV = KeyValue<TKey, TValue>; // Type alias for easier access to KeyValue.

private:
	HashMapElement<TKey, TValue>** _elements = nullptr;
	uint32_t* _hashes = nullptr;
	HashMapElement<TKey, TValue>* _head_element = nullptr;
	HashMapElement<TKey, TValue>* _tail_element = nullptr;

	uint32_t _capacity_idx = 0;
	uint32_t _size = 0;

	_FORCE_INLINE_ static uint32_t _hash(const TKey& p_key)
	{
		uint32_t hash = Hasher::hash(p_key);

		if (unlikely(hash == EMPTY_HASH)) {
			hash = EMPTY_HASH + 1;
		}

		return hash;
	}

	_FORCE_INLINE_ static constexpr void _increment_mod(uint32_t& r_idx, const uint32_t p_capacity)
	{
		r_idx++;
		// `if` is faster than both fastmod and mod.
		if (unlikely(r_idx == p_capacity)) {
			r_idx = 0;
		}
	}

	static _FORCE_INLINE_ uint32_t _get_probe_length(const uint32_t p_idx, const uint32_t p_hash,
		const uint32_t p_capacity, const uint64_t p_capacity_inv)
	{
		const uint32_t original_idx = fastmod(p_hash, p_capacity_inv, p_capacity);
		const uint32_t distance_idx = p_idx - original_idx + p_capacity;
		// At most p_capacity over 0, so we can use an if (faster than fastmod).
		return distance_idx >= p_capacity ? distance_idx - p_capacity : distance_idx;
	}

	bool _lookup_idx(const TKey& p_key, uint32_t& r_idx) const
	{
		return _elements != nullptr && _size > 0 &&
			   _lookup_idx_unchecked(p_key, _hash(p_key), r_idx);
	}

	/// Note: Assumes that _elements != nullptr
	bool _lookup_idx_unchecked(const TKey& p_key, uint32_t p_hash, uint32_t& r_idx) const
	{
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t idx = fastmod(p_hash, capacity_inv, capacity);
		uint32_t distance = 0;

		while (true) {
			if (_hashes[idx] == EMPTY_HASH) {
				return false;
			}

			if (distance > _get_probe_length(idx, _hashes[idx], capacity, capacity_inv)) {
				return false;
			}

			if (_hashes[idx] == p_hash && Comparator::compare(_elements[idx]->data.key, p_key)) {
				r_idx = idx;
				return true;
			}

			_increment_mod(idx, capacity);
			distance++;
		}
	}

	void _insert_element(uint32_t p_hash, HashMapElement<TKey, TValue>* p_value)
	{
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t hash = p_hash;
		HashMapElement<TKey, TValue>* value = p_value;
		uint32_t distance = 0;
		uint32_t idx = fastmod(hash, capacity_inv, capacity);

		while (true) {
			if (_hashes[idx] == EMPTY_HASH) {
				_elements[idx] = value;
				_hashes[idx] = hash;

				_size++;

				return;
			}

			// Not an empty slot, let's check the probing length of the existing one.
			uint32_t existing_probe_len =
				_get_probe_length(idx, _hashes[idx], capacity, capacity_inv);
			if (existing_probe_len < distance) {
				SWAP(hash, _hashes[idx]);
				SWAP(value, _elements[idx]);
				distance = existing_probe_len;
			}

			_increment_mod(idx, capacity);
			distance++;
		}
	}

	void _resize_and_rehash(uint32_t p_new_capacity_idx)
	{
		uint32_t old_capacity = hash_table_size_primes[_capacity_idx];

		// Capacity can't be 0.
		_capacity_idx = MAX((uint32_t)MIN_CAPACITY_INDEX, p_new_capacity_idx);

		uint32_t capacity = hash_table_size_primes[_capacity_idx];

		HashMapElement<TKey, TValue>** old_elements = _elements;
		uint32_t* old_hashes = _hashes;

		_size = 0;
		static_assert(EMPTY_HASH == 0, "Assuming EMPTY_HASH = 0 for alloc_static_zeroed call");

		_hashes =
			reinterpret_cast<uint32_t*>(Memory::alloc_static_zeroed(sizeof(uint32_t) * capacity));
		_elements = reinterpret_cast<HashMapElement<TKey, TValue>**>(
			Memory::alloc_static(sizeof(HashMapElement<TKey, TValue>*) * capacity));

		if (old_capacity == 0) {
			// Nothing to do.
			return;
		}

		for (uint32_t i = 0; i < old_capacity; i++) {
			if (old_hashes[i] == EMPTY_HASH) {
				continue;
			}

			_insert_element(old_hashes[i], old_elements[i]);
		}

		Memory::free_static(old_elements);
		Memory::free_static(old_hashes);
	}

	_FORCE_INLINE_ HashMapElement<TKey, TValue>* _insert(
		const TKey& p_key, const TValue& p_value, uint32_t p_hash, bool p_front_insert = false)
	{
		uint32_t capacity = hash_table_size_primes[_capacity_idx];
		if (unlikely(_elements == nullptr)) {
			// Allocate on demand to save memory.

			static_assert(EMPTY_HASH == 0, "Assuming EMPTY_HASH = 0 for alloc_static_zeroed call");
			_hashes = reinterpret_cast<uint32_t*>(
				Memory::alloc_static_zeroed(sizeof(uint32_t) * capacity));
			_elements = reinterpret_cast<HashMapElement<TKey, TValue>**>(
				Memory::alloc_static(sizeof(HashMapElement<TKey, TValue>*) * capacity));
		}

		if (_size + 1 > MAX_OCCUPANCY * capacity) {
			ERR_FAIL_COND_V_MSG(_capacity_idx + 1 == HASH_TABLE_SIZE_MAX, nullptr,
				"Hash table maximum capacity reached, aborting insertion.");
			_resize_and_rehash(_capacity_idx + 1);
		}

		HashMapElement<TKey, TValue>* elem =
			Allocator::new_allocation(HashMapElement<TKey, TValue>(p_key, p_value));

		if (_tail_element == nullptr) {
			_head_element = elem;
			_tail_element = elem;
		}
		else if (p_front_insert) {
			_head_element->prev = elem;
			elem->next = _head_element;
			_head_element = elem;
		}
		else {
			_tail_element->next = elem;
			elem->prev = _tail_element;
			_tail_element = elem;
		}

		_insert_element(p_hash, elem);
		return elem;
	}

	void _clear_data()
	{
		HashMapElement<TKey, TValue>* current = _tail_element;
		while (current != nullptr) {
			HashMapElement<TKey, TValue>* prev = current->prev;
			Allocator::delete_allocation(current);
			current = prev;
		}
	}

public:
	_FORCE_INLINE_ uint32_t get_capacity() const { return hash_table_size_primes[_capacity_idx]; }

	_FORCE_INLINE_ uint32_t size() const { return _size; }

	/* Standard Godot Container API */

	bool is_empty() const { return _size == 0; }

	void clear()
	{
		if (_elements == nullptr || _size == 0) {
			return;
		}

		_clear_data();
		memset(_hashes, EMPTY_HASH, get_capacity() * sizeof(uint32_t));

		_tail_element = nullptr;
		_head_element = nullptr;
		_size = 0;
	}

	void sort() { sort_custom<KeyValueSort<TKey, TValue>>(); }

	template <typename C> void sort_custom()
	{
		if (size() < 2) {
			return;
		}

		using E = HashMapElement<TKey, TValue>;
		SortList<E, KeyValue<TKey, TValue>, &E::data, &E::prev, &E::next, C> sorter;
		sorter.sort(_head_element, _tail_element);
	}

	TValue& get(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		CRASH_COND_MSG(!exists, "HashMap key not found.");
		return _elements[idx]->data.value;
	}

	const TValue& get(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		CRASH_COND_MSG(!exists, "HashMap key not found.");
		return _elements[idx]->data.value;
	}

	const TValue* getptr(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);

		if (exists) {
			return &_elements[idx]->data.value;
		}
		return nullptr;
	}

	TValue* getptr(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);

		if (exists) {
			return &_elements[idx]->data.value;
		}
		return nullptr;
	}

	_FORCE_INLINE_ bool has(const TKey& p_key) const
	{
		uint32_t _idx = 0;
		return _lookup_idx(p_key, _idx);
	}

	bool erase(const TKey& p_key)
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);

		if (!exists) {
			return false;
		}

		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t next_idx = fastmod((idx + 1), capacity_inv, capacity);
		while (_hashes[next_idx] != EMPTY_HASH &&
			   _get_probe_length(next_idx, _hashes[next_idx], capacity, capacity_inv) != 0) {
			SWAP(_hashes[next_idx], _hashes[idx]);
			SWAP(_elements[next_idx], _elements[idx]);
			idx = next_idx;
			_increment_mod(next_idx, capacity);
		}

		_hashes[idx] = EMPTY_HASH;

		if (_head_element == _elements[idx]) {
			_head_element = _elements[idx]->next;
		}

		if (_tail_element == _elements[idx]) {
			_tail_element = _elements[idx]->prev;
		}

		if (_elements[idx]->prev) {
			_elements[idx]->prev->next = _elements[idx]->next;
		}

		if (_elements[idx]->next) {
			_elements[idx]->next->prev = _elements[idx]->prev;
		}

		Allocator::delete_allocation(_elements[idx]);

		_size--;
		return true;
	}

	// Replace the key of an entry in-place, without invalidating iterators or changing the entries
	// position during iteration. p_old_key must exist in the map and p_new_key must not, unless it
	// is equal to p_old_key.
	bool replace_key(const TKey& p_old_key, const TKey& p_new_key)
	{
		ERR_FAIL_COND_V(_elements == nullptr || _size == 0, false);
		if (p_old_key == p_new_key) {
			return true;
		}
		const uint32_t new_hash = _hash(p_new_key);
		uint32_t idx = 0;
		ERR_FAIL_COND_V(_lookup_idx_unchecked(p_new_key, new_hash, idx), false);
		ERR_FAIL_COND_V(!_lookup_idx(p_old_key, idx), false);
		HashMapElement<TKey, TValue>* element = _elements[idx];

		// Delete the old entries in _hashes and _elements.
		const uint32_t capacity = hash_table_size_primes[_capacity_idx];
		const uint64_t capacity_inv = hash_table_size_primes_inv[_capacity_idx];
		uint32_t next_idx = fastmod((idx + 1), capacity_inv, capacity);
		while (_hashes[next_idx] != EMPTY_HASH &&
			   _get_probe_length(next_idx, _hashes[next_idx], capacity, capacity_inv) != 0) {
			SWAP(_hashes[next_idx], _hashes[idx]);
			SWAP(_elements[next_idx], _elements[idx]);
			idx = next_idx;
			_increment_mod(next_idx, capacity);
		}

		_hashes[idx] = EMPTY_HASH;

		// _insert_element will increment this again.
		_size--;

		// Update the HashMapElement with the new key and reinsert it.
		const_cast<TKey&>(element->data.key) = p_new_key;
		_insert_element(new_hash, element);

		return true;
	}

	// Reserves space for a number of elements, useful to avoid many resizes and rehashes.
	// If adding a known (possibly large) number of elements at once, must be larger than old
	// capacity.
	void reserve(uint32_t p_new_capacity)
	{
		uint32_t new_idx = _capacity_idx;

		while (hash_table_size_primes[new_idx] < p_new_capacity) {
			ERR_FAIL_COND_MSG(new_idx + 1 == (uint32_t)HASH_TABLE_SIZE_MAX, nullptr);
			new_idx++;
		}

		if (new_idx == _capacity_idx) {
			if (p_new_capacity < _size) {
				WARN_VERBOSE("reserve() called with a capacity smaller than the current size. This "
							 "is likely a mistake.");
			}
			return;
		}

		if (_elements == nullptr) {
			_capacity_idx = new_idx;
			return; // Unallocated yet.
		}
		_resize_and_rehash(new_idx);
	}

	/** Iterator API **/

	struct ConstIterator
	{
		_FORCE_INLINE_ const KeyValue<TKey, TValue>& operator*() const { return E->data; }

		_FORCE_INLINE_ const KeyValue<TKey, TValue>* operator->() const { return &E->data; }

		_FORCE_INLINE_ ConstIterator& operator++()
		{
			if (E) {
				E = E->next;
			}
			return *this;
		}

		_FORCE_INLINE_ ConstIterator& operator--()
		{
			if (E) {
				E = E->prev;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator& p_other) const
		{
			return E == p_other.E;
		}

		_FORCE_INLINE_ bool operator!=(const ConstIterator& p_other) const
		{
			return E != p_other.E;
		}

		_FORCE_INLINE_ explicit operator bool() const { return E != nullptr; }

		_FORCE_INLINE_ ConstIterator(const HashMapElement<TKey, TValue>* p_element)
		{
			E = p_element;
		}

		_FORCE_INLINE_ ConstIterator() {}

		_FORCE_INLINE_ ConstIterator(const ConstIterator& p_it) { E = p_it.E; }

		_FORCE_INLINE_ void operator=(const ConstIterator& p_it) { E = p_it.E; }

	private:
		const HashMapElement<TKey, TValue>* E = nullptr;
	};

	struct Iterator
	{
		_FORCE_INLINE_ KeyValue<TKey, TValue>& operator*() const { return E->data; }

		_FORCE_INLINE_ KeyValue<TKey, TValue>* operator->() const { return &E->data; }

		_FORCE_INLINE_ Iterator& operator++()
		{
			if (E) {
				E = E->next;
			}
			return *this;
		}

		_FORCE_INLINE_ Iterator& operator--()
		{
			if (E) {
				E = E->prev;
			}
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator& p_other) const { return E == p_other.E; }

		_FORCE_INLINE_ bool operator!=(const Iterator& p_other) const { return E != p_other.E; }

		_FORCE_INLINE_ explicit operator bool() const { return E != nullptr; }

		_FORCE_INLINE_ Iterator(HashMapElement<TKey, TValue>* p_element) { E = p_element; }

		_FORCE_INLINE_ Iterator() {}

		_FORCE_INLINE_ Iterator(const Iterator& p_it) { E = p_it.E; }

		_FORCE_INLINE_ void operator=(const Iterator& p_it) { E = p_it.E; }

		operator ConstIterator() const { return ConstIterator(E); }

	private:
		HashMapElement<TKey, TValue>* E = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() _LIFETIME_BOUND_ { return Iterator(_head_element); }

	_FORCE_INLINE_ Iterator end() _LIFETIME_BOUND_ { return Iterator(nullptr); }

	_FORCE_INLINE_ Iterator last() _LIFETIME_BOUND_ { return Iterator(_tail_element); }

	_FORCE_INLINE_ Iterator find(const TKey& p_key) _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		if (!exists) {
			return end();
		}
		return Iterator(_elements[idx]);
	}

	_FORCE_INLINE_ void remove(const Iterator& p_iter)
	{
		if (p_iter) {
			erase(p_iter->key);
		}
	}

	_FORCE_INLINE_ ConstIterator begin() const _LIFETIME_BOUND_
	{
		return ConstIterator(_head_element);
	}

	_FORCE_INLINE_ ConstIterator end() const _LIFETIME_BOUND_ { return ConstIterator(nullptr); }

	_FORCE_INLINE_ ConstIterator last() const _LIFETIME_BOUND_
	{
		return ConstIterator(_tail_element);
	}

	_FORCE_INLINE_ ConstIterator find(const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		if (!exists) {
			return end();
		}
		return ConstIterator(_elements[idx]);
	}

	/* Indexing */

	const TValue& operator[](const TKey& p_key) const _LIFETIME_BOUND_
	{
		uint32_t idx = 0;
		bool exists = _lookup_idx(p_key, idx);
		CRASH_COND(!exists);
		return _elements[idx]->data.value;
	}

	TValue& operator[](const TKey& p_key) _LIFETIME_BOUND_
	{
		const uint32_t hash = _hash(p_key);
		uint32_t idx = 0;
		bool exists = _elements && _size > 0 && _lookup_idx_unchecked(p_key, hash, idx);
		if (!exists) {
			return _insert(p_key, TValue(), hash)->data.value;
		}
		else {
			return _elements[idx]->data.value;
		}
	}

	/* Insert */

	Iterator insert(const TKey& p_key, const TValue& p_value, bool p_front_insert = false)
	{
		const uint32_t hash = _hash(p_key);
		uint32_t idx = 0;
		bool exists = _elements && _size > 0 && _lookup_idx_unchecked(p_key, hash, idx);
		if (!exists) {
			return Iterator(_insert(p_key, p_value, hash, p_front_insert));
		}
		else {
			_elements[idx]->data.value = p_value;
			return Iterator(_elements[idx]);
		}
	}

	/* Constructors */

	explicit HashMap(const HashMap& p_other)
	{
		reserve(hash_table_size_primes[p_other._capacity_idx]);

		if (p_other._size == 0) {
			return;
		}

		for (const KeyValue<TKey, TValue>& E : p_other) {
			insert(E.key, E.value);
		}
	}

	HashMap(HashMap&& p_other)
	{
		_elements = p_other._elements;
		_hashes = p_other._hashes;
		_head_element = p_other._head_element;
		_tail_element = p_other._tail_element;
		_capacity_idx = p_other._capacity_idx;
		_size = p_other._size;

		p_other._elements = nullptr;
		p_other._hashes = nullptr;
		p_other._head_element = nullptr;
		p_other._tail_element = nullptr;
		p_other._capacity_idx = MIN_CAPACITY_INDEX;
		p_other._size = 0;
	}

	void operator=(const HashMap& p_other)
	{
		if (this == &p_other) {
			return; // Ignore self assignment.
		}
		if (_size != 0) {
			clear();
		}

		reserve(hash_table_size_primes[p_other._capacity_idx]);

		if (p_other._elements == nullptr) {
			return; // Nothing to copy.
		}

		for (const KeyValue<TKey, TValue>& E : p_other) {
			insert(E.key, E.value);
		}
	}

	HashMap& operator=(HashMap&& p_other)
	{
		if (this == &p_other) {
			return *this;
		}

		if (_size != 0) {
			clear();
		}
		if (_elements != nullptr) {
			Memory::free_static(_elements);
			Memory::free_static(_hashes);
		}

		_elements = p_other._elements;
		_hashes = p_other._hashes;
		_head_element = p_other._head_element;
		_tail_element = p_other._tail_element;
		_capacity_idx = p_other._capacity_idx;
		_size = p_other._size;

		p_other._elements = nullptr;
		p_other._hashes = nullptr;
		p_other._head_element = nullptr;
		p_other._tail_element = nullptr;
		p_other._capacity_idx = MIN_CAPACITY_INDEX;
		p_other._size = 0;

		return *this;
	}

	HashMap(uint32_t p_initial_capacity)
	{
		// Capacity can't be 0.
		_capacity_idx = 0;
		reserve(p_initial_capacity);
	}

	HashMap() { _capacity_idx = MIN_CAPACITY_INDEX; }

	HashMap(std::initializer_list<KeyValue<TKey, TValue>> p_init)
	{
		reserve(p_init.size());
		for (const KeyValue<TKey, TValue>& E : p_init) {
			insert(E.key, E.value);
		}
	}

	uint32_t debug_get_hash(uint32_t p_idx)
	{
		if (_size == 0) {
			return 0;
		}
		ERR_FAIL_INDEX_V(p_idx, get_capacity(), 0);
		return _hashes[p_idx];
	}

	Iterator debug_get_element(uint32_t p_idx)
	{
		if (_size == 0) {
			return Iterator();
		}
		ERR_FAIL_INDEX_V(p_idx, get_capacity(), Iterator());
		return Iterator(_elements[p_idx]);
	}

	~HashMap()
	{
		_clear_data();

		if (_elements != nullptr) {
			Memory::free_static(_elements);
			Memory::free_static(_hashes);
		}
	}
};

// --- Folded from core/templates/lru.h ---
#include "core/templates/list.h"

template <typename TKey, typename TData, typename Hasher = HashMapHasherDefault, typename Comparator = HashMapComparatorDefault<TKey>, void (*BeforeEvict)(TKey &, TData &) = nullptr>
class _WARN_UNUSED_ LRUCache {
public:
	struct Pair {
		TKey key;
		TData data;

		Pair() {}
		Pair(const TKey &p_key, const TData &p_data) :
				key(p_key),
				data(p_data) {
		}
	};

	typedef typename List<Pair>::Element *Element;

private:
	List<Pair> _list;
	HashMap<TKey, Element, Hasher, Comparator> _map;
	size_t capacity;

public:
	const Pair *insert(const TKey &p_key, const TData &p_value) {
		Element *e = _map.getptr(p_key);
		Element n = _list.push_front(Pair(p_key, p_value));

		if (e) {
			VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Waddress")
			if constexpr (BeforeEvict != nullptr) {
				BeforeEvict((*e)->get().key, (*e)->get().data);
			}
			VLTR_GCC_WARNING_POP
			_list.erase(*e);
			_map.erase(p_key);
		}
		_map[p_key] = _list.front();

		while (_map.size() > capacity) {
			Element d = _list.back();
			VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Waddress")
			if constexpr (BeforeEvict != nullptr) {
				BeforeEvict(d->get().key, d->get().data);
			}
			VLTR_GCC_WARNING_POP
			_map.erase(d->get().key);
			_list.pop_back();
		}

		return &n->get();
	}

	void clear() {
		_map.clear();
		_list.clear();
	}

	bool has(const TKey &p_key) const {
		return _map.getptr(p_key);
	}

	bool erase(const TKey &p_key) {
		Element *e = _map.getptr(p_key);
		if (!e) {
			return false;
		}
		_list.move_to_front(*e);
		_map.erase(p_key);
		_list.pop_front();
		return true;
	}

	const TData &get(const TKey &p_key) {
		Element *e = _map.getptr(p_key);
		CRASH_COND(!e);
		_list.move_to_front(*e);
		return (*e)->get().data;
	}

	const TData *getptr(const TKey &p_key) {
		Element *e = _map.getptr(p_key);
		if (!e) {
			return nullptr;
		} else {
			_list.move_to_front(*e);
			return &(*e)->get().data;
		}
	}

	_FORCE_INLINE_ size_t get_capacity() const { return capacity; }
	_FORCE_INLINE_ size_t get_size() const { return _map.size(); }

	void set_capacity(size_t p_capacity) {
		if (capacity > 0) {
			capacity = p_capacity;
			while (_map.size() > capacity) {
				Element d = _list.back();
				VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Waddress")
				if constexpr (BeforeEvict != nullptr) {
					BeforeEvict(d->get().key, d->get().data);
				}
				VLTR_GCC_WARNING_POP
				_map.erase(d->get().key);
				_list.pop_back();
			}
		}
	}

	LRUCache() {
		capacity = 64;
	}

	LRUCache(int p_capacity) {
		capacity = p_capacity;
	}
};

// --- Folded from core/templates/rb_map.h ---
#include "core/typedefs.h"
#include "core/os/memory.h"
#include "core/templates/hashfuncs.h"

#include <initializer_list>

// based on the very nice implementation of rb-trees by:
// https://web.archive.org/web/20120507164830/https://web.mit.edu/~emin/www/source_code/red_black_tree/index.html

template <typename K, typename V, typename C = Comparator<K>, typename A = DefaultAllocator>
class _WARN_UNUSED_ RBMap {
	enum Color {
		RED,
		BLACK
	};
	struct _Data;

public:
	class Element {
	private:
		friend class RBMap<K, V, C, A>;
		int color = RED;
		Element *right = nullptr;
		Element *left = nullptr;
		Element *parent = nullptr;
		Element *_next = nullptr;
		Element *_prev = nullptr;
		KeyValue<K, V> _data;

	public:
		KeyValue<K, V> &key_value() { return _data; }
		const KeyValue<K, V> &key_value() const { return _data; }

		const Element *next() const {
			return _next;
		}
		Element *next() {
			return _next;
		}
		const Element *prev() const {
			return _prev;
		}
		Element *prev() {
			return _prev;
		}
		const K &key() const {
			return _data.key;
		}
		V &value() {
			return _data.value;
		}
		const V &value() const {
			return _data.value;
		}
		V &get() {
			return _data.value;
		}
		const V &get() const {
			return _data.value;
		}
		Element(const KeyValue<K, V> &p_data) :
				_data(p_data) {}
	};

	typedef KeyValue<K, V> ValueType;

	struct Iterator {
		friend class RBMap<K, V, C, A>;

		_FORCE_INLINE_ KeyValue<K, V> &operator*() const {
			return E->key_value();
		}
		_FORCE_INLINE_ KeyValue<K, V> *operator->() const { return &E->key_value(); }
		_FORCE_INLINE_ Iterator &operator++() {
			E = E->next();
			return *this;
		}
		_FORCE_INLINE_ Iterator &operator--() {
			E = E->prev();
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const Iterator &p_it) const { return E == p_it.E; }
		_FORCE_INLINE_ bool operator!=(const Iterator &p_it) const { return E != p_it.E; }
		explicit operator bool() const {
			return E != nullptr;
		}

		Iterator &operator=(const Iterator &p_it) {
			E = p_it.E;
			return *this;
		}
		Iterator(Element *p_element) { E = p_element; }
		Iterator() {}
		Iterator(const Iterator &p_it) { E = p_it.E; }

	private:
		Element *E = nullptr;
	};

	struct ConstIterator {
		_FORCE_INLINE_ const KeyValue<K, V> &operator*() const {
			return E->key_value();
		}
		_FORCE_INLINE_ const KeyValue<K, V> *operator->() const { return &E->key_value(); }
		_FORCE_INLINE_ ConstIterator &operator++() {
			E = E->next();
			return *this;
		}
		_FORCE_INLINE_ ConstIterator &operator--() {
			E = E->prev();
			return *this;
		}

		_FORCE_INLINE_ bool operator==(const ConstIterator &p_it) const { return E == p_it.E; }
		_FORCE_INLINE_ bool operator!=(const ConstIterator &p_it) const { return E != p_it.E; }
		explicit operator bool() const {
			return E != nullptr;
		}

		ConstIterator &operator=(const ConstIterator &p_it) {
			E = p_it.E;
			return *this;
		}
		ConstIterator(const Element *p_element) { E = p_element; }
		ConstIterator() {}
		ConstIterator(const ConstIterator &p_it) { E = p_it.E; }

	private:
		const Element *E = nullptr;
	};

	_FORCE_INLINE_ Iterator begin() {
		return Iterator(front());
	}
	_FORCE_INLINE_ Iterator end() {
		return Iterator(nullptr);
	}

#if 0
	//to use when replacing find()
	_FORCE_INLINE_ Iterator find(const K &p_key) {
		return Iterator(find(p_key));
	}
#endif
	_FORCE_INLINE_ void remove(const Iterator &p_iter) {
		return erase(p_iter.E);
	}

	_FORCE_INLINE_ ConstIterator begin() const {
		return ConstIterator(front());
	}
	_FORCE_INLINE_ ConstIterator end() const {
		return ConstIterator(nullptr);
	}

#if 0
	//to use when replacing find()
	_FORCE_INLINE_ ConstIterator find(const K &p_key) const {
		return ConstIterator(find(p_key));
	}
#endif
private:
	struct _Data {
		Element *_root = nullptr;
		Element *_nil = nullptr;
		int size_cache = 0;

		_FORCE_INLINE_ _Data() {
#ifdef GLOBALNIL_DISABLED
			_nil = memnew_allocator(Element, A);
			_nil->parent = _nil->left = _nil->right = _nil;
			_nil->color = BLACK;
#else
			_nil = (Element *)&_GlobalNilClass::_nil;
#endif
		}

		void _create_root() {
			_root = memnew_allocator(Element(KeyValue<K, V>(K(), V())), A);
			_root->parent = _root->left = _root->right = _nil;
			_root->color = BLACK;
		}

		void _free_root() {
			if (_root) {
				memdelete_allocator<Element, A>(_root);
				_root = nullptr;
			}
		}

		~_Data() {
			_free_root();

#ifdef GLOBALNIL_DISABLED
			memdelete_allocator<Element, A>(_nil);
#endif
		}
	};

	_Data _data;

	inline void _set_color(Element *p_node, int p_color) {
		ERR_FAIL_COND(p_node == _data._nil && p_color == RED);
		p_node->color = p_color;
	}

	inline void _rotate_left(Element *p_node) {
		Element *r = p_node->right;
		p_node->right = r->left;
		if (r->left != _data._nil) {
			r->left->parent = p_node;
		}
		r->parent = p_node->parent;
		if (p_node == p_node->parent->left) {
			p_node->parent->left = r;
		} else {
			p_node->parent->right = r;
		}

		r->left = p_node;
		p_node->parent = r;
	}

	inline void _rotate_right(Element *p_node) {
		Element *l = p_node->left;
		p_node->left = l->right;
		if (l->right != _data._nil) {
			l->right->parent = p_node;
		}
		l->parent = p_node->parent;
		if (p_node == p_node->parent->right) {
			p_node->parent->right = l;
		} else {
			p_node->parent->left = l;
		}

		l->right = p_node;
		p_node->parent = l;
	}

	inline Element *_successor(Element *p_node) const {
		Element *node = p_node;

		if (node->right != _data._nil) {
			node = node->right;
			while (node->left != _data._nil) { /* returns the minimum of the right subtree of node */
				node = node->left;
			}
			return node;
		} else {
			while (node == node->parent->right) {
				node = node->parent;
			}

			if (node->parent == _data._root) {
				return nullptr; // No successor, as p_node = last node
			}
			return node->parent;
		}
	}

	inline Element *_predecessor(Element *p_node) const {
		Element *node = p_node;

		if (node->left != _data._nil) {
			node = node->left;
			while (node->right != _data._nil) { /* returns the minimum of the left subtree of node */
				node = node->right;
			}
			return node;
		} else {
			while (node == node->parent->left) {
				node = node->parent;
			}

			if (node == _data._root) {
				return nullptr; // No predecessor, as p_node = first node
			}
			return node->parent;
		}
	}

	Element *_find(const K &p_key) const {
		Element *node = _data._root->left;
		C less;

		while (node != _data._nil) {
			if (less(p_key, node->_data.key)) {
				node = node->left;
			} else if (less(node->_data.key, p_key)) {
				node = node->right;
			} else {
				return node; // found
			}
		}

		return nullptr;
	}

	Element *_find_closest(const K &p_key) const {
		Element *node = _data._root->left;
		Element *prev = nullptr;
		C less;

		while (node != _data._nil) {
			prev = node;

			if (less(p_key, node->_data.key)) {
				node = node->left;
			} else if (less(node->_data.key, p_key)) {
				node = node->right;
			} else {
				return node; // found
			}
		}

		if (prev == nullptr) {
			return nullptr; // tree empty
		}

		if (less(p_key, prev->_data.key)) {
			prev = prev->_prev;
		}

		return prev;
	}

	Element *_lower_bound(const K &p_key) const {
		Element *node = _data._root->left;
		Element *result = nullptr;
		C less;

		while (node != _data._nil) {
			if (!less(node->_data.key, p_key)) {
				result = node;
				node = node->left;
			} else {
				node = node->right;
			}
		}

		return result;
	}

	void _insert_rb_fix(Element *p_new_node) {
		Element *node = p_new_node;
		Element *nparent = node->parent;
		Element *ngrand_parent = nullptr;

		while (nparent->color == RED) {
			ngrand_parent = nparent->parent;

			if (nparent == ngrand_parent->left) {
				if (ngrand_parent->right->color == RED) {
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent->right, BLACK);
					_set_color(ngrand_parent, RED);
					node = ngrand_parent;
					nparent = node->parent;
				} else {
					if (node == nparent->right) {
						_rotate_left(nparent);
						node = nparent;
						nparent = node->parent;
					}
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent, RED);
					_rotate_right(ngrand_parent);
				}
			} else {
				if (ngrand_parent->left->color == RED) {
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent->left, BLACK);
					_set_color(ngrand_parent, RED);
					node = ngrand_parent;
					nparent = node->parent;
				} else {
					if (node == nparent->left) {
						_rotate_right(nparent);
						node = nparent;
						nparent = node->parent;
					}
					_set_color(nparent, BLACK);
					_set_color(ngrand_parent, RED);
					_rotate_left(ngrand_parent);
				}
			}
		}

		_set_color(_data._root->left, BLACK);
	}

	Element *_insert(const K &p_key, const V &p_value) {
		Element *new_parent = _data._root;
		Element *node = _data._root->left;
		C less;

		while (node != _data._nil) {
			new_parent = node;

			if (less(p_key, node->_data.key)) {
				node = node->left;
			} else if (less(node->_data.key, p_key)) {
				node = node->right;
			} else {
				node->_data.value = p_value;
				return node; // Return existing node with new value
			}
		}

		typedef KeyValue<K, V> KV;
		Element *new_node = memnew_allocator(Element(KV(p_key, p_value)), A);
		new_node->parent = new_parent;
		new_node->right = _data._nil;
		new_node->left = _data._nil;

		//new_node->data=_data;

		if (new_parent == _data._root || less(p_key, new_parent->_data.key)) {
			new_parent->left = new_node;
		} else {
			new_parent->right = new_node;
		}

		new_node->_next = _successor(new_node);
		new_node->_prev = _predecessor(new_node);
		if (new_node->_next) {
			new_node->_next->_prev = new_node;
		}
		if (new_node->_prev) {
			new_node->_prev->_next = new_node;
		}

		_data.size_cache++;
		_insert_rb_fix(new_node);
		return new_node;
	}

	void _erase_fix_rb(Element *p_node) {
		Element *root = _data._root->left;
		Element *node = _data._nil;
		Element *sibling = p_node;
		Element *parent = sibling->parent;

		while (node != root) { // If red node found, will exit at a break
			if (sibling->color == RED) {
				_set_color(sibling, BLACK);
				_set_color(parent, RED);
				if (sibling == parent->right) {
					sibling = sibling->left;
					_rotate_left(parent);
				} else {
					sibling = sibling->right;
					_rotate_right(parent);
				}
			}
			if ((sibling->left->color == BLACK) && (sibling->right->color == BLACK)) {
				_set_color(sibling, RED);
				if (parent->color == RED) {
					_set_color(parent, BLACK);
					break;
				} else { // loop: haven't found any red nodes yet
					node = parent;
					parent = node->parent;
					sibling = (node == parent->left) ? parent->right : parent->left;
				}
			} else {
				if (sibling == parent->right) {
					if (sibling->right->color == BLACK) {
						_set_color(sibling->left, BLACK);
						_set_color(sibling, RED);
						_rotate_right(sibling);
						sibling = sibling->parent;
					}
					_set_color(sibling, parent->color);
					_set_color(parent, BLACK);
					_set_color(sibling->right, BLACK);
					_rotate_left(parent);
					break;
				} else {
					if (sibling->left->color == BLACK) {
						_set_color(sibling->right, BLACK);
						_set_color(sibling, RED);
						_rotate_left(sibling);
						sibling = sibling->parent;
					}

					_set_color(sibling, parent->color);
					_set_color(parent, BLACK);
					_set_color(sibling->left, BLACK);
					_rotate_right(parent);
					break;
				}
			}
		}

		ERR_FAIL_COND(_data._nil->color != BLACK);
	}

	void _erase(Element *p_node) {
		Element *rp = ((p_node->left == _data._nil) || (p_node->right == _data._nil)) ? p_node : p_node->_next;
		Element *node = (rp->left == _data._nil) ? rp->right : rp->left;

		Element *sibling = nullptr;
		if (rp == rp->parent->left) {
			rp->parent->left = node;
			sibling = rp->parent->right;
		} else {
			rp->parent->right = node;
			sibling = rp->parent->left;
		}

		if (node->color == RED) {
			node->parent = rp->parent;
			_set_color(node, BLACK);
		} else if (rp->color == BLACK && rp->parent != _data._root) {
			_erase_fix_rb(sibling);
		}

		if (rp != p_node) {
			ERR_FAIL_COND(rp == _data._nil);

			rp->left = p_node->left;
			rp->right = p_node->right;
			rp->parent = p_node->parent;
			rp->color = p_node->color;
			if (p_node->left != _data._nil) {
				p_node->left->parent = rp;
			}
			if (p_node->right != _data._nil) {
				p_node->right->parent = rp;
			}

			if (p_node == p_node->parent->left) {
				p_node->parent->left = rp;
			} else {
				p_node->parent->right = rp;
			}
		}

		if (p_node->_next) {
			p_node->_next->_prev = p_node->_prev;
		}
		if (p_node->_prev) {
			p_node->_prev->_next = p_node->_next;
		}

		memdelete_allocator<Element, A>(p_node);
		_data.size_cache--;
		ERR_FAIL_COND(_data._nil->color == RED);
	}

	void _calculate_depth(Element *p_element, int &p_max_d, int p_d) const {
		if (p_element == _data._nil) {
			return;
		}

		_calculate_depth(p_element->left, p_max_d, p_d + 1);
		_calculate_depth(p_element->right, p_max_d, p_d + 1);

		if (p_d > p_max_d) {
			p_max_d = p_d;
		}
	}

	void _cleanup_tree(Element *p_element) {
		if (p_element == _data._nil) {
			return;
		}

		_cleanup_tree(p_element->left);
		_cleanup_tree(p_element->right);
		memdelete_allocator<Element, A>(p_element);
	}

	void _copy_from(const RBMap &p_map) {
		clear();
		// not the fastest way, but safeset to write.
		for (Element *I = p_map.front(); I; I = I->next()) {
			insert(I->key(), I->value());
		}
	}

public:
	const Element *find(const K &p_key) const {
		if (!_data._root) {
			return nullptr;
		}

		const Element *res = _find(p_key);
		return res;
	}

	Element *find(const K &p_key) {
		if (!_data._root) {
			return nullptr;
		}

		Element *res = _find(p_key);
		return res;
	}

	const Element *find_closest(const K &p_key) const {
		if (!_data._root) {
			return nullptr;
		}

		const Element *res = _find_closest(p_key);
		return res;
	}

	Element *find_closest(const K &p_key) {
		if (!_data._root) {
			return nullptr;
		}

		Element *res = _find_closest(p_key);
		return res;
	}

	Element *lower_bound(const K &p_key) {
		if (!_data._root) {
			return nullptr;
		}
		return _lower_bound(p_key);
	}

	const Element *lower_bound(const K &p_key) const {
		if (!_data._root) {
			return nullptr;
		}
		return _lower_bound(p_key);
	}

	bool has(const K &p_key) const {
		return find(p_key) != nullptr;
	}

	Element *insert(const K &p_key, const V &p_value) {
		if (!_data._root) {
			_data._create_root();
		}
		return _insert(p_key, p_value);
	}

	void erase(Element *p_element) {
		if (!_data._root || !p_element) {
			return;
		}

		_erase(p_element);
		if (_data.size_cache == 0 && _data._root) {
			_data._free_root();
		}
	}

	bool erase(const K &p_key) {
		if (!_data._root) {
			return false;
		}

		Element *e = find(p_key);
		if (!e) {
			return false;
		}

		_erase(e);
		if (_data.size_cache == 0 && _data._root) {
			_data._free_root();
		}
		return true;
	}

	const V &operator[](const K &p_key) const {
		CRASH_COND(!_data._root);
		const Element *e = find(p_key);
		CRASH_COND(!e);
		return e->_data.value;
	}

	V &operator[](const K &p_key) {
		if (!_data._root) {
			_data._create_root();
		}

		Element *e = find(p_key);
		if (!e) {
			e = insert(p_key, V());
		}

		return e->_data.value;
	}

	Element *front() const {
		if (!_data._root) {
			return nullptr;
		}

		Element *e = _data._root->left;
		if (e == _data._nil) {
			return nullptr;
		}

		while (e->left != _data._nil) {
			e = e->left;
		}

		return e;
	}

	Element *back() const {
		if (!_data._root) {
			return nullptr;
		}

		Element *e = _data._root->left;
		if (e == _data._nil) {
			return nullptr;
		}

		while (e->right != _data._nil) {
			e = e->right;
		}

		return e;
	}

	inline bool is_empty() const {
		return _data.size_cache == 0;
	}
	inline int size() const {
		return _data.size_cache;
	}

	int calculate_depth() const {
		// used for debug mostly
		if (!_data._root) {
			return 0;
		}

		int max_d = 0;
		_calculate_depth(_data._root->left, max_d, 0);
		return max_d;
	}

	void clear() {
		if (!_data._root) {
			return;
		}

		_cleanup_tree(_data._root->left);
		_data._root->left = _data._nil;
		_data.size_cache = 0;
		_data._free_root();
	}

	void operator=(const RBMap &p_map) {
		if (this == &p_map) {
			return;
		}

		_copy_from(p_map);
	}

	void operator=(RBMap &&p_map) {
		if (this == &p_map) {
			return;
		}

		SWAP(_data._root, p_map._data._root);
		SWAP(_data.size_cache, p_map._data.size_cache);
	}

	explicit RBMap(const RBMap &p_map) {
		_copy_from(p_map);
	}

	RBMap(RBMap &&p_map) {
		_data._root = p_map._data._root;
		_data.size_cache = p_map._data.size_cache;

		p_map._data._root = nullptr;
		p_map._data.size_cache = 0;
	}

	RBMap(std::initializer_list<KeyValue<K, V>> p_init) {
		for (const KeyValue<K, V> &E : p_init) {
			insert(E.key, E.value);
		}
	}

	_FORCE_INLINE_ RBMap() {}

	~RBMap() {
		clear();
	}
};
// --- End core/templates/rb_map.h ---
