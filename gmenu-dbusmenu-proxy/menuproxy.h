/*
 * Copyright (C) 2018 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QWindow> // for WId

#include <xcb/xcb_atom.h>

class QDBusServiceWatcher;
class QTimer;

class KDirWatch;

class Window;

class MenuProxy : public QObject
{
    Q_OBJECT

public:
    MenuProxy();
    ~MenuProxy() override;

private Q_SLOTS:
    void onWindowAdded(WId id);
    void onWindowRemoved(WId id);

private:
    bool init();
    void teardown();

    static QString gtkRc2Path();
    static QString gtk3SettingsIniPath();

    void enableGtkSettings(bool enabled);

    void writeGtk2Settings();
    void writeGtk3Settings();

    void addOrRemoveAppMenuGtkModule(QStringList &list);

    xcb_connection_t *m_xConnection;

    QByteArray getWindowPropertyString(WId id, const QByteArray &name);
    void writeWindowProperty(WId id, const QByteArray &name, const QByteArray &value);
    xcb_atom_t getAtom(const QByteArray &name);

    QHash<WId, Window *> m_windows;

    QDBusServiceWatcher *m_serviceWatcher;

    KDirWatch *m_gtk2RcWatch;
    QTimer *m_writeGtk2SettingsTimer;

    bool m_enabled = false;

};
