/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2007-2009 David Jarvie <djarvie@kde.org>
    SPDX-FileCopyrightText: 2009 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <kdedmodule.h>

class KTimeZonedBase : public KDEDModule
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KTimeZoned")

public:
    KTimeZonedBase(QObject *parent, const QList<QVariant> &)
        : KDEDModule(parent)
    {
    }
    ~KTimeZonedBase() override{};

public Q_SLOTS:
    /** D-Bus call to initialize the module.
     *  @param reinit determines whether to reinitialize if the module has already
     *                initialized itself
     */
    Q_SCRIPTABLE void initialize(bool reinit)
    {
        // If we reach here, the module has already been constructed and therefore
        // initialized. So only do anything if reinit is true.
        if (reinit)
            init(true);
    }

Q_SIGNALS:
    /** D-Bus signal emitted when the time zone configuration has changed. */
    void timeZoneChanged();

    /** D-Bus signal emitted when the definition (not the identity) of the local
     *  system time zone has changed.
     */
    void timeZoneDatabaseUpdated();

protected:
    virtual void init(bool) = 0;

    QString m_localZone; // local system time zone name
};
