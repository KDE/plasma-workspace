/*
    SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "appstreamrunner.h"

#include <AppStreamQt/icon.h>

#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QTimer>

#include <KApplicationTrader>
#include <KLocalizedString>
#include <KSycoca>

#include "debug.h"

K_PLUGIN_CLASS_WITH_JSON(InstallerRunner, "plasma-runner-appstream.json")

InstallerRunner::InstallerRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    setObjectName(QStringLiteral("Installation Suggestions"));
    // We want to give the other runners time to check if there are matching applications already installed
    setPriority(AbstractRunner::LowestPriority);

    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Looks for non-installed components according to :q:")));
    setMinLetterCount(3);
}

InstallerRunner::~InstallerRunner()
{
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

void InstallerRunner::match(Plasma::RunnerContext &context)
{
    // Give the other runners a bit of time to produce results
    QEventLoop loop;
    QTimer::singleShot(200, &loop, [&loop]() {
        loop.quit();
    });
    loop.exec();
    if (!context.isValid()) {
        return;
    }

    // Check if other plugins have already found an executable, if that is the case we do
    // not want to ask the user to install anything else
    const QList<Plasma::QueryMatch> matches = context.matches();
    for (const auto &match : matches) {
        if (match.id().startsWith(QLatin1String("exec://"))) {
            return;
        }
    }

    const auto components = findComponentsByString(context.query()).mid(0, 3);

    for (const AppStream::Component &component : components) {
        if (component.kind() != AppStream::Component::KindDesktopApp)
            continue;

        // KApplicationTrader uses KService which uses KSycoca which holds
        // KDirWatch instances to monitor changes. We don't need this on
        // our runner threads - let's not needlessly allocate inotify instances.
        KSycoca::disableAutoRebuild();
        const QString componentId = component.id();
        const auto servicesFound = KApplicationTrader::query([&componentId](const KService::Ptr &service) {
            if (service->exec().isEmpty())
                return false;

            if (service->desktopEntryName().compare(componentId, Qt::CaseInsensitive) == 0)
                return true;

            const auto idWithoutDesktop = QString(componentId).remove(".desktop");
            if (service->desktopEntryName().compare(idWithoutDesktop, Qt::CaseInsensitive) == 0)
                return true;

            const auto renamedFrom = service->property("X-Flatpak-RenamedFrom").toStringList();
            if (renamedFrom.contains(componentId, Qt::CaseInsensitive) || renamedFrom.contains(idWithoutDesktop, Qt::CaseInsensitive))
                return true;

            return false;
        });

        if (!servicesFound.isEmpty())
            continue;

        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::PossibleMatch);
        match.setId(componentId);
        match.setIcon(componentIcon(component));
        match.setText(i18n("Get %1…", component.name()));
        match.setSubtext(component.summary());
        match.setData(QUrl("appstream://" + componentId));
        match.setRelevance(component.name().compare(context.query(), Qt::CaseInsensitive) == 0 ? 1. : 0.7);
        context.addMatch(match);
    }
}

void InstallerRunner::run(const Plasma::RunnerContext & /*context*/, const Plasma::QueryMatch &match)
{
    const QUrl appstreamUrl = match.data().toUrl();
    if (!QDesktopServices::openUrl(appstreamUrl))
        qCWarning(RUNNER_APPSTREAM) << "couldn't open" << appstreamUrl;
}

QList<AppStream::Component> InstallerRunner::findComponentsByString(const QString &query)
{
    QMutexLocker locker(&m_appstreamMutex);
    QString error;
    static bool warnedOnce = false;
    static bool opened = m_db.load(&error);
    if (!opened) {
        if (warnedOnce) {
            qCDebug(RUNNER_APPSTREAM) << "Had errors when loading AppStream metadata pool" << error;
        } else {
            qCWarning(RUNNER_APPSTREAM) << "Had errors when loading AppStream metadata pool" << error;
            warnedOnce = true;
        }
    }

    return m_db.search(query);
}

#include "appstreamrunner.moc"
