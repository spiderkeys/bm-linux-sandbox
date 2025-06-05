#include "raw_socket_device.hpp"

#include <spdlog/spdlog.h>

RawSocketDevice::RawSocketDevice( NetDeviceInfo info )
    : _data{ info }
{
    spdlog::info( "Created RawSocketDevice" );
}

RawSocketDevice::~RawSocketDevice()
{
    spdlog::info( "Destroyed RawSocketDevice" );
}

const NetDeviceInfo& RawSocketDevice::info() const
{
    return _data;
}