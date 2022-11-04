/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "mirroredscreenstracker_p.h"

#include <QGuiApplication>
#include <QRect>
#include <QScreen>

#include "debug.h"

using namespace NotificationManager;

MirroredScreensTracker::MirroredScreensTracker()
    : QObject(nullptr)
{
    checkScreensMirrored();
    const QList<QScreen *> screens = qApp->screens();
    for (const QScreen *screen : screens) {
        connect(screen, &QScreen::geometryChanged, this, &MirroredScreensTracker::checkScreensMirrored);
    }

    connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, &MirroredScreensTracker::checkScreensMirrored);
        checkScreensMirrored();
    });
    connect(qApp, &QGuiApplication::screenRemoved, this, &MirroredScreensTracker::checkScreensMirrored);
}

MirroredScreensTracker::~MirroredScreensTracker() = default;

MirroredScreensTracker::Ptr MirroredScreensTracker::createTracker()
{
    static std::weak_ptr<MirroredScreensTracker> s_instance;
    if (s_instance.expired()) {
        std::shared_ptr<MirroredScreensTracker> ptr(new MirroredScreensTracker());
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
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
    const QList<QScreen *> screens = qApp->screens();

    for (const QScreen *screen : screens) {
        for (const QScreen *checkScreen : screens) {
            if (screen == checkScreen) {
                continue;
            }
            if (screen->geometry().contains(checkScreen->geometry()) || checkScreen->geometry().contains(screen->geometry())) {
                qCDebug(NOTIFICATIONMANAGER) << "Screen geometry" << checkScreen->geometry() << "and" << screen->geometry()
                                             << "are completely overlapping - considering them to be mirrored";
                setScreensMirrored(true);
                return;
            }
        }
    }

    setScreensMirrored(false);
}
