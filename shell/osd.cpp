/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2024 Jakob Petsovits <jpetso@petsovits.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "osd.h"
#include "debug.h"
#include "shellcorona.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlContext>
#include <QTimer>
#include <QWindow>

#include <KLocalizedQmlContext>
#include <KRuntimePlatform>
#include <PlasmaQuick/PlasmaQuick>
#include <klocalizedstring.h>

#include <algorithm> // std::ranges::sort

using namespace Qt::StringLiterals;

Osd::Osd(const KSharedConfig::Ptr &config, ShellCorona *corona)
    : QObject(corona)
    , m_corona(corona)
    , m_engine(PlasmaQuick::globalEngine())
    , m_osdConfigGroup(config, u"OSD"_s)
{
    QDBusConnection::sessionBus().registerObject(u"/org/kde/osdService"_s, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals);
}

Osd::~Osd() = default;

void Osd::screenBrightnessChanged(int percent, const QString &displayId, const QString &displayLabel, int priority, const QRect &screenRect)
{
    // Ensure that an element in m_screenBrightnessInfo exists with unique displayId and sorting by priority
    m_screenBrightnessInfo.insert(displayId,
                                  {
                                      .id = displayId,
                                      .label = displayLabel,
                                      .screenRect = screenRect,
                                      .priority = priority,
                                      .percent = percent,
                                  });

    // Don't show screen brightness OSD on mobile, only emit event (specified in showProgress parameters)

    if (m_corona->numScreens() == 1 && m_screenBrightnessInfo.size() == 1 && screenRect == m_corona->screenGeometry(0)) {
        showProgress(u"video-display-brightness"_s, percent, 100, {}, false);
    } else if (m_screenBrightnessInfo.size() == 1) {
        showProgress(u"video-display-brightness"_s, percent, 100, displayLabel, false);
    } else {
        // TODO: show one progress OSD on each corresponding screen
        QList<ScreenBrightnessInfo> sortedByPriority = m_screenBrightnessInfo.values();
        std::ranges::sort(sortedByPriority, [](const auto &a, const auto &b) {
            return a.priority < b.priority;
        });
        QStringList percentages;
        for (const auto &info : std::as_const(sortedByPriority)) {
            percentages += i18nc("Brightness OSD: display name and brightness percentage", "%1: %2%", info.label, info.percent);
        }

        showText(u"video-display-brightness"_s, percentages.join(u"\n"_s));
    }
}

void Osd::brightnessChanged(int percent)
{
    // Only emit event, don't show OSD on Plasma Mobile
    showProgress(u"video-display-brightness"_s, percent, 100, {}, false);
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

    // Plasma Mobile supplies its own OSD, so just emit the signal but don't show it here.
    showProgress(icon, percent, maximumPercent, {}, false);
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
    if (profile == u"power-saver") {
        icon = QStringLiteral("battery-profile-powersave");
        name = i18nc("Power profile was changed to power save mode, keep short", "Power Save Mode");
    } else if (profile == u"balanced") {
        icon = QStringLiteral("speedometer");
        name = i18nc("Power profile was changed to balanced mode, keep short", "Balanced Power Mode");
    } else if (profile == u"performance") {
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

    if (m_osdObject) {
        return true;
    }

    QQmlComponent osdComponent(m_engine.get(), "org.kde.plasma.workspace.osd", "Osd");

    if (osdComponent.status() != QQmlComponent::Ready) {
        qCWarning(PLASMASHELL) << "Failed to load OSD QML file" << osdComponent.errorString();
        return false;
    }

    auto qmlContext = new QQmlContext(m_engine.get(), this);
    auto i18ncontext = new KLocalizedQmlContext(qmlContext);
    qmlContext->setContextObject(i18ncontext);
    QQmlEngine::setContextForObject(i18ncontext, qmlContext);

    m_osdObject = std::unique_ptr<QObject>(osdComponent.create(qmlContext));

    if (!m_osdObject) {
        qCWarning(PLASMASHELL) << "Failed to create OSD object" << osdComponent.errorString();
    }

    m_timeout = m_osdObject->property("timeout").toInt();

    if (!m_osdTimer) {
        m_osdTimer = new QTimer(this);
        m_osdTimer->setSingleShot(true);
        connect(m_osdTimer, &QTimer::timeout, this, &Osd::hide);
    }

    return true;
}

void Osd::showProgress(const QString &icon, const int percent, const int maximumPercent, const QString &additionalText, bool showOnMobile)
{
    if (!init()) {
        return;
    }

    int value = qBound(0, percent, maximumPercent);

    if (!showOnMobile && KRuntimePlatform::runtimePlatform().contains(u"phone"_s)) {
        Q_EMIT osdProgress(icon, value, maximumPercent, additionalText);
        return;
    }

    // Update max value first to prevent value from being clamped
    m_osdObject->setProperty("osdMaxValue", maximumPercent);
    m_osdObject->setProperty("osdValue", value);
    m_osdObject->setProperty("osdAdditionalText", additionalText);
    m_osdObject->setProperty("showingProgress", true);
    m_osdObject->setProperty("icon", icon);

    Q_EMIT osdProgress(icon, value, maximumPercent, additionalText);
    showOsd();
}

void Osd::showText(const QString &icon, const QString &text)
{
    if (!init()) {
        return;
    }

    m_osdObject->setProperty("showingProgress", false);
    m_osdObject->setProperty("osdValue", text);
    m_osdObject->setProperty("icon", icon);

    Q_EMIT osdText(icon, text);
    showOsd();
}

void Osd::showOsd()
{
    m_osdTimer->stop();

    if (!m_osdObject) {
        return;
    }

    // if our OSD understands animating the opacity, do it;
    // otherwise just show it to not break existing lnf packages
    if (m_osdObject->property("animateOpacity").isValid()) {
        m_osdObject->setProperty("animateOpacity", false);
        m_osdObject->setProperty("opacity", 1);
        m_osdObject->setProperty("visible", true);
        m_osdObject->setProperty("animateOpacity", true);
        m_osdObject->setProperty("opacity", 0);
    } else {
        m_osdObject->setProperty("visible", true);
    }

    m_osdTimer->start(m_timeout);
}

void Osd::hide()
{
    if (!m_osdObject) {
        return;
    }

    if (m_osdTimer) {
        m_osdTimer->stop();
    }

    m_osdObject->setProperty("visible", false);

    // this is needed to prevent fading from "old" values when the OSD shows up
    m_osdObject->setProperty("osdValue", 0);

    m_screenBrightnessInfo.clear();
}

#include "moc_osd.cpp"
