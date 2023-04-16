/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2010 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
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

K_PLUGIN_CLASS_WITH_JSON(WindowedWidgetsRunner, "plasma-runner-windowedwidgets.json")

WindowedWidgetsRunner::WindowedWidgetsRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    setObjectName(QStringLiteral("WindowedWidgets"));
    setPriority(AbstractRunner::HighestPriority);

    addSyntax(KRunner::RunnerSyntax(QStringLiteral(":q:"), i18n("Finds Plasma widgets whose name or description match :q:")));
    addSyntax(KRunner::RunnerSyntax(i18nc("Note this is a KRunner keyword", "mobile applications"),
                                    i18n("list all Plasma widgets that can run as standalone applications")));
    setMinLetterCount(3);
    connect(this, &AbstractRunner::teardown, this, [this]() {
        m_applets.clear();
    });
}

WindowedWidgetsRunner::~WindowedWidgetsRunner()
{
}

void WindowedWidgetsRunner::match(KRunner::RunnerContext &context)
{
    loadMetadataList();
    const QString term = context.query();
    QList<KRunner::QueryMatch> matches;

    for (const KPluginMetaData &md : qAsConst(m_applets)) {
        if (((md.name().contains(term, Qt::CaseInsensitive) || md.value(QLatin1String("GenericName")).contains(term, Qt::CaseInsensitive)
              || md.description().contains(term, Qt::CaseInsensitive))
             || md.category().contains(term, Qt::CaseInsensitive) || term.startsWith(i18nc("Note this is a KRunner keyword", "mobile applications")))) {
            KRunner::QueryMatch match(this);
            match.setText(md.name());
            match.setSubtext(md.description());
            match.setIconName(md.iconName());
            match.setData(md.pluginId());
            if (md.name().compare(term, Qt::CaseInsensitive) == 0) {
                match.setType(KRunner::QueryMatch::ExactMatch);
                match.setRelevance(1);
            } else {
                match.setType(KRunner::QueryMatch::PossibleMatch);
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

void WindowedWidgetsRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    Q_UNUSED(context);
    QProcess::startDetached(QStringLiteral("plasmawindowed"), {match.data().toString()});
}

QMimeData *WindowedWidgetsRunner::mimeDataForMatch(const KRunner::QueryMatch &match)
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
