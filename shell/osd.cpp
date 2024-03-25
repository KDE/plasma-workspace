/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "osd.h"
#include "debug.h"
#include "shellcorona.h"

#include <QDBusConnection>
#include <QDebug>
#include <QTimer>
#include <QWindow>

#include <PlasmaQuick/SharedQmlEngine>
#include <klocalizedstring.h>

using namespace Qt::StringLiterals;

Osd::Osd(const KSharedConfig::Ptr &config, ShellCorona *corona)
    : QObject(corona)
    , m_corona(corona)
    , m_osdConfigGroup(config, u"OSD"_s)
{
    QDBusConnection::sessionBus().registerObject(u"/org/kde/osdService"_s, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
}

Osd::~Osd()
{
}

void Osd::brightnessChanged(int percent)
{
    showProgress(u"video-display-brightness"_s, percent, 100);
}

void Osd::keyboardBrightnessChanged(int percent)
{
    showProgress(u"input-keyboard-brightness"_s, percent, 100);
}

void Osd::volumeChanged(int percent)
{
    volumeChanged(percent, 100);
}

void Osd::volumeChanged(int percent, int maximumPercent)
{
    QString icon;
    if (percent <= 0) {
        icon = u"audio-volume-muted"_s;
        showText(icon, i18nc("OSD informing that the system is muted, keep short", "Audio Muted"));
        return;
    } else if (percent <= 25) {
        icon = u"audio-volume-low"_s;
    } else if (percent <= 75) {
        icon = u"audio-volume-medium"_s;
    } else if (percent <= 100) {
        icon = u"audio-volume-high"_s;
    } else if (percent <= 125) {
        icon = u"audio-volume-high-warning"_s;
    } else {
        icon = u"audio-volume-high-danger"_s;
    }

    showProgress(icon, percent, maximumPercent);
}

void Osd::microphoneVolumeChanged(int percent)
{
    QString icon;
    if (percent <= 0) {
        icon = u"microphone-sensitivity-muted"_s;
        showText(icon, i18nc("OSD informing that the microphone is muted, keep short", "Microphone Muted"));
        return;
    } else if (percent <= 25) {
        icon = u"microphone-sensitivity-low"_s;
    } else if (percent <= 75) {
        icon = u"microphone-sensitivity-medium"_s;
    } else {
        icon = u"microphone-sensitivity-high"_s;
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
    if (m_osdConfigGroup.readEntry("kbdLayoutChangedEnabled", true)) {
        showText(u"keyboard-layout"_s, layoutName);
    }
}

void Osd::virtualDesktopChanged(const QString &currentVirtualDesktopName)
{
    // FIXME: need a VD icon
    showText(QString(), currentVirtualDesktopName);
}

void Osd::touchpadEnabledChanged(bool touchpadEnabled)
{
    if (touchpadEnabled) {
        showText(u"input-touchpad-on"_s, i18nc("touchpad was enabled, keep short", "Touchpad On"));
    } else {
        showText(u"input-touchpad-off"_s, i18nc("touchpad was disabled, keep short", "Touchpad Off"));
    }
}

void Osd::wifiEnabledChanged(bool wifiEnabled)
{
    if (wifiEnabled) {
        showText(u"network-wireless-on"_s, i18nc("wireless lan was enabled, keep short", "Wifi On"));
    } else {
        showText(u"network-wireless-off"_s, i18nc("wireless lan was disabled, keep short", "Wifi Off"));
    }
}

void Osd::bluetoothEnabledChanged(bool bluetoothEnabled)
{
    if (bluetoothEnabled) {
        showText(u"preferences-system-bluetooth"_s, i18nc("Bluetooth was enabled, keep short", "Bluetooth On"));
    } else {
        showText(u"preferences-system-bluetooth-inactive"_s, i18nc("Bluetooth was disabled, keep short", "Bluetooth Off"));
    }
}

void Osd::wwanEnabledChanged(bool wwanEnabled)
{
    if (wwanEnabled) {
        showText(u"network-mobile-on"_s, i18nc("mobile internet was enabled, keep short", "Mobile Internet On"));
    } else {
        showText(u"network-mobile-off"_s, i18nc("mobile internet was disabled, keep short", "Mobile Internet Off"));
    }
}

void Osd::virtualKeyboardEnabledChanged(bool virtualKeyboardEnabled)
{
    if (virtualKeyboardEnabled) {
        showText(u"input-keyboard-virtual-on"_s,
                 i18nc("on screen keyboard was enabled because physical keyboard got unplugged, keep short", "On-Screen Keyboard Activated"));
    } else {
        showText(u"input-keyboard-virtual-off"_s,
                 i18nc("on screen keyboard was disabled because physical keyboard was plugged in, keep short", "On-Screen Keyboard Deactivated"));
    }
}

void Osd::powerManagementInhibitedChanged(bool inhibited)
{
    if (inhibited) {
        showText(QStringLiteral("system-suspend-inhibited"), i18nc("power management was inhibited, keep short", "Sleep and Screen Locking Blocked"));
    } else {
        showText(QStringLiteral("system-suspend-uninhibited"), i18nc("power management was uninhibited, keep short", "Sleep and Screen Locking Unblocked"));
    }
}

void Osd::powerProfileChanged(const QString &profile)
{
    QString icon;
    QString name;
    if (profile == QStringLiteral("power-saver")) {
        icon = QStringLiteral("battery-profile-powersave");
        name = i18nc("Power profile was changed to power save mode, keep short", "Power Save Mode");
    } else if (profile == QStringLiteral("balanced")) {
        icon = QStringLiteral("speedometer");
        name = i18nc("Power profile was changed to balanced mode, keep short", "Balanced Power Mode");
    } else if (profile == QStringLiteral("performance")) {
        icon = QStringLiteral("battery-profile-performance");
        name = i18nc("Power profile was changed to performance mode, keep short", "Performance Mode");
    }
    showText(icon, name);
}

bool Osd::init()
{
    if (!m_osdConfigGroup.readEntry("Enabled", true)) {
        return false;
    }

    if (m_osdObject && m_osdObject->rootObject()) {
        return true;
    }

    const QUrl url = m_corona->lookAndFeelPackage().fileUrl("osdmainscript");
    if (url.isEmpty()) {
        return false;
    }

    if (!m_osdObject) {
        m_osdObject = new PlasmaQuick::SharedQmlEngine(this);
    }

    m_osdObject->setSource(url);

    if (m_osdObject->status() != QQmlComponent::Ready) {
        qCWarning(PLASMASHELL) << "Failed to load OSD QML file" << url;
        auto fallbackUrl = m_corona->lookAndFeelPackage().fallbackPackage().fileUrl("osdmainscript");
        if (fallbackUrl.isEmpty() || fallbackUrl == url) {
            return false;
        }
        qCWarning(PLASMASHELL) << "Trying fallback theme";
        m_osdObject->setSource(fallbackUrl);
        if (m_osdObject->status() != QQmlComponent::Ready) {
            qCWarning(PLASMASHELL) << "Failed to load fallback OSD QML file" << fallbackUrl;
            return false;
        }
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
    // Update max value first to prevent value from being clamped
    rootObject->setProperty("osdMaxValue", maximumPercent);
    rootObject->setProperty("osdValue", value);
    rootObject->setProperty("osdAdditionalText", additionalText);
    rootObject->setProperty("showingProgress", true);
    rootObject->setProperty("icon", icon);

    Q_EMIT osdProgress(icon, value, maximumPercent, additionalText);
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

    Q_EMIT osdText(icon, text);
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
