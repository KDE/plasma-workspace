/*
    SPDX-FileCopyrightText: 2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <Plasma/DataEngine>
#include <kpluginfactory.h>

#include <QDebug>

/**
 * This engine provides the current date and time for a given
 * timezone. Optionally it can also provide solar position info.
 *
 * "Local" is a special source that is an alias for the current
 * timezone.
 */
class TimeEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    TimeEngine(QObject *parent, const QVariantList &args);
    ~TimeEngine() override;

    QStringList sources() const override;

protected:
    bool sourceRequestEvent(const QString &name) override;
    bool updateSourceEvent(const QString &source) override;

protected Q_SLOTS:
    void clockSkewed(); // call when system time changed and all clocks should be updated
    void tzConfigChanged();

private Q_SLOTS:
    void init();
};
