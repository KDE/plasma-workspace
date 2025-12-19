/*
    SPDX-FileCopyrightText: 2025 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "ksecretprompter.h"
#include <KPasswordDialog>
#include <QDBusServiceWatcher>
#include <QObject>

class PromptContext : public QObject
{
    Q_OBJECT
public:
    PromptContext(const QString &callerAddress, const QDBusObjectPath &path, QObject *parent = nullptr);
    ~PromptContext() override;

    Q_DISABLE_COPY_MOVE(PromptContext)

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] KSecretPrompter::Id id() const;
    [[nodiscard]] QString callerAddress() const;

    void setWidget(const std::shared_ptr<KPasswordDialog> &widget);

    void retry(const QString &message);

private:
    bool m_valid = false;
    KSecretPrompter::Id m_id;
    QDBusObjectPath m_path;
    std::shared_ptr<QDBusServiceWatcher> m_watcher;
    std::shared_ptr<KPasswordDialog> m_promptWidget;
};
