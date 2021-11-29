/*
    SPDX-FileCopyrightText: 2012 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "recentcontactsmodel.h"
#include "actionlist.h"
#include "contactentry.h"

#include <QAction>

#include <KLocalizedString>

#include <KActivities/Stats/ResultModel>
#include <KActivities/Stats/Terms>

#include <KPeople/PersonData>
#include <kpeople/widgets/actions.h> //FIXME TODO: Pretty include in KPeople broken.
#include <kpeople/widgets/persondetailsdialog.h>

namespace KAStats = KActivities::Stats;

using namespace KAStats;
using namespace KAStats::Terms;

RecentContactsModel::RecentContactsModel(QObject *parent)
    : ForwardingModel(parent)
{
    refresh();
}

RecentContactsModel::~RecentContactsModel()
{
}

QString RecentContactsModel::description() const
{
    return i18n("Contacts");
}

QVariant RecentContactsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    QString id = sourceModel()->data(index, ResultModel::ResourceRole).toString();

    KPeople::PersonData *data = nullptr;

    if (m_idToData.contains(id)) {
        data = m_idToData[id];
    }

    if (!data) {
        const_cast<RecentContactsModel *>(this)->insertPersonData(id, index.row());
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return data->name();
    } else if (role == Qt::DecorationRole) {
        return data->presenceIconName();
    } else if (role == Kicker::FavoriteIdRole) {
        return id;
    } else if (role == Kicker::HasActionListRole) {
        return true;
    } else if (role == Kicker::ActionListRole) {
        QVariantList actionList;

        const QVariantMap &forgetAction = Kicker::createActionItem(i18n("Forget Contact"), QStringLiteral("edit-clear-history"), QStringLiteral("forget"));
        actionList << forgetAction;

        const QVariantMap &forgetAllAction =
            Kicker::createActionItem(i18n("Forget All Contacts"), QStringLiteral("edit-clear-history"), QStringLiteral("forgetAll"));
        actionList << forgetAllAction;

        actionList << Kicker::createSeparatorActionItem();

        actionList << Kicker::createActionItem(i18n("Show Contact Information…"), QStringLiteral("identity"), QStringLiteral("showContactInfo"));

        return actionList;
    } else if (role == Kicker::DescriptionRole) {
        return QString();
    }

    return QVariant();
}

bool RecentContactsModel::trigger(int row, const QString &actionId, const QVariant &argument)
{
    Q_UNUSED(argument)

    bool withinBounds = row >= 0 && row < rowCount();

    if (actionId.isEmpty() && withinBounds) {
        QString id = sourceModel()->data(sourceModel()->index(row, 0), ResultModel::ResourceRole).toString();

        const QList<QAction *> actionList = KPeople::actionsForPerson(id, this);

        if (!actionList.isEmpty()) {
            QAction *chat = nullptr;

            for (QAction *action : actionList) {
                const QVariant &actionType = action->property("actionType");

                if (!actionType.isNull() && actionType.toInt() == KPeople::ActionType::TextChatAction) {
                    chat = action;
                }
            }

            if (chat) {
                chat->trigger();

                return true;
            }
        }

        return false;
    } else if (actionId == QLatin1String("showContactInfo") && withinBounds) {
        ContactEntry::showPersonDetailsDialog(sourceModel()->data(sourceModel()->index(row, 0), ResultModel::ResourceRole).toString());
    } else if (actionId == QLatin1String("forget") && withinBounds) {
        if (sourceModel()) {
            ResultModel *resultModel = static_cast<ResultModel *>(sourceModel());
            resultModel->forgetResource(row);
        }

        return false;
    } else if (actionId == QLatin1String("forgetAll")) {
        if (sourceModel()) {
            ResultModel *resultModel = static_cast<ResultModel *>(sourceModel());
            resultModel->forgetAllResources();
        }

        return false;
    }

    return false;
}

bool RecentContactsModel::hasActions() const
{
    return rowCount();
}

QVariantList RecentContactsModel::actions() const
{
    QVariantList actionList;

    if (rowCount()) {
        actionList << Kicker::createActionItem(i18n("Forget All Contacts"), QStringLiteral("edit-clear-history"), QStringLiteral("forgetAll"));
    }

    return actionList;
}

void RecentContactsModel::refresh()
{
    QObject *oldModel = sourceModel();

    // clang-format off
    auto query = UsedResources
                    | RecentlyUsedFirst
                    | Agent(QStringLiteral("KTp"))
                    | Type::any()
                    | Activity::current()
                    | Url::startsWith(QStringLiteral("ktp"))
                    | Limit(15);
    // clang-format on

    ResultModel *model = new ResultModel(query);

    QModelIndex index;

    if (model->canFetchMore(index)) {
        model->fetchMore(index);
    }

    // FIXME TODO: Don't wipe entire cache on transactions.
    connect(model, &QAbstractItemModel::rowsInserted, this, &RecentContactsModel::buildCache, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &RecentContactsModel::buildCache, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::rowsMoved, this, &RecentContactsModel::buildCache, Qt::UniqueConnection);
    connect(model, &QAbstractItemModel::modelReset, this, &RecentContactsModel::buildCache, Qt::UniqueConnection);

    setSourceModel(model);

    buildCache();

    delete oldModel;
}

void RecentContactsModel::buildCache()
{
    qDeleteAll(m_idToData);
    m_idToData.clear();
    m_dataToRow.clear();

    QString id;

    for (int i = 0; i < sourceModel()->rowCount(); ++i) {
        id = sourceModel()->data(sourceModel()->index(i, 0), ResultModel::ResourceRole).toString();

        if (!m_idToData.contains(id)) {
            insertPersonData(id, i);
        }
    }
}

void RecentContactsModel::insertPersonData(const QString &id, int row)
{
    KPeople::PersonData *data = new KPeople::PersonData(id);

    m_idToData[id] = data;
    m_dataToRow[data] = row;

    connect(data, &KPeople::PersonData::dataChanged, this, &RecentContactsModel::personDataChanged);
}

void RecentContactsModel::personDataChanged()
{
    KPeople::PersonData *data = static_cast<KPeople::PersonData *>(sender());

    if (m_dataToRow.contains(data)) {
        int row = m_dataToRow[data];

        QModelIndex idx = sourceModel()->index(row, 0);

        Q_EMIT dataChanged(idx, idx);
    }
}
