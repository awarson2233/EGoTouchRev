#include "TSACoreInternal.h"

// ============================================================================
// Function: TSAPrmt_PreProcess @ 6bb3bab7
// ============================================================================

void TSAPrmt_PreProcess(void)

{
                    /* 0xfbab7  1650  TSAPrmt_PreProcess */
  TSADynPrmt_PreProcess();
  return;
}


// ============================================================================
// Function: TSAPrmt_Process @ 6bb3bacb
// ============================================================================

void TSAPrmt_Process(void)

{
                    /* 0xfbacb  1651  TSAPrmt_Process */
  TSATTPrmt_Process();
  TSARTPrmt_Process();
  TSAConstPrmt_Process();
  TSARamPrmt_Process();
  TSADynPrmt_Process();
  TSAPrmt_Validate();
  return;
}


// ============================================================================
// Function: TSAPrmt_GetProjectName @ 6bb3b9df
// ============================================================================

TSAFlashPrmt * TSAPrmt_GetProjectName(void)

{
                    /* 0xfb9df  1647  TSAPrmt_GetProjectName */
  return &g_tsaPrmtFlash;
}


// ============================================================================
// Function: TSAPrmt_Validate @ 6bb3b9ec
// ============================================================================

void TSAPrmt_Validate(void)

{
                    /* 0xfb9ec  1654  TSAPrmt_Validate */
  if (0x50 < g_tsaPrmtFlash.bCols) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Prmt/TSA_Prmt.c",0xd7,
                      "TSA_MAX_DIM1 >= TSAPrmt(sys, dim1Len)");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x50 < g_tsaPrmtFlash.bRows) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Prmt/TSA_Prmt.c",0xd8,
                      "TSA_MAX_DIM2 >= TSAPrmt(sys, dim2Len)");
    HardwareAnalyzer_SetSoftException();
  }
  if (0x1324 < g_tsaPrmtFlash.wBufElementCount) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n","../../src/Prmt/TSA_Prmt.c",0xd9,
                      "TSAPrmt(sys, sensorNum) <= TSA_MAX_SENSOR_NUM");
    HardwareAnalyzer_SetSoftException();
  }
  return;
}


// ============================================================================
// Function: TSAPrmt_ReInit @ 6bb3b8e5
// ============================================================================

void TSAPrmt_ReInit(void)

{
                    /* 0xfb8e5  1652  TSAPrmt_ReInit */
  TSATTPrmt_Init();
  TSARTPrmt_Init();
  TSAConstPrmt_Init();
  TSARamPrmt_Init();
  TSAPrmt_Reset();
  return;
}


// ============================================================================
// Function: TSAPrmt_Reset @ 6bb3b740
// ============================================================================

void TSAPrmt_Reset(void)

{
                    /* 0xfb740  1653  TSAPrmt_Reset */
  TSADynPrmt_Init();
  TSAPrmt_Validate();
  return;
}


// ============================================================================
// Function: TSAPrmt_FlashPrmtLoadProject @ 6bb3b759
// ============================================================================

undefined8 TSAPrmt_FlashPrmtLoadProject(undefined8 param_1)

{
  (void)param_1;
  memcpy(&g_tsaPrmtFlash, kW273AS2700_FlashPrmt, sizeof(kW273AS2700_FlashPrmt));
  memcpy(_refptr_g_tsaPrmtFlashCmfPca, kW273AS2700_CmfPcaPrmt, sizeof(kW273AS2700_CmfPcaPrmt));
  memcpy(_refptr_g_tsaPrmtFlashAsa, kW273AS2700_AsaPrmt, sizeof(kW273AS2700_AsaPrmt));
  return 1;
}


// ============================================================================
// Function: TSAPrmt_Init @ 6bb3b90d
// ============================================================================

bool TSAPrmt_Init(void)

{
  char cVar1;
  
                    /* 0xfb90d  1648  TSAPrmt_Init */
  cVar1 = TSAPrmt_FlashPrmtLoadProject(0);
  if (cVar1 != '\0') {
    TSAPrmt_ReInit();
  }
  return cVar1 != '\0';
}


// ============================================================================
// Function: TSAPrmt_LoadProject @ 6bb3b93a
// ============================================================================

undefined8 TSAPrmt_LoadProject(longlong param_1)

{
  char cVar1;
  int iVar2;
  undefined8 uVar3;
  char *_Str1;
  
                    /* 0xfb93a  1649  TSAPrmt_LoadProject */
  if (param_1 == 0) {
    uVar3 = 0;
  }
  else {
    cVar1 = TSAPrmt_FlashPrmtLoadProject(param_1);
    if (cVar1 == '\0') {
      uVar3 = 0;
    }
    else {
      _Str1 = (char *)TSAPrmt_GetProjectName();
      iVar2 = strcmp(_Str1,"DFLT000000");
      if (iVar2 == 0) {
        g_tsaPrmtFlash.bCols = AFE_GetColNum();
        g_tsaPrmtFlash.bRows = AFE_GetRowNum();
        g_tsaPrmtFlash.wBufElementCount =
             (ushort)g_tsaPrmtFlash.bCols * (ushort)g_tsaPrmtFlash.bRows;
      }
      TSAPrmt_ReInit();
      uVar3 = 1;
    }
  }
  return uVar3;
}


