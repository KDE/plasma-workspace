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
