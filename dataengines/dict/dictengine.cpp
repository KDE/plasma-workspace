/*
 *   Copyright (C) 2007 Thomas Georgiou <TAGeorgiou@gmail.com> and Jeff Cooper <weirdsox11@gmail.com>
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

#include "dictengine.h"
#include <iostream>

#include <QDebug>
#include <QTcpSocket>

#include <Plasma/DataContainer>

DictEngine::DictEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args)
    , m_tcpSocket(0)
{
    Q_UNUSED(args)
    m_serverName = QLatin1String("dict.org"); //In case we need to switch it later
    m_dictName = QLatin1String("wn"); //Default, good dictionary
}

DictEngine::~DictEngine()
{
}

void DictEngine::setDict(const QString &dict)
{
    m_dictName = dict;
}

void DictEngine::setServer(const QString &server)
{
    m_serverName = server;
}

static QString wnToHtml(const QString &word, QByteArray &text)
{
    Q_UNUSED(word)
    QList<QByteArray> splitText = text.split('\n');
    QString def;
    def += QLatin1String("<dl>\n");
    QRegExp linkRx(QStringLiteral("\\{(.*)\\}"));
    linkRx.setMinimal(true);

    bool isFirst=true;
    while (!splitText.empty()) {
        //150 n definitions retrieved - definitions follow
        //151 word database name - text follows
        //250 ok (optional timing information here)
        //552 No match
        QString currentLine = splitText.takeFirst();
        if (currentLine.startsWith(QLatin1String("151"))) {
            isFirst = true;
            continue;
        }

        if (currentLine.startsWith('.')) {
            def += QLatin1String("</dd>");
            continue;
        }

        if (!(currentLine.startsWith(QLatin1String("150"))
           || currentLine.startsWith(QLatin1String("151"))
           || currentLine.startsWith(QLatin1String("250"))
           || currentLine.startsWith(QLatin1String("552")))) {
            currentLine.replace(linkRx,QLatin1String("<a href=\"\\1\">\\1</a>"));

            if (isFirst) {
                def += "<dt><b>" + currentLine + "</b></dt>\n<dd>";
                isFirst = false;
                continue;
            } else {
                if (currentLine.contains(QRegExp(QStringLiteral("([1-9]{1,2}:)")))) {
                    def += QLatin1String("\n<br>\n");
                }
                currentLine.replace(QRegExp(QStringLiteral("^([\\s\\S]*[1-9]{1,2}:)")), QLatin1String("<b>\\1</b>"));
                def += currentLine;
                continue;
            }
        }

    }

    def += QLatin1String("</dl>");
    return def;
}

void DictEngine::getDefinition()
{
    m_tcpSocket->readAll();
    QByteArray ret;

    m_tcpSocket->write(QByteArray("DEFINE "));
    m_tcpSocket->write(m_dictName.toLatin1());
    m_tcpSocket->write(QByteArray(" \""));
    m_tcpSocket->write(m_currentWord.toUtf8());
    m_tcpSocket->write(QByteArray("\"\n"));
    m_tcpSocket->flush();

    while (!ret.contains("250") && !ret.contains("552") && !ret.contains("550")) {
        m_tcpSocket->waitForReadyRead();
        ret += m_tcpSocket->readAll();
    }

    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &DictEngine::socketClosed);
    m_tcpSocket->disconnectFromHost();
    //       setData(m_currentWord, m_dictName, ret);
    //       qWarning()<<ret;
    setData(m_currentWord, QStringLiteral("text"), wnToHtml(m_currentWord,ret));
}

void DictEngine::getDicts()
{
    QMap<QString, QString> theHash;
    m_tcpSocket->readAll();
    QByteArray ret;

    m_tcpSocket->write(QByteArray("SHOW DB\n"));;
    m_tcpSocket->flush();

    m_tcpSocket->waitForReadyRead();
    while (!ret.contains("250")) {
        m_tcpSocket->waitForReadyRead();
        ret += m_tcpSocket->readAll();
    }

    QList<QByteArray> retLines = ret.split('\n');
    QString tmp1, tmp2;

    while (!retLines.empty()) {
        QString curr(retLines.takeFirst());

        if (curr.startsWith(QLatin1String("554"))) {
            //TODO: What happens if no DB available?
            //TODO: Eventually there will be functionality to change the server...
            break;
        }

        // ignore status code and empty lines
        if (curr.startsWith(QLatin1String("250")) || curr.startsWith(QLatin1String("110"))
           || curr.isEmpty()) {
            continue;
        }

        if (!curr.startsWith('-') && !curr.startsWith('.')) {
            curr = curr.trimmed();
            tmp1 = curr.section(' ', 0, 0);
            tmp2 = curr.section(' ', 1);
  //          theHash.insert(tmp1, tmp2);
            //qDebug() << tmp1 + "  " + tmp2;
            setData(QStringLiteral("list-dictionaries"), tmp1, tmp2);
        }
    }

    m_tcpSocket->disconnectFromHost();
//    setData("list-dictionaries", "dictionaries", QByteArray(theHash);
}



void DictEngine::socketClosed()
{
    m_tcpSocket->deleteLater();
    m_tcpSocket = 0;
}

bool DictEngine::sourceRequestEvent(const QString &query)
{
    // FIXME: this is COMPLETELY broken .. it can only look up one query at a time!
    //        a DataContainer subclass that does the look up should probably be made
    if (m_currentQuery == query) {
        return false;
    }

    if (m_tcpSocket) {
        m_tcpSocket->abort(); //stop if lookup is in progress and new query is requested
        m_tcpSocket->deleteLater();
        m_tcpSocket = 0;
    }

    QStringList queryParts = query.split(':', QString::SkipEmptyParts);
    if (queryParts.isEmpty()) {
        return false;
    }

    m_currentWord = queryParts.last();
    m_currentQuery = query;

    //asked for a dictionary?
    if (queryParts.count() > 1) {
        setDict(queryParts[queryParts.count()-2]);
    //default to wordnet
    } else {
        setDict(QStringLiteral("wn"));
    }

    //asked for a server?
    if (queryParts.count() > 2) {
        setServer(queryParts[queryParts.count()-3]);
    //default to wordnet
    } else {
        setServer(QStringLiteral("dict.org"));
    }

    if (m_currentWord.simplified().isEmpty()) {
        setData(m_currentWord, m_dictName, QString());
    } else {
        setData(m_currentWord, m_dictName, QString());
        m_tcpSocket = new QTcpSocket(this);
        m_tcpSocket->abort();
        connect(m_tcpSocket, &QTcpSocket::disconnected, this, &DictEngine::socketClosed);

        if (m_currentWord == QLatin1String("list-dictionaries")) {
            connect(m_tcpSocket, &QTcpSocket::readyRead, this, &DictEngine::getDicts);
        } else {
            connect(m_tcpSocket, &QTcpSocket::readyRead, this, &DictEngine::getDefinition);
        }

        m_tcpSocket->connectToHost(m_serverName, 2628);
    }

    return true;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(dict, DictEngine , "plasma-dataengine-dict.json")

#include "dictengine.moc"
