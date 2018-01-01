#include "packet_network.hpp"
#include <boost/endian/conversion.hpp>

namespace dmn {


packet_network_t::packet_network_t(packet_t&& n) noexcept
    : packet_t(std::move(n))
{
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "");
#endif
}

packet_types_enum packet_network_t::packet_type() const noexcept {
    return header().packet_type;
}

std::uint32_t packet_network_t::expected_body_size() const noexcept {
    return header().size;
}

std::uint32_t packet_network_t::actual_body_size() const noexcept {
    return data_.size() > sizeof(header()) ? data_.size() - sizeof(header()) : 0;
}


packet_t packet_network_t::to_native() && noexcept {
    return packet_t{std::move(data_)};
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "");
#endif
}

} // namespace dmn