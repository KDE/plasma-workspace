/*
 *  Copyright 2014 (c) Martin Klapetek <mklapetek@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "osd.h"
#include "shellcorona.h"

#include <QDBusConnection>
#include <QTimer>
#include <QWindow>
#include <QDebug>
#include <QUrl>

#include <Plasma/Package>
#include <KDeclarative/QmlObject>
#include <klocalizedstring.h>

Osd::Osd(ShellCorona *corona)
    : QObject(corona)
{
    const QString osdPath = corona->lookAndFeelPackage().filePath("osdmainscript");
    if (osdPath.isEmpty()) {
        qWarning() << "Failed to load the OSD script file from" << osdPath;
        return;
    }

    m_osdObject = new KDeclarative::QmlObject(this);
    m_osdObject->setSource(QUrl::fromLocalFile(osdPath));
    m_timeout = m_osdObject->rootObject()->property("timeout").toInt();

    QDBusConnection::sessionBus().registerObject("/org/kde/osdService", this, QDBusConnection::ExportAllSlots);

    m_osdTimer = new QTimer(this);
    m_osdTimer->setSingleShot(true);
    connect(m_osdTimer, &QTimer::timeout, this, &Osd::hideOsd);
}

Osd::~Osd()
{
}

void Osd::brightnessChanged(int percent)
{
    showProgress(QStringLiteral("video-display-brightness"), percent);
}

void Osd::keyboardBrightnessChanged(int percent)
{
    showProgress(QStringLiteral("input-keyboard-brightness"), percent);
}

void Osd::volumeChanged(int percent)
{
    QString icon;
    if (percent >= 75) {
        icon = QStringLiteral("audio-volume-high");
    } else if (percent < 75 && percent >= 25) {
        icon = QStringLiteral("audio-volume-medium");
    } else if (percent < 25 && percent > 0) {
        icon = QStringLiteral("audio-volume-low");
    } else if (percent == 0) {
        icon = QStringLiteral("audio-volume-muted");
        showText(icon, i18nc("OSD informing that the system is muted, keep short", "Audio Muted"));
        return;
    }

    showProgress(icon, percent);
}

void Osd::mediaPlayerVolumeChanged(int percent, const QString &playerName, const QString &playerIconName)
{
    if (percent == 0) {
        showText(playerIconName, i18nc("OSD informing that some media app is muted, eg. Amarok Muted", "%1 Muted", playerName));
    } else {
        showProgress(playerIconName, percent, playerName);
    }
}

void Osd::kbdLayoutChanged(const QString &layoutName)
{
    showText(QStringLiteral("keyboard-layout"), layoutName);
}

void Osd::virtualDesktopChanged(const QString &currentVirtualDesktopName)
{
    //FIXME: need a VD icon
    showText(QString(), currentVirtualDesktopName);
}

void Osd::showProgress(const QString &icon, const int percent, const QString &additionalText)
{
    int value = qBound(0, percent, 100);
    m_osdObject->rootObject()->setProperty("osdValue", value);
    m_osdObject->rootObject()->setProperty("osdAdditionalText", additionalText);
    m_osdObject->rootObject()->setProperty("showingProgress", true);
    m_osdObject->rootObject()->setProperty("icon", icon);

    showOsd();
}

void Osd::showText(const QString &icon, const QString &text)
{
    m_osdObject->rootObject()->setProperty("showingProgress", false);
    m_osdObject->rootObject()->setProperty("osdValue", text);
    m_osdObject->rootObject()->setProperty("icon", icon);

    showOsd();
}

void Osd::showOsd()
{
    m_osdObject->rootObject()->setProperty("visible", true);
    m_osdTimer->start(m_timeout);
}

void Osd::hideOsd()
{
    m_osdObject->rootObject()->setProperty("visible", false);
}
