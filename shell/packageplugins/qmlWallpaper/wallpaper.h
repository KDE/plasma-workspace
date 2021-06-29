/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KPackage/PackageStructure>

class QmlWallpaperPackage : public KPackage::PackageStructure
{
public:
    QmlWallpaperPackage(QObject *, const QVariantList &)
    {
    }
    void initPackage(KPackage::Package *package) override;
};
