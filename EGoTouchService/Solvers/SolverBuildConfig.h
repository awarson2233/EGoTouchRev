#pragma once

// Diagnostic fields are available in Debug builds and for the diagnostic App
#if defined(_DEBUG) || defined(EGOTOUCH_DIAGNOSTICS)
#define EGOTOUCH_DIAG 1
#else
#define EGOTOUCH_DIAG 0
#endif

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

