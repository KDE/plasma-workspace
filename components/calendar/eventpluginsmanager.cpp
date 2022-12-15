/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "eventpluginsmanager.h"

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QJsonObject>
#include <QPluginLoader>

#include <KPluginMetaData>

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
    QList<CalendarEvents::CalendarEventsPlugin *> plugins;
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
        m_roles.insert(Qt::UserRole + 1, QByteArrayLiteral("pluginPath"));
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
            // The currentPlugin path contains the full path including
            // the plugin filename, so it needs to be cut off from the last '/'
            const QStringView prefix = QStringView(currentPlugin).left(currentPlugin.lastIndexOf(QDir::separator()));
            const QString qmlFilePath = metadata.configUi;
            return QStringLiteral("%1%2%3").arg(prefix, QDir::separator(), qmlFilePath);
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
        const QString pluginPath = d->availablePlugins.keys().at(index.row());

        if (enabled) {
            if (!d->enabledPlugins.contains(pluginPath)) {
                d->enabledPlugins << pluginPath;
            }
        } else {
            d->enabledPlugins.removeOne(pluginPath);
        }

        Q_EMIT dataChanged(index, index);

        return true;
    }

    Q_INVOKABLE QVariant get(int row, const QByteArray &role)
    {
        return data(createIndex(row, 0), roleNames().key(role));
    }

private:
    EventPluginsManagerPrivate *d;
    QHash<int, QByteArray> m_roles;
};

EventPluginsManagerPrivate::EventPluginsManagerPrivate()
    : model(std::make_unique<EventPluginsModel>(this))
{
    auto plugins = KPluginMetaData::findPlugins(QStringLiteral("plasmacalendarplugins"), [](const KPluginMetaData &md) {
        return md.rawData().contains(QStringLiteral("KPlugin"));
    });
    for (const KPluginMetaData &plugin : std::as_const(plugins)) {
        availablePlugins.insert(plugin.fileName(),
                                {plugin.name(), plugin.description(), plugin.iconName(), plugin.value(QStringLiteral("X-KDE-PlasmaCalendar-ConfigUi"))});
    }

    // Fallback for legacy pre-KPlugin plugins so we can still load them
    const QStringList paths = QCoreApplication::libraryPaths();
    for (const QString &libraryPath : paths) {
        const QString path(libraryPath + QStringLiteral("/plasmacalendarplugins"));
        QDir dir(path);

        if (!dir.exists()) {
            continue;
        }

        const QStringList entryList = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        for (const QString &fileName : entryList) {
            const QString absolutePath = dir.absoluteFilePath(fileName);
            if (availablePlugins.contains(absolutePath)) {
                continue;
            }

            QPluginLoader loader(absolutePath);
            // Load only our own plugins
            if (loader.metaData().value(QStringLiteral("IID")) == QLatin1String("org.kde.CalendarEventsPlugin")) {
                const auto md = loader.metaData().value(QStringLiteral("MetaData")).toObject();
                availablePlugins.insert(absolutePath,
                                        {md.value(QStringLiteral("Name")).toString(),
                                         md.value(QStringLiteral("Description")).toString(),
                                         md.value(QStringLiteral("Icon")).toString(),
                                         md.value(QStringLiteral("ConfigUi")).toString()});
            }
        }
    }
}

EventPluginsManagerPrivate::~EventPluginsManagerPrivate()
{
    qDeleteAll(plugins);
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
        const QString pluginPath = (*i)->property("pluginPath").toString();
        if (pluginsList.contains(pluginPath)) {
            pluginsList.removeAll(pluginPath);
            ++i;
        } else {
            (*i)->deleteLater();
            i = d->plugins.erase(i);
        }
    }

    // Now load all the plugins left in pluginsList
    for (const QString &pluginPath : std::as_const(pluginsList)) {
        loadPlugin(pluginPath);
    }

    d->model->endResetModel();
    Q_EMIT pluginsChanged();
}

QStringList EventPluginsManager::enabledPlugins() const
{
    return d->enabledPlugins;
}

void EventPluginsManager::loadPlugin(const QString &absolutePath)
{
    QPluginLoader loader(absolutePath);

    if (!loader.load()) {
        qWarning() << "Could not create Plasma Calendar Plugin: " << absolutePath;
        qWarning() << loader.errorString();
        return;
    }

    QObject *obj = loader.instance();
    if (obj) {
        CalendarEvents::CalendarEventsPlugin *eventsPlugin = qobject_cast<CalendarEvents::CalendarEventsPlugin *>(obj);
        if (eventsPlugin) {
            qDebug() << "Loading Calendar plugin" << eventsPlugin;
            eventsPlugin->setProperty("pluginPath", absolutePath);
            d->plugins << eventsPlugin;

            // Connect the relay signals
            connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::dataReady, this, &EventPluginsManager::dataReady);
            connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::eventModified, this, &EventPluginsManager::eventModified);
            connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::eventRemoved, this, &EventPluginsManager::eventRemoved);
            connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::alternateDateReady, this, &EventPluginsManager::alternateDateReady);
            connect(eventsPlugin, &CalendarEvents::CalendarEventsPlugin::subLabelReady, this, &EventPluginsManager::subLabelReady);
        } else {
            // not our/valid plugin, so unload it
            loader.unload();
        }
    } else {
        loader.unload();
    }
}

QList<CalendarEvents::CalendarEventsPlugin *> EventPluginsManager::plugins() const
{
    return d->plugins;
}

QAbstractListModel *EventPluginsManager::pluginsModel() const
{
    return d->model.get();
}

#include "eventpluginsmanager.moc"
