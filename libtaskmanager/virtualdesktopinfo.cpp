/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "virtualdesktopinfo.h"

#include <KLocalizedString>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmavirtualdesktop.h>
#include <KWayland/Client/registry.h>
#include <KWindowSystem>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDebug>

#include <config-X11.h>

#if HAVE_X11
#include <QX11Info>
#include <netwm.h>
#endif

namespace TaskManager
{
class Q_DECL_HIDDEN VirtualDesktopInfo::Private : public QObject
{
    Q_OBJECT

public:
    Private();
    virtual ~Private()
    {
    }

    uint refCount = 1;
    // Fall back to true if we get an invalid DBus response when asking for the
    // user's preference since that's what it was for years and years while
    // 425787 was broken.
    bool navigationWrappingAround = true;

    virtual void init() = 0;
    virtual QVariant currentDesktop() const = 0;
    virtual int numberOfDesktops() const = 0;
    virtual QVariantList desktopIds() const = 0;
    virtual QStringList desktopNames() const = 0;
    virtual quint32 position(const QVariant &desktop) const = 0;
    virtual int desktopLayoutRows() const = 0;
    virtual void requestActivate(const QVariant &desktop) = 0;
    virtual void requestCreateDesktop(quint32 position) = 0;
    virtual void requestRemoveDesktop(quint32 position) = 0;

Q_SIGNALS:
    void currentDesktopChanged() const;
    void numberOfDesktopsChanged() const;
    void desktopIdsChanged() const;
    void desktopNamesChanged() const;
    void desktopLayoutRowsChanged() const;
    void navigationWrappingAroundChanged() const;

protected Q_SLOTS:
    void navigationWrappingAroundChanged(bool newVal);
};

VirtualDesktopInfo::Private::Private()
{
    // Connect to navigationWrappingAroundChanged signal
    const bool connection = QDBusConnection::sessionBus().connect(QStringLiteral("org.kde.KWin"),
                                                                  QStringLiteral("/VirtualDesktopManager"),
                                                                  QStringLiteral("org.kde.KWin.VirtualDesktopManager"),
                                                                  QStringLiteral("navigationWrappingAroundChanged"),
                                                                  this,
                                                                  SLOT(navigationWrappingAroundChanged(bool)));
    if (!connection) {
        qWarning() << "Could not connect to org.kde.KWin.VirtualDesktopManager.navigationWrappingAroundChanged signal";
    }

    // ...Then get the property's current value
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                      QStringLiteral("/VirtualDesktopManager"),
                                                      QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("Get"));
    msg.setArguments({QStringLiteral("org.kde.KWin.VirtualDesktopManager"), QStringLiteral("navigationWrappingAround")});
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(msg), this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QVariant> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qWarning() << "Failed to determine whether virtual desktop navigation wrapping is enabled: " << reply.error().message();
            return;
        }

        navigationWrappingAroundChanged(reply.value().toBool());
    });
}

void VirtualDesktopInfo::Private::navigationWrappingAroundChanged(bool newVal)
{
    if (navigationWrappingAround == newVal) {
        return;
    }
    navigationWrappingAround = newVal;
    Q_EMIT navigationWrappingAroundChanged();
}

#if HAVE_X11
class Q_DECL_HIDDEN VirtualDesktopInfo::XWindowPrivate : public VirtualDesktopInfo::Private
{
    Q_OBJECT
public:
    XWindowPrivate();

    void init() override;
    QVariant currentDesktop() const override;
    int numberOfDesktops() const override;
    QVariantList desktopIds() const override;
    QStringList desktopNames() const override;
    quint32 position(const QVariant &desktop) const override;
    int desktopLayoutRows() const override;
    void requestActivate(const QVariant &desktop) override;
    void requestCreateDesktop(quint32 position) override;
    void requestRemoveDesktop(quint32 position) override;
};

VirtualDesktopInfo::XWindowPrivate::XWindowPrivate()
    : VirtualDesktopInfo::Private()
{
    init();
}

void VirtualDesktopInfo::XWindowPrivate::init()
{
    connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &VirtualDesktopInfo::XWindowPrivate::currentDesktopChanged);

    connect(KWindowSystem::self(), &KWindowSystem::numberOfDesktopsChanged, this, &VirtualDesktopInfo::XWindowPrivate::numberOfDesktopsChanged);

    connect(KWindowSystem::self(), &KWindowSystem::desktopNamesChanged, this, &VirtualDesktopInfo::XWindowPrivate::desktopNamesChanged);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.connect(QString(),
                 QStringLiteral("/VirtualDesktopManager"),
                 QStringLiteral("org.kde.KWin.VirtualDesktopManager"),
                 QStringLiteral("rowsChanged"),
                 this,
                 SIGNAL(desktopLayoutRowsChanged()));
}

QVariant VirtualDesktopInfo::XWindowPrivate::currentDesktop() const
{
    return KWindowSystem::currentDesktop();
}

int VirtualDesktopInfo::XWindowPrivate::numberOfDesktops() const
{
    return KWindowSystem::numberOfDesktops();
}

QVariantList VirtualDesktopInfo::XWindowPrivate::desktopIds() const
{
    QVariantList ids;

    for (int i = 1; i <= KWindowSystem::numberOfDesktops(); ++i) {
        ids << i;
    }

    return ids;
}

QStringList VirtualDesktopInfo::XWindowPrivate::desktopNames() const
{
    QStringList names;

    // Virtual desktop numbers start at 1.
    for (int i = 1; i <= KWindowSystem::numberOfDesktops(); ++i) {
        names << KWindowSystem::desktopName(i);
    }

    return names;
}

quint32 VirtualDesktopInfo::XWindowPrivate::position(const QVariant &desktop) const
{
    bool ok = false;

    const quint32 desktopNumber = desktop.toUInt(&ok);

    if (!ok) {
        return -1;
    }

    return desktopNumber;
}

int VirtualDesktopInfo::XWindowPrivate::desktopLayoutRows() const
{
    const NETRootInfo info(QX11Info::connection(), NET::NumberOfDesktops | NET::DesktopNames, NET::WM2DesktopLayout);
    return info.desktopLayoutColumnsRows().height();
}

void VirtualDesktopInfo::XWindowPrivate::requestActivate(const QVariant &desktop)
{
    bool ok = false;
    const int desktopNumber = desktop.toInt(&ok);

    // Virtual desktop numbers start at 1.
    if (ok && desktopNumber > 0 && desktopNumber <= KWindowSystem::numberOfDesktops()) {
        KWindowSystem::setCurrentDesktop(desktopNumber);
    }
}

void VirtualDesktopInfo::XWindowPrivate::requestCreateDesktop(quint32 position)
{
    Q_UNUSED(position)

    NETRootInfo info(QX11Info::connection(), NET::NumberOfDesktops);
    info.setNumberOfDesktops(info.numberOfDesktops() + 1);
}

void VirtualDesktopInfo::XWindowPrivate::requestRemoveDesktop(quint32 position)
{
    Q_UNUSED(position)

    NETRootInfo info(QX11Info::connection(), NET::NumberOfDesktops);

    if (info.numberOfDesktops() > 1) {
        info.setNumberOfDesktops(info.numberOfDesktops() - 1);
    }
}
#endif

class Q_DECL_HIDDEN VirtualDesktopInfo::WaylandPrivate : public VirtualDesktopInfo::Private
{
    Q_OBJECT
public:
    WaylandPrivate();

    QVariant currentVirtualDesktop;
    QStringList virtualDesktops;
    KWayland::Client::PlasmaVirtualDesktopManagement *virtualDesktopManagement = nullptr;

    void init() override;
    void addDesktop(const QString &id, quint32 position);
    QVariant currentDesktop() const override;
    int numberOfDesktops() const override;
    QVariantList desktopIds() const override;
    QStringList desktopNames() const override;
    quint32 position(const QVariant &desktop) const override;
    int desktopLayoutRows() const override;
    void requestActivate(const QVariant &desktop) override;
    void requestCreateDesktop(quint32 position) override;
    void requestRemoveDesktop(quint32 position) override;
};

VirtualDesktopInfo::WaylandPrivate::WaylandPrivate()
    : VirtualDesktopInfo::Private()
{
    init();
}

void VirtualDesktopInfo::WaylandPrivate::init()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }

    KWayland::Client::ConnectionThread *connection = KWayland::Client::ConnectionThread::fromApplication(this);

    if (!connection) {
        return;
    }

    KWayland::Client::Registry *registry = new KWayland::Client::Registry(this);
    registry->create(connection);

    QObject::connect(registry, &KWayland::Client::Registry::plasmaVirtualDesktopManagementAnnounced, [this, registry](quint32 name, quint32 version) {
        virtualDesktopManagement = registry->createPlasmaVirtualDesktopManagement(name, version, this);

        const QList<KWayland::Client::PlasmaVirtualDesktop *> &desktops = virtualDesktopManagement->desktops();

        QObject::connect(virtualDesktopManagement,
                         &KWayland::Client::PlasmaVirtualDesktopManagement::desktopCreated,
                         this,
                         [this](const QString &id, quint32 position) {
                             addDesktop(id, position);
                         });

        QObject::connect(virtualDesktopManagement, &KWayland::Client::PlasmaVirtualDesktopManagement::desktopRemoved, this, [this](const QString &id) {
            virtualDesktops.removeOne(id);

            Q_EMIT numberOfDesktopsChanged();
            Q_EMIT desktopIdsChanged();
            Q_EMIT desktopNamesChanged();

            if (currentVirtualDesktop == id) {
                currentVirtualDesktop.clear();
                Q_EMIT currentDesktopChanged();
            }
        });

        QObject::connect(virtualDesktopManagement,
                         &KWayland::Client::PlasmaVirtualDesktopManagement::rowsChanged,
                         this,
                         &VirtualDesktopInfo::WaylandPrivate::desktopLayoutRowsChanged);
    });

    registry->setup();
}

void VirtualDesktopInfo::WaylandPrivate::addDesktop(const QString &id, quint32 position)
{
    if (virtualDesktops.indexOf(id) != -1) {
        return;
    }

    virtualDesktops.insert(position, id);

    Q_EMIT numberOfDesktopsChanged();
    Q_EMIT desktopIdsChanged();
    Q_EMIT desktopNamesChanged();

    const KWayland::Client::PlasmaVirtualDesktop *desktop = virtualDesktopManagement->getVirtualDesktop(id);

    QObject::connect(desktop, &KWayland::Client::PlasmaVirtualDesktop::activated, this, [desktop, this]() {
        currentVirtualDesktop = desktop->id();
        Q_EMIT currentDesktopChanged();
    });

    QObject::connect(desktop, &KWayland::Client::PlasmaVirtualDesktop::done, this, [this]() {
        Q_EMIT desktopNamesChanged();
    });

    if (desktop->isActive()) {
        currentVirtualDesktop = id;
        Q_EMIT currentDesktopChanged();
    }
}

QVariant VirtualDesktopInfo::WaylandPrivate::currentDesktop() const
{
    return currentVirtualDesktop;
}

int VirtualDesktopInfo::WaylandPrivate::numberOfDesktops() const
{
    return virtualDesktops.count();
}

quint32 VirtualDesktopInfo::WaylandPrivate::position(const QVariant &desktop) const
{
    return virtualDesktops.indexOf(desktop.toString());
}

QVariantList VirtualDesktopInfo::WaylandPrivate::desktopIds() const
{
    QVariantList ids;

    Q_FOREACH (const QString &id, virtualDesktops) {
        ids << id;
    }

    return ids;
}

QStringList VirtualDesktopInfo::WaylandPrivate::desktopNames() const
{
    if (!virtualDesktopManagement) {
        return QStringList();
    }
    QStringList names;

    Q_FOREACH (const QString &id, virtualDesktops) {
        const KWayland::Client::PlasmaVirtualDesktop *desktop = virtualDesktopManagement->getVirtualDesktop(id);

        if (desktop) {
            names << desktop->name();
        }
    }

    return names;
}

int VirtualDesktopInfo::WaylandPrivate::desktopLayoutRows() const
{
    if (!virtualDesktopManagement) {
        return 0;
    }

    return virtualDesktopManagement->rows();
}

void VirtualDesktopInfo::WaylandPrivate::requestActivate(const QVariant &desktop)
{
    if (!virtualDesktopManagement) {
        return;
    }
    KWayland::Client::PlasmaVirtualDesktop *desktopObj = virtualDesktopManagement->getVirtualDesktop(desktop.toString());

    if (desktopObj) {
        desktopObj->requestActivate();
    }
}

void VirtualDesktopInfo::WaylandPrivate::requestCreateDesktop(quint32 position)
{
    if (!virtualDesktopManagement) {
        return;
    }
    virtualDesktopManagement->requestCreateVirtualDesktop(i18n("New Desktop"), position);
}

void VirtualDesktopInfo::WaylandPrivate::requestRemoveDesktop(quint32 position)
{
    if (!virtualDesktopManagement) {
        return;
    }
    if (virtualDesktops.count() == 1) {
        return;
    }

    if (position > ((quint32)virtualDesktops.count() - 1)) {
        return;
    }

    virtualDesktopManagement->requestRemoveVirtualDesktop(virtualDesktops.at(position));
}

VirtualDesktopInfo::Private *VirtualDesktopInfo::d = nullptr;

VirtualDesktopInfo::VirtualDesktopInfo(QObject *parent)
    : QObject(parent)
{
    if (!d) {
#if HAVE_X11
        if (KWindowSystem::isPlatformX11()) {
            d = new VirtualDesktopInfo::XWindowPrivate;
        } else
#endif
        {
            d = new VirtualDesktopInfo::WaylandPrivate;
        }
    } else {
        ++d->refCount;
    }

    connect(d, &VirtualDesktopInfo::Private::currentDesktopChanged, this, &VirtualDesktopInfo::currentDesktopChanged);
    connect(d, &VirtualDesktopInfo::Private::numberOfDesktopsChanged, this, &VirtualDesktopInfo::numberOfDesktopsChanged);
    connect(d, &VirtualDesktopInfo::Private::desktopIdsChanged, this, &VirtualDesktopInfo::desktopIdsChanged);
    connect(d, &VirtualDesktopInfo::Private::desktopNamesChanged, this, &VirtualDesktopInfo::desktopNamesChanged);
    connect(d, &VirtualDesktopInfo::Private::desktopLayoutRowsChanged, this, &VirtualDesktopInfo::desktopLayoutRowsChanged);
}

VirtualDesktopInfo::~VirtualDesktopInfo()
{
    --d->refCount;

    if (!d->refCount) {
        delete d;
        d = nullptr;
    }
}

QVariant VirtualDesktopInfo::currentDesktop() const
{
    return d->currentDesktop();
}

int VirtualDesktopInfo::numberOfDesktops() const
{
    return d->numberOfDesktops();
}

QVariantList VirtualDesktopInfo::desktopIds() const
{
    return d->desktopIds();
}

QStringList VirtualDesktopInfo::desktopNames() const
{
    return d->desktopNames();
}

quint32 VirtualDesktopInfo::position(const QVariant &desktop) const
{
    return d->position(desktop);
}

int VirtualDesktopInfo::desktopLayoutRows() const
{
    return d->desktopLayoutRows();
}

void VirtualDesktopInfo::requestActivate(const QVariant &desktop)
{
    d->requestActivate(desktop);
}

void VirtualDesktopInfo::requestCreateDesktop(quint32 position)
{
    return d->requestCreateDesktop(position);
}

void VirtualDesktopInfo::requestRemoveDesktop(quint32 position)
{
    return d->requestRemoveDesktop(position);
}

bool VirtualDesktopInfo::navigationWrappingAround() const
{
    return d->navigationWrappingAround;
}

}

#include "virtualdesktopinfo.moc"
