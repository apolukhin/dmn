#pragma once

#include "node_base.hpp"
#include "message.hpp"

namespace dmn {

class node_impl_0_x: public virtual node_base_t {
    static thread_local message_t m_;
public:
    node_impl_0_x(std::istream& in, const char* node_id)
        : node_base_t(in, node_id)
    {}

    void add_message(const void* data, std::size_t size, const char* type = "") override {
        if (type == nullptr) {
            type = "";
        }

        m_.add(data, size, type);
    }

    void send_message() override {
        on_message(std::move(m_));
    }


    ~node_impl_0_x() noexcept = default;
};

}
