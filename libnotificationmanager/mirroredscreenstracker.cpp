/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "mirroredscreenstracker_p.h"

#include <QRect>

#include <KScreen/ConfigMonitor>
#include <KScreen/GetConfigOperation>
#include <KScreen/Output>

#include "debug.h"

using namespace NotificationManager;

MirroredScreensTracker::MirroredScreensTracker()
    : QObject(nullptr)
{
    connect(new KScreen::GetConfigOperation(KScreen::GetConfigOperation::NoEDID),
            &KScreen::ConfigOperation::finished,
            this,
            [this](KScreen::ConfigOperation *op) {
                m_screenConfiguration = qobject_cast<KScreen::GetConfigOperation *>(op)->config();
                checkScreensMirrored();

                KScreen::ConfigMonitor::instance()->addConfig(m_screenConfiguration);
                connect(KScreen::ConfigMonitor::instance(), &KScreen::ConfigMonitor::configurationChanged, this, &MirroredScreensTracker::checkScreensMirrored);
            });
}

MirroredScreensTracker::~MirroredScreensTracker() = default;

MirroredScreensTracker::Ptr MirroredScreensTracker::createTracker()
{
    static QWeakPointer<MirroredScreensTracker> s_instance;
    if (!s_instance) {
        QSharedPointer<MirroredScreensTracker> ptr(new MirroredScreensTracker());
        s_instance = ptr.toWeakRef();
        return ptr;
    }
    return s_instance.toStrongRef();
}

bool MirroredScreensTracker::screensMirrored() const
{
    return m_screensMirrored;
}

void MirroredScreensTracker::setScreensMirrored(bool mirrored)
{
    if (m_screensMirrored != mirrored) {
        m_screensMirrored = mirrored;
        Q_EMIT screensMirroredChanged(mirrored);
    }
}

void MirroredScreensTracker::checkScreensMirrored()
{
    if (!m_screenConfiguration) {
        setScreensMirrored(false);
        return;
    }

    const auto outputs = m_screenConfiguration->outputs();
    for (const KScreen::OutputPtr &output : outputs) {
        if (!output->isConnected() || !output->isEnabled()) {
            continue;
        }

        for (const KScreen::OutputPtr &checkOutput : outputs) {
            if (checkOutput == output || !checkOutput->isConnected() || !checkOutput->isEnabled()) {
                continue;
            }

            if (output->geometry().contains(checkOutput->geometry()) || checkOutput->geometry().contains(output->geometry())) {
                qCDebug(NOTIFICATIONMANAGER) << "Screen geometry" << checkOutput->geometry() << "and" << output->geometry()
                                             << "are completely overlapping - considering them to be mirrored";
                setScreensMirrored(true);
                return;
            }
        }
    }

    setScreensMirrored(false);
}
