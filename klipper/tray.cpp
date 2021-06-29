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
    setContextMenu(m_klipper->popup());
    setAssociatedWidget(m_klipper->popup());
    connect(m_klipper->history(), &History::changed, this, &KlipperTray::slotSetToolTipFromHistory);
    slotSetToolTipFromHistory();
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
