#pragma once

#include <functional>
#include <string>
#include <vector>

template<typename P>
class ISerialCommunicator
{
  public:
    explicit ISerialCommunicator(const std::string& devicePath)
        : devicePath_(devicePath) {}
    virtual ~ISerialCommunicator() = default;

    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool isRunning() const = 0;

    virtual void onReceive(std::function<void(const P &)> cb) = 0;

    virtual bool operator<<(const P &pkt) = 0;

  protected:
    std::string devicePath_;
};
