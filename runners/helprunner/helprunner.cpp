/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "helprunner.h"

#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KPluginMetaData>

HelpRunner::HelpRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args)
    : AbstractRunner(parent, pluginMetaData, args)
{
    setTriggerWords({i18nc("this is a runner keyword", "help"), QStringLiteral("?")});
    m_manager = qobject_cast<RunnerManager *>(parent);
    Q_ASSERT(m_manager);
    m_actionList = {new QAction(QIcon::fromTheme(QStringLiteral("configure")), i18n("Configure plugin"), this)};
}

void HelpRunner::match(RunnerContext &context)
{
    const QString sanatizedQuery = context.query().remove(matchRegex());
    auto runners = m_manager->runners();
    for (auto it = runners.begin(); it != runners.end();) {
        if (*it == this || (*it)->syntaxes().isEmpty()) {
            it = runners.erase(it);
        } else {
            ++it;
        }
    }

    QList<AbstractRunner *> matchingRunners;
    if (sanatizedQuery.isEmpty()) {
        matchingRunners = runners;
    } else {
        for (AbstractRunner *runner : std::as_const(runners)) {
            if (runner->id().contains(sanatizedQuery, Qt::CaseInsensitive) || runner->name().contains(sanatizedQuery, Qt::CaseInsensitive)) {
                matchingRunners << runner;
            }
        }
    }
    QList<QueryMatch> matches;
    bool showExtendedHelp = matchingRunners.count() == 1 && sanatizedQuery.count() >= 3;
    for (AbstractRunner *runner : std::as_const(matchingRunners)) {
        const QList<RunnerSyntax> syntaxes = runner->syntaxes();
        if (showExtendedHelp) {
            for (const RunnerSyntax &syntax : syntaxes) {
                QueryMatch match(this);
                QString matchText;
                QString text = QLatin1String("<b>");
                const auto exampleQueries = syntax.exampleQueries();
                for (auto &query : exampleQueries) {
                    text.append(query.toHtmlEscaped());
                    text.append(QLatin1String("<br>"));
                }
                text.append(QLatin1String("</b>"));
                matchText.append(text);
                matchText.append(syntax.description().toHtmlEscaped());
                match.setText(matchText);
                match.setData(syntax.exampleQueries().constFirst());
                match.setMultiLine(true);
                match.setMatchCategory(runner->name());
                match.setIcon(runner->icon());
                matches << match;
            }
        } else {
            QueryMatch match(this);
            if (runner->metadata().value(QStringLiteral("X-Plasma-ShowDesciptionInOverview"), false)) {
                match.setText(runner->description());
            } else {
                match.setText(syntaxes.constFirst().exampleQueries().constFirst());
                match.setSubtext(runner->description());
            }
            match.setIcon(runner->icon());
            match.setType(QueryMatch::CompletionMatch);
            match.setData(QVariant::fromValue(runner->metadata()));
            if (!runner->metadata().value(QStringLiteral("X-KDE-ConfigModule")).isEmpty()) {
                match.setActions(m_actionList);
            }
            matches << match;
        }
    }
    context.addMatches(matches);
}

void HelpRunner::run(const RunnerContext &context, const QueryMatch &match)
{
    context.ignoreCurrentMatchForHistory();
    if (match.selectedAction()) {
        KIO::CommandLauncherJob *job = nullptr;
        const QStringList args{
            QStringLiteral("kcm_plasmasearch"),
            QStringLiteral("--args"),
            match.data().value<KPluginMetaData>().pluginId(),
        };
        job = new KIO::CommandLauncherJob(QStringLiteral("systemsettings5"), args);
        job->start();
    } else if (match.type() == QueryMatch::CompletionMatch) {
        const KPluginMetaData data = match.data().value<KPluginMetaData>();
        const QString completedRunnerName = QStringLiteral("?") + data.name();
        context.requestQueryStringUpdate(completedRunnerName, -1);
    } else {
        const QString query = match.data().toString();
        static const QRegularExpression placeholderRegex{QStringLiteral("<.+>$")};
        const int idx = query.indexOf(placeholderRegex);
        context.requestQueryStringUpdate(query, idx == -1 ? query.count() : idx);
    }
}

K_PLUGIN_CLASS_WITH_JSON(HelpRunner, "helprunner.json")

#include "helprunner.moc"
