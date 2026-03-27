/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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

QByteArray ClipboardMenu::timezone() const
{
    return m_timezone;
}

void ClipboardMenu::setTimezone(const QByteArray &timezone)
{
    if (m_timezone != timezone) {
        m_timezone = timezone;
        Q_EMIT timezoneChanged();
    }
}

bool ClipboardMenu::secondsIncluded() const
{
    return m_secondsIncluded;
}

void ClipboardMenu::setSecondsIncluded(bool secondsIncluded)
{
    if (m_secondsIncluded != secondsIncluded) {
        m_secondsIncluded = secondsIncluded;
        Q_EMIT secondsIncludedChanged();
    }
}

void ClipboardMenu::setupMenu(QAction *action)
{
    auto *menu = new QMenu;

    connect(menu, &QMenu::aboutToShow, this, [this, menu] {
        menu->clear();
        QDateTime currentDateTime = QDateTime::currentDateTime();
        if (!m_timezone.isEmpty()) {
            currentDateTime.setTimeZone(QTimeZone(m_timezone));
        }
        if (!m_secondsIncluded) {
            QTime timeWithoutSeconds(currentDateTime.time().hour(), currentDateTime.time().minute(), 0);
            currentDateTime.setTime(timeWithoutSeconds);
        }

        const QDate date = currentDateTime.date();
        const QTime time = currentDateTime.time();
        const QRegularExpression rx(QStringLiteral("[^0-9:]"));
        const QChar spaceCharacter = QLatin1Char(' ');
        QString timeString;
        QAction *menuOption;

        // e.g 12:30 PM or 12:30:01 PM
        timeString = m_secondsIncluded ? QLocale::system().toString(time, QLocale::LongFormat) : QLocale::system().toString(time, QLocale::ShortFormat);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        // the same as the option above but shows the opposite of the "show seconds" setting
        // e.g if "show seconds" is enabled it will show the time without seconds and vice-versa
        timeString = m_secondsIncluded ? QLocale::system().toString(time, QLocale::ShortFormat) : QLocale::system().toString(time, QLocale::LongFormat);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        // e.g 4/28/22
        timeString = QLocale::system().toString(date, QLocale::ShortFormat);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        // e.g Thursday, April 28, 2022
        timeString = QLocale::system().toString(date, QLocale::LongFormat);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        // e.g Thursday, April 28, 2022 12:30 PM or Thursday, April 28, 2022 12:30:01 PM -03
        timeString = m_secondsIncluded
            ? QLocale::system().toString(date, QLocale::LongFormat) + spaceCharacter + QLocale::system().toString(time, QLocale::LongFormat)
            : QLocale::system().toString(date, QLocale::LongFormat) + spaceCharacter + QLocale::system().toString(time, QLocale::ShortFormat);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        // e.g 2022-04-28
        timeString = date.toString(Qt::ISODate);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        // e.g 2022-04-28 12:30 PM or 2022-04-28 12:30:01 PM -03
        timeString = m_secondsIncluded ? date.toString(Qt::ISODate) + spaceCharacter + QLocale::system().toString(time, QLocale::LongFormat)
                                       : date.toString(Qt::ISODate) + spaceCharacter + QLocale::system().toString(time, QLocale::ShortFormat);
        menuOption = menu->addAction(timeString);
        menuOption->setData(timeString);

        menu->addSeparator();

        QMenu *otherCalendarsMenu = menu->addMenu(i18n("Other Calendars"));

        /* Add ICU Calendars if QLocale is ready for
                Chinese, Coptic, Ethiopic, (Gregorian), Hebrew, Indian, Islamic, Persian

                otherCalendarsMenu->addSeparator();
        */
        timeString = QString::number(currentDateTime.toMSecsSinceEpoch() / 1000);
        menuOption = otherCalendarsMenu->addAction(i18nc("unix timestamp (seconds since 1.1.1970)", "%1 (UNIX Time)", timeString));
        menuOption->setData(timeString);
        timeString = QString::number(qreal(2440587.5) + qreal(currentDateTime.toMSecsSinceEpoch()) / qreal(86400000), 'f', 5);
        menuOption = otherCalendarsMenu->addAction(i18nc("for astronomers (days and decimals since ~7000 years ago)", "%1 (Julian Date)", timeString));
        menuOption->setData(timeString);
    });

    connect(menu, &QMenu::triggered, menu, [](QAction *action) {
        qApp->clipboard()->setText(action->data().toString());
        qApp->clipboard()->setText(action->data().toString(), QClipboard::Selection);
    });

    // QMenu cannot have QAction as parent and setMenu doesn't take ownership
    connect(action, &QObject::destroyed, menu, &QObject::deleteLater);

    action->setMenu(menu);
}

#include "moc_clipboardmenu.cpp"
