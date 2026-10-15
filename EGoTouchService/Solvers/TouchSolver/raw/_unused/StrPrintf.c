// ============================================================================
// Function: StrPrintf_Process @ 6bb3addf
// ============================================================================

void StrPrintf_Process(int param_1,char *param_2,undefined8 param_3,undefined8 param_4)

{
  longlong lVar1;
  char *pcVar2;
  undefined8 local_res18;
  undefined8 local_res20;
  char local_218 [508];
  int local_1c;
  
                    /* 0xfaddf  1432  StrPrintf_Process */
  if (g_strPrintfPutf != (code *)0x0) {
    pcVar2 = local_218;
    for (lVar1 = 0x3e; lVar1 != 0; lVar1 = lVar1 + -1) {
      pcVar2[0] = '\0';
      pcVar2[1] = '\0';
      pcVar2[2] = '\0';
      pcVar2[3] = '\0';
      pcVar2[4] = '\0';
      pcVar2[5] = '\0';
      pcVar2[6] = '\0';
      pcVar2[7] = '\0';
      pcVar2 = pcVar2 + 8;
    }
    pcVar2[0] = '\0';
    pcVar2[1] = '\0';
    pcVar2[2] = '\0';
    pcVar2[3] = '\0';
    local_res18 = param_3;
    local_res20 = param_4;
    local_1c = vsnprintf_s(local_218,500,499,param_2,(va_list)&local_res18);
    if (local_1c != -1) {
      if (param_1 == 0) {
        if (20000 < g_logTsaBufCnt + 500U) {
          (*g_strPrintfPutf)(&g_logBuf,0);
          StrPrintf_ClearLogBuffer();
        }
        g_logTsaBufPtr = StrPrintf_CatLogBuffer(g_logTsaBufPtr,&g_logTsaBufCnt,local_218);
      }
      else {
        (*g_strPrintfPutf)(local_218,param_1);
      }
    }
  }
  return;
}


// ============================================================================
// Function: StrPrintf_ClearLogBuffer @ 6bb3aca0
// ============================================================================

void StrPrintf_ClearLogBuffer(void)

{
  memset_s(&g_logBuf,20000,0,20000);
  g_logTsaBufPtr = &g_logBuf;
  g_logTsaBufCnt = 0;
  return;
}


// ============================================================================
// Function: StrPrintf_CatLogBuffer @ 6bb3ace3
// ============================================================================

char * StrPrintf_CatLogBuffer(char *param_1,int *param_2,char *param_3)

{
  char *local_res8;
  char *local_res18;
  
  for (local_res8 = param_1; local_res18 = param_3, *local_res8 != '\0'; local_res8 = local_res8 + 1
      ) {
  }
  while (*local_res18 != '\0') {
    *local_res8 = *local_res18;
    local_res8 = local_res8 + 1;
    local_res18 = local_res18 + 1;
    *param_2 = *param_2 + 1;
  }
  return local_res8 + -1;
}


// ============================================================================
// Function: StrPrintf_Init @ 6bb3ad43
// ============================================================================

void StrPrintf_Init(undefined8 param_1)

{
  g_strPrintfPutf = param_1;
                    /* 0xfad43  1431  StrPrintf_Init */
  StrPrintf_ClearLogBuffer();
  return;
}


// ============================================================================
// Function: StrPrintf_Backup @ 6bb3ad9d
// ============================================================================

void StrPrintf_Backup(void)

{
                    /* 0xfad9d  1429  StrPrintf_Backup */
  StrPrintf_FlushProcess();
  g_strPrintfPutfBackup = g_strPrintfPutf;
  return;
}


// ============================================================================
// Function: StrPrintf_Retore @ 6bb3adbe
// ============================================================================

void StrPrintf_Retore(void)

{
                    /* 0xfadbe  1433  StrPrintf_Retore */
  StrPrintf_FlushProcess();
  g_strPrintfPutf = g_strPrintfPutfBackup;
  return;
}


// ============================================================================
// Function: StrPrintf_FlushProcess @ 6bb3ad66
// ============================================================================

void StrPrintf_FlushProcess(void)

{
                    /* 0xfad66  1430  StrPrintf_FlushProcess */
  if (g_strPrintfPutf != (code *)0x0) {
    (*g_strPrintfPutf)(&g_logBuf,0);
    StrPrintf_ClearLogBuffer();
  }
  return;
}


