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

K_EXPORT_PLASMA_RUNNER(placesrunner, PlacesRunner)

//Q_DECLARE_METATYPE(Plasma::RunnerContext)
PlacesRunner::PlacesRunner(QObject* parent, const QVariantList &args)
        : Plasma::AbstractRunner(parent, args)
{
//    qRegisterMetaType
    Q_UNUSED(args)
    setObjectName( QStringLiteral("Places" ));
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

            //if we have to mount it set the device udi instead of the URL, as we can't open it directly
            QUrl url;
            if (m_places.isDevice(current_index) && m_places.setupNeeded(current_index)) {
                url = QUrl(m_places.deviceForIndex(current_index).udi());
            } else {
                url = m_places.url(current_index);
            }

            match.setData(url);
            match.setId(url.toDisplayString());
            matches << match;
        }
    }

    context.addMatches(matches);
}


void PlacesRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action)
{
    Q_UNUSED(context);
    //I don't just pass the model index because the list could change before the user clicks on it, which would make everything go wrong. Ideally we don't want things to go wrong.
    if (action.data().type() == QVariant::Url) {
        new KRun(action.data().toUrl(), 0);
    } else if (action.data().canConvert<QString>()) {
        //search our list for the device with the same udi, then set it up (mount it).
        QString deviceUdi = action.data().toString();

        // gets deleted in setupComplete
        KFilePlacesModel *places = new KFilePlacesModel(this);
        connect(places, SIGNAL(setupDone(QModelIndex,bool)), SLOT(setupComplete(QModelIndex,bool)));
        bool found = false;

        for (int i = 0; i <= places->rowCount();i++) {
            QModelIndex current_index = places->index(i, 0);
            if (places->isDevice(current_index) && places->deviceForIndex(current_index).udi() == deviceUdi) {
                places->requestSetup(current_index);
                found = true;
                break;
            }
        }

        if (!found) {
            delete places;
        }
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

//if a device needed mounting, this slot gets called when it's finished.
void PlacesRunner::setupComplete(QModelIndex index, bool success)
{
    KFilePlacesModel *places = qobject_cast<KFilePlacesModel*>(sender());
    //qDebug() << "setup complete" << places << sender();
    if (success && places) {
        new KRun(places->url(index), 0);
        places->deleteLater();
    }
}

#include "placesrunner.moc"
