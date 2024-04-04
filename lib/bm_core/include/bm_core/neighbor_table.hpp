#pragma once

#include "common.hpp"

#include <chrono>
#include <unordered_map>

namespace bm {
namespace core {

struct NeighborEntry {
    NodeId      node_id;
    uint8_t     remote_egress_port;
    uint8_t     local_ingress_port;
    uint32_t    liveliness_lease_dur_ms;

    std::chrono::steady_clock::time_point last_heartbeat;
};

class NeighborTable {
public:
    void insert( NeighborEntry entry );
    bool find( NodeId id, NeighborEntry& entry );

    std::unordered_map<NodeId, NeighborEntry>& neighbors() { return _neighbors; }

private:
    std::unordered_map<NodeId, NeighborEntry> _neighbors;
};

}
}