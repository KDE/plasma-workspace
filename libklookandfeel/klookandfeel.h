/*
    SPDX-FileCopyrightText: 1997 Matthias Kalle Dalheimer <kalle@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "klookandfeel_export.h"

#include <QObject>
#include <QString>

namespace KPackage
{
class Package;
}

class KLOOKANDFEEL_EXPORT KLookAndFeel
{
    Q_GADGET

public:
    enum class Variant {
        Unknown,
        Dark,
        Light,
    };
    Q_ENUM(Variant)

    static QString colorSchemeFilePath(const KPackage::Package &package);
    static Variant colorSchemeVariant(const KPackage::Package &package);
};
