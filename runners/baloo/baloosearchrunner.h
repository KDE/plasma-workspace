/*
 * This file is part of the KDE Baloo Project
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 * Copyright (C) 2017 David Edmundson <davidedmundson@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
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
