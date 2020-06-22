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

BaseModel::BaseModel(QObject *parent)
    : QStandardItemModel(parent),
      m_showAllItems(false)
{
    connect(this, &BaseModel::rowsInserted, this, &BaseModel::onRowsInserted);
    connect(this, &BaseModel::dataChanged, this, &BaseModel::onDataChanged);
}

QHash<int, QByteArray> BaseModel::roleNames() const
{
    QHash<int, QByteArray> roles = QStandardItemModel::roleNames();

    roles.insert(static_cast<int>(BaseRole::ItemType), QByteArrayLiteral("itemType"));
    roles.insert(static_cast<int>(BaseRole::ItemId), QByteArrayLiteral("itemId"));
    roles.insert(static_cast<int>(BaseRole::CanRender), QByteArrayLiteral("canRender"));
    roles.insert(static_cast<int>(BaseRole::Category), QByteArrayLiteral("category"));
    roles.insert(static_cast<int>(BaseRole::Status), QByteArrayLiteral("status"));
    roles.insert(static_cast<int>(BaseRole::EffectiveStatus), QByteArrayLiteral("effectiveStatus"));

    return roles;
}

void BaseModel::onConfigurationChanged(const KConfigGroup &config)
{
    if (!config.isValid()) {
        return;
    }

    const KConfigGroup generalGroup = config.group("General");

    m_showAllItems = generalGroup.readEntry("showAllItems", false);
    m_shownItems = generalGroup.readEntry("shownItems", QStringList());
    m_hiddenItems = generalGroup.readEntry("hiddenItems", QStringList());

    for (int i = 0; i < rowCount(); i++) {
        QStandardItem *dataItem = item(i);
        updateEffectiveStatus(dataItem);
    }
}

void BaseModel::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    if (parent.isValid()) {
        return;
    }

    for (int i = first; i <= last; ++i) {
        QStandardItem *dataItem = item(i);
        updateEffectiveStatus(dataItem);
    }
}

void BaseModel::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    if (roles.contains(static_cast<int>(BaseRole::Status)) || roles.contains(static_cast<int>(BaseRole::CanRender))) {
        for (int i = topLeft.row(); i <= bottomRight.row(); i++) {
            QStandardItem *dataItem = item(i);
            updateEffectiveStatus(dataItem);
        }
    }
}

void BaseModel::updateEffectiveStatus(QStandardItem *dataItem)
{
    Plasma::Types::ItemStatus status = calculateEffectiveStatus(dataItem);
    dataItem->setData(status, static_cast<int>(BaseModel::BaseRole::EffectiveStatus));
}

Plasma::Types::ItemStatus BaseModel::calculateEffectiveStatus(QStandardItem *dataItem)
{
    bool canRender = dataItem->data(static_cast<int>(BaseRole::CanRender)).toBool();
    if (!canRender) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    Plasma::Types::ItemStatus status = readStatus(dataItem);
    if (status == Plasma::Types::ItemStatus::HiddenStatus) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    QString itemId = dataItem->data(static_cast<int>(BaseRole::ItemId)).toString();

    bool forcedShown = m_showAllItems || m_shownItems.contains(itemId);
    bool forcedHidden = m_hiddenItems.contains(itemId);

    if (forcedShown || (!forcedHidden && status != Plasma::Types::ItemStatus::PassiveStatus)) {
        return Plasma::Types::ItemStatus::ActiveStatus;
    } else {
        return Plasma::Types::ItemStatus::PassiveStatus;
    }
}

Plasma::Types::ItemStatus BaseModel::readStatus(QStandardItem *dataItem) const
{
    QVariant statusData = dataItem->data(static_cast<int>(BaseRole::Status));
    if (statusData.isValid()) {
        return statusData.value<Plasma::Types::ItemStatus>();
    } else {
        return Plasma::Types::ItemStatus::UnknownStatus;
    }
}

static QString plasmoidCategoryForMetadata(const KPluginMetaData &metadata)
{
    QString category = QStringLiteral("UnknownCategory");

    if (metadata.isValid()) {
        const QString notificationAreaCategory = metadata.value(QStringLiteral("X-Plasma-NotificationAreaCategory"));
        if (!notificationAreaCategory.isEmpty()) {
            category = notificationAreaCategory;
        }
    }

    return category;
}

PlasmoidModel::PlasmoidModel(QObject *parent) : BaseModel(parent)
{
    for (const auto &info : Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        if (!info.isValid() || info.value(QStringLiteral("X-Plasma-NotificationArea")) != "true") {
            continue;
        }

        QString name = info.name();
        const QString dbusactivation = info.value(QStringLiteral("X-Plasma-DBusActivationService"));
        if (!dbusactivation.isEmpty()) {
            name += i18n(" (Automatic load)");
        }
        QStandardItem *item = new QStandardItem(QIcon::fromTheme(info.iconName()), name);
        item->setData(info.pluginId(), static_cast<int>(BaseModel::BaseRole::ItemId));
        item->setData(QStringLiteral("Plasmoid"), static_cast<int>(BaseModel::BaseRole::ItemType));
        item->setData(false, static_cast<int>(BaseModel::BaseRole::CanRender));
        item->setData(plasmoidCategoryForMetadata(info), static_cast<int>(BaseModel::BaseRole::Category));
        item->setData(false, static_cast<int>(Role::HasApplet));
        appendRow(item);
    }
}

QHash<int, QByteArray> PlasmoidModel::roleNames() const
{
    QHash<int, QByteArray> roles = BaseModel::roleNames();

    roles.insert(static_cast<int>(Role::Applet), QByteArrayLiteral("applet"));
    roles.insert(static_cast<int>(Role::HasApplet), QByteArrayLiteral("hasApplet"));

    return roles;
}

void PlasmoidModel::addApplet(Plasma::Applet *applet)
{
    auto pluginMetaData = applet->pluginMetaData();
    QStandardItem *dataItem = nullptr;
    for (int i = 0; i < rowCount(); i++) {
        QStandardItem *currentItem = item(i);
        if (currentItem->data(static_cast<int>(BaseModel::BaseRole::ItemId)) == pluginMetaData.pluginId()) {
            dataItem = currentItem;
            break;
        }
    }

    if (!dataItem) {
        QString name = pluginMetaData.name();
        const QString dbusactivation = pluginMetaData.value(QStringLiteral("X-Plasma-DBusActivationService"));
        if (!dbusactivation.isEmpty()) {
            name += i18n(" (Automatic load)");
        }
        dataItem = new QStandardItem(QIcon::fromTheme(pluginMetaData.iconName()), name);
        appendRow(dataItem);
    }

    dataItem->setData(pluginMetaData.pluginId(), static_cast<int>(BaseModel::BaseRole::ItemId));
    dataItem->setData(plasmoidCategoryForMetadata(pluginMetaData), static_cast<int>(BaseModel::BaseRole::Category));
    dataItem->setData(applet->status(), static_cast<int>(BaseModel::BaseRole::Status));
    connect(applet, &Plasma::Applet::statusChanged, this, [dataItem] (Plasma::Types::ItemStatus status) {
        dataItem->setData(status, static_cast<int>(BaseModel::BaseRole::Status));
    });

    dataItem->setData(applet->property("_plasma_graphicObject"), static_cast<int>(Role::Applet));
    dataItem->setData(true, static_cast<int>(Role::HasApplet));

    // CanRender has to be the last one
    dataItem->setData(true, static_cast<int>(BaseModel::BaseRole::CanRender));
}

void PlasmoidModel::removeApplet(Plasma::Applet *applet)
{
    int rows = rowCount();
    for (int i = 0; i < rows; i++) {
        QStandardItem *currentItem = item(i);
        QVariant plugin = currentItem->data(static_cast<int>(BaseModel::BaseRole::ItemId));
        if (plugin.isValid() && plugin.value<QString>() == applet->pluginMetaData().pluginId()) {
            currentItem->setData(false, static_cast<int>(BaseModel::BaseRole::CanRender));
            currentItem->setData(QVariant(), static_cast<int>(Role::Applet));
            currentItem->setData(false, static_cast<int>(Role::HasApplet));
            applet->disconnect(this);
            return;
        }
    }
}

StatusNotifierModel::StatusNotifierModel(QObject *parent) : BaseModel(parent)
{
    m_dataEngine = dataEngine(QStringLiteral("statusnotifieritem"));

    connect(m_dataEngine, &Plasma::DataEngine::sourceAdded, this, &StatusNotifierModel::addSource);
    connect(m_dataEngine, &Plasma::DataEngine::sourceRemoved, this, &StatusNotifierModel::removeSource);

    m_dataEngine->connectAllSources(this);
}

QHash<int, QByteArray> StatusNotifierModel::roleNames() const
{
    QHash<int, QByteArray> roles = BaseModel::roleNames();

    roles.insert(static_cast<int>(Role::DataEngineSource), QByteArrayLiteral("DataEngineSource"));
    roles.insert(static_cast<int>(Role::AttentionIcon), QByteArrayLiteral("AttentionIcon"));
    roles.insert(static_cast<int>(Role::AttentionIconName), QByteArrayLiteral("AttentionIconName"));
    roles.insert(static_cast<int>(Role::AttentionMovieName), QByteArrayLiteral("AttentionMovieName"));
    roles.insert(static_cast<int>(Role::Category), QByteArrayLiteral("Category"));
    roles.insert(static_cast<int>(Role::Icon), QByteArrayLiteral("Icon"));
    roles.insert(static_cast<int>(Role::IconName), QByteArrayLiteral("IconName"));
    roles.insert(static_cast<int>(Role::IconThemePath), QByteArrayLiteral("IconThemePath"));
    roles.insert(static_cast<int>(Role::Id), QByteArrayLiteral("Id"));
    roles.insert(static_cast<int>(Role::ItemIsMenu), QByteArrayLiteral("ItemIsMenu"));
    roles.insert(static_cast<int>(Role::OverlayIconName), QByteArrayLiteral("OverlayIconName"));
    roles.insert(static_cast<int>(Role::Status), QByteArrayLiteral("Status"));
    roles.insert(static_cast<int>(Role::Title), QByteArrayLiteral("Title"));
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
        dataItem->setData(QStringLiteral("StatusNotifier"), static_cast<int>(BaseModel::BaseRole::ItemType));
        dataItem->setData(true, static_cast<int>(BaseModel::BaseRole::CanRender));
    }

    dataItem->setData(data.value("Title"), Qt::DisplayRole);
    QVariant icon = data.value("Icon");
    if (icon.isValid() && icon.canConvert<QIcon>() && !icon.value<QIcon>().isNull()) {
        dataItem->setData(icon, Qt::DecorationRole);
        dataItem->setData(icon, static_cast<int>(Role::Icon));
    } else {
        dataItem->setData(data.value("IconName"), Qt::DecorationRole);
        dataItem->setData(QVariant(), static_cast<int>(Role::Icon));
    }
    QVariant attentionIcon = data.value("AttentionIcon");
    if (attentionIcon.isValid() && attentionIcon.canConvert<QIcon>() && !attentionIcon.value<QIcon>().isNull()) {
        dataItem->setData(attentionIcon, static_cast<int>(Role::AttentionIcon));
    } else {
        dataItem->setData(QVariant(), static_cast<int>(Role::AttentionIcon));
    }

    dataItem->setData(data.value("Id"), static_cast<int>(BaseModel::BaseRole::ItemId));
    QVariant category = data.value("Category");
    dataItem->setData(category.isNull() ? QStringLiteral("UnknownCategory") : data.value("Category"), static_cast<int>(BaseModel::BaseRole::Category));

    QString status = data.value("Status").toString();
    if (status == QLatin1String("Active")) {
        dataItem->setData(Plasma::Types::ItemStatus::ActiveStatus, static_cast<int>(BaseModel::BaseRole::Status));
    } else if (status == QLatin1String("NeedsAttention")) {
        dataItem->setData(Plasma::Types::ItemStatus::NeedsAttentionStatus, static_cast<int>(BaseModel::BaseRole::Status));
    } else if (status == QLatin1String("Passive")) {
        dataItem->setData(Plasma::Types::ItemStatus::PassiveStatus, static_cast<int>(BaseModel::BaseRole::Status));
    } else {
        dataItem->setData(Plasma::Types::ItemStatus::UnknownStatus, static_cast<int>(BaseModel::BaseRole::Status));
    }

    dataItem->setData(sourceName, static_cast<int>(Role::DataEngineSource));
    updateItemData(dataItem, data, Role::AttentionIconName);
    updateItemData(dataItem, data, Role::AttentionMovieName);
    updateItemData(dataItem, data, Role::Category);
    updateItemData(dataItem, data, Role::IconName);
    updateItemData(dataItem, data, Role::IconThemePath);
    updateItemData(dataItem, data, Role::Id);
    updateItemData(dataItem, data, Role::ItemIsMenu);
    updateItemData(dataItem, data, Role::OverlayIconName);
    updateItemData(dataItem, data, Role::Status);
    updateItemData(dataItem, data, Role::Title);
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
    QHashIterator<int, QByteArray> it(sourceModel->roleNames());
    while (it.hasNext()) {
        it.next();

        if (!m_roleNames.contains(it.key())) {
            m_roleNames.insert(it.key(), it.value());
        }
    }

    KConcatenateRowsProxyModel::addSourceModel(sourceModel);
}
