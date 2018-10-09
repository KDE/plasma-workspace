/*****************************************************************
ksmserver - the KDE session management server

Copyright 2016 Martin Graesslin <mgraesslin@kde.org>
Copyright 2018 David Edmundson <davidedmundson@kde.org>

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

#pragma once

#include <QObject>
#include <QVector>

#include <kworkspace.h>

class KSMShutdownDlg;
namespace KWayland {
namespace Client {
    class PlasmaShell;
}
}

class QScreen;

class Greeter : public QObject
{
    Q_OBJECT
public:
    Greeter(bool shutdownAllowed);
    ~Greeter() override;

    void init();

    bool eventFilter(QObject *watched, QEvent *event) override;

public Q_SLOTS:
    void promptLogout();
    void promptShutDown();
    void promptReboot();

private:
    void adoptScreen(QScreen *screen);
    void rejected();
    void setupWaylandIntegration();

    bool m_shutdownAllowed;
    bool m_running = false;

    KWorkSpace::ShutdownType m_shutdownType = KWorkSpace::ShutdownTypeHalt;
    QVector<KSMShutdownDlg *> m_dialogs;
    KWayland::Client::PlasmaShell *m_waylandPlasmaShell;
};
