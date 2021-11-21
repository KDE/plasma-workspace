/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "helprunner.h"

#include <KLocalizedString>

HelpRunner::HelpRunner(QObject *parent, const KPluginMetaData &pluginMetaData, const QVariantList &args)
    : AbstractRunner(parent, pluginMetaData, args)
{
    setTriggerWords({i18nc("this is a runner keyword", "help"), QStringLiteral("?")});
    m_manager = qobject_cast<RunnerManager *>(parent);
    Q_ASSERT(m_manager);
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
                matchText.append(syntax.exampleQueries().join(QStringLiteral("\n")));
                matchText.append(QLatin1String("\n"));
                matchText.append(syntax.description());
                match.setText(matchText);
                match.setData(syntax.exampleQueries().constFirst());
                match.setMultiLine(true);
                match.setMatchCategory(runner->name());
                match.setIcon(runner->icon());
                matches << match;
            }
        } else {
            QueryMatch match(this);
            match.setText(syntaxes.constFirst().exampleQueries().constFirst());
            match.setIcon(runner->icon());
            match.setSubtext(runner->description());
            match.setType(QueryMatch::CompletionMatch);
            match.setData(QVariant(QStringLiteral("?") + runner->name()));
            matches << match;
        }
    }
    context.addMatches(matches);
}

void HelpRunner::run(const RunnerContext &context, const QueryMatch &match)
{
    context.ignoreCurrentMatchForHistory();
    if (match.type() == QueryMatch::CompletionMatch) {
        const QString completedRunnerName = match.data().toString();
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
