/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include <QCoreApplication>
#include <xcb/xcb.h>

xcb_screen_t *defaultScreen(xcb_connection_t *c, int screen)
{
    for (auto it = xcb_setup_roots_iterator(xcb_get_setup(c)); it.rem; --screen, xcb_screen_next(&it)) {
        if (screen == 0) {
            return it.data;
        }
    }

    return nullptr;
}

xcb_window_t rootWindow(xcb_connection_t *c, int screen)
{
    xcb_screen_t *s = defaultScreen(c, screen);
    if (!s) {
        return XCB_WINDOW_NONE;
    }
    return s->root;
}

/**
 * This app grabs the keyboard from X.
 * It is used from ksldtest to verify we report if grabbing failed when the keyboard has been grabbed by another
 * X Client. It needs to be another application, otherwise the grab cannot fail.
 **/
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // connect to xcb
    int screen = 0;
    xcb_connection_t *c = xcb_connect(nullptr, &screen);
    Q_ASSERT(c);

    xcb_grab_keyboard(c, 1, rootWindow(c, screen), XCB_CURRENT_TIME, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    xcb_flush(c);

    const int exitCode = app.exec();

    xcb_ungrab_keyboard(c, XCB_CURRENT_TIME);
    xcb_flush(c);
    xcb_disconnect(c);

    return exitCode;
}
