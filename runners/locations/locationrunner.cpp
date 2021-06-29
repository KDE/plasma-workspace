/*
    SPDX-FileCopyrightText: 2007 Teemu Rytilahti <tpr@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "locationrunner.h"

#include <QDir>
#include <QIcon>
#include <QMimeData>
#include <QUrl>

#include <KApplicationTrader>
#include <KIO/DesktopExecParser>
#include <KIO/Global>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KProtocolInfo>
#include <KShell>
#include <KUriFilter>
#include <QDebug>

K_PLUGIN_CLASS_WITH_JSON(LocationsRunner, "plasma-runner-locations.json")

LocationsRunner::LocationsRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    // set the name shown after the result in krunner window
    setObjectName(QStringLiteral("Locations"));
    addSyntax(
        Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds local directories and files, network locations and Internet sites with paths matching :q:.")));
}

LocationsRunner::~LocationsRunner()
{
}

void LocationsRunner::match(Plasma::RunnerContext &context)
{
    QString term = context.query();
    // If we have a query with an executable and optionally arguments, BUG: 433053
    const QStringList split = KShell::splitArgs(term);
    if (!split.isEmpty()) {
        QFileInfo tmpInfo(KShell::tildeExpand(split.constFirst()));
        if (tmpInfo.isFile() && tmpInfo.isExecutable()) {
            return;
        }
    }
    // We want to expand ENV variables like $HOME to get the actual path, BUG: 358221
    KUriFilter::self()->filterUri(term, {QStringLiteral("kshorturifilter")});
    const QUrl url(term);
    // The uri filter takes care of the shell expansion
    const QFileInfo fileInfo = QFileInfo(url.toLocalFile());

    if (fileInfo.exists()) {
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::ExactMatch);
        match.setText(i18n("Open %1", context.query()));
        match.setIconName(fileInfo.isFile() ? KIO::iconNameForUrl(url) : QStringLiteral("system-file-manager"));

        match.setRelevance(1);
        match.setData(url);
        match.setUrls({url});
        match.setType(Plasma::QueryMatch::ExactMatch);
        context.addMatch(match);
    } else if (!url.isLocalFile() && !url.isEmpty() && !url.scheme().isEmpty()) {
        const QString protocol = url.scheme();
        Plasma::QueryMatch match(this);
        match.setData(url);
        match.setUrls({url});

        if (!KProtocolInfo::isKnownProtocol(protocol) || KProtocolInfo::isHelperProtocol(protocol)) {
            const KService::Ptr service = KApplicationTrader::preferredService(QLatin1String("x-scheme-handler/") + protocol);
            if (service) {
                match.setIconName(service->icon());
                match.setText(i18n("Launch with %1", service->name()));
            } else if (KProtocolInfo::isKnownProtocol(protocol)) {
                Q_ASSERT(KProtocolInfo::isHelperProtocol(protocol));
                match.setIconName(KProtocolInfo::icon(protocol));
                match.setText(i18n("Launch with %1", KIO::DesktopExecParser::executableName(KProtocolInfo::exec(protocol))));
            } else {
                return;
            }
        } else {
            match.setIconName(KProtocolInfo::icon(protocol));
            match.setText(i18n("Go to %1", url.toDisplayString(QUrl::PreferLocalFile)));
            // in case of https://phabricator.kde.org we add a slash at the end to make it comparable to results of other runners
            if (url.scheme() == QLatin1String("https") && url.toString().endsWith(url.host())) {
                match.setId(url.toString() + QLatin1Char('/'));
            }
        }

        if (url.scheme() == QLatin1String("mailto")) {
            match.setText(i18n("Send email to %1", url.path()));
        }
        context.addMatch(match);
    }
}

void LocationsRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    auto *job = new KIO::OpenUrlJob(match.data().toUrl());
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->setRunExecutables(false);
    job->start();
}

#include "locationrunner.moc"
