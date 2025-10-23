#include <iostream>

#include "SerialCommunicator.hpp"
#include "Packet.hpp"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s <DEVICE>\n", argv[0]);
        return 1;
    }
    SerialCommunicator<Packet> serial(argv[1]);
    serial.setBaudRate(115200);
    serial.onReceive([](const Packet & pkt)
    {
        std::printf("[recv] %s\n", pkt.str_data.c_str());
    });
    serial.start();
    std::string line;
    std::printf("Type messages or /quit to exit.\n");
    while (true)
    {
        std::getline(std::cin, line);
        if (line == "/quit") break;
        Packet pkt =
        {
            .type = PacketType::STR,
            .str_data = line,
        };
        serial << pkt;
    }
    std::printf("Goodbye!\n");
    serial.stop();
    return 0;
    return 0;
}
