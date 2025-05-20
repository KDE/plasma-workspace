/*
    SPDX-FileCopyrightText: 2020 Konrad Materka <materka@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "systemtraymodel.h"
#include "debug.h"

#include "plasmoidregistry.h"
#include "statusnotifieritemhost.h"
#include "statusnotifieritemsource.h"
#include "systemtraysettings.h"

#include <KLocalizedString>
#include <KService>
#include <Plasma/Applet>
#include <Plasma5Support/DataContainer>
#include <PlasmaQuick/AppletQuickItem>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QIcon>
#include <QQuickItem>

BaseModel::BaseModel(QPointer<SystemTraySettings> settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(settings)
    , m_showAllItems(m_settings ? m_settings->isShowAllItems() : true)
    , m_shownItems(m_settings ? m_settings->shownItems() : decltype(m_shownItems){})
    , m_hiddenItems(m_settings ? m_settings->hiddenItems() : decltype(m_hiddenItems){})
{
    if (m_settings) {
        connect(m_settings, &SystemTraySettings::configurationChanged, this, &BaseModel::onConfigurationChanged);
    }
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

    if (rowCount() == 0) {
        return; // Avoid assertion
    }

    Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0), {static_cast<int>(BaseModel::BaseRole::EffectiveStatus)});
}

Plasma::Types::ItemStatus BaseModel::calculateEffectiveStatus(bool canRender, Plasma::Types::ItemStatus status, QString itemId) const
{
    if (!canRender) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    }

    bool forcedShown = m_showAllItems || m_shownItems.contains(itemId);
    bool forcedHidden = m_hiddenItems.contains(itemId);
    bool isDisabledSni = m_settings->isDisabledStatusNotifier(itemId);

    if (!forcedShown && (status == Plasma::Types::ItemStatus::HiddenStatus || isDisabledSni)) {
        return Plasma::Types::ItemStatus::HiddenStatus;
    } else if (forcedShown || (!forcedHidden && status != Plasma::Types::ItemStatus::PassiveStatus)) {
        return Plasma::Types::ItemStatus::ActiveStatus;
    } else {
        return Plasma::Types::ItemStatus::PassiveStatus;
    }
}

static QString plasmoidCategoryForMetadata(const KPluginMetaData &metadata)
{
    Q_ASSERT(metadata.isValid());
    return metadata.value(u"X-Plasma-NotificationAreaCategory");
}

PlasmoidModel::PlasmoidModel(const QPointer<SystemTraySettings> &settings, const QPointer<PlasmoidRegistry> &plasmoidRegistry, QObject *parent)
    : BaseModel(settings, parent)
    , m_plasmoidRegistry(plasmoidRegistry)
{
    connect(m_plasmoidRegistry, &PlasmoidRegistry::pluginRegistered, this, &PlasmoidModel::appendRow);
    connect(m_plasmoidRegistry, &PlasmoidRegistry::pluginUnregistered, this, &PlasmoidModel::removeRow);

    const auto appletMetaDataList = m_plasmoidRegistry->systemTrayApplets();
    for (const auto &info : appletMetaDataList) {
        if (!info.isValid() || info.value(u"X-Plasma-NotificationAreaCategory").isEmpty()) {
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
    Plasma::Applet *applet = item.applet;

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole: {
            return pluginMetaData.name();
        }
        case Qt::DecorationRole: {
            QIcon icon = QIcon::fromTheme(applet ? applet->icon() : QString(), QIcon::fromTheme(pluginMetaData.iconName()));
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
        return applet ? QVariant::fromValue(PlasmaQuick::AppletQuickItem::itemForApplet(applet)) : QVariant();
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
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0), {static_cast<int>(BaseRole::Status)});
    });

    Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
}

void PlasmoidModel::removeApplet(Plasma::Applet *applet)
{
    int idx = indexOfPluginId(applet->pluginMetaData().pluginId());
    if (idx >= 0) {
        m_items[idx].applet = nullptr;
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
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

StatusNotifierModel::StatusNotifierModel(QObject *parent)
    : BaseModel(nullptr, parent)
{
    init();
}

StatusNotifierModel::StatusNotifierModel(QPointer<SystemTraySettings> settings, QObject *parent)
    : BaseModel(settings, parent)
{
    init();
}

static Plasma::Types::ItemStatus extractStatus(const StatusNotifierItemSource *sniData)
{
    QString status = sniData->status();
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

static QVariant extractIcon(const QIcon &icon, const QVariant &defaultValue = QVariant())
{
    if (!icon.isNull()) {
        return icon;
    } else {
        return defaultValue;
    }
}

static QString extractItemId(const StatusNotifierItemSource *sniData)
{
    const QString itemId = sniData->id();
    // Bug 378910: workaround for Dropbox not following the SNI specification
    if (itemId.startsWith(QLatin1String("dropbox-client-"))) {
        return QLatin1String("dropbox-client-PID");
    } else {
        return itemId;
    }
}

QVariant StatusNotifierModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const StatusNotifierModel::Item &item = m_items[index.row()];
    StatusNotifierItemSource *sniData = m_sniHost->itemForService(item.source);

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole:
            return sniData->title();
        case Qt::DecorationRole:
            if (!sniData->iconName().isNull()) {
                return sniData->iconName();
            } else {
                return extractIcon(sniData->icon());
            }
        default:
            return QVariant();
        }
    }

    const QString itemId = extractItemId(sniData);

    if (role < static_cast<int>(Role::DataEngineSource)) {
        switch (static_cast<BaseRole>(role)) {
        case BaseRole::ItemType:
            return QStringLiteral("StatusNotifier");
        case BaseRole::ItemId:
            return itemId;
        case BaseRole::CanRender:
            return true;
        case BaseRole::Category: {
            QVariant category = sniData->category();
            return category.isNull() ? QStringLiteral("UnknownCategory") : sniData->category();
        }
        case BaseRole::Status:
            return extractStatus(sniData);
        case BaseRole::EffectiveStatus:
            return calculateEffectiveStatus(true, extractStatus(sniData), itemId);
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
        return extractIcon(sniData->attentionIcon());
    case Role::AttentionIconName:
        return sniData->attentionIconName();
    case Role::AttentionMovieName:
        return sniData->attentionMovieName();
    case Role::Category:
        return sniData->category();
    case Role::Icon:
        return extractIcon(sniData->icon());
    case Role::IconName:
        return sniData->iconName();
    case Role::IconThemePath:
        return sniData->iconThemePath();
    case Role::Id:
        return itemId;
    case Role::ItemIsMenu:
        return sniData->itemIsMenu();
    case Role::OverlayIconName:
        return sniData->overlayIconName();
    case Role::Status:
        return extractStatus(sniData);
    case Role::Title:
        return sniData->title();
    case Role::ToolTipSubTitle:
        return sniData->toolTipSubTitle();
    case Role::ToolTipTitle:
        return sniData->toolTipTitle();
    case Role::WindowId:
        return sniData->windowId();
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
    int count = rowCount();
    beginInsertRows(QModelIndex(), count, count);

    StatusNotifierModel::Item item;
    item.source = source;

    StatusNotifierItemSource *sni = m_sniHost->itemForService(source);
    connect(sni, &StatusNotifierItemSource::dataUpdated, this, [=, this]() {
        dataUpdated(source);
    });
    item.service = sni->createService();
    m_items.append(item);
    endInsertRows();
}

void StatusNotifierModel::removeSource(const QString &source)
{
    int idx = indexOfSource(source);
    if (idx >= 0) {
        beginRemoveRows(QModelIndex(), idx, idx);
        delete m_items[idx].service;
        m_items.removeAt(idx);
        endRemoveRows();
    }
}

void StatusNotifierModel::dataUpdated(const QString &sourceName)
{
    int idx = indexOfSource(sourceName);

    if (idx >= 0) {
        Q_EMIT dataChanged(index(idx, 0), index(idx, 0));
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

void StatusNotifierModel::init()
{
    m_sniHost = StatusNotifierItemHost::self();

    connect(m_sniHost, &StatusNotifierItemHost::itemAdded, this, &StatusNotifierModel::addSource);
    connect(m_sniHost, &StatusNotifierItemHost::itemRemoved, this, &StatusNotifierModel::removeSource);

    for (const QStringList services = m_sniHost->services(); const QString &service : services) {
        addSource(service);
    }
}

const QString DBUS_SERVICE_BACKGROUNDAPPS(QLatin1String("org.freedesktop.background.Monitor"));
const QString DBUS_PATH_BACKGROUNDAPPS(QLatin1String("/org/freedesktop/background/monitor"));
const QString DBUS_INTERFACE_BACKGROUNDAPPS(QLatin1String("org.freedesktop.background.Monitor"));
const QString DBUS_PROPERTY_NAME_BACKGROUNDAPPS(QLatin1String("BackgroundApps"));
const QString DBUS_INTERFACE_FDO_DBUS_PROPERTIES(QLatin1String("org.freedesktop.DBus.Properties"));
const QString DBUS_INTERFACE_FDO_APPLICATION(QLatin1String("org.freedesktop.Application"));

BackgroundAppsModel::BackgroundAppsModel(QPointer<SystemTraySettings> settings, QObject *parent)
    : BaseModel(settings, parent)
{
    auto dbusInterface =
        new QDBusInterface(DBUS_SERVICE_BACKGROUNDAPPS, DBUS_PATH_BACKGROUNDAPPS, DBUS_INTERFACE_FDO_DBUS_PROPERTIES, QDBusConnection::sessionBus(), this);
    QDBusPendingCall pcall = dbusInterface->asyncCall(QLatin1String("Get"), DBUS_INTERFACE_BACKGROUNDAPPS, DBUS_PROPERTY_NAME_BACKGROUNDAPPS);
    auto watcher = new QDBusPendingCallWatcher(pcall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, dbusInterface](QDBusPendingCallWatcher *w) {
        QDBusPendingReply<QVariant> reply(*w);
        if (reply.isError()) {
            qCWarning(SYSTEM_TRAY) << "Failed to get background apps:" << reply.error().message();
        } else {
            auto dbusReply = qdbus_cast<QList<QVariantMap>>(reply.value());
            updateApps(dbusReply);
        }
        w->deleteLater();
        dbusInterface->deleteLater();
    });

    QDBusConnection::sessionBus().connect(DBUS_SERVICE_BACKGROUNDAPPS,
                                          DBUS_PATH_BACKGROUNDAPPS,
                                          DBUS_INTERFACE_FDO_DBUS_PROPERTIES,
                                          QLatin1String("PropertiesChanged"),
                                          this,
                                          SLOT(dbusPropertiesChanged(QString, QVariantMap, QStringList)));
}

QVariant BackgroundAppsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    const BackgroundAppsModel::Item &item = m_items[index.row()];

    if (role <= Qt::UserRole) {
        switch (role) {
        case Qt::DisplayRole:
            return item.name;
        case Qt::DecorationRole:
            return item.icon;
        default:
            return QVariant();
        }
    }

    if (role < static_cast<int>(Role::InstanceId)) {
        switch (static_cast<BaseRole>(role)) {
        case BaseRole::ItemType:
            return QStringLiteral("BackgroundApp");
        case BaseRole::ItemId:
            return item.appId;
        case BaseRole::CanRender:
            return true;
        case BaseRole::Category: {
            return QStringLiteral("BackgroundApp");
        }
        case BaseRole::Status:
            return Plasma::Types::ItemStatus::PassiveStatus;
        case BaseRole::EffectiveStatus:
            return calculateEffectiveStatus(true, Plasma::Types::ItemStatus::PassiveStatus, item.appId);
        default:
            return QVariant();
        }
    }

    switch (static_cast<Role>(role)) {
    case Role::InstanceId:
        return item.instanceId;
    case Role::AppId:
        return item.appId;
    case Role::AppName:
        return item.name;
    case Role::AppIcon:
        return item.icon;
    case Role::Message:
        return item.message;
    default:
        return QVariant();
    }
}

int BackgroundAppsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QHash<int, QByteArray> BackgroundAppsModel::roleNames() const
{
    QHash<int, QByteArray> roles = BaseModel::roleNames();

    roles.insert(static_cast<int>(Role::InstanceId), QByteArrayLiteral("instanceId"));
    roles.insert(static_cast<int>(Role::AppId), QByteArrayLiteral("appId"));
    roles.insert(static_cast<int>(Role::AppName), QByteArrayLiteral("appName"));
    roles.insert(static_cast<int>(Role::AppIcon), QByteArrayLiteral("appIcon"));
    roles.insert(static_cast<int>(Role::Message), QByteArrayLiteral("message"));

    return roles;
}

void BackgroundAppsModel::dbusPropertiesChanged(const QString &interfaceName, const QVariantMap &properties, const QStringList &invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties);
    if (interfaceName != DBUS_INTERFACE_BACKGROUNDAPPS) {
        return;
    }
    auto it = properties.find(DBUS_PROPERTY_NAME_BACKGROUNDAPPS);
    if (it != properties.constEnd()) {
        auto dbusReply = qdbus_cast<QList<QVariantMap>>(it.value());
        updateApps(dbusReply);
    }
}

void BackgroundAppsModel::updateApps(const QList<QVariantMap> &apps)
{
    beginRemoveRows(QModelIndex(), 0, m_items.count() - 1);
    m_items.clear();
    endRemoveRows();
    beginInsertRows(QModelIndex(), 0, apps.count() - 1);
    m_items.reserve(apps.count());
    for (const QVariantMap &appMap : apps) {
        Item a;
        a.appId = appMap.value(QStringLiteral("app_id")).toString();
        a.instanceId = appMap.value(QStringLiteral("instance")).toString();
        a.message = appMap.value(QStringLiteral("message")).toString();
        auto servicePtr = KService::serviceByDesktopName(a.appId);
        if (servicePtr) {
            a.name = servicePtr->name();
            a.icon = servicePtr->icon();
        }
        m_items.append(a);
    }
    endInsertRows();
}

static QString appIdToDBusPath(const QString &appId)
{
    QString dbusPath = appId;
    dbusPath.replace(QLatin1Char('.'), QLatin1Char('/')).replace(QLatin1Char('-'), QLatin1Char('_'));
    dbusPath = QStringLiteral("/") + dbusPath;
    return dbusPath;
}

void BackgroundAppsModel::activateApp(const QString &appId, const QString &instanceId)
{
    Q_UNUSED(instanceId);
    auto dbusInterface = new QDBusInterface(appId, appIdToDBusPath(appId), DBUS_INTERFACE_FDO_APPLICATION, QDBusConnection::sessionBus(), this);
    QDBusPendingCall pcall = dbusInterface->asyncCall(QLatin1String("Activate"), QVariantMap());
    auto watcher = new QDBusPendingCallWatcher(pcall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [dbusInterface](QDBusPendingCallWatcher *w) {
        QDBusPendingReply reply(*w);
        if (reply.isError()) {
            qCWarning(SYSTEM_TRAY) << "Failed to activate background apps via dbus:" << reply.error().message();
        }

        w->deleteLater();
        dbusInterface->deleteLater();
    });
}

void BackgroundAppsModel::terminateApp(const QString &appId, const QString &instanceId)
{
    auto dbusInterface = new QDBusInterface(appId, appIdToDBusPath(appId), DBUS_INTERFACE_FDO_APPLICATION, QDBusConnection::sessionBus(), this);
    QDBusPendingCall pcall = dbusInterface->asyncCall(QLatin1String("ActivateAction"), QLatin1String("quit"), QList<QVariant>(), QVariantMap());
    auto watcher = new QDBusPendingCallWatcher(pcall, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, dbusInterface, instanceId](QDBusPendingCallWatcher *w) {
        QDBusPendingReply reply(*w);
        if (reply.isError()) {
            qCWarning(SYSTEM_TRAY) << "Failed to terminate background apps via dbus:" << reply.error().message();
        } else {
            // Immediately remove the app from the model. Otherwise there could be a delay of several seconds
            // before DBus emits the change signal of the BackgrouproundApps property, and the user might
            // think the quit action failed.
            for (int i = 0; i < m_items.count(); ++i) {
                if (m_items[i].instanceId == instanceId) {
                    beginRemoveRows(QModelIndex(), i, i);
                    m_items.removeAt(i);
                    endRemoveRows();
                    break;
                }
            }
        }

        w->deleteLater();
        dbusInterface->deleteLater();
    });
}

SystemTrayModel::SystemTrayModel(QObject *parent)
    : QConcatenateTablesProxyModel(parent)
{
    m_roleNames = QConcatenateTablesProxyModel::roleNames();
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

    QConcatenateTablesProxyModel::addSourceModel(sourceModel);
}

#include "moc_systemtraymodel.cpp"
