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

} // namespace boost
