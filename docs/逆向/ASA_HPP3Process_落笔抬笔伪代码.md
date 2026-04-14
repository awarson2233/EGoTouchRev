# ASA_HPP3Process 落笔/抬笔 C 风格伪代码

> [!NOTE]
> **类型**: 逆向伪代码文档  
> **来源**: `TSACore.dll` / Ghidra 反编译结果整理  
> **目的**: 用 C 代码风格表达 HPP3 落笔/抬笔链路  
> **说明**: 以下内容是便于阅读的逆向伪代码，不是可直接编译的项目源码

---

## 1. 关键状态的伪结构

```c
typedef struct AsaPenState {
    // 本帧 / 上一帧状态
    uint32_t cur_static_bits;      // DAT_18231950
    uint32_t prev_static_bits;     // DAT_18231954
    uint32_t rpt_static_bits;      // DAT_18231b28

    // 压力相关
    uint16_t cur_pressure;         // DAT_18231b18
    uint16_t prev_pressure;        // DAT_18231c18
    uint8_t  real_press_valid;     // DAT_18231964
    uint8_t  no_press_ink_valid;   // DAT_18231965

    // 无压出墨去抖
    uint8_t  no_press_enter_cnt;   // DAT_18231967
    uint8_t  no_press_exit_cnt;    // DAT_18231966

    // 边缘续压
    uint8_t  first_release;        // g_firstRelease
    uint8_t  need_edge_high_speed; // g_needCoor2EdgeHighSpeed

    // 各类抑制/拖尾状态
    uint8_t  bt_press_exit_flag;           // g_hpp3ExitFlag
    uint8_t  disable_press_edge_sig_low;   // g_disablePressEdgeSignalIsTooLow
    uint8_t  fake_press_added;             // g_fakePressureDecreaseAdded
    uint8_t  fake_press_add_num;           // g_fakePressureDecreaseAddNum
} AsaPenState;
```

---

## 1.1 gaokunhimaxcsot 的实际参数镜像

```c
enum {
    // TSAPrmt.dll 中的真实 ASA 表体，不是 0x6861B180 那个指针变量本身
    GAOKUN_ASA_BASE          = 0x6861A340,
    GAOKUN_ASA_SIZE          = 0x0E40,

    // ASA_LoadStylusHWPrmt(0) 最终会落到 unit0，因为：
    // numUnits = 1
    // unit0.hwVersionSlots = { 5, 0, 0 }
    GAOKUN_STYLUS_UNIT_BASE  = 0x6861A348,
    GAOKUN_STYLUS_CFG_BASE   = 0x6861A350,

    // g_asaPrmtFlash + 0xA28.. 的全局区
    GAOKUN_DIM1_LENGTH       = 60,      // +0xA28
    GAOKUN_DIM2_LENGTH       = 40,      // +0xA29
    GAOKUN_DIM2_PITCH        = 2640,    // +0xA2A
    GAOKUN_DIM1_PITCH        = 4140,    // +0xA2C
    GAOKUN_PRESS_MAP_MODE    = 2,       // +0xA30, incell 顺序表
    GAOKUN_FEATURE_DEFAULT   = 0x00000000, // +0xA40
    GAOKUN_FEATURE_FORCE     = 0x00000023, // +0xA44
    GAOKUN_NOPRESS_MODE      = 1,       // +0xA50
    GAOKUN_FAKE_PRESS_EN     = 0x0C,    // +0xA74, 代码只判断 !=0
    GAOKUN_PRESSMAP_IDX0     = 0,       // +0xA80
    GAOKUN_PRESSMAP_IDX12    = 0,       // +0xA81

    // g_asaPrmtStylus + 若干关键偏移
    GAOKUN_EDGE_LOW_ENTER    = 1500,    // +0x232
    GAOKUN_EDGE_LOW_EXIT     = 3000,    // +0x236
    GAOKUN_NOPRESS_BASE_THD  = 10000,   // +0x240
    GAOKUN_NOPRESS_RATIO_A   = 100,     // +0x244
    GAOKUN_NOPRESS_RATIO_B   = 30,      // +0x245
    GAOKUN_TILT_BASELINE     = 1000,    // +0x246
    GAOKUN_TILT_MAX          = 10000,   // +0x248
    GAOKUN_TILT_SCALE_INIT   = 29,      // +0x24A
    GAOKUN_TILT_LEARN_STRICT = 1,       // +0x24B
    GAOKUN_BT_SUPPRESS_ENTER = 2200,    // +0x24C
    GAOKUN_BT_SUPPRESS_EXIT  = 3200,    // +0x24E
    GAOKUN_FREQSHIFT_FLAG    = 0,       // +0x26E
};
```

## 1.2 参数装载与运行时 Feature 链

```c
void ASA_LoadProjectPrmt(void *asa_flash)
{
    g_asaPrmtFlash = asa_flash;

    // gaokunhimaxcsot:
    // g_asaPrmtFlash = 0x6861A340
    // *(u8 *)(+0xA28) = 60
    // *(u8 *)(+0xA29) = 40
    DAT_1820d610 = *(uint8_t *)(g_asaPrmtFlash + 0xa28);
    DAT_1820d611 = *(uint8_t *)(g_asaPrmtFlash + 0xa29);

    // gaokunhimaxcsot:
    // *(u16 *)(+0xA68) / *(u16 *)(+0xA6A) 由屏幕尺寸决定
    g_asaPrmt    = *(uint16_t *)(g_asaPrmtFlash + 0xa68);
    DAT_1820d602 = *(uint16_t *)(g_asaPrmtFlash + 0xa6a);

    ASA_LoadStylusHWPrmt(0);
}

void ASA_LoadStylusHWPrmt(uint8_t hwVersion)
{
    if (hwVersion == 0) {
        hwVersion = 5;
    }

    // gaokunhimaxcsot:
    // numUnits = 1
    // unit0.hwVersionSlots = {5, 0, 0}
    // 所以一定选中 unit0
    g_asaPrmtStylus = g_asaPrmtFlash + 8;

    // gaokunhimaxcsot:
    // *(u16 *)(0x6861A34C) = 250
    // *(u16 *)(0x6861A34E) = 200
    DAT_1820d628 = *(uint16_t *)(g_asaPrmtStylus + 0x04);
    DAT_1820d62a = *(uint16_t *)(g_asaPrmtStylus + 0x06);

    // gaokunhimaxcsot:
    // memcpy source = 0x6861A350, size = 0x238
    // first byte = 0x0E
    memcpy(&DAT_1820d630, g_asaPrmtStylus + 0x08, 0x238);
}

void ASAStaticInit(longlong flash_ctx)
{
    memset(&g_asaStatic, 0, sizeof(g_asaStatic));

    // gaokunhimaxcsot 静态默认值：
    // +0xA40 = 0x00000000
    // +0xA44 = 0x00000023
    g_FlagHpp3Feature =
        *(uint32_t *)(g_asaPrmtFlash + 0xa40) |
        *(uint32_t *)(g_asaPrmtFlash + 0xa44);
}

void AsaSetFeatures(uint32_t runtime_feature_mask)
{
    // 注意：
    // 即使 gaokunhimaxcsot 的静态默认值不包含 0x40 / 0x80，
    // 调用方仍然可以在运行时把这两个 bit 带进来。
    g_FlagHpp3Feature =
        runtime_feature_mask |
        *(uint32_t *)(g_asaPrmtFlash + 0xa44);
}

void ASA_EnableHpp3NoPressInkFeature(void)
{
    g_FlagHpp3Feature |= 0x40;
}

void ASA_EnableHpp3NoPressTLearnedFeature(void)
{
    g_FlagHpp3Feature |= 0x80;
}
```

---

## 1.3 文档中仍出现的 DAT 变量别名

```c
// 下面这些是文档里仍然保留的核心 DAT 变量。
// 之所以没有全部强行替换掉，是为了让伪代码还能和 Ghidra 反编译结果逐项对照。

#define grid_cols                    DAT_1820d610
#define grid_rows                    DAT_1820d611
#define stylus_cfg_flags             DAT_1820d630

#define tx1_signal_dim1              DAT_18231160
#define tx1_signal_dim2              DAT_18231162
#define tx1_signal_combined          DAT_18231164
#define tx2_signal_dim1              DAT_18231166
#define tx2_signal_dim2              DAT_18231168
#define tx2_signal_combined          DAT_1823116a

#define tx1_grid_cluster_strength    DAT_182309c6
#define tx2_grid_cluster_strength    DAT_18230a1e

#define tx1_coordinate_q10_x         DAT_18231130
#define tx1_coordinate_q10_y         DAT_18231134
#define tx2_coordinate_q10_x         DAT_18231148
#define tx2_coordinate_q10_y         DAT_1823114c

#define asa_static_bits_cur          DAT_18231950
#define asa_static_bits_prev         DAT_18231954
#define real_press_flag              DAT_18231964
#define no_press_ink_flag            DAT_18231965
#define no_press_exit_debounce       DAT_18231966
#define no_press_enter_debounce      DAT_18231967

#define no_press_exit_thd_dim1       DAT_18231968
#define no_press_enter_thd_dim1      DAT_1823196a
#define no_press_exit_thd_dim2       DAT_1823196c
#define no_press_enter_thd_dim2      DAT_1823196e

#define peak_search_base_threshold   DAT_18231988
#define peak_search_active_threshold DAT_1823198a

#define prev_signal_level_dim1       DAT_18231920
#define prev_signal_level_dim2       DAT_18231922

#define no_press_abnormal_flags      DAT_18231a18
#define cur_pressure_out             DAT_18231b18
#define rpt_static_bits              DAT_18231b28
#define prev_pressure_out            DAT_18231c18
#define prev_rpt_static_bits         DAT_18231c28

#define tx1_peak_signal_a            DAT_18230a76
#define tx1_peak_signal_b            DAT_18230c26
#define tx2_peak_signal_a            DAT_18230dd6
#define tx2_peak_signal_b            DAT_18230f86

#define tx1_valid_flag               DAT_18230a85
#define tx2_valid_flag               DAT_18230c35
#define tx1_signal_abnormal_cnt      DAT_18230a8b
#define tx2_signal_abnormal_cnt      DAT_18230c3b
#define tx1_coor_jump_abs            DAT_18230a8c
#define tx2_coor_jump_abs            DAT_18230c3c

#define tilt_learn_ok_flag           DAT_18219eb4
#define tilt_learn_min_tx2           DAT_18219eb6
#define tilt_learn_max_tx2           DAT_18219eb8
#define tilt_comp_scale_active       DAT_18219eba
#define tilt_comp_scale_candidate    DAT_18219ebc

#define tilt_out_prev_x              DAT_18231c14
#define tilt_out_prev_y              DAT_18231c16
#define tilt_raw_prev_x              DAT_18231c10
#define tilt_raw_prev_y              DAT_18231c12

#define no_press_table_max_signal    DAT_1821786a
#define no_press_short_term_max      DAT_18212a98

// 下面这些仍保留 DAT 命名，主要因为它们更像“运行时缓存/临时寄存器”，
// 而不是稳定的算法参数：
//   DAT_1815e604 / DAT_1815e6d8 / DAT_1815e6e8   // freq-shift 时序状态
//   DAT_1820d602 / DAT_1820d628 / DAT_1820d62a   // 屏幕/硬件尺寸缓存
//   DAT_1820dc38                                 // fake-pressure runtime 驱动量
//   DAT_18231958 / DAT_1823195c / DAT_18231976   // 帧状态 / bypass 状态
//   DAT_18231a20 / DAT_18231a2c / DAT_18231a2d   // 时间戳 / recheck 状态
//   DAT_18231a44 / DAT_18231a50 / DAT_18231a68 / DAT_18231a74
//                                               // TX1/TX2 坐标缓存
//   DAT_18231b1c / DAT_18231b24                 // exit stylus 释放状态
//   DAT_18231b44 / DAT_18231b50 / DAT_18231b68 / DAT_18231b74
//                                               // 边缘 release 前后坐标缓存
```

---

## 2. 顶层入口的落笔/抬笔相关部分

```c
int HPP3_DataProcess_PenStateRelevant(void)
{
    int ret = 0;

    switch (g_flagDataType) {
    case 0:
        ret = ASA_HPP3TX1LineDataProcesss();
        break;
    case 1:
        ret = ASA_HPP3TX1IQLineDataProcesss();
        break;
    case 2:
        ret = ASA_HPP3TX1GridDataProcesss();
        break;
    case 3:
        ret = ASA_HPP3TX1TiedGridDataProcesss();
        break;
    default:
        ret = ASA_HPP3TX1LineDataProcesss();
        break;
    }

    // 对当前设备的这份分析，后文统一按 case 2 / Grid 模式来理解观察链。
    // 也就是说，重点路径是：
    //   ASA_HPP3TX1GridDataProcesss()
    //
    // 注意：
    // Line / IQLine / TiedGrid 路径在 out-range 时通常直接返回，不再进入 ASA_HPP3Process
    // Grid 路径在“本帧 out-range、上一帧仍有状态”时是个例外，仍可能继续执行 ASA_HPP3Process
    if (ret != 0) {
        return 3;
    }

    ASA_HPP3Process();
    return 0;
}

void ASA_HPP3Process(void)
{
    HPP3_PressureProcess();
    NoPressInkProcess();
    HPP3_PostPressureProcess();
    EdgeCoorProcess();
    EdgeCoorPostProcess();
    HPP3_ASAStaticStatusPostProcess();
}
```

---

## 2.1 Peak / Projection / Signal 的生成链

```c
/*
 * 术语约定
 *
 * peak:
 *   指某一条 1D 线响应，或某一块 2D 网格响应中的局部极大值。
 *   物理上对应“笔尖电磁场耦合最强的那一簇电极”。
 *
 * projection:
 *   指把 2D 网格热点沿 dim1 / dim2 方向分别求和，投影成两条 1D 曲线。
 *   这样后续就能复用 line-mode 的峰值定位算法。
 *
 * signal:
 *   指从 peak/projection 里提炼出来的强度量，最后写入：
 *     DAT_18231160 / 162 / 164   // TX1
 *     DAT_18231166 / 168 / 16A   // TX2
 *   它既用于压感/无压出墨，也用于异常检测和后续滤波。
 */

void HPP3_ObservationPipeline_GridMode(void)
{
    // 这台设备按你的说明采用 grid 模式，因此这里只保留 grid 路径。
    //
    // 对应真实入口：
    //   ASA_HPP3TX1GridDataProcesss()
    //
    // 真实调用顺序可以概括为：
    //
    //   2D raw grid
    //     -> GetGridTx1Peaks()
    //         -> HPP3_FindPeakOfNormalGrid()
    //         -> 选出主 hotspot cluster
    //         -> 沿 dim1 / dim2 积分，生成 TX1 的两条 1D projection
    //     -> TX1LinePeaksProcess()
    //         -> 在 projection 上找 peak
    //         -> 算 sub-line 坐标
    //     -> HPP3_NoiseProcess()
    //     -> TX1CoordinateProcess()
    //
    //   如果 TX2 非空：
    //     -> GetGridTx2Peaks()
    //         -> ReduceTX2DataTx1Peaks()
    //         -> 在残差 2D grid 上找 TX2 hotspot cluster
    //         -> 生成 TX2 的 peak / signal / coordinate
    //     -> TiltProcess()
    //
    //   然后：
    //     -> UpdateLineSignal()
    //     -> HPP3_NoisePostProcess()
    //     -> ASAStaticStatusProcess()
    //
    // 物理上这条链的含义是：
    //   先在 2D 场分布里找到笔尖热点，
    //   再把这个热点分别投影成 X/Y 两条 1D 分布，
    //   最后用 1D 峰值和重心/三角算法求连续坐标与强度。
}

int ASA_HPP3TX1GridDataProcesss_Expanded(void)
{
    // 1. 从 TX1 grid 找主热点，并把它投影成两条 1D 曲线
    GetGridTx1Peaks();

    // 真实代码会先把 DAT_182309C6 暂存到 combined TX1 signal
    tx1_signal_combined = DAT_182309c6;

    // 2. 在 TX1 的两条 projection 上提取 peak
    TX1LinePeaksProcess();

    // 3. 对 peak 结果做噪声判定，保留真正可信的主峰
    HPP3_NoiseProcess();

    // 4. 从 TX1 projection 求主坐标
    TX1CoordinateProcess();

    // 5. 若 TX2 无效，只能保留上一帧 tilt；
    //    若 TX2 有效，则继续在 TX2 residual grid 上求辅坐标与 tilt
    if (g_flagTX2NotNull == 0) {
        if (GridTx1Valid()) {
            TiltKeepLastFrame();
        }
    } else {
        GetGridTx2Peaks();
        tx2_signal_combined = DAT_18230a1e;
        TiltProcess();
    }

    // 6. grid 路径最后把最终 signal 收敛到标准输出寄存器
    tx1_signal_dim1     = DAT_182309c6;
    tx1_signal_dim2     = DAT_182309c6;
    tx1_signal_combined = DAT_182309c6;
    tx2_signal_dim1     = DAT_18230a1e;
    tx2_signal_dim2     = DAT_182309c6;
    tx2_signal_combined = DAT_182309c6;

    // 7. 后置噪声过滤可能直接把当前帧判成 invalid / out-range
    HPP3_NoisePostProcess();
    ASAStaticStatusProcess();

    // 8. 最后再根据状态位决定是否继续后续 ASA_HPP3Process
    if (DAT_18231976 == 1) {
        memcpy(&g_curASOut, &g_prevASOut, 0xec);
        ASAPropertyPostProcess();
        return 5;
    }

    if (asa_static_bits_cur == 0) {
        if (asa_static_bits_prev == 0) {
            ASAStaticPostProcess();
            ASAPropertyPostProcess();
            return 5;
        }

        ReleaseASAReportExitStylus();
        return 0;
    }

    return 0;
}

bool GridTx1Valid(void)
{
    // g_asaPrpt 非 0 表示 TX1 grid 至少找到一个有效主 cluster
    return g_asaPrpt != 0;
}

bool GridTx2Valid(void)
{
    // DAT_18230A18 非 0 表示 TX2 residual grid 上成功提取出主 cluster
    return DAT_18230a18 != 0;
}

void HPP3_NoiseProcess_Expanded(void)
{
    // 分别对 TX1 dim1 / dim2 两条 projection 做噪声筛查
    NoiseJudge(2);
    NoiseJudge(3);

    // 从峰列表中选真正可用的 TX1 主峰
    GetRealPeak();

    // 再对 TX2 做同类筛查
    GetTx2RealPeak();
}

void TX1CoordinateProcess_Expanded(void)
{
    if ((stylus_cfg_flags & 1) == 0) {
        // gaokunhimaxcsot:
        // DAT_1820D630 的 bit0 来自 0x6861A350 的首字节 0x0E
        // 因此默认走 TriangleOf，而不是纯 GravityOf。
        g_coors[0] = GetCoordinateByTriangleOf(2);
        g_coors[1] = GetCoordinateByTriangleOf(3);
    } else {
        g_coors[0] = GetCoordinateByGravityOf(2);
        g_coors[1] = GetCoordinateByGravityOf(3);
    }

    // 然后对两维分别做多项式补偿
    g_coors[0] = CoorMultiOrderFitCompensate(g_coors[0], &DAT_1820d6c8);
    g_coors[1] = CoorMultiOrderFitCompensate(g_coors[1], &DAT_1820d708);

    // 最后钳到 sensor grid 合法范围内
    if (g_coors[0] < 0) {
        g_coors[0] = 0;
    } else if ((int)(grid_cols * 0x400) <= g_coors[0]) {
        g_coors[0] = grid_cols * 0x400 - 1;
    }

    if (g_coors[1] < 0) {
        g_coors[1] = 0;
    } else if ((int)(grid_rows * 0x400) <= g_coors[1]) {
        g_coors[1] = grid_rows * 0x400 - 1;
    }

    DAT_18231a50 = SensorPitchSizeMapDim1(g_coors[0], 0x400);
    DAT_18231a74 = SensorPitchSizeMapDim2(g_coors[1], 0x400);
    DAT_18231a44 = DAT_18231a50;
    DAT_18231a68 = DAT_18231a74;
}

void TiltProcess_Expanded(void)
{
    // 1. 若上一帧没有有效 ink，或上一帧压力为 0，则重新初始化 tilt 状态
    if ((((prev_rpt_static_bits & 2) == 0) && ((prev_rpt_static_bits & 4) == 0)) ||
        (prev_pressure_out == 0)) {
        TiltInit();
    }

    // 2. grid 模式下，TX1 / TX2 任一无效，都不能可靠估 tilt，只能沿用上一帧
    if (g_flagDataType == 2 &&
        (!GridTx1Valid() || !GridTx2Valid())) {
        TiltKeepLastFrame();
        return;
    }

    g_flagTX2Start = 1;

    // 3. 先用 TX1/TX2 信号比做一层稳定化
    uint16_t ratio = GetTX1TX2SignalRatio();
    BufTX1TX2SignalRatio(ratio);
    g_signalRatio = GetTX1TX2RatioAverage(3);

    // 4. 再看 TX1 与 TX2 坐标之间的几何差分
    int dx = (short)DAT_18231148 - (short)DAT_18231130;
    int dy = (short)DAT_1823114c - (short)DAT_18231134;
    int len_limit = GetTX1TX2LenLimit();

    // 5. 若差分过于突兀，则优先信任历史缓冲；否则做一层 IIR
    // 真正代码里还会拿 secondary peak 的偏移做一次辅助判定，
    // 这里只保留主逻辑：
    if (((abs(dx) > len_limit) || (abs(dy) > len_limit)) &&
        g_coorDifBufCnt != 0) {
        dx = g_coordifdim1Buf;
        dy = g_coordifdim2Buf;
    } else {
        dx = (dx + g_coordifdim1Buf * 7) / 8;
        dy = (dy + g_coordifdim2Buf * 7) / 8;
    }

    // 6. 再做长度钳位，防止 tilt 爆掉
    int dist = Misc_SqrtUint32(dx * dx + dy * dy);
    if (dist == 0) {
        dist = 1;
    }
    if (len_limit < dist) {
        dx = (len_limit * dx) / dist;
        dy = (len_limit * dy) / dist;
    }

    BufTX1TX2CoorDif(dx, dy);
    dx = GetTX1TX2CoorDifAverage(5, 0);
    dy = GetTX1TX2CoorDifAverage(5, 1);

    g_coordifdim1Buf = dx;
    g_coordifdim2Buf = dy;

    DAT_18231b10 = GetTiltByCoorDif(dx, 0);
    DAT_18231b12 = GetTiltByCoorDif(dy, 1);

    BufDim1Dim2Tilt(DAT_18231b10, DAT_18231b12);

    if (prev_pressure_out == 0 || tilt_out_prev_x == 0) {
        DAT_18231b14 = DAT_18231b10;
        DAT_18231b16 = DAT_18231b12;
    } else {
        DAT_18231b14 = GetTiltAverage(5, 0);
        DAT_18231b16 = GetTiltAverage(5, 1);
    }

    DAT_18231b14 = Tilt1DegreeJitFilter(tilt_out_prev_x, DAT_18231b14);
    DAT_18231b16 = Tilt1DegreeJitFilter(tilt_out_prev_y, DAT_18231b16);
}

void HPP3_NoisePostProcess_Expanded(void)
{
    if (DAT_18231976 != 0) {
        return;
    }

    // 1. TX1 两条 projection 的 signal 比例极端失衡 -> invalid
    if ((tx1_signal_dim2 * 5 < tx1_signal_dim1) ||
        (tx1_signal_dim1 * 5 < tx1_signal_dim2)) {
        tx1_valid_flag = 0;
        tx2_valid_flag = 0;
    }

    // 2. 若上一帧仍有 ink，但当前 signal 比上一帧参考值小太多 -> invalid
    if ((prev_rpt_static_bits & 6) != 0 &&
        ((tx1_signal_dim1 * 5 < prev_signal_level_dim1) ||
         (tx1_signal_dim2 * 5 < prev_signal_level_dim2))) {
        tx1_valid_flag = 0;
        tx2_valid_flag = 0;
    }

    // 3. 若坐标跳变过大 -> invalid
    if ((prev_rpt_static_bits & 6) != 0 &&
        (tx1_coor_jump_abs > 0x1400 || tx2_coor_jump_abs > 0x1400)) {
        tx1_valid_flag = 0;
        tx2_valid_flag = 0;
    }

    // 4. 若两路 signal 比例轻度异常，递增异常计数
    if (((tx1_signal_dim2 * 3) / 2 < tx1_signal_dim1) ||
        ((tx1_signal_dim1 * 3) / 2 < tx1_signal_dim2)) {
        tx1_signal_abnormal_cnt++;
        tx2_signal_abnormal_cnt++;
    }

    // 5. 当前帧仍有效时，刷新“最后一次有效输出时间”
    if ((prev_rpt_static_bits & 6) != 0 &&
        tx1_valid_flag != 0 &&
        tx2_valid_flag != 0) {
        g_lastValidOutputTime = GetRealtime();
    }

    HPP3_UpdateSignalLevel();
}

void TX1LinePeaksProcess(void)
{
    DAT_1823198a = DAT_18231988;

    // HPP3 协议下使用通道 2/3；旧协议下使用通道 0/1
    if (g_flagHPP3Protocol == 0) {
        GetPeaks(0);
        GetPeaks(1);
    } else {
        GetPeaks(2);
        GetPeaks(3);
    }

    PeaksLog();
}

void TX2LinePeaksProcess(void)
{
    // TX2 峰值门限默认取 TX1 的一半
    DAT_1823198a = DAT_18231988 >> 1;
    GetTX2Peaks(4);
    GetTX2Peaks(5);
}

void GetPeaks(uint32_t ch)
{
    // SearchPeak:
    // 1. 在一条 1D line response 上找 local maximum
    // 2. TX1 峰值要求:
    //    - 高于门限 DAT_1823198a
    //    - 不低于左右 1 邻点
    //    - 也不低于左右 2 邻点
    // 3. SearchPeakBoundary 向左右扩展边界
    // 4. UpdatePeakPrpt 计算峰高、三点和、积分面积、平均噪声/投影量
    // 5. GetPeakPos 用 gravity 算 sub-line 位置
    // 6. UpdatePeakUnit 把这个 peak 写入当前通道的 peak list
    SearchPeak(ch);

    // UpdatePeaksAge:
    // 若当前峰与上一帧某个峰的位置索引相差不超过 2 个 electrode，
    // 就把“年龄/连续性”继承过来，并把若干历史统计量做融合。
    // 物理意义：
    // 真正的笔尖峰在连续帧里会平滑移动，
    // 利用 age 可以把“连续稳定的峰”与“一闪而过的噪声峰”区分开。
    UpdatePeaksAge(cur_peak_list, prev_peak_list);
}

void GetTX2Peaks(uint32_t ch)
{
    // TX2 与 TX1 的差异:
    // 1. 仍然在 1D line response 上找 local maximum
    // 2. 但 SearchTx2Peak 的峰条件只检查左右 1 邻点
    // 3. 之后会额外把峰边界内的原始和 local_2a 算出来，作为 TX2 强度量
    SearchTx2Peak(ch);
    UpdatePeaksAge(cur_tx2_peak_list, prev_tx2_peak_list);
}

void SearchPeakBoundary(short *line, uint8_t n, uint16_t peak_idx,
                        uint8_t *left, uint8_t *right, uint8_t ratio_q5)
{
    // 物理意义：
    // peak 不是一个采样点，而是一团“笔尖扩散的包络”。
    // 这里从峰顶往左右走，只要相邻点衰减还不够快，就继续把它视作同一个峰的边界。
}

void UpdatePeakPrpt(short *line_data, short *sig_proj, short *noise_proj, Peak *pk)
{
    // 逆向看到的核心量：
    // pk->height      = 峰顶 - 边界内最小底噪
    // pk->sum3        = 峰顶附近三点和
    // pk->area        = 边界内积分面积(减去底噪)
    // pk->avg_noise   = 边界内平均 noise projection
    // pk->sig_sum     = 边界内 signal projection 累加
    //
    // 物理意义：
    // height 反映局部尖锐程度
    // area   反映能量/覆盖面积
    // avg_noise 反映该峰是否被宽噪声或杂散耦合污染
}

int GetPeakPos(uint32_t ch, Peak *pk)
{
    // GetPeakPos 不返回整数采样点，而是：
    //   整数 peak index * 0x400 + 子像素偏移
    //
    // 偏移来自局部 gravity：
    //   TX1: UpdateTX1GravityData
    //   TX2: UpdateTX2GravityData
    //
    // 物理意义：
    // 笔尖的感应包络通常跨多个相邻电极，
    // 用加权重心比“直接取最大电极编号”更接近真实连续位置。
    return sub_line_coordinate_q10;
}

void HPP3_FindPeakOfNormalGrid(void)
{
    // 在 2D grid 上找局部峰:
    // 1. 每个 cell 既要高于阈值，也要满足 HPP3_GridTypeIsPeak
    // 2. SearchPeakFlag / Push 会把相邻热点并成同一个 cluster
    // 3. Update33PeakSum / UpdateGridPeakUnit 记录 cluster 的总强度和代表峰
    //
    // 物理意义：
    // 2D grid 是一幅“笔场分布图”，这里是在找那块最亮的热点斑块。
}

void GetGridTx1Peaks(void)
{
    HPP3_FindPeakOfNormalGrid();

    // 清空两个 1D projection buffer
    // g_asaData + 0xA0 : dim1 projection
    // g_asaData + 0x140: dim2 projection
    clear_projection_buffers();

    if (g_asaPrpt != 0) {
        // 选出当前主峰 cluster 后：
        // 1. 在该 cluster 覆盖的若干行上求和 -> 得到 dim1 projection
        // 2. 在该 cluster 覆盖的若干列上求和 -> 得到 dim2 projection
        //
        // 物理意义：
        // 把 2D 热点分别沿 X/Y 方向积分，得到两个边缘分布(marginal distribution)。
        // 后续 coordinate solver 就可以把它当成两条 line response 来处理。
        project_selected_tx1_grid_cluster_to_dim1_and_dim2();
    }
}

void ReduceTX2DataTx1Peaks(void)
{
    // TX2 grid 会先减去一部分 TX1 grid:
    //   tx2 = max(tx2 - tx1/5, 0)
    //
    // 物理意义（推断）：
    // TX2 路径里混有一部分与 TX1 共模的主笔场成分。
    // 先减掉 TX1 的 20%，可以更突出 TX2 自己那部分正交/倾角相关响应。
}

void GetGridTx2Peaks(void)
{
    ReduceTX2DataTx1Peaks();

    // 然后在去共模后的 TX2 2D grid 上再做一次 cluster/peak 提取。
    // 最终得到 TX2 的 peak、TX2 的坐标、TX2 的 signal。
    find_tx2_peak_cluster_on_residual_grid();
}

void UpdateLineSignal(void)
{
    // strongest TX1 peak unit -> DAT_18230A76 / DAT_18230C26
    DAT_18231160 = DAT_18230a76;
    DAT_18231162 = DAT_18230c26;

    // combined TX1 signal
    DAT_18231164 = DAT_18230c26;
    if (*(int *)(g_asaPrmtFlash + 0xa50) == 0) {
        DAT_18231164 = (DAT_18230c26 + DAT_18230a76) / 2;
    }

    // strongest TX2 peak unit -> DAT_18230DD6 / DAT_18230F86
    DAT_18231166 = DAT_18230dd6;
    DAT_18231168 = DAT_18230f86;

    // combined TX2 signal
    DAT_1823116a = DAT_18230f86;
    if (*(int *)(g_asaPrmtFlash + 0xa50) == 0) {
        DAT_1823116a = (DAT_18230f86 + DAT_18230dd6) / 2;
    }

    // gaokunhimaxcsot:
    // +0xA50 = 1
    // 所以 TX1 combined signal 默认直接取 DAT_18230C26
    //      TX2 combined signal 默认直接取 DAT_18230F86
    //
    // 物理意义（推断）：
    // 这个设备更信任 dim2/secondary path 的综合强度，不走双维平均。
}

void HPP3_CoordinateProcess(void)
{
    // 先从 TX1 projection 求主坐标
    // 再从 TX2 projection 求辅坐标
    // 然后做多项式补偿、pattern 补偿、signal refresh
    //
    // TX1 / TX2 坐标本质都是：
    //   peak neighborhood -> gravity / triangle -> 连续坐标
}
```

---

## 3. 真实压力链

```c
uint16_t GetPressInMapOrder(void)
{
    // gaokunhimaxcsot:
    // *(u8 *)(g_asaPrmtFlash + 0xA30) = 0x02
    // 因此这个设备默认走 incell 压力顺序表
    if (*(uint8_t *)(g_asaPrmtFlash + 0xa30) == 1) {
        if (g_btPressCnt < 6 && raw_press_buffer != NULL) {
            return raw_press_buffer[g_btPressMapOncell[g_btPressCnt]];
        }
        return raw_press_now;
    }

    if (*(uint8_t *)(g_asaPrmtFlash + 0xa30) == 2) {
        if (g_btPressCnt < 4 && raw_press_buffer != NULL) {
            return raw_press_buffer[g_btPressMapIncell[g_btPressCnt]];
        }
        return raw_press_now;
    }

    return raw_press_now;
}

uint16_t HPP3_GetPressureMapping(uint16_t press_in)
{
    uint8_t curve_idx;
    uint16_t out;

    // gaokunhimaxcsot:
    // +0xA80 = 0
    // +0xA81 = 0
    // 所以默认 HW 和 HW1/HW2 最终都落到 pressure-map index 0
    if (g_curStylusHWVersion == 1 || g_curStylusHWVersion == 2) {
        curve_idx = *(uint8_t *)(g_asaPrmtFlash + 0xa81);
    } else {
        curve_idx = *(uint8_t *)(g_asaPrmtFlash + 0xa80);
    }

    if (press_in == 0xfff) {
        out = 0xfff;
    } else if (press_in > high_curve_start[curve_idx]) {
        out = poly4_high(curve_idx, press_in);
    } else if (press_in > low_curve_start[curve_idx]) {
        out = poly4_low(curve_idx, press_in);
    } else {
        out = press_in;
        if (press_in > 1) {
            out = 1;
        }
    }

    if (out > 0xfff) {
        out = 0xfff;
    }

    return out;
}

void PressureIIR(uint8_t alpha)
{
    if ((int8_t)alpha < 0) {
        alpha = 0x7f;
    }

    g_pen.cur_pressure =
        (uint16_t)(((uint32_t)g_pen.prev_pressure * (0x80 - alpha) +
                    (uint32_t)g_pen.cur_pressure  * alpha) >> 7);
}

void HPP3_SuppressBtPressBySignal(void)
{
    if (g_pen.cur_pressure == 0) {
        g_pen.bt_press_exit_flag = 0;
    }

    // gaokunhimaxcsot:
    // enter threshold = *(u16 *)(g_asaPrmtStylus + 0x24C) = 2200
    if (g_pen.bt_press_exit_flag == 0 &&
        DAT_18231164 < *(uint16_t *)(g_asaPrmtStylus + 0x24c) &&
        DAT_18230c34 == 0 &&
        DAT_18230a84 == 0) {
        g_pen.bt_press_exit_flag = 1;
        g_pen.cur_pressure = 0;
        g_pen.real_press_valid = 0;
        return;
    }

    if (g_pen.bt_press_exit_flag == 1) {
        // gaokunhimaxcsot:
        // exit threshold = *(u16 *)(g_asaPrmtStylus + 0x24E) = 3200
        if (DAT_18231164 > *(uint16_t *)(g_asaPrmtStylus + 0x24e)) {
            g_pen.bt_press_exit_flag = 0;
        } else {
            g_pen.cur_pressure = 0;
            g_pen.real_press_valid = 0;
        }
    }
}

void HPP3_PressureProcess(void)
{
    uint16_t press_in_map_order = 0;

    g_pen.real_press_valid = 0;

    if (raw_press_now == 0) {
        g_pen.cur_pressure = 0;
    } else {
        press_in_map_order = GetPressInMapOrder();
        g_pen.cur_pressure = HPP3_GetPressureMapping(press_in_map_order);

        if (g_pen.cur_pressure != 0 && g_pen.prev_pressure != 0) {
            PressureIIR(0x40);
        }
    }

    g_btPressCnt++;
    HPP3_SuppressBtPressBySignal();

    if (g_pen.cur_pressure != 0) {
        g_pen.real_press_valid = 1;
    }
}
```

---

## 4. 无压出墨链

```c
bool ASA_IsHpp3NoPressInkFeatureEnabled(void)
{
    // 静态默认值里 gaokunhimaxcsot 只有 0x23，
    // 但运行时可以通过 AsaSetFeatures / ASA_Enable 系列函数把 0x40 置上。
    return (g_FlagHpp3Feature & 0x40) != 0;
}

bool ASA_IsHpp3NoPressTLearnedFeatureEnabled(void)
{
    // 这个函数本体确实只是 “检查 g_FlagHpp3Feature 的 bit7”：
    //
    //   direct logic:  (g_FlagHpp3Feature & 0x80) != 0
    //
    // 但它的完整语义要加上上下游一起看：
    // 1. bit7 的来源:
    //    - ASAStaticInit 里的 (+0xA40 | +0xA44)
    //    - AsaSetFeatures(runtime_mask)
    //    - ASA_EnableHpp3NoPressTLearnedFeature()
    //
    // 2. bit7 为 0 时:
    //    - NoPressInkProcess 会直接允许 NoPressInkHandle() 工作
    //    - 不要求先有 learned table
    //
    // 3. bit7 为 1 时:
    //    - 只有 g_noPressPara != 0，才允许 NoPressInkHandle() 工作
    //    - g_noPressPara == 0 时，当前帧无压出墨会被压掉，
    //      转而执行 NoPressInkLearningPrepareProcess()
    //    - g_noPressPara != 0 时，会额外执行 NoPressInkLearningProcess()
    //
    // 所以：
    // 这个函数“直接逻辑”很简单，但“有效逻辑”必须把
    // feature bit 的来源 + NoPressInkProcess 对该 bit 的用法一起算上。
    return (g_FlagHpp3Feature & 0x80) != 0;
}

bool IsTiltLearnedOK(void)
{
    // gaokunhimaxcsot:
    // *(u8 *)(g_asaPrmtStylus + 0x24B) = 1
    // 所以该设备默认启用 learned-tilt 严格检查分支。
    if (*(uint8_t *)(g_asaPrmtStylus + 0x24b) == 0) {
        return true;
    }

    if (DAT_18219eb4 == 1) {
        return true;
    }

    if ((uint32_t)DAT_1823116a < DAT_18219eb8 + 200 &&
        (uint32_t)DAT_18219eb6 < DAT_1823116a + 200) {
        return true;
    }

    return false;
}

uint16_t GetNopressInkTholdFromLearnedTable(uint16_t x, uint16_t y);
uint16_t GetNoPressInkTiltCompensation(void);

void UpdateNoPressInkThold(void)
{
    uint16_t base = GetNopressInkTholdFromLearnedTable(DAT_18231a44, DAT_18231a68);
    uint16_t sum  = (uint16_t)(base + g_noPressInkTiltCompensation);

    // gaokunhimaxcsot:
    // +0x244 = 100
    // +0x245 = 30
    // 因此 DAT_1823196A / 6E 用 100%，DAT_18231968 / 6C 用 30%
    DAT_18231968 = (sum * *(uint8_t *)(g_asaPrmtStylus + 0x245)) / 100;
    DAT_1823196a = (sum * *(uint8_t *)(g_asaPrmtStylus + 0x244)) / 100;
    DAT_1823196c = (sum * *(uint8_t *)(g_asaPrmtStylus + 0x245)) / 100;
    DAT_1823196e = (sum * *(uint8_t *)(g_asaPrmtStylus + 0x244)) / 100;
}

bool EnterToNoPressInk(void)
{
    // gaokunhimaxcsot:
    // *(u32 *)(g_asaPrmtFlash + 0xA50) = 1
    // 所以默认走“双通道都高于门限”的快速进入分支。
    if (*(int *)(g_asaPrmtFlash + 0xa50) == 0) {
        return ((DAT_1823196e >> 1) + (DAT_1823196a >> 1)) < DAT_18231164;
    }

    return (DAT_1823196a < DAT_18231162) &&
           (DAT_1823196e < DAT_18231160);
}

bool ExitToNoPressInk(void)
{
    // gaokunhimaxcsot:
    // +0xA50 = 1
    // 所以默认走“双通道都低于退出门限”的快速退出分支。
    if (*(int *)(g_asaPrmtFlash + 0xa50) == 0) {
        return DAT_18231164 < ((DAT_1823196c >> 1) + (DAT_18231968 >> 1));
    }

    return (DAT_18231162 < DAT_18231968) &&
           (DAT_18231160 < DAT_1823196c);
}

void NoPressInkHandle(void)
{
    BuffTX1And2SignalAndPos();
    CheckSignalAbnormalStatus();

    if (ASA_IsHpp3NoPressTLearnedFeatureEnabled() && !IsTiltLearnedOK()) {
        g_pen.no_press_ink_valid = 0;
        return;
    }

    if (g_flagTX2NotNull != 0) {
        // gaokunhimaxcsot:
        // +0x246 = 1000
        // +0x248 = 10000
        g_noPressInkTiltCompensation = GetNoPressInkTiltCompensation();
    }

    UpdateNoPressInkThold();

    if (!EnterToNoPressInk()) {
        if (g_pen.no_press_ink_valid == 0) {
            g_pen.no_press_enter_cnt = 2;
        }
    } else {
        if (g_pen.no_press_enter_cnt != 0) {
            g_pen.no_press_enter_cnt--;
        }
        if (g_pen.no_press_enter_cnt == 0) {
            g_pen.no_press_ink_valid = 1;
            g_pen.no_press_exit_cnt = 2;
        }
    }

    if (!ExitToNoPressInk()) {
        if (g_pen.no_press_ink_valid != 0) {
            g_pen.no_press_exit_cnt = 2;
        }
    } else {
        if (g_pen.no_press_exit_cnt != 0) {
            g_pen.no_press_exit_cnt--;
        }
        if (g_pen.no_press_exit_cnt == 0) {
            g_pen.no_press_ink_valid = 0;
            g_pen.no_press_enter_cnt = 2;
        }
    }

    if (ASA_IsHpp3CoorReiviseFeatureEnabled() && g_flagTX2Start == 0) {
        g_pen.no_press_ink_valid = 0;
    }
}

void NoPressInkProcess(void)
{
    uint8_t new_mix_flag;

    // 注意：
    // gaokunhimaxcsot 的静态默认值 0x23 不含 0x40/0x80，
    // 但实际机器上如果运行时 feature 已被打开，这里就会进入。
    if (!ASA_IsHpp3NoPressInkFeatureEnabled()) {
        return;
    }

    if (!ASA_IsHpp3NoPressTLearnedFeatureEnabled() || g_noPressPara != 0) {
        NoPressInkHandle();
    } else {
        g_pen.no_press_ink_valid = 0;
    }

    if (ASA_IsHpp3NoPressTLearnedFeatureEnabled()) {
        if (g_noPressPara == 0) {
            NoPressInkLearningPrepareProcess();
        } else {
            NoPressInkLearningProcess();
        }

        new_mix_flag = g_pen.real_press_valid | g_pen.no_press_ink_valid;
        if (lastFlagHPP3NoPressInk_6997 != new_mix_flag) {
            NoPressInkLearningLog();
        }
        lastFlagHPP3NoPressInk_6997 = new_mix_flag;
    }
}
```

---

## 4.1 NoPressInk 的学习阈值与倾角动态补偿

```c
uint16_t GetCompensationByTilt(uint16_t tx2_signal)
{
    uint16_t s = tx2_signal;

    // gaokunhimaxcsot:
    // +0x248 = 10000 -> 上限钳位
    if (s > *(uint16_t *)(g_asaPrmtStylus + 0x248)) {
        s = *(uint16_t *)(g_asaPrmtStylus + 0x248);
    }

    // gaokunhimaxcsot:
    // +0x246 = 1000 -> 死区/基线
    if (s < *(uint16_t *)(g_asaPrmtStylus + 0x246)) {
        s = 0;
    } else {
        s -= *(uint16_t *)(g_asaPrmtStylus + 0x246);
    }

    // tilt_comp_scale_active = DAT_18219EBA
    // 物理意义：
    // TX2 信号越大，通常代表倾斜越明显或耦合路径越长，
    // 就需要给无压阈值加更多补偿量。
    return (tilt_comp_scale_active * s) / 100;
}

uint16_t GetNoPressInkTiltCompensation(void)
{
    // 直接使用 tx2_signal_combined 作为倾角代理量
    return GetCompensationByTilt(tx2_signal_combined);
}

uint16_t GetNopressInkTholdFromLearnedTable(uint16_t x_q10, uint16_t y_q10)
{
    uint16_t x = x_q10 >> 10;
    uint16_t y = y_q10 >> 10;

    if (x >= grid_cols) {
        x = grid_cols - 1;
    }
    if (y >= grid_rows) {
        y = grid_rows - 1;
    }

    // 取当前位置周围 3x3 邻域的已学习阈值均值
    // 若 3x3 邻域都还没学到值，则：
    //   1) 优先退回全局 learned max
    //   2) 再退回 g_asaPrmtStylus + 0x240 (=10000)
    {
        uint8_t x0 = (x == 0) ? 0 : (uint8_t)(x - 1);
        uint8_t x1 = (x < grid_cols - 1) ? (uint8_t)(x + 1) : (uint8_t)(grid_cols - 1);
        uint8_t y0 = (y == 0) ? 0 : (uint8_t)(y - 1);
        uint8_t y1 = (y < grid_rows - 1) ? (uint8_t)(y + 1) : (uint8_t)(grid_rows - 1);
        uint32_t sum = 0;
        uint8_t cnt = 0;

        for (uint8_t yy = y0; yy <= y1; ++yy) {
            for (uint8_t xx = x0; xx <= x1; ++xx) {
                uint16_t idx = yy * grid_cols + xx;
                uint16_t th = learned_long_term_table[idx];
                if (th != 0) {
                    sum += th;
                    cnt++;
                }
            }
        }

        if (cnt != 0) {
            return (uint16_t)(sum / cnt);
        }

        if (no_press_table_max_signal != 0) {
            return no_press_table_max_signal;
        }

        return *(uint16_t *)(g_asaPrmtStylus + 0x240);
    }
}

bool IsTiltMeetLearnedCondition(void)
{
    // 学习只允许在“坐标远离边缘”的区域进行，
    // 避免边缘电场畸变把 learned threshold 学坏。
    if (DAT_18231130 < 0x200) return false;
    if (DAT_18231130 > grid_cols * 0x400 - 0x200) return false;
    if (DAT_18231134 < 0x200) return false;
    if (DAT_18231134 > grid_rows * 0x400 - 0x200) return false;

    if (DAT_18231148 < 0x400) return false;
    if (DAT_18231148 > (grid_cols - 1) * 0x400) return false;
    if (DAT_1823114c < 0x400) return false;
    if (DAT_1823114c > (grid_rows - 1) * 0x400) return false;

    return true;
}

void CheckSignalAbnormalStatus(void)
{
    // 这一步是学习链的守门员，不直接决定当前帧是否出墨，
    // 但会阻止坏样本进入 learned table。
    //
    // 它会设置 no_press_abnormal_flags 的多个 bit，主要包括：
    //  - 单帧信号突变过大
    //  - 多帧信号波动过大
    //  - palm coupling noise 过大
    //  - tilt offset 与当前 signal 不一致
    //
    // 物理意义：
    // 只有“稳定、可信、像真实笔迹”的样本才能拿来学习无压阈值。
    CheckSignalAbnormalStatus_Expanded();
}

void NoPressInkLearningPrepareProcess(void)
{
    // 这个阶段发生在：
    //   learned feature 已开，但 g_noPressPara 还没有建立完成
    //
    // 核心目标：
    //   在真实压感按下阶段，累计一个“全局可用”的初始阈值表。

    // 当从未按下 -> 按下时，增加 down count
    if ((asa_static_bits_prev & 2) == 0 && real_press_flag != 0) {
        increase_prepare_down_count();
    }

    if (real_press_flag != 0) {
        // 使用当前 TX1 combined signal 减掉 tilt compensation，
        // 记录 prepare 阶段的最大可用信号。
        uint16_t usable_sig =
            tx1_signal_combined - GetCompensationByTilt(tx2_signal_combined);
        if (prepare_max_signal < usable_sig) {
            prepare_max_signal = usable_sig;
        }
        prepare_sample_count++;
    }

    // 满足：
    //   down_count > 1
    //   sample_count > 100
    //   no_press_abnormal_flags == 0
    // 才认为可以从 prepare 阶段生成初始表。
    if (prepare_down_count > 1 &&
        prepare_sample_count > 100 &&
        no_press_abnormal_flags == 0) {
        g_noPressPara = 1;
        if (no_press_table_max_signal != 0) {
            for (uint16_t idx = 0; idx < grid_cols * grid_rows; ++idx) {
                if (normalized_seed_table[idx] != 0) {
                    learned_long_term_table[idx] =
                        (uint16_t)((prepare_max_signal * normalized_seed_table[idx]) /
                                   no_press_table_max_signal);
                }
            }
            no_press_table_max_signal = prepare_max_signal;
        }
        prepare_down_count = 0;
        prepare_max_signal = 0;
        prepare_sample_count = 0;
    }

    if (no_press_abnormal_flags != 0) {
        prepare_down_count = 0;
        prepare_max_signal = 0;
        prepare_sample_count = 0;
    }
}

bool IsMeetNoPressInkLearningCondition(void)
{
    learning_stable_counter++;

    // 1. 位置必须在允许学习的区域
    if (!IsTiltMeetLearnedCondition()) {
        learning_stable_counter = 0;
    }

    // 2. 原始压力不能过大，否则说明不是“接近无压”的样本
    if (raw_press_now > 4000) {
        learning_stable_counter = 0;
    }

    // 3. 当前样本不能被异常检测打坏
    if (no_press_abnormal_flags != 0) {
        learning_stable_counter = 0;
    }

    // 连续超过 30 帧才允许写表
    if (learning_stable_counter > 0x1d) {
        learning_stable_counter = 0x1e;
        return true;
    }
    return false;
}

void UpdateMaxSignalInTableShortTerm(void)
{
    LearningFrameAvg avg = get_average_learning_window(0, current_learning_buf_count);
    uint16_t comp = GetCompensationByTilt(avg.tx2_signal);
    no_press_short_term_max = max(no_press_short_term_max, avg.tx1_signal - comp);
}

void CalcNoPressInkThd(void)
{
    LearningFrameAvg avg = get_average_learning_window(10, 20);
    uint16_t comp = GetCompensationByTilt(avg.tx2_signal);
    uint16_t cur_signal = avg.tx1_signal - comp;
    uint16_t cell_x = avg.x_q10 >> 10;
    uint16_t cell_y = avg.y_q10 >> 10;
    if (cell_x >= grid_cols) cell_x = grid_cols - 1;
    if (cell_y >= grid_rows) cell_y = grid_rows - 1;
    uint16_t cell = cell_x + grid_cols * cell_y;

    // 如果当前 tilt 补偿已经比当前信号还大，说明 learned tilt 参数失真，直接清空 tilt 参数。
    if (avg.tx1_signal < comp) {
        ClearTiltPrmt();
        return;
    }

    // 两级守门：
    //   1) 当前信号至少要达到 short-term max 的 80%
    //   2) 当前信号至少要达到 long-term table max 的 70%
    if (cur_signal < no_press_short_term_max * 4 / 5) {
        learning_stable_counter = 0;
        return;
    }
    if (cur_signal < no_press_table_max_signal * 7 / 10) {
        learning_stable_counter = 0;
        return;
    }

    // 若这个 cell 尚未学习，或新值显著更大，则更新 short-term table。
    if (((learned_long_term_table[cell] == 0) ||
         ((learned_long_term_table[cell] * 3) / 5 < cur_signal)) &&
        ((learned_axis_threshold_x[cell_x] == 0) ||
         ((learned_axis_threshold_x[cell_x] * 3) / 5 < cur_signal)) &&
        ((learned_axis_threshold_y[cell_y] == 0) ||
         ((learned_axis_threshold_y[cell_y] * 3) / 5 < cur_signal))) {
        if (short_term_table[cell] == 0 || short_term_table[cell] < cur_signal) {
            short_term_table[cell] = cur_signal;
            short_term_table_tx2[cell] = avg.tx2_signal;
        }
    }

    // 同时更新 tilt 相关统计，用于后续修正整张表。
    UpdateTiltLearningStatistics_Expanded(avg.tx1_signal, avg.tx2_signal);
}

void CheckShortTermTable(void)
{
    // 若当前候选 tilt scale 与 active scale 相差超过 5，
    // 说明姿态条件变了，短期表不再可靠，需要清空重学。
    if (tilt_comp_scale_candidate != 0 &&
        abs((int)tilt_comp_scale_active - (int)tilt_comp_scale_candidate) > 5) {
        ClearShortTermTable();
    }
}

void UpdateLongTermTable(void)
{
    // 把 short-term table 合并进 long-term table
    // 并按 (activeScale - candidateScale) * (tx2 - baseline) 做 tilt 重标定。
    for (uint16_t idx = 0; idx < grid_cols * grid_rows; ++idx) {
        if (short_term_table[idx] != 0) {
            learned_long_term_table[idx] = short_term_table[idx];
            learned_long_term_table_tx2[idx] = short_term_table_tx2[idx];
        }

        if (tilt_comp_scale_candidate != 0 &&
            learned_long_term_table[idx] != 0) {
            learned_long_term_table[idx] =
                (uint16_t)(learned_long_term_table[idx] +
                          ((int16_t)(tilt_comp_scale_active - tilt_comp_scale_candidate) *
                           ((uint16_t)learned_long_term_table_tx2[idx] -
                            *(uint16_t *)(g_asaPrmtStylus + 0x246))) / 100);
        }

        if (no_press_table_max_signal < learned_long_term_table[idx]) {
            no_press_table_max_signal = learned_long_term_table[idx];
        }
    }
}

void UpdateTiltCompScal(void)
{
    if (tilt_comp_scale_candidate != 0) {
        tilt_comp_scale_active = tilt_comp_scale_candidate;
    }
}

void NoPressInkLearningProcess(void)
{
    // btPressBak_6922 记录“上一帧是否真实按下”
    if (real_press_flag == 0) {
        // 从按下 -> 松开时，把刚才积累的 short-term table 收敛进 long-term table
        if (btPressBak_6922 == 1) {
            CheckShortTermTable();
            UpdateLongTermTable();
            UpdateTiltCompScal();
            UpdateSafeTable();
        }
    } else if (btPressBak_6922 == 0) {
        // 从松开 -> 按下时，开启一轮新的 short-term 学习
        learning_stable_counter = 0;
        ClearShortTermTable();
    } else {
        // 连续按下阶段：
        // 若稳定条件不够，则继续累计 short-term max
        // 若稳定条件足够，则尝试把当前窗口写入 threshold table
        if (!IsMeetNoPressInkLearningCondition()) {
            UpdateMaxSignalInTableShortTerm();
        } else {
            CalcNoPressInkThd();
        }
    }

    btPressBak_6922 = real_press_flag;
}
```

---

## 4.2 学习链展开版 Helper

```c
uint16_t GetNopressInkTholdFromLearnedTable_Expanded(uint16_t x_q10, uint16_t y_q10)
{
    uint16_t cell_x = x_q10 >> 10;
    uint16_t cell_y = y_q10 >> 10;

    if (cell_x >= grid_cols) {
        cell_x = grid_cols - 1;
    }
    if (cell_y >= grid_rows) {
        cell_y = grid_rows - 1;
    }

    uint8_t x0 = (cell_x == 0) ? 0 : (uint8_t)(cell_x - 1);
    uint8_t x1 = (cell_x < grid_cols - 1) ? (uint8_t)(cell_x + 1) : (uint8_t)(grid_cols - 1);
    uint8_t y0 = (cell_y == 0) ? 0 : (uint8_t)(cell_y - 1);
    uint8_t y1 = (cell_y < grid_rows - 1) ? (uint8_t)(cell_y + 1) : (uint8_t)(grid_rows - 1);

    uint32_t sum = 0;
    uint8_t cnt = 0;

    for (uint8_t yy = y0; yy <= y1; ++yy) {
        for (uint8_t xx = x0; xx <= x1; ++xx) {
            uint16_t idx = yy * grid_cols + xx;
            uint16_t th = learned_long_term_table[idx];
            if (th != 0) {
                sum += th;
                cnt++;
            }
        }
    }

    if (cnt != 0) {
        return (uint16_t)(sum / cnt);
    }

    if (no_press_table_max_signal != 0) {
        return no_press_table_max_signal;
    }

    // gaokunhimaxcsot: g_asaPrmtStylus + 0x240 = 10000
    return *(uint16_t *)(g_asaPrmtStylus + 0x240);
}

void CheckSignalAbnormalStatus_Expanded(void)
{
    if (history_buf_count < 10) {
        return;
    }

    // bit1/bit2: 单帧跳变 / 短窗累积跳变
    no_press_abnormal_flags &= ~((uint32_t)0x2 | (uint32_t)0x4);

    uint16_t sig_now  = history_tx1_sig[0];
    uint16_t sig_prev = history_tx1_sig[1];
    uint16_t tx2_now  = history_tx2_sig[0];
    uint16_t tx2_prev = history_tx2_sig[1];

    uint16_t instant_jump =
        abs((int)(sig_now + tx2_now) - (int)(sig_prev + tx2_prev));

    if (instant_jump > sig_now / 15) {
        no_press_abnormal_flags |= 0x2;
    }

    int sum_jump_10 = 0;
    for (int i = 0; i < 5; ++i) {
        sum_jump_10 +=
            (history_tx2_sig[5 + i] - history_tx1_sig[5 + i]) +
            (history_tx1_sig[i]     - history_tx2_sig[i]);
    }

    if (abs(sum_jump_10) > sig_now / 3) {
        no_press_abnormal_flags |= 0x4;
    }

    // bit3: palm coupling noise
    no_press_abnormal_flags &= ~0x8;
    if (g_ssDifPtr != 0) {
        int noise_sum = 0;
        uint8_t x0 = (DAT_18230a72 < 2) ? 0 : (uint8_t)(DAT_18230a72 - 2);
        uint8_t x1 = (DAT_18230a72 < grid_cols - 2) ? (uint8_t)(DAT_18230a72 + 2)
                                                    : (uint8_t)(grid_cols - 1);
        for (uint8_t x = x0; x <= x1; ++x) {
            noise_sum += ((int *)g_ssDifPtr)[x];
        }
        if (noise_sum > 2000) {
            no_press_abnormal_flags |= 0x8;
        }

        noise_sum = 0;
        uint8_t y0 = (DAT_18230c22 < 2) ? 0 : (uint8_t)(DAT_18230c22 - 2);
        uint8_t y1 = (DAT_18230c22 < grid_rows - 2) ? (uint8_t)(DAT_18230c22 + 2)
                                                    : (uint8_t)(grid_rows - 1);
        for (uint8_t y = y0; y <= y1; ++y) {
            noise_sum += ((int *)g_ssDifPtr)[grid_cols + y];
        }
        if (noise_sum > 2000) {
            no_press_abnormal_flags |= 0x8;
        }
    }

    // bit4: tilt offset mismatch
    no_press_abnormal_flags &= ~0x10;

    uint16_t tilt_idx = GetTiltCompensationIdx(history_tx2_sig[0]);
    if (tilt_hist_count[tilt_idx] > 10) {
        uint16_t tilt_expected =
            tilt_hist_sum[tilt_idx] / tilt_hist_count[tilt_idx];

        if (sig_now < (tilt_expected * 7) / 8 ||
            (tilt_expected * 9) / 8 < sig_now) {
            no_press_abnormal_flags |= 0x10;
        }
    }

    if (tilt_comp_scale_candidate != 0) {
        uint16_t expected2 =
            DAT_18219ebe + (tilt_comp_scale_candidate * history_tx2_sig[0]) / 100;
        uint16_t diff =
            (sig_now < expected2) ? (expected2 - sig_now) : (sig_now - expected2);
        uint16_t div = 6;
        uint16_t half_span = (learned_tilt_idx_max - learned_tilt_idx_min) / 2;

        if (learned_tilt_idx_max < tilt_idx) {
            div = (uint16_t)((half_span * 6) /
                             ((half_span + tilt_idx) - learned_tilt_idx_max));
        }
        if (tilt_idx < learned_tilt_idx_min) {
            div = (uint16_t)((half_span * div) /
                             ((half_span + learned_tilt_idx_min) - tilt_idx));
        }

        if (expected2 / div < diff) {
            no_press_abnormal_flags |= 0x10;
        }
    }
}

void NoPressInkLearningPrepareProcess_Expanded(void)
{
    // prepare 阶段只吃“真实压感按下”的样本。
    // 这是为了先拿到一批高置信度数据，建立一张不会太离谱的初始阈值表。

    if ((asa_static_bits_prev & 2) == 0 && real_press_flag != 0) {
        // DAT_1820DE06
        prepare_down_count++;
    }

    if (real_press_flag != 0) {
        uint16_t usable_sig =
            tx1_signal_combined - GetCompensationByTilt(tx2_signal_combined);

        // DAT_1820DE02
        if (prepare_max_signal < usable_sig) {
            prepare_max_signal = usable_sig;
        }

        // DAT_1820DE04
        prepare_sample_count++;
    }

    if (prepare_down_count > 1 &&
        prepare_sample_count > 100 &&
        no_press_abnormal_flags == 0) {

        // 一旦 prepare 成功，就认为 long-term table 已经可用
        g_noPressPara = 1;

        // 这里不是“凭空造表”，而是把一张已有的归一化模板
        // 按 prepare_max_signal / no_press_table_max_signal 重新缩放，
        // 生成第一版 long-term threshold table。
        if (no_press_table_max_signal != 0) {
            for (uint16_t idx = 0; idx < grid_cols * grid_rows; ++idx) {
                if (normalized_seed_table[idx] != 0) {
                    learned_long_term_table[idx] =
                        (uint16_t)((prepare_max_signal * normalized_seed_table[idx]) /
                                   no_press_table_max_signal);
                }
            }
            no_press_table_max_signal = prepare_max_signal;
        }

        prepare_down_count   = 0;
        prepare_max_signal   = 0;
        prepare_sample_count = 0;
    }

    if (no_press_abnormal_flags != 0) {
        prepare_down_count   = 0;
        prepare_max_signal   = 0;
        prepare_sample_count = 0;
    }
}

bool IsMeetNoPressInkLearningCondition_Expanded(void)
{
    learning_stable_counter++;

    if (!IsTiltMeetLearnedCondition()) {
        learning_stable_counter = 0;
    }

    if (raw_press_now > 4000) {
        learning_stable_counter = 0;
    }

    if (no_press_abnormal_flags != 0) {
        learning_stable_counter = 0;
    }

    if (learning_stable_counter > 0x1d) {
        learning_stable_counter = 0x1e;
        return true;
    }

    return false;
}

void UpdateMaxSignalInTableShortTerm_Expanded(void)
{
    LearningFrameAvg avg = get_average_learning_window(0, history_buf_count);
    uint16_t comp = GetCompensationByTilt(avg.tx2_signal);
    uint16_t usable_sig = avg.tx1_signal - comp;

    if (no_press_short_term_max < usable_sig) {
        no_press_short_term_max = usable_sig;
    }
}

void UpdateTiltLearningStatistics_Expanded(uint16_t tx1_sig, uint16_t tx2_sig)
{
    uint16_t tilt_idx = GetTiltCompensationIdx(tx2_sig);

    if (tilt_idx < learned_tilt_idx_min) {
        learned_tilt_idx_min = (uint8_t)tilt_idx;
    }
    if (learned_tilt_idx_max < tilt_idx) {
        learned_tilt_idx_max = (uint8_t)tilt_idx;
    }

    tilt_hist_sum[tilt_idx] += tx1_sig;
    tilt_hist_count[tilt_idx]++;

    // 命中上限时做“去均值一份样本”，保持滑动统计而不是无限累加
    if (tilt_hist_count[tilt_idx] >= tilt_hist_depth_limit) {
        tilt_hist_sum[tilt_idx] -= tilt_hist_sum[tilt_idx] / tilt_hist_count[tilt_idx];
        tilt_hist_count[tilt_idx]--;
    }

    if (tx2_sig < tilt_learn_min_tx2) {
        tilt_learn_min_tx2 = tx2_sig;
    }
    if (tilt_learn_max_tx2 < tx2_sig) {
        tilt_learn_max_tx2 = tx2_sig;
    }

    // 只有覆盖到足够宽的 tilt 区间，才尝试拟合新的 tilt compensation scale
    if ((learned_tilt_idx_max - learned_tilt_idx_min) > 10) {
        if ((learned_tilt_idx_max - learned_tilt_idx_min) > 20) {
            tilt_learn_ok_flag = 1;
        }

        uint8_t coef = GetTx2CompCoef(tilt_hist_avg_table,
                                      learned_tilt_idx_min,
                                      learned_tilt_idx_max);
        if (coef == 0) {
            coef = *(uint8_t *)(g_asaPrmtStylus + 0x24a);
        }

        tilt_comp_scale_candidate = coef;
    }
}

void CalcNoPressInkThd_Expanded(void)
{
    // 这里使用 [10,20) 窗口的平均值，而不是最近一帧，
    // 是为了滤掉刚落笔/抬笔瞬间的抖动，取“稳定低压力接触”样本。
    LearningFrameAvg avg = get_average_learning_window(10, 20);
    uint16_t comp = GetCompensationByTilt(avg.tx2_signal);

    if (avg.tx1_signal < comp) {
        ClearTiltPrmt();
        return;
    }

    uint16_t usable_sig = avg.tx1_signal - comp;
    uint16_t cell_x = avg.x_q10 >> 10;
    uint16_t cell_y = avg.y_q10 >> 10;
    if (cell_x >= grid_cols) cell_x = grid_cols - 1;
    if (cell_y >= grid_rows) cell_y = grid_rows - 1;
    uint16_t cell   = cell_x + grid_cols * cell_y;

    if (usable_sig < no_press_short_term_max * 4 / 5) {
        learning_stable_counter = 0;
        return;
    }
    if (usable_sig < no_press_table_max_signal * 7 / 10) {
        learning_stable_counter = 0;
        return;
    }

    // 只有当前 cell 以及它的行/列邻域不比当前值明显更强时，才允许写入。
    // 物理上这是在防止把一团宽噪声当成笔尖中心去学习。
    if (((learned_long_term_table[cell] == 0) ||
         ((learned_long_term_table[cell] * 3) / 5 < usable_sig)) &&
        ((learned_axis_threshold_x[cell_x] == 0) ||
         ((learned_axis_threshold_x[cell_x] * 3) / 5 < usable_sig)) &&
        ((learned_axis_threshold_y[cell_y] == 0) ||
         ((learned_axis_threshold_y[cell_y] * 3) / 5 < usable_sig))) {
        if (short_term_table[cell] == 0 || short_term_table[cell] < usable_sig) {
            short_term_table[cell] = usable_sig;
            short_term_table_tx2[cell] = avg.tx2_signal;
        }
        UpdateTiltLearningStatistics_Expanded(avg.tx1_signal, avg.tx2_signal);
    }
}

void CheckShortTermTable_Expanded(void)
{
    if (tilt_comp_scale_candidate != 0 &&
        abs((int)tilt_comp_scale_active - (int)tilt_comp_scale_candidate) > 5) {
        ClearShortTermTable();
    }
}

void UpdateLongTermTable_Expanded(void)
{
    for (uint16_t idx = 0; idx < grid_cols * grid_rows; ++idx) {
        if (short_term_table[idx] != 0) {
            learned_long_term_table[idx] = short_term_table[idx];
            learned_long_term_table_tx2[idx] = short_term_table_tx2[idx];
        }

        if (tilt_comp_scale_candidate != 0 &&
            learned_long_term_table[idx] != 0) {
            learned_long_term_table[idx] =
                (uint16_t)(learned_long_term_table[idx] +
                          ((int16_t)(tilt_comp_scale_active - tilt_comp_scale_candidate) *
                           ((uint16_t)learned_long_term_table_tx2[idx] -
                            *(uint16_t *)(g_asaPrmtStylus + 0x246))) / 100);
        }

        if (no_press_table_max_signal < learned_long_term_table[idx]) {
            no_press_table_max_signal = learned_long_term_table[idx];
        }
    }
}

void ClearTiltPrmt_Expanded(void)
{
    clear_tilt_hist_sum();
    clear_tilt_hist_count();
    ClearTable();  // 清 long-term table / learned max

    learned_tilt_idx_min     = 0xff;
    learned_tilt_idx_max     = 0;
    tilt_comp_scale_active   = *(uint8_t *)(g_asaPrmtStylus + 0x24a);
    tilt_comp_scale_candidate= 0;
    tilt_learn_ok_flag       = 0;
    tilt_learn_min_tx2       = 0xffff;
    tilt_learn_max_tx2       = 0;
    tilt_hist_depth_limit    = *(uint16_t *)(g_asaPrmtStylus + 0x248);

    g_noPressPara = 0;
    no_press_abnormal_flags &= ~0x1f;
}
```

## 4.3 prepare -> short-term -> long-term 的物理逻辑

```c
/*
 * prepare:
 *   只在“真实按下”的阶段积累样本。
 *   这时笔场最强、样本最可信，所以拿它来做第一版 bootstrap。
 *   物理上相当于先估计：
 *     “这支笔在这块屏上，正常接触时大概能提供多大的可靠信号”
 *
 * short-term:
 *   只在一次连续真实按下期间累积。
 *   它是“本次这笔”的局部临时学习表，反映当前姿态、当前区域、当前贴合状态。
 *   物理上相当于：
 *     “这一次笔迹，在当前位置和当前倾角下，最低还能维持多少可靠耦合”
 *
 * long-term:
 *   在抬笔时把 short-term 合并进去。
 *   这是跨多笔、跨多位置逐渐收敛出来的稳定阈值地图。
 *   物理上相当于：
 *     “这块屏幕不同区域、不同姿态下，进入/保持无压书写大概要多少信号”
 *
 * 为什么分三层：
 *   1. 没有 prepare，就没有 bootstrap，冷启动时 long-term table 为空
 *   2. 没有 short-term，就会把单帧噪声直接写进长期表
 *   3. 没有 long-term，每一笔都要重新适应，快落笔/快抬笔就无法稳定
 */
```

## 4.4 学习链如何反馈到快速落笔/抬笔

```c
/*
 * 反馈路径是：
 *
 * learned_long_term_table
 *   -> GetNopressInkTholdFromLearnedTable(x, y)
 *   -> base_threshold
 *   -> base_threshold + GetNoPressInkTiltCompensation(cur_tx2_signal)
 *   -> UpdateNoPressInkThold()
 *   -> DAT_18231968 / 6A / 6C / 6E
 *   -> EnterToNoPressInk() / ExitToNoPressInk()
 *   -> g_pen.no_press_ink_valid
 *   -> HPP3_PostPressureProcess() 补压
 *   -> 快速落笔 / 快速抬笔
 *
 * 对快速落笔的影响：
 *   base_threshold 越低，
 *   越容易在接近无压、但仍有耦合信号时满足 EnterToNoPressInk，
 *   因而更快进入 no-press ink，表现为“更快落笔/不断墨”。
 *
 * 对快速抬笔的影响：
 *   base_threshold 越高，
 *   ExitToNoPressInk 的 30% 退出阈值也随之提高，
 *   当前信号更早跌破退出门限，
 *   因而更快退出 no-press ink，表现为“更快抬笔/少拖尾”。
 *
 * 对倾角的影响：
 *   倾角越大，TX2 combined signal 往往越大，
 *   GetNoPressInkTiltCompensation() 给出的补偿越大，
 *   base_threshold 会被上调。
 *
 *   物理解释：
 *   倾斜时笔场在传感器上的投影会变宽、变偏、主峰变钝，
 *   如果还沿用直立时的阈值，就容易误判“仍可继续出墨”。
 *   所以系统会在大倾角下自动抬高门限，避免假落笔和拖尾。
 */
```

---

## 4.5 外围但相关的边缘函数

```c
void ASA_SetBluetoothPressure_Expanded(uint16_t p0, uint16_t p1,
                                       uint16_t p2, uint16_t p3)
{
    // 蓝牙侧把 4 个 pressure sample 写进原始缓存
    raw_press_buf0 = p0;
    raw_press_buf1 = p1;
    raw_press_buf2 = p2;
    raw_press_now  = p3;

    DAT_1815e604 = 1;        // 进入 BT pressure 活跃状态
    HPP3_ClearBtPressCnt();  // 让 GetPressInMapOrder 从第 0 个映射样本重新开始

    MultiPanelSwitchUpdateBTFlag();
    MultiPanelSwitchProcess();
    HPP3_InkLeakageJudge();
}

void HPP3_InkLeakageJudge_Expanded(void)
{
    // 这里改的是 no_press_abnormal_flags 的 bit0
    // 它影响的是学习样本是否可信，而不是当前帧直接落笔/抬笔。
    if (!GetMultiPanelSwitchTPFlag() && raw_press_now != 0) {
        no_press_abnormal_flags |= 0x1;
    }
    if (raw_press_now == 0) {
        no_press_abnormal_flags &= ~0x1;
    }
}

void AbnormalPeakInfoJudge_Expanded(void)
{
    // 统计本帧 abnormal peak 的个数和总信号。
    // 这是峰质量评估的外围统计，不直接写 cur_pressure / no_press_ink_flag /
    // asa_static_bits_cur 这些主状态。
    g_curFrameAbnormalPeakNum = 0;
    g_curFrameAbnormalPeakSignal = 0;

    for (uint8_t i = 0; i < tx1_valid_flag; ++i) {
        if (tx1_peak_is_abnormal(i)) {
            g_curFrameAbnormalPeakNum++;
            g_curFrameAbnormalPeakSignal += tx1_peak_signal_sum(i);
        }
    }

    for (uint8_t i = 0; i < tx2_valid_flag; ++i) {
        if (tx2_peak_is_abnormal(i)) {
            g_curFrameAbnormalPeakNum++;
            g_curFrameAbnormalPeakSignal += tx2_peak_signal_sum(i);
        }
    }
}

uint16_t ASA_GetTX1Siganl(void)
{
    return tx1_signal_combined;
}

uint16_t ASA_GetTX2Siganl(void)
{
    return tx2_signal_combined;
}

bool ASA_GetRptInk(void)
{
    return (rpt_static_bits & 2) != 0 ||
           (rpt_static_bits & 4) != 0;
}

uint16_t ASA_GetRptPressure(void)
{
    return cur_pressure_out;
}
```

---

## 5. 最终压力生成与后置抬笔

```c
bool HPP3_BtPenInFreqShifting(void)
{
    if (DAT_1815e604 == 1 && g_freqShift != 0) {
        return true;
    }
    if (DAT_18231a20 < DAT_1815e6e8) {
        return true;
    }
    return false;
}

bool HPP3_BtPenFreqShiftingDebounceTimeOut(void)
{
    uint64_t now = GetRealtime();

    // gaokunhimaxcsot:
    // *(u8 *)(g_asaPrmtStylus + 0x26E) = 0
    // 因此这里默认走“未开启特殊 BT 强制匹配”的时间窗判断。
    if (((*(uint8_t *)(g_asaPrmtStylus + 0x26e) == 0 || now <= DAT_1815e6d8 + 0x96) &&
         (*(uint8_t *)(g_asaPrmtStylus + 0x26e) != 0 || now <= DAT_1815e6d8 + 0x32)) ||
        (DAT_18231a20 <= DAT_1815e6d8 + 0x32)) {
        return false;
    }
    return true;
}

uint16_t HPP3_FakePressureDecreaseProcess(void)
{
    uint16_t fake = 0;

    if (g_pen.fake_press_added == 0 && g_pen.fake_press_add_num == 0) {
        if (DAT_1820dc38 <= 100) {
            g_pen.fake_press_add_num = 0;
        } else if (DAT_1820dc38 < 301) {
            g_pen.fake_press_add_num = 1;
        } else if (DAT_1820dc38 < 501) {
            g_pen.fake_press_add_num = 2;
        } else {
            g_pen.fake_press_add_num = 3;
        }
        g_pen.fake_press_added = 1;
    }

    if (g_pen.fake_press_add_num != 0) {
        fake = (uint16_t)(((uint32_t)g_pen.fake_press_add_num * g_pen.prev_pressure) /
                          (g_pen.fake_press_add_num + 1));
        g_pen.fake_press_add_num--;
    }

    return fake;
}

void HPP3_PostPressureProcess(void)
{
    if (g_pen.real_press_valid == 0 && g_pen.no_press_ink_valid != 0) {
        g_pen.cur_pressure = 10;
        if (g_pen.prev_pressure != 0) {
            g_pen.cur_pressure = g_pen.prev_pressure;
        }
    }

    // gaokunhimaxcsot:
    // *(u8 *)(g_asaPrmtFlash + 0xA74) = 0x0C
    // 因此 fake pressure decrease 的总开关默认是打开的。
    if (!HPP3_BtPenInFreqShifting() || HPP3_BtPenFreqShiftingDebounceTimeOut()) {
        if (*(uint8_t *)(g_asaPrmtFlash + 0xa74) != 0 &&
            g_pen.prev_pressure > 500 &&
            g_pen.cur_pressure < 11) {
            g_pen.cur_pressure = HPP3_FakePressureDecreaseProcess();
        }
    } else {
        g_pen.disable_press_edge_sig_low = 0;
        g_pen.fake_press_added = 0;
        g_pen.fake_press_add_num = 0;
    }

    if (g_pen.cur_pressure == 0) {
        g_pen.disable_press_edge_sig_low = 0;
        g_pen.fake_press_added = 0;
        g_pen.fake_press_add_num = 0;
        return;
    }

    if (g_pen.disable_press_edge_sig_low == 0) {
        // gaokunhimaxcsot:
        // *(u16 *)(g_asaPrmtStylus + 0x232) = 1500
        // 进入边缘低信号抑制的条件
        if ((DAT_18230a84 == 0 || DAT_18230c34 == 0)) {
            if ((DAT_18230a84 != 0 && DAT_18230a78 < *(uint16_t *)(g_asaPrmtStylus + 0x232)) ||
                (DAT_18230c34 != 0 && DAT_18230c28 < *(uint16_t *)(g_asaPrmtStylus + 0x232))) {
                g_pen.disable_press_edge_sig_low = 1;
            }
        } else {
            uint16_t th = ((uint16_t)*(uint16_t *)(g_asaPrmtStylus + 0x232) * 2) / 3;
            if (DAT_18230a78 < th || DAT_18230c28 < th) {
                g_pen.disable_press_edge_sig_low = 1;
            }
        }
    }

    // 退出边缘低信号抑制的条件
    // gaokunhimaxcsot:
    // *(u16 *)(g_asaPrmtStylus + 0x236) = 3000
    if (g_pen.disable_press_edge_sig_low == 1 &&
        (DAT_18230a84 == 0 || *(uint16_t *)(g_asaPrmtStylus + 0x236) < DAT_18230a78) &&
        (DAT_18230c34 == 0 || *(uint16_t *)(g_asaPrmtStylus + 0x236) < DAT_18230c28)) {
        g_pen.disable_press_edge_sig_low = 0;
    }

    // 如果仍处于抑制状态，强制抬笔
    if (g_pen.disable_press_edge_sig_low == 1) {
        g_pen.cur_pressure = 0;
        g_pen.real_press_valid = 0;
    }
}
```

---

## 6. 边缘首帧续压

```c
static uint16_t abs_u16_delta(uint16_t a, uint16_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

void EdgeCoorProcess(void)
{
    bool restore_prev_press = false;

    g_pen.need_edge_high_speed = 0;

    // gaokunhimaxcsot:
    // DAT_1820d610 = 60
    // DAT_1820d611 = 40
    // 所以下面的边缘合法区判断分别以 60x40 传感器网格计算。
    if (g_pen.first_release != 0 &&
        DAT_18230a84 == 1 &&
        g_pen.cur_pressure == 0 &&
        g_pen.prev_pressure != 0 &&
        abs_u16_delta(DAT_18231b50, DAT_18231b44) > 0x200 &&
        DAT_18231b50 > 0x200 &&
        DAT_18231b50 < (DAT_1820d610 * 0x400 - 0x200)) {
        g_pen.need_edge_high_speed = 1;
        g_pen.first_release = 0;
        restore_prev_press = true;
    }

    if (g_pen.first_release != 0 &&
        DAT_18230c34 == 1 &&
        g_pen.cur_pressure == 0 &&
        g_pen.prev_pressure != 0 &&
        abs_u16_delta(DAT_18231b74, DAT_18231b68) > 0x200 &&
        DAT_18231b74 > 0x200 &&
        DAT_18231b74 < (DAT_1820d611 * 0x400 - 0x200)) {
        g_pen.need_edge_high_speed = 1;
        g_pen.first_release = 0;
        restore_prev_press = true;
    }

    if (g_pen.cur_pressure != 0) {
        g_pen.first_release = 1;
    }

    if (restore_prev_press) {
        g_pen.cur_pressure = g_pen.prev_pressure;
    }
}

void EdgeCoorPostProcess(void)
{
    if (g_pen.cur_pressure == 0) {
        return;
    }

    // 近屏幕边缘时对坐标做压缩/贴边处理
    // 这里只保留语义，不展开数学细节
    adjust_output_x_near_edge();
    adjust_output_y_near_edge();
}
```

---

## 7. 本帧状态位提交

```c
void HPP3_ASAStaticStatusPostProcess(void)
{
    if (g_pen.real_press_valid != 0) {
        g_pen.cur_static_bits |= 2;
    }

    if (g_pen.cur_pressure != 0) {
        g_pen.cur_static_bits |= 4;
    }

    if (g_pen.cur_static_bits & 4) {
        EnterNoPressInkMode();
    } else {
        ExitNoPressInkMode();
    }

    if (g_pen.cur_static_bits & 2) {
        EnterInkMode();
    } else {
        ExitInkMode();
    }
}

bool ASA_GetRptInk(void)
{
    return (g_pen.rpt_static_bits & 2) != 0 ||
           (g_pen.rpt_static_bits & 4) != 0;
}
```

---

## 8. `ASA_HPP3Process` 之后仍会影响抬笔的代码

```c
void CoorReviseProcess(void)
{
    if (!ASA_IsHpp3CoorReiviseFeatureEnabled()) {
        return;
    }

    // 这是明确的 stylus up 检测
    if (g_pen.prev_pressure != 0 && g_pen.cur_pressure == 0) {
        CoorReviseInit();
    }

    if (g_flagTX2Start == 0) {
        g_pen.no_press_ink_valid = 0;
        return;
    }

    // 反编译结果里有一个二次读取 g_flagTX2Start 并清 cur_pressure 的分支，
    // 这里不作为稳定主路径，仅保留语义说明：
    if (unlikely_flag_changed_between_two_reads()) {
        g_pen.cur_pressure = 0;
    }

    if (g_flagTX2NotNull != 0) {
        CoorReviseCalculation();
    }

    CoorReviseWork();
}

void ASA_CoorPostProcess(void)
{
    LinearFilterProcess();
    GetRealTimeCoor2Buf();
    Get3PointAvgFilter();
    CoorReviseProcess();
    GetCoorSpeed();
    GetIIRCoef();
    CoorFilterProcess();
    AftCoorProcess();
    FitToLcdScreen();

    // 锁存本帧上报状态位
    g_pen.rpt_static_bits = g_pen.cur_static_bits;

    ASAStaticPostProcess();
    ASAPropertyPostProcess();
}
```

---

## 9. 频点切换保帧

```c
bool HPP3_NeedNoiseDebounce(void)
{
    uint64_t now = GetRealtime();
    return (now < g_lastValidOutputTime + 10) &&
           (g_lastValidOutputTime < now);
}

bool NeedASAReportInFreqShifting(void)
{
    bool in_shift = HPP3_BtPenInFreqShifting();
    bool shift_timeout = HPP3_BtPenFreqShiftingDebounceTimeOut();
    bool noise_debounce = HPP3_NeedNoiseDebounce();

    if ((in_shift && !shift_timeout) ||
        ((DAT_18231c28 & 6) != 0 && noise_debounce)) {

        if ((!ASA_GetRptInk() || DAT_18231976 == 1 || DAT_18231a2d == 0) &&
            ((DAT_18231c28 & 6) != 0)) {
            DAT_18231a2c = 1;
        }

        // 当前帧抬笔，但上一帧仍有状态：直接恢复上一帧状态位
        if (g_pen.cur_static_bits == 0 && g_pen.prev_static_bits != 0) {
            g_pen.cur_static_bits = g_pen.prev_static_bits;
            return true;
        }

        // active stylus 位在当前帧消失，也可保帧
        if ((DAT_1823195c & 0x10) != 0 && (DAT_18231958 & 0x10) == 0) {
            DAT_18231958 = DAT_1823195c;
            return true;
        }

        // 当前帧无 ink，但上一帧有 ink
        if (!ASA_GetRptInk() && ((DAT_18231c28 & 6) != 0)) {
            return true;
        }
    }

    return false;
}

bool ReleaseASAReportInFreqShifting(void)
{
    if (!NeedASAReportInFreqShifting()) {
        return false;
    }

    memcpy(&g_curASOut, &g_prevASOut, 0xec);
    return true;
}
```

---

## 10. 退出 stylus 时的释放逻辑

```c
bool EdgeCoorProcessExitStylusWithInk(void)
{
    bool edge_jump = false;

    if (DAT_18231b44 < 0x400 && DAT_18231b44 < DAT_18231b50) {
        edge_jump = true;
    }
    if ((DAT_1820d610 - 1) * 0x400 < DAT_18231b44 && DAT_18231b50 < DAT_18231b44) {
        edge_jump = true;
    }
    if (edge_jump && abs_u16_delta(DAT_18231b50, DAT_18231b44) > 0x200) {
        DAT_18231a50 = DAT_18231b44;
        DAT_18231a74 = DAT_18231b68;
        return true;
    }

    edge_jump = false;
    if (DAT_18231b68 < 0x400 && DAT_18231b68 < DAT_18231b74) {
        edge_jump = true;
    }
    if ((DAT_1820d611 - 1) * 0x400 < DAT_18231b68 && DAT_18231b74 < DAT_18231b68) {
        edge_jump = true;
    }
    if (edge_jump && abs_u16_delta(DAT_18231b74, DAT_18231b68) > 0x200) {
        DAT_18231a50 = DAT_18231b44;
        DAT_18231a74 = DAT_18231b68;
        return true;
    }

    return false;
}

bool ReleaseASAReportExitStylus(void)
{
    if (ReleaseASAReportInFreqShifting()) {
        return true;
    }

    // 上一帧没有 ink
    if ((DAT_18231c28 & 2) == 0 && (DAT_18231c28 & 4) == 0) {
        if ((DAT_18231c28 & 1) == 0) {
            ASAOutClean();
            ASAStaticCounterClean();
            return true;
        }

        memcpy(&g_curASOut, &g_prevASOut, 0xec);
        g_pen.rpt_static_bits = 1;

        if (DAT_18231b1c == 1 && DAT_18231b24 == 1) {
            DAT_18231b24 = 0;
        } else {
            ASAOutClean();
            ASAStaticCounterClean();
            return true;
        }
    } else {
        // 上一帧有 ink
        memcpy(&g_curASOut, &g_prevASOut, 0xec);

        if (!EdgeCoorProcessExitStylusWithInk()) {
            g_pen.rpt_static_bits = 1;
            g_pen.cur_pressure = 0;
        } else {
            g_pen.rpt_static_bits = 4;
        }
    }

    g_flagTX2Start = 0;
    memcpy(&g_prevASOut, &g_curASOut, 0xec);
    return false;
}
```

---

## 11. WinDbg 动态验证清单

> [!NOTE]
> 以下地址都按 `TSACore` 的 **RVA** 书写。  
> 运行时先用 `lm m TSACore` 找模块基址 `base`，然后用 `base + RVA` 下断点或读内存。

### 11.1 推荐断点

```text
Grid 主路径
  0x8668d  ASA_HPP3TX1GridDataProcesss
  0x7c834  GetGridTx1Peaks
  0x7f23b  TX1LinePeaksProcess
  0x791b6  HPP3_NoiseProcess
  0x6f06d  TX1CoordinateProcess
  0x7d3ac  GetGridTx2Peaks
  0x841e6  TiltProcess
  0x79511  HPP3_NoisePostProcess

落笔/抬笔主链
  0x869c2  ASA_HPP3Process
  0x7fd1d  HPP3_PressureProcess
  0x76996  NoPressInkProcess
  0x76814  NoPressInkHandle
  0x7ffa4  HPP3_PostPressureProcess
  0x6f250  EdgeCoorProcess
  0x88935  HPP3_ASAStaticStatusPostProcess
  0x869ef  ASA_CoorPostProcess

学习链
  0x75198  NoPressInkLearningPrepareProcess
  0x75863  NoPressInkLearningProcess
  0x765d4  CheckSignalAbnormalStatus
  0x75a61  CalcNoPressInkThd
  0x75bde  UpdateLongTermTable

释放/保帧
  0x85218  ReleaseASAReportInFreqShifting
  0x8527b  ReleaseASAReportExitStylus
```

### 11.2 建议重点观察的变量

```text
信号
  0x231160  tx1_signal_dim1
  0x231162  tx1_signal_dim2
  0x231164  tx1_signal_combined
  0x231166  tx2_signal_dim1
  0x231168  tx2_signal_dim2
  0x23116A  tx2_signal_combined

状态位
  0x231950  cur_static_bits
  0x231954  prev_static_bits
  0x231B28  rpt_static_bits
  0x231994  g_FlagHpp3Feature

压力/NoPress
  0x231964  real_press_flag
  0x231965  no_press_ink_flag
  0x231966  no_press_exit_debounce
  0x231967  no_press_enter_debounce
  0x231968  no_press_exit_thd_dim1
  0x23196A  no_press_enter_thd_dim1
  0x23196C  no_press_exit_thd_dim2
  0x23196E  no_press_enter_thd_dim2
  0x231B18  cur_pressure_out
  0x231C18  prev_pressure_out

学习/倾角
  0x20DE02  prepare_max_signal
  0x20DE04  prepare_sample_count
  0x20DE06  prepare_down_count
  0x212A98  no_press_short_term_max
  0x21786A  no_press_table_max_signal
  0x219EBA  tilt_comp_scale_active
  0x219EBC  tilt_comp_scale_candidate
  0x231A18  no_press_abnormal_flags
```

### 11.3 常用 WinDbg 读值方式

```text
dw <base+231160> L6    ; 读 6 个 u16 signal
db <base+231964> L8    ; 读 real/no-press/debounce 等 u8
dd <base+231950> L2    ; 读 cur_static_bits / prev_static_bits
dw <base+231B18> L1    ; 读当前压力
dw <base+231C18> L1    ; 读上一帧压力
dd <base+231994> L1    ; 读 g_FlagHpp3Feature
dw <base+20DE02> L3    ; 读 prepare_max_signal / sample_count / down_count
dw <base+219EBA> L2    ; 读 tilt_comp_scale_active / candidate
```

### 11.4 场景一：真实压感正常落笔

断点建议：

- `base+0x8668d` `ASA_HPP3TX1GridDataProcesss`
- `base+0x7fd1d` `HPP3_PressureProcess`
- `base+0x88935` `HPP3_ASAStaticStatusPostProcess`
- `base+0x869ef` `ASA_CoorPostProcess`

你应该看到：

- 在 `ASA_HPP3TX1GridDataProcesss` 之后：
  - `tx1_signal_dim1/2/combined` 为非 0
  - 若 TX2 有效，`tx2_signal_*` 也为非 0
  - `GridTx1Valid()` 为真，通常 `GridTx2Valid()` 也为真
- 在 `HPP3_PressureProcess` 返回后：
  - `cur_pressure_out > 0`
  - `real_press_flag = 1`
  - `no_press_ink_flag` 通常仍为 `0`
- 在 `HPP3_ASAStaticStatusPostProcess` 返回后：
  - `cur_static_bits` 至少包含 `0x02 | 0x04`
  - 若当前仍 in-range，通常同时还带 `0x01`
- 在 `ASA_CoorPostProcess` 之后：
  - `rpt_static_bits == cur_static_bits`
  - `ASA_GetRptInk()` 应为真

### 11.5 场景二：NoPressInk 接管落笔

断点建议：

- `base+0x76996` `NoPressInkProcess`
- `base+0x76814` `NoPressInkHandle`
- `base+0x7ffa4` `HPP3_PostPressureProcess`

你应该看到：

- 进入 `NoPressInkProcess` 时：
  - `g_FlagHpp3Feature & 0x40 != 0`
  - 若走 learned 路径，还应满足：
    - `g_FlagHpp3Feature & 0x80 != 0` 时 `g_noPressPara != 0`
- 在 `NoPressInkHandle` 里：
  - `tx2_signal_combined` 非 0 时，`GetNoPressInkTiltCompensation()` 给出补偿量
  - `no_press_enter_thd_* / no_press_exit_thd_*` 被刷新
  - 连续满足进入条件后，`no_press_enter_debounce -> 0`
  - 然后 `no_press_ink_flag = 1`
- 进入 `HPP3_PostPressureProcess` 时常见组合：
  - `real_press_flag = 0`
  - `no_press_ink_flag = 1`
  - `cur_pressure_out` 仍可能是 `0`
- `HPP3_PostPressureProcess` 返回后：
  - `cur_pressure_out = 10`，或
  - `cur_pressure_out = prev_pressure_out`
- 此时最终常见状态：
  - `cur_static_bits` 有 `0x04`
  - 但没有 `0x02`

### 11.6 场景三：快速抬笔 / 快速退出

断点建议：

- `base+0x76814` `NoPressInkHandle`
- `base+0x7ffa4` `HPP3_PostPressureProcess`
- `base+0x6f250` `EdgeCoorProcess`
- `base+0x869ef` `ASA_CoorPostProcess`

你应该看到：

- 在 `ExitToNoPressInk()` 连续命中时：
  - `no_press_exit_debounce` 从 `2 -> 1 -> 0`
  - 到 `0` 那帧会清 `no_press_ink_flag`
- 若没有 fake pressure decrease / 边缘续压：
  - `cur_pressure_out -> 0`
  - `real_press_flag = 0`
  - `no_press_ink_flag = 0`
- 若命中 fake pressure decrease：
  - 条件通常是 `prev_pressure_out > 500 && cur_pressure_out < 11`
  - `cur_pressure_out` 还会维持 1 到 3 帧的拖尾衰减
- 若命中边缘续压：
  - 在 `EdgeCoorProcess` 里 `cur_pressure_out` 会被改回 `prev_pressure_out`
  - 这是“第一帧 release 仍保一帧压力”的关键证据
- 最终真正抬笔时：
  - `cur_static_bits` 不再含 `0x04`
  - `rpt_static_bits` 也不再含 `0x04`
  - 若仍 in-range，则可能只剩 `0x01`

### 11.7 场景四：学习链 prepare -> short-term -> long-term

断点建议：

- `base+0x75198` `NoPressInkLearningPrepareProcess`
- `base+0x75863` `NoPressInkLearningProcess`
- `base+0x765d4` `CheckSignalAbnormalStatus`
- `base+0x75a61` `CalcNoPressInkThd`
- `base+0x75bde` `UpdateLongTermTable`

你应该看到：

- `prepare` 阶段：
  - 只在 `real_press_flag = 1` 时积累
  - `prepare_down_count` 只在“未真实按下 -> 真实按下”切换时增长
  - `prepare_sample_count` 随连续真实按下增长
  - `prepare_max_signal` 单调不减
  - `no_press_abnormal_flags` 理想情况下为 `0`
- `prepare` 成功门槛：
  - `prepare_down_count > 1`
  - `prepare_sample_count > 100`
  - `no_press_abnormal_flags == 0`
  - 满足后 `g_noPressPara = 1`
- `short-term` 阶段：
  - 一次连续真实按下期间运行
  - `no_press_short_term_max` 会不断抬高
  - 满足稳定条件时，当前 cell 的 `short_term_table[cell]` 被更新
- `long-term` 阶段：
  - 在“真实按下 -> 非真实按下”切换时合并
  - `learned_long_term_table[cell]` 被写入或抬高
  - `no_press_table_max_signal` 会刷新

### 11.8 场景五：倾角动态调阈

断点建议：

- `base+0x841e6` `TiltProcess`
- `base+0x76814` `NoPressInkHandle`
- `base+0x75a61` `CalcNoPressInkThd`

你应该看到：

- 在 `TiltProcess` 里：
  - `tx1_coordinate_q10_x/y` 与 `tx2_coordinate_q10_x/y` 都有效
  - `dx / dy` 经平滑后进入 `GetTiltByCoorDif`
  - `DAT_18231b10/12` 是当前帧 raw tilt
  - `DAT_18231b14/16` 是滤波后的输出 tilt
- 在 `GetNoPressInkTiltCompensation` 路径里：
  - `tx2_signal_combined` 越大，补偿越大
  - `tilt_comp_scale_active` 参与最终补偿量计算
- 反馈到阈值时：
  - `no_press_enter_thd_* / no_press_exit_thd_*` 会随 tilt compensation 增大而升高
  - 倾角增大时更不容易误判为“仍可继续无压出墨”

### 11.9 当前文档里最值得实机重点验证的 6 个判断

- `real_press_flag == 0 && no_press_ink_flag == 1` 时，`HPP3_PostPressureProcess` 是否总能补出非 0 压力
- `no_press_exit_debounce` 是否确实是 `2` 帧退出
- `prepare_down_count > 1 && prepare_sample_count > 100` 是否对应 `g_noPressPara` 从 `0 -> 1`
- `tilt_comp_scale_candidate` 是否在足够宽的 TX2 区间上被更新
- `fake pressure decrease` 是否真的只在 `prev_pressure_out > 500 && cur_pressure_out < 11` 时生效
- `EdgeCoorProcess` 是否只在 release 首帧把 `cur_pressure_out` 恢复成 `prev_pressure_out`

---

## 12. 一句话版本

```c
// HPP3 的落笔条件可以概括为：
// 真实压力成立
//     || 无压出墨成立后被补成非 0 压力
//     || 抬笔首帧被边缘续压恢复上一帧压力
//     || 最终输出阶段因频点切换 / exit stylus 复制了上一帧输出
//
// HPP3 的抬笔条件可以概括为：
// 当前压力为 0
//     && 无压出墨未成立
//     && 没有边缘续压
//     && 没有频点切换保帧
//     && 没有 exit stylus 保帧
```
