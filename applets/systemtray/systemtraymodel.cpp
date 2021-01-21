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

#include "systemtraymodel.h"
#include "debug.h"

#include "plasmoidregistry.h"
#include "systemtraysettings.h"

#include <KLocalizedString>
#include <Plasma/Applet>
#include <Plasma/DataContainer>
#include <Plasma/Service>
#include <PluginLoader>

#include <QIcon>
#include <QQuickItem>

BaseModel::BaseModel(QPointer<SystemTraySettings> settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(settings)
    , m_showAllItems(m_settings->isShowAllItems())
    , m_shownItems(m_settings->shownItems())
    , m_hiddenItems(m_settings->hiddenItems())
{
    connect(m_settings, &SystemTraySettings::configurationChanged, this, &BaseModel::onConfigurationChanged);
}

QHash<int, QByteArray> BaseModel::roleNames() const
{
    return {
        {Qt::DisplayRole, QByteArrayLiteral("display")},
        {Qt::DecorationRole, QByteArrayLiteral("decoration")},
        {static_cast<int>(BaseRole::ItemType), QByteArrayLiteral("itemType")},
        {static_cast<int>(BaseRole::ItemId), QByteArrayLiteral("itemId")},
        {static_cast<int>(BaseRole::CanRender), QByteArrayLiteral("canRender")},
        {static_cast<int>(BaseRole::Category), QByteArrayLiteral("category")},
        {static_cast<int>(BaseRole::Status), QByteArrayLiteral("status")},
        {static_cast<int>(BaseRole::EffectiveStatus), QByteArrayLiteral("effectiveStatus")},
    };
}

void BaseModel::onConfigurationChanged()
{
    m_showAllItems = m_settings->isShowAllItems();
    m_shownItems = m_settings->shownItems();
    m_hiddenItems = m_settings->hiddenItems();

    for (int i = 0; i < rowCount(); i++) {
        dataChanged(index(i, 0), index(i, 0), {static_cast<int>(BaseModel::BaseRole::EffectiveStatus)});
    }
}

Plasma::Types::ItemStatus BaseModel::calculateEffectiveStatus(bool canRender, Plasma::Types::ItemStatus status, QString itemId) const
{
    if (!canRender) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    if (status == Plasma::Types::ItemStatus::HiddenStatus) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    bool forcedShown = m_showAllItems || m_shownItems.contains(itemId);
    bool forcedHidden = m_hiddenItems.contains(itemId);

    if (forcedShown || (!forcedHidden && status != Plasma::Types::ItemStatus::PassiveStatus)) {
        return Plasma::Types::ItemStatus::ActiveStatus;
    } else {
        return Plasma::Types::ItemStatus::PassiveStatus;
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

PlasmoidModel::PlasmoidModel(QPointer<SystemTraySettings> settings, QPointer<PlasmoidRegistry> plasmoidRegistry, QObject *parent)
    : BaseModel(settings, parent)
    , m_plasmoidRegistry(plasmoidRegistry)
{
    connect(m_plasmoidRegistry, &PlasmoidRegistry::pluginRegistered, this, &PlasmoidModel::appendRow);
    connect(m_plasmoidRegistry, &PlasmoidRegistry::pluginUnregistered, this, &PlasmoidModel::removeRow);

    const auto appletMetaDataList = m_plasmoidRegistry->systemTrayApplets();
    for (const auto &info : appletMetaDataList) {
        if (!info.isValid() || info.value(QStringLiteral("X-Plasma-NotificationArea")) != "true") {
            continue;
        }
        appendRow(info);
    }
}

QVariant PlasmoidModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const PlasmoidModel::Item &item = m_items[index.row()];
    const KPluginMetaData &pluginMetaData = item.pluginMetaData;
    const Plasma::Applet *applet = item.applet;

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole: {
            const QString dbusactivation = pluginMetaData.value(QStringLiteral("X-Plasma-DBusActivationService"));
            if (dbusactivation.isEmpty()) {
                return pluginMetaData.name();
            } else {
                return i18nc("Suffix added to the applet name if the applet is autoloaded via DBus activation", "%1 (Automatic load)", pluginMetaData.name());
            }
        }
        case Qt::DecorationRole: {
            QIcon icon = QIcon::fromTheme(pluginMetaData.iconName());
            return icon.isNull() ? QVariant() : icon;
        }
        default:
            return QVariant();
        }
    }

    if (role < static_cast<int>(Role::Applet)) {
        Plasma::Types::ItemStatus status = Plasma::Types::ItemStatus::UnknownStatus;
        if (applet) {
            status = applet->status();
        }

        switch (static_cast<BaseRole>(role)) {
        case BaseRole::ItemType:
            return QStringLiteral("Plasmoid");
        case BaseRole::ItemId:
            return pluginMetaData.pluginId();
        case BaseRole::CanRender:
            return applet != nullptr;
        case BaseRole::Category:
            return plasmoidCategoryForMetadata(pluginMetaData);
        case BaseRole::Status:
            return status;
        case BaseRole::EffectiveStatus:
            return calculateEffectiveStatus(applet != nullptr, status, pluginMetaData.pluginId());
        default:
            return QVariant();
        }
    }

    switch (static_cast<Role>(role)) {
    case Role::Applet:
        return applet ? applet->property("_plasma_graphicObject") : QVariant();
    case Role::HasApplet:
        return applet != nullptr;
    default:
        return QVariant();
    }
}

int PlasmoidModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
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

    int idx = indexOfPluginId(pluginMetaData.pluginId());

    if (idx < 0) {
        idx = rowCount();
        appendRow(pluginMetaData);
    }

    m_items[idx].applet = applet;
    connect(applet, &Plasma::Applet::statusChanged, this, [this, applet](Plasma::Types::ItemStatus status) {
        Q_UNUSED(status)
        int idx = indexOfPluginId(applet->pluginMetaData().pluginId());
        dataChanged(index(idx, 0), index(idx, 0), {static_cast<int>(BaseRole::Status)});
    });

    dataChanged(index(idx, 0), index(idx, 0));
}

void PlasmoidModel::removeApplet(Plasma::Applet *applet)
{
    int idx = indexOfPluginId(applet->pluginMetaData().pluginId());
    if (idx >= 0) {
        m_items[idx].applet = nullptr;
        dataChanged(index(idx, 0), index(idx, 0));
        applet->disconnect(this);
    }
}

void PlasmoidModel::appendRow(const KPluginMetaData &pluginMetaData)
{
    int idx = rowCount();
    beginInsertRows(QModelIndex(), idx, idx);

    PlasmoidModel::Item item;
    item.pluginMetaData = pluginMetaData;
    m_items.append(item);

    endInsertRows();
}

void PlasmoidModel::removeRow(const QString &pluginId)
{
    int idx = indexOfPluginId(pluginId);
    beginRemoveRows(QModelIndex(), idx, idx);
    m_items.removeAt(idx);
    endRemoveRows();
}

int PlasmoidModel::indexOfPluginId(const QString &pluginId) const
{
    for (int i = 0; i < rowCount(); i++) {
        if (m_items[i].pluginMetaData.pluginId() == pluginId) {
            return i;
        }
    }
    return -1;
}

StatusNotifierModel::StatusNotifierModel(QPointer<SystemTraySettings> settings, QObject *parent)
    : BaseModel(settings, parent)
{
    m_dataEngine = dataEngine(QStringLiteral("statusnotifieritem"));

    connect(m_dataEngine, &Plasma::DataEngine::sourceAdded, this, &StatusNotifierModel::addSource);
    connect(m_dataEngine, &Plasma::DataEngine::sourceRemoved, this, &StatusNotifierModel::removeSource);

    m_dataEngine->connectAllSources(this);
}

static Plasma::Types::ItemStatus extractStatus(const Plasma::DataEngine::Data &sniData)
{
    QString status = sniData.value(QStringLiteral("Status")).toString();
    if (status == QLatin1String("Active")) {
        return Plasma::Types::ItemStatus::ActiveStatus;
    } else if (status == QLatin1String("NeedsAttention")) {
        return Plasma::Types::ItemStatus::NeedsAttentionStatus;
    } else if (status == QLatin1String("Passive")) {
        return Plasma::Types::ItemStatus::PassiveStatus;
    } else {
        return Plasma::Types::ItemStatus::UnknownStatus;
    }
}

static QVariant extractIcon(const Plasma::DataEngine::Data &sniData, const QString &key, const QVariant &defaultValue = QVariant())
{
    QVariant icon = sniData.value(key);
    if (icon.isValid() && icon.canConvert<QIcon>() && !icon.value<QIcon>().isNull()) {
        return icon;
    } else {
        return defaultValue;
    }
}

QVariant StatusNotifierModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    StatusNotifierModel::Item item = m_items[index.row()];
    Plasma::DataContainer *dataContainer = m_dataEngine->containerForSource(item.source);
    const Plasma::DataEngine::Data &sniData = dataContainer->data();

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole:
            return sniData.value(QStringLiteral("Title"));
        case Qt::DecorationRole:
            return extractIcon(sniData, QStringLiteral("Icon"), sniData.value(QStringLiteral("IconName")));
        default:
            return QVariant();
        }
    }

    if (role < static_cast<int>(Role::DataEngineSource)) {
        switch (static_cast<BaseRole>(role)) {
        case BaseRole::ItemType:
            return QStringLiteral("StatusNotifier");
        case BaseRole::ItemId:
            return sniData.value(QStringLiteral("Id"));
        case BaseRole::CanRender:
            return true;
        case BaseRole::Category: {
            QVariant category = sniData.value(QStringLiteral("Category"));
            return category.isNull() ? QStringLiteral("UnknownCategory") : sniData.value(QStringLiteral("Category"));
        }
        case BaseRole::Status:
            return extractStatus(sniData);
        case BaseRole::EffectiveStatus:
            return calculateEffectiveStatus(true, extractStatus(sniData), sniData.value(QStringLiteral("Id")).toString());
        default:
            return QVariant();
        }
    }

    switch (static_cast<Role>(role)) {
    case Role::DataEngineSource:
        return item.source;
    case Role::Service:
        return QVariant::fromValue(item.service);
    case Role::AttentionIcon:
        return extractIcon(sniData, QStringLiteral("AttentionIcon"));
    case Role::AttentionIconName:
        return sniData.value(QStringLiteral("AttentionIconName"));
    case Role::AttentionMovieName:
        return sniData.value(QStringLiteral("AttentionMovieName"));
    case Role::Category:
        return sniData.value(QStringLiteral("Category"));
    case Role::Icon:
        return extractIcon(sniData, QStringLiteral("Icon"));
    case Role::IconName:
        return sniData.value(QStringLiteral("IconName"));
    case Role::IconThemePath:
        return sniData.value(QStringLiteral("IconThemePath"));
    case Role::Id:
        return sniData.value(QStringLiteral("Id"));
    case Role::ItemIsMenu:
        return sniData.value(QStringLiteral("ItemIsMenu"));
    case Role::OverlayIconName:
        return sniData.value(QStringLiteral("OverlayIconName"));
    case Role::Status:
        return extractStatus(sniData);
    case Role::Title:
        return sniData.value(QStringLiteral("Title"));
    case Role::ToolTipSubTitle:
        return sniData.value(QStringLiteral("ToolTipSubTitle"));
    case Role::ToolTipTitle:
        return sniData.value(QStringLiteral("ToolTipTitle"));
    case Role::WindowId:
        return sniData.value(QStringLiteral("WindowId"));
    default:
        return QVariant();
    }
}

int StatusNotifierModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QHash<int, QByteArray> StatusNotifierModel::roleNames() const
{
    QHash<int, QByteArray> roles = BaseModel::roleNames();

    roles.insert(static_cast<int>(Role::DataEngineSource), QByteArrayLiteral("DataEngineSource"));
    roles.insert(static_cast<int>(Role::Service), QByteArrayLiteral("Service"));
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

void StatusNotifierModel::addSource(const QString &source)
{
    m_dataEngine->connectSource(source, this);
}

void StatusNotifierModel::removeSource(const QString &source)
{
    m_dataEngine->disconnectSource(source, this);

    int idx = indexOfSource(source);
    if (idx >= 0) {
        beginRemoveRows(QModelIndex(), idx, idx);
        delete m_items[idx].service;
        m_items.removeAt(idx);
        endRemoveRows();
    }
}

void StatusNotifierModel::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(data)

    int idx = indexOfSource(sourceName);

    if (idx < 0) {
        int count = rowCount();
        beginInsertRows(QModelIndex(), count, count);

        StatusNotifierModel::Item item;
        item.source = sourceName;
        item.service = m_dataEngine->serviceForSource(sourceName);

        m_items.append(item);

        endInsertRows();
    } else {
        dataChanged(index(idx, 0), index(idx, 0));
    }
}

int StatusNotifierModel::indexOfSource(const QString &source) const
{
    for (int i = 0; i < rowCount(); i++) {
        if (m_items[i].source == source) {
            return i;
        }
    }
    return -1;
}

SystemTrayModel::SystemTrayModel(QObject *parent)
    : KConcatenateRowsProxyModel(parent)
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
