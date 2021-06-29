/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>
#include <QSharedPointer>

#include <KScreen/Config>

namespace NotificationManager
{
/**
 * @short Tracks whether there are any mirrored screens
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class MirroredScreensTracker : public QObject
{
    Q_OBJECT

public:
    ~MirroredScreensTracker();

    using Ptr = QSharedPointer<MirroredScreensTracker>;
    static Ptr createTracker();

    bool screensMirrored() const;
    /**
     * Set whether screens are mirrored
     *
     * This is public so that automatic do not disturb mode when screens are mirrored
     * can be disabled temporarily until screen configuration changes again.
     */
    void setScreensMirrored(bool mirrored);
    Q_SIGNAL void screensMirroredChanged(bool mirrored);

private:
    MirroredScreensTracker();
    Q_DISABLE_COPY(MirroredScreensTracker)

    void checkScreensMirrored();

    KScreen::ConfigPtr m_screenConfiguration;
    bool m_screensMirrored = false;
};

} // namespace NotificationManager
