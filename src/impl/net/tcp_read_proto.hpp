#pragma once

#include "impl/net/slab_allocator.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

namespace dmn {

class tcp_read_proto_t {
    DMN_PINNED(tcp_read_proto_t);

private:
    boost::asio::ip::tcp::socket socket_;
    using on_error_t = std::function<void(tcp_read_proto_t&, const boost::system::error_code&)>;
    const on_error_t on_error_;

    using on_operation_finished_t = std::function<void(tcp_read_proto_t&)>;
    const on_operation_finished_t on_operation_finished_;

    slab_allocator_t slab_;

    void process_error(const boost::system::error_code& e) {
        on_error_(*this, e);
    }

protected:
    tcp_read_proto_t(boost::asio::ip::tcp::socket socket, on_error_t on_error, on_operation_finished_t on_operation_finished);
    ~tcp_read_proto_t();

public:
    void async_read(boost::asio::mutable_buffers_1 data);
    void close() noexcept;

};

} // namespace dmn
