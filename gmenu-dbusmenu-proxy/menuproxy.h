/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QByteArray>
#include <QHash>
#include <QObject>
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
