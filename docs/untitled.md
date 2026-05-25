  TSA_Processing
    └─ TSA_MSProcess
        ├─ raw copy / raw check
        ├─ Rawdata_Process
        ├─ SS_Process
        ├─ BLIIR_CalcDiffCommon
        ├─ BLSM_Process
        ├─ CMF_Process
        │   └─ CMF_ProcessDim(1)       // current enabled CMF path
        ├─ GridIIR_Process
        ├─ TPSensor / Self / Side prpt
        ├─ charger-noise checkpoint
        ├─ SignalDisparity_Process     // scaling only, no post compensation
        ├─ Peak_Process mode 3
        │   ├─ Peak_DetectInRange
        │   ├─ Peak_Z8Filter
        │   └─ Peak_Z1Filter fallback
        ├─ TZ / CTD / IDT / TS / TE
        ├─ TouchAction / Gesture / Grip / TouchMode
        └─ TouchReport_Process
