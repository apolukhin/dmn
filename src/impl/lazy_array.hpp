#pragma once

#include "utility.hpp"
#include <boost/assert.hpp>
#include <type_traits>
#include <new>
#include <iterator>

namespace dmn {

// Replacement for std::vector<std::unique_ptr<T>> that has less indirections,
// constructible with parameters (unlike new T[]).
//
// OK to store derived from T types in this array as long as they consume at most Storage size.
template <class T, std::size_t Storage = sizeof (T)>
class lazy_array {
    DMN_PINNED(lazy_array);

    using storage_t = std::aligned_storage_t<Storage, alignof(T)>;

    std::size_t size_ = 0;
    storage_t*  data_ = nullptr;
    std::size_t constructed_ = 0;

public:
    template <class Value> class iterator_base;

    using value_type = T;
    using reference = value_type&;
    using iterator = iterator_base<T>;
    using const_iterator = iterator_base<const T>;

    lazy_array() noexcept = default;

    std::size_t size() const noexcept {
        BOOST_ASSERT_MSG(size_ != 0, "Incorrect usage of pinned_container. size() must be called after init()");
        return size_;
    }

    void init(std::size_t size) {
        BOOST_ASSERT_MSG(size_ == 0, "Incorrect usage of pinned_container. it must be inited only once");
        BOOST_ASSERT(size != 0);
        size_ = size;
        data_ = new storage_t[size_];
    }

    ~lazy_array() noexcept {
        static_assert(Storage >= sizeof(T), "Attempt to store type in an unsufficient storage");

        BOOST_ASSERT_MSG(size_ == constructed_, "Incorrect usage of pinned_container. All the elements must be construted");
        for (std::size_t i = 0; i < constructed_; ++i) {
            (*this)[i].~T();
        }

        delete[] data_;
    }

    template <class Derived, class... Args>
    void inplace_construct_derived(std::size_t i, Args&&... args) {
        static_assert(std::is_base_of<T, Derived>::value || std::is_same<T, Derived>::value, "Data type Derived must derive from T");
        static_assert(Storage >= sizeof(Derived), "Attempt to store a derived type in an unsufficient storage");
        static_assert(alignof(T) == alignof(Derived), "Alignments must match");
        static_assert(
            std::has_virtual_destructor<T>::value || std::is_same<T, Derived>::value,
            "Derived data type construction in lazy_array possible ONLY if base type T has virtual destructor"
        );

        BOOST_ASSERT_MSG(i == constructed_, "Incorrect usage of pinned_container. Elements must be construted sequentially, starting from element 0.");
        ++ constructed_;
        BOOST_ASSERT_MSG(constructed_ <= size_, "More elements constructed than initially planned in pinned_container::init().");
        new (data_ + i) Derived(std::forward<Args>(args)...);
    }

    template <class... Args>
    void inplace_construct(std::size_t i, Args&&... args) {
        inplace_construct_derived<T>(i, std::forward<Args>(args)...);
    }

    iterator begin() noexcept {
        return iterator{data_};
    }

    iterator end() noexcept {
        return iterator{data_ + size_};
    }

    const_iterator begin() const noexcept {
        return const_iterator{data_};
    }

    const_iterator end() const noexcept {
        return const_iterator{data_ + size_};
    }

    T& operator[](std::size_t i) noexcept {
        BOOST_ASSERT_MSG(i < size_, "Out of bounds");
        BOOST_ASSERT_MSG(i < constructed_, "Acessing element that is not constructed");
        return *reinterpret_cast<T*>(data_ + i);
    }

    const T& operator[](std::size_t i) const noexcept {
        BOOST_ASSERT_MSG(i < size_, "Out of bounds");
        BOOST_ASSERT_MSG(i < constructed_, "Acessing element that is not constructed");
        return *reinterpret_cast<T*>(data_ + i);
    }
};



template <class T, std::size_t Storage>
template <class Value>
class lazy_array<T, Storage>::iterator_base {
    storage_t* pos_ = nullptr;

public:
    using value_type = Value;
    using reference = Value&;
    using iterator_category = std::forward_iterator_tag;

    iterator_base() noexcept = default;

    explicit iterator_base(storage_t* pos) noexcept
        : pos_(pos)
    {}

    iterator_base operator++() noexcept {  // prefix
        ++ pos_;
        return *this;
    }

    iterator_base operator++(int) noexcept { // postfix
        const auto it = *this;
        ++(*this);
        return it;
    }

    reference operator*() const noexcept {
        return reinterpret_cast<reference>(*pos_);
    }

    bool operator==(iterator_base rhs) const noexcept {
        return pos_ == rhs.pos_;
    }

    bool operator!=(iterator_base rhs) const noexcept {
        return pos_ != rhs.pos_;
    }
};

} // namespace dmn
