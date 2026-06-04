#pragma once

// Diagnostic fields are available in Debug builds and for the diagnostic App.
#ifndef EGOTOUCH_DIAG
#if defined(_DEBUG) || defined(EGOTOUCH_DIAGNOSTICS)
#define EGOTOUCH_DIAG 1
#else
#define EGOTOUCH_DIAG 0
#endif
#endif

// Runtime config is independent from diagnostic fields. It is enabled only when
// the build system explicitly defines EGOTOUCH_CONFIG_ENABLED=1.
#ifndef EGOTOUCH_CONFIG_ENABLED
#define EGOTOUCH_CONFIG_ENABLED 0
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

