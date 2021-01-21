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

#include "systemtraysettings.h"

#include "debug.h"

#include <KConfigLoader>

static const QString KNOWN_ITEMS_KEY = QStringLiteral("knownItems");
static const QString EXTRA_ITEMS_KEY = QStringLiteral("extraItems");
static const QString SHOW_ALL_ITEMS_KEY = QStringLiteral("showAllItems");
static const QString SHOWN_ITEMS_KEY = QStringLiteral("shownItems");
static const QString HIDDEN_ITEMS_KEY = QStringLiteral("hiddenItems");

SystemTraySettings::SystemTraySettings(KConfigLoader *config, QObject *parent)
    : QObject(parent)
    , config(config)
{
    connect(config, &KConfigLoader::configChanged, this, [this]() {
        if (!updatingConfigValue) {
            loadConfig();
        }
    });

    loadConfig();
}

bool SystemTraySettings::isKnownPlugin(const QString &pluginId)
{
    return m_knownItems.contains(pluginId);
}

const QStringList SystemTraySettings::knownPlugins() const
{
    return m_knownItems;
}

void SystemTraySettings::addKnownPlugin(const QString &pluginId)
{
    m_knownItems << pluginId;
    writeConfigValue(KNOWN_ITEMS_KEY, m_knownItems);
}

void SystemTraySettings::removeKnownPlugin(const QString &pluginId)
{
    m_knownItems.removeAll(pluginId);
    writeConfigValue(KNOWN_ITEMS_KEY, m_knownItems);
}

bool SystemTraySettings::isEnabledPlugin(const QString &pluginId) const
{
    return m_extraItems.contains(pluginId);
}

const QStringList SystemTraySettings::enabledPlugins() const
{
    return m_extraItems;
}

void SystemTraySettings::addEnabledPlugin(const QString &pluginId)
{
    m_extraItems << pluginId;
    writeConfigValue(EXTRA_ITEMS_KEY, m_extraItems);
    emit enabledPluginsChanged({pluginId}, {});
}

void SystemTraySettings::removeEnabledPlugin(const QString &pluginId)
{
    m_extraItems.removeAll(pluginId);
    writeConfigValue(EXTRA_ITEMS_KEY, m_extraItems);
    emit enabledPluginsChanged({}, {pluginId});
}

bool SystemTraySettings::isShowAllItems() const
{
    return config->property(SHOW_ALL_ITEMS_KEY).toBool();
}

const QStringList SystemTraySettings::shownItems() const
{
    return config->property(SHOWN_ITEMS_KEY).toStringList();
}

const QStringList SystemTraySettings::hiddenItems() const
{
    return config->property(HIDDEN_ITEMS_KEY).toStringList();
}

void SystemTraySettings::cleanupPlugin(const QString &pluginId)
{
    removeKnownPlugin(pluginId);
    removeEnabledPlugin(pluginId);

    QStringList shown = shownItems();
    shown.removeAll(pluginId);
    writeConfigValue(SHOWN_ITEMS_KEY, shown);

    QStringList hidden = hiddenItems();
    hidden.removeAll(pluginId);
    writeConfigValue(HIDDEN_ITEMS_KEY, hidden);
}

void SystemTraySettings::loadConfig()
{
    if (!config) {
        return;
    }
    config->load();

    m_knownItems = config->property(KNOWN_ITEMS_KEY).toStringList();

    QStringList extraItems = config->property(EXTRA_ITEMS_KEY).toStringList();
    if (extraItems != m_extraItems) {
        QStringList extraItemsOld = m_extraItems;
        m_extraItems = extraItems;
        notifyAboutChangedEnabledPlugins(extraItemsOld, m_extraItems);
    }

    emit configurationChanged();
}

void SystemTraySettings::writeConfigValue(const QString &key, const QVariant &value)
{
    if (!config) {
        return;
    }

    KConfigSkeletonItem *item = config->findItemByName(key);
    if (item) {
        updatingConfigValue = true;
        item->setWriteFlags(KConfigBase::Notify);
        item->setProperty(value);
        config->save();
        // refresh state of config scheme, if not, above writes are ignored
        config->read();
        updatingConfigValue = false;
    }

    emit configurationChanged();
}

void SystemTraySettings::notifyAboutChangedEnabledPlugins(const QStringList &enabledPluginsOld, const QStringList &enabledPluginsNew)
{
    QStringList newlyEnabled;
    QStringList newlyDisabled;

    for (const QString &pluginId : enabledPluginsOld) {
        if (!enabledPluginsNew.contains(pluginId)) {
            newlyDisabled << pluginId;
        }
    }

    for (const QString &pluginId : enabledPluginsNew) {
        if (!enabledPluginsOld.contains(pluginId)) {
            newlyEnabled << pluginId;
        }
    }

    emit enabledPluginsChanged(newlyEnabled, newlyDisabled);
}
