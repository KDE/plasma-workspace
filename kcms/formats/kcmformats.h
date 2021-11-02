/*
    kcmformats.h
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KConfigGroup>
#include <KQuickAddons/ManagedConfigModule>

class FormatsSettings;
class OptionsModel;
class KCMFormats : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT
    Q_PROPERTY(FormatsSettings *settings READ settings CONSTANT)
    Q_PROPERTY(OptionsModel *optionsModel READ optionsModel CONSTANT)
public:
    explicit KCMFormats(QObject *parent, const KPluginMetaData &data, const QVariantList &list = QVariantList());
    virtual ~KCMFormats() override = default;

    FormatsSettings *settings() const;
    OptionsModel *optionsModel() const;
    Q_INVOKABLE QQuickItem *getSubPage(int index) const; // proxy from KQuickAddons to Qml
    Q_INVOKABLE void unset(const QString &setting);

private:
    QHash<QString, QString> m_cachedFlags;

    OptionsModel *m_optionsModel = nullptr;
};
