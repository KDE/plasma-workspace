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
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>
#include <KX11Extras>
#include <algorithm>

#include "historymodel.h"
#include "klipper.h"

using namespace Qt::StringLiterals;

KlipperPopup::KlipperPopup()
    : PlasmaQuick::PlasmaWindow()
    , m_model(HistoryModel::self())
{
    m_engine.engine()->setProperty("_kirigamiTheme", u"KirigamiPlasmaStyle"_s);
    m_engine.setInitializationDelayed(true);

    // used only by screen readers
    setTitle(i18n("Clipboard Popup"));

    connect(&m_engine, &PlasmaQuick::SharedQmlEngine::finished, this, &KlipperPopup::onObjectIncubated);
    m_engine.setSourceFromModule(u"org.kde.plasma.private.clipboard", u"KlipperPopup");
    m_engine.completeInitialization();

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &KlipperPopup::onFocusWindowChanged);
}

void KlipperPopup::show()
{
    if (m_plasmashell) {
        hide();
    }
    positionOnScreen();
    QMetaObject::invokeMethod(mainItem(), "updateContentSize", Q_ARG(QSizeF, screen()->availableSize().toSizeF()));
    resizePopup();
    setVisible(true);
}

void KlipperPopup::setPlasmaShell(KWayland::Client::PlasmaShell *plasmashell)
{
    m_plasmashell = plasmashell;
}

void KlipperPopup::editCurrentClipboard()
{
    if (!isVisible()) {
        show();
    }
    QMetaObject::invokeMethod(mainItem(), "editClipboardContent", Q_ARG(int, 0));
}

void KlipperPopup::hide()
{
    QWindow::hide();
    if (m_plasmashell) {
        destroy(); // Required to recreate wl_surface
    }
}

void KlipperPopup::resizePopup()
{
    // If the popup is off-screen, move it to the closest edge of the screen
    const QSize popupSize = QSize(mainItem()->implicitWidth(), mainItem()->implicitHeight()).grownBy(padding()).boundedTo(screen()->availableSize());

    if (KWindowSystem::isPlatformX11()) {
        const QRect screenGeometry = screen()->geometry();
        QRect popupGeometry(position(), popupSize);
        if (!screenGeometry.contains(popupGeometry)) {
            popupGeometry.moveTo(std::clamp(x(), screenGeometry.left(), screenGeometry.right() - popupSize.width()),
                                 std::clamp(y(), screenGeometry.top(), screenGeometry.bottom() - popupSize.height()));
        }
        setGeometry(popupGeometry);
    } else {
        resize(popupSize);
    }
}

void KlipperPopup::showEvent(QShowEvent *event)
{
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setOnAllDesktops(winId(), true);
    }
    PlasmaWindow::showEvent(event); // NET::SkipTaskbar | NET::SkipPager | NET::SkipSwitcher
    requestActivate();
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::forceActiveWindow(winId());
    }
}

void KlipperPopup::positionOnScreen()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (KWindowSystem::isPlatformX11()) {
        auto screenIt = std::ranges::find_if(screens, [](QScreen *screen) {
            return screen->geometry().contains(QCursor::pos(screen));
        });
        QScreen *const shownOnScreen = screenIt != screens.cend() ? *screenIt : QGuiApplication::primaryScreen();
        setPosition(QCursor::pos(shownOnScreen));
        setScreen(shownOnScreen);
        KX11Extras::setOnDesktop(winId(), KX11Extras::currentDesktop());
    } else if (m_plasmashell && KWindowSystem::isPlatformWayland()) {
        auto surface = KWayland::Client::Surface::fromWindow(this);
        auto plasmaSurface = m_plasmashell->createSurface(surface, this);
        plasmaSurface->openUnderCursor();
        plasmaSurface->setSkipTaskbar(true);
        plasmaSurface->setSkipSwitcher(true);
        plasmaSurface->setRole(KWayland::Client::PlasmaShellSurface::Role::AppletPopup);

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
}

void KlipperPopup::onObjectIncubated()
{
    auto item = qobject_cast<QQuickItem *>(m_engine.rootObject());
    setMainItem(item);

    connect(this, &KlipperPopup::paddingChanged, this, &KlipperPopup::resizePopup);

    connect(item, SIGNAL(requestHidePopup()), this, SLOT(hide()));
}

void KlipperPopup::onFocusWindowChanged(QWindow *focusWindow)
{
    if (focusWindow != this && (!focusWindow || focusWindow->objectName() != u"klipperActionPopupWindow")) {
        hide();
    }
}

#include "moc_klipperpopup.cpp"
