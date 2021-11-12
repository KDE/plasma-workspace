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
#include <KRunner/QueryMatch>

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
    RemoteMatches matchInternal(const QString &searchTerm, const QString &type, const QString &category, QSet<QUrl> &foundUrls);
};
