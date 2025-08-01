/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2014 Vishesh Handa <vhanda@kde.org>
    SPDX-FileCopyrightText: 2016-2020 Harald Sitter <sitter@kde.org>
    SPDX-FileCopyrightText: 2022-2023 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "servicerunner.h"

#include <algorithm>

#include <QMimeData>

#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

#include <KApplicationTrader>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KServiceAction>
#include <KShell>
#include <KStringHandler>
#include <KSycoca>
#include <PlasmaActivities/ResourceInstance>

#include <KIO/ApplicationLauncherJob>
#include <KIO/DesktopExecParser>

#include "debug.h"

using namespace Qt::StringLiterals;

namespace
{

int weightedLength(const QString &query)
{
    return KStringHandler::logicalLength(query);
}

inline bool contains(const QString &result, const QList<QStringView> &queryList)
{
    return std::ranges::all_of(queryList, [&result](QStringView query) {
        return result.contains(query, Qt::CaseInsensitive);
    });
}

inline bool contains(const QStringList &results, const QList<QStringView> &queryList)
{
    return std::ranges::all_of(queryList, [&results](QStringView query) {
        return std::ranges::any_of(results, [&query](QStringView result) {
            return result.contains(query, Qt::CaseInsensitive);
        });
    });
}

} // namespace

/**
 * @brief Finds all KServices for a given runner query
 */
class ServiceFinder
{
public:
    ServiceFinder(ServiceRunner *runner, const QList<KService::Ptr> &list)
        : m_runner(runner)
        , m_services(list)
    {
    }

    void match(KRunner::RunnerContext &context)
    {
        query = context.query();
        // Splitting the query term to match using subsequences
        queryList = QStringView(query).split(QLatin1Char(' '));
        weightedTermLength = weightedLength(query);

        matchNameKeywordAndGenericName();
        matchCategories();
        matchJumpListActions();

        context.addMatches(matches);
    }

private:
    void seen(const KService::Ptr &service)
    {
        m_seen.insert(service->exec());
    }

    void seen(const KServiceAction &action)
    {
        m_seen.insert(action.exec());
    }

    bool hasSeen(const KService::Ptr &service)
    {
        return m_seen.contains(service->exec());
    }

    bool hasSeen(const KServiceAction &action)
    {
        return m_seen.contains(action.exec());
    }

    bool disqualify(const KService::Ptr &service)
    {
        auto ret = hasSeen(service);
        qCDebug(RUNNER_SERVICES) << service->name() << "disqualified?" << ret;
        seen(service);
        return ret;
    }

    enum class Category {
        Name,
        GenericName,
        Comment,
    };
    qreal increaseMatchRelevance(const QString &serviceProperty, const QList<QStringView> &strList, Category category)
    {
        // Increment the relevance based on all the words (other than the first) of the query list
        qreal relevanceIncrement = 0;

        for (int i = 1; i < strList.size(); ++i) {
            const auto &str = strList.at(i);
            if (category == Category::Name) {
                if (serviceProperty.contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == Category::GenericName) {
                if (serviceProperty.contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            } else if (category == Category::Comment) {
                if (serviceProperty.contains(str, Qt::CaseInsensitive)) {
                    relevanceIncrement += 0.01;
                }
            }
        }

        return relevanceIncrement;
    }

    void setupMatch(const KService::Ptr &service, KRunner::QueryMatch &match)
    {
        const QString name = service->name();
        const QString exec = service->exec();

        match.setText(name);

        QUrl url(service->storageId());
        url.setScheme(QStringLiteral("applications"));
        match.setData(url);
        match.setUrls({QUrl::fromLocalFile(service->entryPath())});

        QString urlPath = resolvedArgs(exec);
        if (urlPath.isEmpty()) {
            // Otherwise we might filter out broken services. Rather than hiding them, it is better to show an error message on launch (as done by KIO's jobs)
            urlPath = exec;
        }
        match.setId(QString(u"exec://" + urlPath));
        if (!service->genericName().isEmpty() && service->genericName() != name) {
            match.setSubtext(service->genericName());
        } else if (!service->comment().isEmpty()) {
            match.setSubtext(service->comment());
        }

        if (!service->icon().isEmpty()) {
            match.setIconName(service->icon());
        }
    }

    QString resolvedArgs(const QString &exec)
    {
        const KService syntheticService(QString(), exec, QString());
        KIO::DesktopExecParser parser(syntheticService, {});
        QStringList resultingArgs = parser.resultingArguments();
        if (const auto error = parser.errorMessage(); resultingArgs.isEmpty() && !error.isEmpty()) {
            qCWarning(RUNNER_SERVICES) << "Failed to resolve executable from service. Error:" << error;
            return QString();
        }

        // Remove any environment variables.
        if (KIO::DesktopExecParser::executableName(exec) == QLatin1String("env")) {
            resultingArgs.removeFirst(); // remove "env".

            while (!resultingArgs.isEmpty() && resultingArgs.first().contains(QLatin1Char('='))) {
                resultingArgs.removeFirst();
            }

            // Now parse it again to resolve the path.
            resultingArgs = KIO::DesktopExecParser(KService(QString(), KShell::joinArgs(resultingArgs), QString()), {}).resultingArguments();
            return resultingArgs.join(QLatin1Char(' '));
        }

        // Remove special arguments that have no real impact on the application.
        static const auto specialArgs = {QStringLiteral("-qwindowtitle"), QStringLiteral("-qwindowicon"), QStringLiteral("--started-from-file")};

        for (const auto &specialArg : specialArgs) {
            auto index = resultingArgs.indexOf(specialArg);
            if (index > -1) {
                if (resultingArgs.count() > index) {
                    resultingArgs.removeAt(index);
                }
                if (resultingArgs.count() > index) {
                    resultingArgs.removeAt(index); // remove value, too, if any.
                }
            }
        }
        return resultingArgs.join(QLatin1Char(' '));
    }

    void matchNameKeywordAndGenericName()
    {
        const auto nameKeywordAndGenericNameFilter = [this](const KService::Ptr &service) {
            // Name
            if (contains(service->name(), queryList)) {
                return true;
            }
            // If the term length is < 3, no real point searching the untranslated Name, Keywords and GenericName
            if (weightedTermLength < 3) {
                return false;
            }
            if (contains(service->untranslatedName(), queryList)) {
                return true;
            }

            // Keywords
            if (contains(service->keywords(), queryList)) {
                return true;
            }
            // GenericName
            if (contains(service->genericName(), queryList) || contains(service->untranslatedGenericName(), queryList)) {
                return true;
            }
            // Comment
            if (contains(service->comment(), queryList)) {
                return true;
            }

            return false;
        };

        for (const KService::Ptr &service : m_services) {
            if (!nameKeywordAndGenericNameFilter(service) || disqualify(service)) {
                continue;
            }

            const QString id = service->storageId();
            const QString name = service->name();

            KRunner::QueryMatch::CategoryRelevance categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Moderate;
            qreal relevance(0.6);

            // If the term was < 3 chars and NOT at the beginning of the App's name, then chances are the user doesn't want that app
            if (weightedTermLength < 3) {
                if (name.startsWith(query, Qt::CaseInsensitive)) {
                    relevance = 0.9;
                } else {
                    continue;
                }
            } else if (name.compare(query, Qt::CaseInsensitive) == 0) {
                relevance = 1;
                categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Highest;
            } else if (const auto idx = name.indexOf(queryList[0], 0, Qt::CaseInsensitive); idx != -1) {
                relevance = 0.8;
                relevance += increaseMatchRelevance(name, queryList, Category::Name);
                if (idx == 0) {
                    relevance += 0.1;
                    categoryRelevance = KRunner::QueryMatch::CategoryRelevance::High;
                }
            } else if (const auto idx = service->genericName().indexOf(queryList[0], 0, Qt::CaseInsensitive); idx != -1) {
                relevance = 0.65;
                relevance += increaseMatchRelevance(service->genericName(), queryList, Category::GenericName);
                if (idx == 0) {
                    relevance += 0.05;
                }
            } else if (const auto idx = service->comment().indexOf(queryList[0], 0, Qt::CaseInsensitive); idx != -1) {
                relevance = 0.5;
                relevance += increaseMatchRelevance(service->comment(), queryList, Category::Comment);
                if (idx == 0) {
                    relevance += 0.05;
                }
            }

            KRunner::QueryMatch match(m_runner);
            match.setCategoryRelevance(categoryRelevance);
            setupMatch(service, match);

            qCDebug(RUNNER_SERVICES) << name << "is this relevant:" << relevance;
            match.setRelevance(relevance);

            matches << match;
        }
    }

    void matchCategories()
    {
        // Do not match categories for short queries, BUG: 469769
        if (weightedTermLength < 5) {
            return;
        }
        for (const KService::Ptr &service : m_services) {
            const QStringList categories = service->categories();
            if (disqualify(service) || !contains(categories, queryList)) {
                continue;
            }
            qCDebug(RUNNER_SERVICES) << service->name() << "is an exact match!" << service->storageId() << service->exec();

            KRunner::QueryMatch match(m_runner);
            setupMatch(service, match);

            qreal relevance = 0.4;
            if (std::ranges::any_of(categories, [this](const QString &category) {
                    return category.compare(query, Qt::CaseInsensitive) == 0;
                })) {
                relevance = 0.6;
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
        for (const KService::Ptr &service : m_services) {
            const auto actions = service->actions();
            // Skip SystemSettings as we find KCMs already
            if (actions.isEmpty() || service->storageId() == QLatin1String("systemsettings.desktop")) {
                continue;
            }

            for (const KServiceAction &action : actions) {
                if (action.text().isEmpty() || hasSeen(action)) {
                    continue;
                }
                seen(action);

                const auto matchIndex = action.text().indexOf(query, 0, Qt::CaseInsensitive);
                if (matchIndex < 0) {
                    continue;
                }

                KRunner::QueryMatch match(m_runner);
                if (!action.icon().isEmpty()) {
                    match.setIconName(action.icon());
                } else {
                    match.setIconName(service->icon());
                }
                match.setText(i18nc("Jump list search result, %1 is action (eg. open new tab), %2 is application (eg. browser)",
                                    "%1 - %2",
                                    action.text(),
                                    service->name()));

                QUrl url(service->storageId());
                url.setScheme(QStringLiteral("applications"));

                QUrlQuery urlQuery;
                urlQuery.addQueryItem(QStringLiteral("action"), action.name());
                url.setQuery(urlQuery);

                match.setData(url);

                qreal relevance = 0.5;
                if (action.text().compare(query, Qt::CaseInsensitive) == 0) {
                    relevance = 0.65;
                    match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::High); // Give it a higer match type to ensure it is shown, BUG: 455436
                } else if (matchIndex == 0) {
                    relevance += 0.05;
                }

                match.setRelevance(relevance);

                matches << match;
            }
        }
    }

    ServiceRunner *m_runner;
    QSet<QString> m_seen;
    const QList<KService::Ptr> m_services;

    QList<KRunner::QueryMatch> matches;
    QString query;
    QList<QStringView> queryList;
    int weightedTermLength = -1;
};

ServiceRunner::ServiceRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    addSyntax(QStringLiteral(":q:"), i18n("Finds applications whose name or description match :q:"));

    connect(this, &KRunner::AbstractRunner::prepare, this, [this]() {
        m_matching = true;
        if (m_services.isEmpty()) {
            m_services = KApplicationTrader::query([](const KService::Ptr &service) {
                return !service->noDisplay();
            });
        } else {
            KSycoca::self()->ensureCacheValid();
        }
    });
    connect(this, &KRunner::AbstractRunner::teardown, this, [this]() {
        m_matching = false;
    });
}

void ServiceRunner::init()
{
    //  connect to the thread-local singleton here
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, [this]() {
        if (m_matching) {
            m_services = KApplicationTrader::query([](const KService::Ptr &service) {
                return !service->noDisplay();
            });
        } else {
            // Invalidate for the next match session
            m_services.clear();
        }
    });
}

void ServiceRunner::match(KRunner::RunnerContext &context)
{
    ServiceFinder finder(this, m_services);
    finder.match(context);
}

void ServiceRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    const QUrl dataUrl = match.data().toUrl();

    KService::Ptr service = KService::serviceByStorageId(dataUrl.path());
    if (!service) {
        return;
    }

    KActivities::ResourceInstance::notifyAccessed(QUrl(QString(u"applications:" + service->storageId())), QStringLiteral("org.kde.krunner"));

    KIO::ApplicationLauncherJob *job = nullptr;

    const QString actionName = QUrlQuery(dataUrl).queryItemValue(QStringLiteral("action"));
    if (actionName.isEmpty()) {
        job = new KIO::ApplicationLauncherJob(service);
    } else {
        const auto actions = service->actions();
        auto it = std::ranges::find_if(actions, [&actionName](const KServiceAction &action) {
            return action.name() == actionName;
        });
        Q_ASSERT(it != actions.end());

        job = new KIO::ApplicationLauncherJob(*it);
    }

    auto *delegate = new KNotificationJobUiDelegate;
    delegate->setAutoErrorHandlingEnabled(true);
    job->setUiDelegate(delegate);
    job->start();
}

K_PLUGIN_CLASS_WITH_JSON(ServiceRunner, "plasma-runner-services.json")

#include "servicerunner.moc"

#include "moc_servicerunner.cpp"
