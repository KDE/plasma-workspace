/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "abstractwindowtasksmodel.h"

#include <QGuiApplication>
#include <QScreen>

namespace TaskManager
{
AbstractWindowTasksModel::AbstractWindowTasksModel(QObject *parent)
    : AbstractTasksModel(parent)
{
    // TODO: The following will refresh the ScreenGeometry data role for
    // all rows whenever any screen is added or changes its geometry. No
    // attempt is made to be intelligent and exempt rows that are tech-
    // nically unaffected by the change. Doing so would require tracking
    // far more state (i.e. what screen a window is on) and be so
    // complicated as to invite bugs. As the trigger conditions are
    // expected to be rare, this would be premature optimization at this
    // time. That assessment may change in the future.

    auto screenAdded = [this](const QScreen *screen) {
        connect(screen, &QScreen::geometryChanged, this, [this]() {
            Q_EMIT dataChanged(index(0, 0), index(rowCount() - 1, 0), QVector<int>{ScreenGeometry});
        });
    };

    connect(qGuiApp, &QGuiApplication::screenAdded, this, screenAdded);

    foreach (const QScreen *screen, QGuiApplication::screens()) {
        screenAdded(screen);
    }
}

AbstractWindowTasksModel::~AbstractWindowTasksModel()
{
}

}
