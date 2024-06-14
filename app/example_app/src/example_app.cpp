#include "example_app.hpp"

#include <spdlog/spdlog.h>

#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>
#include <mutex>

#include <spdlog/fmt/bin_to_hex.h>
#include <bm_core/network_device.hpp>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <linux/if.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>

#include <bm_core/bcmp_messages.hpp>

#include "lwip_pseudo.hpp"

#define BM_MIDDLEWARE_PORT 4321
#define STRESS_TEST_PORT 12357
#define BM_BCL_PORT 2222

// char input_buffer_[1600];
// char output_buffer_[1600];

// Polynomial used for Ethernet CRC-32
constexpr uint32_t CRC32_POLYNOMIAL = 0xEDB88320;

// Function to initialize the CRC table
std::array<uint32_t, 256> init_crc32_table() {
    std::array<uint32_t, 256> table;
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

// Function to calculate the Ethernet FCS
uint32_t ethernet_fcs(const uint8_t* data, size_t length) {
    static const auto crc_table = init_crc32_table();
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        uint8_t pos = (data[i] ^ crc) & 0xFF;
        crc = crc_table[pos] ^ (crc >> 8);
    }
    return ~crc;
}


bool recv_bcmp_message(const uint8_t* buffer, size_t length, bcmp_header_t& header, bcmp_heartbeat_t& heartbeat) {
  // Make sure the buffer is at least large enough for the header
  if (length < sizeof(bcmp_header_t)) {
    spdlog::error("Buffer too small for BCMP header.");
    return false;
  }

  // Copy the header from the buffer
  std::memcpy(&header, buffer, sizeof(bcmp_header_t));

  // Validate the type to ensure it's a heartbeat message, assuming 0xBC is the type for heartbeat
  if (header.type != 0x01) {
    spdlog::error("Not a BCMP heartbeat message.");
    return false;
  }

  // Make sure the buffer is large enough for the entire heartbeat message
  size_t expected_size = sizeof(bcmp_header_t) + sizeof(bcmp_heartbeat_t);
  if (length < expected_size) {
    spdlog::error("Buffer too small for BCMP heartbeat message.");
    return false;
  }

  // Copy the heartbeat payload from the buffer
  std::memcpy(&heartbeat, buffer + sizeof(bcmp_header_t), sizeof(bcmp_heartbeat_t));

  spdlog::info( "Heartbeat: {}", spdlog::to_hex(buffer, buffer + 18) );

  return true;
}

bool parse_packet(const uint8_t* buffer, size_t buffer_length, ethhdr& ethernetHeader, ip6_hdr& ipv6Header, udphdr& udpHeader) {
    


    if (buffer_length < sizeof(ethhdr)) {
        spdlog::warn( "DROP: Too short to be ethernet frame" );
        return false;
    }

    // Copy Ethernet header from buffer
    std::memcpy(&ethernetHeader, buffer, sizeof(ethhdr));

    // spdlog::info( "t: {}, dst: {} src: {}",
    //     ntohs( ethernetHeader.h_proto ), 
    //     spdlog::to_hex(ethernetHeader.h_dest, ethernetHeader.h_dest+6), 
    //     spdlog::to_hex(ethernetHeader.h_source, ethernetHeader.h_source+6) );
    
    if (ethernetHeader.h_proto != htons(0x86DD)) { // Check if the EtherType is IPv6
        spdlog::warn( "DROP: Ethertype not IPV6" );
        return false;
    }

    if (buffer_length < sizeof(ethhdr) + sizeof(ip6_hdr)) {
        spdlog::warn( "DROP: Too short to be IPV6 packet" );
        return false;
    }

    // Copy IPv6 header from buffer
    std::memcpy(&ipv6Header, buffer + sizeof(ethhdr), sizeof(ip6_hdr));
    // spdlog::info( "ipv6 proto: {}", ipv6Header.ip6_ctlun.ip6_un1.ip6_un1_nxt );
    if( ipv6Header.ip6_ctlun.ip6_un1.ip6_un1_nxt == 0xBC ){
        spdlog::info( "Got BCMP packet of len={}", ntohs(ipv6Header.ip6_ctlun.ip6_un1.ip6_un1_plen) );

        bcmp_header_t bcmp_hdr{};
        bcmp_heartbeat_t bcmp_hb{};
        
        bool bcmp_ret = recv_bcmp_message( buffer + sizeof(ethhdr) + sizeof(ip6_hdr), ntohs(ipv6Header.ip6_ctlun.ip6_un1.ip6_un1_plen), bcmp_hdr, bcmp_hb );
        if( bcmp_ret )
        {
            spdlog::info( "got bcmp heartbeat: {}", (uint64_t)bcmp_hb.time_since_boot_us );
        }
        return bcmp_ret;
    }
    if (ipv6Header.ip6_ctlun.ip6_un1.ip6_un1_nxt != 17) { // Check if the Next Header is UDP (17)
        spdlog::warn( "DROP: Not UDP packet:" );
        return false;
    }

    if (buffer_length < sizeof(ethhdr) + sizeof(ip6_hdr) + sizeof(udphdr)) {
        spdlog::warn( "DROP: Too short to be UDP packet" );
        return false;
    }

    // Copy UDP header from buffer
    std::memcpy(&udpHeader, buffer + sizeof(ethhdr) + sizeof(ip6_hdr), sizeof(udphdr));

    return true;
}

template<typename Mutex>
class FTXUISink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit FTXUISink(std::vector<std::string>& log_buffer)
        : log_buffer_(log_buffer) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        log_buffer_.push_back(fmt::to_string(formatted));
        // Make sure to remove old log lines if the buffer becomes too big
        if(log_buffer_.size() > max_lines_) {
            log_buffer_.erase(log_buffer_.begin(), log_buffer_.end() - max_lines_);
        }
    }

    void flush_() override {
        // No need to implement for this example
    }

private:
    std::vector<std::string>& log_buffer_;
    size_t max_lines_ = 100;  // maximum number of lines to keep in the buffer
};

using FTXUIThreadSafeSink = FTXUISink<std::mutex>;

namespace {
    bool parse_hex_uint64( const std::string& input, uint64_t& result ) 
    {
        std::istringstream iss( input );
        iss >> std::hex >> result;
        return !iss.fail() && iss.eof();
    }
}

ExampleApp::ExampleApp( const std::string& node_id, const std::string& interface, const std::string& mode )
    : exit_{false}
    , _signals{ _ioc, SIGINT, SIGTERM }
    , _update_timer{ _ioc, std::chrono::milliseconds( 0 ) }
    , _net_if_id{ interface }
    , screen_(ftxui::ScreenInteractive::TerminalOutput()) 
{

    // Set up the spdlog with the custom sink
    std::vector<spdlog::sink_ptr> sinks;
    auto custom_sink = std::make_shared<FTXUIThreadSafeSink>(log_buffer_);
    sinks.push_back(custom_sink);
    auto logger = std::make_shared<spdlog::logger>("logger", begin(sinks), end(sinks));
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);

    // Initialize the menu entries.
    menu_entries_ = {
            "Increment (+1)",
            "Decrement (-1)",
            "Exit"
        };
        

    // Create the counter display.
    auto counter_display = ftxui::Renderer([&] {
        return ftxui::vbox({
            ftxui::text("Manual value: ") | ftxui::bold,
            ftxui::text(std::to_string(_counter_0.load())) | ftxui::color(ftxui::Color::Green),
            ftxui::separator(),
            ftxui::text("Auto counter: ") | ftxui::bold,
            ftxui::text(std::to_string(_counter_1.load())) | ftxui::color(ftxui::Color::Blue),
        }) | ftxui::border;
    });

    ftxui::MenuOption menu_option;
    menu_option.on_enter = [this] {
        switch (selected_) {
            case 0: _counter_0++; spdlog::info( "inc" ); break;
            case 1: _counter_0--; break;
            case 2: this->exit(); break;
        }
    };

    auto menu_component = ftxui::Menu(&menu_entries_, &selected_, menu_option) | ftxui::border;

    int log_index = 0;
    auto log_menu = ftxui::Menu(&log_buffer_, &log_index) | ftxui::border;
    auto log_menu_decorated = ftxui::Renderer(log_menu, [&] {
        if( (size_t)log_index < log_buffer_.size() )
        {
            int begin = std::max(0, log_index - 9);
            int end = std::min(log_buffer_.size(), (long unsigned)log_index + 8);
            ftxui::Elements elements;
            for(int i = begin; i<end; ++i) {
                ftxui::Element element = ftxui::text(log_buffer_[i]);
                if (i == log_index)
                    element = element | ftxui::inverted | ftxui::select;
                elements.push_back(element);
            }
            return vbox(std::move(elements)) | ftxui::vscroll_indicator | ftxui::frame | ftxui::border;
        }
        else
        {
            int span = std::min( 10ul , log_buffer_.size() );
            int begin = log_buffer_.size() - span;
            int end = log_buffer_.size();

            ftxui::Elements elements;
            for(int i = begin; i<end; ++i) {
                ftxui::Element element = ftxui::text(log_buffer_[i]);
                if ((size_t)i == log_buffer_.size())
                    element = element | ftxui::inverted | ftxui::select;
                elements.push_back(element);
            }
            return vbox(std::move(elements)) | ftxui::vscroll_indicator | ftxui::frame | ftxui::border;
        }
        });

    layout_ = ftxui::Container::Vertical({ log_menu_decorated, counter_display, menu_component });

    // Process arguments
    
    // Node ID
    if( !parse_hex_uint64( node_id, _node_id ) ) {
        throw std::invalid_argument( "Node ID argument can not be parsed as a valid hex 64-bit integer" );
    }

    // Network Interface
    if( if_nametoindex( _net_if_id.c_str() ) == 0 ) {
        throw std::invalid_argument( "Network Interface '" + interface + "' does not exist" );
    }

    // Mode
    if( mode == "pub" ) {
        _app_mode = EAppMode::PUBLISHER;
    }
    else if( mode == "sub" ){
        _app_mode = EAppMode::SUBSCRIBER;
    }
    else {
        throw std::invalid_argument( "Unsupported app mode: " + mode );
    }

    // Create BM Node
    _node = std::make_unique<bm::core::Node>( _node_id, std::vector<std::string>( { _net_if_id } ) );
}

ExampleApp::~ExampleApp() {
    exit_ = true;
    if (reader_thread_.joinable()) 
    {
        reader_thread_.join();
    }
    if (ftxui_thread_.joinable()) {
        ftxui_thread_.join();
    }
}

void ExampleApp::handle_signal( const boost::system::error_code& error, int signal_id )
{
    spdlog::info( "Received signal: {}", signal_id );
    if( error )
    {
        spdlog::error( "Signal error occurred: {}", error.message() );
    }

    _ioc.stop();
}

void ExampleApp::exit() {
    exit_ = true;
    screen_.ExitLoopClosure()();
    _ioc.stop();
}

void ExampleApp::run()
{
    spdlog::info( "APPLICATION START" );

    // Attach signal handler
    _signals.async_wait( [&](auto ec, auto sig){  handle_signal( ec, sig ); } );

    // Start update timer
    _update_timer.async_wait( [&]( auto ec ){ update_handler( ec ); } );

    ftxui_thread_ = std::thread([&] {
        screen_.Loop(layout_);

        exit();
    });

    reader_thread_ = std::thread([&] {
        while (!exit_) {
            auto ret = _node->net().dev(0)->read_frame( input_buffer_, bm::core::NetworkDevice::BM_MAX_FRAME_SIZE );
            if( ret > 0 ){
                process_frame( input_buffer_, ret );
            }
        }
    });

    _last_hb = std::chrono::steady_clock::now();

    // Run event loop
    _ioc.run();

    spdlog::info( "APPLICATION END" );
}

void ExampleApp::update_handler( const boost::system::error_code& error )
{
    if( error )
    {
        spdlog::error( "Error in timer: {}", error.message() );
        _ioc.stop();
        return;
    }

    update();

    // Reschedule
    _update_timer.expires_at( _update_timer.expiry() + UPDATE_RATE_MS );
    _update_timer.async_wait( [&]( auto ec ){ update_handler( ec ); } );


}


void ExampleApp::update()
{
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - _last_hb).count() >= 1) {
        // Reset start time
        _last_hb = now;

        send_bcmp_heartbeat();
    }

    // TODO: Do stuff based on app type
    screen_.Post(ftxui::Event::Custom);
}

void ExampleApp::process_frame( char* data, ssize_t len )
{
    ethhdr ethernet_header;
    ip6_hdr ipv6_header;
    udphdr udp_header;

    auto ret = parse_packet( (uint8_t*)data, len, ethernet_header, ipv6_header, udp_header );
    if( ret ){
        // spdlog::info( "Got udp packet, src dst len {} {} {}", ntohs(udp_header.source), ntohs(udp_header.dest), ntohs(udp_header.len) );
    }
}

uint64_t get_nanosecond_timestamp() {
    using namespace std::chrono;

    // Get the current time point from the steady clock
    auto now = steady_clock::now();

    // Convert the time point to nanoseconds since the epoch
    auto nanoseconds_since_epoch = duration_cast<nanoseconds>(now.time_since_epoch());

    // Return the count as uint64_t
    return nanoseconds_since_epoch.count();
}


void ExampleApp::send_bcmp_heartbeat()
{
    // Manually fill in the headers (Ethernet, IPv6, UDP) and data here
    // Define Ethernet header
    ethhdr *eth_header = (struct ethhdr *)output_buffer_;
    ip6_hdr *ipv6_header;
    bcmp_header_t *bcmp_header;
    bcmp_heartbeat_t *bcmp_heartbeat;
    // 5c:85:7e:32:c0:66
	// 4c:cc:6a:d8:52:07
	
    // a0:ce:c8:8e:e0:ba
    memcpy(eth_header->h_source, "\xa0\xce\xc8\x8e\xe0\xba", 6);  // Source MAC (Placeholder)
	memcpy(eth_header->h_dest, "\x33\x33\x00\x00\x00\x01", 6);    // Destination MAC (Placeholder)
    
    eth_header->h_proto = htons(ETH_P_IPV6);

    // Define IPv6 header
    ipv6_header = (ip6_hdr *)(output_buffer_ + sizeof(ethhdr));
    ipv6_header->ip6_flow = htonl((6 << 28) | (0 << 0));    // Version (6), Traffic class (0), Flow label (0)
    ipv6_header->ip6_plen = htons(sizeof(bcmp_header_t) + sizeof(bcmp_heartbeat_t));    // UDP header size + "Hello" size
    ipv6_header->ip6_nxt = 0xBC;                          // Next header: BCMP
    ipv6_header->ip6_hops = 255;                                 // Hop limit

    auto src_addr = "fe80::8aa5:f9b3:72a6:080c";
    auto dest_addr = "ff02::1";

    inet_pton(AF_INET6, src_addr, &ipv6_header->ip6_src);
    inet_pton(AF_INET6, dest_addr, &ipv6_header->ip6_dst);

    // BCMP Header
    bcmp_header = (bcmp_header_t *)(output_buffer_ + sizeof(ethhdr) + sizeof(ip6_hdr));
    bcmp_header->type = BCMP_HEARTBEAT;
    bcmp_header->checksum = 0x0;
    bcmp_header->flags = 0;
    bcmp_header->rsvd = 0;

    bcmp_heartbeat = (bcmp_heartbeat_t *)((uint8_t*)bcmp_header + sizeof( bcmp_header_t) );
    bcmp_heartbeat->liveliness_lease_dur_s=10;
    bcmp_heartbeat->time_since_boot_us = get_nanosecond_timestamp();

    ip6_addr_t src_out{};
    memcpy( reinterpret_cast<void*>(&src_out), ipv6_header->ip6_src.__in6_u.__u6_addr8, 16 );
    ip6_addr_t dst_out{};
    memcpy( reinterpret_cast<void*>(&dst_out), ipv6_header->ip6_dst.__in6_u.__u6_addr8, 16 );

    bcmp_header->checksum = ip6_chksum_pseudo( (uint8_t*)bcmp_header, 0xBC, 18, 
        &src_out,
        &dst_out );

    // Calculate checksums for bcmp message and ethernet frame
    auto frame_size = sizeof(ethhdr) + sizeof(ip6_hdr) + sizeof(bcmp_header_t) + sizeof(bcmp_heartbeat_t);
    spdlog::info( "Sending hb: {}", frame_size );
    _node->net().dev(0)->write_frame( output_buffer_, frame_size );
}


// uint16_t ipv6_pseudo_header_checksum(const char *src_addr, const char *dst_addr, 
//                                      const void *upper_layer_packet, uint32_t upper_layer_len, 
//                                      uint8_t next_header) {