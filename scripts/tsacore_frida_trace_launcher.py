#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
import signal
import struct
import sys
import time
from typing import Any, Iterable

try:
    import frida
except ImportError as exc:  # pragma: no cover
    raise SystemExit(
        "frida-python 未安装。先执行: pip install frida"
    ) from exc


def load_agent_source(agent_path: pathlib.Path, config: dict[str, Any]) -> str:
    source = agent_path.read_text(encoding="utf-8")
    injected = (
        "globalThis.__TSACORE_TRACE_CONFIG__ = "
        + json.dumps(config, ensure_ascii=False, indent=2)
        + ";\n"
    )
    return injected + source


def on_message(message: dict[str, Any], data: bytes | None) -> None:
    mtype = message.get("type")
    if mtype == "send":
        payload = message.get("payload")
        print("[send]", json.dumps(payload, ensure_ascii=False), flush=True)
    elif mtype == "error":
        print("[error]", message.get("stack") or json.dumps(message, ensure_ascii=False), file=sys.stderr, flush=True)
    else:
        print("[message]", json.dumps(message, ensure_ascii=False), flush=True)


def _u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def _u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def parse_pe_exports(dll_path: pathlib.Path) -> list[dict[str, Any]]:
    data = dll_path.read_bytes()
    if data[:2] != b"MZ":
        raise ValueError("Not a PE file")

    pe_off = _u32(data, 0x3C)
    if data[pe_off:pe_off + 4] != b"PE\0\0":
        raise ValueError("Invalid PE signature")

    file_header_off = pe_off + 4
    opt_off = file_header_off + 20
    magic = _u16(data, opt_off)
    if magic == 0x20B:
        data_dir_off = opt_off + 0x70
    elif magic == 0x10B:
        data_dir_off = opt_off + 0x60
    else:
        raise ValueError(f"Unsupported optional header magic: 0x{magic:04X}")

    export_rva = _u32(data, data_dir_off)
    export_size = _u32(data, data_dir_off + 4)
    if export_rva == 0 or export_size == 0:
        return []

    sections_off = opt_off + _u16(data, file_header_off + 16)
    section_count = _u16(data, file_header_off + 2)

    sections: list[dict[str, int]] = []
    for i in range(section_count):
        off = sections_off + i * 40
        virtual_size = _u32(data, off + 8)
        virtual_address = _u32(data, off + 12)
        raw_size = _u32(data, off + 16)
        raw_ptr = _u32(data, off + 20)
        sections.append({
            "va": virtual_address,
            "vs": virtual_size,
            "raw_size": raw_size,
            "raw_ptr": raw_ptr,
        })

    def rva_to_off(rva: int) -> int:
        for s in sections:
            start = s["va"]
            size = max(s["vs"], s["raw_size"])
            end = start + size
            if start <= rva < end:
                return s["raw_ptr"] + (rva - start)
        raise ValueError(f"RVA 0x{rva:X} not found in sections")

    exp_off = rva_to_off(export_rva)
    base_ordinal = _u32(data, exp_off + 0x10)
    number_of_functions = _u32(data, exp_off + 0x14)
    number_of_names = _u32(data, exp_off + 0x18)
    address_of_functions = _u32(data, exp_off + 0x1C)
    address_of_names = _u32(data, exp_off + 0x20)
    address_of_name_ordinals = _u32(data, exp_off + 0x24)

    names_by_index: dict[int, str] = {}
    for i in range(number_of_names):
        name_rva = _u32(data, rva_to_off(address_of_names) + i * 4)
        ord_index = _u16(data, rva_to_off(address_of_name_ordinals) + i * 2)
        name_off = rva_to_off(name_rva)
        end = data.index(b"\0", name_off)
        names_by_index[ord_index] = data[name_off:end].decode("utf-8", errors="replace")

    exports: list[dict[str, Any]] = []
    for i in range(number_of_functions):
        func_rva = _u32(data, rva_to_off(address_of_functions) + i * 4)
        if func_rva == 0:
            continue
        ordinal = base_ordinal + i
        exports.append({
            "ordinal": ordinal,
            "ordinalName": f"Ordinal_{ordinal}",
            "name": names_by_index.get(i),
            "rva": func_rva,
        })
    return exports


def select_exports(raw_exports: Iterable[str], parsed_exports: list[dict[str, Any]]) -> list[dict[str, Any]]:
    by_name = {e["name"]: e for e in parsed_exports if e.get("name")}
    by_ordinal = {e["ordinalName"]: e for e in parsed_exports}
    selected: list[dict[str, Any]] = []

    for item in raw_exports:
        if item in by_name:
            matched = dict(by_name[item])
            matched["requested"] = item
            selected.append(matched)
            continue
        if item in by_ordinal:
            matched = dict(by_ordinal[item])
            matched["requested"] = item
            selected.append(matched)
            continue
        selected.append({"requested": item, "missing": True})

    return selected


def build_config(args: argparse.Namespace) -> dict[str, Any]:
    log_path = pathlib.Path(args.log).resolve()
    log_path.parent.mkdir(parents=True, exist_ok=True)

    config: dict[str, Any] = {
        "moduleName": args.module,
        "logPath": str(log_path),
        "pollIntervalMs": args.poll_ms,
        "flushEveryLines": args.flush_lines,
        "periodicFlushMs": args.flush_ms,
        "snapshotOnEnter": args.snapshot_on_enter,
        "snapshotOnLeave": not args.no_snapshot_on_leave,
        "includeRawGlobals": not args.no_raw_globals,
        "captureBacktrace": args.backtrace,
    }

    raw_requested_exports = args.exports or []
    if raw_requested_exports:
        dll_path = pathlib.Path(r"C:\Program Files\Huawei\HuaweiThpService") / args.module
        parsed_exports = parse_pe_exports(dll_path)
        config["resolvedExports"] = select_exports(raw_requested_exports, parsed_exports)
        config["enabledExports"] = list(raw_requested_exports)
    elif args.exports:
        config["enabledExports"] = args.exports

    getter_names = [
        "ASA_AnimationState",
        "ASA_GetRptInRange",
        "ASA_GetRptInk",
        "ASA_GetRptPressure",
        "ASA_GetRptXPos",
        "ASA_GetRptYPos",
        "ASA_GetRptButtonStatus",
        "ASA_GetEnterNoPressInkThold",
        "ASA_GetExitNoPressInkThold",
        "ASA_GetTX1Siganl",
        "ASA_GetTX1SiganlX",
        "ASA_GetTX1SiganlY",
        "ASA_GetTX2Siganl",
        "ASA_GetTX2SiganlX",
        "ASA_GetTX2SiganlY",
        "ASA_GetGridTiedOriTx1Ptr",
        "ASA_GetGridTiedOriTx2Ptr",
        "ASA_GetGridTiedDeTraceNoiseTx1Ptr",
        "ASA_GetGridTiedDeTraceNoiseTx2Ptr",
    ]
    try:
        dll_path = pathlib.Path(r"C:\Program Files\Huawei\HuaweiThpService") / args.module
        parsed_exports = parse_pe_exports(dll_path)
        config["resolvedGetterExports"] = select_exports(getter_names, parsed_exports)
    except Exception:
        config["resolvedGetterExports"] = []

    config["dumpGrids"] = bool(args.dump_grids)

    if args.backtrace_exports:
        config["backtraceExports"] = args.backtrace_exports

    return config


def main() -> int:
    repo_root = pathlib.Path(__file__).resolve().parents[1]
    default_agent = repo_root / "scripts" / "tsacore_frida_trace.js"

    parser = argparse.ArgumentParser(
        description="Attach Frida to a running process and load TSACore export-only trace hooks."
    )
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--pid", type=int, help="目标进程 PID")
    target.add_argument("--process", help="目标进程名，做前缀匹配，例如 HuaweiThpService")

    parser.add_argument("--module", default="TSACore.dll", help="要等待/监控的模块名")
    parser.add_argument("--agent", default=str(default_agent), help="Frida JS agent 路径")
    parser.add_argument("--log", default="./tsacore_trace.jsonl", help="输出 JSONL 日志路径")

    parser.add_argument("--poll-ms", type=int, default=500, help="等待模块加载的轮询间隔")
    parser.add_argument("--flush-lines", type=int, default=20, help="累计多少行后 flush")
    parser.add_argument("--flush-ms", type=int, default=1000, help="周期性 flush 间隔")

    parser.add_argument("--snapshot-on-enter", action="store_true", help="进入函数时也抓状态")
    parser.add_argument("--no-snapshot-on-leave", action="store_true", help="离开函数时不抓状态")
    parser.add_argument("--no-raw-globals", action="store_true", help="不读取 data RVA 全局变量")
    parser.add_argument("--backtrace", action="store_true", help="对 backtraceExports 抓调用栈")

    parser.add_argument(
        "--exports",
        nargs="+",
        help="覆盖默认导出 hook 列表，例如 --exports TSA_ASAProcess NoPressInkProcess"
    )
    parser.add_argument(
        "--backtrace-exports",
        nargs="+",
        help="覆盖默认 backtrace 导出列表"
    )
    parser.add_argument(
        "--dump-grids",
        action="store_true",
        help="同时导出 TX1/TX2 原始图指针和内容快照"
    )

    args = parser.parse_args()

    agent_path = pathlib.Path(args.agent).resolve()
    if not agent_path.exists():
        raise SystemExit(f"Agent 不存在: {agent_path}")

    config = build_config(args)
    source = load_agent_source(agent_path, config)

    device = frida.get_local_device()

    pid: int
    if args.pid is not None:
        pid = args.pid
    else:
        process_name = args.process.lower()
        matches = [p for p in device.enumerate_processes() if p.name.lower().startswith(process_name)]
        if not matches:
            raise SystemExit(f"未找到进程前缀匹配: {args.process}")
        if len(matches) > 1:
            names = ", ".join(f"{p.name}({p.pid})" for p in matches)
            raise SystemExit(f"匹配到多个进程，请改用 --pid: {names}")
        pid = matches[0].pid

    print(f"[*] Attaching to pid={pid}", flush=True)
    session = device.attach(pid)

    def _cleanup(signum: int, frame: Any) -> None:
        print(f"\n[*] Received signal {signum}, detaching...", flush=True)
        try:
            session.detach()
        finally:
            raise SystemExit(0)

    signal.signal(signal.SIGINT, _cleanup)
    if hasattr(signal, "SIGTERM"):
        signal.signal(signal.SIGTERM, _cleanup)

    script = session.create_script(source)
    script.on("message", on_message)
    script.load()

    print("[*] Script loaded.", flush=True)
    print(f"[*] Log file: {pathlib.Path(config['logPath']).resolve()}", flush=True)
    print("[*] Press Ctrl+C to stop.", flush=True)

    while True:
        time.sleep(0.5)


if __name__ == "__main__":
    raise SystemExit(main())
