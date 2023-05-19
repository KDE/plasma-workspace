/*
    SPDX-FileCopyrightText: 2007 Barış Metin <baris@pardus.org.tr>
    SPDX-FileCopyrightText: 2006 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Richard Moore <rich@kde.org>
    SPDX-FileCopyrightText: 2010 Matteo Agostinelli <agostinelli@gmail.com>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "calculatorrunner.h"

#include "qalculate_engine.h"

#include <QDebug>
#include <QIcon>
#include <QMutex>
#include <QRegularExpression>

#include <KLocalizedString>
#include <krunner/querymatch.h>

K_PLUGIN_CLASS_WITH_JSON(CalculatorRunner, "plasma-runner-calculator.json")

static QMutex s_initMutex;

CalculatorRunner::CalculatorRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    setObjectName(QStringLiteral("Calculator"));

    QString description = i18n(
        "Calculates the value of :q: when :q: is made up of numbers and "
        "mathematical symbols such as +, -, /, *, ! and ^.");
    addSyntax(QStringLiteral(":q:"), description);
    addSyntax(QStringLiteral("=:q:"), description);
    addSyntax(QStringLiteral(":q:="), description);
    addSyntax(QStringLiteral("sqrt(4)"), i18n("Enter a common math function"));

    m_actions = {new QAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy to Clipboard"), this)};
    setMinLetterCount(2);
}

CalculatorRunner::~CalculatorRunner()
{
}

void CalculatorRunner::userFriendlySubstitutions(QString &cmd)
{
    if (QLocale().decimalPoint() != QLatin1Char('.')) {
        cmd.replace(QLocale().decimalPoint(), QLatin1String("."), Qt::CaseInsensitive);
    } else if (!cmd.contains(QLatin1Char('[')) && !cmd.contains(QLatin1Char(']'))) {
        // If we are sure that the user does not want to use vectors we can replace this char
        // Especially when switching between locales that use a different decimal separator
        // this ensures that the results are valid, see BUG: 406388
        cmd.replace(QLatin1Char(','), QLatin1Char('.'), Qt::CaseInsensitive);
    }
}

void CalculatorRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    QString cmd = term;

    // no meanless space between friendly guys: helps simplify code
    cmd = cmd.trimmed().remove(QLatin1Char(' '));

    if (cmd.length() < 2) {
        return;
    }

    if (cmd.compare(QLatin1String("universe"), Qt::CaseInsensitive) == 0 || cmd.compare(QLatin1String("life"), Qt::CaseInsensitive) == 0) {
        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::PossibleMatch);
        match.setIconName(QStringLiteral("accessories-calculator"));
        match.setText(QStringLiteral("42"));
        match.setData(QStringLiteral("42"));
        match.setId(term);
        context.addMatch(match);
        return;
    }

    int base = 10;
    QString customBase;

    bool foundPrefix = false; // is there `=` or `hex=` or other base prefix in cmd

    int equalSignPosition = cmd.indexOf(QLatin1Char('='));
    if (equalSignPosition != -1 && equalSignPosition != cmd.length() - 1) {
        foundPrefix = QalculateEngine::findPrefix(cmd.left(equalSignPosition), &base, &customBase);
    }

    const static QRegularExpression hexRegex(QStringLiteral("0x[0-9a-f]+"), QRegularExpression::CaseInsensitiveOption);
    const bool parseHex = cmd.contains(hexRegex);
    if (!parseHex) {
        userFriendlyMultiplication(cmd);
    }

    const static QRegularExpression functionName(QStringLiteral("^([a-zA-Z]+)\\(.+\\)"));
    if (foundPrefix) {
        cmd.remove(0, cmd.indexOf(QLatin1Char('=')) + 1);
    } else if (cmd.endsWith(QLatin1Char('='))) {
        cmd.chop(1);
    } else if (auto match = functionName.match(cmd); match.hasMatch()) { // BUG: 467418
        {
            QMutexLocker lock(&s_initMutex);
            if (!m_engine) {
                m_engine = std::make_unique<QalculateEngine>();
            }
        }
        const QString functionName = match.captured(1);
        if (!m_engine->isKnownFunction(functionName)) {
            return;
        }

    } else if (!parseHex) {
        bool foundDigit = false;
        for (int i = 0; i < cmd.length(); ++i) {
            QChar c = cmd.at(i);
            if (c.isLetter() && c != QLatin1Char('!')) {
                // not just numbers and symbols, so we return
                return;
            }
            if (c.isDigit()) {
                foundDigit = true;
            }
        }
        if (!foundDigit) {
            return;
        }
    }

    if (cmd.isEmpty()) {
        return;
    }

    userFriendlySubstitutions(cmd);

    bool isApproximate = false;
    QString result = calculate(cmd, &isApproximate, base, customBase);
    if (!result.isEmpty() && (foundPrefix || result != cmd)) {
        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::HelperMatch);
        match.setIconName(QStringLiteral("accessories-calculator"));
        match.setText(result);
        if (isApproximate) {
            match.setSubtext(i18nc("The result of the calculation is only an approximation", "Approximation"));
        }
        match.setData(result);
        match.setId(term);
        match.setActions(m_actions);
        context.addMatch(match);
    }
}

QString CalculatorRunner::calculate(const QString &term, bool *isApproximate, int base, const QString &customBase)
{
    {
        QMutexLocker lock(&s_initMutex);
        if (!m_engine) {
            m_engine = std::make_unique<QalculateEngine>();
        }
    }

    QString result;
    try {
        result = m_engine->evaluate(term, isApproximate, base, customBase);
    } catch (std::exception &e) {
        qDebug() << "qalculate error: " << e.what();
    }

    return result.replace(QLatin1Char('.'), QLocale().decimalPoint(), Qt::CaseInsensitive);
}

void CalculatorRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    if (match.selectedAction()) {
        m_engine->copyToClipboard();
    } else {
        context.requestQueryStringUpdate(match.text(), match.text().length());
    }
}

QMimeData *CalculatorRunner::mimeDataForMatch(const KRunner::QueryMatch &match)
{
    QMimeData *result = new QMimeData();
    result->setText(match.text());
    return result;
}

void CalculatorRunner::userFriendlyMultiplication(QString &cmd)
{
    // convert multiplication sign to *
    cmd.replace(QChar(U'\u00D7'), QChar('*'));

    for (int i = 0; i < cmd.length(); ++i) {
        if (i == 0 || i == cmd.length() - 1) {
            continue;
        }
        const QChar prev = cmd.at(i - 1);
        const QChar current = cmd.at(i);
        const QChar next = cmd.at(i + 1);
        if (current == QLatin1Char('x')) {
            if (prev.isDigit() && (next.isDigit() || next == QLatin1Char(',') || next == QLatin1Char('.'))) {
                cmd[i] = '*';
            }
        }
    }
}

#include "calculatorrunner.moc"
