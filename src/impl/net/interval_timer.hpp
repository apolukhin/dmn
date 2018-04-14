#pragma once

#include "utility.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/optional/optional.hpp>

namespace dmn {

class interval_timer {
    DMN_PINNED(interval_timer);

private:
    boost::optional<boost::asio::steady_timer>                  timer_;
    const std::chrono::milliseconds ms_;

    void reschedule(boost::system::error_code ec) {
        timer_->expires_after(ms_);
        timer_->async_wait([this](boost::system::error_code ec){
            reschedule(ec);
        });
    }

public:
    interval_timer(boost::asio::io_context& ios, std::chrono::milliseconds ms)
        : timer_{ios}
        , ms_(ms)
    {
        timer_->expires_after(ms_);
        timer_->async_wait([this](boost::system::error_code ec){
            reschedule(ec);
        });
    }
    ~interval_timer() {
        BOOST_ASSERT_MSG(!timer_, "Timer must be closed before destruction!");
    }

    template <class F>
    void async_wait_err(F f) {
        timer_->async_wait(std::move(f));
    }

    template <class F>
    void async_wait(F f) {
        timer_->async_wait([f = std::move(f)](auto ec) mutable {
            BOOST_ASSERT_MSG(!ec || ec == boost::system::errc::operation_canceled, "Unexpected error code.");
            f();
        });
    }

    void close() noexcept {
        boost::system::error_code ignore;
        timer_->cancel(ignore);
        timer_.reset();
    }
};

} // namespace dmn
