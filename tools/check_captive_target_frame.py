#!/usr/bin/env python3
"""Check the real CAPPO VGA target marker in a rendered native frame."""

from pathlib import Path
import sys


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} FRAME.ppm", file=sys.stderr)
        return 2
    data = Path(sys.argv[1]).read_bytes()
    try:
        header, pixels = data.split(b"\n255\n", 1)
    except ValueError:
        print("invalid PPM frame", file=sys.stderr)
        return 1
    if header != b"P6\n320 200" or len(pixels) != 320 * 200 * 3:
        print("unexpected native frame format", file=sys.stderr)
        return 1

    def pixel(x: int, y: int) -> bytes:
        offset = (y * 320 + x) * 3
        return pixels[offset:offset + 3]

    # Measured from the supplied authentic CAPPO Mission 0001 VGA state.
    # The real green marker is directly beneath the six-pixel magenta cursor.
    green = bytes((48, 195, 48))
    green_points = ((103, 110), (104, 110), (103, 111), (104, 111))
    if any(pixel(x, y) != green for x, y in green_points):
        print("authentic CAPPO frame did not show the real green target",
              file=sys.stderr)
        return 1

    magenta = bytes((243, 0, 243))
    cursor_points = ((104, 109), (102, 111), (105, 111), (106, 111),
                     (104, 112), (104, 113))
    if any(pixel(x, y) != magenta for x, y in cursor_points):
        print("authentic CAPPO frame did not show the real cursor over target",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
