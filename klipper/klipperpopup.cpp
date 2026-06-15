/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klipperpopup.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QQmlEngine>
#include <QQuickItem>

#include <KLocalizedString>

#include <Plasma/Plasma>
#include <PlasmaQuick/PlasmaQuick>
#include <PlasmaQuick/PlasmaShellWaylandIntegration>

#include <algorithm>

#include "historymodel.h"
#include "klipper.h"

using namespace Qt::StringLiterals;

KlipperPopup::KlipperPopup()
    : PlasmaQuick::PlasmaWindow()
    , m_model(HistoryModel::self())
    , m_engine(PlasmaQuick::globalEngine())
{
    // used only by screen readers
    setTitle(i18n("Clipboard Popup"));

    QQmlComponent component(m_engine.get(), u"org.kde.plasma.private.clipboard", u"KlipperPopup");

    // Initialize in steps, so QML does not complain
    auto item = qobject_cast<QQuickItem *>(component.beginCreate(m_engine->rootContext()));
    setMainItem(item);
    component.completeCreate();

    connect(this, &KlipperPopup::paddingChanged, this, &KlipperPopup::resizePopup);

    connect(item, SIGNAL(requestHidePopup()), this, SLOT(hide()));

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &KlipperPopup::onFocusWindowChanged);
}

KlipperPopup::~KlipperPopup()
{
    delete mainItem();
}

void KlipperPopup::show()
{
    hide();
    positionOnScreen();
    QMetaObject::invokeMethod(mainItem(), "updateContentSize", Q_ARG(QSizeF, screen()->availableSize().toSizeF()));
    resizePopup();
    setVisible(true);
}

void KlipperPopup::editCurrentClipboard()
{
    if (!isVisible()) {
        show();
    }
    QMetaObject::invokeMethod(mainItem(), "editClipboardContent", Q_ARG(int, 0));
}

void KlipperPopup::showCurrentBarcode()
{
    if (!isVisible()) {
        show();
    }
    QMetaObject::invokeMethod(mainItem(), "showBarcode", Q_ARG(int, 0));
}

void KlipperPopup::hide()
{
    QWindow::hide();
    destroy(); // Required to recreate wl_surface
}

void KlipperPopup::resizePopup()
{
    // If the popup is off-screen, move it to the closest edge of the screen
    const QSize popupSize = QSize(mainItem()->implicitWidth(), mainItem()->implicitHeight()).grownBy(padding()).boundedTo(screen()->availableSize());

    resize(popupSize);
}

void KlipperPopup::showEvent(QShowEvent *event)
{
    PlasmaWindow::showEvent(event); // NET::SkipTaskbar | NET::SkipPager | NET::SkipSwitcher
    requestActivate();
}

void KlipperPopup::positionOnScreen()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    auto integration = PlasmaShellWaylandIntegration::get(this);
    integration->openUnderCursor();
    integration->setRole(QtWayland::org_kde_plasma_surface::role_appletpopup);

    if (screens.size() > 1) {
        auto message = QDBusMessage::createMethodCall(u"org.kde.KWin"_s, u"/KWin"_s, u"org.kde.KWin"_s, u"activeOutputName"_s);
        QDBusReply<QString> reply = QDBusConnection::sessionBus().call(message);
        if (reply.isValid()) {
            const QString activeOutputName = reply.value();
            auto screenIt = std::ranges::find_if(screens, [&activeOutputName](QScreen *screen) {
                return screen->name() == activeOutputName;
            });
            setScreen(screenIt != screens.cend() ? *screenIt : QGuiApplication::primaryScreen());
        }
    }
}

void KlipperPopup::onFocusWindowChanged(QWindow *focusWindow)
{
    if (focusWindow != this && (!focusWindow || focusWindow->objectName() != u"klipperActionPopupWindow")) {
        hide();
    }
}

#include "moc_klipperpopup.cpp"
