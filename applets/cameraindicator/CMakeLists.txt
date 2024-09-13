# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
if(NOT TARGET PkgConfig::PipeWire)
    RETURN()
endif()
plasma_install_package(package org.kde.plasma.cameraindicator)
if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
