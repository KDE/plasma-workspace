/*****************************************************************
ksmserver - the KDE session management server

Copyright 2016 Martin Graesslin <mgraesslin@kde.org>
Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>

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

#include <QGuiApplication>
#include <QScreen>

#include "../switchuserdialog.h"

#include <kdisplaymanager.h>
#include <KWindowSystem>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#include <unistd.h>

class Greeter : public QObject
{
    Q_OBJECT
public:
    Greeter();
    ~Greeter() override;

    void init();

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void adoptScreen(QScreen *screen);
    void rejected();
    void setupWaylandIntegration();

    QVector<KSMSwitchUserDialog *> m_dialogs;
    KWayland::Client::PlasmaShell *m_waylandPlasmaShell;
    KDisplayManager m_displayManager;
};

Greeter::Greeter()
    : QObject()
    , m_waylandPlasmaShell(nullptr)
{
}

Greeter::~Greeter()
{
    qDeleteAll(m_dialogs);
}

void Greeter::setupWaylandIntegration()
{
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }
    using namespace KWayland::Client;
    ConnectionThread *connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(connection);
    connect(registry, &Registry::plasmaShellAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandPlasmaShell = registry->createPlasmaShell(name, version, this);
        }
    );
    registry->setup();
    connection->roundtrip();
}

void Greeter::init()
{
    setupWaylandIntegration();
    foreach (QScreen *screen, qApp->screens()) {
        adoptScreen(screen);
    }
    connect(qApp, &QGuiApplication::screenAdded, this, &Greeter::adoptScreen);
}

void Greeter::adoptScreen(QScreen* screen)
{
    KSMSwitchUserDialog *w = new KSMSwitchUserDialog(&m_displayManager, m_waylandPlasmaShell);
    w->installEventFilter(this);
    m_dialogs << w;

    QObject::connect(screen, &QObject::destroyed, w, [w, this] {
        m_dialogs.removeOne(w);
        w->deleteLater();
    });
    connect(w, &KSMSwitchUserDialog::dismissed, qApp, &QCoreApplication::quit);
    w->setScreen(screen);
    w->setGeometry(screen->geometry());
    w->init();
}

bool Greeter::eventFilter(QObject *watched, QEvent *event)
{
    if (qobject_cast<KSMSwitchUserDialog*>(watched)) {
        if (event->type() == QEvent::MouseButtonPress) {
            // check that the position is on no window
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            for (auto it = m_dialogs.constBegin(); it != m_dialogs.constEnd(); ++it) {
                if ((*it)->geometry().contains(me->globalPos())) {
                    return false;
                }
            }
            // click outside, close
            qApp->quit();
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    // Qt does not currently (5.9.4) support fullscreen on xdg_shell v6.
    qputenv("QT_WAYLAND_SHELL_INTEGRATION", "wl-shell");

    QQuickWindow::setDefaultAlphaBuffer(true);
    QGuiApplication app(argc, argv);

    Greeter greeter;
    greeter.init();

    return app.exec();
}

#include "main.moc"
