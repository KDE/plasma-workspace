/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "abstractentry.h"

#include <KService>
#include <KServiceGroup>

#include <QPointer>

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

    QIcon icon() const override;
    QString name() const override;
    QString description() const override;
    KService::Ptr service() const;
    QString group() const override;

    QString id() const override;
    QUrl url() const override;

    bool hasActions() const override;
    QVariantList actions() const override;

    bool run(const QString &actionId = QString(), const QVariant &argument = QVariant()) override;

    QString menuId() const;

    static QString nameFromService(const KService::Ptr &service, NameFormat nameFormat);
    static KService::Ptr defaultAppByName(const QString &name);

private:
    void init(NameFormat nameFormat);

    QString m_id;
    QString m_name;
    QString m_description;
    // Not an actual group name, but the first character for transliterated name.
    mutable QString m_group;
    mutable QIcon m_icon;
    KService::Ptr m_service;
    static MenuEntryEditor *m_menuEntryEditor;
    QMetaObject::Connection m_con;
};

class AppGroupEntry : public AbstractGroupEntry
{
public:
    AppGroupEntry(AppsModel *parentModel, KServiceGroup::Ptr group, bool paginate, int pageSize, bool flat, bool sorted, bool separators, int appNameFormat);

    QIcon icon() const override;
    QString name() const override;

    bool hasChildren() const override;
    AbstractModel *childModel() const override;

private:
    KServiceGroup::Ptr m_group;
    mutable QIcon m_icon;
    QPointer<AbstractModel> m_childModel;
};
