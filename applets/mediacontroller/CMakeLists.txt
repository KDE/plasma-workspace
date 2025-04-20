# SPDX-FileCopyrightText: 2022 Bharadwaj Raju <bharadwaj.raju777@protonmail.com>
# SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

plasma_add_applet(org.kde.plasma.mediacontroller
    QML_SOURCES
        main.qml
        AlbumArtStackView.qml
        CompactRepresentation.qml
        ExpandedRepresentation.qml
    CPP_SOURCES
        globalconfig.cpp
    GENERATE_APPLET_CLASS
)
