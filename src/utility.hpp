#pragma once
#include <cctype>

namespace dmn {
static constexpr std::size_t hardware_destructive_interference_size = 64;
static constexpr std::size_t hardware_constructive_interference_size = 64;

using byte_t = unsigned char;
using bytes_ptr_t = byte_t*;
using cbytes_ptr_t = const byte_t*;

inline bytes_ptr_t as_bytes_ptr(void* p) noexcept {
    return static_cast<bytes_ptr_t>(p);
}

inline cbytes_ptr_t as_bytes_ptr(const void* p) noexcept {
    return static_cast<cbytes_ptr_t>(p);
}

// For classes that must be "pinned" to a particular location.
// Note: not using a base class, because we may have multiple inheritance of those.
#define DMN_PINNED(type)                    \
    type(const type&) = delete;             \
    type(type&&) = delete;                  \
    type& operator=(const type&) = delete;  \
    type& operator=(type&&) = delete        \
    /**/


} // namespace dmn
