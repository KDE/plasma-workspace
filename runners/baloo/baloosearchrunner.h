/*
    This file is part of the KDE Baloo Project
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#ifndef _BALOO_SEARCH_RUNNER_H_
#define _BALOO_SEARCH_RUNNER_H_

#include <QDBusContext>
#include <QDBusMessage>
#include <QObject>

#include "dbusutils_p.h"
#include <KRunner/QueryMatch>

class QTimer;

class SearchRunner : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit SearchRunner(QObject *parent = nullptr);
    ~SearchRunner() override;

    RemoteActions Actions();
    RemoteMatches Match(const QString &searchTerm);
    void Run(const QString &id, const QString &actionId);

private:
    void performMatch();
    RemoteMatches matchInternal(const QString &searchTerm, const QString &type, const QString &category, QSet<QUrl> &foundUrls);

    QDBusMessage m_lastRequest;
    QString m_searchTerm;
    QTimer *m_timer = nullptr;
};

#endif // _BALOO_SEARCH_RUNNER_H_
