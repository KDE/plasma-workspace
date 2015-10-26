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
// own
#include "../lockwindow.h"
// Qt
#include <QtTest/QtTest>
#include <QWindow>
#include <QX11Info>
// xcb
#include <xcb/xcb.h>

template <typename T> using ScopedCPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

class LockWindowTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testBlankScreen();
};

xcb_screen_t *defaultScreen()
{
    int screen = QX11Info::appScreen();
    for (xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(QX11Info::connection()));
            it.rem;
            --screen, xcb_screen_next(&it)) {
        if (screen == 0) {
            return it.data;
        }
    }
    return nullptr;
}

bool isBlack()
{
    xcb_connection_t *c = QX11Info::connection();
    xcb_screen_t *screen = defaultScreen();
    const int width = screen->width_in_pixels;
    const int height = screen->height_in_pixels;
    const auto cookie = xcb_get_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, QX11Info::appRootWindow(),
                                      0, 0, width, height, ~0);
    ScopedCPointer<xcb_get_image_reply_t> xImage(xcb_get_image_reply(c, cookie, nullptr));
    if (xImage.isNull()) {
        return false;
    }

    // this operates on the assumption that X server default depth matches Qt's image format
    QImage image(xcb_get_image_data(xImage.data()), width, height,
                 xcb_get_image_data_length(xImage.data()) / height, QImage::Format_ARGB32_Premultiplied);

    for (int i = 0; i < image.width(); i++) {
        for (int j = 0; j < image.height(); j++) {
            if (QColor(image.pixel(i, j)).rgb() != qRgb(0, 0, 0)) {
                return false;
            }
        }
    }
    return true;
}

bool isColored(const QColor color, const int x, const int y, const int width, const int height)
{
    xcb_connection_t *c = QX11Info::connection();
    const auto cookie = xcb_get_image(c, XCB_IMAGE_FORMAT_Z_PIXMAP, QX11Info::appRootWindow(),
                                      x, y, width, height, ~0);
    ScopedCPointer<xcb_get_image_reply_t> xImage(xcb_get_image_reply(c, cookie, nullptr));
    if (xImage.isNull()) {
        return false;
    }

    // this operates on the assumption that X server default depth matches Qt's image format
    QImage image(xcb_get_image_data(xImage.data()), width, height,
                 xcb_get_image_data_length(xImage.data()) / height, QImage::Format_ARGB32_Premultiplied);

    for (int i = 0; i < image.width(); i++) {
        for (int j = 0; j < image.height(); j++) {
            if (QColor(image.pixel(i, j)) != color) {
                return false;
            }
        }
    }
    return true;
}

xcb_atom_t screenLockerAtom()
{
    const QByteArray atomName = QByteArrayLiteral("_KDE_SCREEN_LOCKER");
    xcb_connection_t *c = QX11Info::connection();
    const auto cookie = xcb_intern_atom(c, false, atomName.length(), atomName.constData());
    ScopedCPointer<xcb_intern_atom_reply_t> atom(xcb_intern_atom_reply(c, cookie, nullptr));
    if (atom.isNull()) {
        return XCB_ATOM_NONE;
    }
    return atom->atom;
}

void LockWindowTest::testBlankScreen()
{
    // create and show a dummy window to ensure the background doesn't start as black
    QWidget dummy;
    dummy.setWindowFlags(Qt::X11BypassWindowManagerHint);
    QPalette p;
    p.setColor(QPalette::Background, Qt::red);
    dummy.setAutoFillBackground(true);
    dummy.setPalette(p);
    dummy.setGeometry(0, 0, 100, 100);
    dummy.show();
    xcb_flush(QX11Info::connection());

    // Lets wait till it gets shown
    QTest::qWait(1000);

    // Verify that red window is shown
    QVERIFY(isColored(Qt::red, 0, 0, 100, 100));

    ScreenLocker::X11Locker lockWindow;
    lockWindow.showLockWindow();

    // the screen used to be blanked once the first lock window gets mapped, so let's create one
    QWindow fakeWindow;
    fakeWindow.setFlags(Qt::X11BypassWindowManagerHint);
    // it's on purpose outside the visual area
    fakeWindow.setGeometry(-1, -1, 1, 1);
    fakeWindow.create();
    xcb_atom_t atom = screenLockerAtom();
    QVERIFY(atom != XCB_ATOM_NONE);
    xcb_connection_t *c = QX11Info::connection();
    xcb_change_property(c, XCB_PROP_MODE_REPLACE, fakeWindow.winId(), atom, atom, 32, 0, nullptr);
    xcb_flush(c);
    fakeWindow.show();

    // give it time to be shown
    QTest::qWait(1000);

    // now lets try to get a screen grab and verify it's not yet black
    QVERIFY(!isBlack());

    // nowadays we need to pass the window id to the lock window
    lockWindow.addAllowedWindow(fakeWindow.winId());

    // give it time to be shown
    QTest::qWait(1000);

    // now lets try to get a screen grab and verify it's black
    QVERIFY(isBlack());
    dummy.hide();

    // destorying the fakeWindow should not remove the blanked screen
    fakeWindow.destroy();
    QTest::qWait(1000);
    QVERIFY(isBlack());

    // let's create another window and try to raise it above the lockWindow
    // using a QWidget to get proper content which won't be black
    QWidget widgetWindow;
    widgetWindow.setGeometry(10, 10, 100, 100);
    QPalette p1;
    p1.setColor(QPalette::Background, Qt::blue);
    widgetWindow.setAutoFillBackground(true);
    widgetWindow.setPalette(p1);
    widgetWindow.show();
    const uint32_t values[] = { XCB_STACK_MODE_ABOVE };
    xcb_configure_window(c, widgetWindow.winId(), XCB_CONFIG_WINDOW_STACK_MODE, values);
    xcb_flush(c);
    QTest::qWait(1000);
    QVERIFY(isBlack());

    lockWindow.hideLockWindow();
}

QTEST_MAIN(LockWindowTest)
#include "lockwindowtest.moc"
