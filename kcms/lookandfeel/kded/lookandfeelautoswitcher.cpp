/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "lookandfeelautoswitcher.h"
#include "idletimeout.h"
#include "lookandfeelautoswitcherstate.h"
#include "lookandfeelsettings.h"

#include <KPluginFactory>

#include <QDateTime>
#include <QDebug>
#include <QProcess>

K_PLUGIN_CLASS_WITH_JSON(LookAndFeelAutoSwitcher, "lookandfeelautoswitcher.json")

LookAndFeelAutoSwitcher::LookAndFeelAutoSwitcher(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
    , m_settings(std::make_unique<LookAndFeelSettings>())
    , m_state(std::make_unique<LookAndFeelAutoSwitcherState>())
    , m_configWatcher(KConfigWatcher::create(m_settings->sharedConfig()))
{
    connect(m_configWatcher.get(), &KConfigWatcher::configChanged, this, &LookAndFeelAutoSwitcher::onConfigChanged);

    reconfigure();
}

LookAndFeelAutoSwitcher::~LookAndFeelAutoSwitcher()
{
}

bool LookAndFeelAutoSwitcher::changesConfig(const KConfigGroup &group, const QByteArrayList &names) const
{
    if (group.name() == QLatin1String("KDE")) {
        const QByteArrayList keys{
            QByteArrayLiteral("AutomaticLookAndFeel"),
            QByteArrayLiteral("DefaultLightLookAndFeel"),
            QByteArrayLiteral("DefaultDarkLookAndFeel"),
        };
        return std::any_of(keys.begin(), keys.end(), [names](const QByteArray &name) {
            return names.contains(name);
        });
    }

    return false;
}

void LookAndFeelAutoSwitcher::onConfigChanged(const KConfigGroup &group, const QByteArrayList &names)
{
    m_settings->read();
    if (changesConfig(group, names)) {
        reconfigure();
    }
}

void LookAndFeelAutoSwitcher::reconfigure()
{
    if (!m_settings->automaticLookAndFeel()) {
        m_scheduleTimer.reset();
        m_scheduleProvider.reset();
        m_skewMonitor.reset();
    } else {
        if (!m_skewMonitor) {
            m_skewMonitor = std::make_unique<KSystemClockSkewNotifier>();
            connect(m_skewMonitor.get(), &KSystemClockSkewNotifier::skewed, this, &LookAndFeelAutoSwitcher::rescheduleAndUpdateNow);
            m_skewMonitor->setActive(true);
        }

        if (!m_scheduleTimer) {
            m_scheduleTimer = std::make_unique<QTimer>();
            m_scheduleTimer->setSingleShot(true);
            connect(m_scheduleTimer.get(), &QTimer::timeout, this, &LookAndFeelAutoSwitcher::rescheduleAndUpdateAuto);
        }

        if (!m_scheduleProvider) {
            m_scheduleProvider = std::make_unique<KDarkLightScheduleProvider>(m_state->serializedSchedule());
            connect(m_scheduleProvider.get(), &KDarkLightScheduleProvider::scheduleChanged, this, [this]() {
                m_state->setSerializedSchedule(m_scheduleProvider->state());
                m_state->save();

                rescheduleAndUpdateAuto();
            });
        }

        rescheduleAndUpdateNow();
    }
}

QString LookAndFeelAutoSwitcher::lookAndFeelAtDateTime(const QDateTime &dateTime) const
{
    switch (m_previousTransition->test(dateTime)) {
    case KDarkLightTransition::Upcoming:
    case KDarkLightTransition::InProgress:
        if (m_previousTransition->type() == KDarkLightTransition::Morning) {
            return m_settings->defaultDarkLookAndFeel();
        } else {
            return m_settings->defaultLightLookAndFeel();
        }
    case KDarkLightTransition::Passed:
        if (m_previousTransition->type() == KDarkLightTransition::Morning) {
            return m_settings->defaultLightLookAndFeel();
        } else {
            return m_settings->defaultDarkLookAndFeel();
        }
    }

    Q_UNREACHABLE();
}

void LookAndFeelAutoSwitcher::applyLookAndFeel(const QString &id)
{
    QProcess::startDetached(QStringLiteral("plasma-apply-lookandfeel"), QStringList({QStringLiteral("--keep-auto"), QStringLiteral("--apply"), id}));
}

void LookAndFeelAutoSwitcher::reschedule()
{
    const QDateTime now = QDateTime::currentDateTime();

    const KDarkLightSchedule schedule = m_scheduleProvider->schedule();
    m_previousTransition = schedule.previousTransition(now);
    m_nextTransition = schedule.nextTransition(now);

    QDateTime rescheduleDateTime;
    if (m_previousTransition->test(now) != KDarkLightTransition::Passed) {
        rescheduleDateTime = m_previousTransition->endDateTime();
    } else {
        rescheduleDateTime = m_nextTransition->endDateTime();
    }
    m_scheduleTimer->start(rescheduleDateTime - now);
}

void LookAndFeelAutoSwitcher::rescheduleAndUpdateAuto()
{
    if (m_settings->automaticLookAndFeelOnIdle()) {
        rescheduleAndUpdateIdle();
    } else {
        rescheduleAndUpdateNow();
    }
}

void LookAndFeelAutoSwitcher::rescheduleAndUpdateNow()
{
    reschedule();
    updateNow();
}

void LookAndFeelAutoSwitcher::rescheduleAndUpdateIdle()
{
    reschedule();
    updateIdle();
}

void LookAndFeelAutoSwitcher::updateNow()
{
    m_idleTimeout.reset();
    applyLookAndFeel(lookAndFeelAtDateTime(QDateTime::currentDateTime()));
}

void LookAndFeelAutoSwitcher::updateIdle()
{
    const std::chrono::seconds idleInterval(m_settings->automaticLookAndFeelIdleInterval());
    m_idleTimeout = std::make_unique<IdleTimeout>(idleInterval);
    connect(m_idleTimeout.get(), &IdleTimeout::timeout, this, [this]() {
        applyLookAndFeel(lookAndFeelAtDateTime(QDateTime::currentDateTime()));
        m_idleTimeout.reset();
    });
}

#include "lookandfeelautoswitcher.moc"
#include "moc_lookandfeelautoswitcher.cpp"
