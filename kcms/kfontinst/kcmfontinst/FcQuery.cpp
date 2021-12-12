/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FcQuery.h"
#include "Fc.h"
#include <QProcess>
#include <stdio.h>

namespace KFI
{
// key: 0(i)(s)
static int getInt(const QString &str)
{
    int rv = KFI_NULL_SETTING, start = str.lastIndexOf(':') + 1, end = str.lastIndexOf("(i)(s)");

    if (end > start) {
        rv = str.mid(start, end - start).trimmed().toInt();
    }

    return rv;
}

CFcQuery::~CFcQuery()
{
}

void CFcQuery::run(const QString &query)
{
    QStringList args;

    m_file = m_font = QString();
    m_buffer = QByteArray();

    if (m_proc) {
        m_proc->kill();
    } else {
        m_proc = new QProcess(this);
    }

    args << "-v" << query;

    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(procExited()));
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &CFcQuery::data);

    m_proc->start("fc-match", args);
}

void CFcQuery::procExited()
{
    QString family;
    int weight(KFI_NULL_SETTING), slant(KFI_NULL_SETTING), width(KFI_NULL_SETTING);
    QStringList results(QString::fromUtf8(m_buffer, m_buffer.length()).split(QLatin1Char('\n')));

    if (!results.isEmpty()) {
        QStringList::ConstIterator it(results.begin()), end(results.end());

        for (; it != end; ++it) {
            QString line((*it).trimmed());

            if (0 == line.indexOf("file:")) // file: "Wibble"(s)
            {
                int endPos = line.indexOf("\"(s)");

                if (-1 != endPos) {
                    m_file = line.mid(7, endPos - 7);
                }
            } else if (0 == line.indexOf("family:")) // family: "Wibble"(s)
            {
                int endPos = line.indexOf("\"(s)");

                if (-1 != endPos) {
                    family = line.mid(9, endPos - 9);
                }
            } else if (0 == line.indexOf("slant:")) { // slant: 0(i)(s)
                slant = getInt(line);
            } else if (0 == line.indexOf("weight:")) { // weight: 0(i)(s)
                weight = getInt(line);
            } else if (0 == line.indexOf("width:")) { // width: 0(i)(s)
                width = getInt(line);
            }
        }
    }

    if (!family.isEmpty()) {
        m_font = FC::createName(family, weight, width, slant);
    }

    Q_EMIT finished();
}

void CFcQuery::data()
{
    m_buffer += m_proc->readAllStandardOutput();
}

}
