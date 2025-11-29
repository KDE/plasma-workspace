/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "tasksmodel.h"

#include <QObject>
#include <QRect>
#include <qqmlregistration.h>

class Occlusion : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool coveringAny READ coveringAny NOTIFY coveringAnyChanged)
    Q_PROPERTY(bool coveringFullScreen READ coveringFullScreen NOTIFY coveringFullScreenChanged)
    Q_PROPERTY(QRect area READ area WRITE setArea NOTIFY areaChanged)

public:
    explicit Occlusion(QObject *parent = nullptr);

    bool coveringAny() const;
    bool coveringFullScreen() const;

    QRect area() const;
    void setArea(const QRect &area);

Q_SIGNALS:
    void coveringAnyChanged();
    void coveringFullScreenChanged();
    void areaChanged();

private:
    void update();

    void setCoveringAny(bool covering);
    void setCoveringFullScreen(bool covering);

    TaskManager::TasksModel *m_tasksModel = nullptr;

    bool m_coveringAny = false;
    bool m_coveringFullScreen = false;
};
