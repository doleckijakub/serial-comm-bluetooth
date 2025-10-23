#pragma once

#define LINUX defined(__linux__)
#define WINDOWS (defined(_WIN32) || defined(WIN32))

#if !LINUX && !WINDOWS
#   error Only windows an linux systems are supported
#endif