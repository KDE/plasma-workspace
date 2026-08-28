/*
    SPDX-FileCopyrightText: 2026 Tomáš Hnyk <tomashnyk@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QCoreApplication>

#include <KConfig>
#include <KConfigGroup>

// Klipper used to store these inverted, as IgnoreSelection and IgnoreImages.
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    KConfig config(QStringLiteral("klipperrc"), KConfig::SimpleConfig);
    KConfigGroup general = config.group(QStringLiteral("General"));

    if (general.hasKey("IgnoreSelection")) {
        general.writeEntry("SaveSelection", !general.readEntry("IgnoreSelection", true));
        general.deleteEntry("IgnoreSelection");
    }
    if (general.hasKey("IgnoreImages")) {
        general.writeEntry("SaveImages", !general.readEntry("IgnoreImages", true));
        general.deleteEntry("IgnoreImages");
    }
    // SelectionTextOnly has no equivalent in the new settings.
    general.deleteEntry("SelectionTextOnly");

    config.sync();
    return 0;
}
