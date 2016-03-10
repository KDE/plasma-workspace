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

#ifndef CURSORNOTIFICATIONHANDLER_H
#define CURSORNOTIFICATIONHANDLER_H

#include <QMap>
#include <QWidget>

#include <X11/Xlib.h>
#include <fixx11h.h>

class CursorNotificationHandler : public QWidget
{
    Q_OBJECT

public:
    CursorNotificationHandler();
    ~CursorNotificationHandler() override;

    QString cursorName();

Q_SIGNALS:
    void cursorNameChanged(const QString &name);

protected:
    bool x11Event(XEvent *);

private:
    QString cursorName(Atom cursor);

private:
    bool haveXfixes;
    int fixesEventBase;
    Atom currentName;
    QMap<Atom, QString> names;
};

#endif
