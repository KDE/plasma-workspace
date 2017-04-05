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
#include <KServiceTypeTrader>

K_EXPORT_PLASMA_RUNNER(installer, InstallerRunner)

InstallerRunner::InstallerRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args)

    setObjectName("Installation Suggestions");
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
    } else foreach(const AppStream::Icon &icon, icons) {
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

    auto components = findComponentsByString(context.query());

    foreach(const AppStream::Component &component, components) {
        if (component.kind() != AppStream::Component::KindDesktopApp)
            continue;

        const auto idWithoutDesktop = component.id().remove(".desktop");
        const auto serviceQuery = QStringLiteral("exist Exec and (('%1' =~ DesktopEntryName) or '%2' =~ DesktopEntryName)").arg(component.id(), idWithoutDesktop);
        const auto servicesFound = KServiceTypeTrader::self()->query(QStringLiteral("Application"), serviceQuery);
        if (!servicesFound.isEmpty())
            continue;

        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::PossibleMatch);
        match.setId(component.id());
        match.setIcon(componentIcon(component));
        match.setText(i18n("Get %1...", component.name()));
        match.setSubtext(component.summary());
        match.setData(QUrl("appstream://" + component.id()));
        context.addMatch(match);
    }
}

void InstallerRunner::run(const Plasma::RunnerContext &/*context*/, const Plasma::QueryMatch &match)
{
    const QUrl appstreamUrl = match.data().toUrl();
    if (!QDesktopServices::openUrl(appstreamUrl))
        qWarning() << "couldn't open" << appstreamUrl;
}

QList<AppStream::Component> InstallerRunner::findComponentsByString(const QString &query)
{
    QMutexLocker locker(&m_appstreamMutex);
    static bool opened = m_db.load();
    if(!opened) {
        qWarning() << "no appstream for you";
        return QList<AppStream::Component>();
    }

    return m_db.search(query);
}

#include "appstreamrunner.moc"
