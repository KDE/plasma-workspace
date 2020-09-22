/***************************************************************************
 *   Copyright © 2016 Aleix Pol Gonzalez <aleixpol@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "appstreamrunner.h"

#include <AppStreamQt/icon.h>

#include <QDir>
#include <QIcon>
#include <QDesktopServices>
#include <QDebug>

#include <KLocalizedString>
#include <KApplicationTrader>
#include <KSycoca>

#include "debug.h"

K_EXPORT_PLASMA_RUNNER_WITH_JSON(InstallerRunner, "plasma-runner-appstream.json")

InstallerRunner::InstallerRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    setObjectName(QStringLiteral("Installation Suggestions"));
    setPriority(AbstractRunner::HighestPriority);

    addSyntax(Plasma::RunnerSyntax(":q:", i18n("Looks for non-installed components according to :q:")));
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
    } else for(const AppStream::Icon &icon : icons) {
        QStringList stock;
        switch(icon.kind()) {
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
    if(context.query().size() <= 2)
        return;

    const auto components = findComponentsByString(context.query()).mid(0, 3);

    for (const AppStream::Component &component : components) {
        if (component.kind() != AppStream::Component::KindDesktopApp)
            continue;

        // KApplicationTrader uses KService which uses KSycoca which holds
        // KDirWatch instances to monitor changes. We don't need this on
        // our runner threads - let's not needlessly allocate inotify instances.
        KSycoca::disableAutoRebuild();
        const QString componentId = component.id();
        const auto servicesFound = KApplicationTrader::query([&componentId] (const KService::Ptr &service) {
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
        match.setText(i18n("Get %1...", component.name()));
        match.setSubtext(component.summary());
        match.setData(QUrl("appstream://" + componentId));
        context.addMatch(match);
    }
}

void InstallerRunner::run(const Plasma::RunnerContext &/*context*/, const Plasma::QueryMatch &match)
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
    if(!opened) {
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
