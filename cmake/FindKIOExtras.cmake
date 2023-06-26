# SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
# SPDX-License-Identifier: BSD-3-Clause

find_path(KIOExtras_PATH thumbnail.so PATHS ${KDE_INSTALL_FULL_PLUGINDIR}/kf6/kio/)

if (KIOExtras_PATH)
    set(KIOExtras_FOUND TRUE)
endif()
