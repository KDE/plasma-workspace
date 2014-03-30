/*
 *   Copyright (C) 2007 Teemu Rytilahti <tpr@iki.fi>
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

#include "locationrunner.h"

#include <QMimeData>

#include <QDebug>
#include <KRun>
#include <KLocale>
#include <KMimeType>
#include <KShell>
#include <KUrl>
#include <KIcon>
#include <KProtocolInfo>
#include <KUriFilter>

#include <kservicetypetrader.h>

K_EXPORT_PLASMA_RUNNER(locations, LocationsRunner)


LocationsRunner::LocationsRunner(QObject *parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args);
    // set the name shown after the result in krunner window
    setObjectName(QLatin1String("Locations"));
    setIgnoredTypes(Plasma::RunnerContext::Executable | Plasma::RunnerContext::ShellCommand);
    addSyntax(Plasma::RunnerSyntax(":q:",
              i18n("Finds local directories and files, network locations and Internet sites with paths matching :q:.")));
}

LocationsRunner::~LocationsRunner()
{
}

void LocationsRunner::match(Plasma::RunnerContext &context)
{
    QString term = context.query();
    Plasma::RunnerContext::Type type = context.type();

    if (type == Plasma::RunnerContext::Directory || type == Plasma::RunnerContext::File) {
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setText(i18n("Open %1", term));

        if (type == Plasma::RunnerContext::File) {
            match.setIcon(KIcon(KMimeType::iconNameForUrl(KUrl(term))));
        } else {
            match.setIcon(KIcon("system-file-manager"));
        }

        match.setRelevance(1);
        match.setData(term);
        match.setType(Plasma::QueryMatch::ExactMatch);

        if (type == Plasma::RunnerContext::Directory) {
            match.setId("opendir");
        } else {
            match.setId("openfile");
        }
        context.addMatch(match);
    } else if (type == Plasma::RunnerContext::Help) {
        //qDebug() << "Locations matching because of" << type;
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setText(i18n("Open %1", term));
        match.setIcon(KIcon("system-help"));
        match.setRelevance(1);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setId("help");
        context.addMatch(match);
    } else if (type == Plasma::RunnerContext::NetworkLocation || type == Plasma::RunnerContext::UnknownType) {
        const bool filtered = KUriFilter::self()->filterUri(term, QStringList() << QLatin1String("kshorturifilter"));

        if (!filtered) {
            return;
        }

        KUrl url(term);

        if (!KProtocolInfo::isKnownProtocol(url.protocol())) {
            return;
        }

        Plasma::QueryMatch match(this);
        match.setText(i18n("Go to %1", url.prettyUrl()));
        match.setIcon(KIcon(KProtocolInfo::icon(url.protocol())));
        match.setData(url.url());

        if (KProtocolInfo::isHelperProtocol(url.protocol())) {
            //qDebug() << "helper protocol" << url.protocol() <<"call external application" ;
            if (url.protocol() == "mailto") {
                match.setText(i18n("Send email to %1",url.path()));
            } else {
                match.setText(i18n("Launch with %1", KProtocolInfo::exec(url.protocol())));
            }
        } else {
            //qDebug() << "protocol managed by browser" << url.protocol();
            match.setText(i18n("Go to %1", url.prettyUrl()));
        }

        if (type == Plasma::RunnerContext::UnknownType) {
            match.setId("openunknown");
            match.setRelevance(0.5);
            match.setType(Plasma::QueryMatch::PossibleMatch);
        } else {
            match.setId("opennetwork");
            match.setRelevance(0.7);
            match.setType(Plasma::QueryMatch::ExactMatch);
        }

        context.addMatch(match);
    }
}

void LocationsRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    QString location = context.query();

    if (location.isEmpty()) {
        return;
    }

    //qDebug() << "command: " << context.query();
    //qDebug() << "url: " << location << data;

    KUrl urlToRun(KUriFilter::self()->filteredUri(location, QStringList() << QLatin1String("kshorturifilter")));

    new KRun(urlToRun, 0);
}

QMimeData * LocationsRunner::mimeDataForMatch(const Plasma::QueryMatch *match)
{
    const QString data = match->data().toString();
    if (!data.isEmpty()) {
        KUrl url(data);
        QList<QUrl> list;
        list << url;
        QMimeData *result = new QMimeData();
        result->setUrls(list);
        result->setText(data);
        return result;
    }

    return 0;
}


#include "locationrunner.moc"
