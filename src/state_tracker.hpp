#pragma once

#include <atomic>
#include <boost/assert.hpp>

#include "utility.hpp"

namespace dmn {

enum class node_state: unsigned {
    RUN = 0,
    STOPPING_READ = 1,
    STOPPING_WRITE = 2,
    STOPPED = 4,
};

class state_tracker_t {
    alignas(hardware_destructive_interference_size) std::atomic<node_state>  state_ {node_state::RUN};

protected:
    virtual void on_stop_reading() noexcept = 0;
    virtual void on_stop_writing() noexcept = 0;
    virtual void on_stoped_writing() noexcept = 0;

    void no_more_readers() noexcept {
        if (state() != node_state::STOPPING_READ) {
            return;
        }

        node_state e = node_state::STOPPING_READ;
        const bool res = state_.compare_exchange_strong(e, node_state::STOPPING_WRITE);
        if (res) {
            on_stop_writing();
        }
    }

    void no_more_writers() noexcept {
        if (state() != node_state::STOPPING_WRITE) {
            return;
        }

        node_state e = node_state::STOPPING_WRITE;
        const bool res = state_.compare_exchange_strong(e, node_state::STOPPED);
        if (res) {
            on_stoped_writing();
            return;
        }

        BOOST_ASSERT(e == node_state::STOPPED);
    }

public:
    void shutdown_gracefully() noexcept {
        node_state e = node_state::RUN;
        const bool res = state_.compare_exchange_strong(e, node_state::STOPPING_READ);
        if (res) {
            on_stop_reading();
            return;
        }
    }

    node_state state() const noexcept {
        return state_.load(std::memory_order_relaxed);
    }
};

}
