#include <spdlog/spdlog.h>
#include <boost/program_options.hpp>

#include "example_app.hpp"

// -n 0xDEADBEEFDEAD0001 -i enx001ec0d1b1a9 -m pub
// -n 0xDEADBEEFDEAD0002 -i enx001ec0d1c003 -m sub

namespace po = boost::program_options;

auto set_log_level( const std::string& level ) -> void;

auto main(int argc, char ** argv) -> int
{
    int ret = EXIT_SUCCESS;

    // Force flush of the stdout buffer.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    try
    {
        // Program arguments
        po::options_description options( "App Options:" );
        po::variables_map 		arg_map;

        // Setup options
        options.add_options()
            ( "help,h",         "Show help." )
            ( "log-level,l",    po::value<std::string>()->default_value( "info" ),  "Logging level" )
            ( "node-id,n",      po::value<std::string>()->default_value( "" ),      "64-bit Node ID" )
            ( "interface,i",    po::value<std::string>()->required(),               "Network Interface ID to create Raw Socket on (ex: 'eth0')" )
            ( "mode,m",         po::value<std::string>()->required(),               "ExampleApp mode [pub/sub]" );

        // Parse command line options
        po::store( po::parse_command_line( argc, argv, options ), arg_map );
        po::notify( arg_map );

        if( arg_map.count( "help" ) )
        {
            std::stringstream ss;
            ss << options;
            spdlog::error( "{}", ss.str() );

            ret = EXIT_FAILURE;
        }
        else
        {
            set_log_level( arg_map["log-level"].as<std::string>() );

            auto node_id    = arg_map["node-id"].as<std::string>();
            auto interface  = arg_map["interface"].as<std::string>();
            auto mode       = arg_map["mode"].as<std::string>();

            // Create node
            auto app = std::make_shared<ExampleApp>( node_id, interface, mode );

            app->run();
        }
    }
    catch( const po::error& e )
    {
        spdlog::critical( "Error processing program args: \n\t{}", e.what() );
        ret = EXIT_FAILURE;
    }
    catch( const std::exception& e )
    {
        // Notify and rethrow unhandled exceptions
        spdlog::critical( "Unhandled exception encountered: \n\t{}", e.what() );
        throw;
    }

    return ret;
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