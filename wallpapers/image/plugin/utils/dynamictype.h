/*
    SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vladzzag@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class DynamicType
{
    Q_GADGET

public:
    enum Type {
        None,
        DayNight,
        /**
         * A solar dynamic wallpaper uses the position of the Sun provided
         * along each image to determine what image(s) reflect the user's light
         * situation most accurately.
         *
         * Beware that a solar dynamic wallpaper will fall back to use time if
         * the user lives close to the North or the South pole.
         */
        Solar,
        /**
         * A timed dynamic wallpaper uses the current time to determine what images
         * reflect the user's light situation most accurately.
         */
        Timed,
    };
    Q_ENUM(Type)
};
