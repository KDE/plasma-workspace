/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfigWatcher>
#include <KDEDModule>
#include <KDarkLightScheduleProvider>
#include <KSystemClockSkewNotifier>

#include <QTimer>

class IdleTimeout;
class Settings;
class State;

enum class LookAndFeelVariant {
    Dark,
    Light,
};

class AutomaticLookAndFeelModule : public KDEDModule
{
    Q_OBJECT

public:
    explicit AutomaticLookAndFeelModule(QObject *parent = nullptr, const QList<QVariant> &arguments = {});
    ~AutomaticLookAndFeelModule() override;

private:
    bool changesConfig(const KConfigGroup &group, const QByteArrayList &names) const;
    void onConfigChanged(const KConfigGroup &group, const QByteArrayList &names);

    void reconfigure();
    void rescheduleAndUpdateAuto();
    void rescheduleAndUpdateNow();
    void rescheduleAndUpdateIdle();
    void reschedule();
    void updateNow();
    void updateIdle();

    QString lookAndFeelAtDateTime(const QDateTime &dateTime) const;
    void applyLookAndFeel(const QString &id);

    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<State> m_state;
    KConfigWatcher::Ptr m_configWatcher;

    std::unique_ptr<KDarkLightScheduleProvider> m_scheduleProvider;
    std::unique_ptr<KSystemClockSkewNotifier> m_skewMonitor;
    std::unique_ptr<QTimer> m_scheduleTimer;
    std::unique_ptr<IdleTimeout> m_idleTimeout;

    std::optional<KDarkLightTransition> m_previousTransition;
    std::optional<KDarkLightTransition> m_nextTransition;
};
