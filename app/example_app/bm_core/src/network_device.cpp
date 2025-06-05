#include "bm_core/network_device.hpp"
#include "bm_core/network_interface.hpp"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <linux/if_ether.h>
#include <unistd.h>

#include <stdexcept>
#include <cstring>

#include <zlib.h>

#include <spdlog/spdlog.h>


int set_promiscuous_mode(const char *interface, int enable) {
    int fd;
    struct ifreq ifr;

    // Create a socket to perform the IOCTL operation
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // Prepare the struct ifreq
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));

    // Get the current interface flags
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == -1) {
        perror("Getting IF flags failed");
        close(fd);
        return -1;
    }

    // Modify the flags to set or unset the IFF_PROMISC flag
    if (enable) {
        ifr.ifr_flags |= IFF_PROMISC;
    } else {
        ifr.ifr_flags &= ~IFF_PROMISC;
    }

    // Set the updated flags on the interface
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) == -1) {
        perror("Setting IF flags failed");
        close(fd);
        return -1;
    }

    // Close the socket
    close(fd);
    return 0;
}

namespace bm {
namespace core {

NetworkDevice::NetworkDevice( NetworkInterface& net_if, const std::string& interface ) 
    : _net_if{ net_if }
{
    // Create socket
    _sock_fd = ::socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IPV6) );
    if( _sock_fd == -1 ){
        throw std::runtime_error( "Failed to create socket" );
    }

    spdlog::info( "Socket" );
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(_sock_fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }
    spdlog::info( "add opt" );

    // Set sockaddr_ll structure
    std::memset( &_sock_addr, 0, sizeof(struct sockaddr_ll));
    _sock_addr.sll_family   = AF_PACKET;
    _sock_addr.sll_protocol = htons(ETH_P_IPV6);
    _sock_addr.sll_ifindex  = if_nametoindex( interface.c_str() );

    if( _sock_addr.sll_ifindex == 0 ) {
        ::close( _sock_fd );
        throw std::runtime_error( "Failed to find network interface" );
    }

    // Look up MAC address for interface
    struct ifreq ifr;
    strncpy( ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1 );
    if( ioctl( _sock_fd, SIOCGIFHWADDR, &ifr ) == -1 ) {
        ::close( _sock_fd );
        throw std::runtime_error( std::string( "ioctl SIOCGIFHWADDR failed: ") + std::strerror(errno) );
    }
    for (int i = 0; i < 6; i++) {
        _info.mac_address[i] = static_cast<uint8_t>( ifr.ifr_hwaddr.sa_data[i] );
    }

    set_promiscuous_mode( "enx001ec0d1b1a9", 1 );
    if (bind(_sock_fd, (struct sockaddr *)&_sock_addr, sizeof(struct sockaddr_ll)) == -1) {
        ::close(_sock_fd);
        throw std::runtime_error( "Failed to find network interface" );
    }


    _info.if_name = interface;
}

NetworkDevice::~NetworkDevice() {
    // Close socket
    if( _sock_fd != -1 ) {
        ::close( _sock_fd );
    }
}

NetDeviceInfo NetworkDevice::info() const {
    return _info;
}

ssize_t NetworkDevice::write_frame( const char* buffer, size_t len ) {
    // The input to this method is a complete ethernet frame

    if( len > BM_MAX_FRAME_SIZE ) {
        errno = EINVAL;
        return -1;
    }

    ssize_t sz = ::sendto( _sock_fd, buffer, len, 0, (struct sockaddr*)&_sock_addr, sizeof( _sock_addr ) );
    return sz;
}

ssize_t NetworkDevice::read_frame( char* buffer, size_t len ) {
    // The output of this method is a complete ethernet frame

    if( len > BM_MAX_FRAME_SIZE ) {
        errno = EINVAL;
        return -1;
    }

    ssize_t sz = ::recvfrom( _sock_fd, buffer, len, 0, NULL, NULL );
    return sz;
}

}
}