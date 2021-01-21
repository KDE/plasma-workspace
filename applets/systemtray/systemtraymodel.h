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

#ifndef SYSTEMTRAYMODEL_H
#define SYSTEMTRAYMODEL_H

#include <QAbstractListModel>
#include <QList>

#include <KCoreAddons/KPluginMetaData>
#include <KItemModels/KConcatenateRowsProxyModel>
#include <Plasma/DataEngine>
#include <Plasma/DataEngineConsumer>

namespace Plasma
{
class Applet;
class PluginLoader;
}

class PlasmoidRegistry;
class SystemTraySettings;

/**
 * @brief Base class for models used in System Tray.
 *
 */
class BaseModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum class BaseRole {
        ItemType = Qt::UserRole + 1,
        ItemId,
        CanRender,
        Category,
        Status,
        EffectiveStatus,
        LastBaseRole,
    };

    explicit BaseModel(QPointer<SystemTraySettings> settings, QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void onConfigurationChanged();

protected:
    Plasma::Types::ItemStatus calculateEffectiveStatus(bool canRender, Plasma::Types::ItemStatus status, QString itemId) const;

private:
    QPointer<SystemTraySettings> m_settings;

    bool m_showAllItems;
    QStringList m_shownItems;
    QStringList m_hiddenItems;
};

/**
 * @brief Data model for plasmoids/applets.
 */
class PlasmoidModel : public BaseModel
{
    Q_OBJECT
public:
    enum class Role {
        Applet = static_cast<int>(BaseModel::BaseRole::LastBaseRole) + 1,
        HasApplet,
    };

    explicit PlasmoidModel(QPointer<SystemTraySettings> settings, QPointer<PlasmoidRegistry> plasmoidRegistry, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void addApplet(Plasma::Applet *applet);
    void removeApplet(Plasma::Applet *applet);

private Q_SLOTS:
    void appendRow(const KPluginMetaData &pluginMetaData);
    void removeRow(const QString &pluginId);

private:
    struct Item {
        KPluginMetaData pluginMetaData;
        Plasma::Applet *applet = nullptr;
    };

    int indexOfPluginId(const QString &pluginId) const;

    QPointer<PlasmoidRegistry> m_plasmoidRegistry;

    QVector<Item> m_items;
};

/**
 * @brief Data model for Status Notifier Items (SNI).
 */
class StatusNotifierModel : public BaseModel, public Plasma::DataEngineConsumer
{
    Q_OBJECT
public:
    enum class Role {
        DataEngineSource = static_cast<int>(BaseModel::BaseRole::LastBaseRole) + 100,
        Service,
        AttentionIcon,
        AttentionIconName,
        AttentionMovieName,
        Category,
        Icon,
        IconName,
        IconThemePath,
        Id,
        ItemIsMenu,
        OverlayIconName,
        Status,
        Title,
        ToolTipSubTitle,
        ToolTipTitle,
        WindowId,
    };

    StatusNotifierModel(QPointer<SystemTraySettings> settings, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void addSource(const QString &source);
    void removeSource(const QString &source);
    void dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data);

private:
    struct Item {
        QString source;
        Plasma::Service *service = nullptr;
    };

    int indexOfSource(const QString &source) const;

    Plasma::DataEngine *m_dataEngine = nullptr;
    QVector<Item> m_items;
};

/**
 * @brief Cantenating model for system tray, that can expose multiple data models as one.
 */
class SystemTrayModel : public KConcatenateRowsProxyModel
{
    Q_OBJECT
public:
    explicit SystemTrayModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

    void addSourceModel(QAbstractItemModel *sourceModel);

private:
    QHash<int, QByteArray> m_roleNames;
};

#endif // SYSTEMTRAYMODEL_H
