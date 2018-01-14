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

    std::size_t                    helper_id_ = std::numeric_limits<std::size_t>::max();

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

    void set_helper_id(std::size_t id) noexcept {
        BOOST_ASSERT_MSG(helper_id_ == id || !is_helper_id_set(), "Setting different link id");
        helper_id_ = id;
    }

    bool is_helper_id_set() const noexcept {
        return helper_id_ != std::numeric_limits<std::size_t>::max();
    }

    std::size_t get_helper_id() const noexcept {
        return helper_id_;
    }
};

} // namespace dmn
