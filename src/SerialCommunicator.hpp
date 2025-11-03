#pragma once

#include "System.hpp"

#if LINUX
    #include "platform/linux/LinuxSerialCommunicator.hpp"
    template <typename P> using SerialCommunicator = LinuxSerialCommunicator<P>;
#else
    #include "platform/windows/WindowsSerialCommunicator.hpp"
#endif