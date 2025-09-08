/*
    SPDX-FileCopyrightText: 2024 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "donationmessage.h"
#include "donationmessage_debug.h"

#include <QDate>
#include <QLoggingCategory>
#include <QStandardPaths>

#include <KConfigGroup>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>
#include <KSharedConfig>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(DonationMessage, "donationmessage.json")

// Otherwise KSharedConfig::openStateConfig() puts it in ~/.local/share/kded6
static const QString STATE_FILE_NAME = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + u"/plasmashellstaterc"_s;
static const QString DATE_FORMAT = u"yyyy-MM-dd"_s;
static const QString GENERAL_GROUP_NAME = u"General"_s;
static const QString SYSTEM_INSTALL_DATE_KEY_NAME = u"SystemInstallDate"_s;
static const QString DONATION_GROUP_NAME = u"ShowDonationMessage"_s;
static const QString LAST_DONATION_KEY_NAME = u"yearLastSuppressedOrDonated"_s;
static const int SHOW_IN_MONTH = 12;
static const int MIN_INSTALLED_DAYS_BEFORE_ASKING = 14;
static const QUrl DONATION_URL = QUrl(u"https://kde.org/donate/?app=plasma-donation-request-notification"_s);

DonationMessage::DonationMessage(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    const QDate currentDate = QDate::currentDate();
    const int currentYear = currentDate.year();
    const int currentMonth = currentDate.month();

    KSharedConfigPtr config = KSharedConfig::openStateConfig(STATE_FILE_NAME);

    KConfigGroup generalGroup(config, GENERAL_GROUP_NAME);
    const QString systemInstallDate = generalGroup.readEntry(SYSTEM_INSTALL_DATE_KEY_NAME, QString());

    if (systemInstallDate.isEmpty()) {
        const QString systemInstallDateString = currentDate.toString(DATE_FORMAT);
        generalGroup.writeEntry(SYSTEM_INSTALL_DATE_KEY_NAME, systemInstallDateString);

        qCDebug(DONATION_MESSAGE) << "User just installed the system and has not had time to form an impression of it yet; not asking for a donation now.";
        return;
    }

    if (QDate::fromString(systemInstallDate, DATE_FORMAT).daysTo(currentDate) < MIN_INSTALLED_DAYS_BEFORE_ASKING) {
        qCDebug(DONATION_MESSAGE) << "User hasn't even been using the system for 2 weeks and has probably not had time to form an impression of it yet; not "
                                     "asking for a donation now.";
        return;
    }

    KConfigGroup donationGroup(config, DONATION_GROUP_NAME);
    const QString yearLastSuppressedOrDonated = donationGroup.readEntry(LAST_DONATION_KEY_NAME, QString());

    if (yearLastSuppressedOrDonated.toInt() == currentYear) {
        qCDebug(DONATION_MESSAGE) << "Already suppressed the donation message or donated this year; not asking again now.";
        return;
    }

    if (currentMonth != SHOW_IN_MONTH) {
        qCDebug(DONATION_MESSAGE) << "Haven't asked for a donation yet in" << currentYear << "but it's not month" << SHOW_IN_MONTH << "; not asking now.";
        return;
    }

    qCDebug(DONATION_MESSAGE) << "Haven't suppressed the donation message or donated this year, and it's month" << SHOW_IN_MONTH << "; asking now!";

    auto *notification = new KNotification(u"ShowDonationMessage"_s);
    notification->setComponentName(u"donationmessage"_s);
    notification->setFlags(KNotification::NotificationFlag::Persistent);
    notification->setTitle(i18nc("@title of a system notification", "Donate to KDE"));
    notification->setText(i18nc("@info body text of a system notification",
                                "KDE needs your help! Donations fund development and infrastructure important for KDE's continued existence."));

    const QString donateActionText = i18nc("@action:button Make a donation now; translate this in as short a form as possible", "Donate");
    const QString dontShowAgainActionText = i18nc("@action:button don't donate this year; translate this in as short a form as possible", "No Thanks");

    const KNotificationAction *defaultDonateAction = notification->addDefaultAction(donateActionText);
    connect(defaultDonateAction, &KNotificationAction::activated, this, &DonationMessage::donate);

    const KNotificationAction *donateAction = notification->addAction(donateActionText);
    connect(donateAction, &KNotificationAction::activated, this, &DonationMessage::donate);

    const KNotificationAction *dontShowAgainAction = notification->addAction(dontShowAgainActionText);
    connect(dontShowAgainAction, &KNotificationAction::activated, notification, &KNotification::close);

    // Everything you can do with the notification ends up here
    connect(notification, &KNotification::closed, this, &DonationMessage::suppressForThisYear);

    notification->sendEvent();
}

void DonationMessage::donate()
{
    qCDebug(DONATION_MESSAGE) << "Woohoo, the user wants to make a donation!";

    auto *job = new KIO::OpenUrlJob(DONATION_URL);
    job->start();
}

void DonationMessage::suppressForThisYear()
{
    qCDebug(DONATION_MESSAGE) << "Not showing a donation message again this year.";

    KSharedConfigPtr config = KSharedConfig::openStateConfig(STATE_FILE_NAME);
    KConfigGroup group(config, DONATION_GROUP_NAME);
    group.writeEntry(LAST_DONATION_KEY_NAME, QDate::currentDate().year());
}

#include "donationmessage.moc"

#include "moc_donationmessage.cpp"
