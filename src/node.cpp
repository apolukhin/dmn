#include "node.hpp"

#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>

namespace dmn {

node_t::~node_t() noexcept = default;

boost::asio::io_service& node_t::ios() noexcept {
    return *ios_;
}

}
