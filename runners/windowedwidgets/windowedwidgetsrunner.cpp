/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *   Copyright (C) 2010 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "windowedwidgetsrunner.h"

#include <QProcess>

#include <QIcon>
#include <QMimeData>

#include <QDebug>
#include <KLocalizedString>
#include <KService>

#include <Plasma/Applet>
#include <Plasma/PluginLoader>

K_EXPORT_PLASMA_RUNNER(windowedwidgets, WindowedWidgetsRunner)

WindowedWidgetsRunner::WindowedWidgetsRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args)

    setObjectName( QLatin1String("WindowedWidgets" ));
    setPriority(AbstractRunner::HighestPriority);

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds Plasma widgets whose name or description match :q:")));
    setDefaultSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "mobile applications"), i18n("list all Plasma widgets that can run as standalone applications")));
}

WindowedWidgetsRunner::~WindowedWidgetsRunner()
{
}

void WindowedWidgetsRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();

    if (!context.singleRunnerQueryMode() && term.length() <  3) {
        return;
    }


   QList<Plasma::QueryMatch> matches;


    foreach (const KPluginInfo &info, Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        KService::Ptr service = info.service();
        if (!service || !service->isValid()) {
            continue;
        }

        if (((service->name().contains(term, Qt::CaseInsensitive) ||
             service->genericName().contains(term, Qt::CaseInsensitive) ||
             service->comment().contains(term, Qt::CaseInsensitive)) ||
             service->categories().contains(term, Qt::CaseInsensitive) ||
             term.startsWith(i18nc("Note this is a KRunner keyword", "mobile applications"))) &&
             !info.property(QStringLiteral("NoDisplay")).toBool()) {

            QVariant val = info.property(QStringLiteral("X-Plasma-StandAloneApp"));
            if (!val.isValid() || !val.toBool()) {
                continue;
            }

            Plasma::QueryMatch match(this);
            setupMatch(service, match);
            if (service->name().compare(term, Qt::CaseInsensitive) == 0) {
                match.setType(Plasma::QueryMatch::ExactMatch);
                match.setRelevance(1);
            } else {
                match.setType(Plasma::QueryMatch::PossibleMatch);
                match.setRelevance(0.7);
            }
            matches << match;

            qDebug() << service;
        }
    }

    if (!context.isValid()) {
        return;
    }

    context.addMatches(matches);
}

void WindowedWidgetsRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context);
    KService::Ptr service = KService::serviceByStorageId(match.data().toString());
    if (service) {
        QProcess::startDetached(QStringLiteral("plasmawindowed"), QStringList() << service->property(QStringLiteral("X-KDE-PluginInfo-Name"), QVariant::String).toString());
    }
}

void WindowedWidgetsRunner::setupMatch(const KService::Ptr &service, Plasma::QueryMatch &match)
{
    const QString name = service->name();

    match.setText(name);
    match.setData(service->storageId());

    if (!service->genericName().isEmpty() && service->genericName() != name) {
        match.setSubtext(service->genericName());
    } else if (!service->comment().isEmpty()) {
        match.setSubtext(service->comment());
    }

    if (!service->icon().isEmpty()) {
        match.setIconName(service->icon());
    }
}

QMimeData * WindowedWidgetsRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    KService::Ptr service = KService::serviceByStorageId(match.data().toString());
    if (service) {

        QMimeData *data = new QMimeData();
        data->setData(QStringLiteral("text/x-plasmoidservicename"),
                      service->property(QStringLiteral("X-KDE-PluginInfo-Name"), QVariant::String).toString().toUtf8());
        return data;

    }

    return 0;
}


#include "windowedwidgetsrunner.moc"

