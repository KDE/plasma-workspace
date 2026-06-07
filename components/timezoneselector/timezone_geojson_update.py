#
# SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#

import json
import requests
import subprocess
import zipfile
import os


def download_latest_release_file(owner, repo, file_name, download_dir="."):
    # GitHub API URL for latest release
    api_url = f"https://api.github.com/repos/{owner}/{repo}/releases/latest"

    try:
        # Get the latest release info
        response = requests.get(api_url)
        response.raise_for_status()
        release_data = response.json()

        # Find the asset with the specified file name
        for asset in release_data["assets"]:
            if asset["name"] == file_name:
                download_url = asset["browser_download_url"]
                break
        else:
            print(f"File '{file_name}' not found in the latest release.")
            return

        # Download the file
        response = requests.get(download_url, stream=True)
        response.raise_for_status()

        # Save the file
        os.makedirs(download_dir, exist_ok=True)
        file_path = os.path.join(download_dir, file_name)
        with open(file_path, "wb") as file:
            for chunk in response.iter_content(chunk_size=8192):
                file.write(chunk)

        print(f"Downloaded '{file_name}' to '{file_path}'")

    except requests.exceptions.RequestException as e:
        print(f"Error downloading file: {e}")


def check_command(command):
    """Check if a command exists in the system."""
    return subprocess.call(['which', command], stdout=subprocess.PIPE, stderr=subprocess.PIPE) == 0


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
    from datetime import datetime
    from zoneinfo import ZoneInfo

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
    Pre-compute timezone band data and add it as GeoJSON properties.

    For each timezone feature, adds:
      - bandGroup: key into the bandGroups table
      - bandMinLon / bandMaxLon: longitude range used for the selection band
        (normally the group envelope; narrowed to the zone's own bounding
        box for Mountain time zones whose group stretches unreasonably wide)
      - bboxMinLat / bboxMaxLat / bboxMinLon / bboxMaxLon: bounding box
      - centerLat / centerLon: center of the bounding box
    """

    with open(filepath, "r", encoding="utf-8") as f:
        data = json.load(f)

    # Pass 1: compute bounding box and standard offset for each feature
    feature_data = []
    for feat in data["features"]:
        tzid = feat["properties"]["tzid"]
        min_lat, max_lat, min_lon, max_lon = compute_bbox(feat["geometry"])
        center_lat = (min_lat + max_lat) / 2
        center_lon = (min_lon + max_lon) / 2

        slash = tzid.find("/")
        region = tzid[:slash] if slash >= 0 else ""
        offset = standard_offset_seconds(tzid, center_lat)
        group_key = f"{region}+{offset}"

        feature_data.append({
            "tzid": tzid,
            "min_lat": min_lat,
            "max_lat": max_lat,
            "min_lon": min_lon,
            "max_lon": max_lon,
            "center_lat": center_lat,
            "center_lon": center_lon,
            "group_key": group_key,
        })

    # Pass 2: merge bounding boxes within each group
    groups = {}
    for fd in feature_data:
        key = fd["group_key"]
        if key not in groups:
            groups[key] = {"min_lon": fd["min_lon"], "max_lon": fd["max_lon"]}
        else:
            groups[key]["min_lon"] = min(groups[key]["min_lon"], fd["min_lon"])
            groups[key]["max_lon"] = max(groups[key]["max_lon"], fd["max_lon"])

    # Pass 3: store band groups as a top-level table and reference by key
    band_groups = {}
    for key, val in groups.items():
        band_groups[key] = {"minLon": val["min_lon"], "maxLon": val["max_lon"]}
    data["bandGroups"] = band_groups

    for feat, fd in zip(data["features"], feature_data):
        feat["properties"]["bandGroup"] = fd["group_key"]
        feat["properties"]["bboxMinLat"] = fd["min_lat"]
        feat["properties"]["bboxMaxLat"] = fd["max_lat"]
        feat["properties"]["bboxMinLon"] = fd["min_lon"]
        feat["properties"]["bboxMaxLon"] = fd["max_lon"]
        feat["properties"]["centerLat"] = fd["center_lat"]
        feat["properties"]["centerLon"] = fd["center_lon"]

        # For the Mountain time zone group (America+-25200) whose group
        # envelope spans from Yukon (-141°) to Nunavut (-89°), use the
        # individual bounding box instead so the selection band stays
        # narrow around the selected timezone.
        if fd["group_key"] == "America+-25200":
            feat["properties"]["bandMinLon"] = fd["min_lon"]
            feat["properties"]["bandMaxLon"] = fd["max_lon"]
        else:
            feat["properties"]["bandMinLon"] = g["min_lon"]
            feat["properties"]["bandMaxLon"] = g["max_lon"]

    groups_json = json.dumps(data.get("bandGroups", {}), separators=(",", ":"))

    with open(filepath, "w", encoding="utf-8") as f:
        f.write('{"type":"FeatureCollection", "bandGroups":')
        f.write(groups_json)
        f.write(', "features": [\n')
        for i, feat in enumerate(data["features"]):
            if i > 0:
                f.write(",\n")
            json.dump(feat, f, separators=(",", ":"))
        f.write("\n]}")

    print(f"Added band data to {len(feature_data)} timezones in {filepath}")


# Example usage
owner = "evansiroky"  # GitHub repository owner
repo = "timezone-boundary-builder"  # GitHub repository name
file_name = "timezones.geojson.zip"  # Replace with the name of the file you want to download
download_dir = "."  # Directory to save the file
json_file = "combined.json"
output_name = "timezones.json"

if not check_command('mapshaper'):
    print('The "mapshaper" command line utility is required to update the timezone information. See https://github.com/mbloch/mapshaper for more information')

download_latest_release_file(owner, repo, file_name, download_dir)

unzip_file(file_name)

run_mapshaper(json_file, output_name)

os.remove(file_name)

os.remove(json_file)

add_band_data(output_name)
