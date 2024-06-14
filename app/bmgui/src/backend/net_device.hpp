#pragma once

#include <cstdint>
#include <string>

enum class ENetDeviceType : uint8_t {
    UNKNOWN = 0,
    NETWORK,
    SERIAL,
    SPI
};

struct NetDeviceInfo {
    ENetDeviceType  type;       // For now, only NETWORK
    std::string     uri;        // In linux, netif name (i.e. 'eth0')
    bool            enabled;

    // uint8_t mac_address[6];
};