#pragma once
#include <iterator>
struct default_tag;

template <typename Tag = default_tag>
struct list_element {
private:
	template <typename FT, typename FTag>
	friend class intrusive_list;
	list_element* next = nullptr;
	list_element* prev = nullptr;

public:
	void unlink() {
		if (next != nullptr) next->prev = prev;
		if (prev != nullptr) prev->next = next;
		prev = next = nullptr;
	}
};

template <typename T, typename Tag = default_tag>
class intrusive_list {
private:
	list_element<Tag> root;

	template <typename IT>
	class iterator_impl {
	public:
		using iterator_category = std::bidirectional_iterator_tag;
		using value_type = std::remove_reference_t<IT>;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

	private:
		friend intrusive_list;
		list_element<Tag>* me;
		explicit iterator_impl(decltype(me) to) noexcept
			: me(to) {}

	public:
		iterator_impl(void) noexcept
			: me(nullptr) {}
		pointer operator->(void) const noexcept { return static_cast<IT*>(me); }
		operator reference(void) const { return static_cast<IT&>(*me); }

		reference operator*(void) const noexcept { return static_cast<IT&>(*me); }

		iterator_impl& operator++(void) noexcept {
			me = me->next;
			return *this;
		}
		iterator_impl operator++(int) noexcept {
			auto copy = me;
			me = me->next;
			return iterator_impl(copy);
		}
		iterator_impl& operator--(void) noexcept {
			me = me->prev;
			return *this;
		}
		iterator_impl operator--(int) noexcept {
			auto copy = me;
			me = me->prev;
			return iterator_impl(copy);
		}

		template <typename T1>
		bool operator==(const iterator_impl<T1>& r) const noexcept {
			return me == r.me;
		}
		template <typename T1>
		bool operator!=(const iterator_impl<T1>& r) const noexcept {
			return !operator==(r);
		}

		operator iterator_impl<const value_type>(void) const noexcept {
			return iterator_impl<const value_type>(me);
		}
	};

public:
	using iterator = iterator_impl<T>;
	using const_iterator = iterator_impl<const T>;

	intrusive_list() noexcept { root.next = root.prev = &root; }
	intrusive_list(intrusive_list const&) = delete;
	intrusive_list(intrusive_list&& r) noexcept
		: intrusive_list() {
		operator=(std::move(r));
	}
	~intrusive_list() { clear(); }

	intrusive_list& operator=(intrusive_list const&) = delete;
	intrusive_list& operator=(intrusive_list&& r) noexcept {
		clear();
		splice(end(), r, r.begin(), r.end());
		return *this;
	}

	void clear() noexcept {
		if (empty()) return;
		// now root.next != root
		root.prev->next = nullptr;
		auto next = root.next;
		root.next = root.prev = &root;

		while (next->next != nullptr) {
			auto save = next->next;
			next->prev = next->next = nullptr;
			next = save;
		}
		next->prev = nullptr;
	}

	void push_back(T& u) noexcept { insert(end(), u); }
	void pop_back() noexcept { root.prev->unlink(); }
	T& back() noexcept { return static_cast<T&>(*root.prev); }
	T const& back() const noexcept { return static_cast<const T&>(*root.prev); }

	void push_front(T& u) noexcept { insert(begin(), u); }
	void pop_front() noexcept { root.next->unlink(); }
	T& front() noexcept { return static_cast<T&>(*root.next); }
	T const& front() const noexcept { return static_cast<T&>(*root.next); }

	bool empty() const noexcept { return root.next == &root; }

	iterator begin() noexcept { return iterator(root.next); }
	const_iterator begin() const noexcept { return const_iterator(root.next); }

	iterator end() noexcept { return iterator(&root); }
	const_iterator end() const noexcept {
		return const_iterator(const_cast<list_element<Tag>*>(&root));
	}

	iterator insert(const_iterator pos, T& u) noexcept {
		auto& v = static_cast<list_element<Tag>&>(u);
		pos.me->prev->next = &v;
		v.prev = pos.me->prev;
		v.next = pos.me;
		pos.me->prev = &v;
		return iterator(&v);
	}
	iterator erase(T& u) noexcept {
		auto pos = iterator(&u);
		iterator ret(pos.me->next);
		pos.me->unlink();
		return ret;
	}
	void splice(const_iterator pos, intrusive_list&, const_iterator first,
		const_iterator last) noexcept {
		if (pos == first || first == last) return;
		auto* true_last = last.me->prev;
		first.me->prev->next = true_last->next;
		true_last->next->prev = first.me->prev;

		pos.me->prev->next = first.me;
		first.me->prev = pos.me->prev;

		pos.me->prev = true_last;
		true_last->next = pos.me;
	}
};
