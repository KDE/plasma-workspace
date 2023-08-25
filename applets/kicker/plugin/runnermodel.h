/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractmodel.h"

#include <KConfigWatcher>
#include <KRunner/QueryMatch>
#include <KSharedConfig>
#include <QAbstractListModel>
#include <QTimer>

namespace KRunner
{
class RunnerManager;
}

class AbstractModel;
class RunnerMatchesModel;

class RunnerModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(AbstractModel *favoritesModel READ favoritesModel WRITE setFavoritesModel NOTIFY favoritesModelChanged)
    Q_PROPERTY(QObject *appletInterface READ appletInterface WRITE setAppletInterface NOTIFY appletInterfaceChanged)
    Q_PROPERTY(QStringList runners READ runners WRITE setRunners NOTIFY runnersChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(bool mergeResults READ mergeResults WRITE setMergeResults NOTIFY mergeResultsChanged)

public:
    explicit RunnerModel(QObject *parent = nullptr);
    ~RunnerModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int count() const;

    Q_INVOKABLE RunnerMatchesModel *modelForRow(int row);

    QStringList runners() const;
    void setRunners(const QStringList &runners);

    QString query() const;
    void setQuery(const QString &query);

    AbstractModel *favoritesModel() const;
    void setFavoritesModel(AbstractModel *model);

    QObject *appletInterface() const;
    void setAppletInterface(QObject *appletInterface);

    bool mergeResults() const;
    void setMergeResults(bool merge);

Q_SIGNALS:
    void countChanged() const;
    void favoritesModelChanged() const;
    void appletInterfaceChanged() const;
    void runnersChanged() const;
    void queryChanged() const;
    void queryFinished();
    void mergeResultsChanged() const;
    void requestUpdateQuery(const QString &query);

private Q_SLOTS:
    void startQuery();

private:
    void clear();
    // Initializes the models when the first query is run or mergeResults changed
    void initializeModels();

    AbstractModel *m_favoritesModel = nullptr;
    QObject *m_appletInterface = nullptr;
    QStringList m_runners;
    QList<RunnerMatchesModel *> m_models;
    QString m_query;
    QTimer m_queryTimer;
    bool m_mergeResults;
    int m_queryingModels = 0;
    KSharedConfigPtr m_krunnerConfig;
    KConfigWatcher::Ptr m_configWatcher;
    QStringList m_favoritePluginIds;
};
