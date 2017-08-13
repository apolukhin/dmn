#pragma once

#include "node.hpp"

#include <cstdint>

namespace dmn {

enum class packet_types_enum: std::uint16_t {
    DATA,
};

enum class message_id_t : std::uint32_t {};

struct message_header {
    std::uint16_t       version = 1;
    packet_types_enum   packet_type = packet_types_enum::DATA;
    message_id_t        message_id;
    std::uint32_t       size = 0;

    void to_native_endian() noexcept;
    void to_little_endian() noexcept;
};

}
