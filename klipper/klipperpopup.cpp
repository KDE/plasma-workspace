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

void KlipperPopup::hide()
{
    setVisible(false);
    destroy(); // Required to recreate wl_surface
}

void KlipperPopup::showEvent(QShowEvent *event)
{
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setOnAllDesktops(winId(), true);
    }
    QQuickWindow::showEvent(event);
    requestActivate();
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::forceActiveWindow(winId());
    }
}

void KlipperPopup::positionOnScreen()
{
    if (KWindowSystem::isPlatformX11()) {
        setPosition(QCursor::pos());
        KX11Extras::setOnDesktop(winId(), KX11Extras::currentDesktop());
        KX11Extras::setState(winId(), NET::SkipTaskbar | NET::SkipPager);
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

    auto updateSize = [this]() {
        resize(QSize(mainItem()->implicitWidth(), mainItem()->implicitHeight()).grownBy(padding()).boundedTo(screen()->availableSize()));
    };

    connect(item, &QQuickItem::implicitHeightChanged, this, updateSize);
    connect(this, &KlipperPopup::paddingChanged, this, updateSize);
    updateSize();

    connect(item, SIGNAL(requestHidePopup()), this, SLOT(hide()));
}

void KlipperPopup::onFocusWindowChanged(QWindow *focusWindow)
{
    if (focusWindow != this) {
        hide();
    }
}

#include "moc_klipperpopup.cpp"
