#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <boost/assert.hpp>

namespace dmn {

enum class packet_types_enum: std::uint16_t {
    DATA,
    SHUTDOWN_GRACEFULLY,
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

    void add_data(const unsigned char* data, std::uint32_t size, const char* type) {
        BOOST_ASSERT(type);
        constexpr auto range = [](const std::uint32_t& v) noexcept {
            return std::make_pair(
                reinterpret_cast<const unsigned char*>(&v),
                reinterpret_cast<const unsigned char*>(&v) + sizeof(v)
            );
        };

        const std::uint32_t type_len = std::strlen(type);
        data_.reserve(
            (
                data_.size()
                + sizeof(std::uint32_t) + type_len + sizeof(std::uint32_t) + size
                + 63
            ) % 64
        );
        data_.insert(data_.end(), range(type_len).first, range(type_len).second);
        data_.insert(data_.end(), type, type + type_len);
        data_.insert(data_.end(), range(size).first, range(size).second);
        data_.insert(data_.end(), data, data + size);
    }

    std::pair<const unsigned char*, std::size_t> get_data(const char* type) const noexcept {
        BOOST_ASSERT(type);
        const std::uint32_t type_len = std::strlen(type);
        const unsigned char* data = data_.data();
        const unsigned char* const data_end = data + data_.size();

        while (data != data_end) {
            std::uint32_t current_type_len; // intentionally unintialized
            std::memcpy(&current_type_len, data, sizeof(std::uint32_t));
            data += sizeof(std::uint32_t);
            BOOST_ASSERT(data < data_end);

            const bool found = (current_type_len == type_len && !std::memcmp(data, type, type_len));
            data += current_type_len;
            BOOST_ASSERT(data < data_end);

            std::uint32_t current_data_len; // intentionally unintialized
            std::memcpy(&current_data_len, data, sizeof(std::uint32_t));
            data += sizeof(std::uint32_t);
            BOOST_ASSERT(data < data_end);

            if (found) {
                return {data, current_data_len};
            }
            data += current_data_len;
        }

        return { nullptr, 0u };
    }
};

struct packet_native_t {
    packet_header_native_t header_;
    packet_body_native_t   body_;
};



} // namespace dmn
