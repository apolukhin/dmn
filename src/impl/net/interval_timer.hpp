#pragma once

#include "utility.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/optional/optional.hpp>

#include "impl/net/wrap_handler.hpp"

namespace dmn {

class interval_timer {
    DMN_PINNED(interval_timer);

private:
    boost::optional<boost::asio::steady_timer>                  timer_;
    const std::chrono::milliseconds ms_;
    slab_allocator_t allocator_;

    template <class F>
    void reschedule(F func) {
        timer_->expires_after(ms_);
        timer_->async_wait(dmn::make_slab_alloc_handler(
            allocator_,
            [this, func = std::move(func)](boost::system::error_code ec) mutable {
                func();
                if (!ec) {
                    reschedule(std::move(func));
                }
            }
        ));
    }

public:
    template <class F>
    interval_timer(boost::asio::io_context& ios, std::chrono::milliseconds ms, F function)
        : timer_{ios}
        , ms_(ms)
    {
        reschedule(std::move(function));
    }

    ~interval_timer() {
        BOOST_ASSERT_MSG(!timer_, "Timer must be closed before destruction!");
    }

    void close() noexcept {
        boost::system::error_code ignore;
        timer_->cancel(ignore);
        timer_.reset();
    }
};

} // namespace dmn
