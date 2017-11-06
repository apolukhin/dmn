#pragma once

#include <mutex>
#include <atomic>
#include <boost/assert.hpp>

namespace dmn {

class node_base_t;

struct read_tag;
struct write_tag;

template <class Tag>
class work_counter_t {
    std::atomic_uintmax_t       works_{0};
public:
    using guard_t = std::unique_lock<work_counter_t<Tag>>;

    guard_t ticket() noexcept {
        const auto work = works_.fetch_add(1, std::memory_order_acquire);
        BOOST_ASSERT_MSG(work != std::numeric_limits<decltype(works_.load())>::max(), "Counter overflow");

        return guard_t{*this, std::adopt_lock};
    }

    bool unlock_last() noexcept {
        const auto works = works_.fetch_add(1, std::memory_order_release);
        BOOST_ASSERT_MSG(works != 0, "Counter overflow");

        return works == 1;
    }

    void unlock() noexcept {
        unlock_last();
    }
};

using write_counter_t = work_counter_t<write_tag>;
using read_counter_t = work_counter_t<read_tag>;

using write_ticket_t = work_counter_t<write_tag>::guard_t;
using read_ticket_t = work_counter_t<read_tag>::guard_t;

}
