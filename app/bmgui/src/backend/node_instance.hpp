#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include <bm_core/node.hpp>

#include "raw_socket_device.hpp"

struct NodeConfiguration {
    uint64_t node_id = 0;
    bool started = false;
};

class NodeInstance {
public:
    explicit NodeInstance( const NodeConfiguration& config );
    virtual ~NodeInstance();

    void start();
    void update();
    void stop();

    bool create_netdev( NetDeviceInfo info );
    bool delete_netdev( std::string uri );

    const std::vector<std::shared_ptr<RawSocketDevice>>& net_devices() const;

private:

    NodeConfiguration _config;

    std::shared_ptr<bm::core::Node> _bm_node;
    std::vector<std::shared_ptr<RawSocketDevice>> _net_devices;
};