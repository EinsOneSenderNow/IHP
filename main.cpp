#include "HolePunchClient.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string coordinatorIp = "127.0.0.1";
    unsigned short coordinatorPort = 12345;
    if (argc >= 2) coordinatorIp = argv[1];
    if (argc >= 3) coordinatorPort = static_cast<unsigned short>(std::stoi(argv[2]));

    IcmpHolePunchClient client(coordinatorIp, coordinatorPort);
    if (!client.initialize()) {
        return 1;
    }
    client.run();
    return 0;
}
