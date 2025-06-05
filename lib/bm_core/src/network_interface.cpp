#include "bm_core/network_interface.hpp"
#include "bm_core/network_device.hpp"
#include <arpa/inet.h> 

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include "bm_core/node.hpp"

namespace bm {
namespace core {

NetworkInterface::NetworkInterface( Node& node ) 
    : _node{ node }
{
    // Create network devices
    // for( auto& iface : interfaces )
    // {
    //     _net_devices.emplace_back( std::make_shared<NetworkDevice>( *this, iface ) );
    // }

    // Create IP Addresses
    _lla.__in6_u.__u6_addr32[0] = htonl(0xFD000000);
    _lla.__in6_u.__u6_addr32[1] = htonl(0x0);
    _lla.__in6_u.__u6_addr32[2] = htonl((_node.id() >> 32) & 0xFFFFFFFF);
    _lla.__in6_u.__u6_addr32[3] = htonl(_node.id() & 0xFFFFFFFF);

    _ula.__in6_u.__u6_addr32[0] = htonl(0xFE800000);
    _ula.__in6_u.__u6_addr32[1] = htonl(0x0);
    _ula.__in6_u.__u6_addr32[2] = htonl((_node.id() >> 32) & 0xFFFFFFFF);
    _ula.__in6_u.__u6_addr32[3] = htonl(_node.id() & 0xFFFFFFFF);
}

void NetworkInterface::send_bcmp_message( const std::string& dest_addr, uint8_t* data, size_t len )
{
    (void)dest_addr;
    (void)data;
    (void)len;
    // Choose which ports to send to based on dest_addr
    // Only allow the well defined multicast addresses for now

    // If dest_addr is multicast, set 


    // FIll ETH Header
    // _eth_header_out.h_dest

    // Fill IPv6 Header

    // Fill Payload

    // Calc FCS

    // Send
}

}
}