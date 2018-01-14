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

struct packet_header_t {
    std::uint16_t       version = 1;
    packet_types_enum   packet_type = packet_types_enum::DATA;
    std::uint16_t       edge_id = 0;
    wave_id_t           wave_id; // TODO:
    std::uint32_t       size = 0;
};

using packet_storage_t = std::vector<unsigned char>;

class packet_t {
protected:
    packet_storage_t data_;


    void clear() noexcept {
        data_.clear();
    }
public:

    void place_header();

    bool empty() const noexcept {
        return data_.empty();
    }

    void add_data(const unsigned char* data, std::uint32_t size, const char* type);
    std::pair<const unsigned char*, std::size_t> get_data(const char* type) const noexcept;

    packet_t() = default;
    packet_t(packet_t&& ) noexcept = default;
    packet_t& operator=(packet_t&&) noexcept = default;

    explicit packet_t(packet_storage_t&& storage) noexcept
        : data_(std::move(storage))
    {}

    const packet_storage_t& raw_storage() const noexcept {
        return data_;
    }


    packet_header_t& header() noexcept {
        BOOST_ASSERT_MSG(!data_.empty(), "Attempt to get header without initing it via place_header()");
        return *reinterpret_cast<packet_header_t*>(data_.data());
    }

    const packet_header_t& header() const noexcept {
        BOOST_ASSERT_MSG(!data_.empty(), "Attempt to get header without initing it via place_header()");
        return *reinterpret_cast<const packet_header_t*>(data_.data());
    }
};



} // namespace dmn
