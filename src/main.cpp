#include <iostream>
#include <fstream>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string>

#include "SerialCommunicator.hpp"
#include "Terminal.hpp"

static char readCharNonBlocking() {
    char c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return 0;
    return c;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr, "Usage: %s <DEVICE>\n", argv[0]);
        return 1;
    }

    auto term = std::make_unique<Terminal>();
    if (!term->init()) {
        std::fprintf(stderr, "Failed to initialize terminal.\n");
        return 1;
    }

    SerialCommunicator<Packet> serial(argv[1]);
    serial.setBaudRate(115200);
    std::atomic<bool> running = true;
    std::string inputLine;

    serial.onReceive([&](const Packet &pkt) {
        switch (pkt.type) {
            case PacketType::STR:
                term->addLine(" << " + pkt.str_data);
                term->render();
                break;
            case PacketType::BIN:
                term->addImage(pkt.bin_data);
                term->render();
                break;
        }
    });

    serial.start();
    term->addLine("Type messages or /quit to exit.");
    term->render();

    while (running) {
        char ch = readCharNonBlocking();
        if (ch) {
            switch (ch) {
                case '\n':
                case '\r':
                    if (inputLine == "/quit") {
                        running = false;
                        break;
                    }
                    else if (inputLine.starts_with("/img ")) {
                        std::string filename = inputLine.substr(5);
                        std::ifstream file(filename, std::ios::binary);
                        if (!file) {
                            term->addLine("Error: Failed to open file: " + filename);
                            break;
                        }
                        std::vector<char> imageData((std::istreambuf_iterator<char>(file)),
                                                    std::istreambuf_iterator<char>());

                        Packet pkt{ .type = PacketType::BIN, .bin_data = imageData };
                        serial << pkt;
                        term->addImage(imageData);
                    }
                    else if (!inputLine.empty()) {
                        Packet pkt{ .type = PacketType::STR, .str_data = inputLine };
                        serial << pkt;
                        term->addLine(" >> " + inputLine);
                    }
                    inputLine.clear();
                    break;
                case 127:
                case '\b':
                    if (!inputLine.empty()) inputLine.pop_back();
                    break;
                default:
                    if (ch >= 32 && ch <= 126) inputLine.push_back(ch);
                    break;
            }
        }

        term->render();
        if (!inputLine.empty()) {
            printf("%s", inputLine.c_str());
            fflush(stdout);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    term->addLine("Goodbye!");
    term->render();
    serial.stop();
    term->shutdown();
    return 0;
}
