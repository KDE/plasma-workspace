/*
    SPDX-FileCopyrightText: 2016 Ivan Cukic <ivan.cukic@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QCoreApplication>

template<typename T>
inline void awaitFuture(const QFuture<T> &future)
{
    while (!future.isFinished()) {
        QCoreApplication::processEvents();
    }
}
