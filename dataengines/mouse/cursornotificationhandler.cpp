/*
    SPDX-FileCopyrightText: 2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "cursornotificationhandler.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

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

    Q_EMIT cursorNameChanged(cursorName(currentName));

    return false;
}
