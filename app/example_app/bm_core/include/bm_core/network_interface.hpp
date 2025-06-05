#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <thread>

#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netinet/ip6.h>

#include "neighbor_table.hpp"

namespace bm {
namespace core {

class Node;
class NetworkDevice;

class NetworkInterface {
public:
    static constexpr uint16_t IP_PROTO_BCMP = (0xBC);

    NetworkInterface( Node& node, const std::vector<std::string>& interfaces );
    auto& devs() { return _net_devices; }
    std::shared_ptr<NetworkDevice> dev( size_t i ){ return _net_devices[ i ]; }
    // Send BCMP Message
     // Takes a destination, data, len

    // Send UDPv6 Message
     // Takes an address, resource ID

    // Transmit a Bristlemouth packet (IPv6 payload with Bristlemouth-conforming MAC header + IPv6 Header)
    int bm_tx( const uint8_t* data, size_t len );

    // BCMP functions


private:
    void send_bcmp_message( const std::string& dest_addr, uint8_t* data, size_t len );

    Node& _node;
    std::vector<std::shared_ptr<NetworkDevice>> _net_devices;

    in6_addr _lla;
    in6_addr _ula;

    ethhdr _eth_header_out;
    ip6_hdr _ipv6_header_out;

    NeighborTable _neighbors;

    std::thread _work_thread;
    std::thread _rx_thread;
};

}
}