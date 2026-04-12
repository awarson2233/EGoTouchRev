#!/usr/bin/env python3
from __future__ import annotations

import argparse
import configparser
import re
import sys
from pathlib import Path
from typing import Callable


SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent

TOUCH_LOAD_CONFIG = REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "TouchPipeline.cpp"
STYLUS_LOAD_CONFIG = REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "StylusPipeline.cpp"
SERVICE_HOST_H = REPO_ROOT / "EGoTouchService" / "include" / "ServiceHost.h"
SERVICE_HOST_CPP = REPO_ROOT / "EGoTouchService" / "source" / "ServiceHost.cpp"


MODULE_TO_FILE: dict[str, Path] = {
    "m_frameParser": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "MasterFrameParser.hpp",
    "m_baseline": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "BaselineSubtraction.hpp",
    "m_cmf": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "CMFProcessor.hpp",
    "m_gridIIR": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "GridIIRProcessor.hpp",
    "m_peakDet": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "PeakDetector.hpp",
    "m_zoneExp": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "ZoneExpander.hpp",
    "m_edgeComp": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "EdgeCompensation.hpp",
    "m_palmReject": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "PalmRejector.hpp",
    "m_tracker": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "TouchTracker.hpp",
    "m_coordFilter": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "CoordinateFilter.hpp",
    "m_gesture": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "TouchGestureStateMachine.hpp",
    "m_coordSolver": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "CoordinateSolver.hpp",
    "m_penStateMachine": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "PenStateMachine.hpp",
    "m_postProcessor": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "CoorPostProcessor.hpp",
    "m_linearFilter": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "LinearFilter.hpp",
    "m_coorReviser": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "CoorReviser.hpp",
    "m_edgeLiftCorrector": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "EdgeLiftCorrector.hpp",
    "m_noiseGate": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "NoiseGate.hpp",
    "m_edgeCoorPost": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "PipelineUtils.hpp",
    "m_cmfFilter": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "CommonModeFilter.hpp",
    "m_pressureSolver": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "PressureSolver.hpp",
    "m_noPressInkGate": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "NoPressInkGate.hpp",
    "m_oneEuroFilter": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "OneEuroFilter.hpp",
}

TOP_LEVEL_FILE: dict[str, Path] = {
    "TouchPipeline": REPO_ROOT / "EGoTouchService" / "Solvers" / "TouchSolver" / "TouchPipeline.h",
    "StylusPipeline": REPO_ROOT / "EGoTouchService" / "Solvers" / "StylusSolver" / "StylusPipeline.h",
}

LOAD_KEY_RE = re.compile(
    r'key\s*==\s*"([^"]+)"\)\s*([A-Za-z_][A-Za-z0-9_\.]+)\s*=',
)

MANUAL_EXPR_MAP: dict[str, dict[str, str]] = {
    "StylusPipeline": {
        "sp.triEdgeSecondaryBlend": "m_coordSolver.triEdgeSecondaryBlend",
        "sp.coordEdgeCompBit3": "m_coordSolver.triEdgeSecondaryBlend",
    },
}


def cpp_bool(value: str) -> str:
    return "true" if value.strip().lower() in {"1", "true", "yes", "on"} else "false"


def cpp_float(value: str) -> str:
    text = value.strip()
    if text.lower().endswith("f"):
        return text
    if not re.search(r"[.eE]", text):
        text += ".0"
    return f"{text}f"


def cpp_int(value: str) -> str:
    return str(int(value.strip(), 10))


def replace_with_regex(text: str, pattern: re.Pattern[str], repl: Callable[[re.Match[str]], str], *, count: int = 1) -> tuple[str, int]:
    return pattern.subn(repl, text, count=count)


class SourceEditor:
    def __init__(self) -> None:
        self._cache: dict[Path, str] = {}
        self.modified: set[Path] = set()

    def read(self, path: Path) -> str:
        if path not in self._cache:
            self._cache[path] = path.read_text(encoding="utf-8")
        return self._cache[path]

    def write_back(self) -> None:
        for path in sorted(self.modified):
            path.write_text(self._cache[path], encoding="utf-8", newline="")

    def patch(self, path: Path, transform: Callable[[str], tuple[str, int]], description: str) -> bool:
        text = self.read(path)
        new_text, count = transform(text)
        if count != 1:
            return False
        if new_text != text:
            self._cache[path] = new_text
            self.modified.add(path)
        return True


def parse_load_config_map(path: Path) -> dict[str, str]:
    mapping: dict[str, str] = {}
    for key, expr in LOAD_KEY_RE.findall(path.read_text(encoding="utf-8")):
        mapping[key] = expr
    return mapping


def replace_direct_field(editor: SourceEditor, path: Path, field_name: str, new_literal: str) -> bool:
    pattern = re.compile(
        rf"(^\s*[\w:<>,\s\*&]+\b{re.escape(field_name)}\b\s*=\s*)([^;]+)(;[^\n]*$)",
        re.MULTILINE,
    )
    return editor.patch(path, lambda text: replace_with_regex(text, pattern, lambda m: f"{m.group(1)}{new_literal}{m.group(3)}"), f"{path}:{field_name}")


def replace_cmf_mode(editor: SourceEditor, value: str) -> bool:
    enum_map = {
        "0": "DimensionMode::None",
        "1": "DimensionMode::RowWise",
        "2": "DimensionMode::ColumnWise",
        "3": "DimensionMode::DualDim",
    }
    enum_value = enum_map.get(value.strip(), "DimensionMode::RowWise")
    return replace_direct_field(
        editor,
        MODULE_TO_FILE["m_cmf"],
        "m_mode",
        enum_value,
    )


def replace_tri_edge(editor: SourceEditor, struct_name: str, field_name: str, raw_value: str) -> bool:
    path = MODULE_TO_FILE["m_coordSolver"]
    value = cpp_int(raw_value)
    index_map = {"ratio": 2, "sumThresholdIdxLast": 4, "sumThresholdIdx0": 6}
    target_group = index_map[field_name]
    pattern = re.compile(
        rf"(^\s*TriangleEdgeParams\s+{re.escape(struct_name)}\s*=\s*\{{\s*)([^,}}]+)(\s*,\s*)([^,}}]+)(\s*,\s*)([^}}]+)(\s*\}};[^\n]*$)",
        re.MULTILINE,
    )

    def repl(match: re.Match[str]) -> str:
        parts = [match.group(i) for i in range(1, 8)]
        parts[target_group - 1] = value
        return "".join(parts)

    return editor.patch(path, lambda text: replace_with_regex(text, pattern, repl), f"{path}:{struct_name}.{field_name}")


def replace_pitch_comp_enabled(editor: SourceEditor, struct_name: str, raw_value: str) -> bool:
    path = MODULE_TO_FILE["m_coordSolver"]
    value = cpp_bool(raw_value)
    pattern = re.compile(
        rf"(?s)(PitchCompensation\s+{re.escape(struct_name)}\s*=\s*\{{\s*\{{.*?\}}\s*,\s*)(true|false)(\s*//[^\n]*\n\s*\}};)",
    )
    return editor.patch(path, lambda text: replace_with_regex(text, pattern, lambda m: f"{m.group(1)}{value}{m.group(3)}"), f"{path}:{struct_name}.enabled")


def replace_edge_coor_post_enabled(editor: SourceEditor, raw_value: str) -> bool:
    path = MODULE_TO_FILE["m_edgeCoorPost"]
    value = cpp_bool(raw_value)
    pattern = re.compile(
        r"(?s)(class\s+EdgeCoorPost\b.*?\n\s*bool\s+enabled\s*=\s*)(true|false)(;)",
    )
    return editor.patch(path, lambda text: replace_with_regex(text, pattern, lambda m: f"{m.group(1)}{value}{m.group(3)}"), f"{path}:EdgeCoorPost.enabled")


def replace_service_mode(editor: SourceEditor, raw_value: str) -> bool:
    enum_value = "ServiceMode::TouchOnly" if raw_value.strip() == "touch_only" else "ServiceMode::Full"
    ok_h = replace_direct_field(editor, SERVICE_HOST_H, "m_mode", enum_value)
    pattern_cpp = re.compile(r"(^\s*m_mode\s*=\s*)([^;]+)(;[^\n]*$)", re.MULTILINE)
    ok_cpp = editor.patch(
        SERVICE_HOST_CPP,
        lambda text: replace_with_regex(text, pattern_cpp, lambda m: f"{m.group(1)}{enum_value}{m.group(3)}"),
        "ServiceHost.cpp:m_mode",
    )
    return ok_h and ok_cpp


def replace_service_bool(editor: SourceEditor, field_name: str, raw_value: str) -> bool:
    value = cpp_bool(raw_value)
    ok_h = replace_direct_field(editor, SERVICE_HOST_H, field_name, value)
    pattern_cpp = re.compile(rf"(^\s*{re.escape(field_name)}\s*=\s*)([^;]+)(;[^\n]*$)", re.MULTILINE)
    ok_cpp = editor.patch(
        SERVICE_HOST_CPP,
        lambda text: replace_with_regex(text, pattern_cpp, lambda m: f"{m.group(1)}{value}{m.group(3)}"),
        f"ServiceHost.cpp:{field_name}",
    )
    return ok_h and ok_cpp


SPECIAL_EXPR_HANDLERS: dict[str, Callable[[SourceEditor, str], bool]] = {
    "m_cmf.m_mode": replace_cmf_mode,
    "m_coordSolver.triEdgeDim1.ratio": lambda ed, v: replace_tri_edge(ed, "triEdgeDim1", "ratio", v),
    "m_coordSolver.triEdgeDim1.sumThresholdIdxLast": lambda ed, v: replace_tri_edge(ed, "triEdgeDim1", "sumThresholdIdxLast", v),
    "m_coordSolver.triEdgeDim1.sumThresholdIdx0": lambda ed, v: replace_tri_edge(ed, "triEdgeDim1", "sumThresholdIdx0", v),
    "m_coordSolver.triEdgeDim2.ratio": lambda ed, v: replace_tri_edge(ed, "triEdgeDim2", "ratio", v),
    "m_coordSolver.triEdgeDim2.sumThresholdIdxLast": lambda ed, v: replace_tri_edge(ed, "triEdgeDim2", "sumThresholdIdxLast", v),
    "m_coordSolver.triEdgeDim2.sumThresholdIdx0": lambda ed, v: replace_tri_edge(ed, "triEdgeDim2", "sumThresholdIdx0", v),
    "m_coordSolver.pitchCompDim1.enabled": lambda ed, v: replace_pitch_comp_enabled(ed, "pitchCompDim1", v),
    "m_coordSolver.pitchCompDim2.enabled": lambda ed, v: replace_pitch_comp_enabled(ed, "pitchCompDim2", v),
    "m_edgeCoorPost.enabled": replace_edge_coor_post_enabled,
}


def infer_literal(expr: str, raw_value: str) -> str:
    text = raw_value.strip()
    lowered = text.lower()
    if lowered in {"0", "1", "true", "false", "yes", "no", "on", "off"}:
        return cpp_bool(text)
    if re.search(r"[.eEfF]", text):
        return cpp_float(text)
    return cpp_int(text)


def apply_expr(editor: SourceEditor, section: str, expr: str, raw_value: str) -> bool:
    if expr in SPECIAL_EXPR_HANDLERS:
        return SPECIAL_EXPR_HANDLERS[expr](editor, raw_value)

    if "." not in expr:
        return replace_direct_field(
            editor,
            TOP_LEVEL_FILE[section],
            expr,
            infer_literal(expr, raw_value),
        )

    module, rest = expr.split(".", 1)
    if "." in rest:
        return False

    path = MODULE_TO_FILE.get(module)
    if path is None:
        return False
    return replace_direct_field(editor, path, rest, infer_literal(expr, raw_value))


def load_ini(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser(interpolation=None)
    parser.optionxform = str
    with path.open("r", encoding="utf-8") as fh:
        parser.read_file(fh)
    return parser


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Replace hardcoded source defaults with values from scripts/config.ini",
    )
    parser.add_argument(
        "--config",
        default=str(SCRIPT_DIR / "config.ini"),
        help="INI file to read (default: scripts/config.ini)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Report changes without writing files",
    )
    args = parser.parse_args()

    config_path = Path(args.config).resolve()
    if not config_path.exists():
        print(f"[ERROR] config file not found: {config_path}", file=sys.stderr)
        return 1

    touch_map = parse_load_config_map(TOUCH_LOAD_CONFIG)
    stylus_map = parse_load_config_map(STYLUS_LOAD_CONFIG)
    ini = load_ini(config_path)
    editor = SourceEditor()

    applied: list[str] = []
    skipped: list[str] = []

    for section in ini.sections():
        items = ini.items(section)
        if section == "Service":
            for key, value in items:
                ok = False
                if key == "mode":
                    ok = replace_service_mode(editor, value)
                elif key == "auto_mode":
                    ok = replace_service_bool(editor, "m_autoMode", value)
                elif key == "stylus_vhf_enabled":
                    ok = replace_service_bool(editor, "m_stylusVhfEnabled", value)
                if ok:
                    applied.append(f"[Service] {key}={value}")
                else:
                    skipped.append(f"[Service] {key}={value}")
            continue

        if section == "TouchPipeline":
            mapping = dict(touch_map)
        elif section == "StylusPipeline":
            mapping = dict(stylus_map)
        else:
            for key, value in items:
                skipped.append(f"[{section}] {key}={value}")
            continue

        mapping.update(MANUAL_EXPR_MAP.get(section, {}))

        for key, value in items:
            expr = mapping.get(key)
            if expr and apply_expr(editor, section, expr, value):
                applied.append(f"[{section}] {key}={value} -> {expr}")
            else:
                skipped.append(f"[{section}] {key}={value}")

    if args.dry_run:
        print("[DRY-RUN] files that would be modified:")
        for path in sorted(editor.modified):
            print(f"  {path.relative_to(REPO_ROOT)}")
    else:
        editor.write_back()
        print("[OK] modified files:")
        for path in sorted(editor.modified):
            print(f"  {path.relative_to(REPO_ROOT)}")

    print(f"\nApplied: {len(applied)}")
    for line in applied:
        print(f"  {line}")

    if skipped:
        print(f"\nSkipped: {len(skipped)}")
        for line in skipped:
            print(f"  {line}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
