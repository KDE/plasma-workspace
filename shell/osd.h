/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <KConfigGroup>
#include <KSharedConfig>

namespace KDeclarative
{
class QmlObjectSharedEngine;
}
namespace Plasma
{
}

class QTimer;
class ShellCorona;

class Osd : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.osdService")
public:
    Osd(const KSharedConfig::Ptr &config, ShellCorona *corona);
    ~Osd() override;

public Q_SLOTS:
    void brightnessChanged(int percent);
    void keyboardBrightnessChanged(int percent);
    void volumeChanged(int percent);
    void volumeChanged(int percent, int maximumPercent);
    void microphoneVolumeChanged(int percent);
    void mediaPlayerVolumeChanged(int percent, const QString &playerName, const QString &playerIconName);
    void kbdLayoutChanged(const QString &layoutName);
    void virtualDesktopChanged(const QString &currentVirtualDesktopName);
    void touchpadEnabledChanged(bool touchpadEnabled);
    void wifiEnabledChanged(bool wifiEnabled);
    void bluetoothEnabledChanged(bool bluetoothEnabled);
    void wwanEnabledChanged(bool wwanEnabled);
    void virtualKeyboardEnabledChanged(bool virtualKeyboardEnabled);
    void showText(const QString &icon, const QString &text);

Q_SIGNALS:
    void osdProgress(const QString &icon, const int percent, const QString &additionalText);
    void osdText(const QString &icon, const QString &text);

private Q_SLOTS:
    void hideOsd();

private:
    bool init();

    void showProgress(const QString &icon, const int percent, const int maximumPercent, const QString &additionalText = QString());
    void showOsd();

    QUrl m_osdUrl;
    KDeclarative::QmlObjectSharedEngine *m_osdObject = nullptr;
    QTimer *m_osdTimer = nullptr;
    int m_timeout = 0;

    KConfigGroup m_osdConfigGroup;
};
