/*
 *   Copyright 2008 David Edmundson <kde@davidedmundson.co.uk>
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

#include "placesrunner.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

#include <QDebug>
#include <QIcon>
#include <QMimeData>
#include <QUrl>
#include <KRun>
#include <KLocalizedString>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(PlacesRunner, "plasma-runner-places.json")

//Q_DECLARE_METATYPE(Plasma::RunnerContext)
PlacesRunner::PlacesRunner(QObject* parent, const QVariantList &args)
        : Plasma::AbstractRunner(parent, args)
{
    setObjectName(QStringLiteral("Places"));
    Plasma::RunnerSyntax defaultSyntax(i18n("places"), i18n("Lists all file manager locations"));
    setDefaultSyntax(defaultSyntax);
    addSyntax(defaultSyntax);
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds file manager locations that match :q:")));

    // ensure the bookmarkmanager, etc. in the places model gets creates created in the main thread
    // otherwise crashes ensue
    m_helper = new PlacesRunnerHelper(this);
}

PlacesRunner::~PlacesRunner()
{
}

void PlacesRunner::match(Plasma::RunnerContext &context)
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        // from the main thread
        //qDebug() << "calling";
        m_helper->match(&context);
    } else {
        // from the non-gui thread
        //qDebug() << "emitting";
        emit doMatch(&context);
    }
    //m_helper->match(c);
}

PlacesRunnerHelper::PlacesRunnerHelper(PlacesRunner *runner)
    : QObject(runner)
{
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    connect(runner, &PlacesRunner::doMatch,
            this, &PlacesRunnerHelper::match,
            Qt::BlockingQueuedConnection);

    connect(&m_places, &KFilePlacesModel::setupDone, this, [this](const QModelIndex &index, bool success) {
        if (success && m_pendingUdi == m_places.deviceForIndex(index).udi()) {
            new KRun(m_places.url(index), nullptr);
        }
        m_pendingUdi.clear();
    });
}

void PlacesRunnerHelper::match(Plasma::RunnerContext *c)
{
    Plasma::RunnerContext &context = *c;
    if (!context.isValid()) {
        return;
    }

    const QString term = context.query();

    if (term.length() < 3) {
        return;
    }

    QList<Plasma::QueryMatch> matches;
    const bool all = term.compare(i18n("places"), Qt::CaseInsensitive) == 0;
    for (int i = 0; i <= m_places.rowCount(); i++) {
        QModelIndex current_index = m_places.index(i, 0);
        Plasma::QueryMatch::Type type = Plasma::QueryMatch::NoMatch;
        qreal relevance = 0;

        const QString text = m_places.text(current_index);
        if ((all && !text.isEmpty()) || text.compare(term, Qt::CaseInsensitive) == 0) {
            type = Plasma::QueryMatch::ExactMatch;
            relevance = all ? 0.9 : 1.0;
        } else if (text.contains(term, Qt::CaseInsensitive)) {
            type = Plasma::QueryMatch::PossibleMatch;
            relevance = 0.7;
        }

        if (type != Plasma::QueryMatch::NoMatch) {
            Plasma::QueryMatch match(static_cast<PlacesRunner *>(parent()));
            match.setType(type);
            match.setRelevance(relevance);
            match.setIcon(m_places.icon(current_index));
            match.setText(text);

            // Add category as subtext so one can tell "Pictures" folder from "Search for Pictures"
            // Don't add it if it would match the category ("Places") of the runner to avoid "Places: Pictures (Places)"
            const QString groupName = m_places.data(current_index, KFilePlacesModel::GroupRole).toString();
            if (!groupName.isEmpty() && !static_cast<PlacesRunner *>(parent())->categories().contains(groupName)) {
                match.setSubtext(groupName);
            }

            //if we have to mount it set the device udi instead of the URL, as we can't open it directly
            if (m_places.isDevice(current_index) && m_places.setupNeeded(current_index)) {
                const QString udi = m_places.deviceForIndex(current_index).udi();
                match.setId(udi);
                match.setData(udi);
            } else {
                const QUrl url = KFilePlacesModel::convertedUrl(m_places.url(current_index));
                match.setData(url);
                match.setId(url.toDisplayString());
            }

            matches << match;
        }
    }

    context.addMatches(matches);
}

void PlacesRunnerHelper::openDevice(const QString &udi)
{
    m_pendingUdi.clear();

    for (int i = 0; i < m_places.rowCount(); ++i) {
        const QModelIndex idx = m_places.index(i, 0);
        if (m_places.isDevice(idx) && m_places.deviceForIndex(idx).udi() == udi) {
            m_pendingUdi = udi;
            m_places.requestSetup(idx);
            break;
        }
    }
}

void PlacesRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action)
{
    Q_UNUSED(context);
    //I don't just pass the model index because the list could change before the user clicks on it, which would make everything go wrong. Ideally we don't want things to go wrong.
    if (action.data().type() == QVariant::Url) {
        new KRun(action.data().toUrl(), nullptr);
    } else if (action.data().canConvert<QString>()) {
        m_helper->openDevice(action.data().toString());
    }
}

QMimeData *PlacesRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    if (match.data().type() == QVariant::Url) {
        QMimeData *result = new QMimeData();
        result->setUrls({match.data().toUrl()});
        return result;
    }

    return nullptr;
}

#include "placesrunner.moc"
