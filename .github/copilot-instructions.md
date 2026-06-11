# Copilot Instructions

## 项目指南
- 在此工作区中，Visual Studio CMake 使用 clang-cl 时不要只依赖 inheritEnvironments；需要显式避免解析到 x86 版本 clang-cl。

## DVR 分析指南
- 在 EGoTouchRev DVR 分析中，用户所说的“rawdata/原始数据”主要指 DVR 帧中的 heatmap 原始矩阵数据，不应仅按 rawDataLength/raw.hex 独立字节块判断；录制 rawdata 时可能不存在手指解算出的 contacts/peaks。