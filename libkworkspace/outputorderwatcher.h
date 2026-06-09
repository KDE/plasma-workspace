/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <memory>

#include "kworkspace_export.h"

class OutputOrderWatcherPrivate;

/**
 * This class exposes the current output order from
 * the wayland backend.
 */
class KWORKSPACE_EXPORT OutputOrderWatcher : public QObject
{
    Q_OBJECT
public:
    OutputOrderWatcher(QObject *parent);
    ~OutputOrderWatcher();

    /**
     * Returns the list of outputs in order
     *
     * Due to async initialisation not everything in this list will be in sync with QGuiApplication::screens
     *
     * Ordering on adding is:
     *   - OutputOrderWatcher::outputOrderChanged (with the new addition(s))
     *   - QGuiApplication::screenAdded
     *
     *  Ordering on removal is:
     *   - QGuiApplication::screenRemoved
     *   - OutputOrderWatcher::outputOrderChanged (without the new entry)
     *
     */
    QStringList outputOrder() const;

Q_SIGNALS:
    void outputOrderChanged(const QStringList &outputOrder);

protected:
    std::unique_ptr<OutputOrderWatcherPrivate> d;
};
