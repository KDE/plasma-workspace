/*
 * Holds one embedded window, registers as DBus entry
 * Copyright (C) 2015 <davidedmundson@kde.org> David Edmundson
 * Copyright (C) 2019 <materka@gmail.com> Konrad Materka
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

#ifndef SNI_PROXY_H
#define SNI_PROXY_H

#include <QObject>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QPixmap>
#include <QPoint>

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#include "snidbus.h"

class SNIProxy : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString Category READ Category)
    Q_PROPERTY(QString Id READ Id)
    Q_PROPERTY(QString Title READ Title)
    Q_PROPERTY(QString Status READ Status)
    Q_PROPERTY(int WindowId READ WindowId)
    Q_PROPERTY(bool ItemIsMenu READ ItemIsMenu)
    Q_PROPERTY(KDbusImageVector IconPixmap READ IconPixmap)

public:
    explicit SNIProxy(xcb_window_t wid, QObject *parent = nullptr);
    ~SNIProxy() override;

    void update();
    void resizeWindow(const uint16_t width, const uint16_t height) const;
    void hideContainerWindow(xcb_window_t windowId) const;

    /**
     * @return the category of the application associated to this item
     * @see Category
     */
    QString Category() const;

    /**
     * @return the id of this item
     */
    QString Id() const;

    /**
     * @return the title of this item
     */
    QString Title() const;

    /**
     * @return The status of this item
     * @see Status
     */
    QString Status() const;

    /**
     * @return The id of the main window of the application that controls the item
     */
    int WindowId() const;

    /**
     * @return The item only support the context menu, the visualization should prefer sending ContextMenu() instead of Activate()
     */
    bool ItemIsMenu() const;

    /**
     * @return a serialization of the icon data
     */
    KDbusImageVector IconPixmap() const;

public Q_SLOTS:
    //interaction
    /**
     * Shows the context menu associated to this item
     * at the desired screen position
     */
    void ContextMenu(int x, int y);

    /**
     * Shows the main widget and try to position it on top
     * of the other windows, if the widget is already visible, hide it.
     */
    void Activate(int x, int y);

    /**
     * The user activated the item in an alternate way (for instance with middle mouse button, this depends from the systray implementation)
     */
    void SecondaryActivate(int x, int y);

    /**
     * Inform this item that the mouse wheel was used on its representation
     */
    void Scroll(int delta, const QString &orientation);

Q_SIGNALS:
    /**
     * Inform the systemtray that the own main icon has been changed,
     * so should be reloaded
     */
    void NewIcon();

    /**
     * Inform the systemtray that there is a new icon to be used as overlay
     */
    void NewOverlayIcon();

    /**
     * Inform the systemtray that the requesting attention icon
     * has been changed, so should be reloaded
     */
    void NewAttentionIcon();

    /**
     * Inform the systemtray that something in the tooltip has been changed
     */
    void NewToolTip();

    /**
     * Signal the new status when it has been changed
     * @see Status
     */
    void NewStatus(const QString &status);

private:
    enum InjectMode {
        Direct,
        XTest
    };

    QSize calculateClientWindowSize() const;
    void sendClick(uint8_t mouseButton, int x, int y);
    QImage getImageNonComposite() const;
    bool isTransparentImage(const QImage &image) const;
    QImage convertFromNative(xcb_image_t *xcbImage) const;
    QPoint calculateClickPoint() const;
    void stackContainerWindow(const uint32_t stackMode) const;

    QDBusConnection m_dbus;
    xcb_window_t m_windowId;
    xcb_window_t m_containerWid;
    static int s_serviceCount;
    QPixmap m_pixmap;
    bool sendingClickEvent;
    InjectMode m_injectMode;
};

#endif // SNIPROXY_H
