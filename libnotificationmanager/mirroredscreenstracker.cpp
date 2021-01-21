/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
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
        emit screensMirroredChanged(mirrored);
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

            if (output->geometry().intersects(checkOutput->geometry())) {
                qCDebug(NOTIFICATIONMANAGER) << "Screen geometry" << checkOutput->geometry() << "intersects" << output->geometry()
                                             << "- considering them to be mirrored";
                setScreensMirrored(true);
                return;
            }
        }
    }

    setScreensMirrored(false);
}
