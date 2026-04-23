#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import pathlib
from typing import Any

STAGE_NAMES = {
    "Ordinal_839": "HPP3_PressureProcess",
    "Ordinal_939": "NoPressInkProcess",
    "Ordinal_838": "HPP3_PostPressureProcess",
}

EXPECTED_ORDER = ["Ordinal_839", "Ordinal_939", "Ordinal_838"]


def load_events(path: pathlib.Path) -> list[dict[str, Any]]:
    events: list[dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                events.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    return events


def simplify_event(ev: dict[str, Any]) -> dict[str, Any]:
    state = ev.get("state") or {}
    exports = state.get("exports") or {}
    return {
        "eventId": ev.get("eventId"),
        "hostTsMs": ev.get("hostTsMs"),
        "tid": ev.get("tid"),
        "exportName": ev.get("exportName"),
        "stage": STAGE_NAMES.get(ev.get("exportName"), ev.get("exportName")),
        "tx1": exports.get("tx1Combined"),
        "tx2": exports.get("tx2Combined"),
        "tx1X": exports.get("tx1X"),
        "tx1Y": exports.get("tx1Y"),
        "tx2X": exports.get("tx2X"),
        "tx2Y": exports.get("tx2Y"),
        "inRange": exports.get("rptInRange"),
        "ink": exports.get("rptInk"),
        "pressure": exports.get("rptPressure"),
        "enterNoPressTh": exports.get("enterNoPressTh"),
        "exitNoPressTh": exports.get("exitNoPressTh"),
        "anim": exports.get("animationState"),
        "durationMs": ev.get("durationMs"),
    }


def collect_stage_triples(events: list[dict[str, Any]]) -> list[list[dict[str, Any]]]:
    leaves = [
        ev for ev in events
        if ev.get("kind") == "call" and ev.get("phase") == "leave" and ev.get("exportName") in EXPECTED_ORDER
    ]

    triples: list[list[dict[str, Any]]] = []
    i = 0
    while i + 2 < len(leaves):
        chunk = leaves[i:i + 3]
        names = [c.get("exportName") for c in chunk]
        if names == EXPECTED_ORDER:
            triples.append([simplify_event(c) for c in chunk])
            i += 3
        else:
            i += 1
    return triples


def write_json(path: pathlib.Path, data: Any) -> None:
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def write_csv(path: pathlib.Path, triples: list[list[dict[str, Any]]]) -> None:
    headers = [
        "tripleIndex",
        "tid",
        "stage",
        "eventId",
        "hostTsMs",
        "tx1",
        "tx2",
        "tx1X",
        "tx1Y",
        "tx2X",
        "tx2Y",
        "inRange",
        "ink",
        "pressure",
        "enterNoPressTh",
        "exitNoPressTh",
        "anim",
        "durationMs",
    ]

    lines = [",".join(headers)]
    for idx, triple in enumerate(triples):
        for ev in triple:
            row = [
                idx,
                ev.get("tid"),
                ev.get("stage"),
                ev.get("eventId"),
                ev.get("hostTsMs"),
                ev.get("tx1"),
                ev.get("tx2"),
                ev.get("tx1X"),
                ev.get("tx1Y"),
                ev.get("tx2X"),
                ev.get("tx2Y"),
                ev.get("inRange"),
                ev.get("ink"),
                ev.get("pressure"),
                ev.get("enterNoPressTh"),
                ev.get("exitNoPressTh"),
                ev.get("anim"),
                ev.get("durationMs"),
            ]
            escaped = []
            for v in row:
                s = "" if v is None else str(v)
                if "," in s or '"' in s:
                    s = '"' + s.replace('"', '""') + '"'
                escaped.append(s)
            lines.append(",".join(escaped))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Aggregate TSACore Frida trace into per-frame stage triples.")
    parser.add_argument("input", help="输入 jsonl 日志路径")
    parser.add_argument("--json-out", help="输出聚合 JSON 路径")
    parser.add_argument("--csv-out", help="输出聚合 CSV 路径")
    parser.add_argument("--limit", type=int, default=50, help="终端打印前 N 组 triple")
    args = parser.parse_args()

    input_path = pathlib.Path(args.input).resolve()
    events = load_events(input_path)
    triples = collect_stage_triples(events)

    summary = {
        "input": str(input_path),
        "eventCount": len(events),
        "tripleCount": len(triples),
        "triples": triples,
    }

    if args.json_out:
        write_json(pathlib.Path(args.json_out).resolve(), summary)
    if args.csv_out:
        write_csv(pathlib.Path(args.csv_out).resolve(), triples)

    print(f"input={input_path}")
    print(f"eventCount={len(events)}")
    print(f"tripleCount={len(triples)}")

    for idx, triple in enumerate(triples[: max(0, args.limit)]):
        print(f"\n=== triple {idx} ===")
        for ev in triple:
            print(
                f"{ev['stage']}: tx1={ev['tx1']} tx2={ev['tx2']} "
                f"inRange={ev['inRange']} ink={ev['ink']} pressure={ev['pressure']} "
                f"enterTh={ev['enterNoPressTh']} exitTh={ev['exitNoPressTh']} anim={ev['anim']}"
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
