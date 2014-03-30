/*
 *   Copyright (C) 2010 Ivan Cukic <ivan.cukic(at)kde.org>
 *   Copyright (C) 2013 Martin Klapetek <mklapetek(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SPLASH_APP_H_
#define SPLASH_APP_H_

#include <QObject>
#include <QApplication>
#include <QBasicTimer>

class SplashWindow;

class SplashApp: public QApplication
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KSplash")

public:
    explicit SplashApp(int &argc, char ** argv);
    virtual ~SplashApp();

public Q_SLOTS:
    Q_SCRIPTABLE void setStage(const QString &messgae);

protected:
    void timerEvent(QTimerEvent *event);
    void setStage(int stage);

private:
    int m_stage;
    QList<SplashWindow *> m_windows;
    bool m_testing;
    bool m_window;
    QStringList m_stages;
    QBasicTimer m_timer;
    QDesktopWidget *m_desktop;

private Q_SLOTS:
    void screenGeometryChanged(int newCount);
};

#endif // SPLASH_APP_H_
