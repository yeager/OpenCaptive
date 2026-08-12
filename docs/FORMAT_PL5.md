# PL5 Graphics Format

> Updated for v1.1.115. Decoder behavior is covered by the release regression suite.

Used by Captive DOS for static images (title screen, ending, shop graphics, etc.).

## File structure

- **Size**: exactly 40000 bytes (no header)
- **Resolution**: 320 × 200 pixels
- **Colors**: 32 (5-bit indexed)
- **Encoding**: custom 5-bit packing

## Decoding algorithm

Every 5 bytes encode 8 pixels. The bit arrangement is non-standard — it is NOT a simple MSB/LSB bitstream.

```c
pixel[0] = src[0] & 0x1F
pixel[1] = src[1] & 0x1F
pixel[2] = (src[0] >> 5) & 0x07 | (src[1] >> 3) & 0x18
pixel[3] = (src[2] << 1 | (src[1] >> 5) & 1) & 0x1F
pixel[4] = src[3] & 0x1F
pixel[5] = (src[2] >> 6) & 0x03 | (src[3] >> 3) & 0x1C
pixel[6] = src[4] & 0x1F
pixel[7] = (src[2] >> 4) & 0x03 | (src[4] >> 3) & 0x1C
```

Total pixels: 320 × 200 = 64000. At 8 pixels per 5 bytes: 64000 / 8 × 5 = 40000 bytes.

## Palette

The palette is NOT stored in PL5 files. It comes from the ANM animation file headers (first 32 entries of the 256-color VGA palette). All ANM files share the same first 32 colors.

## Reference

Reverse-engineered from CaptiveTools.jar (`ru.old_games.captive.pic.Codec.unpackPixels`). Verified with 100% pixel match against reference decoder output.
