#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 AutoTrace contributors
#
# SPDX-License-Identifier: CC0-1.0
"""Regenerate the test images in this directory: python3 mkbmp.py

One 4-colour picture, stored as a 24 bpp reference and in the encodings the
native reader got wrong: RLE8, RLE4, 2 bpp, and 32 bpp BITFIELDS with an alpha
mask.  The same picture with a grey palette is stored at 2, 4 and 8 bpp and as
RLE8 and RLE4; grey palettes used to be looked up twice.  The width is odd so
that every row needs padding, and the stripes force RLE absolute-mode runs of
odd length.

oob8.bmp uses a pixel index beyond its 4-entry palette; the native reader
shows such pixels as black, like oob-ref24.bmp.
"""

import struct

W, H = 37, 29
# Index 0 is the background.
COLOUR = [(255, 255, 255), (220, 30, 30), (30, 60, 200), (0, 0, 0)]
GREY = [(255, 255, 255), (170, 170, 170), (85, 85, 85), (0, 0, 0)]


def picture():
    rows = []
    for y in range(H):
        row = []
        for x in range(W):
            v = 0
            if 4 <= x < 20 and 3 <= y < 14:
                v = 1
            if 15 <= x < 33 and 10 <= y < 25:
                v = 2
            if 22 <= y < 26 and x < 13:
                v = 3 if x % 3 else 1  # short runs: absolute mode in RLE
            row.append(v)
        rows.append(row)
    return rows  # top row first


def file_header(offset, size):
    return struct.pack("<2sIHHI", b"BM", size, 0, 0, offset)


def info_header(bpp, compression, image_size, colours):
    return struct.pack("<IiiHHIIiiII", 40, W, H, 1, bpp, compression, image_size, 2835, 2835,
                       colours, 0)


def palette_bytes(palette):
    return b"".join(struct.pack("<BBBB", b, g, r, 0) for r, g, b in palette)


def write(name, header, colour_table, data):
    offset = 14 + len(header) + len(colour_table)
    with open(name, "wb") as f:
        f.write(file_header(offset, offset + len(data)) + header + colour_table + data)


def pad(row):
    return row + b"\0" * (-len(row) % 4)


def rgb24(rows, colours):
    """colours maps a pixel value to (r, g, b)."""
    return b"".join(pad(bytes(c for v in row for c in reversed(colours(v))))
                    for row in reversed(rows))


def packed(rows, bpp):
    out = b""
    per_byte = 8 // bpp
    for row in reversed(rows):  # BMP rows are stored bottom-up
        if bpp == 8:
            out += pad(bytes(row))
            continue
        b = bytearray((W + per_byte - 1) // per_byte)
        for x, v in enumerate(row):
            b[x // per_byte] |= v << (8 - bpp * (x % per_byte + 1))
        out += pad(bytes(b))
    return out


def rle(rows, bpp):
    """Encode with encoded runs for repeats and absolute mode for short runs."""
    out = bytearray()
    for row in reversed(rows):
        x = 0
        while x < W:
            n = 1
            while x + n < W and row[x + n] == row[x] and n < 255:
                n += 1
            if n >= 3:
                v = row[x] if bpp == 8 else (row[x] << 4) | row[x]
                out += bytes((n, v))
                x += n
                continue
            # absolute mode: gather pixels until a run of 3 or more starts
            start = x
            while x < W and x - start < 255:
                if x + 2 < W and row[x] == row[x + 1] == row[x + 2]:
                    break
                x += 1
            count = x - start
            if count < 3:  # absolute mode needs at least 3 pixels
                for v in row[start:x]:
                    out += bytes((1, v if bpp == 8 else (v << 4) | v))
                continue
            px = row[start:x]
            if bpp == 8:
                body = bytes(px)
            else:
                body = bytes(((px[i] << 4) | (px[i + 1] if i + 1 < count else 0))
                             for i in range(0, count, 2))
            out += bytes((0, count)) + body + (b"\0" if len(body) % 2 else b"")
        out += b"\0\0"  # end of line
    out[-2:] = b"\0\1"  # end of bitmap replaces the last end of line
    return bytes(out)


def indexed(name, rows, palette, bpp, compression=0):
    data = rle(rows, bpp) if compression else packed(rows, bpp)
    write(name, info_header(bpp, compression, len(data), len(palette)), palette_bytes(palette),
          data)


def main():
    rows = picture()

    data = rgb24(rows, lambda v: COLOUR[v])
    write("ref24.bmp", info_header(24, 0, len(data), 0), b"", data)
    indexed("rle8.bmp", rows, COLOUR, 8, 1)
    indexed("rle4.bmp", rows, COLOUR, 4, 2)
    indexed("bpp2.bmp", rows, COLOUR, 2)

    # 32 bpp BITFIELDS with an alpha mask, BITMAPV5HEADER (124 bytes).
    data = b"".join(
        struct.pack("<I", 0xFF000000 | (r << 16) | (g << 8) | b)
        for row in reversed(rows) for r, g, b in (COLOUR[v] for v in row))
    v5 = struct.pack("<IiiHHIIiiII", 124, W, H, 1, 32, 3, len(data), 2835, 2835, 0, 0)
    v5 += struct.pack("<IIII", 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)
    v5 += struct.pack("<I", 0x73524742)  # LCS_sRGB
    v5 += b"\0" * (124 - len(v5))
    write("bpp32-alpha.bmp", v5, b"", data)

    data = rgb24(rows, lambda v: GREY[v])
    write("grey-ref24.bmp", info_header(24, 0, len(data), 0), b"", data)
    indexed("grey2.bmp", rows, GREY, 2)
    indexed("grey4.bmp", rows, GREY, 4)
    indexed("grey8.bmp", rows, GREY, 8)
    indexed("grey-rle8.bmp", rows, GREY, 8, 1)
    indexed("grey-rle4.bmp", rows, GREY, 4, 2)

    # Index 200 lies beyond the 4-entry palette; it is read as black.
    oob = [[200 if v == 2 else v for v in row] for row in rows]
    indexed("oob8.bmp", oob, COLOUR, 8)
    data = rgb24(rows, lambda v: (0, 0, 0) if v == 2 else COLOUR[v])
    write("oob-ref24.bmp", info_header(24, 0, len(data), 0), b"", data)


if __name__ == "__main__":
    main()
