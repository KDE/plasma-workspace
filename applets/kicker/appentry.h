/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractentry.h"

#include <KService>
#include <KServiceAction>
#include <KServiceGroup>

#include <QPointer>

#include <optional>

class AppsModel;
class MenuEntryEditor;

class AppEntry : public AbstractEntry
{
public:
    enum NameFormat {
        NameOnly = 0,
        GenericNameOnly,
        NameAndGenericName,
        GenericNameAndName,
    };

    explicit AppEntry(AbstractModel *owner, KService::Ptr service, NameFormat nameFormat);
    explicit AppEntry(AbstractModel *owner, const QString &id);
    ~AppEntry() override;

    EntryType type() const override
    {
        return RunnableType;
    }

    bool isValid() const override;

    QString icon() const override;
    QString name() const override;
    QString compactName() const override;
    QString description() const override;
    KService::Ptr service() const;
    QString group() const override;

    QString id() const override;
    QUrl url() const override;

    QDate firstSeen() const override;
    void setFirstSeen(const QDate &firstSeen);

    bool isNewlyInstalled() const override;

    bool hasActions() const override;
    QVariantList actions() const override;

    bool run(const QString &actionId = QString(), const QVariant &argument = QVariant()) override;

    QString menuId() const;

    void reload() override;
    void refreshLabels() override;

    static QString nameFromService(const KService::Ptr &service, NameFormat nameFormat);
    static KService::Ptr defaultAppByName(const QString &name);

private:
    void init(NameFormat nameFormat);

    QString m_id;
    QString m_name;
    QString m_compactName;
    QString m_description;
    QDate m_firstSeen;
    // Not an actual group name, but the first character for transliterated name.
    mutable QString m_group;
    mutable QString m_icon;
    KService::Ptr m_service;
    std::optional<KServiceAction> m_serviceAction;
    static MenuEntryEditor *m_menuEntryEditor;
};

class AppGroupEntry : public AbstractGroupEntry
{
public:
    AppGroupEntry(AppsModel *parentModel, KServiceGroup::Ptr group, bool paginate, int pageSize, bool flat, bool sorted, bool separators, int appNameFormat);

    QString icon() const override;
    QString name() const override;
    bool isNewlyInstalled() const override;

    bool hasChildren() const override;
    AbstractModel *childModel() const override;

private:
    KServiceGroup::Ptr m_group;
    mutable QString m_icon;
    QPointer<AbstractModel> m_childModel;
};
