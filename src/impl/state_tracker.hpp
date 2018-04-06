#pragma once

#include <atomic>
#include <boost/assert.hpp>

#include "utility.hpp"
#include "work_counter.hpp"

namespace dmn {

enum class node_state: unsigned {
    RUN = 0,
    STOPPING = 1
};

class state_tracker_t {
    // This variable changes very rarely. No need to alignas(hardware_destructive_interference_size)
    std::atomic<node_state>  state_ {node_state::RUN};

protected:
    virtual void on_stop_reading() noexcept = 0;

public:
    void shutdown_gracefully() noexcept {
        node_state e = node_state::RUN;
        const bool res = state_.compare_exchange_strong(e, node_state::STOPPING);
        if (res) {
            on_stop_reading();
            return;
        }
    }

    node_state state() const noexcept {
        return state_.load();
    }
};

}
