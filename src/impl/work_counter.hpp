#pragma once

#include <atomic>
#include <boost/assert.hpp>

#include "utility.hpp"

namespace dmn {

class work_counter_t {
    alignas(hardware_destructive_interference_size) std::atomic_uintmax_t       works_{0};

public:
    auto add(const std::size_t work_count = 1) noexcept {
        BOOST_ASSERT_MSG(work_count > 0, "Attempt to add 0 work");

        const auto work = works_.fetch_add(work_count, std::memory_order_acquire);
        BOOST_ASSERT_MSG(std::numeric_limits<decltype(works_.load())>::max() - work >= work_count, "Counter overflow");

        return work + work_count;
    }

    auto get() const noexcept {
        return works_.load();
    }

    auto remove(const std::size_t work_count = 1) noexcept {
        BOOST_ASSERT_MSG(work_count > 0, "Attempt to add 0 work");

        const auto work = works_.fetch_sub(work_count, std::memory_order_release);
        BOOST_ASSERT_MSG(work >= work_count, "Counter overflow");

        return work - work_count;
    }
};

}
