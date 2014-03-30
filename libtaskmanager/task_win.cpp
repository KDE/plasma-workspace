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

#include <windows.h>


namespace TaskManager
{

bool Task::updateDemandsAttentionState(WId w)
{
    return false;
}

QString Task::className() const
{
    return QString();
}

QString Task::classClass() const
{
    return QString();
}

int Task::pid() const
{
    return 0; // FIXME!!!
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
}

void Task::setMaximized(bool maximize)
{
    KWindowInfo info = KWindowSystem::windowInfo(d->win, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    if (info.isMinimized()) {
        KWindowSystem::unminimizeWindow(d->win);
    }

    if (!on_current) {
        KWindowSystem::forceActiveWindow(d->win);
    }
}

void Task::restore()
{
    KWindowInfo info = KWindowSystem::windowInfo(d->win, NET::WMState | NET::XAWMState | NET::WMDesktop);
    bool on_current = info.isOnCurrentDesktop();

    if (!on_current) {
        KWindowSystem::setCurrentDesktop(info.desktop());
    }

    if (info.isMinimized()) {
        KWindowSystem::unminimizeWindow(d->win);
    }

    if (!on_current) {
        KWindowSystem::forceActiveWindow(d->win);
    }
}

void Task::close()
{
    PostMessage(d->win, WM_CLOSE, 0, 0);
}

void Task::toDesktop(int desk)
{
    if (desk == KWindowSystem::currentDesktop()) {
        KWindowSystem::forceActiveWindow(d->win);
    }
}

void Task::setAlwaysOnTop(bool stay)
{
    Q_UNUSED(stay);
}

void Task::setKeptBelowOthers(bool below)
{
    Q_UNUSED(below);
}

void Task::setFullScreen(bool fullscreen)
{
    Q_UNUSED(fullscreen);
}

void Task::setShaded(bool shade)
{
    Q_UNUSED(shade);
}

void Task::publishIconGeometry(QRect rect)
{
    if (rect == d->iconGeometry) {
        return;
    }

    d->iconGeometry = rect;
}

void Task::refreshActivities()
{
    return;
}

} // TaskManager namespace
