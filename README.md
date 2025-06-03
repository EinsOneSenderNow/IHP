# ICMP Hole Punch Client

This project contains a simple implementation of an ICMP hole punching client for Windows.
It uses a TCP coordinator to exchange peer addresses and then sends ICMP echo
requests to open a path through NAT.

## Building with CMake

```
mkdir build
cd build
cmake ..
cmake --build .
```

The project requires a Windows build environment with the WinSock2 headers and
libraries available.
