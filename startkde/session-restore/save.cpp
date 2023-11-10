/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
 SPDX-FileCopyrightText: 2023 David Redondo <kde@david-redondo.de>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <chrono>

#include <QDebug>
#include <QGuiApplication>
#include <QTimer>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>

#include <KConfigGroup>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

class WaylandLister : public QObject
{
    Q_OBJECT
public:
    explicit WaylandLister(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_connection.reset(KWayland::Client::ConnectionThread::fromApplication());
        if (!m_connection) {
            qWarning() << "no connection";
            return;
        }

        m_registry.create(m_connection.get());

        QObject::connect(&m_registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, this, [this](quint32 name, quint32 version) {
            m_windowManagement.reset(m_registry.createPlasmaWindowManagement(name, version));
        });

        m_registry.setup();

        // We'll need 3 because getting the registry is async, getting the window management interface is another,
        // then we'll have requested information about every window. By the 3rd sync it should have sent everything.
        static constexpr auto syncTimes = 3;
        for (auto i = 0; i < syncTimes; i++) {
            QCoreApplication::processEvents();
            m_connection->roundtrip();
            QCoreApplication::processEvents();
        }
        QCoreApplication::processEvents();
        if (!m_windowManagement) {
            qCritical() << "Could not access window management probably. This application will not work correctly";
            exit(0);
        }
        const auto windows = m_windowManagement->windows();
        for (const auto &window : windows) {
            insert(window);
        }
    }

    void insert(KWayland::Client::PlasmaWindow *window)
    {
        qDebug() << window->appId() << window->resourceName();
        if (window->appId().isEmpty()) {
            return;
        }
        m_runningApps.insert(window->appId());
    }

    QStringList apps() const
    {
        return m_runningApps.values();
    }

private:
    QSet<QString> m_runningApps;
    std::unique_ptr<KWayland::Client::ConnectionThread> m_connection;
    KWayland::Client::Registry m_registry;
    std::unique_ptr<KWayland::Client::PlasmaWindowManagement> m_windowManagement;
};

int main(int argc, char *argv[])
{
    QGuiApplication::setDesktopFileName(u"plasma-fallback-session-save"_s);
    QGuiApplication a(argc, argv);
    a.setDesktopSettingsAware(false);

    WaylandLister appList;
    KSharedConfig::Ptr config = KSharedConfig::openConfig(u"plasmasessionrestore"_s, KConfig::NoGlobals);

    const QStringList groupList = config->groupList();
    for (const QString &group : groupList) {
        config->deleteGroup(group);
    }

    int i = 0;
    for (const QString &appId : appList.apps()) {
        auto group = config->group(QString::number(i++));
        qDebug() << "Saving" << group.name() << appId;
        group.writeEntry("appId", appId);
    }

    // work around KWayland doing pixmap handling in threads with an unsafe cleanup on app exit
    config->sync();
    quick_exit(0);
}

#include "save.moc"
