#pragma once

#include <memory>


struct compare_addrs_t {
    template <class T>
    bool operator()(const std::unique_ptr<T>& lhs, const T& rhs) const noexcept {
        return lhs.get() < std::addressof(rhs);
    }

    template <class T>
    bool operator()(const std::unique_ptr<T>& lhs, const std::unique_ptr<T>& rhs) const noexcept {
        return lhs.get() < rhs.get();
    }

    template <class T>
    bool operator()(const T& lhs, const std::unique_ptr<T>& rhs) const noexcept {
        return std::addressof(lhs)< rhs.get();
    }
};
