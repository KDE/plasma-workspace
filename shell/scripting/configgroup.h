/*
 *   Copyright 2011-2012 by Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef CONFIGGROUP_H
#define CONFIGGROUP_H

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

#endif
