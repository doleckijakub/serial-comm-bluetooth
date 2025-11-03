#pragma once

#include <string>
#include <vector>

#include "../../ITerminal.hpp"

class LinuxTerminal : public ITerminal
{
  public:
    LinuxTerminal();
    ~LinuxTerminal() override;

    bool init() override;
    void shutdown() override;

    void clearLines() override;
    void addLine(const std::string &line) override;
    void render() override;

    bool addImage(const std::vector<char> &imageData) override;

  private:
    std::vector<std::string> lines_;
    int term_w_ = 0;
    int term_h_ = 0;

    void writeRaw(const void* data, size_t len);
};