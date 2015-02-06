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

namespace KDeclarative {
    class QmlObject;
}
namespace Plasma {
}

class QTimer;
class ShellCorona;

class Osd : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.osdService")
public:
    Osd(ShellCorona *corona);
    ~Osd();

public Q_SLOTS:
    void brightnessChanged(int percent);
    void keyboardBrightnessChanged(int percent);
    void volumeChanged(int percent);
    void mediaPlayerVolumeChanged(int percent, const QString &playerName, const QString &playerIconName);
    void kbdLayoutChanged(const QString &layoutName);
    void virtualDesktopChanged(const QString &currentVirtualDesktopName);

Q_SIGNALS:
    void osdProgress(const QString &icon, const int percent, const QString &additionalText);
    void osdText(const QString &icon, const QString &text);

private Q_SLOTS:
    void hideOsd();

private:
    void showProgress(const QString &icon, const int percent, const QString &additionalText = QString());
    void showText(const QString &icon, const QString &text);
    void showOsd();

    KDeclarative::QmlObject *m_osdObject;
    QTimer *m_osdTimer;
    int m_timeout;
};

#endif // OSD_H
