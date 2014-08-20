/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#include "baloosearchrunner.h"

#include <QIcon>
#include <QDir>
#include <KRun>
#include <KRunner/QueryMatch>
#include <KLocalizedString>

#include <Baloo/Query>
#include <Baloo/Result>

SearchRunner::SearchRunner(QObject* parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
{
}

SearchRunner::SearchRunner(QObject* parent, const QString& serviceId)
    : Plasma::AbstractRunner(parent, serviceId)
{
}

void SearchRunner::init()
{
    Plasma::RunnerSyntax syntax(":q", i18n("Search through files, emails and contacts"));
}

SearchRunner::~SearchRunner()
{
}


QStringList SearchRunner::categories() const
{
    QStringList list;
    list << i18n("Audio")
         << i18n("Image")
         << i18n("Document")
         << i18n("Video")
         << i18n("Folder")
         << i18n("Email");

    return list;
}

QIcon SearchRunner::categoryIcon(const QString& category) const
{
    if (category == i18n("Audio")) {
        return QIcon::fromTheme("audio");
    } else if (category == i18n("Image")) {
        return QIcon::fromTheme("image");
    } else if (category == i18n("Document")) {
        return QIcon::fromTheme("application-pdf");
    } else if (category == i18n("Video")) {
        return QIcon::fromTheme("video");
    } else if (category == i18n("Folder")) {
        return QIcon::fromTheme("folder");
    } else if (category == i18n("Email")) {
        return QIcon::fromTheme("mail-message");
    }

    return Plasma::AbstractRunner::categoryIcon(category);
}

void SearchRunner::match(Plasma::RunnerContext& context, const QString& type,
                         const QString& category)
{
    if (!context.isValid())
        return;

    const QStringList categories = context.enabledCategories();
    if (!categories.isEmpty() && !categories.contains(category))
        return;

    Baloo::Query query;
    query.setSearchString(context.query());
    query.setType(type);
    query.setLimit(10);

    Baloo::ResultIterator it = query.exec();

    int relevance = 100;
    while (context.isValid() && it.next()) {
        Plasma::QueryMatch match(this);
        match.setIcon(QIcon::fromTheme(it.icon()));
        match.setId(it.id());
        match.setText(it.text());
        match.setData(it.url());
        match.setType(Plasma::QueryMatch::PossibleMatch);
        match.setMatchCategory(category);
        match.setRelevance(relevance * .01);
        relevance--;

        QString url = it.url().toLocalFile();
        if (url.startsWith(QDir::homePath())) {
            url.replace(0, QDir::homePath().length(), QLatin1String("~"));
        }
        match.setSubtext(url);

        context.addMatch(match);
    }
}

void SearchRunner::match(Plasma::RunnerContext& context)
{
    match(context, QLatin1String("File/Audio"), i18n("Audio"));
    match(context, QLatin1String("File/Image"), i18n("Image"));
    match(context, QLatin1String("File/Document"), i18n("Document"));
    match(context, QLatin1String("File/Video"), i18n("Video"));
    match(context, QLatin1String("File/Folder"), i18n("Folder"));
    match(context, QLatin1String("Email"), i18n("Email"));
}

void SearchRunner::run(const Plasma::RunnerContext&, const Plasma::QueryMatch& match)
{
    const QUrl url = match.data().toUrl();
    new KRun(url, 0);
}

K_EXPORT_PLASMA_RUNNER(baloosearchrunner, SearchRunner)

#include "baloosearchrunner.moc"
