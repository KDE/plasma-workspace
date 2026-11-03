/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#include <KPluginFactory>
#include <kdedmodule.h>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>

// If a view is available, register an importer service name. This is basically unused on Wayland, but acts as a flag for apps
// to know whether to keep menus inline

// We allow the X11 implementation to take over at any point

constexpr QLatin1StringView VIEW_SERVICE_NAME("org.kde.kappmenuview");
constexpr QLatin1StringView IMPORTER_SERVICE_NAME("com.canonical.AppMenu.Registrar");

class AppMenuStub : public KDEDModule
{
    Q_OBJECT

public:
    AppMenuStub(QObject *parent, const QList<QVariant> &)
        : KDEDModule(parent)
    {
        m_menuViewWatcher = new QDBusServiceWatcher(VIEW_SERVICE_NAME,
                                                    QDBusConnection::sessionBus(),
                                                    QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration,
                                                    this);

        connect(m_menuViewWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this]() {
            handleViewAvailableChanged(true);
        });
        connect(m_menuViewWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
            Q_UNUSED(service)
            handleViewAvailableChanged(false);
        });
        if (QDBusConnection::sessionBus().interface()->isServiceRegistered(VIEW_SERVICE_NAME)) {
            handleViewAvailableChanged(true);
        }

        connect(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceUnregistered, this, [this](const QString &service) {
            if (!m_viewAvailable) {
                return;
            }

            // we've lost our registration of the name, but a view is still available. This means X11 has taken over, requeue ours
            if (service == IMPORTER_SERVICE_NAME) {
                QDBusConnection::sessionBus().interface()->registerService(IMPORTER_SERVICE_NAME,
                                                                           QDBusConnectionInterface::QueueService,
                                                                           QDBusConnectionInterface::AllowReplacement);
            }
        });
    }

private:
    void handleViewAvailableChanged(bool available)
    {
        m_viewAvailable = available;
        if (available) {
            QDBusConnection::sessionBus().interface()->registerService(IMPORTER_SERVICE_NAME,
                                                                       QDBusConnectionInterface::QueueService,
                                                                       QDBusConnectionInterface::AllowReplacement);
        } else {
            QDBusConnection::sessionBus().interface()->unregisterService(IMPORTER_SERVICE_NAME);
        }
    }

    bool m_viewAvailable = false;
    QDBusServiceWatcher *m_menuViewWatcher = nullptr;
};

K_PLUGIN_CLASS_WITH_JSON(AppMenuStub, "appmenu.json")

#include "appmenustub.moc"
