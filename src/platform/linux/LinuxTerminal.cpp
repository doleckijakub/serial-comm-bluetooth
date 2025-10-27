#include "LinuxTerminal.hpp"

#include <curses.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

LinuxTerminal::LinuxTerminal() {}
LinuxTerminal::~LinuxTerminal()
{
    shutdown();
}

bool LinuxTerminal::init()
{
    if (initscr() == nullptr) return false;
    noecho();            // don't echo input
    cbreak();            // disable line buffering
    keypad(stdscr, TRUE);// enable arrow keys
    nodelay(stdscr, TRUE);// non-blocking input
    start_color();
    use_default_colors();
    term_h_ = LINES;
    term_w_ = COLS;
    return true;
}

void LinuxTerminal::shutdown()
{
    endwin();
}

void LinuxTerminal::clearLines()
{
    lines_.clear();
}
void LinuxTerminal::addLine(const std::string &line)
{
    lines_.push_back(line);
}

void LinuxTerminal::render()
{
    erase();
    int start = 0;
    if ((int)lines_.size() > term_h_)
    {
        start = lines_.size() - term_h_;  // skip oldest lines
    }
    int y = 0;
    for (size_t i = start; i < lines_.size(); ++i)
    {
        mvprintw(y++, 0, "%s", lines_[i].c_str());
    }
    refresh();
}


void LinuxTerminal::writeRaw(const void* data, size_t len)
{
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

std::string LinuxTerminal::b64(const uint8_t* data, size_t len)
{
    static const char *B64TAB =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string s; s.reserve(((len + 2) / 3) * 4);
    size_t i = 0;
    while (i < len)
    {
        uint32_t v = (uint32_t)data[i++] << 16;
        if (i < len) v |= (uint32_t)data[i++] << 8;
        if (i < len) v |= (uint32_t)data[i++];
        int pad = (i > len ? 2 : (i == len ? 1 : 0));
        s.push_back(B64TAB[(v >> 18) & 0x3F]);
        s.push_back(B64TAB[(v >> 12) & 0x3F]);
        s.push_back(pad >= 2 ? '=' : B64TAB[(v >> 6) & 0x3F]);
        s.push_back(pad >= 1 ? '=' : B64TAB[v & 0x3F]);
    }
    return s;
}

bool LinuxTerminal::sendKittyImage(const uint8_t* rgba, int width, int height,
                                   int x, int y, int strideBytes)
{
    std::vector<unsigned char> png;
    auto write_func = [](void *context, void *data, int size)
    {
        auto* out = static_cast<std::vector<unsigned char>*>(context);
        out->insert(out->end(), (unsigned char*)data, (unsigned char*)data + size);
    };
    if (!stbi_write_png_to_func(write_func, &png, width, height, 4, rgba,
                                strideBytes))
        return false;
    auto b64png = b64(png.data(), png.size());
    std::string head = "\x1b_Ga=T,t=d,s=" + std::to_string(width) +
                       ",v=" + std::to_string(height) + ",f=100;";
    std::string tail = "\x1b\\";
    writeRaw(head.data(), head.size());
    const size_t CH = 4096;
    size_t off = 0, N = b64png.size();
    while (off < N)
    {
        size_t n = std::min(CH, N - off);
        bool more = (off + n) < N;
        std::string chunk = std::string("m=") + (more ? "1" : "0") +
                            ";data=" + b64png.substr(off, n);
        writeRaw(chunk.data(), chunk.size());
        writeRaw(tail.data(), tail.size());
        if (more) writeRaw("\x1b_G", 2);
        off += n;
    }
    return true;
}

bool LinuxTerminal::renderImageFile(const std::string &path, int x, int y)
{
    int w, h, ch;
    unsigned char *img = stbi_load(path.c_str(), &w, &h, &ch, 4);
    if (!img) return false;
    bool ok = sendKittyImage(img, w, h, x, y, w * 4);
    stbi_image_free(img);
    return ok;
}
