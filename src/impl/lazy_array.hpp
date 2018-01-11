#pragma once

#include "utility.hpp"
#include <boost/assert.hpp>
#include <type_traits>
#include <new>

namespace dmn {

// Replacement for std::vector<std::unique_ptr<T>> that has less indirections,
// constructible with parameters (unlike new T[])
template <class T>
class lazy_array {
    DMN_PINNED(lazy_array);

    std::size_t size_ = 0;
    T*  data_ = nullptr;
    std::size_t constructed_ = 0;

    using storage_t = std::aligned_storage_t<sizeof(T), alignof(T)>;
public:
    using value_type = T;
    using reference = value_type&;

    lazy_array() noexcept = default;

    std::size_t size() const noexcept {
        BOOST_ASSERT_MSG(size_ != 0, "Incorrect usage of pinned_container. size() must be called after init()");
        return size_;
    }

    void init(std::size_t size) {
        BOOST_ASSERT_MSG(size_ == 0, "Incorrect usage of pinned_container. it must be inited only once");
        BOOST_ASSERT(size != 0);
        size_ = size;
        data_ = reinterpret_cast<T*>(new storage_t[size_]);
    }

    ~lazy_array() noexcept {
        BOOST_ASSERT_MSG(size_ == constructed_, "Incorrect usage of pinned_container. All the elements must be construted");
        for (std::size_t i = 0; i < constructed_; ++i) {
            (*this)[i].~T();
        }

        delete[] reinterpret_cast<storage_t*>(data_);
    }

    template <class... Args>
    void inplace_construct_link(std::size_t i, Args&&... args) {
        BOOST_ASSERT_MSG(i == constructed_, "Incorrect usage of pinned_container. Elements must be construted sequentially, starting from element 0.");
        ++ constructed_;
        BOOST_ASSERT_MSG(constructed_ <= size_, "More elements constructed than initially planned in pinned_container::init().");
        new (begin() + i) T(std::forward<Args>(args)...);
    }

    T* begin() noexcept {
        return data_;
    }

    T* end() noexcept {
        return data_ + size_;
    }

    const T* begin() const noexcept {
        return data_;
    }

    const T* end() const noexcept {
        return data_ + size_;
    }

    T& operator[](std::size_t i) noexcept {
        BOOST_ASSERT_MSG(i < size_, "Out of bounds");
        BOOST_ASSERT_MSG(i < constructed_, "Acessing element that is not constructed");
        return data_[i];
    }

    const T& operator[](std::size_t i) const noexcept {
        BOOST_ASSERT_MSG(i < size_, "Out of bounds");
        BOOST_ASSERT_MSG(i < constructed_, "Acessing element that is not constructed");
        return data_[i];
    }
};

} // namespace dmn
