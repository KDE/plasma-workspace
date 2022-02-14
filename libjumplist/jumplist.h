/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QHash>
#include <QIcon>
#include <QObject>
#include <QVariantList>

#include "jumplist_export.h"

class QAction;

namespace JumpList
{

class JumpListBackendPrivate;

class JUMPLIST_EXPORT JumpListBackend : public QObject
{
    Q_OBJECT

public:
    explicit JumpListBackend(QObject *parent = nullptr);
    ~JumpListBackend() override;

    Q_INVOKABLE void loadJumpList(const QUrl &launcherUrl, bool showAllPlaces) const;

    Q_INVOKABLE QVariantList actions(const QUrl &launcherUrl, QObject *parent) const;
    Q_INVOKABLE QVariantList places(const QUrl &launcherUrl, QObject *parent) const;
    Q_INVOKABLE QVariantList recentDocuments(const QUrl &launcherUrl, QObject *parent) const;

public Q_SLOTS:
    void handleRecentDocumentAction() const;
    void showAllPlaces(const QUrl &launcherUrl) const;

Q_SIGNALS:
    void actionsChanged(const QUrl &launcherUrl);
    void placesChanged(const QUrl &launcherUrl);
    void recentDocumentsChanged(const QUrl &launcherUrl);
    void listReady(const QUrl &launcherUrl);

private:
    JumpListBackendPrivate *d;
};

}
