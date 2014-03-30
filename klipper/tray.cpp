/* This file is part of the KDE project

   Copyright (C) by Andrew Stanley-Jones <asj@cban.com>
   Copyright (C) 2000 by Carsten Pfeiffer <pfeiffer@kde.org>
   Copyright (C) 2004  Esben Mose Hansen <kde@mosehansen.dk>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "tray.h"

#include <KGlobal>
#include <KLocalizedString>
#include <KNotification>

#include "klipper.h"
#include "history.h"
#include "klipperpopup.h"

KlipperTray::KlipperTray()
    : KStatusNotifierItem()
{
    setTitle( i18n( "Klipper" ) );
    setIconByName( "klipper" );
    setToolTip( "klipper", i18n( "Clipboard Contents" ), i18n( "Clipboard is empty" ) );
    setCategory( SystemServices );
    setStatus( Active );
    setStandardActionsEnabled( false );

    m_klipper = new Klipper( this, KSharedConfig::openConfig());
    setContextMenu( m_klipper->history()->popup() );
    setAssociatedWidget( m_klipper->history()->popup() );
    connect( m_klipper->history(), SIGNAL(changed()), SLOT(slotSetToolTipFromHistory()));
    slotSetToolTipFromHistory();
    connect( m_klipper, SIGNAL(passivePopup(QString,QString)), SLOT(slotPassivePopup(QString,QString)));
}

void KlipperTray::slotSetToolTipFromHistory()
{
    const int TOOLTIP_LENGTH_LIMIT = 200;
    if (m_klipper->history()->empty()) {
        setToolTipSubTitle( i18n("Clipboard is empty"));
    } else {
        const HistoryItem* top = m_klipper->history()->first();
        if (top->text().length() <= TOOLTIP_LENGTH_LIMIT) {
            setToolTipSubTitle(top->text());
        } else {
            setToolTipSubTitle(top->text().left(TOOLTIP_LENGTH_LIMIT - 3) + "..." );
        }
    }
}

void KlipperTray::slotPassivePopup(const QString& caption, const QString& text)
{
    if (m_notification) {
        m_notification->setTitle(caption);
        m_notification->setText(text);
    } else {
        m_notification = KNotification::event(KNotification::Notification, caption, text,
                                              QIcon::fromTheme("klipper").pixmap(QSize(16, 16)));
    }
}

#include "tray.moc"
