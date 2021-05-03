/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
 *   Copyright (C) 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef STATUSNOTIFIERITEMSOURCE_H
#define STATUSNOTIFIERITEMSOURCE_H

#include <Plasma/DataContainer>
#include <QDBusPendingCallWatcher>
#include <QMenu>
#include <QString>

#include "statusnotifieritem_interface.h"

class DBusMenuImporter;
class KIconLoader;

class StatusNotifierItemSource : public QObject
{
    Q_OBJECT

public:
    StatusNotifierItemSource(const QString &service, QObject *parent);
    ~StatusNotifierItemSource() override;
    Plasma::Service *createService();

    void activate(int x, int y);
    void secondaryActivate(int x, int y);
    void scroll(int delta, const QString &direction);
    void contextMenu(int x, int y);

    QIcon attentionIcon() const;
    QString attentionIconName() const;
    QString attentionMovieName() const;
    QString category() const;
    QIcon icon() const;
    QString iconName() const;
    QString iconThemePath() const;
    QString id() const;
    bool itemIsMenu() const;
    QString overlayIconName() const;
    QString status() const;
    QString title() const;
    QVariant toolTipIcon() const;
    QString toolTipSubTitle() const;
    QString toolTipTitle() const;
    QString windowId() const;

Q_SIGNALS:
    void contextMenuReady(QMenu *menu);
    void activateResult(bool success);
    void dataUpdated();

private Q_SLOTS:
    void contextMenuReady();
    void refreshMenu();
    void refresh();
    void performRefresh();
    void syncStatus(const QString &);
    void refreshCallback(QDBusPendingCallWatcher *);
    void activateCallback(QDBusPendingCallWatcher *);

private:
    QPixmap KDbusImageStructToPixmap(const KDbusImageStruct &image) const;
    QIcon imageVectorToPixmap(const KDbusImageVector &vector) const;
    void overlayIcon(QIcon *icon, QIcon *overlay);
    KIconLoader *iconLoader() const;

    bool m_valid;
    QString m_servicename;
    QTimer m_refreshTimer;
    KIconLoader *m_customIconLoader;
    DBusMenuImporter *m_menuImporter;
    org::kde::StatusNotifierItem *m_statusNotifierItemInterface;
    bool m_refreshing : 1;
    bool m_needsReRefreshing : 1;

    QIcon m_attentionIcon;
    QString m_attentionIconName;
    QString m_attentionMovieName;
    QString m_category;
    QIcon m_icon;
    QString m_iconName;
    QString m_iconThemePath;
    QString m_id;
    bool m_itemIsMenu;
    QString m_overlayIconName;
    QString m_status;
    QString m_title;
    QVariant m_toolTipIcon;
    QString m_toolTipSubTitle;
    QString m_toolTipTitle;
    QString m_windowId;
};

#endif // STATUSNOTIFIERITEMSOURCE_H
