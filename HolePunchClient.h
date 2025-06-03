#pragma once
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>

struct ICMPHeader {
    BYTE type;
    BYTE code;
    USHORT checksum;
    USHORT id;
    USHORT sequence;
};

class IcmpHolePunchClient {
public:
    IcmpHolePunchClient(const std::string& coordinatorIp, unsigned short coordinatorPort);
    ~IcmpHolePunchClient();
    bool initialize();
    void run();
private:
    std::string getPeerAddress();
    void sendLoop(const std::string& peerIp);
    void recvLoop();
    static USHORT calculateChecksum(USHORT* buffer, int size);

    std::string coordinatorIp_;
    unsigned short coordinatorPort_;
    std::atomic<bool> running_{false};
    std::thread sender_;
    std::thread receiver_;
};
