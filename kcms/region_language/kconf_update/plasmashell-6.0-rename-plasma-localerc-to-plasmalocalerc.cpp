/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include <QStandardPaths>

/**
 * A valid DBus object path can't contain '-', thus plasma-localerc can't
 * be used in KConfigWatcher. Rename to plasmalocalerc to make the object
 * name valid.
 *
 * @since 6.0
 */
int main()
{
    const std::string configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).toStdString();
    const std::filesystem::path oldPath = configDir + "/plasma-localerc";
    if (!std::filesystem::exists(oldPath)) {
        std::cout << "No need to rename plasma-localerc" << std::endl;
        return EXIT_SUCCESS;
    }

    const std::filesystem::path newPath = configDir + "/plasmalocalerc";
    if (std::filesystem::exists(newPath)) {
        std::cerr << "plasmalocalerc already exists" << std::endl;
        return EXIT_FAILURE;
    }

    std::error_code errorCode;
    std::filesystem::rename(oldPath, newPath, errorCode);

    return errorCode ? EXIT_FAILURE : EXIT_SUCCESS;
}
