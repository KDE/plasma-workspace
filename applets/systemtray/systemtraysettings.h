/***************************************************************************
 *   Copyright (C) 2020 Konrad Materka <materka@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef SYSTEMTRAYSETTINGS_H
#define SYSTEMTRAYSETTINGS_H

#include <QObject>
#include <QPointer>
#include <QStringList>

class KConfigLoader;

/**
 * @brief The SystemTraySettings class
 */
class SystemTraySettings : public QObject
{
    Q_OBJECT
public:
    explicit SystemTraySettings(KConfigLoader *config, QObject *parent = nullptr);

    virtual bool isKnownPlugin(const QString &pluginId);
    virtual const QStringList knownPlugins() const;
    virtual void addKnownPlugin(const QString &pluginId);
    virtual void removeKnownPlugin(const QString &pluginId);

    virtual bool isEnabledPlugin(const QString &pluginId) const;
    virtual const QStringList enabledPlugins() const;
    virtual void addEnabledPlugin(const QString &pluginId);
    virtual void removeEnabledPlugin(const QString &pluginId);

    virtual bool isShowAllItems() const;
    virtual const QStringList shownItems() const;
    virtual const QStringList hiddenItems() const;

    virtual void cleanupPlugin(const QString &pluginId);

signals:
    void configurationChanged();
    void enabledPluginsChanged(const QStringList &enabledPlugins, const QStringList &disabledPlugins);

private:
    void loadConfig();
    void writeConfigValue(const QString &key, const QVariant &value);
    void notifyAboutChangedEnabledPlugins(const QStringList &enabledPluginsOld, const QStringList &enabledPluginsNew);

    QPointer<KConfigLoader> config;

    bool updatingConfigValue = false;
    QStringList m_extraItems;
    QStringList m_knownItems;
};

#endif // SYSTEMTRAYSETTINGS_H
