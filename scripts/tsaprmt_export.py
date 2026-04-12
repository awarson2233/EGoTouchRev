#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

import pefile


DEFAULT_DLL_PATH = Path(r"C:\Program Files\Huawei\HuaweiThpService\TSAPrmt.dll")
DEFAULT_OFFSET = 0xA30
DEFAULT_LENGTH = 0x10
EXIT_WORDS = {"q", "quit", "exit"}


def format_hex(data: bytes) -> str:
    return " ".join(f"{byte:02X}" for byte in data)


def format_u16le(data: bytes) -> str:
    values = []
    for index in range(0, len(data) - 1, 2):
        values.append(f"0x{struct.unpack_from('<H', data, index)[0]:04X}")
    if len(data) % 2:
        values.append(f"tail=0x{data[-1]:02X}")
    return ", ".join(values)


class TsaprmtReader:
    def __init__(self, dll_path: Path) -> None:
        self.dll_path = dll_path
        self.pe = pefile.PE(str(dll_path))
        self.image_base = self.pe.OPTIONAL_HEADER.ImageBase
        self.exports = {
            symbol.name.decode(errors="ignore"): symbol.address
            for symbol in getattr(self.pe, "DIRECTORY_ENTRY_EXPORT", []).symbols
            if symbol.name
        }

    def read_symbol_slice(self, symbol_name: str, offset: int, length: int) -> dict[str, object]:
        if symbol_name not in self.exports:
            raise KeyError(symbol_name)

        export_rva = self.exports[symbol_name]
        export_file_offset = self.pe.get_offset_from_rva(export_rva)
        real_va = struct.unpack_from("<Q", self.pe.__data__, export_file_offset)[0]
        real_rva = real_va - self.image_base
        target_rva = real_rva + offset
        target_file_offset = self.pe.get_offset_from_rva(target_rva)
        raw = self.pe.__data__[target_file_offset:target_file_offset + length]

        if len(raw) != length:
            raise ValueError(
                f"expected {length} bytes, only got {len(raw)} bytes from file offset 0x{target_file_offset:X}"
            )

        return {
            "symbol_name": symbol_name,
            "export_va": self.image_base + export_rva,
            "real_va": real_va,
            "target_va": self.image_base + target_rva,
            "offset": offset,
            "length": length,
            "bytes": raw,
        }


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Interactively export a byte slice from TSAPrmt symbols."
    )
    parser.add_argument(
        "--dll",
        type=Path,
        default=DEFAULT_DLL_PATH,
        help=f"TSAPrmt.dll path. Default: {DEFAULT_DLL_PATH}",
    )
    parser.add_argument(
        "--offset",
        type=lambda value: int(value, 0),
        default=DEFAULT_OFFSET,
        help=f"Offset inside the real parameter block. Default: 0x{DEFAULT_OFFSET:X}",
    )
    parser.add_argument(
        "--length",
        type=lambda value: int(value, 0),
        default=DEFAULT_LENGTH,
        help=f"Number of bytes to export. Default: 0x{DEFAULT_LENGTH:X}",
    )
    return parser


def print_result(result: dict[str, object]) -> None:
    data = result["bytes"]
    assert isinstance(data, bytes)

    print()
    print(f"symbol    : {result['symbol_name']}")
    print(f"export VA : 0x{result['export_va']:X}")
    print(f"real VA   : 0x{result['real_va']:X}")
    print(f"target VA : 0x{result['target_va']:X}")
    print(f"range     : +0x{result['offset']:X} .. +0x{result['offset'] + result['length'] - 1:X}")
    print(f"bytes     : {format_hex(data)}")
    print(f"u16le     : {format_u16le(data)}")


def main() -> int:
    args = build_parser().parse_args()

    if not args.dll.exists():
        print(f"DLL not found: {args.dll}", file=sys.stderr)
        return 1

    try:
        reader = TsaprmtReader(args.dll)
    except Exception as exc:
        print(f"Failed to open PE file: {args.dll}", file=sys.stderr)
        print(exc, file=sys.stderr)
        return 1

    print(f"DLL       : {args.dll}")
    print(f"offset    : 0x{args.offset:X}")
    print(f"length    : 0x{args.length:X}")
    print("input name : enter a symbol name, or type 'q' / 'quit' / 'exit' to leave")

    while True:
        try:
            raw_name = input("> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            return 0

        if not raw_name:
            continue
        if raw_name.lower() in EXIT_WORDS:
            return 0

        try:
            result = reader.read_symbol_slice(raw_name, args.offset, args.length)
        except KeyError:
            print(f"symbol not found: {raw_name}")
            continue
        except Exception as exc:
            print(f"read failed: {exc}")
            continue

        print_result(result)


if __name__ == "__main__":
    raise SystemExit(main())
