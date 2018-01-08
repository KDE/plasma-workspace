/*
 * Holds one embedded window, registers as DBus entry
 * Copyright (C) 2015 <davidedmundson@kde.org> David Edmundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "sniproxy.h"

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_image.h>

#include "xcbutils.h"
#include "debug.h"

#include <QX11Info>
#include <QScreen>
#include <QGuiApplication>
#include <QTimer>

#include <QBitmap>

#include <KWindowSystem>
#include <netwm.h>

#include "statusnotifieritemadaptor.h"
#include "statusnotifierwatcher_interface.h"

#define SNI_WATCHER_SERVICE_NAME "org.kde.StatusNotifierWatcher"
#define SNI_WATCHER_PATH "/StatusNotifierWatcher"

static uint16_t s_embedSize = 32; //max size of window to embed. We no longer resize the embedded window as Chromium acts stupidly.

int SNIProxy::s_serviceCount = 0;

void
xembed_message_send(xcb_window_t towin,
                    long message, long d1, long d2, long d3)
{
    xcb_client_message_event_t ev;

    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.window = towin;
    ev.format = 32;
    ev.data.data32[0] = XCB_CURRENT_TIME;
    ev.data.data32[1] = message;
    ev.data.data32[2] = d1;
    ev.data.data32[3] = d2;
    ev.data.data32[4] = d3;
    ev.type = Xcb::atoms->xembedAtom;
    xcb_send_event(QX11Info::connection(), false, towin, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
}

SNIProxy::SNIProxy(xcb_window_t wid, QObject* parent):
    QObject(parent),
    //Work round a bug in our SNIWatcher with multiple SNIs per connection.
    //there is an undocumented feature that you can register an SNI by path, however it doesn't detect an object on a service being removed, only the entire service closing
    //instead lets use one DBus connection per SNI
    m_dbus(QDBusConnection::connectToBus(QDBusConnection::SessionBus, QStringLiteral("XembedSniProxy%1").arg(s_serviceCount++))),
    m_windowId(wid)
{
    //create new SNI
    new StatusNotifierItemAdaptor(this);
    m_dbus.registerObject(QStringLiteral("/StatusNotifierItem"), this);

    auto statusNotifierWatcher = new org::kde::StatusNotifierWatcher(QStringLiteral(SNI_WATCHER_SERVICE_NAME), QStringLiteral(SNI_WATCHER_PATH), QDBusConnection::sessionBus(), this);
    auto reply = statusNotifierWatcher->RegisterStatusNotifierItem(m_dbus.baseService());
    reply.waitForFinished();
    if (reply.isError()) {
        qCWarning(SNIPROXY) << "could not register SNI:" << reply.error().message();
    }

    auto c = QX11Info::connection();

    auto cookie = xcb_get_geometry(c, m_windowId);
    QScopedPointer<xcb_get_geometry_reply_t, QScopedPointerPodDeleter>
        clientGeom(xcb_get_geometry_reply(c, cookie, Q_NULLPTR));

    //create a container window
    auto screen = xcb_setup_roots_iterator (xcb_get_setup (c)).data;
    m_containerWid = xcb_generate_id(c);
    uint32_t values[2];
    auto mask = XCB_CW_BACK_PIXEL | XCB_CW_OVERRIDE_REDIRECT;
    values[0] = screen->black_pixel; //draw a solid background so the embeded icon doesn't get garbage in it
    values[1] = true; //bypass wM
    xcb_create_window (c,                          /* connection    */
                    XCB_COPY_FROM_PARENT,          /* depth         */
                     m_containerWid,               /* window Id     */
                     screen->root,                 /* parent window */
                     0, 0,                         /* x, y          */
                     s_embedSize, s_embedSize,     /* width, height */
                     0,                            /* border_width  */
                     XCB_WINDOW_CLASS_INPUT_OUTPUT,/* class         */
                     screen->root_visual,          /* visual        */
                     mask, values);                /* masks         */

    /*
        We need the window to exist and be mapped otherwise the child won't render it's contents

        We also need it to exist in the right place to get the clicks working as GTK will check sendEvent locations to see if our window is in the right place. So even though our contents are drawn via compositing we still put this window in the right place

        We can't composite it away anything parented owned by the root window (apparently)
        Stack Under works in the non composited case, but it doesn't seem to work in kwin's composited case (probably need set relevant NETWM hint)

        As a last resort set opacity to 0 just to make sure this container never appears
    */

#ifndef VISUAL_DEBUG
    const uint32_t stackBelowData[] = {XCB_STACK_MODE_BELOW};
    xcb_configure_window(c, m_containerWid, XCB_CONFIG_WINDOW_STACK_MODE, stackBelowData);

    NETWinInfo wm(c, m_containerWid, screen->root, 0, 0);
    wm.setOpacity(0);
#endif

    xcb_flush(c);

    xcb_map_window(c, m_containerWid);

    xcb_reparent_window(c, wid,
                        m_containerWid,
                        0, 0);

    /*
     * Render the embedded window offscreen
     */
    xcb_composite_redirect_window(c, wid, XCB_COMPOSITE_REDIRECT_MANUAL);


    /* we grab the window, but also make sure it's automatically reparented back
     * to the root window if we should die.
    */
    xcb_change_save_set(c, XCB_SET_MODE_INSERT, wid);

    //tell client we're embedding it
    xembed_message_send(wid, XEMBED_EMBEDDED_NOTIFY, m_containerWid, 0, 0);

    //move window we're embedding
    const uint32_t windowMoveConfigVals[2] = { 0, 0 };

    xcb_configure_window(c, wid,
                             XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
                             windowMoveConfigVals);


    QSize clientWindowSize;

    if (clientGeom) {
        clientWindowSize = QSize(clientGeom->width, clientGeom->height);
    }
    //if the window is a clearly stupid size resize to be something sensible
    //this is needed as chormium and such when resized just fill the icon with transparent space and only draw in the middle
    //however spotify does need this as by default the window size is 900px wide.
    //use an artbitrary heuristic to make sure icons are always sensible
    if (clientWindowSize.isEmpty() || clientWindowSize.width() > s_embedSize || clientWindowSize.height() > s_embedSize )
    {
        qCDebug(SNIPROXY) << "Resizing window" << wid << Title() << "from w*h" << clientWindowSize;

        xcb_configure_notify_event_t event;
        memset(&event, 0x00, sizeof(xcb_configure_notify_event_t));
        event.response_type = XCB_CONFIGURE_NOTIFY;
        event.event = wid;
        event.window = wid;
        event.width = s_embedSize;
        event.height = s_embedSize;
        xcb_send_event(c, false, wid, XCB_EVENT_MASK_STRUCTURE_NOTIFY, (char *) &event);

        const uint32_t windowMoveConfigVals[2] = { s_embedSize, s_embedSize };
        xcb_configure_window(c, wid,
                                XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT,
                                windowMoveConfigVals);

        clientWindowSize = QSize(s_embedSize, s_embedSize);
    }

    //show the embedded window otherwise nothing happens
    xcb_map_window(c, wid);

    xcb_clear_area(c, 0, wid, 0, 0, clientWindowSize.width(), clientWindowSize.height());

    xcb_flush(c);

    //there's no damage event for the first paint, and sometimes it's not drawn immediately
    //not ideal, but it works better than nothing
    //test with xchat before changing
    QTimer::singleShot(500, this, &SNIProxy::update);
}

SNIProxy::~SNIProxy()
{
    auto c = QX11Info::connection();

    xcb_destroy_window(c, m_containerWid);
    QDBusConnection::disconnectFromBus(m_dbus.name());
}

void SNIProxy::update()
{
    const QImage image = getImageNonComposite();
    if (image.isNull()) {
        qCDebug(SNIPROXY) << "No xembed icon for" << m_windowId << Title();
        return;
    }

    int w = image.width();
    int h = image.height();

    m_pixmap = QPixmap::fromImage(image);
    if (w != s_embedSize || h != s_embedSize) {
        qCDebug(SNIPROXY) << "Scaling pixmap of window" << m_windowId << Title() << "from w*h" << w << h;
        m_pixmap = m_pixmap.scaled(s_embedSize, s_embedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    emit NewIcon();
    emit NewToolTip();
}

void sni_cleanup_xcb_image(void *data) {
    xcb_image_destroy(static_cast<xcb_image_t*>(data));
}

bool SNIProxy::isTransparentImage(const QImage& image) const
{
    int w = image.width();
    int h = image.height();

    // check for the center and sub-center pixels first and avoid full image scan
    if (! (qAlpha(image.pixel(w >> 1, h >> 1)) + qAlpha(image.pixel(w >> 2, h >> 2)) == 0))
        return false;

    // skip scan altogether if sub-center pixel found to be opaque
    // and break out from the outer loop too on full scan
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            if (qAlpha(image.pixel(x, y))) {
                // Found an opaque pixel.
                return false;
            }
        }
    }

    return true;
}

QImage SNIProxy::getImageNonComposite() const
{
    auto c = QX11Info::connection();
    auto cookie = xcb_get_geometry(c, m_windowId);
    QScopedPointer<xcb_get_geometry_reply_t, QScopedPointerPodDeleter>
        geom(xcb_get_geometry_reply(c, cookie, Q_NULLPTR));

    if (!geom) {
        return QImage();
    }

    xcb_image_t *image = xcb_image_get(c, m_windowId, 0, 0, geom->width, geom->height, 0xFFFFFFFF, XCB_IMAGE_FORMAT_Z_PIXMAP);

    // Don't hook up cleanup yet, we may use a different QImage after all
    QImage naiveConversion;
    if (image) {
        naiveConversion = QImage(image->data, image->width, image->height, QImage::Format_ARGB32);
    } else {
        qCDebug(SNIPROXY) << "Skip NULL image returned from xcb_image_get() for" << m_windowId << Title();
        return QImage();
    }

    if (isTransparentImage(naiveConversion)) {
        QImage elaborateConversion = QImage(convertFromNative(image));

        // Update icon only if it is at least partially opaque.
        // This is just a workaround for X11 bug: xembed icon may suddenly
        // become transparent for a one or few frames. Reproducible at least
        // with WINE applications.
        if (isTransparentImage(elaborateConversion)) {
            qCDebug(SNIPROXY) << "Skip transparent xembed icon for" << m_windowId << Title();
            return QImage();
        } else
            return elaborateConversion;
    } else {
        // Now we are sure we can eventually delete the xcb_image_t with this version
        return QImage(image->data, image->width, image->height, image->stride, QImage::Format_ARGB32, sni_cleanup_xcb_image, image);
    }
}

QImage SNIProxy::convertFromNative(xcb_image_t *xcbImage) const
{
    QImage::Format format = QImage::Format_Invalid;

    switch (xcbImage->depth) {
    case 1:
        format = QImage::Format_MonoLSB;
        break;
    case 16:
        format = QImage::Format_RGB16;
        break;
    case 24:
        format = QImage::Format_RGB32;
        break;
    case 30: {
        // Qt doesn't have a matching image format. We need to convert manually
        quint32 *pixels = reinterpret_cast<quint32 *>(xcbImage->data);
        for (uint i = 0; i < (xcbImage->size / 4); i++) {
            int r = (pixels[i] >> 22) & 0xff;
            int g = (pixels[i] >> 12) & 0xff;
            int b = (pixels[i] >>  2) & 0xff;

            pixels[i] = qRgba(r, g, b, 0xff);
        }
        // fall through, Qt format is still Format_ARGB32_Premultiplied
    }
    case 32:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        return QImage(); // we don't know
    }

    QImage image(xcbImage->data, xcbImage->width, xcbImage->height, xcbImage->stride, format, sni_cleanup_xcb_image, xcbImage);

    if (image.isNull()) {
        return QImage();
    }

    if (format == QImage::Format_RGB32 && xcbImage->bpp == 32)
    {
        QImage m = image.createHeuristicMask();
        QBitmap mask(QPixmap::fromImage(m));
        QPixmap p = QPixmap::fromImage(image);
        p.setMask(mask);
        image = p.toImage();
    }

    // work around an abort in QImage::color
    if (image.format() == QImage::Format_MonoLSB) {
        image.setColorCount(2);
        image.setColor(0, QColor(Qt::white).rgb());
        image.setColor(1, QColor(Qt::black).rgb());
    }

    return image;
}

//____________properties__________

QString SNIProxy::Category() const
{
    return QStringLiteral("ApplicationStatus");
}

QString SNIProxy::Id() const
{
    const auto title = Title();
    //we always need /some/ ID so if no window title exists, just use the winId.
    if (title.isEmpty()) {
        return QString::number(m_windowId);
    }
    return title;
}

KDbusImageVector SNIProxy::IconPixmap() const
{
    KDbusImageStruct dbusImage(m_pixmap.toImage());
    return KDbusImageVector() << dbusImage;
}

bool SNIProxy::ItemIsMenu() const
{
    return false;
}

QString SNIProxy::Status() const
{
    return QStringLiteral("Active");
}

QString SNIProxy::Title() const
{
    KWindowInfo window (m_windowId, NET::WMName);
    return window.name();
}

int SNIProxy::WindowId() const
{
    return m_windowId;
}

//____________actions_____________

void SNIProxy::Activate(int x, int y)
{
    sendClick(XCB_BUTTON_INDEX_1, x, y);
}

void SNIProxy::SecondaryActivate(int x, int y)
{
    sendClick(XCB_BUTTON_INDEX_2, x, y);
}

void SNIProxy::ContextMenu(int x, int y)
{
    sendClick(XCB_BUTTON_INDEX_3, x, y);
}

void SNIProxy::Scroll(int delta, const QString& orientation)
{
    if (orientation == QLatin1String("vertical")) {
        sendClick(delta > 0 ? XCB_BUTTON_INDEX_4: XCB_BUTTON_INDEX_5, 0, 0);
    } else {
        sendClick(delta > 0 ? 6: 7, 0, 0);
    }
}

void SNIProxy::sendClick(uint8_t mouseButton, int x, int y)
{
    //it's best not to look at this code
    //GTK doesn't like send_events and double checks the mouse position matches where the window is and is top level
    //in order to solve this we move the embed container over to where the mouse is then replay the event using send_event
    //if patching, test with xchat + xchat context menus

    //note x,y are not actually where the mouse is, but the plasmoid
    //ideally we should make this match the plasmoid hit area

    qCDebug(SNIPROXY) << "Received click" << mouseButton << "with passed x*y" << x << y;

    auto c = QX11Info::connection();

    auto cookieSize = xcb_get_geometry(c, m_windowId);
    QScopedPointer<xcb_get_geometry_reply_t, QScopedPointerPodDeleter>
        clientGeom(xcb_get_geometry_reply(c, cookieSize, Q_NULLPTR));

    if (!clientGeom) {
        return;
    }

    auto cookie = xcb_query_pointer(c, m_windowId);
    QScopedPointer<xcb_query_pointer_reply_t, QScopedPointerPodDeleter>
        pointer(xcb_query_pointer_reply(c, cookie, Q_NULLPTR));
    /*qCDebug(SNIPROXY) << "samescreen" << pointer->same_screen << endl
	<< "root x*y" << pointer->root_x << pointer->root_y << endl
	<< "win x*y" << pointer->win_x << pointer->win_y;*/

    //move our window so the mouse is within its geometry
    uint32_t configVals[2] = {0, 0};
    if (mouseButton >= XCB_BUTTON_INDEX_4) {
	//scroll event, take pointer position
	configVals[0] = pointer->root_x;
	configVals[1] = pointer->root_y;
    } else {
	if (pointer->root_x > x + clientGeom->width)
	    configVals[0] = pointer->root_x - clientGeom->width + 1;
	else
	    configVals[0] = static_cast<uint32_t>(x);
	if (pointer->root_y > y + clientGeom->height)
	    configVals[1] = pointer->root_y - clientGeom->height + 1;
	else
	    configVals[1] = static_cast<uint32_t>(y);
    }
    xcb_configure_window(c, m_containerWid, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, configVals);

    //pull window up
    const uint32_t stackAboveData[] = {XCB_STACK_MODE_ABOVE};
    xcb_configure_window(c, m_containerWid, XCB_CONFIG_WINDOW_STACK_MODE, stackAboveData);

    //mouse down
    {
        xcb_button_press_event_t* event = new xcb_button_press_event_t;
        memset(event, 0x00, sizeof(xcb_button_press_event_t));
        event->response_type = XCB_BUTTON_PRESS;
        event->event = m_windowId;
        event->time = QX11Info::getTimestamp();
        event->same_screen = 1;
        event->root = QX11Info::appRootWindow();
        event->root_x = x;
        event->root_y = y;
        event->event_x = 0;
        event->event_y = 0;
        event->child = 0;
        event->state = 0;
        event->detail = mouseButton;

        xcb_send_event(c, false, m_windowId, XCB_EVENT_MASK_BUTTON_PRESS, (char *) event);
        delete event;
    }

    //mouse up
    {
        xcb_button_release_event_t* event = new xcb_button_release_event_t;
        memset(event, 0x00, sizeof(xcb_button_release_event_t));
        event->response_type = XCB_BUTTON_RELEASE;
        event->event = m_windowId;
        event->time = QX11Info::getTimestamp();
        event->same_screen = 1;
        event->root = QX11Info::appRootWindow();
        event->root_x = x;
        event->root_y = y;
        event->event_x = 0;
        event->event_y = 0;
        event->child = 0;
        event->state = 0;
        event->detail = mouseButton;

        xcb_send_event(c, false, m_windowId, XCB_EVENT_MASK_BUTTON_RELEASE, (char *) event);
        delete event;
    }

#ifndef VISUAL_DEBUG
    const uint32_t stackBelowData[] = {XCB_STACK_MODE_BELOW};
    xcb_configure_window(c, m_containerWid, XCB_CONFIG_WINDOW_STACK_MODE, stackBelowData);
#endif
}
