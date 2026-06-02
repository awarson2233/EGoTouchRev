#!/usr/bin/env python3
"""
Config Key 双向同步脚本
========================

维护 config/*.yaml 描述文件 与 C++ 代码中配置键/值的双向一致性。

用法:
  python scripts/config_key_sync.py check       # CI 模式: 不一致即退出非零
  python scripts/config_key_sync.py generate    # 正向: YAML → C++ 代码生成
  python scripts/config_key_sync.py update-yaml # 反向: C++ → YAML (以代码为准)
  python scripts/config_key_sync.py diff         # 仅报告差异, 不修改文件

要求:
  Python 3.10+ (标准库即可, 无外部依赖)
"""

import argparse
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

# ── 项目根目录 (脚本位于 <repo>/scripts/) ──
REPO_ROOT = Path(__file__).resolve().parent.parent

# ── 配置描述文件目录 ──
CONFIG_DIR = REPO_ROOT / "config"

# ── 受管理的 C++ 源文件 ──
MANAGED_FILES: dict[str, dict[str, str | list[Path]]] = {
    "touch_pipeline": {
        "yaml": "config/touch_pipeline_config.yaml",
        "sources": [
            "EGoTouchService/Solvers/TouchSolver/TouchPipeline.cpp",
        ],
        "generated_header": "EGoTouchService/Solvers/TouchSolver/TouchPipelineConfigKeys.h",
        "generated_source": "EGoTouchService/Solvers/TouchSolver/TouchPipelineConfigKeys.cpp",
    },
    "stylus_pipeline": {
        "yaml": "config/stylus_pipeline_config.yaml",
        "sources": [
            "EGoTouchService/Solvers/StylusSolver/StylusPipeline.cpp",
        ],
        "generated_header": "EGoTouchService/Solvers/StylusSolver/StylusPipelineConfigKeys.h",
        "generated_source": "EGoTouchService/Solvers/StylusSolver/StylusPipelineConfigKeys.cpp",
    },
}


# ═══════════════════════════════════════════════════════════════════════
# 数据模型
# ═══════════════════════════════════════════════════════════════════════

@dataclass
class ConfigKeyDef:
    """单个配置键的完整定义。"""
    key: str
    display: str = ""
    type: str = "int"                     # int | float | bool | uint8 | uint16
    member: str = ""                      # C++ 成员字段路径
    default_raw: Optional[str] = None     # YAML 中的默认值字符串
    default_cpp: Optional[str] = None     # 从 C++ constexpr 提取的默认值
    min_val: Optional[str] = None
    max_val: Optional[str] = None
    module: str = ""
    release: str = "active"              # active | frozen
    description: str = ""

    # ── 来源追踪 ──
    in_yaml: bool = False
    in_cpp_schema: bool = False           # GetConfigSchema() 中有条目
    in_cpp_load: bool = False             # LoadConfig() 中有分支
    in_cpp_save: bool = False             # SaveConfig() 中有序列化
    in_cpp_frozen: bool = False           # IsFrozenCurrentTouchConfigKey() 中冻结
    in_cpp_constexpr: bool = False        # 有对应的 constexpr 默认值定义

    @property
    def effective_default(self) -> Optional[str]:
        """实际生效的默认值: C++ 代码为权威来源。"""
        return self.default_cpp or self.default_raw

    @property
    def is_frozen(self) -> bool:
        return self.release == "frozen" or self.in_cpp_frozen

    @property
    def cpp_type(self) -> str:
        """映射到 C++ 类型名。"""
        return {"int": "int", "float": "float", "bool": "bool", "uint8": "uint8_t", "uint16": "uint16_t"}.get(self.type, "int")

    @property
    def cpp_constexpr_name(self) -> str:
        """生成 constexpr 常量名。"""
        return f"k{_cpp_key_ident(self.key)}"


# ═══════════════════════════════════════════════════════════════════════
# YAML 解析 (最小实现, 无 PyYAML 依赖)
# ═══════════════════════════════════════════════════════════════════════

def _parse_yaml_value(s: str) -> str:
    """去除 YAML 值中的引号和尾部注释。"""
    s = s.strip()
    # 去除行内注释 (# 后面有空格或结尾)
    if '#' in s:
        idx = s.index('#')
        s = s[:idx].strip()
    # 去除引号
    if (s.startswith('"') and s.endswith('"')) or (s.startswith("'") and s.endswith("'")):
        s = s[1:-1]
    return s


def parse_yaml_config(yaml_path: Path) -> list[ConfigKeyDef]:
    """从 YAML 描述文件解析配置键列表。

    支持的简化 YAML 结构:
        keys:
          - key: BaselineBackgroundAlphaShift
            display: "Baseline Background Alpha Shift"
            type: int
            member: m_baseline.m_backgroundAlphaShift
            default: 4
            min: 0
            max: 15
            module: "Signal Conditioning"
            release: active
            description: "..."

    若文件不存在, 返回空列表。
    """
    if not yaml_path.exists():
        return []

    lines = yaml_path.read_text(encoding="utf-8").splitlines()
    keys: list[ConfigKeyDef] = []
    current: Optional[dict] = None
    in_keys = False

    for raw_line in lines:
        line = raw_line.rstrip()
        # 跳过空行和顶层注释
        if not line or line.startswith('#') and not line.startswith('  '):
            continue

        stripped = line.lstrip()
        indent = len(line) - len(stripped)

        if stripped.startswith('keys:'):
            in_keys = True
            continue

        if not in_keys:
            continue

        if stripped.startswith('- ') and indent == 2:
            # 新条目开始
            if current:
                keys.append(_dict_to_config_key(current))
            current = {}
            # 检查 inline 写法: "- key: value"
            inline = stripped[2:].strip()
            if ':' in inline:
                k, v = inline.split(':', 1)
                current[k.strip()] = _parse_yaml_value(v)
        elif current is not None and ':' in stripped and indent >= 4:
            k, v = stripped.split(':', 1)
            current[k.strip()] = _parse_yaml_value(v)

    if current:
        keys.append(_dict_to_config_key(current))

    for k in keys:
        k.in_yaml = True

    return keys


def _dict_to_config_key(d: dict) -> ConfigKeyDef:
    return ConfigKeyDef(
        key=d.get("key", ""),
        display=d.get("display", d.get("key", "")),
        type=d.get("type", "int"),
        member=d.get("member", ""),
        default_raw=str(d["default"]) if "default" in d else None,
        min_val=str(d["min"]) if "min" in d else None,
        max_val=str(d["max"]) if "max" in d else None,
        module=d.get("module", ""),
        release=d.get("release", "active"),
        description=d.get("description", ""),
    )


def write_yaml_config(yaml_path: Path, keys: list[ConfigKeyDef]) -> None:
    """将配置键列表写回 YAML 文件。"""
    lines = [
        "# EGoTouchRev Config Key Description",
        f"# Auto-generated by config_key_sync.py — do not hand-edit defaults",
        f"# Default values are authoritative from C++ constexpr definitions.",
        f"",
        f"keys:",
    ]

    for k in sorted(keys, key=lambda x: (x.module, x.key)):
        lines.append(f"  - key: {k.key}")
        if k.display and k.display != k.key:
            lines.append(f'    display: "{k.display}"')
        lines.append(f"    type: {k.type}")
        if k.member:
            lines.append(f"    member: {k.member}")
        if k.effective_default is not None:
            if k.type == "bool":
                lines.append(f"    default: {k.effective_default}")
            else:
                lines.append(f"    default: {k.effective_default}")
        if k.min_val is not None:
            lines.append(f"    min: {k.min_val}")
        if k.max_val is not None:
            lines.append(f"    max: {k.max_val}")
        if k.module:
            lines.append(f'    module: "{k.module}"')
        lines.append(f"    release: {k.release}")
        if k.description:
            lines.append(f'    description: "{k.description}"')
        lines.append("")

    yaml_path.parent.mkdir(parents=True, exist_ok=True)
    yaml_path.write_text("\n".join(lines), encoding="utf-8")


# ═══════════════════════════════════════════════════════════════════════
# C++ 代码解析
# ═══════════════════════════════════════════════════════════════════════

def _strip_if0_blocks(content: str) -> str:
    """移除 #if 0 ... #endif 块, 避免解析保留的旧实现。"""
    kept: list[str] = []
    skip_depth = 0

    for line in content.splitlines():
        stripped = line.strip()
        if skip_depth > 0:
            if re.match(r'#\s*if\b', stripped):
                skip_depth += 1
            elif re.match(r'#\s*endif\b', stripped):
                skip_depth -= 1
            continue

        if re.match(r'#\s*if\s+0\b', stripped):
            skip_depth = 1
            continue

        kept.append(line)

    return "\n".join(kept)


def parse_cpp_extract_keys(cpp_paths: list[Path]) -> dict[str, ConfigKeyDef]:
    """从 C++ 源文件中提取所有配置键信息。

    提取来源:
      1. LoadConfig() 中的 if/else if (key=="...") 分支     → 键名
      2. SaveConfig() 中的 configOut << "KeyName=" << ...   → 键名
      3. GetConfigSchema() 中的 emplace_back("KeyName",...)  → 完整元数据
      4. IsFrozenCurrentTouchConfigKey() 中的字符串数组       → 冻结状态
      5. *ConfigKeys.h 中的 constexpr kKeyName = ...         → 默认值
    """
    keys: dict[str, ConfigKeyDef] = {}
    global_name_constants: dict[str, str] = {}
    for prepass_path in cpp_paths:
        if not prepass_path.exists():
            continue
        prepass_content = _strip_if0_blocks(prepass_path.read_text(encoding="utf-8"))
        global_name_constants.update({m.group(1): m.group(2) for m in re.finditer(
            r'constexpr\s+const\s+char\*\s+k(\w+)Name\s*=\s*"([\w.]+)"\s*;',
            prepass_content
        )})

    for cpp_path in cpp_paths:
        if not cpp_path.exists():
            continue
        content = _strip_if0_blocks(cpp_path.read_text(encoding="utf-8"))
        name_constants = dict(global_name_constants)

        # ── 1. 提取 LoadConfig 分支中的键名 ──
        for m in re.finditer(
            r'(?:if|else\s+if)\s*\(\s*key\s*==\s*(?:"([\w.]+)"|k(\w+)Name)\s*\)',
            content
        ):
            key_name = m.group(1) or name_constants.get(m.group(2), m.group(2))
            _ensure_key(keys, key_name).in_cpp_load = True

        # ── 2. 提取 SaveConfig 序列化行中的键名 ──
        for m in re.finditer(
            r'(?:configOut|out)\s*<<\s*(?:"([\w.]+)="|k(\w+)Name\s*<<\s*"=")',
            content
        ):
            key_name = m.group(1) or name_constants.get(m.group(2), m.group(2))
            _ensure_key(keys, key_name).in_cpp_save = True

        # ── 3. 提取 GetConfigSchema 中的元数据 ──
        # emplace_back("KeyName", "Display Name", ConfigParam::Bool, ...)
        # emplace_back(kKeyNameName, "Display Name", ConfigParam::Bool, ...)
        for m in re.finditer(
            r'emplace_back\(\s*(?:"([\w.]+)"|k(\w+)Name)\s*,\s*"([^"]*)"\s*,\s*ConfigParam::(\w+)',
            content
        ):
            key_name = m.group(1) or name_constants.get(m.group(2), m.group(2))
            display = m.group(3)
            type_str = m.group(4).lower()
            # ConfigParam::Bool → bool, ConfigParam::Int → int, etc.
            type_map = {"bool": "bool", "int": "int", "float": "float", "uint8": "uint8", "uint16": "uint16"}
            k = _ensure_key(keys, key_name)
            k.in_cpp_schema = True
            if display and display != key_name:
                k.display = display
            k.type = type_map.get(type_str, k.type)

        # ── 4. 提取冻结键列表 ──
        frozen_section = False
        for line in content.splitlines():
            if "IsFrozenCurrentTouchConfigKey" in line or "kFrozenKeys" in line:
                frozen_section = True
                continue
            if frozen_section:
                if '}' in line or ';' in line and '"' not in line:
                    frozen_section = False
                    continue
                m = re.search(r'"([\w.]+)"', line)
                if m:
                    _ensure_key(keys, m.group(1)).in_cpp_frozen = True

        # ── 5. 提取 constexpr 默认值 (来自生成的 ConfigKeys.h) ──
        for m in re.finditer(
            r'constexpr\s+(int|float|bool|uint8_t|uint16_t)\s+k(\w+)\s*=\s*([^;]+);',
            content
        ):
            key_name = name_constants.get(m.group(2), m.group(2))
            cpp_type = {"uint8_t": "uint8", "uint16_t": "uint16"}.get(m.group(1), m.group(1))
            default_val = m.group(3).strip()
            # 去除可能的类型转换 (如 static_cast<int>(true))
            default_val = re.sub(r'static_cast<\w+>\((.*?)\)', r'\1', default_val)
            k = _ensure_key(keys, key_name)
            k.in_cpp_constexpr = True
            k.default_cpp = default_val
            k.type = k.type or cpp_type

        # ── 5b. 也搜索 constexpr 在命名空间内的形式 ──
        for m in re.finditer(
            r'constexpr\s+(int|float|bool|uint8_t|uint16_t)\s+(\w+)\s*=\s*([^;]+);',
            content
        ):
            key_name = name_constants.get(m.group(2), m.group(2))
            cpp_type = {"uint8_t": "uint8", "uint16_t": "uint16"}.get(m.group(1), m.group(1))
            default_val = m.group(3).strip()
            default_val = re.sub(r'static_cast<\w+>\((.*?)\)', r'\1', default_val)
            if key_name in keys:
                k = keys[key_name]
                k.in_cpp_constexpr = True
                k.default_cpp = default_val
                k.type = k.type or cpp_type

    return keys


def _ensure_key(keys: dict[str, ConfigKeyDef], name: str) -> ConfigKeyDef:
    if name not in keys:
        # C++ 解析侧的裸键名不一定携带类型信息；避免 dataclass 默认 int
        # 让冻结键等无 schema/constexpr 的引用误触发类型不一致。
        keys[name] = ConfigKeyDef(key=name, type="")
    return keys[name]


# ═══════════════════════════════════════════════════════════════════════
# 差异分析
# ═══════════════════════════════════════════════════════════════════════

@dataclass
class KeyDiff:
    key: str
    issues: list[str] = field(default_factory=list)

    @property
    def is_ok(self) -> bool:
        return len(self.issues) == 0


def diff_keys(yaml_keys: list[ConfigKeyDef],
              cpp_keys: dict[str, ConfigKeyDef]) -> dict[str, KeyDiff]:
    """对比 YAML 描述与 C++ 代码中的配置键, 返回差异报告。"""
    diffs: dict[str, KeyDiff] = {}

    # ── 索引 ──
    yaml_by_key = {k.key: k for k in yaml_keys}

    all_keys = set(yaml_by_key.keys()) | set(cpp_keys.keys())

    for key_name in sorted(all_keys):
        yd = yaml_by_key.get(key_name)
        cd = cpp_keys.get(key_name)
        diff = KeyDiff(key=key_name)

        if yd and not cd:
            diff.issues.append("存在于 YAML 但 C++ 代码中未找到任何引用")
        elif cd and not yd:
            diff.issues.append("存在于 C++ 代码但 YAML 中缺失")
        else:
            # 两端都有, 检查一致性
            assert yd and cd  # type narrowing

            # 冻结状态
            yaml_frozen = (yd.release == "frozen")
            if yaml_frozen != cd.in_cpp_frozen:
                diff.issues.append(
                    f"冻结状态不一致: YAML={yaml_frozen}, C++={cd.in_cpp_frozen}"
                )

            # 类型
            if yd.type != cd.type and cd.type:
                diff.issues.append(
                    f"类型不一致: YAML={yd.type}, C++={cd.type}"
                )

            # 默认值 (以 C++ 为准)
            if yd.effective_default is not None and cd.default_cpp is not None:
                if yd.effective_default != cd.default_cpp:
                    diff.issues.append(
                        f"默认值不一致: YAML={yd.effective_default}, C++={cd.default_cpp}"
                    )

            # 出现在代码各处的一致性
            if cd.in_cpp_load and not cd.in_cpp_save:
                diff.issues.append("LoadConfig 中有但 SaveConfig 中缺失")
            if cd.in_cpp_save and not cd.in_cpp_load:
                diff.issues.append("SaveConfig 中有但 LoadConfig 中缺失")
            if cd.in_cpp_schema and not cd.in_cpp_load:
                diff.issues.append("GetConfigSchema 中有但 LoadConfig 中缺失")

        diffs[key_name] = diff

    return diffs


# ═══════════════════════════════════════════════════════════════════════
# 代码生成 (YAML → C++)
# ═══════════════════════════════════════════════════════════════════════

def generate_config_header(keys: list[ConfigKeyDef], module_name: str) -> str:
    """从 YAML 键列表生成 ConfigKeys.h 内容。"""
    active_keys = [k for k in keys if k.release == "active"]
    if module_name == "stylus_pipeline":
        return _generate_stylus_config_header(keys, module_name)

    lines = [
        "// Auto-generated by scripts/config_key_sync.py — DO NOT EDIT",
        "// Source: config/{module}_config.yaml".format(module=module_name),
        "",
        "#pragma once",
        "",
        "#include <cstdint>",
        "#include <iosfwd>",
        "#include <string>",
        "#include <vector>",
        "",
        '#include "SolverBuildConfig.h"',
        "",
        "namespace Solvers {",
        "struct ConfigParam;",
        "namespace Touch {",
        "class MasterFrameParser;",
        "class BaselineTracker;",
        "class CMFProcessor;",
        "class MacroZoneDetector;",
        "class PeakDetector;",
        "class TouchClassifier;",
        "class ContactExtractor;",
        "class EdgeCompensator;",
        "class EdgeRejector;",
        "class StylusTouchSuppressor;",
        "class TouchTracker;",
        "class CoordinateFilter;",
        "class TouchGestureStateMachine;",
        "} // namespace Touch",
        "",
    ]

    ns = "TouchConfig" if module_name == "touch_pipeline" else "StylusConfig"
    lines.append(f"namespace {ns} {{")
    lines.append("")

    if active_keys:
        lines.append("// ── Active config key defaults (shared by Debug/Release) ──")
        for k in active_keys:
            if k.effective_default is not None:
                cpp_default = _to_cpp_literal(k.effective_default, k.type)
                lines.append(f"constexpr {k.cpp_type} k{_cpp_key_ident(k.key)} = {cpp_default};")
        lines.append("")

    lines.append("// ── Active key name constants ──")
    for k in active_keys:
        lines.append(f'constexpr const char* k{_cpp_key_ident(k.key)}Name = "{k.key}";')
    lines.append("")

    lines.append("#if EGOTOUCH_CONFIG_ENABLED")
    lines.append("")
    if module_name == "touch_pipeline":
        lines.extend([
            "struct TouchPipelineMembers {",
            "    Touch::MasterFrameParser* frameParser;",
            "    Touch::BaselineTracker* baseline;",
            "    Touch::CMFProcessor* cmf;",
            "    Touch::MacroZoneDetector* macroZoneDet;",
            "    Touch::PeakDetector* peakDet;",
            "    Touch::TouchClassifier* touchClassifier;",
            "    Touch::ContactExtractor* contactExtractor;",
            "    Touch::EdgeCompensator* edgeComp;",
            "    Touch::EdgeRejector* edgeReject;",
            "    Touch::StylusTouchSuppressor* stylusSuppress;",
            "    Touch::TouchTracker* tracker;",
            "    Touch::CoordinateFilter* coordFilter;",
            "    Touch::TouchGestureStateMachine* gesture;",
            "};",
            "",
            "void LoadConfig(TouchPipelineMembers& m, const std::string& key, const std::string& value);",
            "void SaveConfig(const TouchPipelineMembers& m, std::ostream& out);",
            "std::vector<ConfigParam> GetConfigSchema(TouchPipelineMembers& m);",
        ])
    else:
        lines.extend([
            "// Generation for this module is not yet wired to concrete member accessors.",
            "void LoadConfig(const std::string& key, const std::string& value);",
            "void SaveConfig(std::ostream& out);",
            "std::vector<ConfigParam> GetConfigSchema();",
        ])
    lines.append("")
    lines.append("#endif  // EGOTOUCH_CONFIG_ENABLED")
    lines.append("")
    lines.append(f"}}  // namespace {ns}")
    lines.append("}  // namespace Solvers")

    return "\n".join(lines) + "\n"


def generate_config_source(keys: list[ConfigKeyDef], module_name: str) -> str:
    """从 YAML 键列表生成 ConfigKeys.cpp 内容。"""
    if module_name == "stylus_pipeline":
        return _generate_stylus_config_source(keys, module_name)
    if module_name != "touch_pipeline":
        return _generate_unsupported_config_source(module_name)

    active_keys = [k for k in keys if k.release == "active" and k.member]
    ns = "TouchConfig"

    lines = [
        "// Auto-generated by scripts/config_key_sync.py — DO NOT EDIT",
        "// Source: config/{module}_config.yaml".format(module=module_name),
        "",
        '#include "TouchPipelineConfigKeys.h"',
        '#include "SolverBuildConfig.h"',
        "",
        "#if EGOTOUCH_CONFIG_ENABLED",
        "",
        '#include "TouchPipeline.h"',
        '#include "ConfigParse.h"',
        "",
        "#include <ostream>",
        "",
        "namespace Solvers {",
        f"namespace {ns} {{",
        "",
        "// ── GetConfigSchema ──",
        "std::vector<ConfigParam> GetConfigSchema(TouchPipelineMembers& m) {",
        "    std::vector<ConfigParam> s;",
    ]
    if active_keys:
        lines.append(f"    s.reserve({len(active_keys)});")
    lines.append("")

    current_module = None
    for k in active_keys:
        if k.module != current_module:
            if current_module is not None:
                lines.append("")
            current_module = k.module
            if current_module:
                lines.append(f"    // ── {current_module} ──")

        cpp_type_enum = {"int": "Int", "float": "Float", "bool": "Bool"}[k.type]
        member = _touch_member_expr(k.member)
        ptr_type = k.cpp_type
        if k.min_val is not None and k.max_val is not None:
            lines.append(
                f'    s.emplace_back(k{_cpp_key_ident(k.key)}Name, "{_cpp_string(k.display or k.key)}", '
                f'ConfigParam::{cpp_type_enum}, const_cast<{ptr_type}*>(&{member}), '
                f'{_to_cpp_range_literal(k.min_val)}, {_to_cpp_range_literal(k.max_val)}).Module("{_cpp_string(k.module)}");'
            )
        else:
            lines.append(
                f'    s.emplace_back(k{_cpp_key_ident(k.key)}Name, "{_cpp_string(k.display or k.key)}", '
                f'ConfigParam::{cpp_type_enum}, const_cast<{ptr_type}*>(&{member})).Module("{_cpp_string(k.module)}");'
            )

    lines.extend([
        "",
        "    return s;",
        "}",
        "",
        "// ── SaveConfig ──",
        "void SaveConfig(const TouchPipelineMembers& m, std::ostream& out) {",
    ])
    for k in active_keys:
        member = _touch_member_expr(k.member)
        if k.type == "bool":
            lines.append(f'    out << k{_cpp_key_ident(k.key)}Name << "=" << ({member} ? "1" : "0") << "\\n";')
        else:
            lines.append(f'    out << k{_cpp_key_ident(k.key)}Name << "=" << {member} << "\\n";')
    lines.extend([
        "}",
        "",
        "// ── LoadConfig ──",
        "void LoadConfig(TouchPipelineMembers& m, const std::string& key, const std::string& value) {",
    ])
    if any(k.type == "bool" for k in active_keys):
        lines.append("    auto toBool = [&](const std::string& v) { return ParseConfigBool(key, v); };")
    lines.append("    try {")

    for i, k in enumerate(active_keys):
        prefix = "if" if i == 0 else "else if"
        member = _touch_member_expr(k.member)
        if k.type == "bool":
            parse_expr = "toBool(value)"
        elif k.type == "int":
            parse_expr = "ParseConfigInt(key, value)"
        elif k.type == "float":
            parse_expr = "ParseConfigFloat(key, value)"
        else:
            continue

        if k.type in ("int", "float") and (k.min_val is not None or k.max_val is not None):
            lines.append(f'        {prefix} (key == k{_cpp_key_ident(k.key)}Name) {{')
            lines.append(f'            {k.cpp_type} v = {parse_expr};')
            if k.min_val is not None:
                min_literal = _to_cpp_range_literal(k.min_val)
                lines.append(f'            if (v < {min_literal}) v = {min_literal};')
            if k.max_val is not None:
                max_literal = _to_cpp_range_literal(k.max_val)
                lines.append(f'            if (v > {max_literal}) v = {max_literal};')
            lines.append(f'            {member} = v;')
            lines.append('        }')
        else:
            lines.append(f'        {prefix} (key == k{_cpp_key_ident(k.key)}Name) {{ {member} = {parse_expr}; }}')

    lines.extend([
        "    } catch (const ConfigParseError& error) {",
        '        LogConfigParseWarning("TouchPipeline", __func__, key, value, error);',
        "    }",
        "}",
        "",
        f"}}  // namespace {ns}",
        "}  // namespace Solvers",
        "",
        "#endif  // EGOTOUCH_CONFIG_ENABLED",
        "",
    ])

    return "\n".join(lines)



def _stylus_member_expr(member: str) -> str:
    root_map = {
        "m_frameParser": "frameParser",
        "m_featureExtractor": "featureExtractor",
        "m_coordinateSolver": "coordinateSolver",
        "m_tiltProcess": "tiltProcess",
        "m_pressureSolver": "pressureSolver",
        "m_postPressure": "postPressure",
        "m_edgeCoorProcess": "edgeCoorProcess",
        "m_edgeCoorPostProcess": "edgeCoorPostProcess",
        "m_noisePostProcess": "noisePostProcess",
        "m_linearFilterProcess": "linearFilterProcess",
        "m_coorReviseProcess": "coorReviseProcess",
        "m_coorSpeedProcess": "coorSpeedProcess",
        "m_coorIIRProcess": "coorIIRProcess",
        "m_aftCoorProcess": "aftCoorProcess",
    }
    root, sep, rest = member.partition('.')
    if root not in root_map:
        raise ValueError(f"unsupported stylus member root: {member}")
    return f"m.{root_map[root]}->{rest}" if sep else f"*m.{root_map[root]}"


def _generate_stylus_config_header(keys: list[ConfigKeyDef], module_name: str) -> str:
    active_keys = [k for k in keys if k.release == "active"]
    lines = [
        "// Auto-generated by scripts/config_key_sync.py — DO NOT EDIT",
        "// Source: config/{module}_config.yaml".format(module=module_name),
        "",
        "#pragma once",
        "",
        "#include <cstdint>",
        "#include <iosfwd>",
        "#include <string>",
        "#include <vector>",
        "",
        '#include "SolverBuildConfig.h"',
        "",
        "namespace Solvers {",
        "struct ConfigParam;",
        "namespace Stylus {",
        "class StylusFrameParser;",
        "class GridFeatureExtractor;",
        "class CoordinateSolver;",
        "class TiltProcess;",
        "class PressureSolver;",
        "class Hpp3PostPressureProcess;",
        "class EdgeCoorProcess;",
        "class EdgeCoorPostProcess;",
        "class Hpp3NoisePostProcess;",
        "class LinearFilterProcess;",
        "class CoorReviseProcess;",
        "class CoorSpeedProcess;",
        "class CoorIIRProcess;",
        "class AftCoorProcess;",
        "} // namespace Stylus",
        "",
        "namespace StylusConfig {",
        "",
    ]
    if active_keys:
        lines.append("// ── Active config key defaults (shared by Debug/Release) ──")
        for k in active_keys:
            if k.effective_default is not None:
                lines.append(f"constexpr {k.cpp_type} k{_cpp_key_ident(k.key)} = {_to_cpp_literal(k.effective_default, k.type)};")
        lines.append("")
    lines.append("// ── Active key name constants ──")
    for k in active_keys:
        lines.append(f'constexpr const char* k{_cpp_key_ident(k.key)}Name = "{_cpp_string(k.key)}";')
    lines.extend([
        "",
        "#if EGOTOUCH_CONFIG_ENABLED",
        "",
        "struct StylusPipelineMembers {",
        "    Stylus::StylusFrameParser* frameParser;",
        "    Stylus::GridFeatureExtractor* featureExtractor;",
        "    Stylus::CoordinateSolver* coordinateSolver;",
        "    Stylus::TiltProcess* tiltProcess;",
        "    Stylus::PressureSolver* pressureSolver;",
        "    Stylus::Hpp3PostPressureProcess* postPressure;",
        "    Stylus::EdgeCoorProcess* edgeCoorProcess;",
        "    Stylus::EdgeCoorPostProcess* edgeCoorPostProcess;",
        "    Stylus::Hpp3NoisePostProcess* noisePostProcess;",
        "    Stylus::LinearFilterProcess* linearFilterProcess;",
        "    Stylus::CoorReviseProcess* coorReviseProcess;",
        "    Stylus::CoorSpeedProcess* coorSpeedProcess;",
        "    Stylus::CoorIIRProcess* coorIIRProcess;",
        "    Stylus::AftCoorProcess* aftCoorProcess;",
        "};",
        "",
        "void LoadConfig(StylusPipelineMembers& m, const std::string& key, const std::string& value);",
        "void SaveConfig(const StylusPipelineMembers& m, std::ostream& out);",
        "std::vector<ConfigParam> GetConfigSchema(StylusPipelineMembers& m);",
        "",
        "#endif  // EGOTOUCH_CONFIG_ENABLED",
        "",
        "}  // namespace StylusConfig",
        "}  // namespace Solvers",
        "",
    ])
    return "\n".join(lines)


def _generate_stylus_config_source(keys: list[ConfigKeyDef], module_name: str) -> str:
    active_keys = [k for k in keys if k.release == "active" and k.member]
    type_enum = {"int": "Int", "float": "Float", "bool": "Bool", "uint8": "UInt8", "uint16": "UInt16"}
    reset_on_false = {
        "sp.tiltProcessEnabled": "m.tiltProcess->Reset();",
        "sp.postPressureEnabled": "m.postPressure->Reset();",
        "sp.edgeCoorEnabled": "m.edgeCoorProcess->Reset();",
        "sp.noisePostEnabled": "m.noisePostProcess->Reset();",
        "sp.linearFilterEnabled": "m.linearFilterProcess->Reset();",
        "sp.coorReviseEnabled": "m.coorReviseProcess->Reset();",
        "sp.coorSpeedEnabled": "m.coorSpeedProcess->Reset();",
        "sp.iirFilterEnabled": "m.coorIIRProcess->Reset();",
        "sp.aftCoorEnabled": "m.aftCoorProcess->Reset();",
    }
    lines = [
        "// Auto-generated by scripts/config_key_sync.py — DO NOT EDIT",
        "// Source: config/{module}_config.yaml".format(module=module_name),
        "",
        '#include "StylusPipelineConfigKeys.h"',
        '#include "SolverBuildConfig.h"',
        "",
        "#if EGOTOUCH_CONFIG_ENABLED",
        "",
        '#include "StylusPipeline.h"',
        '#include "ConfigParse.h"',
        "",
        "#include <algorithm>",
        "#include <ostream>",
        "",
        "namespace Solvers {",
        "namespace StylusConfig {",
        "",
        "// ── GetConfigSchema ──",
        "std::vector<ConfigParam> GetConfigSchema(StylusPipelineMembers& m) {",
        "    std::vector<ConfigParam> s;",
    ]
    if active_keys:
        lines.append(f"    s.reserve({len(active_keys)});")
    lines.append("")
    current_module = None
    for k in active_keys:
        if k.module != current_module:
            if current_module is not None:
                lines.append("")
            current_module = k.module
            if current_module:
                lines.append(f"    // ── {current_module} ──")
        member = _stylus_member_expr(k.member)
        ptr_type = k.cpp_type
        if k.min_val is not None and k.max_val is not None:
            lines.append(f'    s.emplace_back(k{_cpp_key_ident(k.key)}Name, "{_cpp_string(k.display or k.key)}", ConfigParam::{type_enum[k.type]}, const_cast<{ptr_type}*>(&{member}), {_to_cpp_range_literal(k.min_val)}, {_to_cpp_range_literal(k.max_val)}).Module("{_cpp_string(k.module)}");')
        else:
            lines.append(f'    s.emplace_back(k{_cpp_key_ident(k.key)}Name, "{_cpp_string(k.display or k.key)}", ConfigParam::{type_enum[k.type]}, const_cast<{ptr_type}*>(&{member})).Module("{_cpp_string(k.module)}");')
    lines.extend(["", "    return s;", "}", "", "// ── SaveConfig ──", "void SaveConfig(const StylusPipelineMembers& m, std::ostream& out) {"])
    for k in active_keys:
        member = _stylus_member_expr(k.member)
        ident = _cpp_key_ident(k.key)
        if k.type == "bool":
            lines.append(f'    out << k{ident}Name << "=" << ({member} ? "1" : "0") << "\\n";')
        elif k.type == "uint8":
            lines.append(f'    out << k{ident}Name << "=" << static_cast<int>({member}) << "\\n";')
        else:
            lines.append(f'    out << k{ident}Name << "=" << {member} << "\\n";')
    lines.extend(["}", "", "// ── LoadConfig ──", "void LoadConfig(StylusPipelineMembers& m, const std::string& key, const std::string& value) {", "    auto toBool = [&](const std::string& v) { return ParseConfigBool(key, v); };", "    try {"])
    for i, k in enumerate(active_keys):
        prefix = "if" if i == 0 else "else if"
        member = _stylus_member_expr(k.member)
        ident = _cpp_key_ident(k.key)
        lines.append(f'        {prefix} (key == k{ident}Name) {{')
        if k.type == "bool":
            lines.append(f"            {member} = toBool(value);")
            if k.key in reset_on_false:
                lines.append(f"            if (!{member}) {{ {reset_on_false[k.key]} }}")
        else:
            parse_expr = "ParseConfigFloat(key, value)" if k.type == "float" else "ParseConfigInt(key, value)"
            if k.min_val is not None or k.max_val is not None:
                lines.append(f"            int v = {parse_expr};")
                if k.min_val is not None:
                    lines.append(f"            if (v < {_to_cpp_range_literal(k.min_val)}) v = {_to_cpp_range_literal(k.min_val)};")
                if k.max_val is not None:
                    lines.append(f"            if (v > {_to_cpp_range_literal(k.max_val)}) v = {_to_cpp_range_literal(k.max_val)};")
                if k.type in ("uint8", "uint16"):
                    lines.append(f"            {member} = static_cast<{k.cpp_type}>(v);")
                else:
                    lines.append(f"            {member} = v;")
            else:
                lines.append(f"            {member} = {parse_expr};")
        lines.append("        }")
    lines.extend([
        "    } catch (const ConfigParseError& error) {",
        '        LogConfigParseWarning("StylusPipeline", __func__, key, value, error);',
        "    }",
        "}",
        "",
        "}  // namespace StylusConfig",
        "}  // namespace Solvers",
        "",
        "#endif  // EGOTOUCH_CONFIG_ENABLED",
        "",
    ])
    return "\n".join(lines)

def _generate_unsupported_config_source(module_name: str) -> str:
    ns = "TouchConfig" if module_name == "touch_pipeline" else "StylusConfig"
    header = f"{module_name.title().replace('_', '')}ConfigKeys.h"
    return "\n".join([
        "// Auto-generated by scripts/config_key_sync.py — DO NOT EDIT",
        "// Source: config/{module}_config.yaml".format(module=module_name),
        "",
        f'#include "{header}"',
        '#include "SolverBuildConfig.h"',
        "",
        "#if EGOTOUCH_CONFIG_ENABLED",
        "namespace Solvers {",
        f"namespace {ns} {{",
        "void LoadConfig(const std::string&, const std::string&) {}",
        "void SaveConfig(std::ostream&) {}",
        "std::vector<ConfigParam> GetConfigSchema() { return {}; }",
        f"}}  // namespace {ns}",
        "}  // namespace Solvers",
        "#endif  // EGOTOUCH_CONFIG_ENABLED",
        "",
    ])


def _touch_member_expr(member: str) -> str:
    root_map = {
        "m_frameParser": "frameParser",
        "m_baseline": "baseline",
        "m_cmf": "cmf",
        "m_macroZoneDet": "macroZoneDet",
        "m_peakDet": "peakDet",
        "m_touchClassifier": "touchClassifier",
        "m_contactExtractor": "contactExtractor",
        "m_edgeComp": "edgeComp",
        "m_edgeReject": "edgeReject",
        "m_stylusSuppress": "stylusSuppress",
        "m_tracker": "tracker",
        "m_coordFilter": "coordFilter",
        "m_gesture": "gesture",
    }
    root, sep, rest = member.partition('.')
    if root not in root_map:
        raise ValueError(f"unsupported touch member root: {member}")
    return f"m.{root_map[root]}->{rest}" if sep else f"*m.{root_map[root]}"


def _cpp_key_ident(key: str) -> str:
    return "".join(part[:1].upper() + part[1:] for part in re.split(r"[^0-9A-Za-z]+", key) if part)


def _cpp_string(value: str) -> str:
    return value.replace('\\', '\\\\').replace('"', '\\"')


def _to_cpp_range_literal(value: str) -> str:
    return value if value.endswith('f') else value


def _to_cpp_literal(value: str, type_str: str) -> str:
    """将 YAML 值转为 C++ 字面量。"""
    if type_str == "bool":
        v = value.lower().strip()
        return "true" if v in ("true", "1", "yes") else "false"
    elif type_str == "float":
        return value.rstrip('f') + 'f'
    elif type_str in ("uint8", "uint16"):
        return value
    else:
        return value


# ═══════════════════════════════════════════════════════════════════════
# 命令实现
# ═══════════════════════════════════════════════════════════════════════

def cmd_diff(args: argparse.Namespace) -> int:
    """报告 YAML 与 C++ 之间的差异, 不修改任何文件。"""
    exit_code = 0

    for name, cfg in MANAGED_FILES.items():
        yaml_path = REPO_ROOT / cfg["yaml"]
        cpp_paths = [REPO_ROOT / p for p in cfg["sources"]]
        cpp_paths.append(REPO_ROOT / cfg["generated_header"])
        cpp_paths.append(REPO_ROOT / cfg["generated_source"])

        yaml_keys = parse_yaml_config(yaml_path)
        cpp_keys = parse_cpp_extract_keys(cpp_paths)

        diffs = diff_keys(yaml_keys, cpp_keys)
        issues = {k: v for k, v in diffs.items() if not v.is_ok}

        if not issues:
            print(f"[{name}] OK — YAML 与 C++ 一致 ({len(diffs)} 键)")
            continue

        exit_code = 1
        print(f"\n[{name}] 发现 {len(issues)} 个差异:")
        for key_name, diff in issues.items():
            print(f"  [{key_name}]")
            for issue in diff.issues:
                print(f"    ! {issue}")

    return exit_code


def cmd_generate(args: argparse.Namespace) -> int:
    """从 YAML 生成 C++ ConfigKeys 文件。"""
    for name, cfg in MANAGED_FILES.items():
        if args.module and args.module != name:
            continue

        yaml_path = REPO_ROOT / cfg["yaml"]
        if not yaml_path.exists():
            print(f"[{name}] YAML 文件不存在: {yaml_path} — 跳过")
            continue

        yaml_keys = parse_yaml_config(yaml_path)
        if not yaml_keys:
            print(f"[{name}] YAML 中无键定义 — 跳过")
            continue

        header_path = REPO_ROOT / cfg["generated_header"]
        source_path = REPO_ROOT / cfg["generated_source"]

        header_content = generate_config_header(yaml_keys, name)
        source_content = generate_config_source(yaml_keys, name)

        if not args.dry_run:
            header_path.parent.mkdir(parents=True, exist_ok=True)
            source_path.parent.mkdir(parents=True, exist_ok=True)
            header_path.write_text(header_content, encoding="utf-8")
            source_path.write_text(source_content, encoding="utf-8")
            print(f"[{name}] 已生成: {header_path}")
            print(f"[{name}] 已生成: {source_path}")
        else:
            print(f"[{name}] (dry-run) 将生成 {header_path} ({len(header_content)} bytes)")
            print(f"[{name}] (dry-run) 将生成 {source_path} ({len(source_content)} bytes)")

    return 0


def cmd_update_yaml(args: argparse.Namespace) -> int:
    """从 C++ 代码反向更新 YAML 文件。

    策略:
      - C++ 中存在的键而 YAML 中没有 → 新增到 YAML (default 从 constexpr 提取)
      - YAML 中有而 C++ 中没有 → 标记为 "orphan" (不删除, 加注释)
      - C++ 中冻结的键 → YAML 中 release 改为 frozen
      - 默认值以 C++ constexpr 为准覆盖 YAML
    """
    for name, cfg in MANAGED_FILES.items():
        if args.module and args.module != name:
            continue

        yaml_path = REPO_ROOT / cfg["yaml"]
        cpp_paths = [REPO_ROOT / p for p in cfg["sources"]]
        cpp_paths.append(REPO_ROOT / cfg["generated_header"])
        cpp_paths.append(REPO_ROOT / cfg["generated_source"])

        yaml_keys = parse_yaml_config(yaml_path)
        cpp_keys = parse_cpp_extract_keys(cpp_paths)

        yaml_by_key = {k.key: k for k in yaml_keys}

        merged: list[ConfigKeyDef] = []

        # ── 处理 C++ 中出现的所有键 ──
        for key_name, cd in sorted(cpp_keys.items()):
            yd = yaml_by_key.pop(key_name, None)

            if yd:
                # 已有: 更新
                yd.in_cpp_schema = cd.in_cpp_schema
                yd.in_cpp_load = cd.in_cpp_load
                yd.in_cpp_save = cd.in_cpp_save
                yd.in_cpp_frozen = cd.in_cpp_frozen
                if cd.default_cpp is not None:
                    yd.default_cpp = cd.default_cpp
                if cd.type and not yd.type:
                    yd.type = cd.type
                if cd.in_cpp_frozen:
                    yd.release = "frozen"
                merged.append(yd)
            else:
                # C++ 中有但 YAML 中没有: 创建最小条目
                new_k = ConfigKeyDef(
                    key=key_name,
                    display=cd.display or key_name,
                    type=cd.type or "int",
                    member="",  # 无从推断
                    default_cpp=cd.default_cpp,
                    module="",
                    release="frozen" if cd.in_cpp_frozen else "active",
                )
                new_k.in_yaml = False  # 标记为新增
                new_k.in_cpp_schema = cd.in_cpp_schema
                new_k.in_cpp_load = cd.in_cpp_load
                new_k.in_cpp_save = cd.in_cpp_save
                new_k.in_cpp_frozen = cd.in_cpp_frozen
                merged.append(new_k)
                print(f"[{name}] + 新增键: {key_name} (C++ 中存在但 YAML 缺失)")

        # ── YAML 中剩余但 C++ 中不存在的键: 保留但标记 ──
        for key_name, yd in sorted(yaml_by_key.items()):
            yd.description = f"[ORPHAN] C++ code not found — {yd.description or ''}"
            merged.append(yd)
            print(f"[{name}] ? 孤立键: {key_name} (YAML 中存在但 C++ 中未找到)")

        if not args.dry_run:
            write_yaml_config(yaml_path, merged)
            print(f"[{name}] 已更新: {yaml_path} ({len(merged)} 键)")
        else:
            print(f"[{name}] (dry-run) 将更新 {yaml_path} ({len(merged)} 键)")

    return 0


def cmd_check(args: argparse.Namespace) -> int:
    """CI 模式: 检查一致性, 不一致时退出非零。"""
    return cmd_diff(args)


# ═══════════════════════════════════════════════════════════════════════
# 入口
# ═══════════════════════════════════════════════════════════════════════

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Config Key 双向同步脚本 — YAML ↔ C++",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python scripts/config_key_sync.py check
  python scripts/config_key_sync.py diff
  python scripts/config_key_sync.py generate --dry-run
  python scripts/config_key_sync.py generate
  python scripts/config_key_sync.py update-yaml --module touch_pipeline
        """,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("check", help="CI 模式: 检查一致性, 不一致退出非零")
    sub.add_parser("diff", help="报告 YAML 与 C++ 的差异 (不修改文件)")

    gen = sub.add_parser("generate", help="正向: 从 YAML 生成 C++ ConfigKeys 文件")
    gen.add_argument("--module", help="仅处理指定模块 (touch_pipeline | stylus_pipeline)")
    gen.add_argument("--dry-run", action="store_true", help="仅预览, 不写入文件")

    upd = sub.add_parser("update-yaml", help="反向: 从 C++ 代码更新 YAML (以代码为准)")
    upd.add_argument("--module", help="仅处理指定模块")
    upd.add_argument("--dry-run", action="store_true", help="仅预览, 不写入文件")

    args = parser.parse_args()

    handlers = {
        "check": cmd_check,
        "diff": cmd_diff,
        "generate": cmd_generate,
        "update-yaml": cmd_update_yaml,
    }

    return handlers[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
