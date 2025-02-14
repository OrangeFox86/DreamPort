#!/usr/bin/env python3

# MIT License
#
# Copyright (c) 2022-2025 James Smith of OrangeFox86
# https://github.com/OrangeFox86/DreamcastControllerUsbPico

import os
import sys
import argparse
import math
import binascii

def mirror_byte(byte_value):
    mirrored_value = 0
    for i in range(8):
        if byte_value & (1 << i):
            mirrored_value |= (1 << (7 - i))
    return mirrored_value

def main(argv):
    parser = argparse.ArgumentParser(description='Converts a VMU screen command to a monochrome bitmap')
    parser.add_argument('cmd', type=str, help='The Dreamcast screen command (must start with "0CXXXX32 00000004 00000000" unless --screen-only)')
    parser.add_argument('--invert', action="store_true", default=False, help='Invert colors')
    parser.add_argument('--output', type=str, default='screen.bmp', help='output file path (default: screen.bmp)')
    parser.add_argument('--input-endian', type=str, default='big', help='Input endian (default: big)')
    parser.add_argument('--screen-only', action="store_true", default=False, help='Input screen data only')

    args = parser.parse_args(args=argv)

    invert = args.invert
    cmd:str = args.cmd

    # Parse command
    # 0C 01 00 32 00 00 00 04 00 00 00 00 68 00 3F C0 02 01 FC 01 C0 3C 0C 03 FE 06 00 07 98 07 6F 08 00 71 F0 0E FF 10 00 0C 60 1F FF 60 00 02 30 3F 6D C0 00 01 18 36 FF 83 01 F0 14 7F FF BD 06 0E 10 7F 6D CE 38 07 A0 F6 FF 8B C8 07 C0 FF C3 0F 0C 05 40 7F 1F 1F 0E 05 60 7E 67 FD 0B 87 3F C7 8F EB 0D FF F8 F1 33 55 0A BF FF 3D 46 AB 0D 55 63 E1 0F 55 0A AA A0 7B 3A AA 05 55 60 21 E3 56 06 AA A0 3C 82 AA 05 55 60 27 07 56 02 AA A0 21 06 AA 03 55 40 20 07 54 01 AA C0 20 0D AC 00 D5 40 60 1C F8 00 6A 80 70 14 10 00 3F 00 50 22 00 00 18 00 48 22 00 00 0E 00 88 41 00 00 00 00 84 41 00 00 00 01 04 80 80 00 00 01 02
    cmd = cmd.replace(' ', '').replace('\r', '').replace('\n', '').replace('\t', '').replace(',', '').replace('0x', '').replace('X', '0')
    words = [int.from_bytes(binascii.unhexlify(cmd[i:i + 8]), args.input_endian) for i in range(0, len(cmd), 8)]
    if not args.screen_only:
        if len(words) != 51:
            raise RuntimeError(f"Expecting exactly 51 words; given len: {len(words)}")
        if ((words[0] & 0xFF0000FF) != 0xc000032):
            raise RuntimeError(f"First word not 0x0cXXXX32: {hex(words[0])}")
        if (words[1] != 0x4):
            raise RuntimeError(f"Second word not 0x00000004: {hex(words[1])}")
        if (words[2] != 0):
            raise RuntimeError(f"Third word not 0x00000000: {hex(words[2])}")
        screen_words = words[3:]
    else:
        if len(words) != 48:
            raise RuntimeError(f"Expecting exactly 48 words; given len: {len(words)}")
        screen_words = words

    b = bytearray()
    # Write header
    b.extend(bytearray([0x42, 0x4D])) # "BM" for bitmap
    b.extend(int(318).to_bytes(4, byteorder='little')) # file size (little endian)
    b.extend(bytearray([0x00, 0x00, 0x00, 0x00])) # (reserved - unused)
    b.extend(bytearray([0x3E, 0x00, 0x00, 0x00])) # Offset to data start
    # Write DIB header
    b.extend(int(40).to_bytes(4, byteorder='little')) # Total DIB size
    b.extend(int(48).to_bytes(4, byteorder='little')) # Width
    b.extend(int(32).to_bytes(4, byteorder='little')) # Height
    b.extend(bytearray([         # And the rest
        0x01, 0x00, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00,
        0x12, 0x0B, 0x00, 0x00,
        0x12, 0x0B, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    ]))
    # Extra bytes (bit masks?)
    b.extend(bytearray([
        0x00, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0x00
    ]))
    bytes_per_row = 8 # Must be >= 6

    raw_bytes = bytearray()
    for num in screen_words:
        raw_bytes.extend(num.to_bytes(4, "big"))

    if not invert:
        raw_bytes = bytearray([x ^ 255 for x in raw_bytes])

    byte_idx = 0
    for _ in range(32):
        b.extend(bytearray([mirror_byte(raw_bytes[byte_idx + x]) for x in range(5, -1, -1)]))
        byte_idx += 6
        b.extend(bytearray([0x00] * (bytes_per_row - 6)))

    with open(args.output, 'wb') as f:
        # Write header
        f.write(b)

    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
