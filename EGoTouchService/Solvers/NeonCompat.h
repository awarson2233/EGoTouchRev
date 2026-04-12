#pragma once

#if defined(_M_ARM64) || defined(__aarch64__) || defined(__ARM_NEON)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-attributes"
#endif

#include <arm_neon.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif
