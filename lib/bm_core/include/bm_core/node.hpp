#pragma once

#include <vector>
#include <string>

#include "common.hpp"
#include "network_interface.hpp"

namespace bm {
namespace core {

class Node {
public:
    explicit Node( NodeId id, const std::vector<std::string>& interfaces );
    NodeId id() const { return _id; }

    NetworkInterface& net() { return _net_if; }

private:
    NodeId _id;
    NetworkInterface _net_if;
};

}
}