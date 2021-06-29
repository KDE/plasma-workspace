/*
    SPDX-FileCopyrightText: 2011-2012 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QJSValue>
#include <QObject>
#include <QVariant>

#include <KSharedConfig>

class KConfigGroup;
class ConfigGroupPrivate;

class ConfigGroup : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString file READ file WRITE setFile NOTIFY fileChanged)
    Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY groupChanged)
    Q_PROPERTY(QStringList keyList READ keyList NOTIFY keyListChanged)
    Q_PROPERTY(QStringList groupList READ groupList NOTIFY groupListChanged)

public:
    explicit ConfigGroup(QObject *parent = nullptr);
    ~ConfigGroup() override;

    KConfigGroup *configGroup();

    KSharedConfigPtr config() const;
    void setConfig(KSharedConfigPtr config);
    QString file() const;
    void setFile(const QString &filename);
    QString group() const;
    void setGroup(const QString &groupname);
    QStringList keyList() const;
    QStringList groupList() const;

    Q_INVOKABLE QVariant readEntry(const QString &key);
    Q_INVOKABLE bool writeEntry(const QString &key, const QJSValue &value);
    Q_INVOKABLE void deleteEntry(const QString &key);

Q_SIGNALS:
    void fileChanged();
    void groupChanged();
    void keyListChanged();
    void groupListChanged();

private:
    ConfigGroupPrivate *d;

    bool readConfigFile();

private Q_SLOTS:
    void sync();
};
