#pragma once

#include "node_base.hpp"
#include "packet.hpp"
#include "stream.hpp"

#include <boost/asio/io_service.hpp>

namespace dmn {

class node_impl_read_0: public virtual node_base_t {
public:
    node_impl_read_0(std::istream& in, const char* node_id)
        : node_base_t(in, node_id)
    {}

    void start() override final {
        ios().post([this](){
            stream_t s{*this};
            node_base_t::callback_(s);
            start();

            on_packet_send(s.move_out_data());
        });
    }

    void on_packet_accept(packet_native_t&& /*packet*/) override final {
        BOOST_ASSERT_MSG(false, "Must not be called for non accepting vertexes");
    }

    ~node_impl_read_0() noexcept = default;
};

}
