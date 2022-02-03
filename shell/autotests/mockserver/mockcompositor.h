/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCOMPOSITOR_H
#define MOCKCOMPOSITOR_H

#include "corecompositor.h"
#include "coreprotocol.h"
#include "datadevice.h"
#include "primaryoutput.h"
#include "xdgoutputv1.h"
#include "xdgshell.h"

#include <QtGui/QGuiApplication>

// As defined in linux/input-event-codes.h
#ifndef BTN_LEFT
#define BTN_LEFT 0x110
#endif
#ifndef BTN_RIGHT
#define BTN_RIGHT 0x111
#endif
#ifndef BTN_MIDDLE
#define BTN_MIDDLE 0x112
#endif

namespace MockCompositor
{
class DefaultCompositor : public CoreCompositor
{
public:
    explicit DefaultCompositor();
    // Convenience functions
    Output *output(int i = 0)
    {
        return getAll<Output>().value(i, nullptr);
    }
    Surface *surface(int i = 0)
    {
        return get<WlCompositor>()->m_surfaces.value(i, nullptr);
    }
    Subsurface *subSurface(int i = 0)
    {
        return get<SubCompositor>()->m_subsurfaces.value(i, nullptr);
    }
    XdgSurface *xdgSurface(int i = 0)
    {
        return get<XdgWmBase>()->m_xdgSurfaces.value(i, nullptr);
    }
    XdgToplevel *xdgToplevel(int i = 0)
    {
        return get<XdgWmBase>()->toplevel(i);
    }
    XdgPopup *xdgPopup(int i = 0)
    {
        return get<XdgWmBase>()->popup(i);
    }
    Pointer *pointer()
    {
        auto *seat = get<Seat>();
        Q_ASSERT(seat);
        return seat->m_pointer;
    }
    Touch *touch()
    {
        auto *seat = get<Seat>();
        Q_ASSERT(seat);
        return seat->m_touch;
    }
    Surface *cursorSurface()
    {
        auto *p = pointer();
        return p ? p->cursorSurface() : nullptr;
    }
    Keyboard *keyboard()
    {
        auto *seat = get<Seat>();
        Q_ASSERT(seat);
        return seat->m_keyboard;
    }
    XdgOutputV1 *xdgOutput(Output *out)
    {
        return get<XdgOutputManagerV1>()->getXdgOutput(out);
    }
    PrimaryOutputV1 *primaryOutput()
    {
        auto *primary = get<PrimaryOutputV1>();
        Q_ASSERT(primary);
        return primary;
    }

    uint sendXdgShellPing();
    void xdgPingAndWaitForPong();
    // Things that can be changed run-time without confusing the client (i.e. don't require separate tests)
    struct Config {
        bool autoEnter = true;
        bool autoRelease = true;
        bool autoConfigure = false;
    } m_config;
    void resetConfig()
    {
        exec([&] {
            m_config = Config{};
        });
    }
};

} // namespace MockCompositor

#define QCOMPOSITOR_VERIFY(expr)                                                                                                                               \
    QVERIFY(exec([&] {                                                                                                                                         \
        return expr;                                                                                                                                           \
    }))
#define QCOMPOSITOR_TRY_VERIFY(expr)                                                                                                                           \
    QTRY_VERIFY(exec([&] {                                                                                                                                     \
        return expr;                                                                                                                                           \
    }))
#define QCOMPOSITOR_COMPARE(expr, expr2)                                                                                                                       \
    QCOMPARE(exec([&] {                                                                                                                                        \
                 return expr;                                                                                                                                  \
             }),                                                                                                                                               \
             expr2)
#define QCOMPOSITOR_TRY_COMPARE(expr, expr2)                                                                                                                   \
    QTRY_COMPARE(exec([&] {                                                                                                                                    \
                     return expr;                                                                                                                              \
                 }),                                                                                                                                           \
                 expr2)

#define QCOMPOSITOR_TEST_MAIN(test)                                                                                                                            \
    int main(int argc, char **argv)                                                                                                                            \
    {                                                                                                                                                          \
        QTemporaryDir tmpRuntimeDir;                                                                                                                           \
        setenv("XDG_RUNTIME_DIR", tmpRuntimeDir.path().toLocal8Bit(), 1);                                                                                      \
        setenv("XDG_CURRENT_DESKTOP", "qtwaylandtests", 1);                                                                                                    \
        setenv("QT_QPA_PLATFORM", "wayland", 1);                                                                                                               \
        test tc;                                                                                                                                               \
        QGuiApplication app(argc, argv);                                                                                                                       \
        return QTest::qExec(&tc, argc, argv);                                                                                                                  \
    }

#endif
