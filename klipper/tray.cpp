/*

    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tray.h"

#include <KLocalizedString>

#include "history.h"
#include "historyitem.h"
#include "klipper.h"
#include "klipperpopup.h"

KlipperTray::KlipperTray()
    : KStatusNotifierItem()
{
    setTitle(i18n("Klipper"));
    const QString klipperIconName = QStringLiteral("klipper");
    setIconByName(klipperIconName);
    setToolTip(klipperIconName, i18n("Clipboard Contents"), i18n("Clipboard is empty"));
    setCategory(SystemServices);
    setStatus(Active);
    setStandardActionsEnabled(false);

    m_klipper = new Klipper(this, KSharedConfig::openConfig());
    setContextMenu(m_klipper->actionsPopup());
    setAssociatedWidget(m_klipper->popup());
    connect(m_klipper->history(), &History::changed, this, &KlipperTray::slotSetToolTipFromHistory);
    slotSetToolTipFromHistory();
}

KlipperTray::~KlipperTray()
{
    // Klipper abuses the KStatusNotifierItem slightly by setting both
    // the associated widget and the context menu to the same widget,
    // the KlipperPopup.  This is done so that either a left click or a
    // right click on the icon brings up the combined menu.  Unfortunately
    // this causes a crash in ~KStatusNotifierItem() when it first
    // deletes the menu and then tries to disconnect the associated widget.
    // Work around this by resetting the associated widget first.
    setAssociatedWidget(nullptr);
}

void KlipperTray::slotSetToolTipFromHistory()
{
    const int TOOLTIP_LENGTH_LIMIT = 200;
    if (m_klipper->history()->empty()) {
        setToolTipSubTitle(i18n("Clipboard is empty"));
    } else {
        HistoryItemConstPtr top = m_klipper->history()->first();
        if (top->text().length() <= TOOLTIP_LENGTH_LIMIT) {
            setToolTipSubTitle(top->text());
        } else {
            setToolTipSubTitle(top->text().left(TOOLTIP_LENGTH_LIMIT - 1) + QStringLiteral("…"));
        }
    }
}
