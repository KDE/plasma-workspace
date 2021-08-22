# SPDX-FileCopyrightText: 2021 David Edmundson <kde@davidedmundson.co.uk>
# SPDX-License-Identifier: BSD-3-Clause

find_program(KIOFuse_PATH kio-fuse PATHS ${KDE_INSTALL_FULL_LIBEXECDIR})

if (KIOFuse_PATH)
    set(KIOFuse_FOUND TRUE)
endif()
