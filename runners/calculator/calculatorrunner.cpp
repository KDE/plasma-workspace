/*
 *   Copyright (C) 2007 Barış Metin <baris@pardus.org.tr>
 *   Copyright (C) 2006 David Faure <faure@kde.org>
 *   Copyright (C) 2007 Richard Moore <rich@kde.org>
 *   Copyright (C) 2010 Matteo Agostinelli <agostinelli@gmail.com>
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

#include "calculatorrunner.h"

#ifdef ENABLE_QALCULATE
#include "qalculate_engine.h"
#else
#include <QJSEngine>
#include <QGuiApplication>
#include <QClipboard>
#endif

#include <QRegularExpression>
#include <QIcon>
#include <QDebug>

#include <KLocalizedString>
#include <krunner/querymatch.h>

K_EXPORT_PLASMA_RUNNER_WITH_JSON(CalculatorRunner, "plasma-runner-calculator.json")

CalculatorRunner::CalculatorRunner(QObject *parent, const QVariantList &args)
    : Plasma::AbstractRunner(parent, args)
{
    #ifdef ENABLE_QALCULATE
    m_engine = new QalculateEngine;
    setSpeed(SlowSpeed);
    #endif

    setObjectName(QStringLiteral("Calculator"));
    setIgnoredTypes(Plasma::RunnerContext::Directory | Plasma::RunnerContext::File |
                         Plasma::RunnerContext::NetworkLocation | Plasma::RunnerContext::Executable |
                         Plasma::RunnerContext::ShellCommand);

    QString description = i18n("Calculates the value of :q: when :q: is made up of numbers and "
                               "mathematical symbols such as +, -, /, * and ^.");
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), description));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral("=:q:"), description));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:="), description));

    addAction(QStringLiteral("copyToClipboard"), QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy to Clipboard"));
}

CalculatorRunner::~CalculatorRunner()
{
    #ifdef ENABLE_QALCULATE
    delete m_engine;
    #endif
}

void CalculatorRunner::powSubstitutions(QString &cmd)
{
    if (cmd.contains(QLatin1String("e+"), Qt::CaseInsensitive)) {
        cmd.replace(QLatin1String("e+"), QLatin1String("*10^"), Qt::CaseInsensitive);
    }

    if (cmd.contains(QLatin1String("e-"), Qt::CaseInsensitive)) {
        cmd.replace(QLatin1String("e-"), QLatin1String("*10^-"), Qt::CaseInsensitive);
    }

    // the below code is scary mainly because we have to honor priority
    // honor decimal numbers and parenthesis.
    while (cmd.contains(QLatin1Char('^'))) {
        int where = cmd.indexOf(QLatin1Char('^'));
        cmd.replace(where, 1, QLatin1Char(','));
        int preIndex = where - 1;
        int postIndex = where + 1;
        int count = 0;

        QChar decimalSymbol = QLocale().decimalPoint();
        //avoid out of range on weird commands
        preIndex = qMax(0, preIndex);
        postIndex = qMin(postIndex, cmd.length()-1);

        //go backwards looking for the beginning of the number or expression
        while (preIndex != 0) {
            QChar current = cmd.at(preIndex);
            QChar next = cmd.at(preIndex-1);
            //qDebug() << "index " << preIndex << " char " << current;
            if (current == QLatin1Char(')')) {
                count++;
            } else if (current == QLatin1Char('(')) {
                count--;
            } else {
                if (((next <= QLatin1Char('9') ) && (next >= QLatin1Char('0'))) || next == decimalSymbol) {
                    preIndex--;
                    continue;
                }
            }
            if (count == 0) {
                //check for functions
                if (!((next <= QLatin1Char('z') ) && (next >= QLatin1Char('a')))) {
                    break;
                }
            }
            preIndex--;
        }

       //go forwards looking for the end of the number or expression
        count = 0;
        while (postIndex != cmd.size() - 1) {
            QChar current=cmd.at(postIndex);
            QChar next=cmd.at(postIndex + 1);

            //check for functions
            if ((count == 0) && (current <= QLatin1Char('z')) && (current >= QLatin1Char('a'))) {
                postIndex++;
                continue;
            }

            if (current == QLatin1Char('(')) {
                count++;
            } else if (current == QLatin1Char(')')) {
                count--;
            } else {
                if (((next <= QLatin1Char('9') ) && (next >= QLatin1Char('0'))) || next == decimalSymbol) {
                    postIndex++;
                    continue;
                 }
            }
            if (count == 0) {
                break;
            }
            postIndex++;
        }

        preIndex = qMax(0, preIndex);
        postIndex = qMin(postIndex, cmd.length());

        cmd.insert(preIndex,QLatin1String("pow("));
        // +1 +4 == next position to the last number after we add 4 new characters pow(
        cmd.insert(postIndex + 1 + 4, QLatin1Char(')'));
        //qDebug() << "from" << preIndex << " to " << postIndex << " got: " << cmd;
    }
}

void CalculatorRunner::hexSubstitutions(QString& cmd)
{
    if (cmd.contains(QLatin1String("0x"))) {
        //Append +0 so that the calculator can serve also as a hex converter
        cmd.append(QLatin1String("+0"));
        bool ok;
        int pos = 0;
        QString hex;

        while (cmd.contains(QLatin1String("0x"))) {
            hex.clear();
            pos = cmd.indexOf(QLatin1String("0x"), pos);

            for (int q = 0; q < cmd.size(); q++) {//find end of hex number
                QChar current = cmd[pos+q+2];
                if (((current <= QLatin1Char('9') ) && (current >= QLatin1Char('0')))
                        || ((current <= QLatin1Char('F') ) && (current >= QLatin1Char('A')))
                        || ((current <= QLatin1Char('f') ) && (current >= QLatin1Char('a')))) { //Check if valid hex sign
                    hex[q] = current;
                } else {
                    break;
                }
            }
            cmd = cmd.replace(pos, 2+hex.length(), QString::number(hex.toInt(&ok,16))); //replace hex with decimal
        }
    }
}

void CalculatorRunner::userFriendlySubstitutions(QString& cmd)
{
    cmd.replace(QLocale().decimalPoint(), QLatin1Char('.'), Qt::CaseInsensitive);

    // the following substitutions are not needed with libqalculate
#ifndef ENABLE_QALCULATE
    hexSubstitutions(cmd);
    powSubstitutions(cmd);

    QRegularExpression re(QStringLiteral("(\\d+)and(\\d+)"));
    cmd.replace(re, QStringLiteral("\\1&\\2"));

    re.setPattern(QStringLiteral("(\\d+)or(\\d+)"));
    cmd.replace(re, QStringLiteral("\\1|\\2"));

    re.setPattern(QStringLiteral("(\\d+)xor(\\d+)"));
    cmd.replace(re, QStringLiteral("\\1^\\2"));
#endif
}


void CalculatorRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    QString cmd = term;

    //no meanless space between friendly guys: helps simplify code
    cmd = cmd.trimmed().remove(QLatin1Char(' '));

    if (cmd.length() < 3) {
        return;
    }

    if (cmd.toLower() == QLatin1String("universe") || cmd.toLower() == QLatin1String("life")) {
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::InformationalMatch);
        match.setIconName(QStringLiteral("accessories-calculator"));
        match.setText(QStringLiteral("42"));
        match.setData(QStringLiteral("42"));
        match.setId(term);
        context.addMatch(match);
        return;
    }

    bool toHex = cmd.startsWith(QLatin1String("hex="));
    bool startsWithEquals = !toHex && cmd[0] == QLatin1Char('=');

    userFriendlyMultiplication(cmd);

    if (toHex || startsWithEquals) {
        cmd.remove(0, cmd.indexOf(QLatin1Char('=')) + 1);
    } else if (cmd.endsWith(QLatin1Char('='))) {
        cmd.chop(1);
    } else {
        bool foundDigit = false;
        for (int i = 0; i < cmd.length(); ++i) {
            QChar c = cmd.at(i);
            if (c.isLetter()) {
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
    #ifndef ENABLE_QALCULATE
    //needed for accessing math functions like sin(),....
    cmd.replace(QRegularExpression(QStringLiteral("([a-zA-Z]+)")), QStringLiteral("Math.\\1"));
    #endif

    bool isApproximate = false;
    QString result = calculate(cmd, &isApproximate);
    if (!result.isEmpty() && result != cmd) {
        if (toHex) {
            result = QLatin1String("0x") + QString::number(result.toInt(), 16).toUpper();
        }

        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::InformationalMatch);
        match.setIconName(QStringLiteral("accessories-calculator"));
        match.setText(result);
        if (isApproximate) {
            match.setSubtext(i18nc("The result of the calculation is only an approximation", "Approximation"));
        }
        match.setData(result);
        match.setId(term);
        context.addMatch(match);
    }
}

QString CalculatorRunner::calculate(const QString& term, bool *isApproximate)
{
    #ifdef ENABLE_QALCULATE
    QString result;

    try {
        result = m_engine->evaluate(term, isApproximate);
    } catch(std::exception& e) {
        qDebug() << "qalculate error: " << e.what();
    }

    return result.replace(QLatin1Char('.'), QLocale().decimalPoint(), Qt::CaseInsensitive);
    #else
    Q_UNUSED(isApproximate);
    //qDebug() << "calculating" << term;
    QJSEngine eng;
    QJSValue result = eng.evaluate(QStringLiteral("var result = %1; result").arg(term));

    if (result.isError()) {
        return QString();
    }

    const QString resultString = result.toString();
    if (resultString.isEmpty()) {
        return QString();
    }

    if (!resultString.contains(QLatin1Char('.'))) {
        return resultString;
    }

    //ECMAScript has issues with the last digit in simple rational computations
    //This script rounds off the last digit; see bug 167986
    QString roundedResultString = eng.evaluate(QStringLiteral("var exponent = 14-(1+Math.floor(Math.log(Math.abs(result))/Math.log(10)));\
                                                var order=Math.pow(10,exponent);\
                                                (order > 0? Math.round(result*order)/order : 0)")).toString();

    roundedResultString.replace(QLatin1Char('.'), QLocale().decimalPoint(), Qt::CaseInsensitive);

    return roundedResultString;
    #endif
}

void CalculatorRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)
    if (match.selectedAction()) {
#ifdef ENABLE_QALCULATE
        m_engine->copyToClipboard();
#else
        QGuiApplication::clipboard()->setText(match.text());
#endif
    }
}

QList<QAction *> CalculatorRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    return actions().values();
}

QMimeData * CalculatorRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    //qDebug();
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
