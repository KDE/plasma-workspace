/*
 *   Copyright © 2007 Fredrik Höglund <fredrik@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef MOUSEENGINE_H
#define MOUSEENGINE_H

#include <QPoint>
#include <QTimerEvent>

#include <Plasma/DataEngine>

#include <config-X11.h>

#ifdef HAVE_XFIXES
class CursorNotificationHandler;
#endif

class MouseEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    MouseEngine(QObject *parent, const QVariantList &args);
    ~MouseEngine() override;

    QStringList sources() const override;

protected:
    void init();
    void timerEvent(QTimerEvent *) override;

private Q_SLOTS:
    void updateCursorName(const QString &name);

private:
    QPoint lastPosition;
    int timerId;
#ifdef HAVE_XFIXES
    CursorNotificationHandler *handler;
#endif
};

#endif
