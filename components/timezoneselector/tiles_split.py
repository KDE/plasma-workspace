#!/usr/bin/env python3
"""
Render a world SVG into flat tile images (256px tiles).

Full pipeline:
  1. python tiles_split.py --download-mvt
  2. Open https://mapbox.github.io/mvt-cruncher/html/mvt2svg.html
     drag world.mvt onto the page, copy SVG output, save as world.svg
  3. python tiles_split.py world.svg --min-zoom 0 --max-zoom 4 --outdir offline_tiles --overwrite

The script parses the SVG as XML, normalises the viewBox, replaces sea
colour (fill:red -> #AAD3DF), removes border strokes, and splits into
256px PNG tiles.

Dependencies: pip install cairosvg pillow
"""

import argparse
import os
import sys
import tempfile
import xml.etree.ElementTree as ET
from io import BytesIO
from urllib.request import urlretrieve

from PIL import Image

TILE_SIZE = 256
MVT_URL = 'https://vector.openstreetmap.org/shortbread_v1/0/0/0.mvt'
MVT_FILENAME = 'world.mvt'

XML_NS = 'http://www.w3.org/2000/svg'


def download_mvt(outpath: str = MVT_FILENAME) -> None:
    if os.path.exists(outpath):
        print(f"{outpath} already exists, skipping download.")
        return
    print(f"Downloading {MVT_URL} ...")
    urlretrieve(MVT_URL, outpath)
    print(f"Saved to {outpath}")
    print(f"\nNext steps:")
    print(f"  1. Open https://mapbox.github.io/mvt-cruncher/html/mvt2svg.html")
    print(f"  2. Drag {outpath} onto the page, copy all SVG output")
    print(f"  3. Save as world.svg")
    print(f"  4. Run: python {sys.argv[0]} world.svg --min-zoom 0 --max-zoom 4 --outdir offline_tiles")


def ensure_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def fix_style(style: str | None) -> str | None:
    """Remove stroke properties, fix fill colour and opacity."""
    if not style:
        return style
    parts = []
    for part in style.split(';'):
        part = part.strip()
        if not part:
            continue
        if part.startswith('stroke'):
            continue
        if part == 'fill:red':
            part = 'fill:#AAD3DF'
        elif part == 'fill-opacity:0.3':
            part = 'fill-opacity:1'
        parts.append(part)
    return ';'.join(parts) if parts else None


def process_element(elem: ET.Element) -> None:
    """Recursively fix style attributes in an XML element tree."""
    style = elem.get('style')
    if style:
        new_style = fix_style(style)
        if new_style:
            elem.set('style', new_style)
        else:
            del elem.attrib['style']
    for child in elem:
        process_element(child)


def process_svg(svg_content: str) -> str:
    """Parse SVG as XML, fix viewBox/styles, return modified SVG string."""
    root = ET.fromstring(svg_content)

    w = root.get('width', '1024')
    h = root.get('height', '1024')
    root.set('width', str(int(float(w))))
    root.set('height', str(int(float(h))))
    root.set('viewBox', '0 0 {} {}'.format(int(float(w)), int(float(h))))

    process_element(root)

    return ET.tostring(root, encoding='unicode')


def render_full_png(svg_path: str, width_px: int) -> Image.Image:
    import cairosvg

    with open(svg_path, encoding='utf-8') as f:
        svg_content = f.read()

    svg_content = process_svg(svg_content)

    with tempfile.NamedTemporaryFile(suffix='.svg', mode='w', encoding='utf-8', delete=False) as tmp:
        tmp.write(svg_content)
        tmp_path = tmp.name

    try:
        png_bytes = cairosvg.svg2png(url=tmp_path, output_width=width_px)
    finally:
        os.unlink(tmp_path)

    img = Image.open(BytesIO(png_bytes))
    if img.mode != 'RGBA':
        img = img.convert('RGBA')
    return img


def make_tiles_from_image(img: Image.Image, zoom: int, outdir: str) -> None:
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


def main() -> None:
    parser = argparse.ArgumentParser(description='Render SVG into flat tile PNGs')
    parser.add_argument('svg', nargs='?', help='input SVG file')
    parser.add_argument('--download-mvt', action='store_true')
    parser.add_argument('--min-zoom', type=int, default=0)
    parser.add_argument('--max-zoom', type=int, default=4)
    parser.add_argument('--outdir', default='tiles')
    parser.add_argument('--overwrite', action='store_true')
    args = parser.parse_args()

    if args.download_mvt:
        download_mvt()
        return

    if not args.svg:
        parser.print_help()
        sys.exit(1)

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
                print(f"  - tiles for zoom {z} already exist. Skipping.")
                continue

        print(f"  - splitting into {tiles_per_side}x{tiles_per_side} tiles...")
        make_tiles_from_image(img, z, outdir)
        print(f"  - zoom {z} done.")

    print("All done.")


if __name__ == '__main__':
    main()
