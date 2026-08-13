#!/usr/bin/env python3
"""Reject a CAPPO transition probe when the original runtime is unchanged."""

from pathlib import Path
import hashlib
import sys


def digest(path: str) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main() -> int:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} BEFORE AFTER ACTION", file=sys.stderr)
        return 2
    before, after, action = sys.argv[1:]
    if digest(before) == digest(after):
        print(f"CAPPO {action} produced no original-runtime state change",
              file=sys.stderr)
        return 1
    print(f"CAPPO {action} changed the original-runtime dump")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
