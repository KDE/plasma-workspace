#
# SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#

import json
import os
import subprocess
import zipfile
from datetime import datetime
from shutil import which
from zoneinfo import ZoneInfo

import requests


def download_latest_release_file(owner, repo, file_name, download_dir="."):
    """Download a specific file from the latest GitHub release of a repository."""
    # GitHub API URL for latest release
    api_url = f"https://api.github.com/repos/{owner}/{repo}/releases/latest"

    try:
        # Get the latest release info
        resp = requests.get(api_url)
        resp.raise_for_status()
        release_data = resp.json()

        # Find the asset with the specified file name
        for asset in release_data["assets"]:
            if asset["name"] == file_name:
                download_url = asset["browser_download_url"]
                break
        else:
            print(f"File '{file_name}' not found in the latest release.")
            return

        file_path = os.path.join(download_dir, file_name)
        dl = requests.get(download_url, stream=True)
        dl.raise_for_status()
        with open(file_path, "wb") as f:
            for chunk in dl.iter_content(chunk_size=8192):
                f.write(chunk)

        print(f"Downloaded '{file_name}' to '{file_path}'")

    except requests.exceptions.RequestException as e:
        print(f"Error downloading file: {e}")


def unzip_file(zip_file_path, extract_to='.'):
    """Unzip the downloaded file."""
    print(f"Unzipping {zip_file_path}...")
    with zipfile.ZipFile(zip_file_path, 'r') as zip_ref:
        zip_ref.extractall(extract_to)
    print(f"Extracted to {extract_to}")


def run_mapshaper(json_file, output_name):
    """Run mapshaper on the specified JSON file."""
    print(f"Running mapshaper on {json_file}...")
    subprocess.run(['mapshaper', json_file, "-simplify", "0.2%", "keep-shapes", "-o", output_name])


def _collect_coords(geometry):
    """Yield (lon, lat) for every vertex in a GeoJSON geometry."""
    gtype = geometry["type"]
    coords = geometry["coordinates"]
    if gtype == "Polygon":
        for ring in coords:
            for pt in ring:
                yield pt[0], pt[1]
    elif gtype == "MultiPolygon":
        for polygon in coords:
            for ring in polygon:
                for pt in ring:
                    yield pt[0], pt[1]


def compute_bbox(geometry):
    """Compute bounding box from a GeoJSON geometry, handling anti-meridian."""
    all_lons = []
    all_lats = []
    for lon, lat in _collect_coords(geometry):
        all_lats.append(lat)
        all_lons.append(lon)

    min_lat = min(all_lats)
    max_lat = max(all_lats)
    min_lon = min(all_lons)
    max_lon = max(all_lons)

    # Anti-meridian normalization: if the span is >= 180, the polygon
    # crosses the anti-meridian. Shift negative longitudes by +360.
    if max_lon - min_lon >= 180:
        shifted_lons = [lon + 360 if lon < 0 else lon for lon in all_lons]
        min_lon = min(shifted_lons)
        max_lon = max(shifted_lons)

    return min_lat, max_lat, min_lon, max_lon


def standard_offset_seconds(tzid, center_lat):
    """Get the standard (non-DST) UTC offset in seconds."""
    try:
        tz = ZoneInfo(tzid)
    except (KeyError, Exception):
        return 0

    # Use a winter reference time for the respective hemisphere
    if center_lat >= 0:
        ref = datetime(2026, 1, 15, 12, 0, 0, tzinfo=tz)
    else:
        ref = datetime(2026, 7, 15, 12, 0, 0, tzinfo=tz)

    offset = ref.utcoffset()
    if offset is None:
        return 0
    return int(offset.total_seconds())


def add_band_data(filepath):
    """
    Add per-feature timezone band data to the GeoJSON file.

    For each feature, adds:
      - bandGroup: region+offset group key (used for group highlighting)
      - bboxMinLat / bboxMaxLat / bboxMinLon / bboxMaxLon: bounding box (used for auto-zoom)
      - centerLat / centerLon: center of the bounding box (used for map pan and dot)

    groupOutlines is preserved from the source data (produced by mapshaper).
    """
    with open(filepath, "r", encoding="utf-8") as f:
        data = json.load(f)

    for feat in data["features"]:
        geometry = feat["geometry"]
        tzid = feat["properties"]["tzid"]
        min_lat, max_lat, min_lon, max_lon = compute_bbox(geometry)
        center_lat = (min_lat + max_lat) / 2
        center_lon = (min_lon + max_lon) / 2

        slash = tzid.find("/")
        region = tzid[:slash] if slash >= 0 else ""
        offset = standard_offset_seconds(tzid, center_lat)

        feat["properties"]["bandGroup"] = f"{region}+{offset}"
        feat["properties"]["bboxMinLat"] = min_lat
        feat["properties"]["bboxMaxLat"] = max_lat
        feat["properties"]["bboxMinLon"] = min_lon
        feat["properties"]["bboxMaxLon"] = max_lon
        feat["properties"]["centerLat"] = center_lat
        feat["properties"]["centerLon"] = center_lon

    out = '{"type":"FeatureCollection",\n'
    out += ' "groupOutlines":'
    out += json.dumps(data.get("groupOutlines", {}), separators=(",", ":"))
    out += ',\n'
    out += ' "features":[\n'
    for i, feat in enumerate(data["features"]):
        if i > 0:
            out += ",\n"
        out += json.dumps(feat, separators=(",", ":"))
    out += "\n]}\n"
    with open(filepath, "w", encoding="utf-8") as f:
        f.write(out)

    print(f"Added band data to {len(data['features'])} timezones in {filepath}")


if __name__ == "__main__":
    # Example usage
    owner = "evansiroky"  # GitHub repository owner
    repo = "timezone-boundary-builder"  # GitHub repository name
    file_name = "timezones.geojson.zip"  # Replace with the name of the file you want to download
    download_dir = "."  # Directory to save the file
    json_file = "combined.json"
    output_name = "timezones.json"

    if not which("mapshaper"):
        print('The "mapshaper" command line utility is required to update the timezone information.')
        print('Install it with: npm install -g mapshaper')
        print('or: pnpm add -g mapshaper')
        exit(1)

    download_latest_release_file(owner, repo, file_name, download_dir)

    unzip_file(file_name)

    run_mapshaper(json_file, output_name)

    os.remove(file_name)

    os.remove(json_file)

    add_band_data(output_name)
