#!/usr/bin/env python3
"""Validate the source PO catalogs without requiring gettext at build time."""

from __future__ import annotations

import ast
import sys
from pathlib import Path


REQUIRED_MESSAGES = {
    "ARROWS: SELECT  ENTER: START  D: SCAN DATA  F1: CONTROLS  F10: IN-GAME OPTIONS",
    "EN %d",
    "HP %d",
    "R:RENAME  S:SWAP  ENTER:START",
}


def parse_quoted(value: str, path: Path, line_number: int) -> str:
    try:
        parsed = ast.literal_eval(value)
    except (SyntaxError, ValueError) as exc:
        raise ValueError(f"{path}:{line_number}: invalid PO string: {value}") from exc
    if not isinstance(parsed, str):
        raise ValueError(f"{path}:{line_number}: PO string is not text")
    return parsed


def message_ids(path: Path) -> list[str]:
    messages: list[str] = []
    current: str | None = None
    reading_id = False

    def finish() -> None:
        if current is not None:
            messages.append(current)

    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.strip()
        if line.startswith("msgid "):
            finish()
            current = parse_quoted(line[6:].strip(), path, line_number)
            reading_id = True
        elif reading_id and line.startswith('"'):
            if current is None:
                raise ValueError(f"{path}:{line_number}: continuation without msgid")
            current += parse_quoted(line, path, line_number)
        elif reading_id:
            reading_id = False

    finish()
    return messages


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    catalogs = sorted((root / "po").glob("*.po"))
    errors: list[str] = []
    if len(catalogs) != 18:
        errors.append(f"expected 18 translated catalogs, found {len(catalogs)}")

    for catalog in catalogs:
        try:
            ids = message_ids(catalog)
        except (OSError, ValueError) as exc:
            errors.append(str(exc))
            continue
        duplicates = sorted({msgid for msgid in ids if ids.count(msgid) > 1})
        if duplicates:
            errors.append(f"{catalog}: duplicate msgid(s): {duplicates}")
        missing = sorted(REQUIRED_MESSAGES.difference(ids))
        if missing:
            errors.append(f"{catalog}: missing required msgid(s): {missing}")

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1
    print(f"validated {len(catalogs)} gettext catalogs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
