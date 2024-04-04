#pragma once

#include <memory>
#include <atomic>

#include <boost/asio.hpp>


#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"  // for Button, Horizontal, Renderer
#include "ftxui/component/component_base.hpp"      // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, separator, Element, operator|, vbox, border

#include <bm_core/node.hpp>

enum class EAppMode {
    PUBLISHER,
    SUBSCRIBER
};

class ExampleApp
{
public:

    explicit ExampleApp( const std::string& node_id, const std::string& interface, const std::string& mode );
    virtual ~ExampleApp();

    void exit();
    void run();

private:
    static constexpr std::chrono::milliseconds UPDATE_RATE_MS{ 10 };

    void handle_signal( const boost::system::error_code& error, int signal_id );
    void update_handler( const boost::system::error_code& ec );

    void update();

    void process_frame( char* data, ssize_t len );

    void send_bcmp_heartbeat();


    // Attributes
    std::atomic<bool> exit_;

    boost::asio::io_context         _ioc;
    boost::asio::signal_set         _signals;
    boost::asio::steady_timer       _update_timer;

    uint64_t                        _node_id;
    std::string                     _net_if_id;
    EAppMode                        _app_mode;

    std::atomic<int>                _counter_0;
    std::atomic<int>                _counter_1;

    ftxui::ScreenInteractive screen_;

    ftxui::Component layout_;
    std::vector<std::string> menu_entries_;

    std::unique_ptr<bm::core::Node> _node;
    int selected_ = 0;

    std::vector<std::string> log_buffer_;

    std::thread ftxui_thread_;
    std::thread reader_thread_;

    std::chrono::steady_clock::time_point _last_hb;

    char input_buffer_[1600];
    char output_buffer_[1600];
    
};