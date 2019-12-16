/***************************************************************************
 *   Copyright (C) 2019 Konrad Materka <materka@gmail.com>                 *
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

#include "systemtraymodel.h"
#include "debug.h"

#include <QQuickItem>

#include <Plasma/Applet>
#include <Plasma/DataContainer>
#include <Plasma/Service>
#include <PluginLoader>

#include <KLocalizedString>

PlasmoidModel::PlasmoidModel(QObject *parent) : QStandardItemModel(parent)
{
    for (const auto &info : Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        if (!info.isValid() || info.value(QStringLiteral("X-Plasma-NotificationArea")) != "true") {
            continue;
        }

        QString name = info.name();
        const QString dbusactivation =
                info.rawData().value(QStringLiteral("X-Plasma-DBusActivationService")).toString();

        if (!dbusactivation.isEmpty()) {
            name += i18n(" (Automatic load)");
        }
        QStandardItem *item = new QStandardItem(QIcon::fromTheme(info.iconName()), name);
        item->setData(info.pluginId(), static_cast<int>(BaseRole::ItemId));
        item->setData("Plasmoid", static_cast<int>(BaseRole::ItemType));
        item->setData(false, static_cast<int>(BaseRole::CanRender));
        item->setData(false, static_cast<int>(Role::HasApplet));
        appendRow(item);
    }
    sort(0);
}

QHash<int, QByteArray> PlasmoidModel::roleNames() const
{
    QHash<int, QByteArray> roles = QStandardItemModel::roleNames();

    roles.insert(static_cast<int>(BaseRole::ItemType), QByteArrayLiteral("itemType"));
    roles.insert(static_cast<int>(BaseRole::ItemId), QByteArrayLiteral("itemId"));
    roles.insert(static_cast<int>(BaseRole::CanRender), QByteArrayLiteral("canRender"));

    roles.insert(static_cast<int>(Role::Applet), QByteArrayLiteral("applet"));
    roles.insert(static_cast<int>(Role::HasApplet), QByteArrayLiteral("hasApplet"));

    return roles;
}

void PlasmoidModel::addApplet(Plasma::Applet *applet)
{
    auto info = applet->pluginMetaData();
    QStandardItem *dataItem = nullptr;
    for (int i = 0; i < rowCount(); i++) {
        QStandardItem *currentItem = item(i);
        if (currentItem->data(static_cast<int>(BaseRole::ItemId)) == info.pluginId()) {
            dataItem = currentItem;
            break;
        }
    }

    if (!dataItem) {
        dataItem = new QStandardItem(QIcon::fromTheme(info.iconName()), info.name());
        appendRow(dataItem);
    }

    dataItem->setData(info.pluginId(), static_cast<int>(BaseRole::ItemId));
    dataItem->setData(applet->property("_plasma_graphicObject"), static_cast<int>(Role::Applet));
    dataItem->setData(true, static_cast<int>(Role::HasApplet));
    dataItem->setData(true, static_cast<int>(BaseRole::CanRender));
}

void PlasmoidModel::removeApplet(Plasma::Applet *applet)
{
    int rows = rowCount();
    for (int i = 0; i < rows; i++) {
        QStandardItem *currentItem = item(i);
        QVariant plugin = currentItem->data(static_cast<int>(BaseRole::ItemId));
        if (plugin.isValid() && plugin.value<QString>() == applet->pluginMetaData().pluginId()) {
            currentItem->setData(false, static_cast<int>(BaseRole::CanRender));
            currentItem->setData(QVariant(), static_cast<int>(Role::Applet));
            currentItem->setData(false, static_cast<int>(Role::HasApplet));
            return;
        }
    }
}

StatusNotifierModel::StatusNotifierModel(QObject *parent) : QStandardItemModel(parent)
{
    m_dataEngine = dataEngine(QStringLiteral("statusnotifieritem"));

    connect(m_dataEngine, &Plasma::DataEngine::sourceAdded, this, &StatusNotifierModel::addSource);
    connect(m_dataEngine, &Plasma::DataEngine::sourceRemoved, this, &StatusNotifierModel::removeSource);

    m_dataEngine->connectAllSources(this);
}

QHash<int, QByteArray> StatusNotifierModel::roleNames() const
{
    QHash<int, QByteArray> roles = QStandardItemModel::roleNames();

    roles.insert(static_cast<int>(BaseRole::ItemType), QByteArrayLiteral("itemType"));
    roles.insert(static_cast<int>(BaseRole::ItemId), QByteArrayLiteral("itemId"));
    roles.insert(static_cast<int>(BaseRole::CanRender), QByteArrayLiteral("canRender"));

    roles.insert(static_cast<int>(Role::DataEngineSource), QByteArrayLiteral("DataEngineSource"));
    roles.insert(static_cast<int>(Role::AttentionIcon), QByteArrayLiteral("AttentionIcon"));
    roles.insert(static_cast<int>(Role::AttentionIconName), QByteArrayLiteral("AttentionIconName"));
    roles.insert(static_cast<int>(Role::AttentionMovieName), QByteArrayLiteral("AttentionMovieName"));
    roles.insert(static_cast<int>(Role::Category), QByteArrayLiteral("Category"));
    roles.insert(static_cast<int>(Role::Icon), QByteArrayLiteral("Icon"));
    roles.insert(static_cast<int>(Role::IconName), QByteArrayLiteral("IconName"));
    roles.insert(static_cast<int>(Role::IconThemePath), QByteArrayLiteral("IconThemePath"));
    roles.insert(static_cast<int>(Role::IconsChanged), QByteArrayLiteral("IconsChanged"));
    roles.insert(static_cast<int>(Role::Id), QByteArrayLiteral("Id"));
    roles.insert(static_cast<int>(Role::ItemIsMenu), QByteArrayLiteral("ItemIsMenu"));
    roles.insert(static_cast<int>(Role::OverlayIconName), QByteArrayLiteral("OverlayIconName"));
    roles.insert(static_cast<int>(Role::Status), QByteArrayLiteral("Status"));
    roles.insert(static_cast<int>(Role::StatusChanged), QByteArrayLiteral("StatusChanged"));
    roles.insert(static_cast<int>(Role::Title), QByteArrayLiteral("Title"));
    roles.insert(static_cast<int>(Role::TitleChanged), QByteArrayLiteral("TitleChanged"));
    roles.insert(static_cast<int>(Role::ToolTipChanged), QByteArrayLiteral("ToolTipChanged"));
    roles.insert(static_cast<int>(Role::ToolTipIcon), QByteArrayLiteral("ToolTipIcon"));
    roles.insert(static_cast<int>(Role::ToolTipSubTitle), QByteArrayLiteral("ToolTipSubTitle"));
    roles.insert(static_cast<int>(Role::ToolTipTitle), QByteArrayLiteral("ToolTipTitle"));
    roles.insert(static_cast<int>(Role::WindowId), QByteArrayLiteral("WindowId"));

    return roles;
}

Plasma::Service *StatusNotifierModel::serviceForSource(const QString &source)
{
    if (m_services.contains(source)) {
        return m_services.value(source);
    }

    Plasma::Service *service = m_dataEngine->serviceForSource(source);
    if (!service) {
        return nullptr;
    }
    m_services[source] = service;
    return service;
}

void StatusNotifierModel::addSource(const QString &source)
{
    m_dataEngine->connectSource(source, this);
}

void StatusNotifierModel::removeSource(const QString &source)
{
    m_dataEngine->disconnectSource(source, this);
    if (m_sources.contains(source)) {
        removeRow(m_sources.indexOf(source));
        m_sources.removeAll(source);
    }

    QHash<QString, Plasma::Service *>::iterator it = m_services.find(source);
    if (it != m_services.end()) {
        delete it.value();
        m_services.erase(it);
    }
}

void StatusNotifierModel::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    QStandardItem *dataItem;
    if (m_sources.contains(sourceName)) {
        dataItem = item(m_sources.indexOf(sourceName));
    } else {
        dataItem = new QStandardItem();
        dataItem->setData("StatusNotifier", static_cast<int>(BaseRole::ItemType));
        dataItem->setData(true, static_cast<int>(BaseRole::CanRender));
    }

    dataItem->setData(data.value("Title"), Qt::DisplayRole);
    QVariant icon = data.value("Icon");
    if (icon.isValid() && icon.canConvert<QIcon>() && !icon.value<QIcon>().isNull()) {
        dataItem->setData(icon, Qt::DecorationRole);
    } else {
        dataItem->setData(data.value("IconName"), Qt::DecorationRole);
    }

    dataItem->setData(data.value("Id"), static_cast<int>(BaseRole::ItemId));

    dataItem->setData(sourceName, static_cast<int>(Role::DataEngineSource));
    updateItemData(dataItem, data, Role::AttentionIcon);
    updateItemData(dataItem, data, Role::AttentionIconName);
    updateItemData(dataItem, data, Role::AttentionMovieName);
    updateItemData(dataItem, data, Role::Category);
    updateItemData(dataItem, data, Role::Icon);
    updateItemData(dataItem, data, Role::IconName);
    updateItemData(dataItem, data, Role::IconThemePath);
    updateItemData(dataItem, data, Role::IconsChanged);
    updateItemData(dataItem, data, Role::Id);
    updateItemData(dataItem, data, Role::ItemIsMenu);
    updateItemData(dataItem, data, Role::OverlayIconName);
    updateItemData(dataItem, data, Role::Status);
    updateItemData(dataItem, data, Role::StatusChanged);
    updateItemData(dataItem, data, Role::Title);
    updateItemData(dataItem, data, Role::TitleChanged);
    updateItemData(dataItem, data, Role::ToolTipChanged);
    updateItemData(dataItem, data, Role::ToolTipIcon);
    updateItemData(dataItem, data, Role::ToolTipSubTitle);
    updateItemData(dataItem, data, Role::ToolTipTitle);
    updateItemData(dataItem, data, Role::WindowId);

    if (!m_sources.contains(sourceName)) {
        m_sources.append(sourceName);
        appendRow(dataItem);
    }
}

void StatusNotifierModel::updateItemData(QStandardItem *dataItem,
                                         const Plasma::DataEngine::Data &data, const Role role)
{
    int roleId = static_cast<int>(role);
    dataItem->setData(data.value(roleNames().value(roleId)), roleId);
}

SystemTrayModel::SystemTrayModel(QObject *parent) : KConcatenateRowsProxyModel(parent)
{
    m_roleNames = KConcatenateRowsProxyModel::roleNames();
}

QHash<int, QByteArray> SystemTrayModel::roleNames() const
{
    return m_roleNames;
}

void SystemTrayModel::addSourceModel(QAbstractItemModel *sourceModel)
{
    m_roleNames.unite(sourceModel->roleNames());
    KConcatenateRowsProxyModel::addSourceModel(sourceModel);
}
