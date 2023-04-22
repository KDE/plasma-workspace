/*
    SPDX-FileCopyrightText: 2008 David Edmundson <kde@davidedmundson.co.uk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "placesrunner.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

#include <QDebug>
#include <QIcon>
#include <QMimeData>
#include <QUrl>

#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>

K_PLUGIN_CLASS_WITH_JSON(PlacesRunner, "plasma-runner-places.json")

PlacesRunner::PlacesRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    setObjectName(QStringLiteral("Places"));
    addSyntax(i18n("places"), i18n("Lists all file manager locations"));
    addSyntax(QStringLiteral(":q:"), i18n("Finds file manager locations that match :q:"));

    // ensure the bookmarkmanager, etc. in the places model gets creates created in the main thread
    // otherwise crashes ensue
    m_helper = new PlacesRunnerHelper(this);
    setMinLetterCount(3);
}

PlacesRunner::~PlacesRunner()
{
}

void PlacesRunner::match(KRunner::RunnerContext &context)
{
    if (QThread::currentThread() == QCoreApplication::instance()->thread()) {
        // from the main thread
        // qDebug() << "calling";
        m_helper->match(&context);
    } else {
        // from the non-gui thread
        // qDebug() << "emitting";
        Q_EMIT doMatch(&context);
    }
    // m_helper->match(c);
}

PlacesRunnerHelper::PlacesRunnerHelper(PlacesRunner *runner)
    : QObject(runner)
{
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    connect(runner, &PlacesRunner::doMatch, this, &PlacesRunnerHelper::match, Qt::BlockingQueuedConnection);

    connect(&m_places, &KFilePlacesModel::setupDone, this, [this](const QModelIndex &index, bool success) {
        if (success && m_pendingUdi == m_places.deviceForIndex(index).udi()) {
            auto *job = new KIO::OpenUrlJob(m_places.url(index));
            job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
            job->setRunExecutables(false);
            job->start();
        }
        m_pendingUdi.clear();
    });
}

void PlacesRunnerHelper::match(KRunner::RunnerContext *c)
{
    KRunner::RunnerContext &context = *c;
    if (!context.isValid()) {
        return;
    }

    const QString term = context.query();
    QList<KRunner::QueryMatch> matches;
    const bool all = term.compare(i18n("places"), Qt::CaseInsensitive) == 0;
    for (int i = 0; i <= m_places.rowCount(); i++) {
        QModelIndex current_index = m_places.index(i, 0);
        KRunner::QueryMatch::Type type = KRunner::QueryMatch::NoMatch;
        qreal relevance = 0;

        const QString text = m_places.text(current_index);
        if ((all && !text.isEmpty()) || text.compare(term, Qt::CaseInsensitive) == 0) {
            type = KRunner::QueryMatch::ExactMatch;
            relevance = all ? 0.9 : 1.0;
        } else if (text.contains(term, Qt::CaseInsensitive)) {
            type = KRunner::QueryMatch::PossibleMatch;
            relevance = 0.7;
        }

        if (type != KRunner::QueryMatch::NoMatch) {
            KRunner::QueryMatch match(static_cast<PlacesRunner *>(parent()));
            match.setType(type);
            match.setRelevance(relevance);
            match.setIcon(m_places.icon(current_index));
            match.setText(text);

            // Add category as subtext so one can tell "Pictures" folder from "Search for Pictures"
            // Don't add it if it would match the category ("Places") of the runner to avoid "Places: Pictures (Places)"
            const QString groupName = m_places.data(current_index, KFilePlacesModel::GroupRole).toString();
            if (!groupName.isEmpty() && static_cast<PlacesRunner *>(parent())->name() != groupName) {
                match.setSubtext(groupName);
            }

            // if we have to mount it set the device udi instead of the URL, as we can't open it directly
            if (m_places.isDevice(current_index) && m_places.setupNeeded(current_index)) {
                const QString udi = m_places.deviceForIndex(current_index).udi();
                match.setId(udi);
                match.setData(udi);
            } else {
                const QUrl url = KFilePlacesModel::convertedUrl(m_places.url(current_index));
                match.setData(url);
                match.setUrls({url});
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

void PlacesRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action)
{
    Q_UNUSED(context);
    // I don't just pass the model index because the list could change before the user clicks on it, which would make everything go wrong. Ideally we don't want
    // things to go wrong.
    if (action.data().type() == QVariant::Url) {
        auto *job = new KIO::OpenUrlJob(action.data().toUrl());
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
        job->setRunExecutables(false);
        job->start();
    } else if (action.data().canConvert<QString>()) {
        m_helper->openDevice(action.data().toString());
    }
}

#include "placesrunner.moc"
