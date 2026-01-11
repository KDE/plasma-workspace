/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>
    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QDBusContext>
#include <QDBusMessage>
#include <QObject>

#include "dbusutils_p.h"
#include <KRunner/Action>
#include <KRunner/QueryMatch>

class SearchRunner : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit SearchRunner(QObject *parent = nullptr);

    KRunner::Actions Actions();
    RemoteMatches Match(const QString &searchTerm);
    void Run(const QString &id, const QString &actionId);
    void SetActivationToken(const QString &token);
    QVariantMap Config();
    void Teardown();

private:
    RemoteMatches matchInternal(const QString &searchTerm, const QStringList &types, const QString &category, QSet<QUrl> &foundUrls);
    QString m_activationToken;
};
