#include "node.hpp"

#include <boost/asio/io_service.hpp>

namespace {
    boost::asio::io_service ios;
}

namespace dmn {

node_t::~node_t() noexcept = default;

boost::asio::io_service& node_t::ios() noexcept {
    return ::ios;
}

}
