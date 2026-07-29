# ANM Animation Format

Used by Captive DOS for intro cutscene and in-game animations.

## File structure

| Offset | Size | Description |
|--------|------|-------------|
| 0 | 768 | VGA palette (256 colors × 3 bytes, 6-bit RGB) |
| 768 | 2 | `cmd_end` — little-endian word, byte offset past the commands section |
| 770 | varies | Commands data (purpose TBD, possibly animation scripting) |
| cmd_end | varies | Frames stored backwards from end of file |

## Palette

768 bytes: 256 entries of (R, G, B), each component 0-63 (6-bit VGA). Convert to 8-bit by shifting left 2. The first 32 entries match the PL5 game palette.

## Frame storage

Frames are stored from the **end of the file backwards**. Each frame is followed by a **little-endian 16-bit word** at the end that gives the total size **including the 2-byte size field itself**.

To enumerate frames (working backwards from EOF):

```
pos = file_size
while pos > cmd_end:
    size_word = LE16(data[pos-2 .. pos-1])
    packed_data_size = size_word - 2
    frame_data = data[pos - 2 - packed_data_size .. pos - 2]
    pos = pos - 2 - packed_data_size
```

## Frame decoding (XOR delta)

Each frame is decoded against a 64000-byte frame buffer (320×200, chunky 8-bit). Frame 0 starts from a zeroed buffer; each subsequent frame XORs against the previous.

```
pos = 0  (position in frame buffer)
si = 0   (position in packed data)

while si < packed_size and pos < 64000:
    cmd = packed[si++]
    if cmd != 0:
        frame[pos] ^= cmd
        pos++
    else:
        skip = packed[si++]
        if skip == 0: break  (end of frame)
        pos += skip
```

## Test data

| File | Size | Frames | Content |
|------|------|--------|---------|
| TEST0.ANM | 2472 | 1 | "Courtroom 101, 2127ad." |
| TEST9.ANM | varies | 43 | Space station animation |
| TEST10.ANM | varies | 18 | Multiple scenes |
| TEST13.ANM | varies | 23 | Cryo-pod sequence |

## Reference

Reverse-engineered from CaptiveTools.jar (`ru.old_games.captive.anim.AnimationParser`). Key discovery: all size words are **little-endian** (not big-endian as initially assumed from Java's DataInputStream).
