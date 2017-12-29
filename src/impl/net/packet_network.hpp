#pragma once

#include "impl/packet.hpp"

#include <cstdint>
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/assert.hpp>

namespace dmn {

class packet_network_t: private packet_t {
public:
    packet_network_t() = default;
    packet_network_t(packet_network_t&&) = default;
    packet_network_t& operator=(packet_network_t&&) = default;
    explicit packet_network_t(packet_t&& n) noexcept;

    auto header_const_buffer() noexcept {
        return boost::asio::const_buffers_1(reinterpret_cast<unsigned char*>(&header()), sizeof(packet_header_t));
    }
    auto header_mutable_buffer() noexcept {
        place_header();
        return boost::asio::mutable_buffers_1(reinterpret_cast<unsigned char*>(&header()), sizeof(packet_header_t));
    }

    auto body_mutable_buffer() {
        data_.resize(expected_body_size() + sizeof(packet_header_t));
        return boost::asio::mutable_buffers_1{
            boost::asio::mutable_buffer(data_.data() + sizeof(packet_header_t), expected_body_size())
        };
    }

    boost::asio::const_buffers_1 const_buffer() const noexcept {
        return boost::asio::const_buffers_1{
            data_.data(), data_.size()
        };
    }

    packet_types_enum packet_type() const noexcept;
    std::uint32_t expected_body_size() const noexcept;
    std::uint32_t actual_body_size() const noexcept;
    using packet_t::clear;
    using packet_t::empty;
    packet_t to_native() && noexcept;

    const void* data_address() const noexcept {
        return data_.data();
    }
};


} // namespace dmn
