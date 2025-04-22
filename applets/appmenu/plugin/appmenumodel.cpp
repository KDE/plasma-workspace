/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appmenumodel.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QMenu>

#include <abstracttasksmodel.h>
#include <dbusmenuimporter.h>

class KDBusMenuImporter : public DBusMenuImporter
{
public:
    KDBusMenuImporter(const QString &service, const QString &path, QObject *parent)
        : DBusMenuImporter(service, path, parent)
    {
    }

protected:
    QIcon iconForName(const QString &name) override
    {
        return QIcon::fromTheme(name);
    }
};

AppMenuModel::AppMenuModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_tasksModel(new TaskManager::TasksModel(this))
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    m_tasksModel->setFilterByScreen(true);
    connect(m_tasksModel, &TaskManager::TasksModel::activeTaskChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel,
            &TaskManager::TasksModel::dataChanged,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>()) {
                Q_UNUSED(topLeft)
                Q_UNUSED(bottomRight)
                if (roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuObjectPath)
                    || roles.contains(TaskManager::AbstractTasksModel::ApplicationMenuServiceName) || roles.isEmpty()) {
                    onActiveWindowChanged();
                }
            });
    connect(m_tasksModel, &TaskManager::TasksModel::activityChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::virtualDesktopChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::countChanged, this, &AppMenuModel::onActiveWindowChanged);
    connect(m_tasksModel, &TaskManager::TasksModel::screenGeometryChanged, this, &AppMenuModel::screenGeometryChanged);

    onActiveWindowChanged();

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    // if our current DBus connection gets lost, close the menu
    // we'll select the new menu when the focus changes
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        if (serviceName == m_serviceName) {
            setMenuAvailable(false);
        }
    });
}

AppMenuModel::~AppMenuModel() = default;

bool AppMenuModel::menuAvailable() const
{
    return m_menuAvailable;
}

void AppMenuModel::setMenuAvailable(bool set)
{
    if (m_menuAvailable != set) {
        m_menuAvailable = set;
        setVisible(true);
        Q_EMIT menuAvailableChanged();
    }
}

bool AppMenuModel::visible() const
{
    return m_visible;
}

void AppMenuModel::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        Q_EMIT visibleChanged();
    }
}

QRect AppMenuModel::screenGeometry() const
{
    return m_tasksModel->screenGeometry();
}

void AppMenuModel::setScreenGeometry(QRect geometry)
{
    m_tasksModel->setScreenGeometry(geometry);
}

int AppMenuModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_entries.size();
}

void AppMenuModel::onActiveWindowChanged()
{
    // Do not change active window when panel gets focus
    // See ShellCorona::init() in shell/shellcorona.cpp
    if (m_containmentStatus == Plasma::Types::AcceptingInputStatus) {
        return;
    }

    const QModelIndex activeTaskIndex = m_tasksModel->activeTask();
    const QString objectPath = m_tasksModel->data(activeTaskIndex, TaskManager::AbstractTasksModel::ApplicationMenuObjectPath).toString();
    const QString serviceName = m_tasksModel->data(activeTaskIndex, TaskManager::AbstractTasksModel::ApplicationMenuServiceName).toString();
    updateApplicationMenu(serviceName, objectPath);
}

QHash<int, QByteArray> AppMenuModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[TextRole] = QByteArrayLiteral("text");
    roleNames[VisibleRole] = QByteArrayLiteral("visible");
    roleNames[ActionRole] = QByteArrayLiteral("action");
    return roleNames;
}

QVariant AppMenuModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    const int row = index.row();
    if (row >= m_entries.count()) {
        return QVariant();
    }

    switch (role) {
    case TextRole:
        return m_entries.at(row).text;
    case VisibleRole:
        return m_entries.at(row).visible;
    case ActionRole:
        return QVariant::fromValue(m_entries.at(row).action.data());
    default:
        return QVariant();
    }
}

void AppMenuModel::updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath)
{
    if (m_serviceName == serviceName && m_menuObjectPath == menuObjectPath) {
        if (m_importer) {
            QMetaObject::invokeMethod(m_importer, "updateMenu", Qt::QueuedConnection);
        }
        return;
    }

    if (serviceName.isEmpty() || menuObjectPath.isEmpty()) {
        setMenuAvailable(false);
        setVisible(false);

        m_serviceName = QString();
        m_menuObjectPath = QString();
        m_serviceWatcher->setWatchedServices({});

        if (m_importer) {
            m_importer->disconnect(this);
            m_importer->deleteLater();
            m_importer = nullptr;
        }
    } else {
        m_serviceName = serviceName;
        m_menuObjectPath = menuObjectPath;
        m_serviceWatcher->setWatchedServices(QStringList({m_serviceName}));

        if (m_importer) {
            m_importer->disconnect(this);
            m_importer->deleteLater();
        }

        m_importer = new KDBusMenuImporter(serviceName, menuObjectPath, this);
        QMetaObject::invokeMethod(m_importer, "updateMenu", Qt::QueuedConnection);

        connect(m_importer.data(), &DBusMenuImporter::menuUpdated, this, [=, this](QMenu *menu) {
            QMenu *topMenu = m_importer->menu();
            if (topMenu && topMenu == menu) {
                rebuild();
            }
        });

        connect(m_importer.data(), &DBusMenuImporter::actionActivationRequested, this, [this](QAction *action) {
            // TODO submenus
            for (int i = 0; i < m_entries.size(); ++i) {
                if (m_entries[i].action == action) {
                    Q_EMIT requestActivateIndex(i);
                }
            }
        });

        setMenuAvailable(true);
        setVisible(true);

        rebuild();
    }
}

void AppMenuModel::rebuild()
{
    QList<AppMenuEntry> entries;
    if (m_importer && m_importer->menu()) {
        const auto actions = m_importer->menu()->actions();
        for (QAction *action : actions) {
            // TODO: connect QAction::changed()
            entries.append({
                .action = action,
                .text = action->text(),
                .visible = action->isVisible(),
            });

            // Preload second level menus for better responsiveness.
            if (action->menu()) {
                m_importer->updateMenu(action->menu());
            }
        }
    }

    beginResetModel();
    m_entries = entries;
    endResetModel();

    setMenuAvailable(true);
}

#include "moc_appmenumodel.cpp"
