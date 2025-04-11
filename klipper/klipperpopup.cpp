/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "klipperpopup.h"

#include <QQmlEngine>
#include <QQuickItem>

#include <KLocalizedString>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/surface.h>
#include <KWindowSystem>
#include <KX11Extras>

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

void KlipperPopup::onRequestResizePopup()
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
    if (KWindowSystem::isPlatformX11()) {
        const QList<QScreen *> screens = QGuiApplication::screens();
        auto screenIt = std::find_if(screens.cbegin(), screens.cend(), [](QScreen *screen) {
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
    }
}

void KlipperPopup::onObjectIncubated()
{
    auto item = qobject_cast<QQuickItem *>(m_engine.rootObject());
    setMainItem(item);

    connect(this, &KlipperPopup::paddingChanged, this, &KlipperPopup::onRequestResizePopup);
    connect(item, SIGNAL(requestResizePopup()), this, SLOT(onRequestResizePopup()));

    connect(item, SIGNAL(requestHidePopup()), this, SLOT(hide()));
}

void KlipperPopup::onFocusWindowChanged(QWindow *focusWindow)
{
    if (focusWindow != this && (!focusWindow || focusWindow->objectName() != u"klipperActionPopupWindow")) {
        hide();
    }
}

#include "moc_klipperpopup.cpp"
