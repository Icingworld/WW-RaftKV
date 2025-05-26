#pragma once

#include <iostream>

namespace WW
{

#ifdef NDEBUG
    #define DEBUG(...) ((void)0)
    #define INFO(...)  ((void)0)
    #define WARN(...)  ((void)0)
    #define ERROR(...) ((void)0)
#else
    #define DEBUG(...) do { printf("[DEBUG] "); printf(__VA_ARGS__); printf("\n"); } while (0)
    #define INFO(...)  do { printf("[INFO] ");  printf(__VA_ARGS__); printf("\n"); } while (0)
    #define WARN(...)  do { printf("[WARN] ");  printf(__VA_ARGS__); printf("\n"); } while (0)
    #define ERROR(...) do { printf("[ERROR] "); printf(__VA_ARGS__); printf("\n"); } while (0)
#endif

} // namespace WW
