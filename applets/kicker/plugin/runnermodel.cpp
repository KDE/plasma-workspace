/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "runnermodel.h"
#include "runnermatchesmodel.h"

#include <QSet>

#include <KConfigGroup>
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
    , m_krunnerConfig(KSharedConfig::openConfig(QStringLiteral("krunnerrc")))
{
    m_queryTimer.setSingleShot(true);
    m_queryTimer.setInterval(10ms);
    connect(&m_queryTimer, &QTimer::timeout, this, &RunnerModel::startQuery);
    const auto readFavorites = [this]() {
        m_favoritePluginIds = m_krunnerConfig
                                  ->group(QStringLiteral("Plugins")) //
                                  .group(QStringLiteral("Favorites"))
                                  .readEntry("plugins", QStringList(QStringLiteral("krunner_services")));
        if (m_mergeResults && !m_models.isEmpty()) {
            m_models.constFirst()->setFavoriteIds(m_favoritePluginIds);
        }
    };
    m_configWatcher = KConfigWatcher::create(m_krunnerConfig);
    connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, readFavorites);
    readFavorites();
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

RunnerMatchesModel *RunnerModel::modelForRow(int row)
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
            m_models.constFirst()->runnerManager()->setAllowedRunners(runners);
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
    if (m_query == query) {
        return; // do not init models if the query doesn't change. particularly important during startup!
    }
    if (m_models.isEmpty()) {
        initializeModels();
    }
    m_query = query;
    m_queryTimer.start();
    Q_EMIT queryChanged();
}

void RunnerModel::startQuery()
{
    if (m_query.isEmpty()) {
        clear();
        QTimer::singleShot(0, this, &RunnerModel::queryFinished);
    } else {
        m_queryingModels = m_models.size();
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
        model->setFavoriteIds(m_favoritePluginIds);
        m_models.append(model);
    } else {
        for (const QString &runnerId : std::as_const(m_runners)) {
            m_models.append(new RunnerMatchesModel(runnerId, std::nullopt, this));
        }
    }
    for (auto model : std::as_const(m_models)) {
        connect(model->runnerManager(), &KRunner::RunnerManager::queryFinished, this, [this]() {
            if (--m_queryingModels == 0) {
                Q_EMIT queryFinished();
            }
        });
    }
    endResetModel();
    Q_EMIT countChanged();
}
