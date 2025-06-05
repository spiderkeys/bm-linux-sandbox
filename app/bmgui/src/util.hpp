#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bmgui {

uint64_t generate_random_node_id();

std::string uint64_to_hex_string( uint64_t value );

std::vector<std::string> get_available_raw_socket_ifaces();
bool check_capabilities();
void restart_with_sudo_net_caps(const char* executablePath);

} // namespace bmgui

