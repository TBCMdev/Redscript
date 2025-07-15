#pragma once
#include <cstdio>

#define INFO(x, ...) printf("[INFO] " x "\n", ##__VA_ARGS__)

#if 1

#define ERROR_COLOR           "\x1b[38;5;196m"  // Bright red
#define ERROR_RESET           "\x1b[0m"
#define ERROR_NOTE_COLOR      "\x1b[38;5;33m"   // Bright blue (deep sky blue)
#define GRAY_COLOR            "\x1b[38;5;245m"  // Medium-light gray

#define KEYWORD_HIGHLIGHT_COLOR "\x1b[1;38;5;201m"  // Bold + Vivid magenta/pink
#define TYPE_HIGHLIGHT_COLOR    "\x1b[1;38;5;226m"  // Bold + Bright yellow

#define ERROR(x, ...) printf(ERROR_COLOR "[ERROR] " x "\x1b[0m\n", ##__VA_ARGS__);
#define SUCCESS(x,  ...) printf("\x1b[32m[SUCCESS] " x "\x1b[0m\n", ##__VA_ARGS__);
#define WARN(x,  ...) printf("\x1b[33m[WARN] " x "\x1b[0m\n", ##__VA_ARGS__);
#define UNIMPORTANT(x, ...) printf("\x1b[30m[UNIMPORTANT] " x "\x1b[0m\n", ##__VA_ARGS__);
#else
#define ERROR(x, ...) printf("[ERROR] " x '\n', __VA_ARGS__)
#define SUCCESS(x,  ...) printf("[SUCCESS] " x '\n', __VA_ARGS__)
#define WARN(x,  ...) printf("[WARN] " x '\n', __VA_ARGS__)
#define UNIMPORTANT(x, ...) printf("[UNIMPORTANT] " x '\n', ##__VA_ARGS__);

#endif
