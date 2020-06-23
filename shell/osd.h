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

#ifndef OSD_H
#define OSD_H

#include <QObject>
#include <QString>
#include <QUrl>

#include <KSharedConfig>

namespace KDeclarative {
    class QmlObjectSharedEngine;
}
namespace Plasma {
}

class QTimer;
class ShellCorona;

class Osd : public QObject {
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

    KSharedConfig::Ptr m_config;
};

#endif // OSD_H
