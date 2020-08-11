/*
 * Copyright (C) 2019 David Edmundson <davidedmundson@kde.org>
 * Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#pragma once

#include <KQuickAddons/ManagedConfigModule>

#include <KSharedConfig>

class FeedbackSettings;

class Feedback : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(QJsonArray feedbackSources MEMBER m_feedbackSources NOTIFY feedbackSourcesChanged)
    Q_PROPERTY(bool feedbackEnabled READ feedbackEnabled CONSTANT)
    Q_PROPERTY(FeedbackSettings *feedbackSettings READ feedbackSettings CONSTANT)

    public:
        explicit Feedback(QObject *parent = nullptr, const QVariantList &list = QVariantList());
        ~Feedback() override;

        bool feedbackEnabled() const;
        FeedbackSettings *feedbackSettings() const;

        void programFinished(int exitCode);

    Q_SIGNALS:
        void feedbackSourcesChanged();

    private:
        QHash<int, QHash<QString, QJsonArray>> m_uses;
        QJsonArray m_feedbackSources;
        FeedbackSettings *m_feedbackSettings;
};
