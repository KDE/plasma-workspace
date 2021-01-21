/********************************************************************
Copyright (C) 2012, 2013 Martin Graesslin <mgraesslin@kde.org>
Copyright (C) 2015 David Edmudson <davidedmundson@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#ifndef KWIN_XCB_UTILS_H
#define KWIN_XCB_UTILS_H

#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_event.h>

#include <QScopedPointer>
#include <QVector>
#include <QX11Info>

/** XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_WINDOW_DEACTIVATE 2
#define XEMBED_REQUEST_FOCUS 3
#define XEMBED_FOCUS_IN 4
#define XEMBED_FOCUS_OUT 5
#define XEMBED_FOCUS_NEXT 6
#define XEMBED_FOCUS_PREV 7

namespace Xcb
{
typedef xcb_window_t WindowId;

template<typename T> using ScopedCPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

class Atom
{
public:
    explicit Atom(const QByteArray &name, bool onlyIfExists = false, xcb_connection_t *c = QX11Info::connection())
        : m_connection(c)
        , m_retrieved(false)
        , m_cookie(xcb_intern_atom_unchecked(m_connection, onlyIfExists, name.length(), name.constData()))
        , m_atom(XCB_ATOM_NONE)
        , m_name(name)
    {
    }
    Atom() = delete;
    Atom(const Atom &) = delete;

    ~Atom()
    {
        if (!m_retrieved && m_cookie.sequence) {
            xcb_discard_reply(m_connection, m_cookie.sequence);
        }
    }

    operator xcb_atom_t() const
    {
        (const_cast<Atom *>(this))->getReply();
        return m_atom;
    }
    bool isValid()
    {
        getReply();
        return m_atom != XCB_ATOM_NONE;
    }
    bool isValid() const
    {
        (const_cast<Atom *>(this))->getReply();
        return m_atom != XCB_ATOM_NONE;
    }

    inline const QByteArray &name() const
    {
        return m_name;
    }

private:
    void getReply()
    {
        if (m_retrieved || !m_cookie.sequence) {
            return;
        }
        ScopedCPointer<xcb_intern_atom_reply_t> reply(xcb_intern_atom_reply(m_connection, m_cookie, nullptr));
        if (!reply.isNull()) {
            m_atom = reply->atom;
        }
        m_retrieved = true;
    }
    xcb_connection_t *m_connection;
    bool m_retrieved;
    xcb_intern_atom_cookie_t m_cookie;
    xcb_atom_t m_atom;
    QByteArray m_name;
};

class Atoms
{
public:
    Atoms()
        : xembedAtom("_XEMBED")
        , selectionAtom(xcb_atom_name_by_screen("_NET_SYSTEM_TRAY", QX11Info::appScreen()))
        , opcodeAtom("_NET_SYSTEM_TRAY_OPCODE")
        , messageData("_NET_SYSTEM_TRAY_MESSAGE_DATA")
        , visualAtom("_NET_SYSTEM_TRAY_VISUAL")
    {
    }

    Atom xembedAtom;
    Atom selectionAtom;
    Atom opcodeAtom;
    Atom messageData;
    Atom visualAtom;
};

extern Atoms *atoms;

} // namespace Xcb

#endif // KWIN_XCB_UTILS_H
