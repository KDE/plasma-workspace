/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "runnermatchesmodel.h"
#include "actionlist.h"
#include "runnermodel.h"

#include <QAction>
#include <QIcon>
#include <QUrlQuery>

#include <KIO/ApplicationLauncherJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KRunner/RunnerManager>

#include <Plasma/Plasma>

RunnerMatchesModel::RunnerMatchesModel(const QString &runnerId, const QString &name, Plasma::RunnerManager *manager, QObject *parent)
    : AbstractModel(parent)
    , m_runnerId(runnerId)
    , m_name(name)
    , m_runnerManager(manager)
{
    connect(m_runnerManager, &Plasma::RunnerManager::setSearchTerm, this, &RunnerMatchesModel::requestUpdateQueryString);
}

QString RunnerMatchesModel::description() const
{
    return m_name;
}

QVariant RunnerMatchesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_matches.count()) {
        return QVariant();
    }

    Plasma::QueryMatch match = m_matches.at(index.row());

    if (role == Qt::DisplayRole) {
        return match.text();
    } else if (role == Qt::DecorationRole) {
        if (!match.iconName().isEmpty()) {
            return match.iconName();
        }

        return match.icon();
    } else if (role == Kicker::DescriptionRole) {
        return match.subtext();
    } else if (role == Kicker::FavoriteIdRole) {
        if (match.runner()->id() == QLatin1String("services")) {
            return match.data().toString();
        }
    } else if (role == Kicker::UrlRole) {
        const QList<QUrl> urls = match.urls();

        if (!urls.isEmpty()) {
            return urls.first();
        }
    } else if (role == Kicker::HasActionListRole) {
        return match.runner()->id() == QLatin1String("services") || !match.runner()->findChildren<QAction *>().isEmpty();
    } else if (role == Kicker::IsMultilineTextRole) {
        return match.isMultiLine();
    } else if (role == Kicker::ActionListRole) {
        QVariantList actionList;
        const QList<QAction *> actions = m_runnerManager->actionsForMatch(match);
        for (QAction *action : actions) {
            QVariantMap item = Kicker::createActionItem(action->text(), //
                                                        action->icon().name(),
                                                        QStringLiteral("runnerAction"),
                                                        QVariant::fromValue<QObject *>(action));

            actionList << item;
        }

        // Only try to get a KService for matches from the services and systemsettings runner. Assuming
        // that any other runner returns something we want to turn into a KService is
        // unsafe, e.g. files from the Baloo runner might match a storageId just by
        // accident, creating a dangerous false positive.
        if (match.runner()->id() != QLatin1String("services") && match.runner()->id() != QLatin1String("krunner_systemsettings")) {
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

int RunnerMatchesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_matches.count();
}

bool RunnerMatchesModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    if (row < 0 || row >= m_matches.count()) {
        return false;
    }

    Plasma::QueryMatch match = m_matches.at(row);

    if (!match.isEnabled()) {
        return false;
    }

    // BUG 442970: Skip creating KService if there is no actionId, or the action is from a runner.
    if (actionId.isEmpty() || actionId == QLatin1String("runnerAction")) {
        if (!actionId.isEmpty()) {
            QObject *obj = argument.value<QObject *>();

            if (!obj) {
                return false;
            }

            QAction *action = qobject_cast<QAction *>(obj);

            if (!action) {
                return false;
            }
            match.setSelectedAction(action);
        }

        return m_runnerManager->runMatch(match);
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

void RunnerMatchesModel::setMatches(const QList<Plasma::QueryMatch> &matches)
{
    int oldCount = m_matches.count();
    int newCount = matches.count();

    bool emitCountChange = (oldCount != newCount);

    int ceiling = qMin(oldCount, newCount);
    bool emitDataChange = false;

    for (int row = 0; row < ceiling; ++row) {
        if (!(m_matches.at(row) == matches.at(row))) {
            emitDataChange = true;
            m_matches[row] = matches.at(row);
        }
    }

    if (emitDataChange) {
        Q_EMIT dataChanged(index(0, 0), index(ceiling - 1, 0));
    }

    if (newCount > oldCount) {
        beginInsertRows(QModelIndex(), oldCount, newCount - 1);

        m_matches = matches;

        endInsertRows();
    } else if (newCount < oldCount) {
        beginRemoveRows(QModelIndex(), newCount, oldCount - 1);

        m_matches = matches;

        endRemoveRows();
    }

    if (emitCountChange) {
        Q_EMIT countChanged();
    }
}

AbstractModel *RunnerMatchesModel::favoritesModel()
{
    return static_cast<RunnerModel *>(parent())->favoritesModel();
}

Q_DECLARE_METATYPE(KServiceAction)
