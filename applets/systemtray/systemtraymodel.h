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

#ifndef SYSTEMTRAYMODEL_H
#define SYSTEMTRAYMODEL_H

#include <QStandardItemModel>

#include <KItemModels/KConcatenateRowsProxyModel>
#include <Plasma/DataEngineConsumer>
#include <Plasma/DataEngine>

namespace Plasma {
    class Applet;
}

class BaseModel: public QStandardItemModel
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
        LastBaseRole
    };

    explicit BaseModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

public slots:
    void onConfigurationChanged(const KConfigGroup &config);

private slots:
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

private:
    void updateEffectiveStatus(QStandardItem *dataItem);
    Plasma::Types::ItemStatus calculateEffectiveStatus(QStandardItem *dataItem);
    Plasma::Types::ItemStatus readStatus(QStandardItem *dataItem) const;

    bool m_showAllItems;
    QStringList m_shownItems;
    QStringList m_hiddenItems;
};

class PlasmoidModel: public BaseModel
{
    Q_OBJECT
public:
    enum class Role {
        Applet = static_cast<int>(BaseModel::BaseRole::LastBaseRole) + 1,
        HasApplet
    };

    explicit PlasmoidModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;

public slots:
    void addApplet(Plasma::Applet *applet);
    void removeApplet(Plasma::Applet *applet);
};

class StatusNotifierModel : public BaseModel, public Plasma::DataEngineConsumer {
    Q_OBJECT
public:
    enum class Role {
        DataEngineSource = static_cast<int>(BaseModel::BaseRole::LastBaseRole) + 100,
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
        WindowId
    };

    StatusNotifierModel(QObject* parent);

    QHash<int, QByteArray> roleNames() const override;

    Plasma::Service *serviceForSource(const QString &source);

public slots:
    void addSource(const QString &source);
    void removeSource(const QString &source);
    void dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data);

private:
    void updateItemData(QStandardItem *dataItem, const Plasma::DataEngine::Data &data, const Role role);

    Plasma::DataEngine *m_dataEngine = nullptr;
    QStringList m_sources;
    QHash<QString, Plasma::Service *> m_services;
};

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
