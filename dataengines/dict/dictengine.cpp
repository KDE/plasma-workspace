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
#include <QRegularExpression>
#include <QTcpSocket>
#include <QUrl>
#include <KLocalizedString>

#include <Plasma/DataContainer>

DictEngine::DictEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args)
    , m_tcpSocket(nullptr)
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
    QList<QByteArray> splitText = text.split('\n');
    QString def;
    def += QLatin1String("<dl>\n");
    static QRegularExpression linkRx(QStringLiteral("{(.*?)}"));

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

        if (currentLine.startsWith("552")) {
            return i18n("No match found for %1", word);
        }

        if (!(currentLine.startsWith(QLatin1String("150"))
           || currentLine.startsWith(QLatin1String("151"))
           || currentLine.startsWith(QLatin1String("250")))) {
            // Handle links
            int offset = 0;
            QRegularExpressionMatchIterator it = linkRx.globalMatch(currentLine);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QUrl url;
                url.setScheme("dict");
                url.setPath(match.captured(1));
                const QString linkText = QStringLiteral("<a href=\"%1\">%2</a>").arg(url.toString(), match.captured(1));
                currentLine.replace(match.capturedStart(0) + offset, match.capturedLength(0), linkText);
                offset += linkText.length() - match.capturedLength(0);
            }

            if (isFirst) {
                def += "<dt><b>" + currentLine + "</b></dt>\n<dd>";
                isFirst = false;
                continue;
            } else {
                static QRegularExpression newLineRx(QStringLiteral("([1-9]{1,2}:)"));
                if (currentLine.contains(newLineRx)) {
                    def += QLatin1String("\n<br>\n");
                }
                static QRegularExpression makeMeBoldRx(QStringLiteral("^([\\s\\S]*[1-9]{1,2}:)"));
                currentLine.replace(makeMeBoldRx, QLatin1String("<b>\\1</b>"));
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


    const QByteArray command = QByteArray("DEFINE ") + m_dictName.toLatin1() + " \"" + m_currentWord.toUtf8() + "\"\n";
    //qDebug() << command;
    m_tcpSocket->write(command);
    m_tcpSocket->flush();

    while (!ret.contains("250") && !ret.contains("552") && !ret.contains("550")) {
        m_tcpSocket->waitForReadyRead();
        ret += m_tcpSocket->readAll();
    }

    m_tcpSocket->disconnectFromHost();
    const QString html = wnToHtml(m_currentWord, ret);
    // setData(m_currentQuery, m_dictName, html);
    setData(m_currentQuery, QStringLiteral("text"), html);
}

void DictEngine::getDicts()
{
    m_tcpSocket->readAll();
    QByteArray ret;

    m_tcpSocket->write(QByteArray("SHOW DB\n"));
    m_tcpSocket->flush();

    m_tcpSocket->waitForReadyRead();
    while (!ret.contains("250")) {
        m_tcpSocket->waitForReadyRead();
        ret += m_tcpSocket->readAll();
    }

    QVariantMap *availableDicts = new QVariantMap;
    const QList<QByteArray> retLines = ret.split('\n');
    for (const QByteArray &curr : retLines) {
        if (curr.startsWith("554")) {
            //TODO: What happens if no DB available?
            //TODO: Eventually there will be functionality to change the server...
            break;
        }

        // ignore status code and empty lines
        if (curr.startsWith("250") || curr.startsWith("110")
           || curr.isEmpty()) {
            continue;
        }

        if (!curr.startsWith('-') && !curr.startsWith('.')) {
            const QString line = QString::fromUtf8(curr).trimmed();
            const QString id = line.section(' ', 0, 0);
            QString description = line.section(' ', 1);
            if (description.startsWith('"') && description.endsWith('"')) {
                description.remove(0, 1);
                description.chop(1);
            }
            setData(QStringLiteral("list-dictionaries"), id, description); // this is additive
            availableDicts->insert(id, description);
        }
    }
    m_availableDictsCache.insert(m_serverName, availableDicts);

    m_tcpSocket->disconnectFromHost();
}


void DictEngine::socketClosed()
{
    if (m_tcpSocket) {
        m_tcpSocket->deleteLater();
    }
    m_tcpSocket = nullptr;
}

bool DictEngine::sourceRequestEvent(const QString &query)
{
    // FIXME: this is COMPLETELY broken .. it can only look up one query at a time!
    //        a DataContainer subclass that does the look up should probably be made
    if (m_tcpSocket) {
        m_tcpSocket->abort(); //stop if lookup is in progress and new query is requested
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
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
        setData(m_currentQuery, m_dictName, QString());
    } else {
        if (m_currentWord == QLatin1String("list-dictionaries")) {
            // Use cache if available
            QVariantMap *dicts = m_availableDictsCache.object(m_serverName);
            if (dicts) {
                for (auto it = dicts->constBegin(); it != dicts->constEnd(); ++it) {
                    setData(m_currentQuery, it.key(), it.value());
                }
                return true;
            }
        }

        // We need to do this in order to create the DataContainer immediately in DataEngine
        // so it can connect to updates. Not sure why DataEnginePrivate::requestSource
        // doesn't create the DataContainer when sourceRequestEvent returns true, by doing
        // source(sourceName) instead of source(sourceName, false), but well, I'm too scared to change that.
        setData(m_currentQuery, QVariant());

        m_tcpSocket = new QTcpSocket(this);
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
