#pragma once

#include <string>
#include <vector>
#include <cstdint>


class ITerminal {
public:
    virtual ~ITerminal() = default;


    virtual bool init() = 0;
    virtual void shutdown() = 0;


    virtual void clearLines() = 0;
    virtual void addLine(const std::string &line) = 0;
    virtual void render() = 0;


    virtual bool renderImageFile(const std::string &path, int x, int y) = 0;
};