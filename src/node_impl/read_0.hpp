#pragma once

#include "node_base.hpp"
#include "packet.hpp"
#include "stream.hpp"
#include "work_counter.hpp"

#include <boost/asio/io_service.hpp>

namespace dmn {

class node_impl_read_0: public virtual node_base_t {
    std::atomic<std::uint16_t> subwave_id_{0};

    wave_id_t new_wave() noexcept {
        constexpr std::uint32_t shift = (sizeof(wave_id_t) - sizeof(host_id_)) * CHAR_BIT;
        const std::uint32_t subwave = subwave_id_.load(std::memory_order_relaxed);
        const std::uint32_t res = (host_id_ << shift) | subwave;
        return static_cast<wave_id_t>(res);
    }

    void start() {
        ios().post([this]() {
            packet_t p{};
            p.place_header();
            p.header().wave_id = new_wave();
            on_packet_accept(std::move(p));
            if (state() == node_state::RUN) {
                start();
            } else {
                no_more_readers(); // TODO: mutithreaded run
            }
        });
    }
public:
    node_impl_read_0() {
        start(); // TODO: mutithreaded run
    }

    void on_stop_reading() noexcept override final {
        // Noop
    }

    ~node_impl_read_0() noexcept = default;
};

}
