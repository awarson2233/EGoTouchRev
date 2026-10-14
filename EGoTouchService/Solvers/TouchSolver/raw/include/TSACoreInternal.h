#pragma once
// ============================================================================
// TSACoreInternal.h
// Master Umbrella Header for Reconstructed TSACore Algorithm Pipeline
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// Global Bitmasks & Scalar Constants
// ----------------------------------------------------------------------------
#define PROC_TSA_ACTIVE 0x40000000u
#define PROC_MS_ACTIVE  0x80000000u

static const char _rdata = '\0';

#include "TSAPrunedStubs.h"
#include "TSAPrmtStaticTables.h"
#include "TSABssMemoryImage.h"
#include "TSAFuncDecls.h"

// Common/TSAState.c：回放比对用的状态登记，不属于原 DLL
void TSAState_OnAlloc(void *ptr,uint32_t size);
void TSAState_OnFree(void *ptr);

// ----------------------------------------------------------------------------
// Type Aliases
// ----------------------------------------------------------------------------
typedef PeakTable_t PeakArray;

// ----------------------------------------------------------------------------
// String Constants for Core/AFE.c
// ----------------------------------------------------------------------------
#define s_CapsIsNull_6bb5bd97                "CapsIsNull"
#define s_FrameIsNull_6bb5bdba               "FrameIsNull"
#define s_InfoIsNull_6bb5bde8                "InfoIsNull"
#define s_NoFreqShiftDoneflag_6bb5b206       "NoFreqShiftDoneflag"
#define s_MaxRaw2RawDif_exceed_6bb5b5bf      "MaxRaw2RawDif_exceed"
#define s_MinRaw2RawDif_exceed_6bb5b5fa      "MinRaw2RawDif_exceed"
#define s_FreqShiftLatency_exceed_6bb5b635   "FreqShiftLatency_exceed"
#define s_IDLEEnterTime_exceed_6bb5b7e1      "IDLEEnterTime_exceed"
#define s_IDLEExitTime_exceed_6bb5b844       "IDLEExitTime_exceed"
#define s_IDLEIntervalTime_exceed_6bb5b8e3   "IDLEIntervalTime_exceed"
#define s_IDLEIntervalTime_TooShort_6bb5b959 "IDLEIntervalTime_TooShort"

// ----------------------------------------------------------------------------
// Data & RData Memory Aliases
// ----------------------------------------------------------------------------
// Sine table at 0x6bb4f040 in .data (12-bit fixed point sin table)
#define g_miscSinTable (*((const uint8_t*)TSA_MEM_AT(0x6bb4f040ULL)))

// RT Reflection literal pointer tables in .data
#define PTR___compound_literal_1_6bb4fce8 (*(uint8_t**)TSA_MEM_AT(0x6bb4fce8ULL))
#define PTR___compound_literal_3_6bb4fcf8 (*(uint8_t**)TSA_MEM_AT(0x6bb4fcf8ULL))

// Dynamic & CMF parameter alias macros
#define _g_tsaPrmtDynamic (*((undefined8*)TSA_MEM_AT(0x6bcab680ULL)))
#define _DAT_6bcab688     (*((undefined8*)TSA_MEM_AT(0x6bcab688ULL)))
#define _DAT_6bcab690     (*((undefined8*)TSA_MEM_AT(0x6bcab690ULL)))
#define _DAT_6bcab698     (*((undefined8*)TSA_MEM_AT(0x6bcab698ULL)))
#define _DAT_6bcab6a0     (*((undefined4*)TSA_MEM_AT(0x6bcab6a0ULL)))
#define _DAT_6bcab6a4     (*((undefined8*)TSA_MEM_AT(0x6bcab6a4ULL)))
#define _DAT_6bcab6b4     (*((undefined8*)TSA_MEM_AT(0x6bcab6b4ULL)))
#define _DAT_6bcab6c4     (*((short*)TSA_MEM_AT(0x6bcab6c4ULL)))
#define _DAT_6bcab700     (*((undefined4*)TSA_MEM_AT(0x6bcab700ULL)))

#define _g_param          (*((undefined8*)TSA_MEM_AT(0x6bb80080ULL)))
#define _DAT_6bb80090     (*((void**)TSA_MEM_AT(0x6bb80090ULL)))
#define _DAT_6bb800b0     (*((void**)TSA_MEM_AT(0x6bb800b0ULL)))
#define _DAT_6bb800b8     (*((void**)TSA_MEM_AT(0x6bb800b8ULL)))
#define _DAT_6bb800c8     (*((undefined8*)TSA_MEM_AT(0x6bb800c8ULL)))
#define _DAT_6bb800d0     (*((void**)TSA_MEM_AT(0x6bb800d0ULL)))

#define _DAT_6bb4f202     (*((uint16_t*)TSA_MEM_AT(0x6bb4f202ULL)))
#define _DAT_6bb4f208     (*((uint32_t*)TSA_MEM_AT(0x6bb4f208ULL)))
#define _DAT_6bb4f244     (*((uint32_t*)TSA_MEM_AT(0x6bb4f244ULL)))
#define _DAT_6bb4f248     (*((uint32_t*)TSA_MEM_AT(0x6bb4f248ULL)))
#define _DAT_6bb4f250     (*((uint32_t*)TSA_MEM_AT(0x6bb4f250ULL)))
#define _DAT_6bb4f254     (*((uint32_t*)TSA_MEM_AT(0x6bb4f254ULL)))
#define _DAT_6bb52d4c     (*((const float*)TSA_MEM_AT(0x6bb52d4cULL)))

#define _DAT_6bc86ca8     (*((uint32_t*)TSA_MEM_AT(0x6bc86ca8ULL)))
#define _DAT_6bc86cac     (*((uint32_t*)TSA_MEM_AT(0x6bc86cacULL)))
#define _DAT_6bc86cb0     (*((uint64_t*)TSA_MEM_AT(0x6bc86cb0ULL)))

// Buffer pointer aliases for TSABuffer_Free & ToeSynaBl
#define g_tsaBufGridIIRDif                    (*((void**)TSA_MEM_AT(0x6bb81348ULL)))
#define g_tsaBufBlNormal                      (*((void**)TSA_MEM_AT(0x6bb81358ULL)))
#define g_tsaBufBlGrid                        (*((void**)TSA_MEM_AT(0x6bb81350ULL)))
#define g_tsaBufBl                            (*((void**)TSA_MEM_AT(0x6bb81310ULL)))
#define g_tsaBufTzGrid                        (*((void**)TSA_MEM_AT(0x6bb81360ULL)))
#define g_tsaBufSd                            (*((void**)TSA_MEM_AT(0x6bb81368ULL)))
#define g_tsaBufSSChargerNFilterDif           (*((void**)TSA_MEM_AT(0x6bb81370ULL)))
#define g_tsaBufRawStoredSinceSSNoSignal      (*((void**)TSA_MEM_AT(0x6bb81380ULL)))
#define g_tsaBufRawStoredSinceMutualSSNotMatch (*((void**)TSA_MEM_AT(0x6bb81388ULL)))
#define g_tsaBufRawStoredSinceClothRisk       (*((void**)TSA_MEM_AT(0x6bbbf598ULL)))
#define g_tsaBufPreCMFDif                     (*((void**)TSA_MEM_AT(0x6bc38448ULL)))
#define g_tsaBufPreDif                        (*((void**)TSA_MEM_AT(0x6bb81340ULL)))
#define g_tsaBufPreHWRaw                      (*((void**)TSA_MEM_AT(0x6bb81328ULL)))
#define g_tsaBufRaw2rawDif                    (*((void**)TSA_MEM_AT(0x6bb81330ULL)))
#define g_tsaBufRaw2rawDifAftCMF              (*((void**)TSA_MEM_AT(0x6bb81338ULL)))
#define g_tsaBufCommonModeNoise               (*((void**)TSA_MEM_AT(0x6bb81320ULL)))


// ----------------------------------------------------------------------------
// Final Phase Fixes: Symbols, Buffers & Functions
// ----------------------------------------------------------------------------
#define s_ShiftToAbnormalFreq_6bb5b1cb  "ShiftToAbnormalFreq"
#define s_AbnormalExitIdleMode_6bb5b2af "AbnormalExitIdleMode"
#define s_FailedExitIdleMode_6bb5b306   "FailedExitIdleMode"

#define _DAT_6bb4f204 (*((uint32_t*)TSA_MEM_AT(0x6bb4f204ULL)))
#define _DAT_6bb4f210 (*((uint16_t*)TSA_MEM_AT(0x6bb4f210ULL)))
#define _DAT_6bb4f212 (*((uint16_t*)TSA_MEM_AT(0x6bb4f212ULL)))

#define _DAT_6bcab6c8 (*((short*)TSA_MEM_AT(0x6bcab6c8ULL)))
#define _DAT_6bb51d50 (*((const double*)TSA_MEM_AT(0x6bb51d50ULL)))

#define g_afeHalSpec   TSA_VAR_AT(undefined8, 0x6bb4fa40ULL)
#define g_afeGridData  (*((void**)TSA_MEM_AT(0x6bbaa898ULL)))
#define g_afeLineData  (*((void**)TSA_MEM_AT(0x6bbaa8a0ULL)))
#define g_simuAFEInfo  (*((AFE_Info*)TSA_MEM_AT(0x6bbaa700ULL)))
#define g_simuAFECaps  (*((AFE_Caps*)TSA_MEM_AT(0x6bbaa780ULL)))
#define g_simuAFEFrame (*((AFE_Frame*)TSA_MEM_AT(0x6bbaa800ULL)))
#define _DAT_6bbaa7b0  TSA_VAR_AT(undefined4, 0x6bbaa7b0ULL)
#define DAT_6bbaa7b0   TSA_VAR_AT(uint8_t, 0x6bbaa7b0ULL)

#define g_clickNum     TSA_VAR_AT(uint8_t, 0x6bba3380ULL)
#define g_flickNum     TSA_VAR_AT(uint8_t, 0x6bba3381ULL)

#define g_tsaBufRawCapture            (*((void**)TSA_MEM_AT(0x6bb812e8ULL)))
#define g_tsaBufRawCaptureNextSide    (*((void**)TSA_MEM_AT(0x6bb812f0ULL)))
#define g_tsaBufTzAge                 (*((void**)TSA_MEM_AT(0x6bb812e0ULL)))
#define g_tsaBufCleanBaseline         (*((void**)TSA_MEM_AT(0x6bb812d8ULL)))
#define g_tsaBufPreUnnormalizedRaw    (*((void**)TSA_MEM_AT(0x6bb812b8ULL)))
#define g_tsaBufPreCMFRawGrid         (*((void**)TSA_MEM_AT(0x6bb812d0ULL)))
#define g_tsaBufPreCMFRaw             (*((void**)TSA_MEM_AT(0x6bb81308ULL)))
#define g_tsaBufRawGrid               (*((void**)TSA_MEM_AT(0x6bb812c0ULL)))
#define g_tsaBufDifGrid               (*((void**)TSA_MEM_AT(0x6bb812c8ULL)))
#define g_gridMapBufSize              (*((ushort*)TSA_MEM_AT(0x6bc1f348ULL)))


#define g_rawDataBuf            TSA_VAR_AT(uint8_t, 0x6bc47fc0ULL)
#define g_indexGridData         TSA_VAR_AT(uint32_t, 0x6bb81400ULL)
#define g_ideGridData           TSA_VAR_AT(uint8_t, 0x6bc44f40ULL)
#define _g_ideGridData          (*((undefined8*)TSA_MEM_AT(0x6bc44f40ULL)))
#define g_tsaBufFoldLineSelf    TSA_VAR_AT(uint8_t, 0x6bb81440ULL)
#define g_tsaBufFoldLineTX1     TSA_VAR_AT(uint8_t, 0x6bb816c0ULL)
#define g_tsaBufFoldLineTX2     TSA_VAR_AT(uint8_t, 0x6bb81800ULL)
#define g_frameExt              TSA_VAR_AT(AFE_Frame, 0x6bb81980ULL)


#define g_simuScanFreq    TSA_VAR_AT(uint16_t, 0x6bbaa860ULL)
#define _g_simuScanFreq   TSA_VAR_AT(uint16_t, 0x6bbaa860ULL)
#define g_simuScanRate    TSA_VAR_AT(uint16_t, 0x6bbaa880ULL)
#define _g_simuScanRate   TSA_VAR_AT(uint16_t, 0x6bbaa880ULL)
#define _DAT_6bbaa7ac     TSA_VAR_AT(uint32_t, 0x6bbaa7acULL)
#define _DAT_6bbaa790     (*((void**)TSA_MEM_AT(0x6bbaa790ULL)))
#define _DAT_6bbaa7a0     (*((void**)TSA_MEM_AT(0x6bbaa7a0ULL)))
#define _DAT_6bbaa862     TSA_VAR_AT(uint16_t, 0x6bbaa862ULL)
#define _DAT_6bbaa864     TSA_VAR_AT(uint16_t, 0x6bbaa864ULL)
#define _DAT_6bbaa866     TSA_VAR_AT(uint16_t, 0x6bbaa866ULL)
#define _DAT_6bbaa882     TSA_VAR_AT(uint16_t, 0x6bbaa882ULL)
#define _DAT_6bbaa884     TSA_VAR_AT(uint16_t, 0x6bbaa884ULL)
#define _DAT_6bbaa886     TSA_VAR_AT(uint16_t, 0x6bbaa886ULL)
#define _DAT_6bbaa888     TSA_VAR_AT(uint16_t, 0x6bbaa888ULL)
#define _DAT_6bbaa88a     TSA_VAR_AT(uint16_t, 0x6bbaa88aULL)

#ifdef __cplusplus
}
#endif
