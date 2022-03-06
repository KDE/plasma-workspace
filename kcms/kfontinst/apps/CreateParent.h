#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QLatin1String>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif
#include <X11/Xlib.h>

//
// *Very* hacky way to get some KDE dialogs to appear to be transient
// for 'xid'
//
// Create's a QWidget with size 0/0 and no border, makes this transient
// for xid, and all other widgets can use this as their parent...
static QWidget *createParent(int xid)
{
    if (!xid)
        return nullptr;

    QWidget *parent = new QWidget(nullptr, Qt::FramelessWindowHint);

    parent->resize(1, 1);
    parent->show();

    XWindowAttributes attr;
    int rx, ry;
    Window junkwin;

    XSetTransientForHint(QX11Info::display(), parent->winId(), xid);
    if (XGetWindowAttributes(QX11Info::display(), xid, &attr)) {
        XTranslateCoordinates(QX11Info::display(), xid, attr.root, -attr.border_width, -16, &rx, &ry, &junkwin);

        rx = (rx + (attr.width / 2));
        if (rx < 0)
            rx = 0;
        ry = (ry + (attr.height / 2));
        if (ry < 0)
            ry = 0;
        parent->move(rx, ry);
    }
    parent->setWindowOpacity(0);
    parent->setWindowTitle(QLatin1String("KFI"));

    return parent;
}
