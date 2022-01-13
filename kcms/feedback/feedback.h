/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include <KSharedConfig>

class FeedbackSettings;
class FeedbackData;

class Feedback : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(QJsonArray feedbackSources MEMBER m_feedbackSources NOTIFY feedbackSourcesChanged)
    Q_PROPERTY(QJsonArray audits READ audits CONSTANT)
    Q_PROPERTY(bool feedbackEnabled READ feedbackEnabled CONSTANT)
    Q_PROPERTY(FeedbackSettings *feedbackSettings READ feedbackSettings CONSTANT)

public:
    explicit Feedback(QObject *parent, const KPluginMetaData &data, const QVariantList &list = QVariantList());
    ~Feedback() override;

    bool feedbackEnabled() const;
    FeedbackSettings *feedbackSettings() const;

    QJsonArray audits() const;
    void programFinished(int exitCode);

Q_SIGNALS:
    void feedbackSourcesChanged();

private:
    QHash<int, QHash<QString, QJsonArray>> m_uses;
    QJsonArray m_feedbackSources;
    FeedbackData *m_data;
};
