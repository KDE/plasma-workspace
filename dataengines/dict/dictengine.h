/*
    SPDX-FileCopyrightText: 2007 Jeff Cooper <weirdsox11@gmail.com>
    SPDX-FileCopyrightText: 2007 Thomas Georgiou <TAGeorgiou@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once
#include <Plasma/DataEngine>
#include <QCache>
#include <QMap>
#include <QVariantMap>
class QTcpSocket;

/**
 * This class evaluates the basic expressions given in the interface.
 */

class DictEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    DictEngine(QObject *parent, const QVariantList &args);
    ~DictEngine() override;

protected:
    bool sourceRequestEvent(const QString &word) override;

private Q_SLOTS:
    void getDefinition();
    void socketClosed();
    void getDicts();

private:
    void setDict(const QString &dict);
    void setServer(const QString &server);

    QHash<QString, QString> m_dictNameToDictCode;
    QTcpSocket *m_tcpSocket;
    QString m_currentWord;
    QString m_currentQuery;
    QString m_dictName;
    QString m_serverName;
    QCache<QString, QVariantMap> m_availableDictsCache;
};
