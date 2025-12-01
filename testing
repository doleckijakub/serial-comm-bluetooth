#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <cstdlib>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <optional>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "SerialCommunicator.hpp"

#define next_arg() (assert(argc), --argc, *argv++)

const char *prog;

void usage() {
    std::fprintf(stderr, "Usage: %s [OPTIONS] recv-file <filename> -- saves serial port output to file <filename>\n", prog);
    std::fprintf(stderr, "       %s [OPTIONS] send-file <filename> -- sends file <filename> to serial port\n", prog);
    std::fprintf(stderr, "Options:\n");
    std::fprintf(stderr, "       -b <N> - sets the baud rate to N\n");
#if LINUX
    std::fprintf(stderr, "       -d <D> - sets the device to D (e.g. /dev/rfcomm0)\n");
#elif WINDOWS
    std::fprintf(stderr, "       -d <D> - sets the device to D (e.g. COM1)\n");
#endif
    std::fprintf(stderr, "       %s [OPTIONS] recv-image <filename> -- receive raw RGBA image data\n", prog);
    std::fprintf(stderr, "       %s [OPTIONS] send-image <filename> -- send raw RGBA image data\n", prog);
}

static SerialCommunicator<Packet> *serial;

static inline void write_u16_le(std::vector<char>& out, uint16_t v) {
    out.push_back((char)(v & 0xFF));
    out.push_back((char)((v >> 8) & 0xFF));
}

static inline uint16_t read_u16_le(const unsigned char* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

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

void send_image(int argc, char **argv) {
    if (!argc) {
        std::fprintf(stderr, "Error: Subcommand 'send-image' requires a filename\n");
        usage();
        std::exit(1);
    }

    std::string filename = next_arg();

    int w, h, ch;
    unsigned char* img = stbi_load(filename.c_str(), &w, &h, &ch, 4);
    if (!img) {
        std::fprintf(stderr, "Error: Failed to load image: %s\n", filename.c_str());
        std::exit(1);
    }

    std::fprintf(stderr, "Loaded %s: %dx%d (%d channels), sending...\n",
                 filename.c_str(), w, h, ch);

    std::vector<char> bytes;
    bytes.reserve(w * h * 4 + 4);

    write_u16_le(bytes, (uint16_t)w);
    write_u16_le(bytes, (uint16_t)h);

    bytes.insert(bytes.end(), img, img + w*h*4);

    stbi_image_free(img);

    serial->start();
    const auto chunks = chunk(bytes);

    for (const auto &pkt : chunks)
        *serial << pkt;

    std::fprintf(stderr, "Image sent (%zu bytes)\n", bytes.size());
}

void recv_image(int argc, char** argv) {
    static uint16_t width = 0, height = 0;
    static sf::Image img;
    static sf::Texture tex;
    static std::unique_ptr<sf::Sprite> sprite;
    static std::unique_ptr<sf::RenderWindow> window;

    static size_t bytesReceived = 0;
    static std::vector<char> buffer;

    serial->onReceive([&](const Packet &pkt) {
        buffer.insert(buffer.end(), pkt.payload.begin(), pkt.payload.end());

        if (width == 0 && buffer.size() >= 4) {
            width = read_u16_le((unsigned char*)buffer.data());
            height = read_u16_le((unsigned char*)buffer.data() + 2);
            buffer.erase(buffer.begin(), buffer.begin() + 4);

            std::fprintf(stderr, "Receiving image: %d x %d\n", width, height);

            img = sf::Image({static_cast<unsigned>(width),
                             static_cast<unsigned>(height)}, sf::Color::Black);

            tex.loadFromImage(img);
            sprite = std::make_unique<sf::Sprite>(tex);
            sprite->setScale(sf::Vector2f(2.f, 2.f));

            window = std::make_unique<sf::RenderWindow>(
                sf::VideoMode({static_cast<unsigned>(width * 2),
                               static_cast<unsigned>(height * 2)}),
                "Receiving Image");
        }

        if (!width) return;

        while (buffer.size() >= 4 && bytesReceived / 4 < width * height) {
            size_t pixelIndex = bytesReceived / 4;
            uint16_t x = pixelIndex % width;
            uint16_t y = pixelIndex / width;

            sf::Color c(
                static_cast<std::uint8_t>(buffer[0]),
                static_cast<std::uint8_t>(buffer[1]),
                static_cast<std::uint8_t>(buffer[2]),
                static_cast<std::uint8_t>(buffer[3])
            );

            img.setPixel({x, y}, c);

            buffer.erase(buffer.begin(), buffer.begin() + 4);
            bytesReceived++;

            if (x + 1 == width) {
                tex.update(img);
                window->clear();
                window->draw(*sprite);
                window->display();
            }
        }

        while (auto evOpt = window->pollEvent()) {
            if (!evOpt) continue;

            auto &ev = *evOpt;
            if (ev.is<sf::Event::Closed>()) {
                window->close();
                std::exit(0);
            }
        }

        if (bytesReceived / 4 == width * height) {
            std::fprintf(stderr, "Image received!\n");
            while (window->isOpen()) {
                while (auto evOpt = window->pollEvent()) {
                    auto &ev = *evOpt;
                    if (ev.is<sf::Event::Closed>())
                        window->close();
                }
                window->clear();
                window->draw(*sprite);
                window->display();
            }
            std::exit(0);
        }
    });

    serial->start();

    while (true) sf::sleep(sf::milliseconds(10));
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
        } else if (arg == "recv-image") {
            handler = recv_image;
            break;
        }
        else if (arg == "send-image") {
            handler = send_image;
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