#pragma once
// ── Device.h — Facade Header ─────────────────────────────────────────────────
// This header aggregates all Device-layer sub-module type headers so that
// existing #include "Device.h" directives continue to work without changes.
//
// New code should prefer including the specific sub-header it actually needs:
//   common/DeviceError.h   — ChipError, ChipResult<T>
//   common/StylusState.h   — StylusFreqPair, StylusState
//   himax/AfeTypes.h       — AFE_Command, command, THP_AFE_MODE

#include "common/DeviceError.h"
#include "common/StylusState.h"
#include "himax/AfeTypes.h"