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
#include <QScriptEngine>
#include <QGuiApplication>
#include <QClipboard>
#endif

#include <QIcon>
#include <QDebug>

#include <KLocalizedString>
#include <krunner/querymatch.h>

K_EXPORT_PLASMA_RUNNER(calculatorrunner, CalculatorRunner)

CalculatorRunner::CalculatorRunner( QObject* parent, const QVariantList &args )
    : Plasma::AbstractRunner(parent, args)
{
    Q_UNUSED(args)

    #ifdef ENABLE_QALCULATE
    m_engine = new QalculateEngine;
    setSpeed(SlowSpeed);
    #endif

    setObjectName( QStringLiteral("Calculator" ));
    setIgnoredTypes(Plasma::RunnerContext::Directory | Plasma::RunnerContext::File |
                         Plasma::RunnerContext::NetworkLocation | Plasma::RunnerContext::Executable |
                         Plasma::RunnerContext::ShellCommand);

    QString description = i18n("Calculates the value of :q: when :q: is made up of numbers and "
                               "mathematical symbols such as +, -, /, * and ^.");
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:"), description));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral("=:q:"), description));
    addSyntax(Plasma::RunnerSyntax(QStringLiteral(":q:="), description));
}

CalculatorRunner::~CalculatorRunner()
{
    #ifdef ENABLE_QALCULATE
    delete m_engine;
    #endif
}

void CalculatorRunner::powSubstitutions(QString& cmd)
{
    if (cmd.contains(QStringLiteral("e+"), Qt::CaseInsensitive)) {
        cmd = cmd.replace(QLatin1String("e+"), QLatin1String("*10^"), Qt::CaseInsensitive);
    }

    if (cmd.contains(QStringLiteral("e-"), Qt::CaseInsensitive)) {
        cmd = cmd.replace(QLatin1String("e-"), QLatin1String("*10^-"), Qt::CaseInsensitive);
    }

    // the below code is scary mainly because we have to honor priority
    // honor decimal numbers and parenthesis.
    while (cmd.contains('^')) {
        int where = cmd.indexOf('^');
        cmd = cmd.replace(where, 1, ',');
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
            if (current == ')') {
                count++;
            } else if (current == '(') {
                count--;
            } else {
                if (((next <= '9' ) && (next >= '0')) || next == decimalSymbol) {
                    preIndex--;
                    continue;
                }
            }
            if (count == 0) {
                //check for functions
                if (!((next <= 'z' ) && (next >= 'a'))) {
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
            if ((count == 0) && (current <= 'z') && (current >= 'a')) {
                postIndex++;
                continue;
            }

            if (current == '(') {
                count++;
            } else if (current == ')') {
                count--;
            } else {
                if (((next <= '9' ) && (next >= '0')) || next == decimalSymbol) {
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
        cmd.insert(postIndex + 1 + 4, ')');
        //qDebug() << "from" << preIndex << " to " << postIndex << " got: " << cmd;
    }
}

void CalculatorRunner::hexSubstitutions(QString& cmd)
{
    if (cmd.contains(QStringLiteral("0x"))) {
        //Append +0 so that the calculator can serve also as a hex converter
        cmd.append("+0");
        bool ok;
        int pos = 0;
        QString hex;

        while (cmd.contains(QStringLiteral("0x"))) {
            hex.clear();
            pos = cmd.indexOf(QStringLiteral("0x"), pos);

            for (int q = 0; q < cmd.size(); q++) {//find end of hex number
                QChar current = cmd[pos+q+2];
                if (((current <= '9' ) && (current >= '0')) || ((current <= 'F' ) && (current >= 'A')) || ((current <= 'f' ) && (current >= 'a'))) { //Check if valid hex sign
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
    if (cmd.contains(QLocale().decimalPoint(), Qt::CaseInsensitive)) {
         cmd=cmd.replace(QLocale().decimalPoint(), QChar('.'), Qt::CaseInsensitive);
    }

    // the following substitutions are not needed with libqalculate
    #ifndef ENABLE_QALCULATE
    hexSubstitutions(cmd);
    powSubstitutions(cmd);

    if (cmd.contains(QRegExp(QStringLiteral("\\d+and\\d+")))) {
         cmd = cmd.replace(QRegExp(QStringLiteral("(\\d+)and(\\d+)")), QStringLiteral("\\1&\\2"));
    }
    if (cmd.contains(QRegExp(QStringLiteral("\\d+or\\d+")))) {
         cmd = cmd.replace(QRegExp(QStringLiteral("(\\d+)or(\\d+)")), QStringLiteral("\\1|\\2"));
    }
    if (cmd.contains(QRegExp(QStringLiteral("\\d+xor\\d+")))) {
         cmd = cmd.replace(QRegExp(QStringLiteral("(\\d+)xor(\\d+)")), QStringLiteral("\\1^\\2"));
    }
    #endif
}


void CalculatorRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    QString cmd = term;

    //no meanless space between friendly guys: helps simplify code
    cmd = cmd.trimmed().remove(' ');

    if (cmd.length() < 3) {
        return;
    }

    if (cmd.toLower() == QLatin1String("universe") || cmd.toLower() == QLatin1String("life")) {
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::InformationalMatch);
        match.setIconName(QStringLiteral("accessories-calculator"));
        match.setText(QStringLiteral("42"));
        match.setData("42");
        match.setId(term);
        context.addMatch(match);
        return;
    }

    bool toHex = cmd.startsWith(QLatin1String("hex="));
    bool startsWithEquals = !toHex && cmd[0] == '=';

    if (toHex || startsWithEquals) {
        cmd.remove(0, cmd.indexOf('=') + 1);
    } else if (cmd.endsWith('=')) {
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
    cmd.replace(QRegExp(QStringLiteral("([a-zA-Z]+)")), QStringLiteral("Math.\\1")); //needed for accessing math funktions like sin(),....
    #endif

    bool isApproximate = false;
    QString result = calculate(cmd, &isApproximate);
    if (!result.isEmpty() && result != cmd) {
        if (toHex) {
            result = "0x" + QString::number(result.toInt(), 16).toUpper();
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

    return result.replace('.', QLocale().decimalPoint(), Qt::CaseInsensitive);
    #else
    //qDebug() << "calculating" << term;
    QScriptEngine eng;
    QScriptValue result = eng.evaluate(" var result ="+term+"; result");

    if (result.isError()) {
        return QString();
    }

    const QString resultString = result.toString();
    if (resultString.isEmpty()) {
        return QString();
    }

    if (!resultString.contains('.')) {
        return resultString;
    }

    //ECMAScript has issues with the last digit in simple rational computations
    //This script rounds off the last digit; see bug 167986
    QString roundedResultString = eng.evaluate(QStringLiteral("var exponent = 14-(1+Math.floor(Math.log(Math.abs(result))/Math.log(10)));\
                                                var order=Math.pow(10,exponent);\
                                                (order > 0? Math.round(result*order)/order : 0)")).toString();

    roundedResultString.replace('.', QLocale().decimalPoint(), Qt::CaseInsensitive);

    return roundedResultString;
    #endif
}

void CalculatorRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context);
#ifdef ENABLE_QALCULATE
    if (match.selectedAction()) {
        m_engine->copyToClipboard();
    }
#else
    QGuiApplication::clipboard()->setText(match.text());
#endif
}

QList<QAction *> CalculatorRunner::actionsForMatch(const Plasma::QueryMatch &match)
{
    Q_UNUSED(match)

    const QString copyToClipboard = QStringLiteral("copyToClipboard");

    if (!action(copyToClipboard)) {
        (addAction(copyToClipboard, QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy to Clipboard")));
    }

    return {action(copyToClipboard)};
}

QMimeData * CalculatorRunner::mimeDataForMatch(const Plasma::QueryMatch &match)
{
    //qDebug();
    QMimeData *result = new QMimeData();
    result->setText(match.text());
    return result;
}

#include "calculatorrunner.moc"
