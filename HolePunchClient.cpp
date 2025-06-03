#include "HolePunchClient.h"
#include <iostream>

IcmpHolePunchClient::IcmpHolePunchClient(const std::string& coordinatorIp, unsigned short coordinatorPort)
    : coordinatorIp_(coordinatorIp), coordinatorPort_(coordinatorPort) {}

IcmpHolePunchClient::~IcmpHolePunchClient() {
    running_ = false;
    if (sender_.joinable()) sender_.join();
    if (receiver_.joinable()) receiver_.join();
    WSACleanup();
}

bool IcmpHolePunchClient::initialize() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    return true;
}

USHORT IcmpHolePunchClient::calculateChecksum(USHORT* buffer, int size) {
    unsigned long cksum = 0;
    while (size > 1) {
        cksum += *buffer++;
        size -= sizeof(USHORT);
    }
    if (size) {
        cksum += *(UCHAR*)buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (USHORT)(~cksum);
}

std::string IcmpHolePunchClient::getPeerAddress() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Cannot create TCP socket\n";
        return {};
    }
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(coordinatorPort_);
    server.sin_addr.s_addr = inet_addr(coordinatorIp_.c_str());
    if (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Cannot connect to coordinator\n";
        closesocket(sock);
        return {};
    }
    char buffer[64] = {0};
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    closesocket(sock);
    if (received <= 0) {
        std::cerr << "Failed to get peer info\n";
        return {};
    }
    buffer[received] = '\0';
    std::string peerIp(buffer);
    if (peerIp.find(':') != std::string::npos)
        peerIp = peerIp.substr(0, peerIp.find(':'));
    return peerIp;
}

void IcmpHolePunchClient::sendLoop(const std::string& targetIp) {
    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[!] ICMP send socket error\n";
        return;
    }
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(targetIp.c_str());
    for (int seq = 1; running_; ++seq) {
        char packet[64];
        ICMPHeader* icmp = (ICMPHeader*)packet;
        icmp->type = 8;
        icmp->code = 0;
        icmp->id = (USHORT)GetCurrentProcessId();
        icmp->sequence = seq;
        memset(packet + sizeof(ICMPHeader), 'A', sizeof(packet) - sizeof(ICMPHeader));
        icmp->checksum = 0;
        icmp->checksum = calculateChecksum((USHORT*)packet, sizeof(packet));
        sendto(sock, packet, sizeof(packet), 0, (SOCKADDR*)&dest, sizeof(dest));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    closesocket(sock);
}

void IcmpHolePunchClient::recvLoop() {
    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[!] ICMP recv socket error\n";
        return;
    }
    char buffer[1024];
    sockaddr_in from{};
    int fromLen = sizeof(from);
    while (running_) {
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, (SOCKADDR*)&from, &fromLen);
        if (len > 0) {
            ICMPHeader* icmp = (ICMPHeader*)(buffer + 20);
            if (icmp->type == 0 && icmp->code == 0) {
                std::cout << "[<-] ICMP Echo Reply from " << inet_ntoa(from.sin_addr) << "\n";
            }
        }
    }
    closesocket(sock);
}

void IcmpHolePunchClient::run() {
    std::string peerIp = getPeerAddress();
    if (peerIp.empty()) return;
    std::cout << "[i] Peer IP: " << peerIp << "\n";
    running_ = true;
    sender_ = std::thread(&IcmpHolePunchClient::sendLoop, this, peerIp);
    receiver_ = std::thread(&IcmpHolePunchClient::recvLoop, this);
    std::cout << "[*] Press ENTER to stop...\n";
    std::cin.get();
    running_ = false;
    if (sender_.joinable()) sender_.join();
    if (receiver_.joinable()) receiver_.join();
}
