#pragma once

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>

class Pinger {
   public:
    explicit Pinger(const std::string& interface, int packet_size_) : interface_(interface), packet_size_(packet_size_) {
        ifindex_ = -1;
        sockfd_ = -1;
    }

    void init();
    void send_ping(const std::string& target_ip);
    void receive_ping();
    void close();

   private:
    struct sockaddr_ll* socket_address_{};  // Used to store the MAC address of the interface
    std::string interface_;                 // Egress interface name
    std::string source_ip_;                 // Source IP address, obtained from the egress interface
    int sockfd_;
    int ifindex_;
    int packet_size_;
};
