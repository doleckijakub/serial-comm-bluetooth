#include "Terminal.hpp"
#include "SerialCommunicator.hpp"
#include "Packet.hpp"
#include "System.hpp"

#include <curses.h>

#include <memory>
#include <thread>
#include <atomic>
#include <string>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "Usage: %s <DEVICE>\n", argv[0]);
        return 1;
    }
    auto term = std::make_unique<Terminal>();
    if (!term->init())
    {
        std::fprintf(stderr, "Failed to initialize terminal.\n");
        return 1;
    }
    SerialCommunicator<Packet> serial(argv[1]);
    serial.setBaudRate(115200);
    std::atomic<bool> running = true;
    std::string inputLine;
    
    serial.onReceive([&](const Packet & pkt)
    {
        term->addLine(" << " + pkt.str_data);
        term->render();
    });
    
    serial.start();
    term->addLine("Type messages or /quit to exit.");
    term->render();
    int ch;
    while (running)
    {
        ch = getch();
        if (ch != ERR)
        {
            switch (ch)
            {
                case '\n':
                case '\r':
                    if (inputLine == "/quit")
                    {
                        running = false;
                    }
                    else if (inputLine.starts_with("/img "))
                    {
                        std::string path = inputLine.substr(5);
                        term->renderImageFile(path, 0, 3);
                    }
                    else if (!inputLine.empty())
                    {
                        Packet pkt{ .type = PacketType::STR, .str_data = inputLine };
                        serial << pkt;
                        term->addLine(" >> " + inputLine);
                    }
                    inputLine.clear();
                    break;
                case KEY_BACKSPACE:
                case 127:
                    if (!inputLine.empty()) inputLine.pop_back();
                    break;
                default:
                    if (ch >= 32 && ch <= 126)
                    {
                        inputLine.push_back((char)ch);
                    }
                    break;
            }
        }
        
        term->render();
        
        if (1 /* !inputLine.empty() */)
        {
            move(LINES - 1, 0);
            clrtoeol();
            printw("> %s", inputLine.c_str());
            refresh();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    term->addLine("Goodbye!");
    term->render();
    serial.stop();
    term->shutdown();
    return 0;
}
