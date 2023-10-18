/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "virtualdesktopinfo.h"
#include "libtaskmanager_debug.h"

#include <KLocalizedString>
#include <KWindowSystem>
#include <KX11Extras>

#include <qwayland-org-kde-plasma-virtual-desktop.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QWaylandClientExtension>

#include <config-X11.h>

#if HAVE_X11
#include <netwm.h>
#endif // HAVE_X11

namespace X11Info
{
[[nodiscard]] inline auto connection()
{
    return qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
}
}

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
        qCWarning(TASKMANAGER_DEBUG) << "Could not connect to org.kde.KWin.VirtualDesktopManager.navigationWrappingAroundChanged signal";
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
            qCWarning(TASKMANAGER_DEBUG) << "Failed to determine whether virtual desktop navigation wrapping is enabled: " << reply.error().message();
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
    connect(KX11Extras::self(), &KX11Extras::currentDesktopChanged, this, &VirtualDesktopInfo::XWindowPrivate::currentDesktopChanged);

    connect(KX11Extras::self(), &KX11Extras::numberOfDesktopsChanged, this, &VirtualDesktopInfo::XWindowPrivate::numberOfDesktopsChanged);

    connect(KX11Extras::self(), &KX11Extras::desktopNamesChanged, this, &VirtualDesktopInfo::XWindowPrivate::desktopNamesChanged);

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
    return KX11Extras::currentDesktop();
}

int VirtualDesktopInfo::XWindowPrivate::numberOfDesktops() const
{
    return KX11Extras::numberOfDesktops();
}

QVariantList VirtualDesktopInfo::XWindowPrivate::desktopIds() const
{
    QVariantList ids;

    for (int i = 1; i <= KX11Extras::numberOfDesktops(); ++i) {
        ids << i;
    }

    return ids;
}

QStringList VirtualDesktopInfo::XWindowPrivate::desktopNames() const
{
    QStringList names;

    // Virtual desktop numbers start at 1.
    for (int i = 1; i <= KX11Extras::numberOfDesktops(); ++i) {
        names << KX11Extras::desktopName(i);
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
    const NETRootInfo info(X11Info::connection(), NET::NumberOfDesktops | NET::DesktopNames, NET::WM2DesktopLayout);
    return info.desktopLayoutColumnsRows().height();
}

void VirtualDesktopInfo::XWindowPrivate::requestActivate(const QVariant &desktop)
{
    bool ok = false;
    const int desktopNumber = desktop.toInt(&ok);

    // Virtual desktop numbers start at 1.
    if (ok && desktopNumber > 0 && desktopNumber <= KX11Extras::numberOfDesktops()) {
        KX11Extras::setCurrentDesktop(desktopNumber);
    }
}

void VirtualDesktopInfo::XWindowPrivate::requestCreateDesktop(quint32 position)
{
    Q_UNUSED(position)

    NETRootInfo info(X11Info::connection(), NET::NumberOfDesktops);
    info.setNumberOfDesktops(info.numberOfDesktops() + 1);
}

void VirtualDesktopInfo::XWindowPrivate::requestRemoveDesktop(quint32 position)
{
    Q_UNUSED(position)

    NETRootInfo info(X11Info::connection(), NET::NumberOfDesktops);

    if (info.numberOfDesktops() > 1) {
        info.setNumberOfDesktops(info.numberOfDesktops() - 1);
    }
}
#endif // HAVE_X11

class PlasmaVirtualDesktop : public QObject, public QtWayland::org_kde_plasma_virtual_desktop
{
    Q_OBJECT
public:
    PlasmaVirtualDesktop(::org_kde_plasma_virtual_desktop *object, const QString &id)
        : org_kde_plasma_virtual_desktop(object)
        , id(id)
    {
    }
    ~PlasmaVirtualDesktop()
    {
        wl_proxy_destroy(reinterpret_cast<wl_proxy *>(object()));
    }
    const QString id;
    QString name;
Q_SIGNALS:
    void done();
    void activated();

protected:
    void org_kde_plasma_virtual_desktop_name(const QString &name) override
    {
        this->name = name;
    }
    void org_kde_plasma_virtual_desktop_done() override
    {
        Q_EMIT done();
    }
    void org_kde_plasma_virtual_desktop_activated() override
    {
        Q_EMIT activated();
    }
};

class PlasmaVirtualDesktopManagement : public QWaylandClientExtensionTemplate<PlasmaVirtualDesktopManagement>,
                                       public QtWayland::org_kde_plasma_virtual_desktop_management
{
    Q_OBJECT
public:
    PlasmaVirtualDesktopManagement()
        : QWaylandClientExtensionTemplate(2)
    {
        connect(this, &QWaylandClientExtension::activeChanged, this, [this] {
            if (!isActive()) {
                wl_proxy_destroy(reinterpret_cast<wl_proxy *>(object()));
            }
        });
    }
    ~PlasmaVirtualDesktopManagement()
    {
        if (isActive()) {
            wl_proxy_destroy(reinterpret_cast<wl_proxy *>(object()));
        }
    }
Q_SIGNALS:
    void desktopCreated(const QString &id, quint32 position);
    void desktopRemoved(const QString &id);
    void rowsChanged(const quint32 rows);

protected:
    void org_kde_plasma_virtual_desktop_management_desktop_created(const QString &desktop_id, uint32_t position) override
    {
        Q_EMIT desktopCreated(desktop_id, position);
    }
    void org_kde_plasma_virtual_desktop_management_desktop_removed(const QString &desktop_id) override
    {
        Q_EMIT desktopRemoved(desktop_id);
    }
    void org_kde_plasma_virtual_desktop_management_rows(uint32_t rows) override
    {
        Q_EMIT rowsChanged(rows);
    }
};

class Q_DECL_HIDDEN VirtualDesktopInfo::WaylandPrivate : public VirtualDesktopInfo::Private
{
    Q_OBJECT
public:
    WaylandPrivate();

    QVariant currentVirtualDesktop;
    std::vector<std::unique_ptr<PlasmaVirtualDesktop>> virtualDesktops;
    std::unique_ptr<PlasmaVirtualDesktopManagement> virtualDesktopManagement;
    quint32 rows;

    auto findDesktop(const QString &id) const;

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

auto VirtualDesktopInfo::WaylandPrivate::findDesktop(const QString &id) const
{
    return std::find_if(virtualDesktops.begin(), virtualDesktops.end(), [&id](const std::unique_ptr<PlasmaVirtualDesktop> &desktop) {
        return desktop->id == id;
    });
}

void VirtualDesktopInfo::WaylandPrivate::init()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }

    virtualDesktopManagement = std::make_unique<PlasmaVirtualDesktopManagement>();

    connect(virtualDesktopManagement.get(), &PlasmaVirtualDesktopManagement::activeChanged, this, [this] {
        if (!virtualDesktopManagement->isActive()) {
            rows = 0;
            virtualDesktops.clear();
            currentVirtualDesktop.clear();
            Q_EMIT currentDesktopChanged();
            Q_EMIT numberOfDesktopsChanged();
            Q_EMIT navigationWrappingAroundChanged();
            Q_EMIT desktopIdsChanged();
            Q_EMIT desktopNamesChanged();
            Q_EMIT desktopLayoutRowsChanged();
        }
    });

    connect(virtualDesktopManagement.get(), &PlasmaVirtualDesktopManagement::desktopCreated, this, &WaylandPrivate::addDesktop);

    connect(virtualDesktopManagement.get(), &PlasmaVirtualDesktopManagement::desktopRemoved, this, [this](const QString &id) {
        std::erase_if(virtualDesktops, [id](const std::unique_ptr<PlasmaVirtualDesktop> &desktop) {
            return desktop->id == id;
        });

        Q_EMIT numberOfDesktopsChanged();
        Q_EMIT desktopIdsChanged();
        Q_EMIT desktopNamesChanged();

        if (currentVirtualDesktop == id) {
            currentVirtualDesktop.clear();
            Q_EMIT currentDesktopChanged();
        }
    });

    connect(virtualDesktopManagement.get(), &PlasmaVirtualDesktopManagement::rowsChanged, this, [this](quint32 rows) {
        this->rows = rows;
        Q_EMIT desktopLayoutRowsChanged();
    });
}

void VirtualDesktopInfo::WaylandPrivate::addDesktop(const QString &id, quint32 position)
{
    if (findDesktop(id) != virtualDesktops.end()) {
        return;
    }

    auto desktop = std::make_unique<PlasmaVirtualDesktop>(virtualDesktopManagement->get_virtual_desktop(id), id);

    connect(desktop.get(), &PlasmaVirtualDesktop::activated, this, [id, this]() {
        currentVirtualDesktop = id;
        Q_EMIT currentDesktopChanged();
    });

    connect(desktop.get(), &PlasmaVirtualDesktop::done, this, [this]() {
        Q_EMIT desktopNamesChanged();
    });

    virtualDesktops.insert(std::next(virtualDesktops.begin(), position), std::move(desktop));

    Q_EMIT numberOfDesktopsChanged();
    Q_EMIT desktopIdsChanged();
    Q_EMIT desktopNamesChanged();
}

QVariant VirtualDesktopInfo::WaylandPrivate::currentDesktop() const
{
    return currentVirtualDesktop;
}

int VirtualDesktopInfo::WaylandPrivate::numberOfDesktops() const
{
    return virtualDesktops.size();
}

quint32 VirtualDesktopInfo::WaylandPrivate::position(const QVariant &desktop) const
{
    return std::distance(virtualDesktops.begin(), findDesktop(desktop.toString()));
}

QVariantList VirtualDesktopInfo::WaylandPrivate::desktopIds() const
{
    QVariantList ids;
    ids.reserve(virtualDesktops.size());

    std::transform(virtualDesktops.cbegin(), virtualDesktops.cend(), std::back_inserter(ids), [](const std::unique_ptr<PlasmaVirtualDesktop> &desktop) {
        return desktop->id;
    });
    return ids;
}

QStringList VirtualDesktopInfo::WaylandPrivate::desktopNames() const
{
    if (!virtualDesktopManagement->isActive()) {
        return QStringList();
    }
    QStringList names;
    names.reserve(virtualDesktops.size());

    std::transform(virtualDesktops.cbegin(), virtualDesktops.cend(), std::back_inserter(names), [](const std::unique_ptr<PlasmaVirtualDesktop> &desktop) {
        return desktop->name;
    });
    return names;
}

int VirtualDesktopInfo::WaylandPrivate::desktopLayoutRows() const
{
    if (!virtualDesktopManagement->isActive()) {
        return 0;
    }

    return rows;
}

void VirtualDesktopInfo::WaylandPrivate::requestActivate(const QVariant &desktop)
{
    if (!virtualDesktopManagement->isActive()) {
        return;
    }

    if (auto it = findDesktop(desktop.toString()); it != virtualDesktops.end()) {
        (*it)->request_activate();
    }
}

void VirtualDesktopInfo::WaylandPrivate::requestCreateDesktop(quint32 position)
{
    if (!virtualDesktopManagement->isActive()) {
        return;
    }
    virtualDesktopManagement->request_create_virtual_desktop(i18n("New Desktop"), position);
}

void VirtualDesktopInfo::WaylandPrivate::requestRemoveDesktop(quint32 position)
{
    if (!virtualDesktopManagement->isActive()) {
        return;
    }
    if (virtualDesktops.size() == 1) {
        return;
    }

    if (position > (virtualDesktops.size() - 1)) {
        return;
    }

    virtualDesktopManagement->request_remove_virtual_desktop(virtualDesktops.at(position)->id);
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
#endif // HAVE_X11
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
