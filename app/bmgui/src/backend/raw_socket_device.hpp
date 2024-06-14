#pragma once

#include "net_device.hpp"

#include <bm_core/common.hpp>

#include <functional>
#include <atomic>
#include <thread>
#include <array>

enum class ENetDeviceEvent {
    UNKNOWN = 0,
    FRAME_RECEIVED,             // Received valid frame
    FRAME_ERROR_FCS,            // Received frame that failed FCS check
    FRAME_ERROR_INVALID,        // Received that was invalid per other requirements
    INCOMING_FRAME_DROPPED,     // RX buffers full, dropped frame
    DEVICE_ERROR                // Generic error with network device (disconnected etc)
};

class RawSocketDevice {
public:
    explicit RawSocketDevice( NetDeviceInfo info );
    virtual ~RawSocketDevice();

    const NetDeviceInfo& info() const;

    void enable();
    void disable();

private:
    void rx_thread_function();

    NetDeviceInfo _data;

    std::atomic<bool> _terminate;
    std::thread _rx_thread;
    std::function<void(const uint8_t* data, size_t len)> _on_rx_cb;

    std::array<uint8_t, bm::core::BM_MAX_FRAME_SIZE> _tx_frame_buffer;
    std::array<uint8_t, bm::core::BM_MAX_FRAME_SIZE> _rx_frame_buffer;
};