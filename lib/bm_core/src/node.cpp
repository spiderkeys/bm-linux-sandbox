#include "bm_core/node.hpp"

namespace bm {
namespace core {

Node::Node( NodeId id )
    : _id{ id }
    , _net_if{ *this }
{
}

}
}