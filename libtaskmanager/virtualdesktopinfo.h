/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QObject>

#include "taskmanager_export.h"

namespace TaskManager
{
/**
 * @short Provides basic virtual desktop information. The underlying windowing
 * system is abstracted away.
 *
 * This class provides basic information about the virtual desktops present
 * in the session as a set of notifiable properties.
 *
 * @author Eike Hein <hein@kde.org>
 **/

class TASKMANAGER_EXPORT VirtualDesktopInfo : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariant currentDesktop READ currentDesktop NOTIFY currentDesktopChanged)
    Q_PROPERTY(int numberOfDesktops READ numberOfDesktops NOTIFY numberOfDesktopsChanged)
    Q_PROPERTY(QVariantList desktopIds READ desktopIds NOTIFY desktopIdsChanged)
    Q_PROPERTY(QStringList desktopNames READ desktopNames NOTIFY desktopNamesChanged)
    Q_PROPERTY(int desktopLayoutRows READ desktopLayoutRows NOTIFY desktopLayoutRowsChanged)
    Q_PROPERTY(int navigationWrappingAround READ navigationWrappingAround NOTIFY navigationWrappingAroundChanged)

public:
    explicit VirtualDesktopInfo(QObject *parent = nullptr);
    ~VirtualDesktopInfo() override;

    /**
     * The currently active virtual desktop.
     *
     * @returns the id of the currently active virtual desktop. QString on
     * Wayland; uint >0 on X11.
     **/
    QVariant currentDesktop() const;

    /**
     * The number of virtual desktops present in the session.
     *
     * @returns the number of virtual desktops present in the session.
     **/
    int numberOfDesktops() const;

    /**
     * The ids of all virtual desktops present in the session.
     *
     * On Wayland, the ids are QString. On X11, they are uint >0.
     *
     * @returns a the list of ids of the virtual desktops present in the
     * session.
     **/
    QVariantList desktopIds() const;

    /**
     * The names of all virtual desktops present in the session.
     *
     * Note that on X11, virtual desktops are indexed starting at 1, so
     * the name for virtual desktop 1 is at index 0 in this list.
     *
     * @returns the list of names of the virtual desktops present in the
     * session.
     **/
    QStringList desktopNames() const;

    /**
     * Returns the position of the passed-in virtual desktop.
     * @param desktop A virtual desktop id (QString on Wayland; uint >0 on X11).
     * @returns the position of the virtual desktop, or -1 if the desktop
     * id is not valid.
     **/
    quint32 position(const QVariant &desktop) const;

    /**
     * The number of rows in the virtual desktop layout.
     *
     * @returns the number of rows in the virtual desktop layout.
     **/
    int desktopLayoutRows() const;

    /**
     * Request activating the passed-in virtual desktop.
     *
     * @param desktop A virtual desktop id (QString on Wayland; uint >0 on X11).
     **/
    void requestActivate(const QVariant &desktop);

    /**
     * Request adding a new virtual desktop at the specified position.
     *
     * On X11, the position parameter is ignored and the new desktop is always
     * created at the end of the list.
     *
     * @param position The position of the requested new virtual desktop (ignored on X11).
     **/
    void requestCreateDesktop(quint32 position);

    /**
     * Request removing the virtual desktop at the specified position.
     *
     * On X11, the position parameter is ignored and the last desktop in the list
     * is always the one removed.
     *
     * @param position The position of the virtual desktop to remove (ignored on X11).
     **/
    void requestRemoveDesktop(quint32 position);

    /**
     * Whether or not to wrap navigation such that activating the next virtual
     * desktop when at the last one will go to the first one, and activating the
     * previous virtual desktop when at the first one will go to the last one.
     */
    bool navigationWrappingAround() const;

Q_SIGNALS:
    void currentDesktopChanged() const;
    void numberOfDesktopsChanged() const;
    void desktopIdsChanged() const;
    void desktopNamesChanged() const;
    void desktopLayoutRowsChanged() const;
    void navigationWrappingAroundChanged() const;

private:
    class Private;
    class XWindowPrivate;
    class WaylandPrivate;
    static Private *d;
};

}
