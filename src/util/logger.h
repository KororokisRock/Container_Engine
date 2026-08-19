#pragma once

#include <stdio.h>

#define COLOR_RESET   "\x1b[0m"
#define COLOR_INFO    "\x1b[32m"
#define COLOR_WARN    "\x1b[33m"
#define COLOR_ERROR   "\x1b[31m"

#define LOG_INFO(fmt, ...) \
    fprintf(stdout, COLOR_INFO "[INFO] " COLOR_RESET fmt "\n", ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    fprintf(stderr, COLOR_WARN "[WARN] " COLOR_RESET "%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    fprintf(stderr, COLOR_ERROR "[ERROR] " COLOR_RESET "%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define LOG_SYSERR_AND_CLEANUP(FUNC, COMP) do { \
    if ((FUNC) == (COMP)) { \
        fprintf(stderr, COLOR_ERROR "[SYSERR] " COLOR_RESET "[%s:%d] ", __FILE__, __LINE__); \
        perror(#FUNC); \
        goto cleanup; \
    } \
} while (0)

#define LOG_SYSWARN(FUNC, COMP) do { \
    if ((FUNC) == (COMP)) { \
        fprintf(stderr, COLOR_WARN "[SYSWARN] " COLOR_RESET "[%s:%d] ", __FILE__, __LINE__); \
        perror(#FUNC); \
    } \
} while (0)
