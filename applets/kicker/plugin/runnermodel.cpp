/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "runnermodel.h"
#include "runnermatchesmodel.h"

#include <QSet>

#include <KLocalizedString>
#include <KRunner/AbstractRunner>
#include <KRunner/RunnerManager>
#include <chrono>
#include <optional>

using namespace std::chrono_literals;

RunnerModel::RunnerModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_favoritesModel(nullptr)
    , m_appletInterface(nullptr)
    , m_mergeResults(false)
    , m_deleteWhenEmpty(false)
{
    m_queryTimer.setSingleShot(true);
    m_queryTimer.setInterval(10ms);
    connect(&m_queryTimer, &QTimer::timeout, this, &RunnerModel::startQuery);
}

RunnerModel::~RunnerModel()
{
}

QHash<int, QByteArray> RunnerModel::roleNames() const
{
    return {{Qt::DisplayRole, "display"}};
}

AbstractModel *RunnerModel::favoritesModel() const
{
    return m_favoritesModel;
}

void RunnerModel::setFavoritesModel(AbstractModel *model)
{
    if (m_favoritesModel != model) {
        m_favoritesModel = model;

        clear();

        if (!m_query.isEmpty()) {
            m_queryTimer.start();
        }

        Q_EMIT favoritesModelChanged();
    }
}

QObject *RunnerModel::appletInterface() const
{
    return m_appletInterface;
}

void RunnerModel::setAppletInterface(QObject *appletInterface)
{
    if (m_appletInterface != appletInterface) {
        m_appletInterface = appletInterface;

        clear();

        if (!m_query.isEmpty()) {
            m_queryTimer.start();
        }

        Q_EMIT appletInterfaceChanged();
    }
}

bool RunnerModel::deleteWhenEmpty() const
{
    return m_deleteWhenEmpty;
}

void RunnerModel::setDeleteWhenEmpty(bool deleteWhenEmpty)
{
    if (m_deleteWhenEmpty != deleteWhenEmpty) {
        m_deleteWhenEmpty = deleteWhenEmpty;

        clear();

        if (!m_query.isEmpty()) {
            m_queryTimer.start();
        }

        Q_EMIT deleteWhenEmptyChanged();
    }
}

bool RunnerModel::mergeResults() const
{
    return m_mergeResults;
}

void RunnerModel::setMergeResults(bool merge)
{
    if (m_mergeResults != merge) {
        m_mergeResults = merge;
        Q_EMIT mergeResultsChanged();

        // If we haven't lazy-initialzed our models, we do not need to re-create them
        if (!m_models.isEmpty()) {
            qDeleteAll(m_models);
            m_models.clear();
            // Just re-create all models,
            initializeModels();
        }
    }
}

QVariant RunnerModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_models.count()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return m_models.at(index.row())->name();
    }

    return QVariant();
}

int RunnerModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_models.count();
}

int RunnerModel::count() const
{
    return rowCount();
}

QObject *RunnerModel::modelForRow(int row)
{
    if (row < 0 || row >= m_models.count()) {
        return nullptr;
    }

    return m_models.at(row);
}

QStringList RunnerModel::runners() const
{
    return m_runners;
}

void RunnerModel::setRunners(const QStringList &runners)
{
    if (runners == m_runners) {
        return;
    }

    m_runners = runners;
    Q_EMIT runnersChanged();

    // Update the existing models only, if we have intialized the models
    if (!m_models.isEmpty()) {
        if (m_mergeResults) {
            Q_ASSERT(m_models.length() == 1);
            m_models.first()->runnerManager()->setAllowedRunners(runners);
        } else {
            // Just re-create all the models, it is an edge-case anyway
            qDeleteAll(m_models);
            m_models.clear();
            initializeModels();
        }
    }
}

QString RunnerModel::query() const
{
    return m_query;
}

void RunnerModel::setQuery(const QString &query)
{
    if (m_models.isEmpty()) {
        initializeModels();
    }
    if (m_query != query) {
        m_query = query;
        m_queryTimer.start();
        Q_EMIT queryChanged();
    }
}

void RunnerModel::startQuery()
{
    if (m_query.isEmpty()) {
        clear();
    } else {
        for (KRunner::ResultsModel *model : std::as_const(m_models)) {
            model->setQueryString(m_query);
        }
    }
}

void RunnerModel::clear()
{
    for (KRunner::ResultsModel *model : std::as_const(m_models)) {
        model->clear();
    }
}

void RunnerModel::initializeModels()
{
    beginResetModel();
    if (m_mergeResults) {
        auto model = new RunnerMatchesModel(QString(), i18n("Search results"), this);
        model->runnerManager()->setAllowedRunners(m_runners);
        m_models.append(model);
    } else {
        for (const QString &runnerId : std::as_const(m_runners)) {
            m_models.append(new RunnerMatchesModel(runnerId, std::nullopt, this));
        }
    }
    endResetModel();
}
