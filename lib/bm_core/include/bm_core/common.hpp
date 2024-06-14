#pragma once

#include <cstdint>

namespace bm {
namespace core {

// Constants
static constexpr size_t ETH_HEADER_BYTES = 14;
static constexpr size_t ETH_FCS_BYTES = 4;
static constexpr size_t BM_MTU = 1500;
static constexpr size_t BM_MAX_FRAME_SIZE = ETH_HEADER_BYTES + ETH_FCS_BYTES + BM_MTU;

// Types
typedef uint64_t NodeId;



}
}