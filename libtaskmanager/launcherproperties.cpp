/*****************************************************************

Copyright (C) 2011 Craig Drummond <craig.p.drummond@gmail.com>

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

#include "launcherproperties.h"
#include <KOpenWithDialog>
#include <KDesktopFile>
#include <KWindowInfo>
#include <KWindowSystem>
#include <KLocalizedString>

#include <QtGui/QMouseEvent>
#include <QtX11Extras/QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <fixx11h.h>
#include <QDialogButtonBox>

namespace TaskManager
{

LauncherProperties::LauncherProperties(QWidget *parent)
    : QDialog(parent)
    , grabber(0L)
{
    setWindowTitle(i18n("Launcher Properties"));
    QWidget *mainWidet = new QWidget(this);
    ui.setupUi(mainWidet);

    setLayout(new QVBoxLayout(this));
    layout()->addWidget(mainWidet);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout()->addWidget(buttons);

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    ui.browse->setIcon(QIcon::fromTheme("document-open"));
    connect(ui.detect, SIGNAL(clicked(bool)), SLOT(detect()));
    connect(ui.browse, SIGNAL(clicked(bool)), SLOT(browse()));
    connect(ui.classClass, SIGNAL(textChanged(const QString &)), SLOT(check()));
    connect(ui.className, SIGNAL(textChanged(const QString &)), SLOT(check()));
    connect(ui.launcher, SIGNAL(textChanged(const QString &)), SLOT(check()));
    connect(buttons, SIGNAL(accepted()), SLOT(okClicked()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    resize(400, 100);
}

LauncherProperties::~LauncherProperties()
{
    delete grabber;
}

void LauncherProperties::run(const QString &cc, const QString &cn, const QString &l)
{
    ui.classClass->setText(cc);
    ui.className->setText(cn);
    ui.launcher->setText(l);
    check();
    show();
}

void LauncherProperties::check()
{
    buttons->button(QDialogButtonBox::Ok)->setEnabled(!ui.classClass->text().isEmpty() && !ui.launcher->text().isEmpty());
}

void LauncherProperties::detect()
{
    // Taken from oxygen...
    // use a dialog, so that all user input is blocked
    // use WX11BypassWM and moving away so that it's not actually visible
    // grab only mouse, so that keyboard can be used e.g. for switching windows
    grabber = new QDialog(0, Qt::X11BypassWindowManagerHint);
    grabber->move(-1000, -1000);
    grabber->setModal(true);
    grabber->show();
    grabber->grabMouse(Qt::CrossCursor);
    grabber->installEventFilter(this);
}

void LauncherProperties::browse()
{
    KOpenWithDialog *dlg = new KOpenWithDialog(QList<QUrl>(), i18n("Select launcher application:"), QString(), this);
    dlg->hideNoCloseOnExit();
    dlg->hideRunInTerminal();
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::WindowModal);
    connect(dlg, SIGNAL(accepted()), SLOT(launcherSelected()));
    dlg->show();
}

void LauncherProperties::launcherSelected()
{
    KOpenWithDialog *dlg = qobject_cast<KOpenWithDialog*>(sender());

    if (dlg) {
        KService::Ptr srv = dlg->service();

        if (srv && srv->isApplication() && !srv->entryPath().isEmpty()) {
            QUrl url = QUrl::fromLocalFile(srv->entryPath());

            if (url.isLocalFile() && KDesktopFile::isDesktopFile(url.toLocalFile())) {
                ui.launcher->setText(url.toDisplayString(QUrl::PrettyDecoded));
            }
        } else {
            QString path = dlg->text();

            if (!path.isEmpty()) {
                QUrl url = QUrl::fromLocalFile(path);
                if (url.isLocalFile()) {
                    ui.launcher->setText(url.toDisplayString(QUrl::PrettyDecoded));
                }
            }
        }
    }
}

void LauncherProperties::okClicked()
{
    emit properties(ui.classClass->text(), ui.className->text(), ui.launcher->text());
    accept();
}

bool LauncherProperties::eventFilter(QObject *o, QEvent *e)
{
    // check object and event type
    if (o != grabber || QEvent::MouseButtonRelease != e->type()) {
        return false;
    }

    // delete old grabber
    delete grabber;
    grabber = 0;

    // check button
    if (Qt::LeftButton != static_cast< QMouseEvent* >(e)->button()) {
        return true;
    }

    // read window information
    WId window = findWindow();

    if (0 != window) {
        KWindowInfo info(window, 0, NET::WM2WindowClass);
        if (info.valid()) {
            ui.classClass->setText(info.windowClassClass());
            ui.className->setText(info.windowClassName());
        }
    }

    return true;
}

WId LauncherProperties::findWindow()
{
    Window root;
    Window child;
    uint mask;
    int rootX, rootY, x, y;
    Window parent = QX11Info::appRootWindow();
    Atom wm_state = XInternAtom(QX11Info::display(), "WM_STATE", False);

    // why is there a loop of only 10 here
    for (int i = 0; i < 10; ++i) {
        XQueryPointer(QX11Info::display(), parent, &root, &child, &rootX, &rootY, &x, &y, &mask);
        if (child == None) return 0;
        Atom type;
        int format;
        unsigned long nitems, after;
        unsigned char* prop;
        if (XGetWindowProperty(
                    QX11Info::display(), child, wm_state, 0, 0, False,
                    AnyPropertyType, &type, &format, &nitems, &after, &prop) == Success) {
            if (prop != NULL) XFree(prop);
            if (type != None) return child;
        }
        parent = child;
    }

    return 0;
}

}

#include "launcherproperties.moc"
