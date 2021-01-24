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

#include <KLocalizedString>
#include <QDebug>

#include <Plasma/Applet>
#include <Plasma/PluginLoader>
#include <QMutexLocker>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(WindowedWidgetsRunner, "plasma-runner-windowedwidgets.json")

WindowedWidgetsRunner::WindowedWidgetsRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    setObjectName(QStringLiteral("WindowedWidgets"));
    setPriority(AbstractRunner::HighestPriority);

    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds Plasma widgets whose name or description match :q:")));
    addSyntax(Plasma::RunnerSyntax(i18nc("Note this is a KRunner keyword", "mobile applications"),
                                   i18n("list all Plasma widgets that can run as standalone applications")));
    setMinLetterCount(3);
    connect(this, &AbstractRunner::teardown, this, [this]() {
        m_applets.clear();
    });
}

WindowedWidgetsRunner::~WindowedWidgetsRunner()
{
}

void WindowedWidgetsRunner::match(Plasma::RunnerContext &context)
{
    loadMetadataList();
    const QString term = context.query();
    QList<Plasma::QueryMatch> matches;

    for (const KPluginMetaData &md : qAsConst(m_applets)) {
        if (((md.name().contains(term, Qt::CaseInsensitive) || md.value(QLatin1String("GenericName")).contains(term, Qt::CaseInsensitive)
              || md.description().contains(term, Qt::CaseInsensitive))
             || md.category().contains(term, Qt::CaseInsensitive) || term.startsWith(i18nc("Note this is a KRunner keyword", "mobile applications")))) {
            Plasma::QueryMatch match(this);
            match.setText(md.name());
            match.setSubtext(md.description());
            match.setIconName(md.iconName());
            match.setData(md.pluginId());
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
    QProcess::startDetached(QStringLiteral("plasmawindowed"), {match.data().toString()});
}

QMimeData *WindowedWidgetsRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    QMimeData *data = new QMimeData();
    data->setData(QStringLiteral("text/x-plasmoidservicename"), match.data().toString().toUtf8());
    return data;
}

void WindowedWidgetsRunner::loadMetadataList()
{
    // We call this method in the match thread
    QMutexLocker locker(&m_mutex);
    // If the entries have already been loaded we reuse them for the same session
    if (!m_applets.isEmpty()) {
        return;
    }
    const auto &listMetadata = Plasma::PluginLoader::self()->listAppletMetaData(QString());
    for (const KPluginMetaData &md : listMetadata) {
        if (md.isValid() && !md.rawData().value(QStringLiteral("NoDisplay")).toBool()
            && md.rawData().value(QStringLiteral("X-Plasma-StandAloneApp")).toBool()) {
            m_applets << md;
        }
    }
}

#include "windowedwidgetsrunner.moc"
