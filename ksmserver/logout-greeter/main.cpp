/*****************************************************************
ksmserver - the KDE session management server

Copyright 2016 Martin Graesslin <mgraesslin@kde.org>

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
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QScreen>
#include "../shutdowndlg.h"

#include <KQuickAddons/QtQuickSettings>

#include <KWindowSystem>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#include <unistd.h>

class Greeter : public QObject
{
    Q_OBJECT
public:
    Greeter(int fd, bool shutdownAllowed, bool choose, KWorkSpace::ShutdownType type);
    virtual ~Greeter();

    void init();

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void adoptScreen(QScreen *screen);
    void rejected();
    void setupWaylandIntegration();

    int m_fd;
    bool m_shutdownAllowed;
    bool m_choose;
    KWorkSpace::ShutdownType m_shutdownType;
    QVector<KSMShutdownDlg *> m_dialogs;
    KWayland::Client::PlasmaShell *m_waylandPlasmaShell;
};

Greeter::Greeter(int fd, bool shutdownAllowed, bool choose, KWorkSpace::ShutdownType type)
    : QObject()
    , m_fd(fd)
    , m_shutdownAllowed(shutdownAllowed)
    , m_choose(choose)
    , m_shutdownType(type)
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
    // TODO: last argument is the theme, maybe add command line option for it?
    KSMShutdownDlg *w = new KSMShutdownDlg(nullptr, m_shutdownAllowed, m_choose, m_shutdownType, QString(), m_waylandPlasmaShell);
    w->installEventFilter(this);
    m_dialogs << w;

    QObject::connect(screen, &QObject::destroyed, w, [w, this] {
        m_dialogs.removeOne(w);
        w->deleteLater();
    });
    connect(w, &KSMShutdownDlg::rejected, this, &Greeter::rejected);
    connect(w, &KSMShutdownDlg::accepted, this,
        [w, this] {
            if (m_fd != -1) {
                QFile f;
                if (f.open(m_fd, QFile::WriteOnly, QFile::AutoCloseHandle)) {
                    f.write(QByteArray::number(int(w->shutdownType())));
                    f.close();
                }
            }
            QApplication::quit();
        }
    );
    w->setScreen(screen);
    w->setGeometry(screen->geometry());
    w->init();
}

void Greeter::rejected()
{
    if (m_fd != -1) {
        close(m_fd);
    }
    QApplication::exit(1);
}

bool Greeter::eventFilter(QObject *watched, QEvent *event)
{
    if (qobject_cast<KSMShutdownDlg*>(watched)) {
        if (event->type() == QEvent::MouseButtonPress) {
            // check that the position is on no window
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            for (auto it = m_dialogs.constBegin(); it != m_dialogs.constEnd(); ++it) {
                if ((*it)->geometry().contains(me->globalPos())) {
                    return false;
                }
            }
            // click outside, close
            rejected();
        }
    }
    return false;
}

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);

    KQuickAddons::QtQuickSettings::init();

    QCommandLineParser parser;
    parser.addHelpOption();

    // TODO: should these things be translated? It's internal after all...
    QCommandLineOption shutdownAllowedOption(QStringLiteral("shutdown-allowed"),
                                             QStringLiteral("Whether the user is allowed to shut down the system."));
    parser.addOption(shutdownAllowedOption);

    QCommandLineOption chooseOption(QStringLiteral("choose"),
                                    QStringLiteral("Whether the user is offered the choices between logout, shutdown, etc."));
    parser.addOption(chooseOption);

    QCommandLineOption modeOption(QStringLiteral("mode"),
                                  QStringLiteral("The initial exit mode to offer to the user."),
                                  QStringLiteral("logout|shutdown|reboot"),
                                  QStringLiteral("logout"));
    parser.addOption(modeOption);

    QCommandLineOption fdOption(QStringLiteral("mode-fd"),
                                QStringLiteral("An optional file descriptor the selected mode is written to on accepted"),
                                QStringLiteral("fd"), QString::number(-1));
    parser.addOption(fdOption);

    parser.process(app);

    KWorkSpace::ShutdownType type = KWorkSpace::ShutdownTypeDefault;
    if (parser.isSet(modeOption)) {
        const QString modeValue = parser.value(modeOption);
        if (QString::compare(QLatin1String("logout"), modeValue, Qt::CaseInsensitive) == 0) {
            type = KWorkSpace::ShutdownTypeNone;
        } else if (QString::compare(QLatin1String("shutdown"), modeValue, Qt::CaseInsensitive) == 0) {
            type = KWorkSpace::ShutdownTypeHalt;
        } else if (QString::compare(QLatin1String("reboot"), modeValue, Qt::CaseInsensitive) == 0) {
            type = KWorkSpace::ShutdownTypeReboot;
        } else {
            return 1;
        }
    }

    int fd = -1;
    if (parser.isSet(fdOption)) {
        bool ok = false;
        const int passedFd = parser.value(fdOption).toInt(&ok);
        if (ok) {
            fd = dup(passedFd);
        }
    }
    Greeter greeter(fd, parser.isSet(shutdownAllowedOption), parser.isSet(chooseOption), type);
    greeter.init();

    return app.exec();
}

#include "main.moc"
