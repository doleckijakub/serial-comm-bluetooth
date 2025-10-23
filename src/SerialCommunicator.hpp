#pragma once

#include "System.hpp"

#if LINUX
#   include "platform/linux/SerialCommunicator.hpp"
#else
#   include "platform/windows/SerialCommunicator.hpp"
#endif