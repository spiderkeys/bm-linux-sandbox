#include "bm_core/node.hpp"

namespace bm {
namespace core {

Node::Node( NodeId id, const std::vector<std::string>& interfaces )
    : _id{ id }
    , _net_if{ *this, interfaces }
{
}

}
}