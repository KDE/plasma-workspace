/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <memory>

#include <QObject>

#include "tasksmodel.h"

class QRect;

/**
 * This class monitors if there is any maximized or fullscreen window.
 * It is used by the animated image component.
 */
class MaximizedWindowMonitor : public TaskManager::TasksModel
{
    Q_OBJECT

    Q_PROPERTY(QRect targetRect READ targetRect WRITE setTargetRect NOTIFY targetRectChanged)

public:
    explicit MaximizedWindowMonitor(QObject *parent = nullptr);
    ~MaximizedWindowMonitor();

    QRect targetRect() const;
    void setTargetRect(const QRect &rect);

Q_SIGNALS:
    void targetRectChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    class Private;

    std::unique_ptr<Private> d;
};
