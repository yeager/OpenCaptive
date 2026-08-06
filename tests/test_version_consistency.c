#include "opencaptive.h"

#ifndef OPENCAPTIVE_CMAKE_VERSION_MAJOR
#error "CMake version definitions are required"
#endif

#if OPENCAPTIVE_VERSION_MAJOR != OPENCAPTIVE_CMAKE_VERSION_MAJOR || \
    OPENCAPTIVE_VERSION_MINOR != OPENCAPTIVE_CMAKE_VERSION_MINOR || \
    OPENCAPTIVE_VERSION_PATCH != OPENCAPTIVE_CMAKE_VERSION_PATCH
#error "include/opencaptive.h version must match the CMake project version"
#endif

int main(void) {
    return 0;
}
