/*****************************************************************

Copyright (c) 2000-2001 Matthias Elter <elter@kde.org>
Copyright (c) 2001 Richard Moore <rich@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#include "task_p.h"

#include <QX11Info>
#include <X11/Xutil.h>
#include <fixx11h.h>

namespace TaskManager
{

bool Task::updateDemandsAttentionState(WId w)
{
    const bool empty = d->transientsDemandingAttention.isEmpty();
    if (window() != w) {
        // 'w' is a transient for this task
        NETWinInfo i(QX11Info::connection(), w, QX11Info::appRootWindow(), NET::WMState);
        if (i.state() & NET::DemandsAttention) {
            if (!d->transientsDemandingAttention.contains(w)) {
                d->transientsDemandingAttention.insert(w);
            }
        } else {
            d->transientsDemandingAttention.remove(w);
        }
    }

    return empty != d->transientsDemandingAttention.isEmpty();
}

void Task::addTransient(WId w, const KWindowInfo &info)
{
    d->transients.insert(w);
    if (info.hasState(NET::DemandsAttention)) {
        d->transientsDemandingAttention.insert(w);
        emit changed(TransientsChanged | StateChanged | AttentionChanged);
    }
}

QString Task::className() const
{
    XClassHint hint;
    if (XGetClassHint(QX11Info::display(), d->win, &hint)) {
        QString nh(hint.res_name);
        XFree(hint.res_name);
        XFree(hint.res_class);
        return nh;
    }
    return QString();
}

QString Task::classClass() const
{
    XClassHint hint;
    if (XGetClassHint(QX11Info::display(), d->win, &hint)) {
        QString ch(hint.res_class);
        XFree(hint.res_name);
        XFree(hint.res_class);
        return ch;
    }
    return QString();
}

int Task::pid() const
{
    return NETWinInfo(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMPid).pid();
}

void Task::move()
{
    bool on_current = d->info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(d->info.desktop());
        KWindowSystem::forceActiveWindow(d->win);
    }

    if (d->info.isMinimized()) {
        KWindowSystem::unminimizeWindow(d->win);
    }

    QRect geom = d->info.geometry();
    QCursor::setPos(geom.center());

    NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(d->win, geom.center().x(),
                         geom.center().y(), NET::Move);
}

void Task::resize()
{
    bool on_current = d->info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(d->info.desktop());
        KWindowSystem::forceActiveWindow(d->win);
    }

    if (d->info.isMinimized()) {
        KWindowSystem::unminimizeWindow(d->win);
    }

    QRect geom = d->info.geometry();
    QCursor::setPos(geom.bottomRight());

    NETRootInfo ri(QX11Info::connection(), NET::WMMoveResize);
    ri.moveResizeRequest(d->win, geom.bottomRight().x(),
                         geom.bottomRight().y(), NET::BottomRight);
}

void Task::setMaximized(bool maximize)
{
    KWindowInfo info(d->win, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    if (info.isMinimized()) {
        KWindowSystem::unminimizeWindow(d->win);
    }

    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMState);

    if (maximize) {
        ni.setState(NET::Max, NET::Max);
    } else {
        ni.setState(0, NET::Max);
    }

    if (!on_current) {
        KWindowSystem::forceActiveWindow(d->win);
    }
}

void Task::restore()
{
    KWindowInfo info(d->win, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    if (info.isMinimized()) {
        KWindowSystem::unminimizeWindow(d->win);
    }

    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMState);
    ni.setState(0, NET::Max);

    if (!on_current) {
        KWindowSystem::forceActiveWindow(d->win);
    }
}

void Task::close()
{
    NETRootInfo ri(QX11Info::connection(), NET::CloseWindow);
    ri.closeWindowRequest(d->win);
}

void Task::toDesktop(int desk)
{
    if (desk == 0) {
        if (isOnAllDesktops()) {
            KWindowSystem::setOnDesktop(d->win, KWindowSystem::currentDesktop());
            KWindowSystem::forceActiveWindow(d->win);
        } else {
            KWindowSystem::setOnAllDesktops(d->win, true);
        }

        return;
    }

    KWindowSystem::setOnDesktop(d->win, desk);

    if (desk == KWindowSystem::currentDesktop()) {
        KWindowSystem::forceActiveWindow(d->win);
    }
}

void Task::setAlwaysOnTop(bool stay)
{
    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMState);
    if (stay)
        ni.setState(NET::StaysOnTop, NET::StaysOnTop);
    else
        ni.setState(0, NET::StaysOnTop);
}

void Task::setKeptBelowOthers(bool below)
{
    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMState);

    if (below) {
        ni.setState(NET::KeepBelow, NET::KeepBelow);
    } else {
        ni.setState(0, NET::KeepBelow);
    }
}

void Task::setFullScreen(bool fullscreen)
{
    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMState);

    if (fullscreen) {
        ni.setState(NET::FullScreen, NET::FullScreen);
    } else {
        ni.setState(0, NET::FullScreen);
    }
}

void Task::setShaded(bool shade)
{
    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), NET::WMState);
    if (shade)
        ni.setState(NET::Shaded, NET::Shaded);
    else
        ni.setState(0, NET::Shaded);
}

void Task::publishIconGeometry(QRect rect)
{
    if (rect == d->iconGeometry) {
        return;
    }

    d->iconGeometry = rect;
    NETWinInfo ni(QX11Info::connection(), d->win, QX11Info::appRootWindow(), 0);
    NETRect r;

    if (rect.isValid()) {
        r.pos.x = rect.x();
        r.pos.y = rect.y();
        r.size.width = rect.width();
        r.size.height = rect.height();
    }
    ni.setIconGeometry(r);
}

void Task::refreshActivities()
{
    NETWinInfo info(QX11Info::connection(), d->win, QX11Info::appRootWindow(), 0, NET::WM2Activities);
    QString result(info.activities());
    if (result.isEmpty() || result == "00000000-0000-0000-0000-000000000000") {
        d->activities.clear();
    } else {
        d->activities = result.split(',');
    }
}

} // namespace
