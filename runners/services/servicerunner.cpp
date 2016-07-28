/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2014 Vishesh Handa <vhanda@kde.org>
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
    const QString term = context.query();

    QList<Plasma::QueryMatch> matches;
    QSet<QString> seen;
    QString query;

    if (term.length() > 1) {
        // Search for applications which are executable and case-insensitively match the search term
        // See http://techbase.kde.org/Development/Tutorials/Services/Traders#The_KTrader_Query_Language
        // if the following is unclear to you.
        query = QStringLiteral("exist Exec and ('%1' =~ Name)").arg(term);
        KService::List services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), query);

        if (!services.isEmpty()) {
            //qDebug() << service->name() << "is an exact match!" << service->storageId() << service->exec();
            foreach (const KService::Ptr &service, services) {
                if (!service->noDisplay() && service->property(QStringLiteral("NotShowIn"), QVariant::String) != "KDE") {
                    Plasma::QueryMatch match(this);
                    match.setType(Plasma::QueryMatch::ExactMatch);
                    setupMatch(service, match);
                    match.setRelevance(1);
                    matches << match;
                    seen.insert(service->storageId());
                    seen.insert(service->exec());
                }
            }
        }
    }

    if (!context.isValid()) {
        return;
    }

    // If the term length is < 3, no real point searching the Keywords and GenericName
    if (term.length() < 3) {
        query = QStringLiteral("exist Exec and ( (exist Name and '%1' ~~ Name) or ('%1' ~~ Exec) )").arg(term);
    } else {
        // Search for applications which are executable and the term case-insensitive matches any of
        // * a substring of one of the keywords
        // * a substring of the GenericName field
        // * a substring of the Name field
        // Note that before asking for the content of e.g. Keywords and GenericName we need to ask if
        // they exist to prevent a tree evaluation error if they are not defined.
        query = QStringLiteral("exist Exec and ( (exist Keywords and '%1' ~subin Keywords) or (exist GenericName and '%1' ~~ GenericName) or (exist Name and '%1' ~~ Name) or ('%1' ~~ Exec) or (exist Comment and '%1' ~~ Comment) )").arg(term);
    }

    KService::List services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), query);
    services += KServiceTypeTrader::self()->query(QStringLiteral("KCModule"), query);

    //qDebug() << "got " << services.count() << " services from " << query;
    foreach (const KService::Ptr &service, services) {
        if (service->noDisplay()) {
            continue;
        }

        const QString id = service->storageId();
        const QString name = service->desktopEntryName();
        const QString exec = service->exec();

        if (seen.contains(id) || seen.contains(exec)) {
            //qDebug() << "already seen" << id << exec;
            continue;
        }

        //qDebug() << "haven't seen" << id << "so processing now";
        seen.insert(id);
        seen.insert(exec);

        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::PossibleMatch);
        setupMatch(service, match);
        qreal relevance(0.6);

        // If the term was < 3 chars and NOT at the beginning of the App's name or Exec, then
        // chances are the user doesn't want that app.
        if (term.length() < 3) {
            if (name.startsWith(term) || exec.startsWith(term)) {
                relevance = 0.9;
            } else {
                continue;
            }
        } else if (service->name().contains(term, Qt::CaseInsensitive)) {
            relevance = 0.8;

            if (service->name().startsWith(term, Qt::CaseInsensitive)) {
                relevance += 0.1;
            }
        } else if (service->genericName().contains(term, Qt::CaseInsensitive)) {
            relevance = 0.65;

            if (service->genericName().startsWith(term, Qt::CaseInsensitive)) {
                relevance += 0.05;
            }
        } else if (service->exec().contains(term, Qt::CaseInsensitive)) {
            relevance = 0.7;

            if (service->exec().startsWith(term, Qt::CaseInsensitive)) {
                relevance += 0.05;
            }
        } else if (service->comment().contains(term, Qt::CaseInsensitive)) {
            relevance = 0.5;

            if (service->comment().startsWith(term, Qt::CaseInsensitive)) {
                relevance += 0.05;
            }
        }

        if (service->categories().contains(QStringLiteral("KDE")) || service->serviceTypes().contains(QStringLiteral("KCModule"))) {
            //qDebug() << "found a kde thing" << id << match.subtext() << relevance;
            if (id.startsWith(QLatin1String("kde-"))) {
                //qDebug() << "old" << service->type();
                if (!service->isApplication()) {
                    // avoid showing old kcms and what not
                    continue;
                }

                // This is an older version, let's disambiguate it
                QString subtext(QStringLiteral("KDE3"));

                if (!match.subtext().isEmpty()) {
                    subtext.append(", " + match.subtext());
                }

                match.setSubtext(subtext);
            } else {
                relevance += .09;
            }
        }

        //qDebug() << service->name() << "is this relevant:" << relevance;
        match.setRelevance(relevance);
        if (service->serviceTypes().contains(QStringLiteral("KCModule"))) {
            match.setMatchCategory(i18n("System Settings"));
        }
        matches << match;
    }

    //search for applications whose categories contains the query
    query = QStringLiteral("exist Exec and (exist Categories and '%1' ~subin Categories)").arg(term);
    services = KServiceTypeTrader::self()->query(QStringLiteral("Application"), query);

    //qDebug() << service->name() << "is an exact match!" << service->storageId() << service->exec();
    foreach (const KService::Ptr &service, services) {
        if (!service->noDisplay()) {
            QString id = service->storageId();
            QString exec = service->exec();
            if (seen.contains(id) || seen.contains(exec)) {
                //qDebug() << "already seen" << id << exec;
                continue;
            }
            Plasma::QueryMatch match(this);
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

    // search for jump list actions
    if (term.length() >= 3) {
        query = QStringLiteral("exist Actions"); // doesn't work
        services = KServiceTypeTrader::self()->query(QStringLiteral("Application"));//, query);

        foreach (const KService::Ptr &service, services) {
            if (service->noDisplay()) {
                continue;
            }

            foreach (const KServiceAction &action, service->actions()) {
                if (action.text().isEmpty() || action.exec().isEmpty() || seen.contains(action.exec())) {
                    continue;
                }

                if (!action.text().contains(term, Qt::CaseInsensitive)) {
                    continue;
                }

                Plasma::QueryMatch match(this);
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

    //context.addMatches(term, matches);
    context.addMatches(matches);
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

void ServiceRunner::setupMatch(const KService::Ptr &service, Plasma::QueryMatch &match)
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

QMimeData * ServiceRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    KService::Ptr service = KService::serviceByStorageId(match.data().toString());
    if (service) {
        QMimeData * result = new QMimeData();
        QList<QUrl> urls;
        urls << QUrl::fromLocalFile(service->entryPath());
        qDebug() << urls;
        result->setUrls(urls);
        return result;
    }

    return 0;
}

K_EXPORT_PLASMA_RUNNER(services, ServiceRunner)

#include "servicerunner.moc"

