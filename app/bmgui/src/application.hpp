#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>

#include <boost/asio.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// Ignore anonymous struct warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <imgui_toggle.h>
#pragma GCC diagnostic pop

#include "backend/node_instance.hpp"

struct AppConfig {
    std::string executable_path;
};

struct Connection {
    std::string State;
    bool isEnabled = false;
};

enum class EAppState : uint8_t {
    CONFIGURING = 0,
    ACTIVE
};

class Application {
public:
    Application( AppConfig config );
    virtual ~Application() = default;

    void run();

private:
    static constexpr auto _UPDATE_RATE_MS{std::chrono::milliseconds(1)};

    static constexpr int APP_WINDOW_DEFAULT_WIDTH = 1280;
    static constexpr int APP_WINDOW_DEFAULT_HEIGHT = 720;

    static constexpr int CONNECTION_WINDOW_WIDTH = 200;
    static constexpr int TOP_WINDOW_HEIGHT = 120;

    static constexpr int TARGET_FPS = 120;
    static constexpr std::chrono::milliseconds RENDER_TIMESTEP = std::chrono::milliseconds( 1000 / TARGET_FPS );

    void handle_signal(const boost::system::error_code& error, int signal_id);
    void update_handler(boost::system::error_code ec);

    void init();
    void init_imgui();

    void update();

    void draw_gui();

    void draw_permission_modal();

    void draw_node_config_window();

    void draw_connections_window();
    void on_new_connection_clicked();
    void draw_new_connection_modal();

    void draw_activity_window();

    void render();

    void cleanup();
    void cleanup_imgui();

    // Non-gui Methods
    void create_bm_node();

    // Attributes
    boost::asio::io_context _ioc;
    boost::asio::signal_set _signals;
    boost::asio::steady_timer _update_timer;

    AppConfig _config;

    // App state
    GLFWwindow* _window;
    int _window_width;
    int _window_height;

    std::chrono::steady_clock::time_point _next_render_time;

    EAppState _app_state;

    bool _has_net_caps;
    std::unordered_map<std::string, Connection> _connections;
    std::vector<std::string> _net_devices;
    NodeConfiguration _node_config;

    std::unique_ptr<NodeInstance> _node_instance;
};
