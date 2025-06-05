#include "node_instance.hpp"

#include <algorithm>


#include <spdlog/spdlog.h>

#include "util.hpp"

NodeInstance::NodeInstance( const NodeConfiguration& config )
    : _config{ config }
{
    spdlog::info( "Created Node instance with ID: '{}'", bmgui::uint64_to_hex_string( _config.node_id ) );
}

NodeInstance::~NodeInstance()
{
    try
    {
        // Shutdown node if needed
        stop();
    }
    catch( ... )
    {

    }
}

void NodeInstance::start()
{
    // Create BM Node
    _bm_node = std::make_unique<bm::core::Node>( _config.node_id );
}

void NodeInstance::update()
{

}

void NodeInstance::stop()
{

}

const std::vector<std::shared_ptr<RawSocketDevice>>& NodeInstance::net_devices() const
{
    return _net_devices;
}

bool NodeInstance::create_netdev( NetDeviceInfo info )
{
    spdlog::info( "Creating NetDevice of type={} with uri='{}'", static_cast<int>(info.type), info.uri );

    // Check if the device already exists
    auto it = std::find_if( _net_devices.begin(), _net_devices.end(),
        [&info](const auto& device) 
        {
            return device->info().uri == info.uri;
        }
    );

    // If the device does not exist, add it to the vector
    if (it == _net_devices.end()) 
    {
        _net_devices.push_back( std::make_shared<RawSocketDevice>( info ) );
        return true;
    } 
    else 
    {
        spdlog::error( "Device already exists" );
    }

    // Create Network Device based on type
    return false;
}

bool NodeInstance::delete_netdev( std::string uri )
{
    // Get iterator to matching element if it exists and remove from iterator range
    auto it = std::remove_if(_net_devices.begin(), _net_devices.end(),
    [&uri](const auto& device) {
        return device->info().uri == uri;
    });
    
    if( it != _net_devices.end() ) 
    {
        // Delete specified element from the vector
        _net_devices.erase( it, _net_devices.end() );
        return true;
    } 
    else 
    {
        spdlog::error( "NetDevice does not exist" );
        return false;
    }
}