#pragma once

#include <stdio.h>
#include <errno.h>

#define LOG_ERROR_AND_CLEANUP(FUNC, COMP) do { \
    if ((FUNC) == (COMP)) { \
        perror("ERROR"); \
        goto cleanup; \
    } \
} while (0)


#define IF_CLEANUP(FUNC, COMP) do { \
    if ((FUNC) == (COMP)) { \
        goto cleanup; \
    } \
} while (0)


#define UNUSED(x) (void)(x)
