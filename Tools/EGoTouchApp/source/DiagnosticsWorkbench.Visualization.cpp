#include "DiagnosticsWorkbench.h"
#include "imgui.h"
#include "StylusSolver/AsaTypes.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <vector>

namespace App {

void DiagnosticsWorkbench::DrawHeatmap() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

    if (m_fullscreen) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        if (platform_io.Monitors.Size > 0) {
            ImGui::SetNextWindowSize(platform_io.Monitors[0].MainSize);
        } else {
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        }

        window_flags |= ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
    } else {
        ImGui::SetNextWindowPos(ImVec2(550, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600, 800), ImGuiCond_FirstUseEver);
    }

    ImGui::Begin("Heatmap", nullptr, window_flags);

    if (ImGui::BeginTabBar("HeatmapTabs")) {
        if (ImGui::BeginTabItem("Master")) {
            ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
            ImVec2 canvas_p = ImGui::GetCursorScreenPos();

            const int rows = 40;
            const int cols = 60;

            float cell_w, cell_h;
            if (m_fullscreen || canvas_sz.x > 800) {
                cell_w = canvas_sz.x / cols;
                cell_h = canvas_sz.y / rows;
            } else {
                cell_w = (float)m_heatmapScale;
                cell_h = (float)m_heatmapScale;
            }

            if (!m_fullscreen) {
                ImGui::Text("Service Timestamp (raw): %llu | App Receive Epoch Us: %llu | Cell Size: %.1fx%.1f",
                            (unsigned long long)m_currentFrame.timestamp,
                            (unsigned long long)m_currentFrame.receiveSystemEpochUs,
                            cell_w,
                            cell_h);
            }

            ImDrawList* draw_list = ImGui::GetWindowDrawList();

            for (int y = 0; y < rows; ++y) {
                for (int x = 0; x < cols; ++x) {
                    int16_t val = m_currentFrame.heatmapMatrix[rows - 1 - y][cols - 1 - x];
                    float normalized = std::clamp(val / m_colorRange, -1.0f, 1.0f);

                    ImVec4 colorVec;
                    if (normalized == 0.0f) {
                        colorVec = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                    } else if (normalized > 0.0f) {
                        float v = normalized * 4.0f;
                        float r = std::clamp(std::min(v - 1.5f, -v + 4.5f), 0.0f, 1.0f);
                        float g = std::clamp(std::min(v - 0.5f, -v + 3.5f), 0.0f, 1.0f);
                        float b = std::clamp(std::min(v + 0.5f, -v + 2.5f), 0.0f, 1.0f);
                        colorVec = ImVec4(r, g, b, 1.0f);
                    } else {
                        float v = (-normalized) * 2.0f;
                        float r = std::clamp(v * 0.5f, 0.0f, 0.5f);
                        float g = std::clamp(v - 1.0f, 0.0f, 1.0f);
                        float b = std::clamp(v, 0.0f, 1.0f);
                        colorVec = ImVec4(r, g, b, 1.0f);
                    }

                    ImU32 color = ImGui::ColorConvertFloat4ToU32(colorVec);
                    ImVec2 p_min = ImVec2(canvas_p.x + x * cell_w, canvas_p.y + y * cell_h);
                    ImVec2 p_max = ImVec2(p_min.x + cell_w, p_min.y + cell_h);
                    draw_list->AddRectFilled(p_min, p_max, color);
                    draw_list->AddRect(p_min, p_max, IM_COL32(50, 50, 50, m_fullscreen ? 50 : 255));
                }
            }

            {
                const auto& peaks = m_currentFrame.peaks;
                const auto& zones = m_currentFrame.touchZones;

                int currentPeakCount = peaks.size();
                int currentContactCount = static_cast<int>(m_currentFrame.contacts.size());
                if (m_autoCaptureMode == 1) {
                    if (m_autoExportTargetPeaks > 0 &&
                        currentPeakCount == m_autoExportTargetPeaks &&
                        m_lastPeakCount != m_autoExportTargetPeaks) {
                        ExportCurrentFrameCsv(true);
                    }
                } else if (m_autoCaptureMode == 2) {
                    if (currentContactCount == 0 && m_lastContactCount > 0) {
                        ExportCurrentFrameCsv(true);
                    }
                }
                m_lastPeakCount = currentPeakCount;
                m_lastContactCount = currentContactCount;

                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        uint8_t zoneId = zones[r * cols + c];
                        if (zoneId > 0) {
                            int mirrored_r = rows - 1 - r;
                            int mirrored_c = cols - 1 - c;
                            ImVec2 p_min = ImVec2(canvas_p.x + mirrored_c * cell_w, canvas_p.y + mirrored_r * cell_h);
                            ImVec2 p_max = ImVec2(p_min.x + cell_w, p_min.y + cell_h);

                            ImVec4 outlineCol;
                            switch (zoneId % 6) {
                                case 1: outlineCol = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                                case 2: outlineCol = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                                case 3: outlineCol = ImVec4(0.0f, 0.5f, 1.0f, 1.0f); break;
                                case 4: outlineCol = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); break;
                                case 5: outlineCol = ImVec4(0.0f, 1.0f, 1.0f, 1.0f); break;
                                case 0: outlineCol = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); break;
                            }

                            ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(outlineCol);
                            draw_list->AddRectFilled(p_min, p_max, ImGui::ColorConvertFloat4ToU32(ImVec4(outlineCol.x, outlineCol.y, outlineCol.z, 0.2f)));

                            bool v_diff_top = (r == rows - 1) || (zones[(r + 1) * cols + c] != zoneId);
                            bool v_diff_bottom = (r == 0) || (zones[(r - 1) * cols + c] != zoneId);
                            bool v_diff_left = (c == cols - 1) || (zones[r * cols + (c + 1)] != zoneId);
                            bool v_diff_right = (c == 0) || (zones[r * cols + (c - 1)] != zoneId);

                            float border_thickness = 2.0f;
                            if (v_diff_top) draw_list->AddLine(ImVec2(p_min.x, p_min.y), ImVec2(p_max.x, p_min.y), colU32, border_thickness);
                            if (v_diff_bottom) draw_list->AddLine(ImVec2(p_min.x, p_max.y), ImVec2(p_max.x, p_max.y), colU32, border_thickness);
                            if (v_diff_left) draw_list->AddLine(ImVec2(p_min.x, p_min.y), ImVec2(p_min.x, p_max.y), colU32, border_thickness);
                            if (v_diff_right) draw_list->AddLine(ImVec2(p_max.x, p_min.y), ImVec2(p_max.x, p_max.y), colU32, border_thickness);
                        }
                    }
                }

                const auto& pzones = m_currentFrame.peakZones;
                for (int r = 0; r < rows; ++r) {
                    for (int c = 0; c < cols; ++c) {
                        int idx = r * cols + c;
                        int mr = rows - 1 - r;
                        int mc = cols - 1 - c;
                        ImVec2 p0(canvas_p.x + mc * cell_w, canvas_p.y + mr * cell_h);
                        ImVec2 p1(p0.x + cell_w, p0.y + cell_h);

                        if (pzones[idx] > 0) {
                            uint8_t zid = pzones[idx];
                            auto nz = [&](int ri, int ci) -> uint8_t {
                                if (ri < 0 || ri >= rows || ci < 0 || ci >= cols) return 0;
                                return pzones[ri * cols + ci];
                            };
                            bool edge = (nz(r + 1, c) != zid) || (nz(r - 1, c) != zid) || (nz(r, c + 1) != zid) || (nz(r, c - 1) != zid);
                            if (edge) {
                                ImU32 zCol = IM_COL32(255, 255, 255, 180);
                                float t = 1.0f;
                                if (nz(r + 1, c) != zid) draw_list->AddLine(p0, {p1.x, p0.y}, zCol, t);
                                if (nz(r - 1, c) != zid) draw_list->AddLine({p0.x, p1.y}, p1, zCol, t);
                                if (nz(r, c + 1) != zid) draw_list->AddLine(p0, {p0.x, p1.y}, zCol, t);
                                if (nz(r, c - 1) != zid) draw_list->AddLine({p1.x, p0.y}, p1, zCol, t);
                            }
                        }
                    }
                }

                for (const auto& peak : peaks) {
                    int mirrored_r = rows - 1 - peak.r;
                    int mirrored_c = cols - 1 - peak.c;
                    float cx = canvas_p.x + mirrored_c * cell_w + cell_w * 0.5f;
                    float cy = canvas_p.y + mirrored_r * cell_h + cell_h * 0.5f;

                    ImU32 markerColor = IM_COL32(255, 255, 0, 255);
                    float crossSize = std::min(cell_w, cell_h) * 0.6f;
                    draw_list->AddLine(ImVec2(cx - crossSize, cy), ImVec2(cx + crossSize, cy), markerColor, 2.0f);
                    draw_list->AddLine(ImVec2(cx, cy - crossSize), ImVec2(cx, cy + crossSize), markerColor, 2.0f);

                    char label[32];
                    snprintf(label, sizeof(label), "%d", peak.z);
                    ImU32 textColor = IM_COL32(255, 255, 255, 255);
                    ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
                    ImVec2 textPos(cx + 4.0f, cy + 4.0f);
                    draw_list->AddText(ImVec2(textPos.x - 1, textPos.y - 1), outlineColor, label);
                    draw_list->AddText(ImVec2(textPos.x + 1, textPos.y - 1), outlineColor, label);
                    draw_list->AddText(ImVec2(textPos.x - 1, textPos.y + 1), outlineColor, label);
                    draw_list->AddText(ImVec2(textPos.x + 1, textPos.y + 1), outlineColor, label);
                    draw_list->AddText(textPos, textColor, label);
                }

                for (const auto& contact : m_currentFrame.contacts) {
                    float mirrored_r = (rows - 1.0f) - contact.y;
                    float mirrored_c = (cols - 1.0f) - contact.x;
                    float cx = canvas_p.x + mirrored_c * cell_w + cell_w * 0.5f;
                    float cy = canvas_p.y + mirrored_r * cell_h + cell_h * 0.5f;

                    ImU32 markerColor = IM_COL32(0, 255, 0, 255);
                    float crossSize = std::min(cell_w, cell_h) * 0.5f;
                    draw_list->AddLine(ImVec2(cx - crossSize, cy - crossSize), ImVec2(cx + crossSize, cy + crossSize), markerColor, 2.5f);
                    draw_list->AddLine(ImVec2(cx - crossSize, cy + crossSize), ImVec2(cx + crossSize, cy - crossSize), markerColor, 2.5f);

                    char label[32];
                    snprintf(label, sizeof(label), "ID:%d", contact.id);
                    ImU32 textColor = IM_COL32(0, 255, 0, 255);
                    ImU32 outlineColor = IM_COL32(0, 0, 0, 255);
                    ImVec2 textPos(cx + 6.0f, cy - 12.0f);
                    draw_list->AddText(ImVec2(textPos.x - 1, textPos.y - 1), outlineColor, label);
                    draw_list->AddText(ImVec2(textPos.x + 1, textPos.y - 1), outlineColor, label);
                    draw_list->AddText(ImVec2(textPos.x - 1, textPos.y + 1), outlineColor, label);
                    draw_list->AddText(ImVec2(textPos.x + 1, textPos.y + 1), outlineColor, label);
                    draw_list->AddText(textPos, textColor, label);
                }
            }

            if (!m_fullscreen) {
                ImGui::Dummy(ImVec2(cols * cell_w, rows * cell_h));
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("TX1 / TX2")) {
            DrawSlaveHeatmap();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DiagnosticsWorkbench::DrawSlaveHeatmap() {
    const auto& stylus = m_currentFrame.stylus;
    const auto& point = stylus.output.point;
    const auto& input = stylus.input;
    const auto& interop = stylus.interop;
#if EGOTOUCH_DIAG
    const auto& diag = stylus.debug.coord;
#endif

    if (m_currentFrame.slaveSuffixValid) {
        Asa::AsaGridData gridData = Asa::ExtractGridFromSlaveWords(m_currentFrame.slaveSuffix.words, Frame::kSlaveSuffixWords);

        ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
        float draw_width = std::max(200.0f, canvas_sz.x * 0.42f);
        float cell_w = draw_width / 9.0f;
        float cell_h = cell_w;
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        auto draw_grid = [&](const Asa::FreqBlock& grid, float start_x, float start_y, const char* label, float ptX, float ptY) {
            ImGui::SetCursorScreenPos(ImVec2(start_x, start_y - 25.0f));
            ImGui::Text("%s (Anchor: R%d C%d)", label, grid.anchorRow, grid.anchorCol);

            ImVec2 canvas_p(start_x, start_y);
            draw_list->AddText(ImVec2(canvas_p.x + 9 * cell_w + 5.0f, canvas_p.y - 18.0f), IM_COL32(255, 255, 255, 255), "X");
            draw_list->AddText(ImVec2(canvas_p.x - 18.0f, canvas_p.y + 9 * cell_h + 5.0f), IM_COL32(255, 255, 255, 255), "Y");

            for (int i = 0; i < 9; ++i) {
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", 8 - i);
                draw_list->AddText(ImVec2(canvas_p.x + i * cell_w + cell_w * 0.5f - 4, canvas_p.y - 18.0f), IM_COL32(200, 200, 200, 255), buf);
                draw_list->AddText(ImVec2(canvas_p.x - 18.0f, canvas_p.y + i * cell_h + cell_h * 0.5f - 6), IM_COL32(200, 200, 200, 255), buf);
            }

            for (int r = 0; r < 9; ++r) {
                for (int c = 0; c < 9; ++c) {
                    int mirrored_r = 8 - r;
                    int mirrored_c = 8 - c;
                    int16_t val = grid.grid[mirrored_r][mirrored_c];
                    float normalized = std::clamp((float)val / m_colorRange, 0.0f, 1.0f);

                    ImVec4 colorVec;
                    if (normalized == 0.0f) {
                        colorVec = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                    } else {
                        float v = normalized * 4.0f;
                        float red = std::clamp(std::min(v - 1.5f, -v + 4.5f), 0.0f, 1.0f);
                        float green = std::clamp(std::min(v - 0.5f, -v + 3.5f), 0.0f, 1.0f);
                        float blue = std::clamp(std::min(v + 0.5f, -v + 2.5f), 0.0f, 1.0f);
                        colorVec = ImVec4(red, green, blue, 1.0f);
                    }

                    ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(colorVec);
                    ImVec2 p_min(canvas_p.x + c * cell_w, canvas_p.y + r * cell_h);
                    ImVec2 p_max(p_min.x + cell_w, p_min.y + cell_h);
                    draw_list->AddRectFilled(p_min, p_max, colU32);
                    draw_list->AddRect(p_min, p_max, IM_COL32(50, 50, 50, 255));

                    if (cell_w > 20.0f) {
                        char vbuf[16];
                        snprintf(vbuf, sizeof(vbuf), "%d", val);
                        ImVec2 tsz = ImGui::CalcTextSize(vbuf);
                        draw_list->AddText(ImVec2(p_min.x + (cell_w - tsz.x) * 0.5f, p_min.y + (cell_h - tsz.y) * 0.5f), IM_COL32(255, 255, 255, 255), vbuf);
                    }
                }
            }

            float markerX = ptX;
            float markerY = ptY;
            if (markerX < 0.0f || markerY < 0.0f) {
                int peakRow = -1;
                int peakCol = -1;
                int16_t peakValue = 0;
                for (int r = 0; r < 9; ++r) {
                    for (int c = 0; c < 9; ++c) {
                        if (grid.grid[r][c] > peakValue) {
                            peakValue = grid.grid[r][c];
                            peakRow = r;
                            peakCol = c;
                        }
                    }
                }
                if (peakRow >= 0 && peakCol >= 0) {
                    markerX = static_cast<float>(peakCol) + 0.5f;
                    markerY = static_cast<float>(peakRow) + 0.5f;
                }
            }

            if (markerX >= 0.0f && markerY >= 0.0f) {
                float cx = canvas_p.x + (9.0f - markerX) * cell_w;
                float cy = canvas_p.y + (9.0f - markerY) * cell_h;
                ImU32 markerColor = IM_COL32(0, 255, 0, 255);
                float crossSize = cell_w * 0.4f;
                draw_list->AddLine(ImVec2(cx - crossSize, cy - crossSize), ImVec2(cx + crossSize, cy + crossSize), markerColor, 2.5f);
                draw_list->AddLine(ImVec2(cx - crossSize, cy + crossSize), ImVec2(cx + crossSize, cy - crossSize), markerColor, 2.5f);

                char coordBuf[32];
                snprintf(coordBuf, sizeof(coordBuf), "(%.2f, %.2f)", markerX, markerY);
                ImVec2 textPos(cx + crossSize + 2.0f, cy - crossSize - 12.0f);
                draw_list->AddText(ImVec2(textPos.x - 1, textPos.y - 1), IM_COL32(0, 0, 0, 255), coordBuf);
                draw_list->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 255), coordBuf);
                draw_list->AddText(textPos, IM_COL32(0, 255, 0, 255), coordBuf);
            }
        };

        ImVec2 base_cp = ImGui::GetCursorScreenPos();
        base_cp.x += 20.0f;
        base_cp.y += 10.0f;

        const float tx1LocalX = diag.localCoorDim1 > 0 ? static_cast<float>(diag.localCoorDim1) / Asa::kCoorUnit : -1.0f;
        const float tx1LocalY = diag.localCoorDim2 > 0 ? static_cast<float>(diag.localCoorDim2) / Asa::kCoorUnit : -1.0f;

        if (gridData.tx1.valid) {
            draw_grid(gridData.tx1, base_cp.x, base_cp.y + 35.0f, "TX1 Grid", tx1LocalX, tx1LocalY);
        } else {
            ImGui::SetCursorScreenPos(base_cp);
            ImGui::Text("TX1 Grid Invalid");
        }

        if (gridData.tx2.valid) {
            draw_grid(gridData.tx2, base_cp.x + draw_width + 50.0f, base_cp.y + 35.0f, "TX2 Grid", -1.0f, -1.0f);
        } else {
            ImGui::SetCursorScreenPos(ImVec2(base_cp.x + draw_width + 50.0f, base_cp.y));
            ImGui::Text("TX2 Grid Invalid");
        }

        ImGui::Dummy(ImVec2(draw_width * 2 + 50.0f, cell_h * 9 + 60.0f));
    } else {
        ImGui::TextUnformatted("Slave suffix data is unavailable for TX1/TX2 rendering.");
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "TX1/TX2 Diagnostics");

    if (ImGui::BeginTable("StylusTxDiagnostics", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame)) {
        ImGui::TableNextColumn();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), "Signal / Coordinate");
        ImGui::Text("Point: %s", point.valid ? "Valid" : "Invalid");
        ImGui::Text("XY: %.3f, %.3f", point.x, point.y);
        ImGui::Text("Confidence: %.3f", point.confidence);
        ImGui::Text("TX1: %.3f, %.3f", point.tx1X, point.tx1Y);
        ImGui::Text("TX2: %.3f, %.3f", point.tx2X, point.tx2Y);
        ImGui::Text("Peak TX1/TX2 (Raw): %u / %u", interop.signalX, interop.signalY);
        ImGui::Text("Signal TX1/TX2 (Composite): %u / %u", point.peakTx1, point.peakTx2);
        ImGui::Text("Max Raw Peak: %u", interop.maxRawPeak);
        if (m_currentFrame.masterSuffixValid) {
            ImGui::Text("Master Noise F0/F1: %u / %u", m_currentFrame.masterSuffix.penF0NoiseCount(), m_currentFrame.masterSuffix.penF1NoiseCount());
            ImGui::Text("Master TP Freq1/Timestamp: %u / %u", m_currentFrame.masterSuffix.tpFreq1(), m_currentFrame.masterSuffix.timestamp());
            ImGui::Text("Master FreqShiftDone: %u", m_currentFrame.masterSuffix.freqShiftDone());
        }

        ImGui::TableNextColumn();
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.8f, 1.0f), "Status / Output");
        ImGui::Text("Slave Valid: %s", input.slaveValid ? "Y" : "N");
        ImGui::Text("Checksum: %s (0x%04X)", input.checksumOk ? "OK" : "FAIL", input.checksum16);
        ImGui::Text("TX1/TX2 Block: %s / %s", input.tx1BlockValid ? "Y" : "N", input.tx2BlockValid ? "Y" : "N");
        ImGui::Text("Pressure: %u (Raw:%u Mapped:%u)", stylus.output.pressure, point.rawPressure, point.mappedPressure);
#if EGOTOUCH_DIAG
        ImGui::Text("BT Seq: %u  PredAge: %u  Real: %s",
                    static_cast<unsigned int>(diag.btSeq),
                    static_cast<unsigned int>(diag.predictedAgeFrames),
                    diag.pressureIsReal ? "Y" : "N");
#endif
        ImGui::Text("Status: 0x%08X", static_cast<unsigned int>(input.status));
        ImGui::Text("Pipeline Stage: %u  Pressure Source: %s",
                    static_cast<unsigned int>(stylus.output.pipelineStage),
                    diag.pressureIsReal ? "Real(BT)" : "Predicted/Synth");
        ImGui::Text("Recheck: %s / %s / %s", interop.recheckEnabled ? "Enabled" : "Disabled", interop.recheckPassed ? "Pass" : "Fail", interop.recheckOverlap ? "Overlap" : "NoOverlap");
        ImGui::Text("Recheck Threshold: %u", interop.recheckThreshold);
        ImGui::Text("Touch Suppress: %s  NullLike: %s  Remain: %u", interop.touchSuppressActive ? "Y" : "N", interop.touchNullLike ? "Y" : "N", static_cast<unsigned int>(interop.touchSuppressFrames));
        ImGui::EndTable();
    }
}

void DiagnosticsWorkbench::DrawCoordinateTable() {

    if (!m_currentFrame.contacts.empty()) {
        // Keep a stable UI order by touch ID, so rows do not jump when X/Y changes.
        std::vector<const Solvers::TouchContact*> orderedContacts;
        orderedContacts.reserve(m_currentFrame.contacts.size());
        for (const auto& c : m_currentFrame.contacts) {
            orderedContacts.push_back(&c);
        }
        std::stable_sort(orderedContacts.begin(), orderedContacts.end(),
                         [](const Solvers::TouchContact* a, const Solvers::TouchContact* b) {
                             return a->id < b->id;
                         });

        if (ImGui::BeginTable("ContactsTable", 14, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40.0f);
            ImGui::TableSetupColumn("X (Sub-pixel)", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Y (Sub-pixel)", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("ScrX", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("ScrY", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("Area", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("SigSum", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Size(mm)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Reported", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("RptEvt", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn("LifeFlg", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("RptFlg", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Dbg", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableHeadersRow();

            for (const auto* contactPtr : orderedContacts) {
                const auto& contact = *contactPtr;
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", contact.id);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.4f", contact.x);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.4f", contact.y);

                // Compute VHF screen coordinates (same formula as BuildTouchVhfReports)
                auto toVhfVal = [](float gridVal, float gridMax, float logMax, bool invert) -> int {
                    float norm = std::clamp(gridVal / gridMax, 0.0f, 1.0f);
                    int vhf = std::clamp(static_cast<int>(std::lround(norm * logMax)), 0, static_cast<int>(logMax));
                    return invert ? (static_cast<int>(logMax) - vhf) : vhf;
                };
                const int scrX = toVhfVal(contact.x, 60.0f, 16000.0f, true);   // X Logical Max per descriptor
                const int scrY = toVhfVal(contact.y, 40.0f, 25600.0f, false);  // Y Logical Max per descriptor

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", scrX);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%d", scrY);

                ImGui::TableSetColumnIndex(5);
                const char* stateStr = "UNK";
                if (contact.state == 0) stateStr = "Down";
                else if (contact.state == 1) stateStr = "Move";
                else if (contact.state == 2) stateStr = "Up";
                ImGui::Text("%s", stateStr);

                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%d", contact.area);

                ImGui::TableSetColumnIndex(7);
                ImGui::Text("%d", contact.signalSum);

                ImGui::TableSetColumnIndex(8);
                ImGui::Text("%.2f", contact.sizeMm);

                ImGui::TableSetColumnIndex(9);
                ImGui::Text("%s", contact.isReported ? "Y" : "N");

                ImGui::TableSetColumnIndex(10);
                const char* reportEventStr = "UNK";
                if (contact.reportEvent == Solvers::TouchReportIdle) reportEventStr = "Idle";
                else if (contact.reportEvent == Solvers::TouchReportDown) reportEventStr = "Down";
                else if (contact.reportEvent == Solvers::TouchReportMove) reportEventStr = "Move";
                else if (contact.reportEvent == Solvers::TouchReportUp) reportEventStr = "Up";
                ImGui::Text("%s", reportEventStr);

                ImGui::TableSetColumnIndex(11);
                ImGui::Text("0x%X", static_cast<unsigned int>(contact.lifeFlags));

                ImGui::TableSetColumnIndex(12);
                ImGui::Text("0x%X", static_cast<unsigned int>(contact.reportFlags));

                ImGui::TableSetColumnIndex(13);
                ImGui::Text("0x%X", contact.debugFlags);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::TextDisabled("No contacts in current frame.");
    }

    ImGui::Separator();
    auto drawTouchPacket = [&](const char* label, const Solvers::TouchPacket& packet) {
        ImGui::Text("%s: %s (RID=0x%02X Len=%u)", label, packet.valid ? "Valid" : "Invalid",
                    packet.reportId, packet.length);
        if (!packet.valid) {
            return;
        }
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < packet.bytes.size(); ++i) {
            oss << std::setw(2) << static_cast<unsigned int>(packet.bytes[i]);
            if (i + 1 < packet.bytes.size()) {
                oss << " ";
            }
        }
        ImGui::TextUnformatted(oss.str().c_str());
    };
    drawTouchPacket("TouchPacket[0]", m_currentFrame.touchPackets[0]);
    drawTouchPacket("TouchPacket[1]", m_currentFrame.touchPackets[1]);
}

void DiagnosticsWorkbench::DrawStylusPanel() {
    ImGui::SetNextWindowPos(ImVec2(1180, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(520, 540), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stylus Debug (ASA/HPP2/HPP3-lite)");

    const auto& stylus = m_currentFrame.stylus;
    const auto& input = stylus.input;
    const auto& output = stylus.output;
    const auto& point = output.point;
    const auto& interop = stylus.interop;
#if EGOTOUCH_DIAG
    const auto& diag = stylus.debug.coord;
#endif
    ImGui::Text("Slave Valid: %s", input.slaveValid ? "Y" : "N");
    ImGui::Text("Slave Offset: %u  Checksum16: 0x%04X (%s)",
                static_cast<unsigned int>(input.slaveWordOffset),
                static_cast<unsigned int>(input.checksum16),
                input.checksumOk ? "OK" : "FAIL");
    ImGui::Text("TX1 Block: %s | TX2 Block: %s",
                input.tx1BlockValid ? "Y" : "N",
                input.tx2BlockValid ? "Y" : "N");
    ImGui::Text("MasterSuffix: %s  F0Noise=%u F1Noise=%u",
                m_currentFrame.masterSuffixValid ? "Y" : "N",
                m_currentFrame.masterSuffixValid ? static_cast<unsigned int>(m_currentFrame.masterSuffix.penF0NoiseCount()) : 0u,
                m_currentFrame.masterSuffixValid ? static_cast<unsigned int>(m_currentFrame.masterSuffix.penF1NoiseCount()) : 0u);
    ImGui::Text("Status: 0x%08X", static_cast<unsigned int>(input.status));
    ImGui::Text("Recheck: En=%s Pass=%s Overlap=%s Th=%u",
                interop.recheckEnabled ? "Y" : "N",
                interop.recheckPassed ? "Y" : "N",
                interop.recheckOverlap ? "Y" : "N",
                static_cast<unsigned int>(interop.recheckThreshold));
    ImGui::Text("TouchSuppress: Active=%s NullLike=%s Remain=%u",
                interop.touchSuppressActive ? "Y" : "N",
                interop.touchNullLike ? "Y" : "N",
                static_cast<unsigned int>(interop.touchSuppressFrames));
    ImGui::Text("Pressure: %u (Raw:%u Mapped:%u)",
                stylus.output.pressure,
                static_cast<unsigned int>(point.rawPressure),
                static_cast<unsigned int>(point.mappedPressure));
#if EGOTOUCH_DIAG
    ImGui::Text("BT Seq: %u  PredAge: %u  Real: %s",
                static_cast<unsigned int>(diag.btSeq),
                static_cast<unsigned int>(diag.predictedAgeFrames),
                diag.pressureIsReal ? "Y" : "N");
#endif
    ImGui::Text("Peak TX1/TX2 (Raw): %u / %u  MaxPeak: %u",
                static_cast<unsigned int>(interop.signalX),
                static_cast<unsigned int>(interop.signalY),
                static_cast<unsigned int>(interop.maxRawPeak));
    ImGui::Text("Pressure Source: %s  PredAge=%u",
                diag.pressureIsReal ? "Real(BT)" : "Predicted/Synth",
                static_cast<unsigned int>(diag.predictedAgeFrames));

    if (point.valid) {
        ImGui::Text("Point: X=%.3f  Y=%.3f  Confidence=%.3f",
                    point.x,
                    point.y,
                    point.confidence);
        ImGui::Text("Report Coord: X=%u  Y=%u",
                    static_cast<unsigned int>(point.reportX),
                    static_cast<unsigned int>(point.reportY));
        ImGui::Text("TX1/TX2 Coord: (%.3f,%.3f) / (%.3f,%.3f)",
                    point.tx1X,
                    point.tx1Y,
                    point.tx2X,
                    point.tx2Y);
        ImGui::Text("Signal TX1/TX2 (Composite): %u / %u", point.peakTx1, point.peakTx2);
        ImGui::Text("Tilt: Valid=%s Pre(%d,%d) Out(%d,%d) |Mag|=%.2f Az=%.1f",
                    point.tiltValid ? "Y" : "N",
                    static_cast<int>(point.preTiltX),
                    static_cast<int>(point.preTiltY),
                    static_cast<int>(point.tiltX),
                    static_cast<int>(point.tiltY),
                    point.tiltMagnitude,
                    point.tiltAzimuthDeg);
    } else {
        ImGui::TextUnformatted("Point: Invalid");
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Legacy Stylus Packet (optional)")) {
#if EGOTOUCH_DIAG
        if (output.packet.valid) {
            ImGui::Text("Packet (RID=0x%02X, Len=%u):", output.packet.reportId, output.packet.length);
            std::ostringstream oss;
            oss << std::hex << std::setfill('0');
            for (size_t i = 0; i < output.packet.bytes.size(); ++i) {
                oss << std::setw(2) << static_cast<unsigned int>(output.packet.bytes[i]);
                if (i + 1 < output.packet.bytes.size()) {
                    oss << " ";
                }
            }
            ImGui::TextUnformatted(oss.str().c_str());
        } else {
            ImGui::TextDisabled("Legacy packet unavailable for current frame.");
        }
#else
        ImGui::TextDisabled("Legacy packet mirror is disabled in non-diagnostic builds.");
#endif
    }

    ImGui::End();
}

void DiagnosticsWorkbench::DrawMasterSuffixTable() {
    if (m_currentFrame.masterSuffixValid) {
        float itemWidth = ImGui::CalcTextSize("[000]: 0000 (00000)").x + ImGui::GetStyle().CellPadding.x * 2.0f + 10.0f;
        int columns = std::max(1, std::min(64, static_cast<int>(ImGui::GetContentRegionAvail().x / itemWidth)));

        if (ImGui::BeginTable("MasterSuffixTable", columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
            for (int i = 0; i < Frame::kMasterSuffixWords; ++i) {
                if (i % columns == 0) ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(i % columns);
                uint16_t val = m_currentFrame.masterSuffix.words[i];
                ImGui::Text("[%03d]: %04X (%5d)", i, val, val);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::Text("Insufficient frame data length.");
    }

}

void DiagnosticsWorkbench::DrawSlaveSuffixTable() {
    if (m_currentFrame.slaveSuffixValid) {
        float itemWidth = ImGui::CalcTextSize("[000]: 0000 (00000)").x + ImGui::GetStyle().CellPadding.x * 2.0f + 10.0f;
        int columns = std::max(1, std::min(64, static_cast<int>(ImGui::GetContentRegionAvail().x / itemWidth)));

        if (ImGui::BeginTable("SlaveSuffixTable", columns, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit)) {
            for (int i = 0; i < Frame::kSlaveSuffixWords; ++i) {
                if (i % columns == 0) ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(i % columns);
                uint16_t val = m_currentFrame.slaveSuffix.words[i];
                ImGui::Text("[%03d]: %04X (%5d)", i, val, val);
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::Text("Insufficient slave overlay data length.");
    }

}

} // namespace App
