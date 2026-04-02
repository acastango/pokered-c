#!/usr/bin/env python3
"""Generate src/data/trainer_data.c from pokered-master trainer ASM.

Source of truth:
  - pokered-master/data/trainers/parties.asm
  - pokered-master/data/trainers/pic_pointers_money.asm

Usage:
  python tools/extract_trainers.py
"""

from __future__ import annotations

import re
from pathlib import Path

SRC = Path(r"C:\Users\Anthony\pokered\pokered-master")
OUT = Path(r"C:\Users\Anthony\pokered\src\data")

PARTIES_ASM = SRC / "data" / "trainers" / "parties.asm"
MONEY_ASM = SRC / "data" / "trainers" / "pic_pointers_money.asm"
OUT_C = OUT / "trainer_data.c"


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_pointer_order(text: str) -> list[str]:
    names: list[str] = []
    in_table = False
    for raw_line in text.splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        if line.startswith("TrainerDataPointers:"):
            in_table = True
            continue
        if not in_table:
            continue
        if line.startswith("assert_table_length"):
            break
        match = re.match(r"dw\s+([A-Za-z0-9_]+)\s*$", line)
        if match:
            names.append(match.group(1))
    if not names:
        raise ValueError("Failed to parse TrainerDataPointers from parties.asm")
    return names


def split_sections(text: str) -> dict[str, list[str]]:
    sections: dict[str, list[str]] = {}
    current_label: str | None = None
    current_lines: list[str] = []

    for line in text.splitlines():
        match = re.match(r"^([A-Za-z0-9_]+Data):\s*$", line)
        if match:
            if current_label is not None:
                sections[current_label] = current_lines
            current_label = match.group(1)
            current_lines = []
            continue
        if current_label is not None:
            current_lines.append(line)

    if current_label is not None:
        sections[current_label] = current_lines
    return sections


def parse_party_sections(text: str, order: list[str]) -> dict[str, list[tuple[str | None, list[str]]]]:
    sections = split_sections(text)
    parsed: dict[str, list[tuple[str | None, list[str]]]] = {}

    for label in order:
        raw_lines = sections.get(label)
        if raw_lines is None:
            raise ValueError(f"Missing trainer data section: {label}")

        parties: list[tuple[str | None, list[str]]] = []
        pending_comment: str | None = None
        pending_stmt: str | None = None

        def flush_stmt() -> None:
            nonlocal pending_stmt, pending_comment
            if pending_stmt is None:
                return
            stmt = pending_stmt.strip()
            if not stmt.startswith("db "):
                raise ValueError(f"Unexpected trainer statement in {label}: {stmt}")
            payload = stmt[3:].strip()
            tokens = [token.strip() for token in payload.split(",") if token.strip()]
            if not tokens:
                raise ValueError(f"Empty db statement in {label}")
            parties.append((pending_comment, tokens))
            pending_comment = None
            pending_stmt = None

        for raw_line in raw_lines:
            stripped = raw_line.strip()
            if not stripped:
                flush_stmt()
                continue
            if stripped.startswith(";"):
                flush_stmt()
                pending_comment = stripped[1:].strip()
                continue
            line_no_comment = raw_line.split(";", 1)[0].rstrip()
            stripped_no_comment = line_no_comment.strip()
            if not stripped_no_comment:
                flush_stmt()
                continue
            if stripped_no_comment.startswith("db "):
                flush_stmt()
                pending_stmt = stripped_no_comment
                continue
            if pending_stmt is None:
                raise ValueError(f"Unexpected continuation in {label}: {raw_line}")
            pending_stmt += " " + stripped_no_comment

        flush_stmt()
        parsed[label] = parties

    return parsed


def parse_base_money(text: str) -> list[int]:
    values: list[int] = []
    in_table = False
    for raw_line in text.splitlines():
        line = raw_line.split(";", 1)[0].strip()
        if not line:
            continue
        if line.startswith("TrainerPicAndMoneyPointers::"):
            in_table = True
            continue
        if not in_table:
            continue
        if line.startswith("assert_table_length"):
            break
        match = re.match(r"pic_money\s+[A-Za-z0-9_]+,\s*([0-9]+)\s*$", line)
        if match:
            values.append(int(match.group(1)))
    if not values:
        raise ValueError("Failed to parse TrainerPicAndMoneyPointers from pic_pointers_money.asm")
    return values


def to_c_token(token: str) -> str:
    token = token.strip()
    if token.startswith("$"):
        value = int(token[1:], 16)
        if value == 0xFF:
            return "TRAINER_PARTY_FMT_B"
        return str(value)
    if re.fullmatch(r"[0-9]+", token):
        return token
    return f"SPECIES_{token}"


def format_bytes(tokens: list[str], indent: str = "    ", width: int = 96) -> list[str]:
    lines: list[str] = []
    current = indent
    for i, token in enumerate(tokens):
        piece = f"{token}," if i == len(tokens) - 1 else f"{token}, "
        if current != indent and len(current) + len(piece) > width:
            lines.append(current.rstrip())
            current = indent + piece
        else:
            current += piece
    lines.append(current.rstrip())
    return lines


def emit_trainer_array(label: str, parties: list[tuple[str | None, list[str]]], class_no: int) -> list[str]:
    symbol = label.removesuffix("Data")
    lines = [f"/* ---- {symbol} (class {class_no}) ------------------------------------------ */"]
    lines.append(f"static const uint8_t g{symbol}Parties[] = {{")

    if not parties:
        lines.append("    0,")
        lines.append("};")
        lines.append("")
        return lines

    for comment, tokens in parties:
        if comment:
            lines.append(f"    /* {comment} */")
        for entry in format_bytes([to_c_token(token) for token in tokens]):
            lines.append(entry)
    lines.append("};")
    lines.append("")
    return lines


def build_output(order: list[str], parties: dict[str, list[tuple[str | None, list[str]]]], money: list[int]) -> str:
    if len(money) != len(order):
        raise ValueError(
            f"Trainer class count mismatch: {len(order)} party classes vs {len(money)} money entries"
        )

    lines = [
        "/* trainer_data.c — Generated trainer party data.",
        " *",
        " * Source of truth:",
        " *   - pokered-master/data/trainers/parties.asm",
        " *   - pokered-master/data/trainers/pic_pointers_money.asm",
        " *",
        " * DO NOT EDIT THIS FILE MANUALLY.",
        " * Regenerate with: python tools/extract_trainers.py",
        " */",
        '#include "trainer_data.h"',
        '#include "game/constants.h"',
        "",
    ]

    for class_no, label in enumerate(order, start=1):
        lines.extend(emit_trainer_array(label, parties[label], class_no))

    lines.append("const uint8_t * const gTrainerPartyData[NUM_TRAINERS] = {")
    for class_no, label in enumerate(order, start=1):
        symbol = label.removesuffix("Data")
        lines.append(f"    g{symbol}Parties, /* class {class_no} */")
    lines.append("};")
    lines.append("")

    lines.append("const uint16_t gTrainerBaseMoney[NUM_TRAINERS] = {")
    for class_no, value in enumerate(money, start=1):
        lines.append(f"    {value}, /* class {class_no} */")
    lines.append("};")
    lines.append("")

    return "\n".join(lines)


def main() -> None:
    parties_text = read_text(PARTIES_ASM)
    money_text = read_text(MONEY_ASM)

    order = parse_pointer_order(parties_text)
    parties = parse_party_sections(parties_text, order)
    money = parse_base_money(money_text)

    content = build_output(order, parties, money)
    OUT_C.write_text(content, encoding="utf-8")
    print(f"Wrote {OUT_C}")
    print(f"  classes: {len(order)}")
    print(f"  money entries: {len(money)}")


if __name__ == "__main__":
    main()
