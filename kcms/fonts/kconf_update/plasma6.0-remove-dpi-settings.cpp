/*
    SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KSharedConfig>
#include <cstdlib>

#include <QCoreApplication>

/**
 * In plasma6 scaling is completely managed by the global scaling settings, both in Wayland and X11,
 * setting fonts dpi is not supported anymore and can lead to undesired results
 *
 * @since 6.0
 */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const KSharedConfigPtr configPtr = KSharedConfig::openConfig(QString::fromLatin1("kcmfonts"), KConfig::FullConfig);
    KConfigGroup cg(configPtr, QStringLiteral("General"));

    cg.deleteEntry("forceFontDPIWayland");

    cg.sync();

    return EXIT_SUCCESS;
}
