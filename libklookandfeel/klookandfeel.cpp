/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "klookandfeel.h"

#include <KConfigGroup>
#include <KPackage/Package>
#include <KSharedConfig>

#include <QColor>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

static QString resolveColorSchemeFilePath(const QString &schemeName)
{
    QString colorScheme(schemeName);
    colorScheme.remove(QLatin1Char('\'')); // So Foo's does not become FooS
    QRegularExpression fixer(QStringLiteral("[\\W,.-]+(.?)"));
    for (auto match = fixer.match(colorScheme); match.hasMatch(); match = fixer.match(colorScheme)) {
        colorScheme.replace(match.capturedStart(), match.capturedLength(), match.captured(1).toUpper());
    }
    colorScheme.replace(0, 1, colorScheme.at(0).toUpper());

    // NOTE: why this loop trough all the scheme files?
    // the scheme theme name is an heuristic, there is no plugin metadata whatsoever.
    // is based on the file name stripped from weird characters or the
    // eventual id- prefix store.kde.org puts, so we can just find a
    // theme that ends as the specified name
    const QStringList schemeDirs =
        QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("color-schemes"), QStandardPaths::LocateDirectory);
    for (const QString &dir : schemeDirs) {
        const QStringList fileNames = QDir(dir).entryList(QStringList() << QStringLiteral("*.colors"));
        for (const QString &file : fileNames) {
            if (file.endsWith(colorScheme + QStringLiteral(".colors"))) {
                return dir + QLatin1Char('/') + file;
            }
        }
    }
    return {};
}

QString KLookAndFeel::colorSchemeFilePath(const KPackage::Package &package)
{
    QString colorScheme = package.filePath("colors");
    if (colorScheme.isEmpty()) {
        KSharedConfigPtr conf = KSharedConfig::openConfig(package.filePath("defaults"));
        KConfigGroup group(conf, QStringLiteral("kdeglobals"));
        group = KConfigGroup(&group, QStringLiteral("General"));
        const QString schemeName = group.readEntry("ColorScheme", QString());
        if (!schemeName.isEmpty()) {
            colorScheme = resolveColorSchemeFilePath(schemeName);
        }
    }

    return colorScheme;
}

KLookAndFeel::Variant KLookAndFeel::colorSchemeVariant(const KPackage::Package &package)
{
    const QString colorScheme = colorSchemeFilePath(package);
    if (colorScheme.isNull()) {
        return KLookAndFeel::Variant::Unknown;
    }

    const KSharedConfigPtr colorSchemeConfig = KSharedConfig::openConfig(colorScheme);
    const KConfigGroup group(colorSchemeConfig, QStringLiteral("Colors:Window"));
    if (!group.hasKey(QStringLiteral("BackgroundNormal"))) {
        return KLookAndFeel::Variant::Unknown;
    }

    const QColor backgroundNormal = group.readEntry(QStringLiteral("BackgroundNormal"), QColor());
    const int backgroundGray = qGray(backgroundNormal.rgb());
    if (backgroundGray < 192) {
        return KLookAndFeel::Variant::Dark;
    } else {
        return KLookAndFeel::Variant::Light;
    }
}

#include "moc_klookandfeel.cpp"
