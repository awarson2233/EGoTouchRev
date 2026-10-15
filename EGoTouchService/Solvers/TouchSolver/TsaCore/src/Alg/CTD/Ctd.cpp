#include "Alg/CTD/Ctd.h"

#include <array>

#include "Alg/Touch/Touch.h"
#include "Alg/Touch/TouchData.h"
#include "App/TsaBuffer.h"
#include "Prmt/Prmt.h"

namespace Solvers::Tsa::Core::Ctd {

namespace {

/// TouchUnit::edgeFlags 低 4 位：触点挨着的阵列边缘。
constexpr uint32_t kEdgeDim1Near = 0x1;
constexpr uint32_t kEdgeDim1Far = 0x2;
constexpr uint32_t kEdgeDim2Near = 0x4;
constexpr uint32_t kEdgeDim2Far = 0x8;
/// TouchUnit::volatileFlags：本帧的坐标被边缘修正改动过。
constexpr uint32_t kVolatileDim1EdgeCorrected = 0x100;
constexpr uint32_t kVolatileDim2EdgeCorrected = 0x200;

/// FlashPrmt::ctdEdge 的下标。
enum EdgeTable : uint8_t { kDim1NearTable, kDim1FarTable, kDim2NearTable, kDim2FarTable };

/// g_ctd256Ln：kLn[i] = ⌊256·ln i⌋，kLn[0] 记 0。
constexpr std::array<uint16_t, 256> kLn{
    0, 0, 177, 281, 354, 412, 458, 498, 532, 562, 589, 613, 636, 656, 675, 693,
    709, 725, 739, 753, 766, 779, 791, 802, 813, 824, 834, 843, 853, 862, 870, 879,
    887, 895, 902, 910, 917, 924, 931, 937, 944, 950, 956, 962, 968, 974, 980, 985,
    991, 996, 1001, 1006, 1011, 1016, 1021, 1025, 1030, 1035, 1039, 1043, 1048, 1052, 1056, 1060,
    1064, 1068, 1072, 1076, 1080, 1083, 1087, 1091, 1094, 1098, 1101, 1105, 1108, 1112, 1115, 1118,
    1121, 1124, 1128, 1131, 1134, 1137, 1140, 1143, 1146, 1149, 1151, 1154, 1157, 1160, 1163, 1165,
    1168, 1171, 1173, 1176, 1178, 1181, 1183, 1186, 1188, 1191, 1193, 1196, 1198, 1200, 1203, 1205,
    1207, 1210, 1212, 1214, 1216, 1219, 1221, 1223, 1225, 1227, 1229, 1231, 1233, 1236, 1238, 1240,
    1242, 1244, 1246, 1248, 1249, 1251, 1253, 1255, 1257, 1259, 1261, 1263, 1265, 1266, 1268, 1270,
    1272, 1274, 1275, 1277, 1279, 1281, 1282, 1284, 1286, 1287, 1289, 1291, 1292, 1294, 1296, 1297,
    1299, 1300, 1302, 1304, 1305, 1307, 1308, 1310, 1311, 1313, 1314, 1316, 1317, 1319, 1320, 1322,
    1323, 1325, 1326, 1327, 1329, 1330, 1332, 1333, 1335, 1336, 1337, 1339, 1340, 1341, 1343, 1344,
    1345, 1347, 1348, 1349, 1351, 1352, 1353, 1355, 1356, 1357, 1358, 1360, 1361, 1362, 1363, 1365,
    1366, 1367, 1368, 1370, 1371, 1372, 1373, 1374, 1376, 1377, 1378, 1379, 1380, 1381, 1383, 1384,
    1385, 1386, 1387, 1388, 1389, 1391, 1392, 1393, 1394, 1395, 1396, 1397, 1398, 1399, 1400, 1401,
    1403, 1404, 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413, 1414, 1415, 1416, 1417, 1418,
};

// 原代码不检查段数：segmentNum 超过 4 时按同样的步长读到分段表后面的 RamPrmt 字节。
const CtdEdgeSegment& SegmentAt(const CtdEdgePrmt& table, uint8_t idx) {
    return reinterpret_cast<const CtdEdgeSegment*>(reinterpret_cast<const uint8_t*>(&table) + offsetof(CtdEdgePrmt, segment))[idx];
}

// CTD_ECGetOffset @ 0x6ba553a6
// 按触点尺寸在当前分段表（RamPrmt::ctdEdge）里选段，返回 ratio 在该段 ln 区间里的位置 ×256，封顶 255。
int16_t GetOffset(uint8_t ratio, uint8_t sizeMm) {
    const CtdEdgePrmt& table = PrmtRam().ctdEdge;
    uint8_t seg = 0;
    while (seg < table.segmentNum - 1 && SegmentAt(table, seg).maxSizeMm < sizeMm) ++seg;
    const auto& ln = kLn;
    const uint32_t low = ln[SegmentAt(table, seg).lnLowIdx];
    const uint32_t span = ln[SegmentAt(table, seg).lnHighIdx] - low;
    int32_t offset = 0;
    if (span != 0) offset = static_cast<int32_t>((ln[ratio] - low) * 0x100) / static_cast<int32_t>(span);
    if (offset > 0xff) offset = 0xff;
    return static_cast<int16_t>(offset);
}

// CTD_ECGetFinalOffset @ 0x6ba554f7
// 质心到边缘的距离 dist（Q8.8）不超过一格时取修正值；一格到一格加 1/4 之间向原距离线性过渡；更远不修正。
int32_t GetFinalOffset(int32_t dist, int32_t corrected) {
    const int32_t beyond = dist - 0x100;
    if (beyond < 1) return corrected;
    if (beyond < 0x40) return (dist * beyond * 4 + (0x100 - beyond * 4) * corrected) / 256;
    return dist;
}

/// 一个方向上的边缘修正：先看末端再看首端。修正时把机型参数里对应的分段表复制到 RamPrmt 供 GetOffset 使用。
void CorrectAxis(TouchUnit& unit, uint16_t rawPos, uint16_t& pos, uint8_t ratio, uint32_t nearFlag, uint32_t farFlag,
                 EdgeTable nearTable, EdgeTable farTable, uint16_t nearEdge, uint16_t farEdge, uint32_t correctedFlag) {
    const FlashPrmt& flash = PrmtFlash();
    unit.volatileFlags &= ~correctedFlag;
    int32_t dist;
    int32_t final;
    if ((unit.edgeFlags & farFlag) != 0) {
        PrmtRam().ctdEdge = flash.ctdEdge[farTable];
        const int16_t offset = GetOffset(ratio, unit.sizeMm);
        dist = static_cast<int32_t>(static_cast<uint32_t>(farEdge) - rawPos);
        final = GetFinalOffset(dist, 0x100 - offset);
        pos = static_cast<uint16_t>(farEdge - static_cast<int16_t>(final));
    } else if ((unit.edgeFlags & nearFlag) != 0) {
        PrmtRam().ctdEdge = flash.ctdEdge[nearTable];
        const int16_t offset = GetOffset(ratio, unit.sizeMm);
        dist = static_cast<int32_t>(static_cast<uint32_t>(rawPos) - nearEdge);
        final = GetFinalOffset(dist, 0x100 - offset);
        pos = static_cast<uint16_t>(static_cast<int16_t>(final) + nearEdge);
    } else {
        return;
    }
    if (final != dist) unit.volatileFlags |= correctedFlag;
}

// CTD_ECProcessUnit @ 0x6ba55c6b（CTD_ECUpdatePosToEC 只是 Touch_SetEdgeDist）
// 两个方向的修正比例分别取 TouchUnit::gripRatioDim1 / gripRatioDim2（TZ 写入）在 ln 表里的值。
void EcProcessUnit(uint8_t idx) {
    Touch::SetEdgeDist(idx);
    TouchUnit& unit = Touches().unit[idx];
    const FlashConstPrmt& c = PrmtFlashConst();
    CorrectAxis(unit, unit.rawPos.dim1, unit.pos.dim1, unit.gripRatioDim1, kEdgeDim1Near, kEdgeDim1Far, kDim1NearTable,
                kDim1FarTable, c.dim1NearEdge, c.dim1FarEdge, kVolatileDim1EdgeCorrected);
    CorrectAxis(unit, unit.rawPos.dim2, unit.pos.dim2, unit.gripRatioDim2, kEdgeDim2Near, kEdgeDim2Far, kDim2NearTable,
                kDim2FarTable, c.dim2NearEdge, c.dim2FarEdge, kVolatileDim2EdgeCorrected);
}

}  // namespace

// CTD_ECProcess @ 0x6ba55ca0
void EcProcess() {
    for (uint8_t i = 0; i < Touches().count; ++i) {
        TouchUnit& unit = Touches().unit[i];
        unit.pos = unit.rawPos;
        EcProcessUnit(i);
        unit.rptPos = unit.pos;
        Touch::SetEdgeDist(i);
    }
}

// CTD_General @ 0x6ba5523c
// 窗口按 int 遍历，越界判断按 uint8 截断：负的行列号截断后超出阵列被跳过，超过 255 的会回绕到阵列内并按原值加权。
void General(uint8_t dim1, uint8_t dim2, uint8_t dim1Span, uint8_t dim2Span, uint16_t* centerDim1, uint16_t* centerDim2) {
    const FlashPrmt& flash = PrmtFlash();
    const int16_t* dif = Buffers().dif;
    int32_t dim1Sum = 0;
    int32_t dim2Sum = 0;
    int32_t weight = 0;
    for (int d1 = dim1 - (dim1Span >> 1); d1 <= (dim1Span >> 1) + dim1; ++d1) {
        if (static_cast<uint8_t>(d1) >= flash.dim1Len) continue;
        for (int d2 = dim2 - (dim2Span >> 1); d2 <= (dim2Span >> 1) + dim2; ++d2) {
            if (static_cast<uint8_t>(d2) >= flash.dim2Len) continue;
            const int16_t sig = dif[static_cast<uint16_t>(flash.dim1Len * d2 + d1)];
            if (sig <= 0) continue;
            dim1Sum += sig * d1;
            dim2Sum += sig * d2;
            weight += sig;
        }
    }
    *centerDim1 = static_cast<uint16_t>(BasicCalculation(dim1Sum, weight));
    *centerDim2 = static_cast<uint16_t>(BasicCalculation(dim2Sum, weight));
}

// CTD_BasicCalculation @ 0x6ba551e0
int64_t BasicCalculation(int32_t weightedSum, int32_t weight) {
    if (weight == 0) return 0;
    const int64_t center = (static_cast<int64_t>(weightedSum) << 8) / weight + 0x80;
    return center < 0 ? 0 : center;
}

}  // namespace Solvers::Tsa::Core::Ctd
