#include "LinuxTerminal.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <termios.h>
#include <fcntl.h>
#include <thread>
#include <atomic>
#include <fstream>

static struct termios orig_termios;

static void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

LinuxTerminal::LinuxTerminal() {}
LinuxTerminal::~LinuxTerminal()
{
    shutdown();
}

bool LinuxTerminal::init()
{
    enableRawMode();
    term_h_ = 24;
    term_w_ = 80;
    const char *env_lines = getenv("LINES");
    const char *env_cols  = getenv("COLUMNS");
    if (env_lines) term_h_ = atoi(env_lines);
    if (env_cols)  term_w_ = atoi(env_cols);
    return true;
}

void LinuxTerminal::shutdown()
{
    disableRawMode();
}

void LinuxTerminal::clearLines()
{
    lines_.clear();
}
void LinuxTerminal::addLine(const std::string &line)
{
    lines_.push_back(line);
}

void LinuxTerminal::render() {
    printf("\033[H\033[J");

    int start = 0;
    if ((int)lines_.size() > term_h_)
        start = lines_.size() - term_h_;

    for (size_t i = start; i < lines_.size(); ++i) {
        const auto &line = lines_[i];
        if (!line.empty() && line[0] == '\r') {
            write(STDOUT_FILENO, line.c_str(), line.size());
            fflush(stdout);
            printf("\n");
        } else {
            printf("%s\n", line.c_str());
        }
    }

    printf("> ");
    fflush(stdout);
}


void LinuxTerminal::writeRaw(const void* data, size_t len)
{
    fwrite(data, 1, len, stdout);
    fflush(stdout);
}

static std::string base64Encode(const std::string& input)
{
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : input)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    } if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return result;
}

bool LinuxTerminal::addImage(const std::vector<char> &imageData)
{
    char tmpName[] = "/tmp/kitty_img_XXXXXX.png";
    int fd = mkstemps(tmpName, 4);
    if (fd == -1) return false;
    write(fd, imageData.data(), imageData.size());
    close(fd);
    std::string encodedPath = base64Encode(tmpName);
    std::stringstream kitty;
    kitty << "\r\033_G" << "a=T" << ",q=2" << ",f=100" << ",t=f" << ",X=1;" <<
          encodedPath << "\033\\" << "\n";
    addLine(kitty.str());
    return true;
}