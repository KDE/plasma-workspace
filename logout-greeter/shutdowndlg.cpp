/*
    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 2007 Urs Wolfer <uwolfer @ kde.org>

    SPDX-License-Identifier: MIT
*/

#include "shutdowndlg.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QFile>
#include <QPainter>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>
#include <QTimer>
#include <private/qtx11extras_p.h>

#include <KAuthorized>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KUser>
#include <KWindowEffects>
#include <KWindowSystem>
#include <KX11Extras>
#include <LayerShellQt/Window>

#include <netwm.h>
#include <stdio.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <fixx11h.h>

#include <config-workspace.h>
#include <debug.h>

static const QString s_login1Service = QStringLiteral("org.freedesktop.login1");
static const QString s_login1Path = QStringLiteral("/org/freedesktop/login1");
static const QString s_dbusPropertiesInterface = QStringLiteral("org.freedesktop.DBus.Properties");
static const QString s_login1ManagerInterface = QStringLiteral("org.freedesktop.login1.Manager");
static const QString s_login1RebootToFirmwareSetup = QStringLiteral("RebootToFirmwareSetup");

KSMShutdownDlg::KSMShutdownDlg(QWindow *parent, const QString &defaultAction, QScreen *screen)
    : QuickViewSharedEngine(parent)
// this is a WType_Popup on purpose. Do not change that! Not
// having a popup here has severe side effects.
{
    // window stuff
    setColor(QColor(Qt::transparent));
    setScreen(screen);

    if (KWindowSystem::isPlatformWayland() && !m_windowed) {
        if (auto w = LayerShellQt::Window::get(this)) {
            w->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityExclusive);
            w->setExclusiveZone(-1);
            w->setLayer(LayerShellQt::Window::LayerOverlay);
        }
    }

    setResizeMode(PlasmaQuick::QuickViewSharedEngine::SizeRootObjectToView);

    // Qt doesn't set this on unmanaged windows
    // FIXME: or does it?
    if (KWindowSystem::isPlatformX11()) {
        XChangeProperty(QX11Info::display(),
                        winId(),
                        XInternAtom(QX11Info::display(), "WM_WINDOW_ROLE", False),
                        XA_STRING,
                        8,
                        PropModeReplace,
                        (unsigned char *)"logoutdialog",
                        strlen("logoutdialog"));

        XClassHint classHint;
        classHint.res_name = const_cast<char *>("ksmserver-logout-greeter");
        classHint.res_class = const_cast<char *>("ksmserver-logout-greeter");
        XSetClassHint(QX11Info::display(), winId(), &classHint);
    }

    // QQuickView *windowContainer = QQuickView::createWindowContainer(m_view, this);
    // windowContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QQmlContext *context = rootContext();
    context->setContextProperty(QStringLiteral("defaultAction"), defaultAction);

    // Trying to access a non-existent context property throws an error, always create the property and then update it later
    context->setContextProperty("rebootToFirmwareSetup", false);

    QDBusMessage message = QDBusMessage::createMethodCall(s_login1Service, s_login1Path, s_dbusPropertiesInterface, QStringLiteral("Get"));
    message.setArguments({s_login1ManagerInterface, s_login1RebootToFirmwareSetup});
    QDBusPendingReply<QVariant> call = QDBusConnection::systemBus().asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(call, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, context, [context](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<QVariant> reply = *watcher;
        watcher->deleteLater();

        if (reply.value().toBool()) {
            context->setContextProperty("rebootToFirmwareSetup", true);
        }
    });

    // engine stuff
    engine()->rootContext()->setContextObject(new KLocalizedContext(engine().get()));
    engine()->setProperty("_kirigamiTheme", QStringLiteral("KirigamiPlasmaStyle"));
}

void KSMShutdownDlg::init(const KPackage::Package &package)
{
    rootContext()->setContextProperty(QStringLiteral("screenGeometry"), screen()->geometry());

    const QString fileName = package.filePath("logoutmainscript");

    if (QFile::exists(fileName)) {
        setSource(package.fileUrl("logoutmainscript"));
    } else {
        qCWarning(LOGOUT_GREETER) << "Couldn't find a theme for the Shutdown dialog" << fileName;
        return;
    }

    if (!errors().isEmpty()) {
        qCWarning(LOGOUT_GREETER) << errors();
    }

    connect(screen(), &QScreen::geometryChanged, this, [this] {
        setGeometry(screen()->geometry());
    });

    // decide in backgroundcontrast whether doing things darker or lighter
    // set backgroundcontrast here, because in QEvent::PlatformSurface
    // is too early and we don't have the root object yet
    const QColor backgroundColor = rootObject() ? rootObject()->property("backgroundColor").value<QColor>() : QColor();
    KWindowEffects::enableBackgroundContrast(this, true, 0.4, (backgroundColor.value() > 128 ? 1.6 : 0.3), 1.7);
    if (m_windowed) {
        show();
    } else {
        showFullScreen();
        setFlag(Qt::FramelessWindowHint);
    }
    requestActivate();

    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
    }

    setKeyboardGrabEnabled(true);
    KWindowEffects::enableBlurBehind(this, true);
}

void KSMShutdownDlg::resizeEvent(QResizeEvent *e)
{
    PlasmaQuick::QuickViewSharedEngine::resizeEvent(e);

    if (KX11Extras::compositingActive()) {
        // TODO: reenable window mask when we are without composite?
        //        clearMask();
    } else {
        //        setMask(m_view->mask());
    }
}
