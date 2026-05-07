#pragma once

// Diagnostic fields are available in Debug builds and for the diagnostic App
#if defined(_DEBUG) || defined(EGOTOUCH_DIAGNOSTICS)
#define EGOTOUCH_DIAG 1
#else
#define EGOTOUCH_DIAG 0
#endif
