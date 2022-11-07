/*
    SPDX-FileCopyrightText: 2007 Teemu Rytilahti <tpr@iki.fi>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <KServiceAction>

class WebshortcutRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    WebshortcutRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~WebshortcutRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match) override;

private Q_SLOTS:
    void loadSyntaxes();
    void configurePrivateBrowsingActions();

private:
    Plasma::QueryMatch m_match;
    bool m_filterBeforeRun;

    QChar m_delimiter;
    QString m_lastFailedKey;
    QString m_lastKey;
    QString m_lastProvider;
    QRegularExpression m_regex;

    KServiceAction m_privateAction;

    Plasma::RunnerContext m_lastUsedContext;
    QString m_defaultKey;
};
