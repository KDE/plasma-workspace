/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "runnermatchesmodel.h"
#include "actionlist.h"
#include "runnermodel.h"

#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QPluginLoader>
#include <QUrlQuery>

#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KRunner/RunnerManager>

#include <KConfigGroup>
#include <KSharedConfig>
#include <Plasma/Plasma>

RunnerMatchesModel::RunnerMatchesModel(const QString &runnerId, const std::optional<QString> &name, QObject *parent)
    : KRunner::ResultsModel(KSharedConfig::openConfig(QStringLiteral("krunnerrc"))->group(QStringLiteral("Plugins")),
                            KSharedConfig::openStateConfig()->group(QStringLiteral("KickerRunnerManager")),
                            parent)
    , m_runnerId(runnerId)
{
    runnerManager()->setHistoryEnvironmentIdentifier(m_consumer.currentActivity());
    connect(&m_consumer, &KActivities::Consumer::currentActivityChanged, runnerManager(), &KRunner::RunnerManager::setHistoryEnvironmentIdentifier);
    connect(this, &RunnerMatchesModel::rowsInserted, this, &RunnerMatchesModel::countChanged);
    connect(this, &RunnerMatchesModel::rowsRemoved, this, &RunnerMatchesModel::countChanged);
    connect(this, &RunnerMatchesModel::modelReset, this, &RunnerMatchesModel::countChanged);

    if (name.has_value()) {
        m_name = name.value();
    } else {
        Q_ASSERT(!runnerId.isEmpty());
        runnerManager()->setAllowedRunners({runnerId});

        const static auto availableRunners = KRunner::RunnerManager::runnerMetaDataList();
        for (const KPluginMetaData &runner : availableRunners) {
            if (runner.pluginId() == runnerId) {
                auto instance = runnerManager()->loadRunner(runner);
                m_name = instance ? instance->name() : QString();
            }
        }
    }
    connect(runnerManager(), &KRunner::RunnerManager::requestUpdateQueryString, this, &RunnerMatchesModel::requestUpdateQueryString);
}

AbstractModel *RunnerMatchesModel::favoritesModel() const
{
    return m_favoritesModel;
}

void RunnerMatchesModel::setFavoritesModel(AbstractModel *model)
{
    if (m_favoritesModel != model) {
        m_favoritesModel = model;
        Q_EMIT favoritesModelChanged();
    }
}

QVariant RunnerMatchesModel::data(const QModelIndex &index, int role) const
{
    KRunner::QueryMatch match = getQueryMatch(index);
    if (!match.isValid()) {
        return QVariant();
    }

    // Since we have different enums than the KRunner model, we have to implement reading all the data manually
    if (role == Qt::DisplayRole) {
        return match.text();
    } else if (role == Qt::DecorationRole) {
        if (!match.iconName().isEmpty()) {
            return match.iconName();
        }

        return match.icon();
    } else if (role == Kicker::GroupRole) {
        return KRunner::ResultsModel::data(index, CategoryRole);
    } else if (role == Kicker::DescriptionRole) {
        return match.subtext();
    } else if (role == Kicker::FavoriteIdRole) {
        if (match.runner()->id() == QLatin1String("krunner_services")) {
            return match.data().toString();
        }
    } else if (role == Kicker::UrlRole) {
        const QList<QUrl> urls = match.urls();
        if (urls.isEmpty()) {
            return QUrl();
        }
        QUrl url = urls.first();

        if (!url.isLocalFile()) {
            return url;
        }

        QString path = url.path();
        QFileInfo info(path);

        if (!info.exists()) {
            return {};
        }

        if (info.isSymLink()) {
            path = info.symLinkTarget();

            // If the target is relative, make it absolute relative to the link's directory
            if (QFileInfo(path).isRelative()) {
                path = QDir(info.absolutePath()).absoluteFilePath(path);
            }
        }
        return QUrl::fromLocalFile(path);
    } else if (role == Kicker::HasActionListRole) {
        return match.runner()->id() == QLatin1String("krunner_services") || !match.runner()->findChildren<QAction *>().isEmpty();
    } else if (role == Kicker::IsMultilineTextRole) {
        return match.isMultiLine();
    } else if (role == Kicker::ActionListRole) {
        QVariantList actionList;
        const auto actions = match.actions();
        for (auto action : actions) {
            QVariantMap item = Kicker::createActionItem(action.text(), //
                                                        action.iconSource(),
                                                        QStringLiteral("runnerAction"),
                                                        QVariant::fromValue(action));

            actionList << item;
        }

        // Only try to get a KService for matches from the services and systemsettings runner. Assuming
        // that any other runner returns something we want to turn into a KService is
        // unsafe, e.g. files from the Baloo runner might match a storageId just by
        // accident, creating a dangerous false positive.
        if (match.runner()->id() != QLatin1String("krunner_services") && match.runner()->id() != QLatin1String("krunner_systemsettings")) {
            return actionList;
        }

        QUrl dataUrl(match.data().toUrl());
        if (dataUrl.isEmpty() && !match.urls().isEmpty()) {
            // needed for systemsettigs runner
            dataUrl = match.urls().constFirst();
        }
        if (dataUrl.scheme() != QLatin1String("applications")) {
            return actionList;
        }

        // Don't offer jump list actions on a jump list action.
        const QString actionName = QUrlQuery(dataUrl).queryItemValue(QStringLiteral("action"));
        if (!actionName.isEmpty()) {
            return actionList;
        }

        const KService::Ptr service = KService::serviceByStorageId(dataUrl.path());
        if (service) {
            if (!actionList.isEmpty()) {
                actionList << Kicker::createSeparatorActionItem();
            }

            const QVariantList &jumpListActions = Kicker::jumpListActions(service);
            if (!jumpListActions.isEmpty()) {
                actionList << jumpListActions << Kicker::createSeparatorActionItem();
            }

            QObject *appletInterface = static_cast<RunnerModel *>(parent())->appletInterface();

            bool systemImmutable = false;
            if (appletInterface) {
                systemImmutable = (appletInterface->property("immutability").toInt() == Plasma::Types::SystemImmutable);
            }

            const QVariantList &addLauncherActions = Kicker::createAddLauncherActionList(appletInterface, service);
            bool needsSeparator = false;
            if (!systemImmutable && !addLauncherActions.isEmpty()) {
                actionList << addLauncherActions;
                needsSeparator = true;
            }

            const QVariantList &recentDocuments = Kicker::recentDocumentActions(service);
            if (!recentDocuments.isEmpty()) {
                actionList << recentDocuments;
                needsSeparator = false;
            }

            if (needsSeparator) {
                actionList << Kicker::createSeparatorActionItem();
            }

            const QVariantList &additionalActions = Kicker::additionalAppActions(service);
            if (!additionalActions.isEmpty()) {
                actionList << additionalActions << Kicker::createSeparatorActionItem();
            }

            // Don't allow adding launchers, editing, hiding, or uninstalling applications
            // when system is immutable.
            if (systemImmutable) {
                return actionList;
            }

            if (service->isApplication()) {
                actionList << Kicker::editApplicationAction(service);
                actionList << Kicker::appstreamActions(service);
            }
        }

        return actionList;
    }

    return QVariant();
}

bool RunnerMatchesModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    const KRunner::QueryMatch match = getQueryMatch(index(row, 0));
    if (!match.isValid()) {
        return false;
    }
    if (!match.isEnabled()) {
        return false;
    }

    // BUG 442970: Skip creating KService if there is no actionId, or the action is from a runner.
    if (actionId.isEmpty() || actionId == QLatin1String("runnerAction")) {
        if (!actionId.isEmpty()) {
            if (auto action = argument.value<KRunner::Action>()) {
                return runnerManager()->run(match, action);
            }
            return false;
        }

        return runnerManager()->run(match);
    }

    QObject *appletInterface = static_cast<RunnerModel *>(parent())->appletInterface();

    KService::Ptr service = KService::serviceByStorageId(match.data().toUrl().toString(QUrl::RemoveScheme));
    if (!service && !match.urls().isEmpty()) {
        // needed for systemsettigs runner
        service = KService::serviceByStorageId(match.urls().constFirst().toString(QUrl::RemoveScheme));
    }

    if (Kicker::handleAddLauncherAction(actionId, appletInterface, service)) {
        return false; // We don't want to close Kicker, BUG: 390585
    } else if (Kicker::handleEditApplicationAction(actionId, service)) {
        return true;
    } else if (Kicker::handleAppstreamActions(actionId, service)) {
        return true;
    } else if (actionId == QLatin1String("_kicker_jumpListAction")) {
        auto job = new KIO::ApplicationLauncherJob(argument.value<KServiceAction>());
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        return job->exec();
    } else if (actionId == QLatin1String("_kicker_recentDocument") || actionId == QLatin1String("_kicker_forgetRecentDocuments")) {
        return Kicker::handleRecentDocumentAction(service, actionId, argument);
    } else if (Kicker::handleAdditionalAppActions(actionId, service, argument)) {
        return true;
    }

    return false;
}

#include "moc_runnermatchesmodel.cpp"
