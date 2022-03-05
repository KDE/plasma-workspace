/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "appentry.h"
#include "forwardingmodel.h"

#include <QSortFilterProxyModel>
#include <Solid/StorageAccess>

class SimpleFavoritesModel;

class QConcatenateTablesProxyModel;
class KFilePlacesModel;

namespace Solid
{
class Device;
}

class FilteredPlacesModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FilteredPlacesModel(QObject *parent = nullptr);
    ~FilteredPlacesModel() override;

    QUrl url(const QModelIndex &index) const;
    bool isDevice(const QModelIndex &index) const;
    Solid::Device deviceForIndex(const QModelIndex &index) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    KFilePlacesModel *m_placesModel;
};

class RunCommandModel : public AbstractModel
{
    Q_OBJECT

public:
    RunCommandModel(QObject *parent = nullptr);
    ~RunCommandModel() override;

    QString description() const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;
};

class ComputerModel : public ForwardingModel
{
    Q_OBJECT

    Q_PROPERTY(int appNameFormat READ appNameFormat WRITE setAppNameFormat NOTIFY appNameFormatChanged)
    Q_PROPERTY(QObject *appletInterface READ appletInterface WRITE setAppletInterface NOTIFY appletInterfaceChanged)
    Q_PROPERTY(QStringList systemApplications READ systemApplications WRITE setSystemApplications NOTIFY systemApplicationsChanged)

public:
    explicit ComputerModel(QObject *parent = nullptr);
    ~ComputerModel() override;

    QString description() const override;

    int appNameFormat() const;
    void setAppNameFormat(int format);

    QObject *appletInterface() const;
    void setAppletInterface(QObject *appletInterface);

    QStringList systemApplications() const;
    void setSystemApplications(const QStringList &apps);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE bool trigger(int row, const QString &actionId, const QVariant &argument) override;

Q_SIGNALS:
    void appNameFormatChanged() const;
    void appletInterfaceChanged() const;
    void systemApplicationsChanged() const;

private Q_SLOTS:
    void onSetupDone(Solid::ErrorType error, QVariant errorData, const QString &udi);

private:
    QConcatenateTablesProxyModel *m_concatProxy;
    RunCommandModel *m_runCommandModel;
    SimpleFavoritesModel *m_systemAppsModel;
    FilteredPlacesModel *m_filteredPlacesModel;
    AppEntry::NameFormat m_appNameFormat;
    QObject *m_appletInterface;
};
