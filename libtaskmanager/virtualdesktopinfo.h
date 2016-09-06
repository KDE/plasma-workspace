/********************************************************************
Copyright 2016  Eike Hein <hein.org>

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

#ifndef VIRTUALDESKTOPINFO_H
#define VIRTUALDESKTOPINFO_H

#include <QObject>

#include "taskmanager_export.h"

#include <config-X11.h>

namespace TaskManager
{

/**
 * @short Provides basic virtual desktop information.
 *
 * This class provides basic information about the virtual desktops present
 * in the session as a set of notifiable properties.
 *
 * @NOTE: This is a placeholder, to be moved into KWindowSystem (which it
 * wraps) or the Task Manager applet backend (which used to fill this role
 * in the past).
 *
 * @see KWindowSystem
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT VirtualDesktopInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int currentDesktop READ currentDesktop NOTIFY currentDesktopChanged)
    Q_PROPERTY(int numberOfDesktops READ numberOfDesktops NOTIFY numberOfDesktopsChanged)
    Q_PROPERTY(QStringList desktopNames READ desktopNames NOTIFY desktopNamesChanged)
    Q_PROPERTY(int desktopLayoutRows READ desktopLayoutRows NOTIFY desktopLayoutRowsChanged)

public:
    explicit VirtualDesktopInfo(QObject *parent = 0);
    virtual ~VirtualDesktopInfo();

    /**
     * The currently active virtual desktop.
     *
     * @returns the number of the currently active virtual desktop.
     **/
    int currentDesktop() const;

    /**
     * The number of virtual desktops present in the session.
     *
     * @returns the number of virtual desktops present in the session.
     **/
    int numberOfDesktops() const;

    /**
     * The names of all virtual desktops present in the session. Note that
     * virtual desktops are indexed starting at 1, so the name for virtual
     * desktop 1 is at index 0 in this list.
     *
     * @returns a the list of names for the virtual desktops present in the
     * session.
     **/
    QStringList desktopNames() const;

    /**
     * The number of rows in the virtual desktop layout.
     *
     * @returns the number of rows in the virtual desktop layout.
     **/
    int desktopLayoutRows() const;

Q_SIGNALS:
    void currentDesktopChanged() const;
    void numberOfDesktopsChanged() const;
    void desktopNamesChanged() const;
    void desktopLayoutRowsChanged() const;
};

}

#endif
