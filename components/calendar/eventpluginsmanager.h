/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef EVENTPLUGINSMANAGER_H
#define EVENTPLUGINSMANAGER_H

#include <QMap>
#include <QObject>
#include <QStringList>

namespace CalendarEvents
{
class CalendarEventsPlugin;
class EventData;
}
class EventPluginsModel;
class QAbstractListModel;

class EventPluginsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel *model READ pluginsModel NOTIFY pluginsChanged)
    Q_PROPERTY(QStringList enabledPlugins READ enabledPlugins WRITE setEnabledPlugins NOTIFY pluginsChanged)

public:
    explicit EventPluginsManager(QObject *parent = nullptr);
    ~EventPluginsManager() override;

    QList<CalendarEvents::CalendarEventsPlugin *> plugins() const;
    QAbstractListModel *pluginsModel() const;

    // This is a helper function to set which plugins
    // are enabled without needing to go through setEnabledPlugins
    // which also loads the plugins; from the Applet config
    // the plugins are not required to be actually loaded
    Q_INVOKABLE void populateEnabledPluginsList(const QStringList &pluginsList);

    void setEnabledPlugins(QStringList &pluginsList);
    QStringList enabledPlugins() const;

Q_SIGNALS:
    void pluginsChanged();

    // These three signals below are used for relaying the
    // plugin signals so that the EventPluginsManager don't
    // have to worry about connecting to newly loaded plugins
    void dataReady(const QMultiHash<QDate, CalendarEvents::EventData> &data);
    void eventModified(const CalendarEvents::EventData &modifiedEvent);
    void eventRemoved(const QString &uid);

private:
    void loadPlugin(const QString &absolutePath);

    friend class EventPluginsModel;
    EventPluginsModel *m_model = nullptr;
    QList<CalendarEvents::CalendarEventsPlugin *> m_plugins;
    struct PluginData {
        QString name;
        QString desc;
        QString icon;
        QString configUi;
    };
    QMap<QString, PluginData> m_availablePlugins;
    QStringList m_enabledPlugins;
};

#endif
