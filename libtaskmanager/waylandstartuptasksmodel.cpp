/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "waylandstartuptasksmodel.h"
#include "libtaskmanager_debug.h"
#include "tasktools.h"

#include <KConfigGroup>
#include <KConfigWatcher>
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>

#include <QDebug>
#include <QTimer>
#include <QUrl>

namespace TaskManager
{
class Q_DECL_HIDDEN WaylandStartupTasksModel::Private
{
public:
    Private(WaylandStartupTasksModel *q);

    void addActivation(KWayland::Client::PlasmaActivation *activation);
    void removeActivation(KWayland::Client::PlasmaActivation *activation);

    void init();
    void loadConfig();

    struct Startup {
        QString name;
        QIcon icon;
        QString applicationId;
        QUrl launcherUrl;
        KWayland::Client::PlasmaActivation *activation;
    };

    WaylandStartupTasksModel *q;
    KConfigWatcher::Ptr configWatcher = nullptr;
    KWayland::Client::PlasmaActivationFeedback *feedback = nullptr;
    KWayland::Client::Registry *registry = nullptr;
    QVector<Startup> startups;
    std::chrono::seconds startupTimeout = std::chrono::seconds::zero();
};

WaylandStartupTasksModel::Private::Private(WaylandStartupTasksModel *q)
    : q(q)
{
}

void WaylandStartupTasksModel::Private::init()
{
    configWatcher = KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("klaunchrc"), KConfig::NoGlobals));
    QObject::connect(configWatcher.data(), &KConfigWatcher::configChanged, q, [this] {
        loadConfig();
    });

    loadConfig();
}

void WaylandStartupTasksModel::Private::loadConfig()
{
    KConfigGroup feedbackConfig(configWatcher->config(), "FeedbackStyle");

    if (!feedbackConfig.readEntry("TaskbarButton", true)) {
        delete feedback;
        feedback = nullptr;
        delete registry;
        registry = nullptr;

        q->beginResetModel();
        startups.clear();
        q->endResetModel();
        return;
    }

    const KConfigGroup taskbarButtonConfig(configWatcher->config(), "TaskbarButtonSettings");
    startupTimeout = std::chrono::seconds(taskbarButtonConfig.readEntry("Timeout", 5));

    if (!registry) {
        using namespace KWayland::Client;

        ConnectionThread *connection = ConnectionThread::fromApplication(q);
        if (!connection) {
            return;
        }

        registry = new Registry(q);
        registry->create(connection);

        QObject::connect(registry, &Registry::plasmaActivationFeedbackAnnounced, q, [this](quint32 name, quint32 version) {
            feedback = registry->createPlasmaActivationFeedback(name, version, q);

            QObject::connect(feedback, &PlasmaActivationFeedback::interfaceAboutToBeReleased, q, [this] {
                q->beginResetModel();
                startups.clear();
                q->endResetModel();
            });

            QObject::connect(feedback, &PlasmaActivationFeedback::activation, q, [this](PlasmaActivation *activation) {
                addActivation(activation);
            });
        });

        registry->setup();
    }
}

void WaylandStartupTasksModel::Private::addActivation(KWayland::Client::PlasmaActivation *activation)
{
    QObject::connect(activation, &KWayland::Client::PlasmaActivation::applicationId, q, [this, activation](const QString &appId) {
        // The application id is guaranteed to be the desktop filename without ".desktop"
        const QString desktopFileName = appId + QLatin1String(".desktop");
        const QString desktopFilePath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, desktopFileName);
        if (desktopFilePath.isEmpty()) {
            qCWarning(TASKMANAGER_DEBUG) << "Got invalid activation app_id:" << appId;
            return;
        }

        const QUrl launcherUrl(QStringLiteral("applications:") + desktopFileName);
        const AppData appData = appDataFromUrl(QUrl::fromLocalFile(desktopFilePath));

        const int count = startups.count();
        q->beginInsertRows(QModelIndex(), count, count);
        startups.append(Startup{
            .name = appData.name,
            .icon = appData.icon,
            .applicationId = appId,
            .launcherUrl = launcherUrl,
            .activation = activation,
        });
        q->endInsertRows();

        // Remove the activation if it doesn't finish within certain time interval.
        QTimer *timeoutTimer = new QTimer(activation);
        QObject::connect(timeoutTimer, &QTimer::timeout, q, [this, activation]() {
            removeActivation(activation);
        });
        timeoutTimer->setSingleShot(true);
        timeoutTimer->start(startupTimeout);
    });

    QObject::connect(activation, &KWayland::Client::PlasmaActivation::finished, q, [this, activation]() {
        removeActivation(activation);
    });
}

void WaylandStartupTasksModel::Private::removeActivation(KWayland::Client::PlasmaActivation *activation)
{
    int position = -1;
    for (int i = 0; i < startups.count(); ++i) {
        if (startups[i].activation == activation) {
            position = i;
            break;
        }
    }
    if (position != -1) {
        q->beginRemoveRows(QModelIndex(), position, position);
        startups.removeAt(position);
        q->endRemoveRows();
    }
}

WaylandStartupTasksModel::WaylandStartupTasksModel(QObject *parent)
    : AbstractTasksModel(parent)
    , d(new Private(this))
{
    d->init();
}

WaylandStartupTasksModel::~WaylandStartupTasksModel()
{
}

QVariant WaylandStartupTasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->startups.count()) {
        return QVariant();
    }

    const auto &data = d->startups[index.row()];
    if (role == Qt::DisplayRole) {
        return data.name;
    } else if (role == Qt::DecorationRole) {
        return data.icon;
    } else if (role == AppId) {
        return data.applicationId;
    } else if (role == AppName) {
        return data.name;
    } else if (role == LauncherUrl || role == LauncherUrlWithoutIcon) {
        return data.launcherUrl;
    } else if (role == IsStartup) {
        return true;
    } else if (role == CanLaunchNewInstance) {
        return false;
    }

    return QVariant();
}

int WaylandStartupTasksModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : d->startups.count();
}

} // namespace TaskManager
