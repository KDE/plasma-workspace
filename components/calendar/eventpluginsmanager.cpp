/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "eventpluginsmanager.h"
#include "debug_p.h"

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QPluginLoader>

#include <KPluginMetaData>

using namespace Qt::StringLiterals;

class EventPluginsManagerPrivate
{
public:
    explicit EventPluginsManagerPrivate();
    ~EventPluginsManagerPrivate();

    friend class EventPluginsModel;
    struct PluginData {
        QString name;
        QString desc;
        QString icon;
        QString configUi;
    };

    std::unique_ptr<EventPluginsModel> model;
    // These pointers are owned and managed by QPluginLoader internals and deleted once all consumers unload.
    QMap<QString, CalendarEvents::CalendarEventsPlugin *> plugins;
    QMap<QString, PluginData> availablePlugins;
    QStringList enabledPlugins;
};

class EventPluginsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    EventPluginsModel(EventPluginsManagerPrivate *d)
        : d(d)
        , m_roles(QAbstractListModel::roleNames())
    {
        m_roles.insert(Qt::EditRole, QByteArrayLiteral("checked"));
        m_roles.insert(Qt::UserRole, QByteArrayLiteral("configUi"));
        m_roles.insert(Qt::UserRole + 1, QByteArrayLiteral("pluginId"));
    }

    // make these two available to the manager
    void beginResetModel()
    {
        QAbstractListModel::beginResetModel();
    }

    void endResetModel()
    {
        QAbstractListModel::endResetModel();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return m_roles;
    }

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return d->availablePlugins.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || !d) {
            return QVariant();
        }

        const auto it = std::next(d->availablePlugins.cbegin(), index.row());
        const QString currentPlugin = it.key();
        const EventPluginsManagerPrivate::PluginData metadata = it.value();

        switch (role) {
        case Qt::DisplayRole:
            return metadata.name;
        case Qt::ToolTipRole:
            return metadata.desc;
        case Qt::DecorationRole:
            return metadata.icon;
        case Qt::UserRole: {
            return metadata.configUi;
        }
        case Qt::UserRole + 1:
            return currentPlugin;
        case Qt::EditRole:
            return d->enabledPlugins.contains(currentPlugin);
        }

        return QVariant();
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole || !index.isValid()) {
            return false;
        }

        bool enabled = value.toBool();
        const QString pluginId = d->availablePlugins.keys().at(index.row());

        if (enabled) {
            if (!d->enabledPlugins.contains(pluginId)) {
                d->enabledPlugins << pluginId;
            }
        } else {
            d->enabledPlugins.removeOne(pluginId);
        }

        Q_EMIT dataChanged(index, index);

        return true;
    }

    Q_INVOKABLE QVariant get(int row, const QByteArray &role)
    {
        return data(createIndex(row, 0), roleNames().key(role));
    }

private:
    EventPluginsManagerPrivate *const d;
    QHash<int, QByteArray> m_roles;
};

EventPluginsManagerPrivate::EventPluginsManagerPrivate()
    : model(std::make_unique<EventPluginsModel>(this))
{
    const auto plugins = KPluginMetaData::findPlugins(QStringLiteral("plasmacalendarplugins"));
    for (const KPluginMetaData &plugin : plugins) {
        const QString prefix = plugin.fileName().left(plugin.fileName().lastIndexOf(QLatin1Char('/')));
        const QString configName = plugin.value(u"X-KDE-PlasmaCalendar-ConfigUi");

        availablePlugins.insert(plugin.pluginId(), {plugin.name(), plugin.description(), plugin.iconName(), prefix + QLatin1Char('/') + configName});
    }
}

EventPluginsManagerPrivate::~EventPluginsManagerPrivate()
{
    // Don't try to delete the root component. - the Qt documentation
    plugins.clear();
}

EventPluginsManager::EventPluginsManager(QObject *parent)
    : QObject(parent)
    , d(new EventPluginsManagerPrivate)
{
}

EventPluginsManager::~EventPluginsManager()
{
    delete d;
}

void EventPluginsManager::populateEnabledPluginsList(const QStringList &pluginsList)
{
    d->model->beginResetModel();
    d->enabledPlugins = pluginsList;
    d->model->endResetModel();
}

void EventPluginsManager::setEnabledPlugins(QStringList &pluginsList)
{
    d->model->beginResetModel();
    d->enabledPlugins = pluginsList;

    // Remove all already loaded plugins from the pluginsList
    // and unload those plugins that are not in the pluginsList
    auto i = d->plugins.begin();
    while (i != d->plugins.end()) {
        const QString pluginId = i.key();
        if (pluginsList.contains(pluginId)) {
            pluginsList.removeAll(pluginId);
            ++i;
        } else {
            i.value()->deleteLater();
            i = d->plugins.erase(i);
        }
    }

    // Now load all the plugins left in pluginsList
    for (const QString &pluginId : std::as_const(pluginsList)) {
        loadPlugin(pluginId);
    }

    d->model->endResetModel();
    Q_EMIT pluginsChanged();
}

QStringList EventPluginsManager::enabledPlugins() const
{
    return d->enabledPlugins;
}

void EventPluginsManager::loadPlugin(const QString &pluginId)
{
    QPluginLoader loader(QString(u"plasmacalendarplugins/" + QDir::cleanPath(pluginId)));
    if (auto eventsPlugin = qobject_cast<CalendarEvents::CalendarEventsPlugin *>(loader.instance())) {
        qCDebug(COMPONENTS::CALENDAR) << "Loading Calendar plugin" << eventsPlugin;
        d->plugins.insert(pluginId, eventsPlugin);

        // Connect the relay signals
        connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::dataReady, this, &EventPluginsManager::dataReady);
        connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::eventModified, this, &EventPluginsManager::eventModified);
        connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::eventRemoved, this, &EventPluginsManager::eventRemoved);
        connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::alternateCalendarDateReady, this, &EventPluginsManager::alternateCalendarDateReady);
        connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::subLabelReady, this, &EventPluginsManager::subLabelReady);
    } else {
        qCWarning(COMPONENTS::CALENDAR) << "Could not create Plasma Calendar Plugin: " << pluginId;
        qCWarning(COMPONENTS::CALENDAR) << loader.errorString();
        loader.unload();
    }
}

QList<CalendarEvents::CalendarEventsPlugin *> EventPluginsManager::plugins() const
{
    return d->plugins.values();
}

QAbstractListModel *EventPluginsManager::pluginsModel() const
{
    return d->model.get();
}

#include "eventpluginsmanager.moc"
#include "moc_eventpluginsmanager.cpp"
