/*
    Copyright (C) 2010  Milian Wolff <mail@milianw.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "gdbhighlighter.h"

#include <QTextDocument>

#include <KColorScheme>

GdbHighlighter::GdbHighlighter(QTextDocument* parent, const QList<BacktraceLine> & gdbLines)
    : QSyntaxHighlighter(parent)
{
    // setup line lookup
    int l = 0;
    foreach(const BacktraceLine& line, gdbLines) {
        lines.insert(l, line);
        l += line.toString().count('\n');
    }

    // setup formates
    KColorScheme scheme(QPalette::Active);

    crashFormat.setForeground(scheme.foreground(KColorScheme::NegativeText));
    nullptrFormat.setForeground(scheme.foreground(KColorScheme::NegativeText));
    nullptrFormat.setFontWeight(QFont::Bold);
    assertFormat = nullptrFormat;
    threadFormat.setForeground(scheme.foreground(KColorScheme::NeutralText));
    urlFormat.setForeground(scheme.foreground(KColorScheme::LinkText));
    funcFormat.setForeground(scheme.foreground(KColorScheme::VisitedText));
    funcFormat.setFontWeight(QFont::Bold);
    otheridFormat.setForeground(scheme.foreground(KColorScheme::PositiveText));
    crapFormat.setForeground(scheme.foreground(KColorScheme::InactiveText));
}

void GdbHighlighter::highlightBlock(const QString& text)
{
    int cur = 0;
    int next;
    int diff;
    const QRegExp hexptrPattern("0x[0-9a-f]+", Qt::CaseSensitive, QRegExp::RegExp2);
    int lineNr = currentBlock().firstLineNumber();
    while ( cur < text.length() ) {
        next = text.indexOf('\n', cur);
        if (next == -1) {
            next = text.length();
        }
        if (lineNr == 0) {
            // line that contains 'Application: ...'
            ++lineNr;
            cur = next;
            continue;
        }

        diff = next - cur;

        QString lineStr = text.mid(cur, diff).append('\n');
        // -1 since we skip the first line
        QMap< int, BacktraceLine >::iterator it = lines.lowerBound(lineNr - 1);
        Q_ASSERT(it != lines.end());
        // lowerbound would return the next higher item, even though we want the former one
        if (it.key() > lineNr - 1) {
            --it;
        }
        const BacktraceLine& line = it.value();

        if (line.type() == BacktraceLine::KCrash) {
            setFormat(cur, diff, crashFormat);
        } else if (line.type() == BacktraceLine::ThreadStart || line.type() == BacktraceLine::ThreadIndicator) {
            setFormat(cur, diff, threadFormat);
        } else if (line.type() == BacktraceLine::Crap) {
            setFormat(cur, diff, crapFormat);
        } else if (line.type() == BacktraceLine::StackFrame) {
            if (!line.fileName().isEmpty()) {
                int colonPos = line.fileName().lastIndexOf(':');
                setFormat(lineStr.indexOf(line.fileName()), colonPos == -1 ? line.fileName().length() : colonPos, urlFormat);
            }
            if (!line.libraryName().isEmpty()) {
                setFormat(lineStr.indexOf(line.libraryName()), line.libraryName().length(), urlFormat);
            }
            if (!line.functionName().isEmpty()) {
                int idx = lineStr.indexOf(line.functionName());
                if (idx != -1) {
                    // highlight Id::Id::Id::Func
                    // Id should have otheridFormat, :: no format and Func funcFormat
                    int i = idx;
                    int from = idx;
                    while (i < idx + line.functionName().length()) {
                        if (lineStr.at(i) == ':') {
                            setFormat(from, i - from, otheridFormat);
                            // skip ::
                            i += 2;
                            from = i;
                            continue;
                        } else if (lineStr.at(i) == '<' || lineStr.at(i) == '>') {
                            setFormat(from, i - from, otheridFormat);
                            ++i;
                            from = i;
                            continue;
                        }
                        ++i;
                    }
                    if (line.functionName() == "qFatal" || line.functionName() == "abort" || line.functionName() == "__assert_fail"
                        || line.functionName() == "*__GI___assert_fail" || line.functionName() == "*__GI_abort") {
                        setFormat(from, i - from, assertFormat);
                    } else {
                        setFormat(from, i - from, funcFormat);
                    }
                }
            }
            // highlight hexadecimal ptrs
            int idx = 0;
            while ((idx = hexptrPattern.indexIn(lineStr, idx)) != -1) {
                if (hexptrPattern.cap() == "0x0") {
                    setFormat(idx, hexptrPattern.matchedLength(), nullptrFormat);
                }
                idx += hexptrPattern.matchedLength();
            }
        }

        cur = next;
        ++lineNr;
    }
}
