/*
 * Copyright 2013  Bhushan Shah <bhush94@gmail.com>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef ICONPRIVATE_H
#define ICONPRIVATE_H

#include <QObject>
#include <QUrl>

class QJsonArray;

class IconPrivate : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString icon READ icon NOTIFY iconChanged)
    Q_PROPERTY(QString genericName READ genericName NOTIFY genericNameChanged)
    Q_PROPERTY(QVariantList jumpListActions READ jumpListActions NOTIFY jumpListActionsChanged)

public:
    IconPrivate();
    ~IconPrivate() override;

    QUrl url() const;
    QString name() const;
    QString icon() const;
    QString genericName() const;
    QVariantList jumpListActions() const;

    void setUrl(const QUrl &url);

    Q_INVOKABLE void open();
    Q_INVOKABLE bool processDrop(QObject *dropEvent);
    Q_INVOKABLE void execJumpList(int index);

Q_SIGNALS:
    void urlChanged(QUrl newUrl);
    void nameChanged(QString newName);
    void iconChanged(QString newIcon);
    void genericNameChanged(QString newGenericName);
    void jumpListActionsChanged(const QVariantList &jumpListActions);

private:
    void setUrlInternal(const QUrl &url);

    QUrl m_url;
    QString m_name;
    QString m_icon;
    QString m_genericName;
    QVariantList m_jumpListActions;

};

#endif
