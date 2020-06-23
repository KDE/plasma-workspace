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
#include <KDeclarative/QmlObjectSharedEngine>
#include <klocalizedstring.h>

Osd::Osd(const KSharedConfig::Ptr &config, ShellCorona *corona)
    : QObject(corona)
    , m_osdUrl(corona->lookAndFeelPackage().fileUrl("osdmainscript"))
    , m_config(config)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/kde/osdService"), this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
}

Osd::~Osd()
{
}

void Osd::brightnessChanged(int percent)
{
    showProgress(QStringLiteral("video-display-brightness"), percent, 100);
}

void Osd::keyboardBrightnessChanged(int percent)
{
    showProgress(QStringLiteral("input-keyboard-brightness"), percent, 100);
}

void Osd::volumeChanged(int percent)
{
    volumeChanged(percent, 100);
}

void Osd::volumeChanged(int percent, int maximumPercent)
{
    QString icon;
    if (percent <= 0) {
        icon = QStringLiteral("audio-volume-muted");
        showText(icon, i18nc("OSD informing that the system is muted, keep short", "Audio Muted"));
        return;
    } else if (percent <= 25) {
        icon = QStringLiteral("audio-volume-low");
    } else if (percent <= 75) {
        icon = QStringLiteral("audio-volume-medium");
    } else {
        icon = QStringLiteral("audio-volume-high");
    }

    showProgress(icon, percent, maximumPercent);
}

void Osd::microphoneVolumeChanged(int percent)
{
    QString icon;
    if (percent <= 0) {
        icon = QStringLiteral("microphone-sensitivity-muted");
        showText(icon, i18nc("OSD informing that the microphone is muted, keep short", "Microphone Muted"));
        return;
    } else if (percent <= 25) {
        icon = QStringLiteral("microphone-sensitivity-low");
    } else if (percent <= 75) {
        icon = QStringLiteral("microphone-sensitivity-medium");
    } else {
        icon = QStringLiteral("microphone-sensitivity-high");
    }

    showProgress(icon, percent, 100);
}

void Osd::mediaPlayerVolumeChanged(int percent, const QString &playerName, const QString &playerIconName)
{
    if (percent == 0) {
        showText(playerIconName, i18nc("OSD informing that some media app is muted, eg. Amarok Muted", "%1 Muted", playerName));
    } else {
        showProgress(playerIconName, percent, 100, playerName);
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

void Osd::touchpadEnabledChanged(bool touchpadEnabled)
{
    if (touchpadEnabled) {
        showText(QStringLiteral("input-touchpad-on"), i18nc("touchpad was enabled, keep short", "Touchpad On"));
    } else {
        showText(QStringLiteral("input-touchpad-off"), i18nc("touchpad was disabled, keep short", "Touchpad Off"));
    }
}

void Osd::wifiEnabledChanged(bool wifiEnabled)
{
    if (wifiEnabled) {
        showText(QStringLiteral("network-wireless-on"), i18nc("wireless lan was enabled, keep short", "Wifi On"));
    } else {
        showText(QStringLiteral("network-wireless-off"), i18nc("wireless lan was disabled, keep short", "Wifi Off"));
    }
}

void Osd::bluetoothEnabledChanged(bool bluetoothEnabled)
{
    if (bluetoothEnabled) {
        showText(QStringLiteral("preferences-system-bluetooth"), i18nc("Bluetooth was enabled, keep short", "Bluetooth On"));
    } else {
        showText(QStringLiteral("preferences-system-bluetooth-inactive"), i18nc("Bluetooth was disabled, keep short", "Bluetooth Off"));
    }
}

void Osd::wwanEnabledChanged(bool wwanEnabled)
{
    if (wwanEnabled) {
        showText(QStringLiteral("network-mobile-on"), i18nc("mobile internet was enabled, keep short", "Mobile Internet On"));
    } else {
        showText(QStringLiteral("network-mobile-off"), i18nc("mobile internet was disabled, keep short", "Mobile Internet Off"));
    }
}

void Osd::virtualKeyboardEnabledChanged(bool virtualKeyboardEnabled)
{
    if (virtualKeyboardEnabled) {
        showText(QStringLiteral("input-keyboard-virtual-on"), i18nc("on screen keyboard was enabled because physical keyboard got unplugged, keep short", "On-Screen Keyboard Activated"));
    } else {
        showText(QStringLiteral("input-keyboard-virtual-off"), i18nc("on screen keyboard was disabled because physical keyboard was plugged in, keep short", "On-Screen Keyboard Deactivated"));
    }
}

bool Osd::init()
{
    if (m_config && !KConfigGroup(m_config, QStringLiteral("OSD")).readEntry(QStringLiteral("Enabled"), true)) {
        return false;
    }

    if (m_osdObject && m_osdObject->rootObject()) {
        return true;
    }

    if (m_osdUrl.isEmpty()) {
        return false;
    }

    if (!m_osdObject) {
        m_osdObject = new KDeclarative::QmlObjectSharedEngine(this);
    }

    m_osdObject->setSource(m_osdUrl);

    if (m_osdObject->status() != QQmlComponent::Ready) {
        qWarning() << "Failed to load OSD QML file" << m_osdUrl;
        return false;
    }

    m_timeout = m_osdObject->rootObject()->property("timeout").toInt();

    if (!m_osdTimer) {
        m_osdTimer = new QTimer(this);
        m_osdTimer->setSingleShot(true);
        connect(m_osdTimer, &QTimer::timeout, this, &Osd::hideOsd);
    }

    return true;
}

void Osd::showProgress(const QString &icon, const int percent, const int maximumPercent, const QString &additionalText)
{
    if (!init()) {
        return;
    }

    auto *rootObject = m_osdObject->rootObject();
    int value = qBound(0, percent, maximumPercent);
    rootObject->setProperty("osdValue", value);
    rootObject->setProperty("osdMaxValue", maximumPercent);
    rootObject->setProperty("osdAdditionalText", additionalText);
    rootObject->setProperty("showingProgress", true);
    rootObject->setProperty("icon", icon);

    emit osdProgress(icon, value, additionalText);
    showOsd();
}

void Osd::showText(const QString &icon, const QString &text)
{
    if (!init()) {
        return;
    }

    auto *rootObject = m_osdObject->rootObject();

    rootObject->setProperty("showingProgress", false);
    rootObject->setProperty("osdValue", text);
    rootObject->setProperty("icon", icon);

    emit osdText(icon, text);
    showOsd();
}

void Osd::showOsd()
{
    m_osdTimer->stop();

    auto *rootObject = m_osdObject->rootObject();

    // if our OSD understands animating the opacity, do it;
    // otherwise just show it to not break existing lnf packages
    if (rootObject->property("animateOpacity").isValid()) {
        rootObject->setProperty("animateOpacity", false);
        rootObject->setProperty("opacity", 1);
        rootObject->setProperty("visible", true);
        rootObject->setProperty("animateOpacity", true);
        rootObject->setProperty("opacity", 0);
    } else {
        rootObject->setProperty("visible", true);
    }

    m_osdTimer->start(m_timeout);
}

void Osd::hideOsd()
{
    auto *rootObject = m_osdObject->rootObject();
    if (!rootObject) {
        return;
    }

    rootObject->setProperty("visible", false);

    // this is needed to prevent fading from "old" values when the OSD shows up
    rootObject->setProperty("osdValue", 0);
}
