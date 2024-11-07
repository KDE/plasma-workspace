#
# SPDX-FileCopyrightText: 2024 Niccolò Venerandi <niccolo@venerandi.com>
#
# SPDX-License-Identifier: GPL-2.0-or-later
#

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

# Example usage
owner = "evansiroky"  # GitHub repository owner
repo = "timezone-boundary-builder"    # GitHub repository name
file_name = "timezones.geojson.zip"  # Replace with the name of the file you want to download
download_dir = "."    # Directory to save the file
json_file = "combined.json"
output_name = "timezones.json"


if not check_command('mapshaper'):
    print('The "mapshaper" command line utility is required to update the timezone information. See https://github.com/mbloch/mapshaper for more information')

download_latest_release_file(owner, repo, file_name, download_dir)

unzip_file(file_name)

run_mapshaper(json_file, output_name)

os.remove(file_name)

os.remove(json_file)


