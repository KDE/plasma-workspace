/*
    SPDX-FileCopyrightText: 2012, 2013 Martin Graesslin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2015 David Edmudson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_event.h>

#include <QGuiApplication>
#include <QList>

#include "../c_ptr.h"
#include <X11/Xlib.h>

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

class Atom
{
public:
    explicit Atom(const QByteArray &name,
                  bool onlyIfExists = false,
                  xcb_connection_t *c = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection())
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
        UniqueCPointer<xcb_intern_atom_reply_t> reply(xcb_intern_atom_reply(m_connection, m_cookie, nullptr));
        if (reply) {
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
        , selectionAtom(xcb_atom_name_by_screen("_NET_SYSTEM_TRAY", DefaultScreen(qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->display())))
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

class TrayVisual
{
public:
    TrayVisual()
    {
        auto c = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
        auto screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
        visualId = screen->root_visual;
        visualDepth = screen->root_depth;
        colormap = screen->default_colormap;

        xcb_depth_iterator_t depth_iterator = xcb_screen_allowed_depths_iterator(screen);
        xcb_depth_t *depth = nullptr;

        while (depth_iterator.rem) {
            if (depth_iterator.data->depth == 32) {
                depth = depth_iterator.data;
                break;
            }
            xcb_depth_next(&depth_iterator);
        }
        if (!depth) {
            return;
        }

        xcb_visualtype_iterator_t visualtype_iterator = xcb_depth_visuals_iterator(depth);
        while (visualtype_iterator.rem) {
            xcb_visualtype_t *visualtype = visualtype_iterator.data;
            if (visualtype->_class == XCB_VISUAL_CLASS_TRUE_COLOR) {
                visualId = visualtype->visual_id;
                break;
            }
            xcb_visualtype_next(&visualtype_iterator);
        }
        if (visualId == screen->root_visual) {
            return;
        }

        // these are required for xcb_create_window when visual is different from parent
        visualDepth = depth->depth;
        colormap = xcb_generate_id(c);
        xcb_create_colormap(c, XCB_COLORMAP_ALLOC_NONE, colormap, screen->root, visualId);
        auto allocColorCookie = xcb_alloc_color(c, colormap, 0, 0, 0);
        auto allocColorReply = xcb_alloc_color_reply(c, allocColorCookie, nullptr);
        blackPixel = allocColorReply->pixel;
        free(allocColorReply);
    }

    ~TrayVisual()
    {
        auto c = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()->connection();
        // colormap might be screen->default_colormap, but Xlib doc says it's safe to free a default colormap
        xcb_free_colormap(c, colormap);
    }

    xcb_visualid_t visualId;
    uint8_t visualDepth;
    xcb_colormap_t colormap;
    uint32_t blackPixel;
};

extern TrayVisual *trayVisual;

} // namespace Xcb
