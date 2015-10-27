/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2002 Luboš Luňák <l.lunak@kde.org>
Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
Copyright (C) 2008 Chani Armitage <chanika@gmail.com>
Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

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
#include "x11locker.h"
#include "globalaccel.h"
#include "abstractlocker.h"
// KDE
#include <KLocalizedString>
// Qt
#include <QApplication>
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QDesktopWidget>
#include <QPainter>
#include <QScreen>
#include <QX11Info>
// X11
#include <X11/Xatom.h>
#include <fixx11h.h>
#include <xcb/xcb.h>

static Window gVRoot = 0;
static Window gVRootData = 0;
static Atom   gXA_VROOT;
static Atom   gXA_SCREENSAVER_VERSION;

namespace ScreenLocker
{

X11Locker::X11Locker()
    : AbstractLocker()
    , QAbstractNativeEventFilter()
{
    initialize();
}

X11Locker::~X11Locker()
{
    qApp->removeNativeEventFilter(this);
}

void X11Locker::initialize()
{
    qApp->installNativeEventFilter(this);

    XWindowAttributes rootAttr;
    XGetWindowAttributes(QX11Info::display(), QX11Info::appRootWindow(), &rootAttr);
    QApplication::desktop(); // make Qt set its event mask on the root window first
    XSelectInput( QX11Info::display(), QX11Info::appRootWindow(),
                  SubstructureNotifyMask | rootAttr.your_event_mask );
    // Get root window size
    updateGeo();

    // virtual root property
    gXA_VROOT = XInternAtom (QX11Info::display(), "__SWM_VROOT", False);
    gXA_SCREENSAVER_VERSION = XInternAtom (QX11Info::display(), "_SCREENSAVER_VERSION", False);

        // read the initial information about all toplevel windows
    Window r, p;
    Window* real;
    unsigned nreal;
    if( XQueryTree( QX11Info::display(), QX11Info::appRootWindow(), &r, &p, &real, &nreal )
        && real != NULL ) {
        for( unsigned i = 0; i < nreal; ++i ) {
            XWindowAttributes winAttr;
            if (XGetWindowAttributes(QX11Info::display(), real[ i ], &winAttr)) {
                WindowInfo info;
                info.window = real[ i ];
                info.viewable = ( winAttr.map_state == IsViewable );
                m_windowInfo.append( info ); // ordered bottom to top
            }
        }
        XFree( real );
    }
    connect(QApplication::desktop(), SIGNAL(resized(int)), SLOT(updateGeo()));
    connect(QApplication::desktop(), SIGNAL(screenCountChanged(int)), SLOT(updateGeo()));
}

void X11Locker::showLockWindow()
{
    m_background->hide();

    // Some xscreensaver hacks check for this property
    const char *version = "KDE 4.0";

    XChangeProperty (QX11Info::display(), m_background->winId(),
                     gXA_SCREENSAVER_VERSION, XA_STRING, 8, PropModeReplace,
                     (unsigned char *) version, strlen(version));

    qDebug() << "Lock window Id: " << m_background->winId();

    m_background->setPosition(0, 0);
    XSync(QX11Info::display(), False);

    setVRoot( m_background->winId(), m_background->winId() );
}

//---------------------------------------------------------------------------
//
// Hide the screen locker window
//
void X11Locker::hideLockWindow()
{
  emit userActivity();
  m_background->hide();
  m_background->lower();
  removeVRoot(m_background->winId());
  XDeleteProperty(QX11Info::display(), m_background->winId(), gXA_SCREENSAVER_VERSION);
  if ( gVRoot ) {
      unsigned long vroot_data[1] = { gVRootData };
      XChangeProperty(QX11Info::display(), gVRoot, gXA_VROOT, XA_WINDOW, 32,
                      PropModeReplace, (unsigned char *)vroot_data, 1);
      gVRoot = 0;
  }
  XSync(QX11Info::display(), False);
  m_allowedWindows.clear();
}

//---------------------------------------------------------------------------
static int ignoreXError(Display *, XErrorEvent *)
{
    return 0;
}

//---------------------------------------------------------------------------
//
// Save the current virtual root window
//
void X11Locker::saveVRoot()
{
  Window rootReturn, parentReturn, *children;
  unsigned int numChildren;
  Window root = QX11Info::appRootWindow();

  gVRoot = 0;
  gVRootData = 0;

  int (*oldHandler)(Display *, XErrorEvent *);
  oldHandler = XSetErrorHandler(ignoreXError);

  if (XQueryTree(QX11Info::display(), root, &rootReturn, &parentReturn,
      &children, &numChildren))
  {
    for (unsigned int i = 0; i < numChildren; i++)
    {
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytesafter;
      unsigned char *newRoot = 0;

      if ((XGetWindowProperty(QX11Info::display(), children[i], gXA_VROOT, 0, 1,
          False, XA_WINDOW, &actual_type, &actual_format, &nitems, &bytesafter,
          &newRoot) == Success) && newRoot)
      {
        gVRoot = children[i];
        Window *dummy = (Window*)newRoot;
        gVRootData = *dummy;
        XFree ((char*) newRoot);
        break;
      }
    }
    if (children)
    {
      XFree((char *)children);
    }
  }

  XSetErrorHandler(oldHandler);
}

//---------------------------------------------------------------------------
//
// Set the virtual root property
//
void X11Locker::setVRoot(Window win, Window vr)
{
    if (gVRoot)
        removeVRoot(gVRoot);

    unsigned long rw = QX11Info::appRootWindow();
    unsigned long vroot_data[1] = { vr };

    Window rootReturn, parentReturn, *children;
    unsigned int numChildren;
    Window top = win;
    while (1) {
        if (!XQueryTree(QX11Info::display(), top , &rootReturn, &parentReturn,
                                 &children, &numChildren))
            return;
        if (children)
            XFree((char *)children);
        if (parentReturn == rw) {
            break;
        } else
            top = parentReturn;
    }

    XChangeProperty(QX11Info::display(), top, gXA_VROOT, XA_WINDOW, 32,
                     PropModeReplace, (unsigned char *)vroot_data, 1);
}

//---------------------------------------------------------------------------
//
// Remove the virtual root property
//
void X11Locker::removeVRoot(Window win)
{
    XDeleteProperty (QX11Info::display(), win, gXA_VROOT);
}

static void fakeFocusIn( WId window )
{
    // We have keyboard grab, so this application will
    // get keyboard events even without having focus.
    // Fake FocusIn to make Qt realize it has the active
    // window, so that it will correctly show cursor in the dialog.
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xfocus.display = QX11Info::display();
    ev.xfocus.type = FocusIn;
    ev.xfocus.window = window;
    ev.xfocus.mode = NotifyNormal;
    ev.xfocus.detail = NotifyAncestor;
    XSendEvent( QX11Info::display(), window, False, NoEventMask, &ev );
    XFlush(QX11Info::display());
}

template< typename T>
void coordFromEvent(xcb_generic_event_t *event, int *x, int *y)
{
    T *e = reinterpret_cast<T*>(event);
    *x = e->event_x;
    *y = e->event_y;
}

template<typename T>
void sendEvent(xcb_generic_event_t *event, xcb_window_t target, int x, int y)
{
    T e = *(reinterpret_cast<T*>(event));
    e.event = target;
    e.child = target;
    e.event_x = x;
    e.event_y = y;
    xcb_send_event(QX11Info::connection(), false, target, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char*>(&e));
}

bool X11Locker::nativeEventFilter(const QByteArray &eventType, void *message, long int *)
{
    if (eventType != QByteArrayLiteral("xcb_generic_event_t")) {
        return false;
    }
    xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
    const uint8_t responseType = event->response_type & ~0x80;
    if (globalAccel() && responseType == XCB_KEY_PRESS) {
        if (globalAccel()->checkKeyPress(reinterpret_cast<xcb_key_press_event_t*>(event))) {
            emit userActivity();
            return true;
        }
    }
    bool ret = false;
    switch (responseType) {
        case XCB_BUTTON_PRESS:
        case XCB_BUTTON_RELEASE:
        case XCB_KEY_PRESS:
        case XCB_KEY_RELEASE:
        case XCB_MOTION_NOTIFY:
            emit userActivity();
            if (!m_lockWindows.isEmpty()) {
                int x = 0;
                int y = 0;
                if (responseType == XCB_KEY_PRESS || responseType == XCB_KEY_RELEASE) {
                    coordFromEvent<xcb_key_press_event_t>(event, &x, &y);
                } else if (responseType == XCB_BUTTON_PRESS || responseType == XCB_BUTTON_RELEASE) {
                    coordFromEvent<xcb_button_press_event_t>(event, &x, &y);
                } else if (responseType == XCB_MOTION_NOTIFY) {
                    coordFromEvent<xcb_motion_notify_event_t>(event, &x, &y);
                }
                Window root_return;
                int x_return, y_return;
                unsigned int width_return, height_return, border_width_return, depth_return;
                foreach (WId window, m_lockWindows) {
                    if (XGetGeometry(QX11Info::display(), window, &root_return,
                                &x_return, &y_return,
                                &width_return, &height_return,
                                &border_width_return, &depth_return)
                        &&
                        (x>=x_return && x<=x_return+(int)width_return)
                        &&
                        (y>=y_return && y<=y_return+(int)height_return) ) {
                        const int targetX = x - x_return;
                        const int targetY = y - y_return;
                        if (responseType == XCB_KEY_PRESS || responseType == XCB_KEY_RELEASE) {
                            sendEvent<xcb_key_press_event_t>(event, window, targetX, targetY);
                        } else if (responseType == XCB_BUTTON_PRESS || responseType == XCB_BUTTON_RELEASE) {
                            sendEvent<xcb_button_press_event_t>(event, window, targetX, targetY);
                        } else if (responseType == XCB_MOTION_NOTIFY) {
                            sendEvent<xcb_motion_notify_event_t>(event, window, targetX, targetY);
                        }
                        break;
                    }
                }
                ret = true;
            }
            break;
        case XCB_CONFIGURE_NOTIFY: { // from SubstructureNotifyMask on the root window
            xcb_configure_notify_event_t *xc = reinterpret_cast<xcb_configure_notify_event_t*>(event);
            if (xc->event == QX11Info::appRootWindow()) {
                int index = findWindowInfo( xc->window );
                if( index >= 0 ) {
                    int index2 = xc->above_sibling ? findWindowInfo( xc->above_sibling ) : 0;
                    if( index2 < 0 )
                        qDebug() << "Unknown above for ConfigureNotify";
                    else { // move just above the other window
                        if( index2 < index )
                            ++index2;
                        m_windowInfo.move( index, index2 );
                    }
                } else
                    qDebug() << "Unknown toplevel for ConfigureNotify";
                //kDebug() << "ConfigureNotify:";
                //the stacking order changed, so let's change the stacking order again to what we want
                stayOnTop();
                ret = true;
            }
            break;
        }
        case XCB_MAP_NOTIFY: { // from SubstructureNotifyMask on the root window
            xcb_map_notify_event_t *xm = reinterpret_cast<xcb_map_notify_event_t*>(event);
            if (xm->event == QX11Info::appRootWindow()) {
                qDebug() << "MapNotify:" << xm->window;
                int index = findWindowInfo( xm->window );
                if( index >= 0 )
                    m_windowInfo[ index ].viewable = true;
                else
                    qDebug() << "Unknown toplevel for MapNotify";
                if (m_allowedWindows.contains(xm->window)) {
                    if (m_lockWindows.contains(xm->window)) {
                        qDebug() << "uhoh! duplicate!";
                    } else {
                        if (!m_background->isVisible()) {
                            // not yet shown and we have a lock window, so we show our own window
                            m_background->show();
                        }
                        m_lockWindows.prepend(xm->window);
                        fakeFocusIn(xm->window);
                    }
                }
                if (xm->window == m_background->winId()) {
                    m_background->update();
                    emit lockWindowShown();
                    return false;
                }
                stayOnTop();
                ret = true;
            }
            break;
        }
        case XCB_UNMAP_NOTIFY: {
            xcb_unmap_notify_event_t *xu = reinterpret_cast<xcb_unmap_notify_event_t*>(event);
            if (xu->event == QX11Info::appRootWindow()) {
                qDebug() << "UnmapNotify:" << xu->window;
                int index = findWindowInfo( xu->window );
                if( index >= 0 )
                    m_windowInfo[ index ].viewable = false;
                else
                    qDebug() << "Unknown toplevel for MapNotify";
                m_lockWindows.removeAll(xu->event);
                ret = true;
            }
            break;
        }
        case XCB_CREATE_NOTIFY: {
            xcb_create_notify_event_t *xc = reinterpret_cast<xcb_create_notify_event_t*>(event);
            if (xc->parent == QX11Info::appRootWindow()) {
                qDebug() << "CreateNotify:" << xc->window;
                int index = findWindowInfo( xc->window );
                if( index >= 0 )
                    qDebug() << "Already existing toplevel for CreateNotify";
                else {
                    WindowInfo info;
                    info.window = xc->window;
                    info.viewable = false;
                    m_windowInfo.append( info );
                }
                ret = true;
            }
            break;
        }
        case XCB_DESTROY_NOTIFY: {
            xcb_destroy_notify_event_t *xd = reinterpret_cast<xcb_destroy_notify_event_t *>(event);
            if (xd->event == QX11Info::appRootWindow()) {
                int index = findWindowInfo( xd->window );
                if( index >= 0 )
                    m_windowInfo.removeAt( index );
                else
                    qDebug() << "Unknown toplevel for DestroyNotify";
                ret = true;
            }
            break;
        }
        case XCB_REPARENT_NOTIFY: {
            xcb_reparent_notify_event_t *xr = reinterpret_cast<xcb_reparent_notify_event_t*>(event);
            if (xr->event == QX11Info::appRootWindow() && xr->parent != QX11Info::appRootWindow()) {
                int index = findWindowInfo( xr->window );
                if( index >= 0 )
                    m_windowInfo.removeAt( index );
                else
                    qDebug() << "Unknown toplevel for ReparentNotify away";
            } else if (xr->parent == QX11Info::appRootWindow()) {
                int index = findWindowInfo( xr->window );
                if( index >= 0 )
                    qDebug() << "Already existing toplevel for ReparentNotify";
                else {
                    WindowInfo info;
                    info.window = xr->window;
                    info.viewable = false;
                    m_windowInfo.append( info );
                }
            }
            break;
        }
        case XCB_CIRCULATE_NOTIFY: {
            xcb_circulate_notify_event_t *xc = reinterpret_cast<xcb_circulate_notify_event_t*>(event);
            if (xc->event == QX11Info::appRootWindow()) {
                int index = findWindowInfo( xc->window );
                if( index >= 0 ) {
                    m_windowInfo.move( index, xc->place == PlaceOnTop ? m_windowInfo.size() - 1 : 0 );
                } else
                    qDebug() << "Unknown toplevel for CirculateNotify";
            }
            break;
        }
    }
    return ret;
}

int X11Locker::findWindowInfo(Window w)
{
    for( int i = 0;
         i < m_windowInfo.size();
         ++i )
        if( m_windowInfo[ i ].window == w )
            return i;
    return -1;
}

void X11Locker::stayOnTop()
{

    // this restacking is written in a way so that
    // if the stacking positions actually don't change,
    // all restacking operations will be no-op,
    // and no ConfigureNotify will be generated,
    // thus avoiding possible infinite loops
    QVector< Window > stack( m_lockWindows.count() + 1 );
    int count = 0;
    foreach( WId w, m_lockWindows )
        stack[ count++ ] = w;
    // finally, the lock window
    stack[ count++ ] = m_background->winId();
    // do the actual restacking if needed
    XRaiseWindow( QX11Info::display(), stack[ 0 ] );
    if( count > 1 )
        XRestackWindows( QX11Info::display(), stack.data(), count );
    XFlush(QX11Info::display());
}

void X11Locker::updateGeo()
{
    QDesktopWidget *desktop = QApplication::desktop();
    m_background->setGeometry(desktop->geometry());
    m_background->update();
}

void X11Locker::addAllowedWindow(quint32 window)
{
    m_allowedWindows << window;
    // test whether it's to show
    const int index = findWindowInfo( window );
    if (index == -1 || !m_windowInfo[ index ].viewable) {
        return;
    }
    if (m_lockWindows.contains(window)) {
        qDebug() << "uhoh! duplicate!";
    } else {
        if (!m_background->isVisible()) {
            // not yet shown and we have a lock window, so we show our own window
            m_background->show();
        }
        m_lockWindows.prepend(window);
        fakeFocusIn(window);
        stayOnTop();
    }
}

}
