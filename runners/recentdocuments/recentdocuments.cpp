/*
 *   Copyright 2008 Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
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

#include "recentdocuments.h"

#include <QMimeData>

#include <KDesktopFile>
#include <KConfigGroup>
#include <QDebug>
#include <KDirWatch>
#include <KRun>
#include <KRecentDocument>
#include <KLocalizedString>

K_EXPORT_PLASMA_RUNNER(recentdocuments, RecentDocuments)

RecentDocuments::RecentDocuments(QObject *parent, const QVariantList& args)
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args);
    setObjectName( QLatin1String("Recent Documents" ));
    m_icon = QIcon::fromTheme("document-open-recent");
    loadRecentDocuments();
    // listen for changes to the list of recent documents
    KDirWatch *recentDocWatch = new KDirWatch(this);
    recentDocWatch->addDir(KRecentDocument::recentDocumentDirectory(), KDirWatch::WatchFiles);
    connect(recentDocWatch, &KDirWatch::created, this, &RecentDocuments::loadRecentDocuments);
    connect(recentDocWatch, &KDirWatch::deleted, this, &RecentDocuments::loadRecentDocuments);
    connect(recentDocWatch, &KDirWatch::dirty, this, &RecentDocuments::loadRecentDocuments);
    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Looks for documents recently used with names matching :q:.")));
}

RecentDocuments::~RecentDocuments()
{
}

void RecentDocuments::loadRecentDocuments()
{
    //qDebug() << "Refreshing recent documents.";
    m_recentdocuments = KRecentDocument::recentDocuments();
}


void RecentDocuments::match(Plasma::RunnerContext &context)
{
    if (m_recentdocuments.isEmpty()) {
        return;
    }

    const QString term = context.query();
    if (term.length() < 3) {
        return;
    }

    foreach (const QString &document, m_recentdocuments) {
        if (!context.isValid()) {
            return;
        }

        if (document.contains(term, Qt::CaseInsensitive)) {
            KDesktopFile config(document);
            Plasma::QueryMatch match(this);
            match.setType(Plasma::QueryMatch::PossibleMatch);
            match.setRelevance(1.0);
            match.setIcon(QIcon::fromTheme(config.readIcon()));
            match.setData(config.readUrl());
            match.setText(config.readName());
            match.setSubtext(i18n("Recent Document"));
            context.addMatch(match);
        }
    }
}

void RecentDocuments::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)
    QString url = match.data().toString();
    qDebug() << "Opening Recent Document" << url;
    new KRun(url, 0);
}

QMimeData * RecentDocuments::mimeDataForMatch(const Plasma::QueryMatch& match)
{
    QMimeData * result = new QMimeData();
    QList<QUrl> urls;
    urls << QUrl(match.data().toString());
    result->setUrls(urls);

    result->setText(match.data().toString());
    return result;
}

#include "recentdocuments.moc"
