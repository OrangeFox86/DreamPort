#!/usr/bin/env python3

# MIT License
#
# Copyright (c) 2022-2025 James Smith & Mike Kosek of OrangeFox86
# https://github.com/OrangeFox86/DreamcastControllerUsbPico

import os
import sys
import argparse
import math
from PIL import Image

def mirror_byte(byte_value):
    mirrored_value = 0
    for i in range(8):
        if byte_value & (1 << i):
            mirrored_value |= (1 << (7 - i))
    return mirrored_value

def convert_png_to_bitmap(png_path, bitmap_path):
    with Image.open(png_path) as img:
        img = img.convert('1')  # Convert to 1-bit pixels, black and white
        img.save(bitmap_path, format='BMP')

def main(argv):
    parser = argparse.ArgumentParser(description='Converts a 48x32 monochrome bitmap or (color) PNG to command')
    parser.add_argument('image', type=str, help='Path to bitmap or PNG')
    parser.add_argument('--invert', action="store_true", default=False, help='Invert colors')
    parser.add_argument('--screen-only', action="store_true", default=False, help='Output screen data only')
    parser.add_argument('--maple-index', type=int, default=0, help='The maple/player index to send this command to [0,3]')
    parser.add_argument('--format', type=str, default='{:02X}{:02X}{:02X}{:02X}', help='Format string (default: {:02X}{:02X}{:02X}{:02X})')
    parser.add_argument('--output-endian', type=str, default='big', help='Output endian (default: big)')

    args = parser.parse_args(args=argv)

    if args.maple_index < 0 or args.maple_index > 3:
        raise RuntimeError(f"Invalid maple index: {args.maple_index}")
    if args.output_endian != 'little' and args.output_endian != 'big':
        raise RuntimeError(f"Output endian must be either little or big; given: {args.output_endian}")

    invert = args.invert

    # Check if the input file is a PNG and convert it to a bitmap if necessary
    if args.image.lower().endswith('.png'):
        bitmap_path = args.image[:-4] + '.bmp'
        convert_png_to_bitmap(args.image, bitmap_path)
    else:
        bitmap_path = args.image

    with open(bitmap_path, 'rb') as f:
        # Move past header
        header = f.read(14)
        if len(header) < 14:
            print('EOF before header read')
            return 1
        # Read DIB header
        dib_header = f.read(1)
        if len(dib_header) < 1:
            print('EOF before DIB header read')
            return 1
        dib_header_len = int(dib_header[0])
        dib_header += f.read(dib_header_len - 1)
        if len(dib_header) < dib_header_len:
            print('EOF before DIB header read')
            return 1
        # Extract image dimensions
        width = int.from_bytes(dib_header[4:8], "little")
        height = int.from_bytes(dib_header[8:12], "little")
        # Skip past other bytes
        other_header = f.read(8)
        if len(other_header) < 8:
            print('EOF before header read')
            return 1
        # Read in image
        image_bytes = f.read()
        if len(image_bytes) * 8 < (width * height):
            print('EOF before full image read')
            return 1

        # Number of bytes in each row must be divisible by 4
        bytes_per_row = math.ceil(width / 8)
        if bytes_per_row % 4 != 0:
            bytes_per_row += 4 - (bytes_per_row % 4)

        if not invert:
            image_bytes = bytearray([x ^ 255 for x in image_bytes])

        # Arrange bytes into rows of bytes
        rows = [image_bytes[i*bytes_per_row:(i+1)*bytes_per_row] for i in range(height)]

        if len(rows) < 32:
            rows.extend([bytes([0] * 6) for _ in range(32 - len(rows))])

        screen_bytes = bytearray()

        for i in range(32):
            row = rows[i]
            if len(row) < 6:
                row = row + bytes([0] * (6 - len(row)))
            line = [mirror_byte(row[x]) for x in range(5, -1, -1)]
            screen_bytes.extend(bytearray(line))

        cmd_bytes = screen_bytes

        if not args.screen_only:
            host_addr = [0x00, 0x40, 0x80, 0xC0]
            host_addr = host_addr[args.maple_index]
            dev_addr = host_addr | 1
            cmd_bytes = bytearray([
                0x0C, dev_addr, host_addr, 0x32,
                0x00, 0x00, 0x00, 0x04,
                0x00, 0x00, 0x00, 0x00
            ]) + cmd_bytes

        cmd = ""

        for i in range(0, len(cmd_bytes), 4):
            if args.output_endian != 'little':
                word_str = args.format.format(*cmd_bytes[i:i+4])
            else:
                word_str = args.format.format(*bytearray([cmd_bytes[i + x] for x in range(3, -1, -1)]))

            if len(cmd) > 0:
                cmd += " "
            cmd += word_str

        print(cmd)

        return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
