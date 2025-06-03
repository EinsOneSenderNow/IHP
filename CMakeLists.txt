cmake_minimum_required(VERSION 3.8)
project(IcmpHolePunchClient)

add_executable(IcmpHolePunchClient
    HolePunchClient.cpp
    main.cpp
)

target_link_libraries(IcmpHolePunchClient ws2_32)
