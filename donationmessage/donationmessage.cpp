/*
    SPDX-FileCopyrightText: 2024 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "donationmessage.h"
#include "donationmessage_debug.h"

#include <QDate>
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
static const QString FILE_NAME = QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + u"/plasmashellstaterc"_s;
static const QString GROUP_NAME = u"ShowDonationMessage"_s;
static const QString KEY_NAME = u"yearLastSuppressedOrDonated"_s;
static const int SHOW_IN_MONTH = 12;

DonationMessage::DonationMessage(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    KSharedConfigPtr config = KSharedConfig::openStateConfig(FILE_NAME);
    KConfigGroup group(config, GROUP_NAME);

    const QString yearLastSuppressedOrDonated = group.readEntry(KEY_NAME, QString());

    const QDate currentDate = QDate::currentDate();
    const int currentMonth = currentDate.month();
    const int currentYear = currentDate.year();

    if (yearLastSuppressedOrDonated.toInt() == currentYear) {
        qCDebug(DONATION_MESSAGE) << "Already suppressed the donation message or donated this year; not asking again now.";
        return;
    }

    if (currentMonth != SHOW_IN_MONTH) {
        qCDebug(DONATION_MESSAGE) << "Haven't asked for a donation yet in" << currentYear << "but it's not month" << SHOW_IN_MONTH << "; not asking now.";
        return;
    }

    qCDebug(DONATION_MESSAGE) << "Haven't suppressed the donation message or donated this year, and it's month" << SHOW_IN_MONTH << "; asking now!";

    KNotification *notification = new KNotification(u"ShowDonationMessage"_s);
    notification->setComponentName(u"donationmessage"_s);
    notification->setFlags(KNotification::NotificationFlag::Persistent);
    notification->setTitle(i18nc("@title of a system notification", "Donate to KDE"));
    notification->setText(i18nc("@info body text of a system notification",
                                "KDE needs your help! Donations fund development and infrastructure important for KDE's continued existence."));

    const QString donateActionText = i18nc("@action:button translate this in as short a form as possible", "Donate Now");
    const QString dontShowAgainActionText = i18nc("@action:button don't donate this year; translate this in as short a form as possible", "No Thanks");

    const KNotificationAction *defaultDonateAction = notification->addDefaultAction(donateActionText);
    connect(defaultDonateAction, &KNotificationAction::activated, this, &DonationMessage::donate);

    const KNotificationAction *donateAction = notification->addAction(donateActionText);
    connect(donateAction, &KNotificationAction::activated, this, &DonationMessage::donate);

    const KNotificationAction *dontShowAgainAction = notification->addAction(dontShowAgainActionText);
    connect(dontShowAgainAction, &KNotificationAction::activated, this, [=] {
        notification->close();
    });

    // Everything you can do with the notification ends up here
    connect(notification, &KNotification::closed, this, &DonationMessage::suppressMessageForThisYear);

    notification->sendEvent();
}

void DonationMessage::donate()
{
    qCDebug(DONATION_MESSAGE) << "Woohoo, the user wants to make a donation!";

    const QUrl url(u"https://kde.org/community/donations/?app=plasma-donation-request-notification"_s);
    auto *job = new KIO::OpenUrlJob(url);
    job->start();
}

void DonationMessage::suppressMessageForThisYear()
{
    qCDebug(DONATION_MESSAGE) << "Not showing a donation message again this year.";

    KSharedConfigPtr config = KSharedConfig::openStateConfig(FILE_NAME);
    KConfigGroup group(config, GROUP_NAME);

    QDate currentDate = QDate::currentDate();
    group.writeEntry(KEY_NAME, currentDate.year());
}

#include "donationmessage.moc"
