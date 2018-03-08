/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2014 Vishesh Handa <vhanda@kde.org>
 *   Copyright (C) 2016 Harald Sitter <sitter@kde.org>
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

#include "servicerunner.h"

#include <QMimeData>

#include <QIcon>
#include <QDebug>
#include <QUrl>

#include <KActivities/ResourceInstance>
#include <KLocalizedString>
#include <KRun>
#include <KService>
#include <KServiceTypeTrader>
#include <KStringHandler>

#include "debug.h"

namespace {

int weightedLength(const QString &query) {
    return KStringHandler::logicalLength(query);
}

}  // namespace

/**
 * @brief Finds all KServices for a given runner query
 */
class ServiceFinder
{
public:
    ServiceFinder(ServiceRunner *runner)
         : m_runner(runner)
    {}


    void match(Plasma::RunnerContext &context)
    {
        if (!context.isValid()) {
            return;
        }

        term = context.query();
        weightedTermLength = weightedLength(term);

        matchExectuables();
        matchNameKeywordAndGenericName();
        matchCategories();
        matchJumpListActions();

        context.addMatches(matches);
    }

private:

    void seen(const KService::Ptr &service)
    {
        m_seen.insert(service->storageId());
        m_seen.insert(service->exec());
    }

    void seen(const KServiceAction &action)
    {
        m_seen.insert(action.exec());
    }

    bool hasSeen(const KService::Ptr &service)
    {
        return m_seen.contains(service->storageId()) &&
               m_seen.contains(service->exec());
    }

    bool hasSeen(const KServiceAction &action)
    {
        return m_seen.contains(action.exec());
    }

    bool disqualify(const KService::Ptr &service)
    {
        auto ret = hasSeen(service) || service->noDisplay();
        qCDebug(RUNNER_SERVICES) << service->name() << "disqualified?" << ret;
        seen(service);
        return ret;
    }

    qreal increaseMatchRelavance(const KService::Ptr &service, QVector<QStringRef> &strList, QString category)
    {
        //Increment the relevance based on all the words (other than the first) of the query list
        qreal relevanceIncrement = 0;

        for(int i=1; i<strList.size(); i++)
        {
            if (category == QLatin1String("Name")) {
                if (service->name().contains(strList[i], Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == QLatin1String("GenericName")) {
                if (service->genericName().contains(strList[i], Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == QLatin1String("Exec")) {
                if (service->exec().contains(strList[i], Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == QLatin1String("Comment")) {
                if (service->comment().contains(strList[i], Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            }
        }

        return relevanceIncrement;
    }

    QString generateQuery(QVector<QStringRef> &strList)
    {
        QString keywordTemplate = QStringLiteral("exist Keywords");
        QString genericNameTemplate = QStringLiteral("exist GenericName");
        QString nameTemplate = QStringLiteral("exist Name");
        QString commentTemplate = QStringLiteral("exist Comment");

        // Search for applications which are executable and the term case-insensitive matches any of
        // * a substring of one of the keywords
        // * a substring of the GenericName field
        // * a substring of the Name field
        // Note that before asking for the content of e.g. Keywords and GenericName we need to ask if
        // they exist to prevent a tree evaluation error if they are not defined.
        foreach (QStringRef str, strList)
        {
            keywordTemplate += QStringLiteral(" and '%1' ~subin Keywords").arg(str.toString());
            genericNameTemplate += QStringLiteral(" and '%1' ~~ GenericName").arg(str.toString());
            nameTemplate += QStringLiteral(" and '%1' ~~ Name").arg(str.toString());
            commentTemplate += QStringLiteral(" and '%1' ~~ Comment").arg(str.toString());
        }

        QString finalQuery = QStringLiteral("exist Exec and ( (%1) or (%2) or (%3) or ('%4' ~~ Exec) or (%5) )")
            .arg(keywordTemplate, genericNameTemplate, nameTemplate, strList[0].toString(), commentTemplate);

        qCDebug(RUNNER_SERVICES) << "Final query : " << finalQuery;
        return finalQuery;
    }

    void setupMatch(const KService::Ptr &service, Plasma::QueryMatch &match)
    {
        const QString name = service->name();

        match.setText(name);
        match.setData(service->storageId());

        if (!service->genericName().isEmpty() && service->genericName() != name) {
            match.setSubtext(service->genericName());
        } else if (!service->comment().isEmpty()) {
            match.setSubtext(service->comment());
        }

        if (!service->icon().isEmpty()) {
            match.setIconName(service->icon());
        }
    }

    void matchExectuables()
    {
        if (weightedTermLength < 2) {
            return;
        }

        // Search for applications which are executable and case-insensitively match the search term
        // See http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
        // if the following is unclear to you.
        query = QStringLiteral("exist Exec and ('%1' =~ Name)").arg(term);
        KService::List services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), query);

        if (services.isEmpty()) {
            return;
        }

        foreach (const KService::Ptr &service, services) {
            qCDebug(RUNNER_SERVICES) << service->name() << "is an exact match!" << service->storageId() << service->exec();
            if (disqualify(service)) {
                continue;
            }
            Plasma::QueryMatch match(m_runner);
            match.setType(Plasma::QueryMatch::ExactMatch);
            setupMatch(service, match);
            match.setRelevance(1);
            matches << match;
        }
    }

    void matchNameKeywordAndGenericName()
    {
        //Splitting the query term to match using subsequences
        QVector<QStringRef> queryList = term.splitRef(QLatin1Char(' '));

        // If the term length is < 3, no real point searching the Keywords and GenericName
        if (weightedTermLength < 3) {
            query = QStringLiteral("exist Exec and ( (exist Name and '%1' ~~ Name) or ('%1' ~~ Exec) )").arg(term);
        } else {
            //Match using subsequences (Bug: 262837)
            query = generateQuery(queryList);
        }

        KService::List services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), query);
        services += KServiceTypeTrader::self()->query(QStringLiteral("KCModule"), query);

        qCDebug(RUNNER_SERVICES) << "got " << services.count() << " services from " << query;
        foreach (const KService::Ptr &service, services) {
            if (disqualify(service)) {
                continue;
            }

            const QString id = service->storageId();
            const QString name = service->desktopEntryName();
            const QString exec = service->exec();

            Plasma::QueryMatch match(m_runner);
            match.setType(Plasma::QueryMatch::PossibleMatch);
            setupMatch(service, match);
            qreal relevance(0.6);

            // If the term was < 3 chars and NOT at the beginning of the App's name or Exec, then
            // chances are the user doesn't want that app.
            if (weightedTermLength < 3) {
                if (name.startsWith(term) || exec.startsWith(term)) {
                    relevance = 0.9;
                } else {
                    continue;
                }
            } else if (service->name().contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.8;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("Name"));

                if (service->name().startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.1;
                }
            } else if (service->genericName().contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.65;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("GenericName"));

                if (service->genericName().startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.05;
                }
            } else if (service->exec().contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.7;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("Exec"));

                if (service->exec().startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.05;
                }
            } else if (service->comment().contains(queryList[0], Qt::CaseInsensitive)) {
                relevance = 0.5;
                relevance += increaseMatchRelavance(service, queryList, QStringLiteral("Comment"));

                if (service->comment().startsWith(queryList[0], Qt::CaseInsensitive)) {
                    relevance += 0.05;
                }
            }

            if (service->categories().contains(QStringLiteral("KDE")) || service->serviceTypes().contains(QStringLiteral("KCModule"))) {
                qCDebug(RUNNER_SERVICES) << "found a kde thing" << id << match.subtext() << relevance;
                if (id.startsWith(QLatin1String("kde-"))) {
                    qCDebug(RUNNER_SERVICES) << "old" << !service->isApplication();
                    if (!service->isApplication()) {
                        // avoid showing old kcms and what not
                        continue;
                    }

                    // This is an older version, let's disambiguate it
                    QString subtext(QStringLiteral("KDE3"));

                    if (!match.subtext().isEmpty()) {
                        subtext.append(QStringLiteral(", ") + match.subtext());
                    }

                    match.setSubtext(subtext);
                } else {
                    relevance += .09;
                }
            }

            qCDebug(RUNNER_SERVICES) << service->name() << "is this relevant:" << relevance;
            match.setRelevance(relevance);
            if (service->serviceTypes().contains(QStringLiteral("KCModule"))) {
                match.setMatchCategory(i18n("System Settings"));
            }
            matches << match;
        }
    }

    void matchCategories()
    {
        //search for applications whose categories contains the query
        query = QStringLiteral("exist Exec and (exist Categories and '%1' ~subin Categories)").arg(term);
        auto services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), query);

        foreach (const KService::Ptr &service, services) {
            qCDebug(RUNNER_SERVICES) << service->name() << "is an exact match!" << service->storageId() << service->exec();
            if (disqualify(service)) {
                continue;
            }

            Plasma::QueryMatch match(m_runner);
            match.setType(Plasma::QueryMatch::PossibleMatch);
            setupMatch(service, match);

            qreal relevance = 0.6;
            if (service->categories().contains(QStringLiteral("X-KDE-More")) ||
                    !service->showInCurrentDesktop()) {
                relevance = 0.5;
            }

            if (service->isApplication()) {
                relevance += .04;
            }

            match.setRelevance(relevance);
            matches << match;
        }
    }

    void matchJumpListActions()
    {
        if (weightedTermLength < 3) {
            return;
        }

        query = QStringLiteral("exist Actions"); // doesn't work
        auto services = KServiceTypeTrader::self()->query(QStringLiteral("Application"));//, query);

        foreach (const KService::Ptr &service, services) {
            if (service->noDisplay()) {
                continue;
            }

            foreach (const KServiceAction &action, service->actions()) {
                if (action.text().isEmpty() || action.exec().isEmpty() || hasSeen(action)) {
                    continue;
                }
                seen(action);


                if (!action.text().contains(term, Qt::CaseInsensitive)) {
                    continue;
                }

                Plasma::QueryMatch match(m_runner);
                match.setType(Plasma::QueryMatch::HelperMatch);
                if (!action.icon().isEmpty()) {
                    match.setIconName(action.icon());
                } else {
                    match.setIconName(service->icon());
                }
                match.setText(i18nc("Jump list search result, %1 is action (eg. open new tab), %2 is application (eg. browser)",
                                    "%1 - %2", action.text(), service->name()));
                match.setData(action.exec());

                qreal relevance = 0.5;
                if (action.text().startsWith(term, Qt::CaseInsensitive)) {
                    relevance += 0.05;
                }

                match.setRelevance(relevance);

                matches << match;
            }
        }
    }

    ServiceRunner *m_runner;
    QSet<QString> m_seen;

    QList<Plasma::QueryMatch> matches;
    QString query;
    QString term;
    int weightedTermLength;
};

ServiceRunner::ServiceRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args)

    setObjectName( QStringLiteral("Application" ));
    setPriority(AbstractRunner::HighestPriority);

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds applications whose name or description match :q:")));
}

ServiceRunner::~ServiceRunner()
{
}

QStringList ServiceRunner::categories() const
{
    QStringList cat;
    cat << i18n("Applications") << i18n("System Settings");

    return cat;
}

QIcon ServiceRunner::categoryIcon(const QString& category) const
{
    if (category == i18n("Applications")) {
        return QIcon::fromTheme(QStringLiteral("applications-other"));
    } else if (category == i18n("System Settings")) {
        return QIcon::fromTheme(QStringLiteral("preferences-system"));
    }

    return Plasma::AbstractRunner::categoryIcon(category);
}


void ServiceRunner::match(Plasma::RunnerContext &context)
{
    // This helper class aids in keeping state across numerous
    // different queries that together form the matches set.
    ServiceFinder finder(this);
    finder.match(context);
}

void ServiceRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context);
    if (match.type() == Plasma::QueryMatch::HelperMatch) { // Jump List Action
         KRun::run(match.data().toString(), {}, nullptr);
         return;
    }

    KService::Ptr service = KService::serviceByStorageId(match.data().toString());
    if (service) {
        KActivities::ResourceInstance::notifyAccessed(
            QUrl(QStringLiteral("applications:") + service->storageId()),
            QStringLiteral("org.kde.krunner")
        );

        KRun::runService(*service, {}, nullptr, true);
    }
}

QMimeData * ServiceRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    KService::Ptr service = KService::serviceByStorageId(match.data().toString());
    if (service) {
        QMimeData * result = new QMimeData();
        QList<QUrl> urls;
        urls << QUrl::fromLocalFile(service->entryPath());
        qCDebug(RUNNER_SERVICES) << urls;
        result->setUrls(urls);
        return result;
    }

    return nullptr;
}

K_EXPORT_PLASMA_RUNNER(services, ServiceRunner)

#include "servicerunner.moc"
