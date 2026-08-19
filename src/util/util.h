#pragma once
#include "logger.h"

#define IF_CLEANUP(FUNC, COMP) do { \
    if ((FUNC) == (COMP)) { \
        goto cleanup; \
    } \
} while (0)

#define UNUSED(x) (void)(x)
