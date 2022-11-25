/*
    SPDX-FileCopyrightText: 2013 Aurélien Gâteau <agateau@kde.org>
    SPDX-FileCopyrightText: 2014-2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QVariant>

#include <KService>

class KFileItem;

namespace Kicker
{
enum {
    DescriptionRole = Qt::UserRole + 1,
    GroupRole,
    FavoriteIdRole,
    IsSeparatorRole,
    IsDropPlaceholderRole,
    IsParentRole,
    HasChildrenRole,
    HasActionListRole,
    ActionListRole,
    UrlRole,
    DisabledRole,
    IsMultilineTextRole,
};

QVariantMap createActionItem(const QString &label, const QString &icon, const QString &actionId, const QVariant &argument = QVariant());

QVariantMap createTitleActionItem(const QString &label);

QVariantMap createSeparatorActionItem();

QVariantList createActionListForFileItem(const KFileItem &fileItem);
bool handleFileItemAction(const KFileItem &fileItem, const QString &actionId, const QVariant &argument, bool *close);

QVariantList createAddLauncherActionList(QObject *appletInterface, const KService::Ptr &service);
bool handleAddLauncherAction(const QString &actionId, QObject *appletInterface, const KService::Ptr &service);

QVariantList jumpListActions(KService::Ptr service);
QVariantList systemSettingsActions();

QVariantList recentDocumentActions(const KService::Ptr &service);
bool handleRecentDocumentAction(KService::Ptr service, const QString &actionId, const QVariant &argument);

bool canEditApplication(const QString &entryPath);
void editApplication(const QString &entryPath, const QString &menuId);
QVariantList editApplicationAction(const KService::Ptr &service);
bool handleEditApplicationAction(const QString &actionId, const KService::Ptr &service);

QVariantList appstreamActions(const KService::Ptr &service);
bool handleAppstreamActions(const QString &actionId, const KService::Ptr &service);

QVariantList additionalAppActions(const KService::Ptr &service);
bool handleAdditionalAppActions(const QString &actionId, const KService::Ptr &service, const QVariant &argument);

QString resolvedServiceEntryPath(const KService::Ptr &service);

}
