#include "util.hpp"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <random>

#include <cstdlib>
#include <cerrno>
#include <climits>
#include <cstring>

#include <sys/wait.h>
#include <sys/capability.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if_packet.h>

namespace bmgui {

uint64_t generate_random_node_id() 
{
    std::random_device rd;
    std::mt19937_64 gen( rd() );
    std::uniform_int_distribution<uint64_t> dis;
    return dis( gen );
}

std::string uint64_to_hex_string( uint64_t value ) 
{
    std::stringstream ss;
    ss << std::setw(16) << std::setfill('0') << std::hex << value;
    return ss.str();
}

std::vector<std::string> get_available_raw_socket_ifaces() 
{
    std::vector<std::string> rawSocketInterfaces;

    struct ifaddrs *ifap, *ifa;
    if (getifaddrs(&ifap) == 0) {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            freeifaddrs(ifap);
            return rawSocketInterfaces;
        }

        for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_PACKET) {
                struct ifreq ifr;
                std::strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ-1);

                // Check if the interface is up and running
                if (ioctl(sock, SIOCGIFFLAGS, &ifr) != 0) {
                    std::cerr << "ioctl SIOCGIFFLAGS failed for " << ifa->ifa_name << std::endl;
                    continue;
                }

                if (!(ifr.ifr_flags & IFF_UP) || !(ifr.ifr_flags & IFF_RUNNING)) {
                    // Interface is down or not running, skip it
                    continue;
                }

                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                    if (ifr.ifr_hwaddr.sa_family == ARPHRD_ETHER) {
                        // Now check if the interface supports SOCK_RAW
                        int raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
                        if (raw_sock >= 0) {
                            struct sockaddr_ll sll;
                            memset(&sll, 0, sizeof(sll));
                            sll.sll_family = AF_PACKET;
                            sll.sll_ifindex = if_nametoindex(ifa->ifa_name);
                            sll.sll_protocol = htons(ETH_P_ALL);
                            if (bind(raw_sock, (struct sockaddr *)&sll, sizeof(sll)) == 0) {
                                rawSocketInterfaces.push_back(ifa->ifa_name);
                            }
                            close(raw_sock);
                        }
                    }
                } else {
                    std::cerr << "ioctl SIOCGIFHWADDR failed for " << ifa->ifa_name << std::endl;
                }
            }
        }
        close(sock);
        freeifaddrs(ifap);
    } else {
        std::cerr << "Failed to get network interfaces" << std::endl;
    }

    return rawSocketInterfaces;
}

// Function to check if the current process has the required capabilities
bool check_capabilities() 
{
    cap_t caps = cap_get_proc();
    if (caps == NULL) {
        std::cerr << "Failed to get process capabilities: " << strerror(errno) << std::endl;
        return false;
    }

    cap_flag_value_t cap_net_raw;
    cap_flag_value_t cap_net_admin;
    if (cap_get_flag(caps, CAP_NET_RAW, CAP_EFFECTIVE, &cap_net_raw) == -1 ||
        cap_get_flag(caps, CAP_NET_ADMIN, CAP_EFFECTIVE, &cap_net_admin) == -1) {
        std::cerr << "Failed to get capability flags: " << strerror(errno) << std::endl;
        cap_free(caps);
        return false;
    }

    cap_free(caps);
    return (cap_net_raw == CAP_SET && cap_net_admin == CAP_SET);
}

void restart_with_sudo_net_caps(const char* executablePath) 
{
    char resolvedPath[PATH_MAX];
    if (realpath(executablePath, resolvedPath) == NULL) {
        std::cerr << "Failed to resolve the full path of the executable: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Failed to fork process: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process: execute the setcap command
        execlp("pkexec", "pkexec", "setcap", "cap_net_raw,cap_net_admin=eip", resolvedPath, nullptr);
        // If execlp fails
        std::cerr << "Failed to execute pkexec: " << strerror(errno) << std::endl;
        _exit(EXIT_FAILURE);
    } else {
        // Parent process: wait for the child process to complete
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "waitpid failed: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Relaunch the application normally
            execl(resolvedPath, resolvedPath, nullptr);
            std::cerr << "Failed to relaunch application: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        } else {
            std::cerr << "Failed to set capabilities, exit status: " << WEXITSTATUS(status) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

} // namespace bmgui