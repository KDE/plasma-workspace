/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QDBusContext>
#include <QDBusObjectPath>
#include <QObject>

class QTimer;
class PromptContext;

class KSecretPrompter : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    using Id = QPair<QString, QString>;
    KSecretPrompter(QObject *parent = nullptr);

public Q_SLOTS:
    void UnlockCollectionPrompt(const QDBusObjectPath &path, const QString &windowId, const QString &activationToken, const QString &collectionName);
    void CreateCollectionPrompt(const QDBusObjectPath &path, const QString &windowId, const QString &activationToken, const QString &collectionName);

private Q_SLOTS:
    void onRetryRequest(const QString &message);
    void onDismissRequest();

private:
    void Prompt(const QDBusObjectPath &path, const QString &title, const QString &prompt, const QString &windowId);
    void dropPrompt(const Id &id);
    void maybeQuit();
    QHash<Id, std::shared_ptr<PromptContext>> m_activePrompts;
    QTimer *m_quitTimer;
};
