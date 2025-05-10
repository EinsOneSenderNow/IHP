#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

struct ICMPHeader {
    BYTE type;
    BYTE code;
    USHORT checksum;
    USHORT id;
    USHORT sequence;
};

USHORT calculateChecksum(USHORT* buffer, int size) {
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

std::atomic<bool> running(true);

void sendIcmp(const char* targetIp) {
    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[!] ICMP send socket error\n";
        return;
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(targetIp);

    for (int seq = 1; running; ++seq) {
        char packet[64];
        ICMPHeader* icmp = (ICMPHeader*)packet;
        icmp->type = 8;
        icmp->code = 0;
        icmp->id = (USHORT)GetCurrentProcessId();
        icmp->sequence = seq;
        memset(packet + sizeof(ICMPHeader), 'A', sizeof(packet) - sizeof(ICMPHeader));
        icmp->checksum = 0;
        icmp->checksum = calculateChecksum((USHORT*)packet, sizeof(packet));

        int sent = sendto(sock, packet, sizeof(packet), 0, (SOCKADDR*)&dest, sizeof(dest));
        if (sent != SOCKET_ERROR) {
            std::cout << "[->] ICMP Echo Request to " << targetIp << "\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    closesocket(sock);
}

void recvIcmp() {
    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[!] ICMP recv socket error\n";
        return;
    }

    char buffer[1024];
    sockaddr_in from{};
    int fromLen = sizeof(from);

    while (running) {
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, (SOCKADDR*)&from, &fromLen);
        if (len > 0) {
            ICMPHeader* icmp = (ICMPHeader*)(buffer + 20); // skip IP header
            if (icmp->type == 0 && icmp->code == 0) {
                std::cout << "[<-] ICMP Echo Reply from " << inet_ntoa(from.sin_addr) << "\n";
            }
        }
    }

    closesocket(sock);
}

int main() {
    WSADATA wsaData;
    SOCKET sock;

    const char* coordinatorIp = "127.0.0.1"; // IP координатора
    const int coordinatorPort = 12345;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // TCP клиент
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(coordinatorPort);
    server.sin_addr.s_addr = inet_addr(coordinatorIp);

    if (connect(sock, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "Cannot connect to coordinator\n";
        return 1;
    }

    char buffer[64] = { 0 };
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    closesocket(sock);
    if (received <= 0) {
        std::cerr << "Failed to get peer info\n";
        return 1;
    }

    buffer[received] = '\0';
    std::string peerIp = std::string(buffer);
    if (peerIp.find(':') != std::string::npos) {
        peerIp = peerIp.substr(0, peerIp.find(':')); // обрезаем порт
    }

    std::cout << "[i] Peer IP: " << peerIp << "\n";

    // Запуск потоков
    std::thread sender(sendIcmp, peerIp.c_str());
    std::thread receiver(recvIcmp);

    std::cout << "[*] Press ENTER to stop...\n";
    std::cin.get();
    running = false;

    sender.join();
    receiver.join();

    WSACleanup();
    return 0;
}
