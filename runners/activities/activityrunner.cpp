/*
 *   Copyright (C) 2011 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "activityrunner.h"

#include <QDebug>
#include <QIcon>
#include <klocalizedstring.h>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(ActivityRunner, "plasma-runner-activityrunner.json")

ActivityRunner::ActivityRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
    , m_activities(new KActivities::Controller(this))
    , m_consumer(new KActivities::Consumer(this))
    , m_keywordi18n(i18nc("KRunner keyword", "activity"))
    , m_keyword(QStringLiteral("activity"))
{
    setObjectName(QStringLiteral("Activities"));
    addSyntax(Plasma::RunnerSyntax(m_keywordi18n, i18n("Lists all activities currently available to be run.")));
    addSyntax(Plasma::RunnerSyntax(i18nc("KRunner keyword", "activity :q:"), i18n("Switches to activity :q:.")));

    qRegisterMetaType<KActivities::Consumer::ServiceStatus>();
    connect(m_consumer, &KActivities::Consumer::serviceStatusChanged, this, &ActivityRunner::serviceStatusChanged);
    serviceStatusChanged(m_activities->serviceStatus());
    setTriggerWords({m_keyword, m_keywordi18n});
}

void ActivityRunner::serviceStatusChanged(KActivities::Consumer::ServiceStatus status)
{
    suspendMatching(status == KActivities::Consumer::NotRunning);
}

ActivityRunner::~ActivityRunner()
{
}

void ActivityRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query().trimmed();
    bool list = false;
    QString name;

    if (term.startsWith(m_keywordi18n, Qt::CaseInsensitive)) {
        if (term.size() == m_keywordi18n.size()) {
            list = true;
        } else {
            name = term.right(term.size() - m_keywordi18n.size()).trimmed();
            list = name.isEmpty();
        }
    } else if (term.startsWith(m_keyword, Qt::CaseInsensitive)) {
        if (term.size() == m_keyword.size()) {
            list = true;
        } else {
            name = term.right(term.size() - m_keyword.size()).trimmed();
            list = name.isEmpty();
        }
    } else if (context.singleRunnerQueryMode()) {
        name = term;
    } else {
        return;
    }

    QList<Plasma::QueryMatch> matches;
    QStringList activities = m_consumer->activities();
    std::sort(activities.begin(), activities.end());

    const QString current = m_activities->currentActivity();

    if (!context.isValid()) {
        return;
    }

    if (list) {
        for (const QString &activity : qAsConst(activities)) {
            if (current == activity) {
                continue;
            }

            KActivities::Info info(activity);
            addMatch(info, matches);

            if (!context.isValid()) {
                return;
            }
        }
    } else {
        for (const QString &activity : qAsConst(activities)) {
            if (current == activity) {
                continue;
            }

            KActivities::Info info(activity);
            if (info.name().startsWith(name, Qt::CaseInsensitive)) {
                addMatch(info, matches);
            }

            if (!context.isValid()) {
                return;
            }
        }
    }

    context.addMatches(matches);
}

void ActivityRunner::addMatch(const KActivities::Info &activity, QList<Plasma::QueryMatch> &matches)
{
    Plasma::QueryMatch match(this);
    match.setData(activity.id());
    match.setType(Plasma::QueryMatch::ExactMatch);
    match.setIconName(activity.icon().isEmpty() ? QStringLiteral("activities") : activity.icon());
    match.setText(i18n("Switch to \"%1\"", activity.name()));
    match.setRelevance(0.7 + ((activity.state() == KActivities::Info::Running || activity.state() == KActivities::Info::Starting) ? 0.1 : 0));
    matches << match;
}

void ActivityRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    m_activities->setCurrentActivity(match.data().toString());
}

#include "activityrunner.moc"
