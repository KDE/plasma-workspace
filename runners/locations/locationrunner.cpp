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
#include <QIcon>
#include <QUrl>
#include <QDir>

#include <QDebug>
#include <KRun>
#include <KLocalizedString>
#include <KProtocolInfo>
#include <KUriFilter>
#include <KIO/Global>

#include <kservicetypetrader.h>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(LocationsRunner, "plasma-runner-locations.json")


LocationsRunner::LocationsRunner(QObject *parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
{
    // set the name shown after the result in krunner window
    setObjectName(QStringLiteral("Locations"));
    setIgnoredTypes(Plasma::RunnerContext::Executable | Plasma::RunnerContext::ShellCommand);
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"),
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
            match.setIconName(KIO::iconNameForUrl(QUrl(term)));
        } else {
            match.setIconName(QStringLiteral("system-file-manager"));
        }

        match.setRelevance(1);
        match.setData(term);
        match.setType(Plasma::QueryMatch::ExactMatch);

        if (type == Plasma::RunnerContext::Directory) {
            match.setId(QStringLiteral("opendir"));
        } else {
            match.setId(QStringLiteral("openfile"));
        }
        context.addMatch(match);
    } else if (type == Plasma::RunnerContext::Help) {
        //qDebug() << "Locations matching because of" << type;
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setText(i18n("Open %1", term));
        match.setIconName(QStringLiteral("system-help"));
        match.setRelevance(1);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setId(QStringLiteral("help"));
        context.addMatch(match);
    } else if (type == Plasma::RunnerContext::NetworkLocation || type == Plasma::RunnerContext::UnknownType) {
        const bool filtered = KUriFilter::self()->filterUri(term, QStringList() << QStringLiteral("kshorturifilter"));

        if (!filtered) {
            return;
        }

        QUrl url(term);

        if (url.isEmpty() || !KProtocolInfo::isKnownProtocol(url.scheme())) {
            return;
        }

        Plasma::QueryMatch match(this);
        match.setIconName(KProtocolInfo::icon(url.scheme()));
        match.setData(url.url());

        if (KProtocolInfo::isHelperProtocol(url.scheme())) {
            //qDebug() << "helper protocol" << url.protocol() <<"call external application" ;
            if (url.scheme() == QLatin1String("mailto")) {
                match.setText(i18n("Send email to %1",url.path()));
            } else {
                match.setText(i18n("Launch with %1", KProtocolInfo::exec(url.scheme())));
            }
        } else {
            //qDebug() << "protocol managed by browser" << url.protocol();
            match.setText(i18n("Go to %1", url.toDisplayString()));
        }

        if (type == Plasma::RunnerContext::UnknownType) {
            match.setId(QStringLiteral("openunknown"));
            match.setRelevance(0.5);
            match.setType(Plasma::QueryMatch::PossibleMatch);
        } else {
            match.setId(QStringLiteral("opennetwork"));
            match.setRelevance(0.7);
            match.setType(Plasma::QueryMatch::ExactMatch);
        }

        context.addMatch(match);
    }
}

static QString convertCaseInsensitivePath(const QString &path)
{
    // Split the string on /
    const auto dirNames = path.splitRef(QDir::separator(), QString::SkipEmptyParts);

    // if split result is empty, path string can only contain separator.
    if (dirNames.empty()) {
        return QStringLiteral("/");
    }

    // Match folders
    QDir dir(QStringLiteral("/"));
    for (int i = 0; i < dirNames.size() - 1; ++i) {
        const QStringRef dirName = dirNames.at(i);

        bool foundMatch = false;
        const QStringList entries = dir.entryList(QDir::Dirs);
        for (const QString &entry : entries) {
            if (entry.compare(dirName, Qt::CaseInsensitive) == 0) {
                foundMatch = dir.cd(entry);
                if (foundMatch) {
                    break;
                }
            }
        }

        if (!foundMatch) {
            return path;
        }
    }

    const QStringRef finalName = dirNames.last();
    const QStringList entries = dir.entryList();
    for (const QString &entry : entries) {
        if (entry.compare(finalName, Qt::CaseInsensitive) == 0) {
            return dir.absoluteFilePath(entry);
        }
    }

    return path;
}

void LocationsRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    QString location = context.query();

    if (location.isEmpty()) {
        return;
    }

    location = convertCaseInsensitivePath(location);

    QUrl urlToRun(KUriFilter::self()->filteredUri(location, {QStringLiteral("kshorturifilter")}));

    new KRun(urlToRun, nullptr);
}

QMimeData * LocationsRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    const QString data = match.data().toString();
    if (!data.isEmpty()) {
        QMimeData *result = new QMimeData();
        result->setUrls({QUrl(data)});
        return result;
    }

    return nullptr;
}


#include "locationrunner.moc"
