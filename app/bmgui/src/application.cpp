#include "application.hpp"

#include <spdlog/spdlog.h>

#include "util.hpp"

void glfw_error_callback( int error, const char* description )
{
    spdlog::error( "Glfw Error {}: {}", error, description );
}

Application::Application( AppConfig config )
    : _signals{ _ioc, SIGINT, SIGTERM }
    , _update_timer{ _ioc }
    , _config{ std::move( config ) }
    , _app_state{ EAppState::CONFIGURING }
{
}

void Application::handle_signal(const boost::system::error_code& error, int signal_id) {
    spdlog::info("Received signal: {}", signal_id);
    if (error) {
        spdlog::error("Signal error occurred: {}", error.message());
    }

    _ioc.stop();
}

void Application::run() 
{
    // Attach signal handler
    _signals.async_wait( [&]( auto ec, auto sig ) { handle_signal( ec, sig ); } );

    // Start update timer (fire once immediately upon starting)
    _update_timer.expires_from_now(std::chrono::milliseconds(0));
    _update_timer.async_wait([&](auto ec) { update_handler(ec); });

    init();

    // Run event loop
    spdlog::info("bmgui main loop: START");
    _ioc.run();
    spdlog::info("bmgui main loop: END");

    cleanup();
}

void Application::update_handler(boost::system::error_code ec) 
{
    if (ec) {
        spdlog::error("Error in timer: {}", ec.message());
        _ioc.stop();
        return;
    }

    // NOTE(Charles): Trade off more exact periodicity for benign back-pressure behavior
    // Can use expires_at( now+period ) to get consistent period, but needs special handling if it gets behind
    // Appears to drift roughly 1ms per 35 periods on my laptop with RT scheduling enabled when period=1ms
    _update_timer.expires_from_now(_UPDATE_RATE_MS);
    // _update_timer.expires_at(_update_timer.expiry() + _UPDATE_RATE_MS);

    update();

    // Reschedule
    _update_timer.async_wait([&](auto ec) { update_handler(ec); });
}

void Application::init()
{
    init_imgui();

    // Check if the application has the required capabilities
    _has_net_caps = bmgui::check_capabilities();

    _net_devices = bmgui::get_available_raw_socket_ifaces();
    _connections = {};
    _node_config = { 0xDEADBEEFDEAD0001ULL, false };

    _next_render_time = std::chrono::steady_clock::now();
}

void Application::init_imgui()
{
    // Setup window
    glfwSetErrorCallback( glfw_error_callback );
    if( !glfwInit() )
    {
        throw std::runtime_error( "Failed to initialize glfw" );
    }

    // Create window with graphics context
    _window = glfwCreateWindow( APP_WINDOW_DEFAULT_WIDTH, APP_WINDOW_DEFAULT_HEIGHT, "bmgui", NULL, NULL );
    if( !_window )
    {
        throw std::runtime_error( "Failed to create glfw window" );
    }

    glfwMakeContextCurrent( _window );

     // Enable vsync
    glfwSwapInterval(1);

    // Initialize OpenGL loader (GLAD in this case)
    if( !gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) )
    {
        throw std::runtime_error( "Failed to initialize OpenGL loader" );
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL( _window, true );
    ImGui_ImplOpenGL3_Init( "#version 130" );
}

void Application::update()
{
    auto now = std::chrono::steady_clock::now();

    if( glfwWindowShouldClose( _window ) )
    {
        spdlog::info( "Got request for window to close" );
        _ioc.stop();
        return;
    }

    // TODO: Handle non-gui events/logic here (comms events, periodic things, etc)

    // Update GUI roughly at the prescribed framerate (avoids spending CPU needlessly)
    if( now - _next_render_time >= RENDER_TIMESTEP ) 
    {
        // Handle GUI events
        glfwPollEvents();

        // Draw GUI
        draw_gui();

        // Render to window
        render();

        _next_render_time = now + RENDER_TIMESTEP;
    }
}

void Application::draw_gui()
{
    // Start the ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Get the current dimensions of the window
    glfwGetFramebufferSize( _window, &_window_width, &_window_height );

    // Show restart modal if we don't have the required capabilities
    if( !_has_net_caps )
    {
        draw_permission_modal();
    }
    else
    {
        // Configuration window
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)_window_width, TOP_WINDOW_HEIGHT));
        draw_node_config_window();

        // Connections window
        ImGui::SetNextWindowPos(ImVec2(0, TOP_WINDOW_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2(CONNECTION_WINDOW_WIDTH, (float)_window_height - TOP_WINDOW_HEIGHT));
        draw_connections_window();

        // Dynamic-width window on the right
        ImGui::SetNextWindowPos(ImVec2(CONNECTION_WINDOW_WIDTH, TOP_WINDOW_HEIGHT));
        ImGui::SetNextWindowSize(ImVec2((float)_window_width - CONNECTION_WINDOW_WIDTH, (float)_window_height - TOP_WINDOW_HEIGHT));
        draw_activity_window();
    }
}

void Application::draw_permission_modal()
{
    ImGui::OpenPopup("Restart Required");

    if (ImGui::BeginPopupModal("Restart Required", NULL, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        ImGui::Text("The application needs additional permissions to run.");
        ImGui::Text("Would you like to restart with elevated privileges?");
        
        if (ImGui::Button("Restart")) 
        {
            ImGui::CloseCurrentPopup();
            bmgui::restart_with_sudo_net_caps( _config.executable_path.c_str() );
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) 
        {
            ImGui::CloseCurrentPopup();
            glfwSetWindowShouldClose( _window, GLFW_TRUE );
        }

        ImGui::EndPopup();
    }
}

void Application::draw_node_config_window()
{
    // NOTE: Right now, we can never return to this state. We set this configuration once and then use it for the rest of the app's lifetime
    // This design can be revisited later, if needed

    ImGui::Begin("Node Configuration", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // Disable controls if needed
    if( _app_state != EAppState::CONFIGURING ) 
    {
        ImGui::BeginDisabled( true );
    }

    ImGui::Text("Node Configuration");

    if( ImGui::BeginTable("NodeConfigTable", 3, ImGuiTableFlags_Borders) ) 
    {
        ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("Node ID");

        ImGui::TableSetColumnIndex(1);
        char node_id_str[17];
        std::string node_id_hex = bmgui::uint64_to_hex_string( _node_config.node_id );
        std::strncpy( node_id_str, node_id_hex.c_str(), sizeof( node_id_str ) );
        node_id_str[16] = '\0'; // Ensure null-terminated

        if (ImGui::InputText("##node_id", node_id_str, sizeof(node_id_str), ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase)) 
        {
            std::stringstream ss;
            ss << std::hex << node_id_str;
            ss >> _node_config.node_id;
        }

        ImGui::TableSetColumnIndex(2);
        if (ImGui::Button("Random")) 
        {
            _node_config.node_id = bmgui::generate_random_node_id();
        }

        ImGui::EndTable();
    }

    ImGui::Dummy( ImVec2(0.0f, 10.0f) );

    if (ImGui::Button("Start Node")) 
    {
        // We have the required config to now create the node
        // NOTE: Network interfaces do not need to be known at node creation time
        _app_state = EAppState::ACTIVE;
        create_bm_node();
    }

    // Re-enable controls if they were disabled
    if( _app_state != EAppState::CONFIGURING ) 
    {
        ImGui::EndDisabled();
    }

    ImGui::End();
}

void Application::draw_connections_window()
{
    ImGui::Begin( "Connections", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse );
    
    // Disable controls if needed
    if( _app_state != EAppState::ACTIVE ) 
    {
        ImGui::BeginDisabled( true );
    }

    // No need to draw UI if there is no node instance yet
    if( _node_instance )
    {
        ImGui::Text("Net Devices");
    
        if (ImGui::BeginTable("Active Connections", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) 
        {
            // Set column widths
            ImGui::TableSetupColumn("Interface", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 40.0f); // Adjust the width as needed
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 15.0f); // Adjust the width as needed
            ImGui::TableHeadersRow();

            std::vector<std::string> toDelete;

            for (auto& [name, connection] : _connections) {
                ImGui::TableNextRow();

                // Interface Text
                ImGui::TableSetColumnIndex(0);
                ImGui::Dummy(ImVec2(0.0f, 2.0f)); 
                ImGui::Text("%s", name.c_str());

                // Toggle Switch for State using ImGui::Toggle
                ImGui::TableSetColumnIndex(1);
                ImGui::Dummy(ImVec2(0.0f, 2.0f)); 
                std::string toggleId = "##Enable##" + name;
                ImGui::Toggle(toggleId.c_str(), &connection.isEnabled);

                // Button
                ImGui::TableSetColumnIndex(2);
                ImGui::Dummy(ImVec2(0.0f, 2.0f)); 
                std::string buttonId = "X##" + name;
                if (ImGui::Button(buttonId.c_str())) {
                    bool ret = _node_instance->delete_netdev( name );
                    if( ret )
                    {
                        // Update local copy of connection list
                        toDelete.push_back(name);
                    }
                }
            }

            ImGui::EndTable();

            // Delete connections after the loop to avoid invalidating the iterator
            for (const std::string& name : toDelete) {
                _connections.erase(name);
            }
        }

        // Add padding below the table
        ImGui::Dummy(ImVec2(0.0f, 10.0f)); 

        // Centered "Add Connection" Button
        float buttonWidth = ImGui::CalcTextSize("Add New Connection").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        float buttonPosX = (ImGui::GetWindowSize().x - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(buttonPosX);
        if (ImGui::Button("Add New Connection")) 
        {
            on_new_connection_clicked();
        }
        draw_new_connection_modal();
    }

    // Re-enable controls if they were disabled
    if( _app_state != EAppState::ACTIVE ) 
    {
        ImGui::EndDisabled();
    }

    ImGui::End();
}

void Application::on_new_connection_clicked()
{
    // Refresh the list when button is clicked
    _net_devices = bmgui::get_available_raw_socket_ifaces(); 
    ImGui::OpenPopup("Add Connection");
}

void Application::draw_new_connection_modal()
{
    if (ImGui::BeginPopupModal("Add Connection", NULL, ImGuiWindowFlags_AlwaysAutoResize)) 
    {
        static int selectedInterface = 0;

        ImGui::Text("Select Interface:");

        // Filter interfaces that are not already in activeConnections
        std::vector<const char*> filteredInterfaces;
        for (const auto& iface : _net_devices) {
            if (_connections.find(iface) == _connections.end()) {
                filteredInterfaces.push_back(iface.c_str());
            }
        }

        if (!filteredInterfaces.empty()) {
            // Dropdown list
            ImGui::Combo("##interfaces", &selectedInterface, filteredInterfaces.data(), filteredInterfaces.size());

            // Move the refresh button to the right of the dropdown
            ImGui::SameLine();
            if (ImGui::Button("Refresh")) {
                _net_devices = bmgui::get_available_raw_socket_ifaces(); // Refresh the list on button click
                selectedInterface = 0; // Reset the selection
            }

            // Add some spacing and a separator before the buttons
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Align OK/Cancel buttons to the bottom right
            float buttonWidth = ImGui::CalcTextSize("Add").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float cancelWidth = ImGui::CalcTextSize("Cancel").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float totalWidth = buttonWidth + cancelWidth + ImGui::GetStyle().ItemSpacing.x;
            float availableWidth = ImGui::GetContentRegionAvail().x;

            ImGui::SetCursorPosX(availableWidth - totalWidth);

            if (ImGui::Button("Add")) {
                bool ret = _node_instance->create_netdev( NetDeviceInfo{ ENetDeviceType::NETWORK, filteredInterfaces[ selectedInterface ], false } );
                if( ret )
                {
                    // Update local copy of connection list
                    _connections[filteredInterfaces[selectedInterface]] = Connection{ "NULL", false };
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
        } 
        else 
        {
            ImGui::Text("No available interfaces.");
            if (ImGui::Button("Refresh")) {
                _net_devices = bmgui::get_available_raw_socket_ifaces(); // Refresh the list on button click
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void Application::draw_activity_window()
{
    ImGui::Begin("ActivityWindow", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    
    // Disable controls if needed
    if( _app_state != EAppState::ACTIVE ) 
    {
        ImGui::BeginDisabled( true );
    }

    if (ImGui::BeginTabBar("ActivityTabs"))
    {
        if (ImGui::BeginTabItem("Node Info"))
        {
            ImGui::Text("My node information");
            // Identifiers
            // MAC addresses
            // IP addresses
            // Other stats

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Peers"))
        {
            ImGui::Text("Discovered Peers");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BCMP"))
        {
            ImGui::Text("Exercise BCMP functions and see logging for BCMP traffic");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Raw Send/Recieve"))
        {
            ImGui::Text("Send and Receive packets without the middleware layer");
            // UDP send
                // link local
                // global multicast
                // routed multicast
            // UDP recv
                // types
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Middleware"))
        {
            // Allow user to create publications, subscriptions, services, clients, see discovered resources, etc
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // Re-enable controls if they were disabled
    if( _app_state != EAppState::ACTIVE ) 
    {
        ImGui::EndDisabled();
    }

    ImGui::End();
}

void Application::render()
{
    // Rendering
    ImGui::Render();
    glViewport( 0, 0, _window_width, _window_height );
    glClearColor( 0.45f, 0.55f, 0.60f, 1.00f );
    glClear( GL_COLOR_BUFFER_BIT );
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

    glfwSwapBuffers( _window );
}

void Application::cleanup()
{
    cleanup_imgui();
}

void Application::cleanup_imgui()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow( _window );
    glfwTerminate();
}

void Application::create_bm_node()
{
    _node_instance = std::make_unique<NodeInstance>( _node_config );
}