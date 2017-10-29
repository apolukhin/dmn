#include "packet_network.hpp"
#include <boost/endian/conversion.hpp>

namespace dmn {

packet_header_network_t::packet_header_network_t(packet_header_native_t&& n) noexcept
    : version(boost::endian::native_to_little(n.version))
    , packet_type(boost::endian::native_to_little(n.packet_type))
    , wave_id(boost::endian::native_to_little(n.wave_id))
    , size(boost::endian::native_to_little(n.size))
{}

packet_header_native_t packet_header_network_t::to_native() noexcept {
    return {
          boost::endian::little_to_native(version)
        , boost::endian::little_to_native(packet_type)
        , boost::endian::little_to_native(wave_id)
        , boost::endian::little_to_native(size)
    };
}

packet_body_network_t::packet_body_network_t(packet_body_native_t&& n) noexcept
#ifdef BOOST_LITTLE_ENDIAN
    : data_(std::move(n.data_))
#endif
{
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "");
#endif
}

packet_body_native_t packet_body_network_t::to_native() noexcept {
#ifndef BOOST_LITTLE_ENDIAN
    static_assert(false, "");
#endif
    return {std::move(data_)};
}

} // namespace dmn
