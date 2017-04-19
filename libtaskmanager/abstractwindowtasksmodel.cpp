/********************************************************************
Copyright 2016  Eike Hein <hein@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

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
        connect(screen, &QScreen::geometryChanged, this,
            [this]() {
                dataChanged(index(0, 0), index(rowCount() - 1, 0), QVector<int>{ScreenGeometry});
            }
        );
    };

    connect(qGuiApp, &QGuiApplication::screenAdded, this, screenAdded);

    foreach(const QScreen *screen, QGuiApplication::screens()) {
        screenAdded(screen);
    }
}

AbstractWindowTasksModel::~AbstractWindowTasksModel()
{
}

}
