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

#include "cursornotificationhandler.h"

#include <QX11Info>

#include <X11/extensions/Xfixes.h>

/*
 * This class is a QWidget because we need an X window to
 * be able to receive XFixes events. We don't actually map
 * the widget.
 */

CursorNotificationHandler::CursorNotificationHandler()
    : QWidget()
    , currentName(0)
{
    Display *dpy = QX11Info::display();
    int errorBase;
    haveXfixes = false;

    // Request cursor change notification events
    if (XFixesQueryExtension(dpy, &fixesEventBase, &errorBase)) {
        int major, minor;
        XFixesQueryVersion(dpy, &major, &minor);

        if (major >= 2) {
            XFixesSelectCursorInput(dpy, winId(), XFixesDisplayCursorNotifyMask);
            haveXfixes = true;
        }
    }
}

CursorNotificationHandler::~CursorNotificationHandler()
{
}

QString CursorNotificationHandler::cursorName()
{
    if (!haveXfixes)
        return QString();

    if (!currentName) {
        // Xfixes doesn't have a request for getting the current cursor name,
        // but it's included in the XFixesCursorImage struct.
        XFixesCursorImage *image = XFixesGetCursorImage(QX11Info::display());
        currentName = image->atom;
        XFree(image);
    }

    return cursorName(currentName);
}

QString CursorNotificationHandler::cursorName(Atom cursor)
{
    QString name;

    // XGetAtomName() is a synchronous call, so we cache the name
    // in an atom<->string map the first time we see a name
    // to keep the X server round trips down.
    if (names.contains(cursor))
        name = names[cursor];
    else {
        char *data = XGetAtomName(QX11Info::display(), cursor);
        name = QString::fromUtf8(data);
        XFree(data);

        names.insert(cursor, name);
    }

    return name;
}

bool CursorNotificationHandler::x11Event(XEvent *event)
{
    if (event->type != fixesEventBase + XFixesCursorNotify)
        return false;

    XFixesCursorNotifyEvent *xfe = reinterpret_cast<XFixesCursorNotifyEvent *>(event);
    currentName = xfe->cursor_name;

    emit cursorNameChanged(cursorName(currentName));

    return false;
}
