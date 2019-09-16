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

#include <KQuickAddons/ConfigModule>

#include <KSharedConfig>

class Feedback : public KQuickAddons::ConfigModule
{
    Q_OBJECT

    Q_PROPERTY(bool feedbackEnabled READ feedbackEnabled WRITE setFeedbackEnabled NOTIFY feedbackEnabledChanged)
    Q_PROPERTY(int plasmaFeedbackLevel READ plasmaFeedbackLevel WRITE setPlasmaFeedbackLevel NOTIFY plasmaFeedbackLevelChanged)

    public:
        explicit Feedback(QObject* parent = nullptr, const QVariantList &list = QVariantList());
        ~Feedback() override;

        bool feedbackEnabled() const { return m_feedbackEnabled; }
        int plasmaFeedbackLevel() const { return m_plasmaFeedbackLevel; }

        void setFeedbackEnabled(bool feedbackEnabled) {
            if (feedbackEnabled != m_feedbackEnabled) {
                m_feedbackEnabled = feedbackEnabled;
                Q_EMIT feedbackEnabledChanged(feedbackEnabled);
            }
        }

        void setPlasmaFeedbackLevel(int plasmaFeedbackLevel) {
            if (plasmaFeedbackLevel != m_plasmaFeedbackLevel) {
                m_plasmaFeedbackLevel = plasmaFeedbackLevel;
                Q_EMIT plasmaFeedbackLevelChanged(plasmaFeedbackLevel);
            }
        }

    public Q_SLOTS:
        void load() override;
        void save() override;
        void defaults() override;

    Q_SIGNALS:
        void feedbackEnabledChanged(bool feedbackEnabled) const;
        void plasmaFeedbackLevelChanged(bool plasmaFeedbackLevel);

    private:
        KSharedConfig::Ptr m_globalConfig;
        KSharedConfig::Ptr m_plasmaConfig;
        bool m_feedbackEnabled = false;
        int m_plasmaFeedbackLevel = 0;
};
