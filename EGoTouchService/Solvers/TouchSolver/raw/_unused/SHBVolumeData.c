// ============================================================================
// Function: SHBVolumeData_Process @ 6bafee0d
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void SHBVolumeData_Process(void)

{
  uint uVar1;
  uint uVar2;
  ushort uVar3;
  int iVar4;
  uint local_c;
  
                    /* 0xbee0d  1184  SHBVolumeData_Process */
  if (((*_refptr_g_tsaStaticPtr)->dwFeatureFlags & 0x8000) != 0) {
    SHBVolumeData_Reset();
    uVar1 = *(uint *)(_refptr_g_touchesGrid + 0x18);
    uVar2 = *(uint *)(_refptr_g_touchesSide + 0x18);
    iVar4 = SideTouch_GetVirtualVolumeKey();
    if (iVar4 == 0x72) {
      DAT_6bbaa010 = DAT_6bbaa010 | 4;
    }
    else if (iVar4 == 0x73) {
      DAT_6bbaa010 = DAT_6bbaa010 | 2;
    }
    else if (iVar4 == 0xff) {
      DAT_6bbaa010 = DAT_6bbaa010 | 8;
    }
    uVar3 = SideTouch_GetVirtualVolumeYPos();
    DAT_6bbaa014 = (uint)uVar3;
    SHBVolumeData_EventMapping(uVar1 | uVar2);
    (*_refptr_g_tsaStaticPtr)->nField_0x28c = (*_refptr_g_tsaStaticPtr)->nField_0x28c & 0xfffffffd;
    if (DAT_6bbaa02c != DAT_6bbaa008._4_4_) {
      (*_refptr_g_tsaStaticPtr)->nField_0x28c = (*_refptr_g_tsaStaticPtr)->nField_0x28c | 2;
      SHBVolumeData_PrintTPEventAppLog();
    }
    if (DAT_6bbaa030 != DAT_6bbaa010) {
      (*_refptr_g_tsaStaticPtr)->nField_0x28c = (*_refptr_g_tsaStaticPtr)->nField_0x28c | 2;
      SHBVolumeData_PrintVolumeKeyAppLog();
    }
    if (DAT_6bbaa010 != 0) {
      if (g_prevReportedYPos < DAT_6bbaa014) {
        local_c = DAT_6bbaa014 - g_prevReportedYPos;
      }
      else {
        local_c = g_prevReportedYPos - DAT_6bbaa014;
      }
      if (*(ushort *)&_refptr_g_tsaPrmtFlash[2].field_0x66a <= local_c) {
        (*_refptr_g_tsaStaticPtr)->nField_0x28c = (*_refptr_g_tsaStaticPtr)->nField_0x28c | 2;
      }
    }
    g_dataTPToPressure = (*_refptr_g_tsaStaticPtr)->pFrameBufferPtr;
    if (((*_refptr_g_tsaStaticPtr)->nField_0x28c & 2U) != 0) {
      g_prevReportedYPos = DAT_6bbaa014;
    }
    _DAT_6bbaa028 = CONCAT44(DAT_6bbaa008._4_4_,(undefined4)DAT_6bbaa008);
    _DAT_6bbaa030 = CONCAT44(DAT_6bbaa014,DAT_6bbaa010);
    _g_dataTPToPressurePrev = g_dataTPToPressure;
  }
  return;
}


// ============================================================================
// Function: SHBVolumeData_Reset @ 6bafe900
// ============================================================================

void SHBVolumeData_Reset(void)

{
  int iVar1;
  
                    /* 0xbe900  1185  SHBVolumeData_Reset */
  iVar1 = memset_s(&g_dataTPToPressure,0x18,0,0x18);
  if (iVar1 != 0) {
    StrPrintf_Process(0,"\nAssertion failed: %s line %d: \"%s\"\n",
                      "../../src/Extension/SHBVolumeData/SHBVolumeData.c",0x18,"rc == EOK");
    HardwareAnalyzer_SetSoftException();
  }
  DAT_6bbaa008._0_4_ = 0x20100;
  return;
}


// ============================================================================
// Function: SHBVolumeData_EventMapping @ 6bafe9e6
// ============================================================================

void SHBVolumeData_EventMapping(uint param_1)

{
  char cVar1;
  
                    /* 0xbe9e6  1175  SHBVolumeData_EventMapping */
  DAT_6bbaa008._4_4_ = 0;
  if ((param_1 & 2) != 0) {
    DAT_6bbaa008._4_4_ = 2;
  }
  if ((param_1 & 4) != 0) {
    DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 4;
  }
  if ((param_1 & 8) != 0) {
    DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 8;
  }
  if ((param_1 & 0x10) != 0) {
    DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x10;
  }
  cVar1 = SideTouch_IsSHBVolumeEventEnabled();
  if (cVar1 != '\0') {
    if ((param_1 & 0x400) != 0) {
      DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x20;
    }
    if ((param_1 & 0x800) != 0) {
      DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x40;
    }
    if ((param_1 & 0x1000) != 0) {
      DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x80;
    }
    if ((param_1 & 0x2000) != 0) {
      DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x100;
    }
    if ((param_1 & 0x4000) != 0) {
      DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x200;
    }
  }
  cVar1 = SideTouch_IsSHBPowerEventEnabled();
  if ((cVar1 != '\0') && ((param_1 & 0x8000) != 0)) {
    DAT_6bbaa008._4_4_ = DAT_6bbaa008._4_4_ | 0x400;
  }
  return;
}


// ============================================================================
// Function: SHBVolumeData_PrintTPEventAppLog @ 6bafeb1c
// ============================================================================

void SHBVolumeData_PrintTPEventAppLog(void)

{
  uint uVar1;
  
                    /* 0xbeb1c  1182  SHBVolumeData_PrintTPEventAppLog */
  uVar1 = DAT_6bbaa008._4_4_ & ~DAT_6bbaa02c;
  if ((uVar1 & 2) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[phone on left hand]");
  }
  if ((uVar1 & 4) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[phone on right hand]");
  }
  if ((uVar1 & 0x20) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[volume trigger on left hand]");
  }
  if ((uVar1 & 0x40) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[volume trigger on right hand]");
  }
  if ((uVar1 & 0x80) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[volume+]");
  }
  if ((uVar1 & 0x100) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[volume-]");
  }
  if ((uVar1 & 0x200) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[volume0]");
  }
  if ((uVar1 & 0x400) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[powerKey]");
  }
  if ((uVar1 & 0x800) != 0) {
    StrPrintf_Process(0,"\nAppLog: ");
    StrPrintf_Process(4,"Ring-shbEvent[volume trigger off]");
  }
  return;
}


// ============================================================================
// Function: SHBVolumeData_PrintVolumeKeyAppLog @ 6bafecd6
// ============================================================================

void SHBVolumeData_PrintVolumeKeyAppLog(void)

{
  uint uVar1;
  
                    /* 0xbecd6  1183  SHBVolumeData_PrintVolumeKeyAppLog */
  uVar1 = DAT_6bbaa030 ^ DAT_6bbaa010;
  if ((uVar1 & 2) != 0) {
    if ((DAT_6bbaa010 & 2) == 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-volumeKey[+Off]");
    }
    else {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-volumeKey[+On]");
    }
  }
  if ((uVar1 & 4) != 0) {
    if ((DAT_6bbaa010 & 4) == 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-volumeKey[-Off]");
    }
    else {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-volumeKey[-On]");
    }
  }
  if ((uVar1 & 8) != 0) {
    if ((DAT_6bbaa010 & 8) == 0) {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-volumeKey[xOff]");
    }
    else {
      StrPrintf_Process(0,"\nAppLog: ");
      StrPrintf_Process(4,"Ring-volumeKey[xOn]");
    }
  }
  return;
}


// ============================================================================
// Function: SHBVolumeData_Init @ 6bafe96d
// ============================================================================

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void SHBVolumeData_Init(void)

{
                    /* 0xbe96d  1181  SHBVolumeData_Init */
  SHBVolumeData_Reset();
  _g_dataTPToPressurePrev = g_dataTPToPressure;
  _DAT_6bbaa028 = DAT_6bbaa008;
  _DAT_6bbaa030 = _DAT_6bbaa010;
  return;
}


// ============================================================================
// Function: SHBVolumeData_GetVolumeKey @ 6bafe9aa
// ============================================================================

undefined4 SHBVolumeData_GetVolumeKey(void)

{
                    /* 0xbe9aa  1179  SHBVolumeData_GetVolumeKey */
  return DAT_6bbaa010;
}


// ============================================================================
// Function: SHBVolumeData_GetTPEvent @ 6bafe9b6
// ============================================================================

undefined4 SHBVolumeData_GetTPEvent(void)

{
                    /* 0xbe9b6  1178  SHBVolumeData_GetTPEvent */
  return DAT_6bbaa008._4_4_;
}


// ============================================================================
// Function: SHBVolumeData_GetYPos @ 6bafe9c2
// ============================================================================

undefined4 SHBVolumeData_GetYPos(void)

{
                    /* 0xbe9c2  1180  SHBVolumeData_GetYPos */
  return DAT_6bbaa014;
}


