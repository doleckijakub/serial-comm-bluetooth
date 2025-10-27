#pragma once

#include "System.hpp"

#if LINUX
    #include "platform/linux/LinuxTerminal.hpp"
    using Terminal = LinuxTerminal;
#else
    #include "platform/windows/WindowsTerminal.hpp"
#endif