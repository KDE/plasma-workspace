/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *   Copyright (C) 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "statusnotifieritemsource.h"
#include "systemtraytypes.h"
#include "statusnotifieritemservice.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QDebug>
#include <KIcon>
#include <KIconLoader>
#include <KStandardDirs>
#include <KGlobal>
#include <QPainter>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QVariantMap>
#include <QImage>
#include <QMenu>
#include <QPixmap>
#include <QSysInfo>

#include <netinet/in.h>

#include <dbusmenuimporter.h>
#ifndef DBUSMENUQT_VERSION
// DBUSMENUQT_VERSION was introduced in DBusMenuQt 0.4.0
#define DBUSMENUQT_VERSION 0x000305
#endif

class PlasmaDBusMenuImporter : public DBusMenuImporter
{
public:
    PlasmaDBusMenuImporter(const QString &service, const QString &path, KIconLoader *iconLoader, QObject *parent)
    : DBusMenuImporter(service, path, parent)
    , m_iconLoader(iconLoader)
    {}

protected:
    virtual QIcon iconForName(const QString &name)
    {
        return KIcon(name, m_iconLoader);
    }

private:
    KIconLoader *m_iconLoader;
};

StatusNotifierItemSource::StatusNotifierItemSource(const QString &notifierItemId, QObject *parent)
    : Plasma::DataContainer(parent),
      m_customIconLoader(0),
      m_menuImporter(0),
      m_refreshing(false),
      m_needsReRefreshing(false),
      m_titleUpdate(true),
      m_iconUpdate(true),
      m_tooltipUpdate(true),
      m_statusUpdate(true)
{
    setObjectName(notifierItemId);
    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();

    m_typeId = notifierItemId;
    m_name = notifierItemId;

    int slash = notifierItemId.indexOf('/');
    if (slash == -1) {
        qWarning() << "Invalid notifierItemId:" << notifierItemId;
        m_valid = false;
        m_statusNotifierItemInterface = 0;
        return;
    }
    QString service = notifierItemId.left(slash);
    QString path = notifierItemId.mid(slash);

    m_statusNotifierItemInterface = new org::kde::StatusNotifierItem(service, path,
                                                                     QDBusConnection::sessionBus(), this);

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(10);
    connect(&m_refreshTimer, SIGNAL(timeout()), this, SLOT(performRefresh()));

    m_valid = !service.isEmpty() && m_statusNotifierItemInterface->isValid();
    if (m_valid) {
        connect(m_statusNotifierItemInterface, SIGNAL(NewTitle()), this, SLOT(refreshTitle()));
        connect(m_statusNotifierItemInterface, SIGNAL(NewIcon()), this, SLOT(refreshIcons()));
        connect(m_statusNotifierItemInterface, SIGNAL(NewAttentionIcon()), this, SLOT(refreshIcons()));
        connect(m_statusNotifierItemInterface, SIGNAL(NewOverlayIcon()), this, SLOT(refreshIcons()));
        connect(m_statusNotifierItemInterface, SIGNAL(NewToolTip()), this, SLOT(refreshToolTip()));
        connect(m_statusNotifierItemInterface, SIGNAL(NewStatus(QString)), this, SLOT(syncStatus(QString)));
        refresh();
    }
}

StatusNotifierItemSource::~StatusNotifierItemSource()
{
    delete m_statusNotifierItemInterface;
}

KIconLoader *StatusNotifierItemSource::iconLoader() const
{
    return m_customIconLoader ? m_customIconLoader : KIconLoader::global();
}

Plasma::Service *StatusNotifierItemSource::createService()
{
    return new StatusNotifierItemService(this);
}

void StatusNotifierItemSource::syncStatus(QString status)
{
    setData("TitleChanged", false);
    setData("IconsChanged", false);
    setData("TooltipChanged", false);
    setData("StatusChanged", true);
    setData("Status", status);
    checkForUpdate();
}

void StatusNotifierItemSource::refreshTitle()
{
    m_titleUpdate = true;
    refresh();
}

void StatusNotifierItemSource::refreshIcons()
{
    m_iconUpdate = true;
    refresh();
}

void StatusNotifierItemSource::refreshToolTip()
{
    m_tooltipUpdate = true;
    refresh();
}

void StatusNotifierItemSource::refresh()
{
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}

void StatusNotifierItemSource::performRefresh()
{
    if (m_refreshing) {
        m_needsReRefreshing = true;
        return;
    }

    m_refreshing = true;
    QDBusMessage message = QDBusMessage::createMethodCall(m_statusNotifierItemInterface->service(),
                                                          m_statusNotifierItemInterface->path(), "org.freedesktop.DBus.Properties", "GetAll");

    message << m_statusNotifierItemInterface->interface();
    QDBusPendingCall call = m_statusNotifierItemInterface->connection().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), this, SLOT(refreshCallback(QDBusPendingCallWatcher*)));
}

/**
  \todo add a smart pointer to guard call and to automatically delete it at the end of the function
  */
void StatusNotifierItemSource::refreshCallback(QDBusPendingCallWatcher *call)
{
    m_refreshing = false;
    if (m_needsReRefreshing) {
        m_needsReRefreshing = false;
        performRefresh();
        call->deleteLater();
        return;
    }

    QDBusPendingReply<QVariantMap> reply = *call;
    if (reply.isError()) {
        m_valid = false;
    } else {
        // record what has changed
        setData("TitleChanged", m_titleUpdate);
        m_titleUpdate = false;
        setData("IconsChanged", m_iconUpdate);
        m_iconUpdate = false;
        setData("ToolTipChanged", m_tooltipUpdate);
        m_tooltipUpdate = false;
        setData("StatusChanged", m_statusUpdate);
        m_statusUpdate = false;

        //IconThemePath (handle this one first, because it has an impact on
        //others)
        QVariantMap properties = reply.argumentAt<0>();
        if (!m_customIconLoader) {
            QString path = properties["IconThemePath"].toString();
            if (!path.isEmpty()) {
                // FIXME: If last part of path is not "icons", this won't work!
                QStringList tokens = path.split('/', QString::SkipEmptyParts);
                if (tokens.length() >= 3 && tokens.takeLast() == "icons") {
                    QString appName = tokens.takeLast();
                    QString prefix = '/' + tokens.join("/");
                    // FIXME: Fix KIconLoader and KIconTheme so that we can use
                    // our own instance of KStandardDirs
                    KGlobal::dirs()->addResourceDir("data", prefix);
                    // We use a separate instance of KIconLoader to avoid
                    // adding all application dirs to KIconLoader::global(), to
                    // avoid potential icon name clashes between application
                    // icons
                    m_customIconLoader = new KIconLoader(appName, QStringList() /* dirs */, this);
                } else {
                    qWarning() << "Wrong IconThemePath" << path << ": too short or does not end with 'icons'";
                }
            }
        }
        setData("IconThemePath", properties["IconThemePath"]);

        setData("Category", properties["Category"]);
        setData("Status", properties["Status"]);
        setData("Title", properties["Title"]);
        setData("Id", properties["Id"]);
        setData("WindowId", properties["WindowId"]);
        setData("ItemIsMenu", properties["ItemIsMenu"]);

        //Attention Movie
        setData("AttentionMovieName", properties["AttentionMovieName"]);

        QIcon overlay;
        QStringList overlayNames;

        //Icon
        {
            KDbusImageVector image;
            QIcon icon;
            QString iconName;

            properties["OverlayIconPixmap"].value<QDBusArgument>() >> image;
            if (image.isEmpty()) {
                QString iconName = properties["OverlayIconName"].toString();
                setData("OverlayIconName", iconName);
                if (!iconName.isEmpty()) {
                    overlayNames << iconName;
                    overlay = KIcon(iconName, iconLoader());
                }
            } else {
                overlay = imageVectorToPixmap(image);
            }

            properties["IconPixmap"].value<QDBusArgument>() >> image;
            if (image.isEmpty()) {
                iconName = properties["IconName"].toString();
                if (!iconName.isEmpty()) {
                    icon = KIcon(iconName, iconLoader(), overlayNames);

                    if (overlayNames.isEmpty() && !overlay.isNull()) {
                        overlayIcon(&icon, &overlay);
                    }
                }
            } else {
                icon = imageVectorToPixmap(image);
                if (!icon.isNull() && !overlay.isNull()) {
                    overlayIcon(&icon, &overlay);
                }
            }
            setData("Icon", icon);
            setData("IconName", iconName);
        }

        //Attention icon
        {
            KDbusImageVector image;
            QIcon attentionIcon;

            properties["AttentionIconPixmap"].value<QDBusArgument>() >> image;
            if (image.isEmpty()) {
                QString iconName = properties["AttentionIconName"].toString();
                setData("AttentionIconName", iconName);
                if (!iconName.isEmpty()) {
                    attentionIcon = KIcon(iconName, iconLoader(), overlayNames);

                    if (overlayNames.isEmpty() && !overlay.isNull()) {
                        overlayIcon(&attentionIcon, &overlay);
                    }
                }
            } else {
                attentionIcon = imageVectorToPixmap(image);
                if (!attentionIcon.isNull() && !overlay.isNull()) {
                    overlayIcon(&attentionIcon, &overlay);
                }
            }
            setData("AttentionIcon", attentionIcon);
        }

        //ToolTip
        {
            KDbusToolTipStruct toolTip;
            properties["ToolTip"].value<QDBusArgument>() >> toolTip;
            if (toolTip.title.isEmpty()) {
                setData("ToolTipTitle", QVariant());
                setData("ToolTipSubTitle", QVariant());
                setData("ToolTipIcon", QVariant());
            } else {
                QIcon toolTipIcon;
                if (toolTip.image.size() == 0) {
                    toolTipIcon = KIcon(toolTip.icon, iconLoader());
                } else {
                    toolTipIcon = imageVectorToPixmap(toolTip.image);
                }
                setData("ToolTipTitle", toolTip.title);
                setData("ToolTipSubTitle", toolTip.subTitle);
                setData("ToolTipIcon", toolTipIcon);
            }
        }

        //Menu
        if (!m_menuImporter) {
            QString menuObjectPath = properties["Menu"].value<QDBusObjectPath>().path();
            if (!menuObjectPath.isEmpty()) {
                if (menuObjectPath == "/NO_DBUSMENU") {
                    // This is a hack to make it possible to disable DBusMenu in an
                    // application. The string "/NO_DBUSMENU" must be the same as in
                    // KStatusNotifierItem::setContextMenu().
                    qWarning() << "DBusMenu disabled for this application";
                } else {
                    m_menuImporter = new PlasmaDBusMenuImporter(m_statusNotifierItemInterface->service(), menuObjectPath, iconLoader(), this);
#if DBUSMENUQT_VERSION >= 0x000400
                    connect(m_menuImporter, SIGNAL(menuUpdated()), this, SLOT(contextMenuReady()));
#endif
                }
            }
        }
    }

    checkForUpdate();
    call->deleteLater();
}

void StatusNotifierItemSource::contextMenuReady()
{
#if DBUSMENUQT_VERSION < 0x000400
    // Work around to avoid infinite recursion because menuReadyToBeShown() is emitted
    // by DBusMenuImporter at the end of its slot connected to aboutToShow()
    // (dbusmenu-qt 0.3.5)
    disconnect(m_menuImporter, SIGNAL(menuReadyToBeShown()), this, SLOT(contextMenuReady()));
#endif
    emit contextMenuReady(m_menuImporter->menu());
}

QPixmap StatusNotifierItemSource::KDbusImageStructToPixmap(const KDbusImageStruct &image) const
{
    //swap from network byte order if we are little endian
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        uint *uintBuf = (uint *) image.data.data();
        for (uint i = 0; i < image.data.size()/sizeof(uint); ++i) {
            *uintBuf = ntohl(*uintBuf);
            ++uintBuf;
        }
    }
    QImage iconImage(image.width, image.height, QImage::Format_ARGB32 );
    memcpy(iconImage.bits(), (uchar*)image.data.data(), iconImage.numBytes());

    return QPixmap::fromImage(iconImage);
}

QIcon StatusNotifierItemSource::imageVectorToPixmap(const KDbusImageVector &vector) const
{
    QIcon icon;

    for (int i = 0; i<vector.size(); ++i) {
        icon.addPixmap(KDbusImageStructToPixmap(vector[i]));
    }

    return icon;
}

void StatusNotifierItemSource::overlayIcon(QIcon *icon, QIcon *overlay)
{
    QIcon tmp;
    QPixmap m_iconPixmap = icon->pixmap(KIconLoader::SizeSmall, KIconLoader::SizeSmall);

    QPainter p(&m_iconPixmap);

    const int size = KIconLoader::SizeSmall/2;
    p.drawPixmap(QRect(size, size, size, size), overlay->pixmap(size, size), QRect(0,0,size,size));
    p.end();
    tmp.addPixmap(m_iconPixmap);

    //if an m_icon exactly that size wasn't found don't add it to the vector
    m_iconPixmap = icon->pixmap(KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium);
    if (m_iconPixmap.width() == KIconLoader::SizeSmallMedium) {
        const int size = KIconLoader::SizeSmall/2;
        QPainter p(&m_iconPixmap);
        p.drawPixmap(QRect(m_iconPixmap.width()-size, m_iconPixmap.height()-size, size, size), overlay->pixmap(size, size), QRect(0,0,size,size));
        p.end();
        tmp.addPixmap(m_iconPixmap);
    }

    m_iconPixmap = icon->pixmap(KIconLoader::SizeMedium, KIconLoader::SizeMedium);
    if (m_iconPixmap.width() == KIconLoader::SizeMedium) {
        const int size = KIconLoader::SizeSmall/2;
        QPainter p(&m_iconPixmap);
        p.drawPixmap(QRect(m_iconPixmap.width()-size, m_iconPixmap.height()-size, size, size), overlay->pixmap(size, size), QRect(0,0,size,size));
        p.end();
        tmp.addPixmap(m_iconPixmap);
    }

    m_iconPixmap = icon->pixmap(KIconLoader::SizeLarge, KIconLoader::SizeLarge);
    if (m_iconPixmap.width() == KIconLoader::SizeLarge) {
        const int size = KIconLoader::SizeSmall;
        QPainter p(&m_iconPixmap);
        p.drawPixmap(QRect(m_iconPixmap.width()-size, m_iconPixmap.height()-size, size, size), overlay->pixmap(size, size), QRect(0,0,size,size));
        p.end();
        tmp.addPixmap(m_iconPixmap);
    }

    // We can't do 'm_icon->addPixmap()' because if 'm_icon' uses KIconEngine,
    // it will ignore the added pixmaps. This is not a bug in KIconEngine,
    // QIcon::addPixmap() doc says: "Custom m_icon engines are free to ignore
    // additionally added pixmaps".
    *icon = tmp;
    //hopefully huge and enormous not necessary right now, since it's quite costly
}

void StatusNotifierItemSource::activate(int x, int y)
{
    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        m_statusNotifierItemInterface->call(QDBus::NoBlock, "Activate", x, y);
    }
}

void StatusNotifierItemSource::secondaryActivate(int x, int y)
{
    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        m_statusNotifierItemInterface->call(QDBus::NoBlock, "SecondaryActivate", x, y);
    }
}

void StatusNotifierItemSource::scroll(int delta, const QString &direction)
{
    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        m_statusNotifierItemInterface->call(QDBus::NoBlock, "Scroll", delta, direction);
    }
}

void StatusNotifierItemSource::contextMenu(int x, int y)
{
    if (m_menuImporter) {
    #if DBUSMENUQT_VERSION >= 0x000400
        m_menuImporter->updateMenu();
    #else
        QMenu *menu = m_menuImporter->menu();
        // Simulate an "aboutToShow" so that menu->sizeHint() is correct. Otherwise
        // the menu may show up over the applet if new actions are added on the
        // fly.
        connect(m_menuImporter, SIGNAL(menuReadyToBeShown()), this, SLOT(contextMenuReady()));
        QMetaObject::invokeMethod(menu, "aboutToShow");
    #endif
    } else {
        qWarning() << "Could not find DBusMenu interface, falling back to calling ContextMenu()";
        if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
            m_statusNotifierItemInterface->call(QDBus::NoBlock, "ContextMenu", x, y);
        }
    }
}

#include "statusnotifieritemsource.moc"
