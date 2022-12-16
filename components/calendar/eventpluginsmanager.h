/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QStringList>

#include <CalendarEvents/CalendarEventsPlugin>

namespace CalendarEvents
{
class EventData;
}
class EventPluginsModel;
class QAbstractListModel;
class EventPluginsManagerPrivate;

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

    // These 5 signals below are used for relaying the
    // plugin signals so that the EventPluginsManager don't
    // have to worry about connecting to newly loaded plugins
    void dataReady(const QMultiHash<QDate, CalendarEvents::EventData> &data);
    void eventModified(const CalendarEvents::EventData &modifiedEvent);
    void eventRemoved(const QString &uid);

    /**
     * Relays the plugin signal that contains alternate dates
     *
     * @param data a hash from CalendarEventsPlugin
     */
    void alternateDateReady(const QHash<QDate, QDate> &data);

    /**
     * Relays the plugin signal that contains sub-labels
     *
     * @param data a hash from CalendarEventsPlugin
     */
    void subLabelReady(const QHash<QDate, CalendarEvents::CalendarEventsPlugin::SubLabel> &data);

private:
    void loadPlugin(const QString &absolutePath);

    EventPluginsManagerPrivate *const d;
};
