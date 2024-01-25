/*
    SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appstreamrunner.h"

#include <unordered_set>

#include <AppStreamQt/icon.h>

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QTimer>

#include <KApplicationTrader>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KSycoca>

#include "debug.h"

K_PLUGIN_CLASS_WITH_JSON(InstallerRunner, "plasma-runner-appstream.json")

InstallerRunner::InstallerRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    addSyntax(":q:", i18n("Looks for non-installed components according to :q:"));
    setMinLetterCount(3);
}

static QIcon componentIcon(const AppStream::Component &comp)
{
    QIcon ret;
    const auto icons = comp.icons();
    if (icons.isEmpty()) {
        ret = QIcon::fromTheme(QStringLiteral("package-x-generic"));
    } else
        for (const AppStream::Icon &icon : icons) {
            QStringList stock;
            switch (icon.kind()) {
            case AppStream::Icon::KindLocal:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
            case AppStream::Icon::KindCached:
                ret.addFile(icon.url().toLocalFile(), icon.size());
                break;
            case AppStream::Icon::KindStock:
                stock += icon.name();
                break;
            default:
                break;
            }
            if (ret.isNull() && !stock.isEmpty()) {
                ret = QIcon::fromTheme(stock.first());
            }
        }
    return ret;
}

void InstallerRunner::match(KRunner::RunnerContext &context)
{
    // Give the other runners a bit of time to produce results
    QEventLoop loop;
    QTimer::singleShot(200, &loop, &QEventLoop::quit);
    loop.exec();
    if (!context.isValid()) {
        return;
    }

    // Check if other plugins have already found an executable, if that is the case we do
    // not want to ask the user to install anything else
    const QList<KRunner::QueryMatch> matches = context.matches();
    const bool execFound = std::any_of(matches.cbegin(), matches.cend(), [](const KRunner::QueryMatch &match) {
        return match.id().startsWith(QLatin1String("exec://"));
    });
    if (execFound) {
        return;
    }

    std::unordered_set<QString> uniqueIds;
    const auto components = findComponentsByString(context.query());

    for (auto it = components.cbegin(); it != components.cend() && uniqueIds.size() < 3; it = std::next(it)) {
        if (it->kind() != AppStream::Component::KindDesktopApp)
            continue;

        const QString componentId = it->id();
        const auto servicesFound = KApplicationTrader::query([&componentId](const KService::Ptr &service) {
            if (service->exec().isEmpty())
                return false;

            if (service->desktopEntryName().compare(componentId, Qt::CaseInsensitive) == 0)
                return true;

            const auto idWithoutDesktop = QString(componentId).remove(".desktop");
            if (service->desktopEntryName().compare(idWithoutDesktop, Qt::CaseInsensitive) == 0)
                return true;

            const auto renamedFrom = service->property<QStringList>("X-Flatpak-RenamedFrom");
            if (renamedFrom.contains(componentId, Qt::CaseInsensitive) || renamedFrom.contains(idWithoutDesktop, Qt::CaseInsensitive))
                return true;

            return false;
        });

        if (!servicesFound.isEmpty())
            continue;
        const auto [_, inserted] = uniqueIds.insert(componentId);
        if (!inserted) {
            continue;
        }

        KRunner::QueryMatch match(this);
        match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Lowest); // Make sure it is less relavant than KCMs or apps
        match.setId(componentId);
        match.setIcon(componentIcon(*it));
        match.setText(i18n("Get %1…", it->name()));
        match.setSubtext(it->summary());
        match.setData(QUrl("appstream://" + componentId));
        match.setRelevance(it->name().compare(context.query(), Qt::CaseInsensitive) == 0 ? 1. : 0.7);
        context.addMatch(match);
    }
}

void InstallerRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    const QUrl appstreamUrl = match.data().toUrl();

    auto job = new KIO::OpenUrlJob(appstreamUrl);
    job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoErrorHandlingEnabled));
    job->start();
}

void InstallerRunner::init()
{
    // KApplicationTrader uses KService which uses KSycoca which holds
    // KDirWatch instances to monitor changes. We don't need this on
    // our runner threads - let's not needlessly allocate inotify instances.
    KSycoca::disableAutoRebuild();
}

QList<AppStream::Component> InstallerRunner::findComponentsByString(const QString &query)
{
    static bool warnedOnce = false;
    static bool opened = m_db.load();
    if (!opened) {
        if (warnedOnce) {
            qCDebug(RUNNER_APPSTREAM) << "Had errors when loading AppStream metadata pool" << m_db.lastError();
        } else {
            qCWarning(RUNNER_APPSTREAM) << "Had errors when loading AppStream metadata pool" << m_db.lastError();
            warnedOnce = true;
        }
    }

    return m_db.search(query).toList();
}

#include "appstreamrunner.moc"
