#include "impl/net/tcp_read_proto.hpp"

#include "impl/net/proto_common.hpp"
#include "impl/net/wrap_handler.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>

namespace dmn {

tcp_read_proto_t::tcp_read_proto_t(boost::asio::ip::tcp::socket socket, on_error_t on_error, on_operation_finished_t on_operation_finished)
    : socket_(std::move(socket))
    , on_error_(std::move(on_error))
    , on_operation_finished_(std::move(on_operation_finished))
{
    set_socket_options(socket_);
}

tcp_read_proto_t::~tcp_read_proto_t() = default;


void tcp_read_proto_t::async_read(boost::asio::mutable_buffers_1 data) {
    BOOST_ASSERT(socket_.is_open());

    auto on_read = [this, data](const boost::system::error_code& e, std::size_t bytes_read) {
        if (e) {
            process_error(e);
            return;
        }

        BOOST_ASSERT_MSG(bytes_read == boost::asio::buffer_size(data), "Wrong bytes read");
        on_operation_finished_(*this);
    };

    boost::asio::async_read(
        socket_,
        data,
        make_slab_alloc_handler(slab_, std::move(on_read))
    );
}

void tcp_read_proto_t::close() noexcept {
    boost::system::error_code ignore;
    socket_.shutdown(decltype(socket_)::shutdown_both, ignore);
    socket_.close(ignore);
}

} // namespace dmn
