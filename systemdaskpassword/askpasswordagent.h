/*
    SPDX-FileCopyrightText: 2025 Daan De Meyer <daan.j.demeyer@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>

#include <sys/types.h>
#include <utility>

class KDirWatch;
class KPasswordDialog;

class AskPasswordAgent : public QObject
{
    Q_OBJECT

public:
    explicit AskPasswordAgent(const QString &directory, QObject *parent = nullptr);
    ~AskPasswordAgent() override;

private:
    using FileIdentity = std::pair<dev_t, ino_t>;

    void scanDirectory();
    void processRequest(const QString &filePath);
    void finishRequest();
    void respond(const QString &password, bool success);
    int sendResponse(const QString &password, bool success);

    QString m_directory;
    KDirWatch *m_watcher;
    FileIdentity m_directoryIdentity{};
    QHash<QString, FileIdentity> m_seen;
    QString m_requestPath;
    QString m_socketPath;
    QObject *m_request = nullptr;
    QPointer<KPasswordDialog> m_dialog;
};
