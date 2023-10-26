/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "notificationapplet.h"

#include <QGuiApplication>
#include <QJSEngine>
#include <QJSValue>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>

#include <KX11Extras>

#include <Plasma/Containment>
#include <PlasmaQuick/AppletQuickItem>
#include <PlasmaQuick/Dialog>
#include <PlasmaQuick/PlasmaWindow>

#include "fileinfo.h"
#include "filemenu.h"
#include "globalshortcuts.h"
#include "jobaggregator.h"
#include "thumbnailer.h"
#include "wheelforwarder.h"

class InputDisabler
{
    Q_GADGET

public:
    Q_INVOKABLE void makeTransparentForInput(QQuickItem *item)
    {
        if (item) {
            item->setAcceptedMouseButtons(Qt::NoButton);
            item->setAcceptHoverEvents(false);
            item->setAcceptTouchEvents(false);
            item->unsetCursor();
        }
    }
};

NotificationApplet::NotificationApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Applet(parent, data, args)
{
    static bool s_typesRegistered = false;
    if (!s_typesRegistered) {
        const char uri[] = "org.kde.plasma.private.notifications";
        qmlRegisterType<FileInfo>(uri, 2, 0, "FileInfo");
        qmlRegisterType<FileMenu>(uri, 2, 0, "FileMenu");
        qmlRegisterType<GlobalShortcuts>(uri, 2, 0, "GlobalShortcuts");
        qmlRegisterType<JobAggregator>(uri, 2, 0, "JobAggregator");
        qmlRegisterType<Thumbnailer>(uri, 2, 0, "Thumbnailer");
        qmlRegisterType<WheelForwarder>(uri, 2, 0, "WheelForwarder");
        qmlRegisterSingletonType(uri, 2, 0, "InputDisabler", [](QQmlEngine *, QJSEngine *jsEngine) -> QJSValue {
            return jsEngine->toScriptValue(InputDisabler());
        });
        qmlProtectModule(uri, 2);
        s_typesRegistered = true;
    }

    connect(qApp, &QGuiApplication::focusWindowChanged, this, &NotificationApplet::focussedPlasmaDialogChanged);
}

NotificationApplet::~NotificationApplet() = default;

void NotificationApplet::init()
{
}

void NotificationApplet::configChanged()
{
}

static bool isPlasmaWindow(QWindow *window)
{
    if (qobject_cast<PlasmaQuick::Dialog *>(window) || qobject_cast<PlasmaQuick::PlasmaWindow *>(window)) {
        return true;
    }
    return false;
}

QWindow *NotificationApplet::focussedPlasmaDialog() const
{
    auto *focusWindow = qApp->focusWindow();
    if (isPlasmaWindow(focusWindow)) {
        return focusWindow;
    }
    if (focusWindow) {
        if (isPlasmaWindow(focusWindow->transientParent())) {
            return focusWindow->transientParent();
        }
    }

    return nullptr;
}

QQuickItem *NotificationApplet::systemTrayRepresentation() const
{
    auto *c = containment();
    if (!c) {
        return nullptr;
    }

    if (strcmp(c->metaObject()->className(), "SystemTray") != 0) {
        return nullptr;
    }

    return PlasmaQuick::AppletQuickItem::itemForApplet(c);
}

bool NotificationApplet::isPrimaryScreen(const QRect &rect) const
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return false;
    }

    // HACK
    return rect == screen->geometry();
}

void NotificationApplet::forceActivateWindow(QWindow *window)
{
    if (window && window->winId()) {
        KX11Extras::forceActiveWindow(window->winId());
    }
}

K_PLUGIN_CLASS(NotificationApplet)

#include "notificationapplet.moc"
