#pragma once

#include <boost/asio/ip/tcp.hpp>
namespace dmn {

    template <class Socket>
    void set_ping_keepalives(Socket& s) noexcept {
        BOOST_ASSERT_MSG(s.is_open(), "Attempt to set socket options on a closed socket");
    #if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
        static_assert(false, "");
    #else
        int optval = 1; // enable
        constexpr socklen_t optlen = sizeof(optval);
        BOOST_VERIFY_MSG(setsockopt(s.native_handle(), SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) == 0, "Failed to set SO_KEEPALIVE for a reading socket");

        optval = 10; // 10s
        BOOST_VERIFY_MSG(setsockopt(s.native_handle(), SOL_TCP, TCP_KEEPIDLE, &optval, optlen) == 0, "Failed to set KEEPALIVE timeout for a reading socket");
    #endif
    }

    template <class Socket>
    void set_writing_ack_timeout(Socket& s) noexcept {
        BOOST_ASSERT_MSG(s.is_open(), "Attempt to set socket options on a closed socket");
    #if defined _WIN32 || defined WIN32 || defined OS_WIN64 || defined _WIN64 || defined WIN64 || defined WINNT
        static_assert(false, "");
    #else
        unsigned int optval = 150; // 150ms
        constexpr socklen_t optlen = sizeof(optval);
        BOOST_VERIFY_MSG(setsockopt(s.native_handle(), SOL_TCP, TCP_USER_TIMEOUT, &optval, optlen) == 0, "Failed to set ACK timeout for a socket");
    #endif
    }

    template <class Socket>
    void set_socket_options(Socket& s) {
        BOOST_ASSERT_MSG(s.is_open(), "Attempt to set socket options on a closed socket");

        boost::asio::ip::tcp::no_delay option(true);
        boost::system::error_code ignore;
        s.set_option(option, ignore);
        BOOST_ASSERT_MSG(!ignore, "Failed to set no_dealy for socket");

        set_ping_keepalives(s);
    }


} // namespace dmn
