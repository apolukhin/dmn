#pragma once

#include <chrono>

#include "utility.hpp"

namespace dmn {

class saturation_timer_t {
    unsigned value_{0};
    static constexpr unsigned max_value_ = 12;
public:
    void operator--() noexcept {
        if (!value_) {
            return;
        }
        --value_;

    }

    void operator++() noexcept {
        if (is_max()) {
            return;
        }
        ++value_;
    }

    bool is_max() const noexcept {
        return value_ == max_value_;
    }

    std::chrono::milliseconds timeout() const noexcept {
        return std::chrono::milliseconds(1 << value_);
    }
};

}
