#pragma once

#include <string>
#include <memory>
#include <array>

#include <linux/if_packet.h>
#include <linux/if_ether.h>

namespace bm {
namespace core {

class NetworkInterface;

struct NetDeviceInfo {
    std::string if_name;
    uint8_t mac_address[6];
};

class NetworkDevice {
public:
    static constexpr size_t ETH_HEADER_BYTES = 14;
    static constexpr size_t ETH_FCS_BYTES = 4;
    static constexpr size_t BM_MTU = 1500;
    static constexpr size_t BM_MAX_FRAME_SIZE = ETH_HEADER_BYTES + ETH_FCS_BYTES + BM_MTU;

    explicit NetworkDevice( NetworkInterface& net_if, const std::string& interface );
    virtual ~NetworkDevice();

    NetDeviceInfo info() const;

    ssize_t write_frame( const char* buffer, size_t len );
    ssize_t read_frame( char* buffer, size_t len );

private:
    
    NetworkInterface& _net_if;

    std::array<uint8_t, BM_MAX_FRAME_SIZE> _output_buffer;
    std::array<uint8_t, BM_MAX_FRAME_SIZE> _input_buffer;

    int             _sock_fd;
    sockaddr_ll     _sock_addr;
    NetDeviceInfo   _info;
};

}
}