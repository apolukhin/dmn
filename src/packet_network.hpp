#pragma once

#include "packet.hpp"

#include <cstdint>
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/assert.hpp>

namespace dmn {

class packet_header_network_t {
    std::uint16_t       version = 1;
    packet_types_enum   packet_type = packet_types_enum::DATA;
    wave_id_t           wave_id; // TODO:
    std::uint32_t       size = 0;

public:
    packet_header_network_t() = default;
    packet_header_network_t(packet_header_network_t&&) = default;
    packet_header_network_t& operator=(packet_header_network_t&&) = default;

    packet_header_network_t(const packet_header_network_t&) = delete;
    packet_header_network_t& operator=(const packet_header_network_t&) = delete;

    explicit packet_header_network_t(packet_header_native_t&& n) noexcept;
    packet_header_native_t to_native() noexcept;
    auto buffer() noexcept {
        return boost::asio::buffer(reinterpret_cast<unsigned char*>(this), sizeof(packet_header_network_t));
    }
};

class packet_body_network_t {
    std::vector<unsigned char> data_;

public:
    packet_body_network_t() = default;
    packet_body_network_t(packet_body_network_t&&) = default;
    packet_body_network_t& operator=(packet_body_network_t&&) = default;

    packet_body_network_t(const packet_body_network_t&) = delete;
    packet_body_network_t& operator=(const packet_body_network_t&) = delete;

    explicit packet_body_network_t(packet_body_native_t&& n) noexcept;
    packet_body_native_t to_native() noexcept;
    auto buffer() noexcept {
        BOOST_ASSERT(!data_.empty());
        return boost::asio::mutable_buffers_1{
            data_.empty() ? boost::asio::mutable_buffer() : boost::asio::mutable_buffer(&data_[0], data_.size())
        };
    }
};

struct packet_network_t {
    packet_header_network_t header_;
    packet_body_network_t   body_;

    packet_network_t() = default;
    explicit packet_network_t(packet_native_t&& n) noexcept
        : header_(std::move(n.header_))
        , body_(std::move(n.body_))
    {}

    packet_native_t to_native() noexcept {
        return packet_native_t{
            header_.to_native(),
            body_.to_native()
        };
    }    

    std::array<boost::asio::mutable_buffer, 2> buffer() noexcept {
        return std::array<boost::asio::mutable_buffer, 2> {
            *header_.buffer().begin(),
            *body_.buffer().begin()
        };
    }
};


} // namespace dmn