#pragma once

#include <cstdint>
#include <vector>

namespace dmn {

enum class packet_types_enum: std::uint16_t {
    DATA,
};

enum class wave_id_t : std::uint32_t {};

struct packet_header_native_t {
    std::uint16_t       version = 1;
    packet_types_enum   packet_type = packet_types_enum::DATA;
    wave_id_t           wave_id; // TODO:
    std::uint32_t       size = 0;
};

struct packet_body_native_t {
    std::vector<unsigned char> data_;
};

struct packet_native_t {
    packet_header_native_t header_;
    packet_body_native_t   body_;
};



} // namespace dmn
