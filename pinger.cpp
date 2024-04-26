#include <cstdlib>
#include <cstring>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "pinger.hpp"
#include "loguru.hpp"

unsigned short checksum(void* b, int len) {
    unsigned short* buf = static_cast<unsigned short*>(b);
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void Pinger::init() {
    LOG_SCOPE_F(INFO, "Pinger initialization");
    sockfd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sockfd_ < 0) {
        LOG_F(ERROR, "Socket creation failed: %s", strerror(errno));
        return;
    }
    LOG_F(INFO, "Binding raw socket to device: %s", interface_.c_str());

    ifindex_ = if_nametoindex(interface_.c_str());

    const struct sockaddr_ll sll = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),
        .sll_ifindex = ifindex_,
    };

    if (bind(sockfd_, reinterpret_cast<const struct sockaddr*>(&sll), sizeof(sll)) < 0) {
        LOG_F(ERROR, "Bind failed: %s", strerror(errno));
        return;
    }

    LOG_F(INFO, "Socket bound to device: %s", interface_.c_str());

    struct ifreq ifr = {};
    std::strncpy(ifr.ifr_name, interface_.c_str(), IFNAMSIZ - 1);
    if (ioctl(sockfd_, SIOCGIFHWADDR, &ifr) < 0) {
        LOG_F(ERROR, "Failed to get mac address: %s", std::strerror(errno));
        return;
    }

    LOG_F(INFO, "Got mac: %02X:%02X:%02X:%02X:%02X:%02X for egresss interface: %s", static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[0]),
          static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[1]), static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[2]),
          static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[3]), static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[4]),
          static_cast<unsigned char>(ifr.ifr_hwaddr.sa_data[5]), interface_.c_str());

    socket_address_ = new sockaddr_ll();

    socket_address_->sll_halen = ETH_ALEN;
    socket_address_->sll_protocol = htons(ETH_P_ALL);
    socket_address_->sll_ifindex = ifindex_;
    std::memcpy(socket_address_->sll_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);

    struct ifreq ifr_ip = {};
    std::strncpy(ifr_ip.ifr_name, interface_.c_str(), IFNAMSIZ - 1);
    if (ioctl(sockfd_, SIOCGIFADDR, &ifr_ip) < 0) {
        LOG_F(ERROR, "Failed to get IP address: %s", std::strerror(errno));
        return;
    }
    source_ip_ = inet_ntoa(reinterpret_cast<struct sockaddr_in*>(&ifr_ip.ifr_addr)->sin_addr);
    LOG_F(INFO, "Got IP: %s for egress interface: %s", source_ip_.c_str(), interface_.c_str());

    LOG_F(INFO, "Pinger initialized successfully");
}

void Pinger::send_ping(std::string target_ip) {
    LOG_SCOPE_F(INFO, "Sending ICMP echo request to: %s", target_ip.c_str());
    char* packet = new char[packet_size_];
    std::memset(packet, 0, packet_size_);

    struct ethhdr* eth = reinterpret_cast<struct ethhdr*>(packet);
    struct iphdr* ip = reinterpret_cast<struct iphdr*>(packet + sizeof(struct ethhdr));
    struct icmphdr* icmp = reinterpret_cast<struct icmphdr*>(packet + sizeof(struct ethhdr) + sizeof(struct iphdr));

    eth->h_proto = htons(ETH_P_IP);
    std::memcpy(eth->h_source, socket_address_->sll_addr, ETH_ALEN);

    // get MAC address of target IP using system arp table, SIOCGARP
    struct arpreq arpreq = {};
    std::strncpy(arpreq.arp_dev, interface_.c_str(), IFNAMSIZ - 1);
    struct sockaddr_in* target_ip_addr = reinterpret_cast<struct sockaddr_in*>(&arpreq.arp_pa);
    target_ip_addr->sin_family = AF_INET;
    target_ip_addr->sin_addr.s_addr = inet_addr(target_ip.c_str());
    if (ioctl(sockfd_, SIOCGARP, &arpreq) < 0) {
        LOG_F(ERROR, "Failed to get MAC address of target IP: %s", std::strerror(errno));
    }
    std::memcpy(eth->h_dest, arpreq.arp_ha.sa_data, ETH_ALEN);
    LOG_F(INFO, "Got MAC: %02X:%02X:%02X:%02X:%02X:%02X", static_cast<unsigned char>(eth->h_dest[0]),
          static_cast<unsigned char>(eth->h_dest[1]), static_cast<unsigned char>(eth->h_dest[2]),
          static_cast<unsigned char>(eth->h_dest[3]), static_cast<unsigned char>(eth->h_dest[4]),
          static_cast<unsigned char>(eth->h_dest[5]));

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(packet_size_ - sizeof(struct ethhdr));
    ip->id = htons(0);
    ip->frag_off = 0;
    ip->ttl = 64;
    ip->protocol = IPPROTO_ICMP;
    ip->saddr = inet_addr(source_ip_.c_str());
    ip->daddr = inet_addr(target_ip.c_str());

    icmp->type = ICMP_ECHO;
    icmp->code = 0;
    icmp->un.echo.id = 0;
    icmp->un.echo.sequence = 0;
    icmp->checksum = 0;
    icmp->checksum = checksum(reinterpret_cast<uint16_t*>(icmp), packet_size_ - sizeof(struct ethhdr) - sizeof(struct iphdr));

    if (sendto(sockfd_, packet, packet_size_, 0, reinterpret_cast<struct sockaddr*>(socket_address_), sizeof(*socket_address_)) < 0) {
        LOG_F(ERROR, "Failed to send packet: %s", std::strerror(errno));
    }

    LOG_F(INFO, "Sent ICMP echo request to: %s", target_ip.c_str());
    delete[] packet;
}

void Pinger::receive_ping() {
    LOG_SCOPE_F(INFO, "Receiving ICMP echo reply");
    char* buffer = new char[packet_size_];
    std::memset(buffer, 0, packet_size_);

    struct sockaddr saddr = {};
    int saddr_len = sizeof(saddr);
    const int buflen = recvfrom(sockfd_, buffer, packet_size_, 0, &saddr, reinterpret_cast<socklen_t*>(&saddr_len));
    if (buflen < 0) {
        LOG_F(ERROR, "Failed to receive packet: %s", std::strerror(errno));
    }

    const struct ethhdr* eth = reinterpret_cast<struct ethhdr*>(buffer);
    if (eth->h_proto != htons(ETH_P_IP)) {
        delete[] buffer;
        return;
    }

    struct iphdr* ip = reinterpret_cast<struct iphdr*>(buffer + sizeof(struct ethhdr));

    if (ip->protocol != IPPROTO_ICMP) {
        delete[] buffer;
        return;
    }

    const struct icmphdr* icmp = reinterpret_cast<struct icmphdr*>(buffer + sizeof(struct ethhdr) + sizeof(struct iphdr));

    if (icmp->type != ICMP_ECHOREPLY) {
        LOG_F(ERROR, "Didn't receive ICMP echo reply");
        delete[] buffer;
        return;
    }
    LOG_F(INFO, "Received ICMP echo reply from: %s", inet_ntoa(*reinterpret_cast<struct in_addr*>(&ip->saddr)));

    delete[] buffer;
}

void Pinger::close() {
    LOG_F(INFO, "Closing raw socket");
    if (sockfd_ != -1) {
        ::close(sockfd_);
    }
    if (socket_address_ != nullptr) {
        delete socket_address_;
    }
}
