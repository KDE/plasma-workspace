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

#pragma once

#include <Plasma/Applet>

#include <QPointer>

#include <KPropertiesDialog>

class IconApplet : public Plasma::Applet
{
    Q_OBJECT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(QString genericName READ genericName NOTIFY genericNameChanged)
    Q_PROPERTY(QVariantList jumpListActions READ jumpListActions NOTIFY jumpListActionsChanged)

public:
    explicit IconApplet(QObject *parent, const QVariantList &data);
    ~IconApplet() override;

    void init() override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString name() const;
    QString iconName() const;
    QString genericName() const;
    QVariantList jumpListActions() const;

    Q_INVOKABLE void open();
    Q_INVOKABLE void processDrop(QObject *dropEvent);
    Q_INVOKABLE void execJumpList(int index);
    Q_INVOKABLE void configure();

signals:
    void urlChanged(const QUrl &url);

    void nameChanged(const QString &name);
    void iconNameChanged(const QString &iconName);
    void genericNameChanged(const QString &genericName);
    void jumpListActionsChanged(const QVariantList &jumpListActions);

private:
    void setIconName(const QString &iconName);

    void makeExecutable(const QString &filePath);

    void populate();
    void populateFromDesktopFile(const QString &path);

    QString localPath() const;
    void setLocalPath(const QString &localPath);

    QUrl m_url;
    QString m_localPath;

    QString m_name;
    QString m_iconName;
    QString m_genericName;
    QVariantList m_jumpListActions;

    QPointer<KPropertiesDialog> m_configDialog;

};
