#pragma once

#include "node.hpp"

#include "stream.hpp"

namespace dmn {

class node_impl_x_0: public virtual node_base_t {
public:
    node_impl_x_0(std::istream& in, const char* node_id)
        : node_base_t(in, node_id)
    {}

    ~node_impl_x_0() noexcept = default;
};

}
