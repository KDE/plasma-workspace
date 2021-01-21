/*
 * Copyright 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "clipboardmenu.h"
#include "klocalizedstring.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMenu>
#include <QRegularExpression>

ClipboardMenu::ClipboardMenu(QObject *parent)
    : QObject(parent)
{
}

ClipboardMenu::~ClipboardMenu() = default;

QDateTime ClipboardMenu::currentDate() const
{
    return m_currentDate;
}

void ClipboardMenu::setCurrentDate(const QDateTime &currentDate)
{
    if (m_currentDate != currentDate) {
        m_currentDate = currentDate;
        emit currentDateChanged();
    }
}

bool ClipboardMenu::secondsIncluded() const
{
    return m_secondsIncluded;
}

void ClipboardMenu::setSecondsIncluded(const bool &secondsIncluded)
{
    if (m_secondsIncluded != secondsIncluded) {
        m_secondsIncluded = secondsIncluded;
        emit secondsIncludedChanged();
    }
}

void ClipboardMenu::setupMenu(QAction *action)
{
    QMenu *menu = new QMenu;

    /*
     * The refresh rate of m_currentDate depends of what is shown in the plasmoid.
     * If only minutes are shown, the value is updated at the full minute and
     * seconds are always 0 or 59 and thus useless/confusing to offer for copy.
     * Use a reference to the config's showSeconds to decide if seconds are sent
     * to the clipboard. There was no workaround found ...
     */
    connect(menu, &QMenu::aboutToShow, this, [this, menu] {
        menu->clear();

        const QDate date = m_currentDate.date();
        const QTime time = m_currentDate.time();
        const QRegularExpression rx("[^0-9:]");
        const QChar ws = QLatin1Char(' ');
        QString s;
        QAction *a;

        s = date.toString(Qt::SystemLocaleShortDate);
        a = menu->addAction(s);
        a->setData(s);
        s = date.toString(Qt::ISODate);
        a = menu->addAction(s);
        a->setData(s);
        s = date.toString(Qt::SystemLocaleLongDate);
        a = menu->addAction(s);
        a->setData(s);

        menu->addSeparator();

        s = time.toString(Qt::SystemLocaleShortDate);
        a = menu->addAction(s);
        a->setData(s);
        if (m_secondsIncluded) {
            s = time.toString(Qt::SystemLocaleLongDate);
            s.remove(rx);
            a = menu->addAction(s);
            a->setData(s);
            s = time.toString(Qt::SystemLocaleLongDate);
            a = menu->addAction(s);
            a->setData(s);
        }

        menu->addSeparator();

        s = date.toString(Qt::SystemLocaleShortDate) + ws + time.toString(Qt::SystemLocaleShortDate);
        a = menu->addAction(s);
        a->setData(s);
        if (m_secondsIncluded) {
            s = date.toString(Qt::SystemLocaleShortDate) + ws + time.toString(Qt::SystemLocaleLongDate).remove(rx);
            a = menu->addAction(s);
            a->setData(s);
            s = date.toString(Qt::SystemLocaleShortDate) + ws + time.toString(Qt::SystemLocaleLongDate);
            a = menu->addAction(s);
            a->setData(s);
        }
        s = date.toString(Qt::ISODate) + ws + time.toString(Qt::SystemLocaleShortDate);
        a = menu->addAction(s);
        a->setData(s);
        if (m_secondsIncluded) {
            s = date.toString(Qt::ISODate) + ws + time.toString(Qt::SystemLocaleLongDate).remove(rx);
            a = menu->addAction(s);
            a->setData(s);
            s = date.toString(Qt::ISODate) + ws + time.toString(Qt::SystemLocaleLongDate);
            a = menu->addAction(s);
            a->setData(s);
        }
        s = date.toString(Qt::SystemLocaleLongDate) + ws + time.toString(Qt::SystemLocaleShortDate);
        a = menu->addAction(s);
        a->setData(s);

        menu->addSeparator();

        QMenu *otherCalendarsMenu = menu->addMenu(i18n("Other Calendars"));

        /* Add ICU Calendars if QLocale is ready for
                Chinese, Coptic, Ethiopic, (Gregorian), Hebrew, Indian, Islamic, Persian

                otherCalendarsMenu->addSeparator();
        */
        s = QString::number(m_currentDate.toMSecsSinceEpoch() / 1000);
        a = otherCalendarsMenu->addAction(i18nc("unix timestamp (seconds since 1.1.1970)", "%1 (UNIX Time)", s));
        a->setData(s);
        s = QString::number(qreal(2440587.5) + qreal(m_currentDate.toMSecsSinceEpoch()) / qreal(86400000), 'f', 5);
        a = otherCalendarsMenu->addAction(i18nc("for astronomers (days and decimals since ~7000 years ago)", "%1 (Julian Date)", s));
        a->setData(s);
    });

    connect(menu, &QMenu::triggered, menu, [](QAction *action) {
        qApp->clipboard()->setText(action->data().toString());
        qApp->clipboard()->setText(action->data().toString(), QClipboard::Selection);
    });

    // QMenu cannot have QAction as parent and setMenu doesn't take ownership
    connect(action, &QObject::destroyed, menu, &QObject::deleteLater);

    action->setMenu(menu);
}
