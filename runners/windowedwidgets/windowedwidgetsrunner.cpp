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

    foreach (const KPluginMetaData &md, Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        if (!md.isValid()) {
            continue;
        }

        if (((md.name().contains(term, Qt::CaseInsensitive) ||
             md.value(QLatin1String("GenericName")).contains(term, Qt::CaseInsensitive) ||
             md.description().contains(term, Qt::CaseInsensitive)) ||
             md.category().contains(term, Qt::CaseInsensitive) ||
             term.startsWith(i18nc("Note this is a KRunner keyword", "mobile applications"))) &&
             !md.rawData().value(QStringLiteral("NoDisplay")).toBool()) {

            QVariant val = md.value(QStringLiteral("X-Plasma-StandAloneApp"));
            if (!val.isValid() || !val.toBool()) {
                continue;
            }

            Plasma::QueryMatch match(this);
            setupMatch(md, match);
            if (md.name().compare(term, Qt::CaseInsensitive) == 0) {
                match.setType(Plasma::QueryMatch::ExactMatch);
                match.setRelevance(1);
            } else {
                match.setType(Plasma::QueryMatch::PossibleMatch);
                match.setRelevance(0.7);
            }
            matches << match;
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
    KPluginMetaData md(match.data().toString());
    if (md.isValid()) {
        QProcess::startDetached(QStringLiteral("plasmawindowed"), QStringList() << md.pluginId());
    }
}

void WindowedWidgetsRunner::setupMatch(const KPluginMetaData &md, Plasma::QueryMatch &match)
{
    const QString name = md.pluginId();

    match.setText(name);
    match.setData(md.metaDataFileName());

    if (!md.name().isEmpty() && md.name() != name) {
        match.setSubtext(md.name());
    } else if (!md.description().isEmpty()) {
        match.setSubtext(md.description());
    }

    if (!md.iconName().isEmpty()) {
        match.setIconName(md.iconName());
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

