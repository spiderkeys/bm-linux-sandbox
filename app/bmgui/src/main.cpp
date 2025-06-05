#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include <iostream>

#include "application.hpp"

namespace po = boost::program_options;

auto set_log_level( const std::string& level ) -> void;

auto main(int argc, char ** argv) -> int
{
    // Force flush of the stdout buffer.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    spdlog::info("-----------------------------------------");
    spdlog::info("APPLICATION START");

    // Program arguments
    po::options_description options( "App Options:" );

    // Setup options
    options.add_options()
        ( "help,h",         "Show help." )
        // Node Parameters
        ( "log-level,l",    po::value<std::string>()->default_value( "info" ),  "Logging level" );

    po::variables_map arg_map;
    try
    {
        // Parse command line options
        po::store( po::parse_command_line( argc, argv, options ), arg_map );
        po::notify( arg_map );

        if( arg_map.count( "help" ) )
        {
            std::cout << options << std::endl;
            return 1;
        }
        else
        {
            set_log_level( arg_map["log-level"].as<std::string>() );

            AppConfig app_config;
            app_config.executable_path = argv[0];

            // Create node
            auto app = std::make_shared<Application>( app_config );
            app->run();
        }

    } catch (const po::error& e) {
        spdlog::critical("Error parsing command line:\n{}", e.what());
        return 1;
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception in main:\n{}", e.what());
        return 1;
    }

    spdlog::info("APPLICATION END");
    return 0;
}

auto set_log_level( const std::string& level ) -> void
{
    if( level == "off" )
    {
        spdlog::set_level( spdlog::level::off ); return;
    }
    if( level == "critical" )
    {
        spdlog::set_level( spdlog::level::critical ); return;
    }
    if( level == "error" )
    {
        spdlog::set_level( spdlog::level::err ); return;
    }
    if( level == "warn" )
    {
        spdlog::set_level( spdlog::level::warn ); return;
    }
    if( level == "info" )
    {
        spdlog::set_level( spdlog::level::info ); return;
    }
    if( level == "debug" )
    {
        spdlog::set_level( spdlog::level::debug ); return;
    }
    if( level == "trace" )
    {
        spdlog::set_level( spdlog::level::trace ); return;
    }

    spdlog::error( "Unsupported log level, defaulting to info" );
    spdlog::set_level( spdlog::level::info );
}