#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <cstdlib>

#include "SerialCommunicator.hpp"

#define next_arg() (assert(argc), --argc, *argv++)

const char *prog;

void usage() {
    std::fprintf(stderr, "Usage: %s [OPTIONS] recv-file <filename> -- saves serial port output to file <filename>\n", prog);
    std::fprintf(stderr, "       %s [OPTIONS] send-file <filename> -- sends file <filename> to serial port\n", prog);
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "       -b <N> - sets the baud rate to N");
#if LINUX
    std::fprintf(stderr, "       -d <D> - sets the device to D (e.g. /dev/rfcomm0)");
#elif WINDOWS
    std::fprintf(stderr, "       -d <D> - sets the device to D (e.g. COM1)");
#endif
}

static SerialCommunicator<Packet> *serial;

void recv_file(int argc, char **argv) {
    if (!argc) {
        std::fprintf(stderr, "Error: Subcommand 'recv-file' requires a filename\n");
        usage();
        std::exit(1);
    }

    std::string filename = next_arg();

    std::remove(filename.c_str());

    serial->onReceive([&](const Packet &pkt) {
        std::ofstream file(filename, std::ios::out | std::ios::binary | std::ios::app);
        file.write(pkt.payload.data(), pkt.payload.size());
        file.flush();

        std::fprintf(stderr, "Recieved chunk %d / %d\n", pkt.header.chunkIndex + 1, pkt.header.totalChunks);

        if (pkt.header.chunkIndex + 1 == pkt.header.totalChunks) {
            std::exit(0);
        }
    });

    serial->start();

    while (true);
}

void send_file(int argc, char **argv) {
    if (!argc) {
        std::fprintf(stderr, "Error: Subcommand 'send-file' requires a filename\n");
        usage();
        std::exit(1);
    }

    std::string filename = next_arg();

    serial->start();

    std::ifstream file(filename, std::ios::binary);

    if (!file) {
        std::fprintf(stderr, "Error: Failed to open file: %s\n", filename.c_str());
        std::exit(1);
    }

    std::vector<char> fileData((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

    const auto chunks = chunk(fileData);

    for (const auto pkt : chunks) {
        *serial << pkt;
    }
}

int main(int argc, char **argv)
{
    prog = next_arg();

    std::function<void(int, char **)> handler = nullptr;
    int baudRate = 600;
    std::string device;

    while (argc) {
        std::string arg = next_arg();

        if (arg == "recv-file") {
            handler = recv_file;
            break;
        } else if (arg == "send-file") {
            handler = send_file;
            break;
        } else if (arg == "-b") {
            if (!argc) {
                std::fprintf(stderr, "Error: Flag '-b' requires a value\n");
                usage();
                return 1;
            }

            const char *b = next_arg();
            baudRate = atoi(b);
        } else if (arg == "-d") {
            if (!argc) {
                std::fprintf(stderr, "Error: Flag '-d' requires a value\n");
                usage();
                return 1;
            }

            device = next_arg();
        } else {
            std::fprintf(stderr, "Error: Argument '%s' not recognized\n", arg.c_str());
            usage();
            return 1;
        }
    }

    if (!handler) {
        std::fprintf(stderr, "Error: No subcommand provided\n");
        usage();
        return 1;
    }

    serial = new SerialCommunicator<Packet>(device);
    serial->setBaudRate(baudRate);

    handler(argc, argv);

    return 0;
}