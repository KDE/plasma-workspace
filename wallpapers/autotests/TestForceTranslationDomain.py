#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: MIT

import os
import sys
import unittest


class TestTranslationDomain(unittest.TestCase):
    """
    For Plasma wallpaper plugins, it's required to use i18nd/i18ndc to specify the translation domain,
    otherwise the wallpaper config dialog will show untranslated terms and sentences.
    """

    test_folder_paths: list[str] = [
        os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir)),
    ]
    __error_list: list[tuple[str, int]] = []


    def __check_translation_domain(self, path: str) -> None:
        """
        Checks translation domain in the source code
        """

        with open(path, mode="r", encoding="utf-8") as handler:
            count: int = 1
            line_str: str = handler.readline()

            while line_str:
                if "i18n(" in line_str or "i18nc(" in line_str or "qsTr(" in line_str:
                    self.__error_list.append((path, count))
                line_str = handler.readline()
                count += 1

    def test_i18nd(self) -> None:
        """
        Tests all translatable strings have specified translation domains.
        """
        for parent_dir in self.test_folder_paths:
            for path, _, files in os.walk(parent_dir):
                for name in files:
                    if not name.endswith((".qml", ".js")):
                        continue
                    fullpath = os.path.join(path, name)
                    self.__check_translation_domain(fullpath)

        self.assertTrue(
            len(self.__error_list) == 0, "\n".join(
                f"{path} line {count} contains a translatable string that does not have a translation domain."
                for path, count in self.__error_list))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        TestTranslationDomain.test_folder_paths = sys.argv.pop().split(',')

    unittest.main()
