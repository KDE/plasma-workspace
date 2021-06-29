/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "notificationapplet.h"

#include <QClipboard>
#include <QDrag>
#include <QGuiApplication>
#include <QMetaObject>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QStyleHints>
#include <QWindow>

#include <KWindowSystem>

#include <Plasma/Containment>
#include <PlasmaQuick/Dialog>

#include "fileinfo.h"
#include "filemenu.h"
#include "globalshortcuts.h"
#include "texteditclickhandler.h"
#include "thumbnailer.h"

NotificationApplet::NotificationApplet(QObject *parent, const QVariantList &data)
    : Plasma::Applet(parent, data)
{
    static bool s_typesRegistered = false;
    if (!s_typesRegistered) {
        const char uri[] = "org.kde.plasma.private.notifications";
        qmlRegisterType<FileInfo>(uri, 2, 0, "FileInfo");
        qmlRegisterType<FileMenu>(uri, 2, 0, "FileMenu");
        qmlRegisterType<GlobalShortcuts>(uri, 2, 0, "GlobalShortcuts");
        qmlRegisterType<TextEditClickHandler>(uri, 2, 0, "TextEditClickHandler");
        qmlRegisterType<Thumbnailer>(uri, 2, 0, "Thumbnailer");
        qmlProtectModule(uri, 2);
        s_typesRegistered = true;
    }

    connect(qApp, &QGuiApplication::focusWindowChanged, this, &NotificationApplet::focussedPlasmaDialogChanged);
}

NotificationApplet::~NotificationApplet() = default;

void NotificationApplet::init()
{
}

void NotificationApplet::configChanged()
{
}

bool NotificationApplet::dragActive() const
{
    return m_dragActive;
}

int NotificationApplet::dragPixmapSize() const
{
    return m_dragPixmapSize;
}

void NotificationApplet::setDragPixmapSize(int dragPixmapSize)
{
    if (m_dragPixmapSize != dragPixmapSize) {
        m_dragPixmapSize = dragPixmapSize;
        emit dragPixmapSizeChanged();
    }
}

bool NotificationApplet::isDrag(int oldX, int oldY, int newX, int newY) const
{
    return ((QPoint(oldX, oldY) - QPoint(newX, newY)).manhattanLength() >= qApp->styleHints()->startDragDistance());
}

void NotificationApplet::startDrag(QQuickItem *item, const QUrl &url, const QString &iconName)
{
    startDrag(item, url, QIcon::fromTheme(iconName).pixmap(m_dragPixmapSize, m_dragPixmapSize));
}

void NotificationApplet::startDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap)
{
    // This allows the caller to return, making sure we don't crash if
    // the caller is destroyed mid-drag

    QMetaObject::invokeMethod(this, "doDrag", Qt::QueuedConnection, Q_ARG(QQuickItem *, item), Q_ARG(QUrl, url), Q_ARG(QPixmap, pixmap));
}

void NotificationApplet::doDrag(QQuickItem *item, const QUrl &url, const QPixmap &pixmap)
{
    if (item && item->window() && item->window()->mouseGrabberItem()) {
        item->window()->mouseGrabberItem()->ungrabMouse();
    }

    QDrag *drag = new QDrag(item);

    QMimeData *mimeData = new QMimeData();

    if (!url.isEmpty()) {
        mimeData->setUrls(QList<QUrl>() << url);
    }

    drag->setMimeData(mimeData);

    if (!pixmap.isNull()) {
        drag->setPixmap(pixmap);
    }

    m_dragActive = true;
    emit dragActiveChanged();

    drag->exec();

    m_dragActive = false;
    emit dragActiveChanged();
}

QWindow *NotificationApplet::focussedPlasmaDialog() const
{
    auto *focusWindow = qApp->focusWindow();
    if (qobject_cast<PlasmaQuick::Dialog *>(focusWindow)) {
        return focusWindow;
    }

    if (focusWindow) {
        return qobject_cast<PlasmaQuick::Dialog *>(focusWindow->transientParent());
    }

    return nullptr;
}

QQuickItem *NotificationApplet::systemTrayRepresentation() const
{
    auto *c = containment();
    if (!c) {
        return nullptr;
    }

    if (strcmp(c->metaObject()->className(), "SystemTray") != 0) {
        return nullptr;
    }

    return c->property("_plasma_graphicObject").value<QQuickItem *>();
}

void NotificationApplet::setSelectionClipboardText(const QString &text)
{
    // FIXME KDeclarative Clipboard item uses QClipboard::Mode for "mode"
    // which is an enum inaccessible from QML
    QGuiApplication::clipboard()->setText(text, QClipboard::Selection);
}

bool NotificationApplet::isPrimaryScreen(const QRect &rect) const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return false;
    }

    // HACK
    return rect == screen->geometry();
}

void NotificationApplet::forceActivateWindow(QWindow *window)
{
    if (window && window->winId()) {
        KWindowSystem::forceActiveWindow(window->winId());
    }
}

K_PLUGIN_CLASS_WITH_JSON(NotificationApplet, "metadata.json")

#include "notificationapplet.moc"
