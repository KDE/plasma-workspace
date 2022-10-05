#!/usr/bin/env python3
'''
    SPDX-FileCopyrightText: 2022 Niccolò Venerandi <niccolo@venerandi.com>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
'''

from argparse import ArgumentParser
import os
from typing import Final

from PIL import Image, ImageFilter

SCRIPT_DIR: Final = os.path.abspath(os.path.dirname(__file__))
BASE_DIR: Final = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))


def get_parser() -> ArgumentParser:
    """
    Creates an argument parser
    """
    parser = ArgumentParser(description="Generate theme previews")
    parser.add_argument(
        "-l",
        "--wallpaper-light",
        type=str,
        help="Source path of the light wallpaper",
        required=False,
        default="/usr/share/wallpapers/Next/contents/images/1920x1080.png")
    parser.add_argument(
        "-d",
        "--wallpaper-dark",
        type=str,
        help="Source path of the dark wallpaper",
        required=False,
        default="/usr/share/wallpapers/Next/contents/images_dark/1920x1080.png"
    )
    parser.add_argument("--breeze-light",
                        type=str,
                        help="Folder path of org.kde.breeze",
                        required=False,
                        default=os.path.join(BASE_DIR, "org.kde.breeze"))
    parser.add_argument("--breeze-dark",
                        type=str,
                        help="Folder path of org.kde.breezedark",
                        required=False,
                        default=os.path.join(BASE_DIR, "org.kde.breezedark"))
    parser.add_argument("--breeze-twilight",
                        type=str,
                        help="Folder path of org.kde.breezetwilight",
                        required=False,
                        default=os.path.join(BASE_DIR,
                                             "org.kde.breezetwilight"))
    parser.add_argument("--sddm-theme",
                        type=str,
                        help="Folder path of sddm-theme",
                        required=False,
                        default=os.path.join(BASE_DIR, "sddm-theme"))

    return parser


def generate_previews(light_path: str, dark_path: str, breeze_light_path: str,
                      breeze_dark_path: str, breeze_twilight_path: str,
                      sddm_path: str) -> None:
    """
    Generates previews for the login screen and the desktop
    """
    assert os.path.isfile(light_path), "Invalid light wallpaper path"
    assert os.path.isdir(
        os.path.join(breeze_light_path, "contents",
                     "previews")), f"Invalid Breeze folder {breeze_light_path}"
    assert os.path.isdir(os.path.join(
        breeze_dark_path, "contents",
        "previews")), f"Invalid Breeze Dark folder {breeze_dark_path}"
    assert os.path.isdir(
        os.path.join(breeze_twilight_path, "contents", "previews")
    ), f"Invalid Breeze Twilight folder {breeze_twilight_path}"
    assert os.path.isdir(sddm_path), f"Invalid sddm-theme folder {sddm_path}"

    if not (dark_path and os.path.isfile(dark_path)):
        dark_path = light_path

    variants = (
        ("light", light_path, breeze_light_path),
        ("dark", dark_path, breeze_dark_path),
        ("twilight", light_path, breeze_twilight_path),
    )

    for asset_name, wallpaper_path, save_folder in variants:
        print(f"Generating {asset_name}")
        wallpaper = Image.open(wallpaper_path).convert('RGBA')
        asset_img = Image.open(
            os.path.join(SCRIPT_DIR, "assets", f"{asset_name}.png"))
        img = Image.alpha_composite(wallpaper, asset_img)
        full_target_path: str = os.path.join(save_folder, "contents",
                                             "previews",
                                             "fullscreenpreview.jpg")
        img.convert('RGB').save(full_target_path,
                                quality=90,
                                optimize=True,
                                subsampling=1)
        preview_target_path: str = os.path.join(save_folder, "contents",
                                                "previews", "preview.png")
        img.resize((600, 337)).save(preview_target_path,
                                    optimize=True,
                                    subsampling=1)

    print("Generating login screen")
    blurred_wallpaper = wallpaper.filter(ImageFilter.GaussianBlur(20))
    login_img = Image.open(os.path.join(SCRIPT_DIR, "assets", "login.png"))
    blurred_login = Image.alpha_composite(blurred_wallpaper, login_img)
    blurred_login_resized = blurred_login.resize((600, 337))

    login_preview_path: str = os.path.join(breeze_light_path, "contents",
                                           "previews", "lockscreen.png")
    blurred_login_resized.save(login_preview_path,
                               optimize=True,
                               subsampling=1)

    sddm_preview_path: str = os.path.join(sddm_path, "preview.png")
    blurred_login.save(sddm_preview_path, optimize=True, subsampling=1)


def main() -> None:
    """
    Main script

    Usage example: ./generate_previews.py -l /usr/share/wallpapers/Next/contents/images/1920x1080.png -d /usr/share/wallpapers/Next/contents/images_dark/1920x1080.png
    """
    parser = get_parser()
    result = parser.parse_args()

    light_path: str = result.wallpaper_light
    dark_path: str = result.wallpaper_dark
    breeze_light_path: str = result.breeze_light
    breeze_dark_path: str = result.breeze_dark
    breeze_twilight_path: str = result.breeze_twilight
    sddm_path: str = result.sddm_theme

    generate_previews(light_path, dark_path, breeze_light_path,
                      breeze_dark_path, breeze_twilight_path, sddm_path)
    print("Success")


if __name__ == "__main__":
    main()
