#!/usr/bin/env python3
"""
svg_to_tiles_flat.py

Render a world SVG into flat tile images with filenames like:
    osm_100-l-<z>-<x>-<y>.png

Each tile is 256x256.

Usage:
    python svg_to_tiles_flat.py input.svg --min-zoom 0 --max-zoom 5 --outdir tiles

Dependencies:
    pip install cairosvg pillow
"""

import argparse
import os
from io import BytesIO

from PIL import Image
import cairosvg

TILE_SIZE = 256


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def render_full_png(svg_path, width_px):
    png_bytes = cairosvg.svg2png(url=svg_path, output_width=width_px)
    img = Image.open(BytesIO(png_bytes))
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    return img


def make_tiles_from_image(img, zoom, outdir):
    tiles_per_side = 2 ** zoom
    expected_size = TILE_SIZE * tiles_per_side
    if img.width != expected_size or img.height != expected_size:
        print(f"  - Rescaling rendered image from {img.width}x{img.height} to {expected_size}x{expected_size}")
        img = img.resize((expected_size, expected_size), resample=Image.LANCZOS)

    for x in range(tiles_per_side):
        for y in range(tiles_per_side):
            left = x * TILE_SIZE
            upper = y * TILE_SIZE
            right = left + TILE_SIZE
            lower = upper + TILE_SIZE
            tile = img.crop((left, upper, right, lower))
            ensure_dir(outdir)
            tile_name = f"osm_100-l-1-{zoom}-{x}-{y}.png"
            tile_path = os.path.join(outdir, tile_name)
            tile.save(tile_path, format='PNG')
        if x % 16 == 0:
            print(f"    rendered column x={x}/{tiles_per_side}")


def main():
    parser = argparse.ArgumentParser(description='Render SVG into flat tile PNGs (256px)')
    parser.add_argument('svg', help='input SVG file (world map)')
    parser.add_argument('--min-zoom', type=int, default=0, help='minimum zoom level (inclusive)')
    parser.add_argument('--max-zoom', type=int, default=5, help='maximum zoom level (inclusive)')
    parser.add_argument('--outdir', default='tiles', help='output directory')
    parser.add_argument('--overwrite', action='store_true', help='overwrite existing tiles')
    args = parser.parse_args()

    svg_path = args.svg
    outdir = args.outdir

    if not os.path.isfile(svg_path):
        raise SystemExit(f"SVG file not found: {svg_path}")

    for z in range(args.min_zoom, args.max_zoom + 1):
        tiles_per_side = 2 ** z
        width_px = TILE_SIZE * tiles_per_side
        print(f"Zoom {z}: rendering full PNG at {width_px}x{width_px}...")
        img = render_full_png(svg_path, width_px)
        print(f"  - rendered image size: {img.width}x{img.height}")

        if not args.overwrite:
            sample_tile = os.path.join(outdir, f"osm_100-l-1-{z}-0-0.png")
            if os.path.exists(sample_tile):
                print(f"  - tiles for zoom {z} already exist (use --overwrite to regenerate). Skipping.")
                continue

        print(f"  - splitting into {tiles_per_side}x{tiles_per_side} tiles...")
        make_tiles_from_image(img, z, outdir)
        print(f"  - zoom {z} done. Tiles saved under {outdir}")

    print("All done.")


if __name__ == '__main__':
    main()
