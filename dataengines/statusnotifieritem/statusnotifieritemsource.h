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

class KIconLoader;

class DBusMenuImporter;

class StatusNotifierItemSource : public Plasma::DataContainer
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

Q_SIGNALS:
    void contextMenuReady(QMenu *menu);
    void activateResult(bool success);

private Q_SLOTS:
    void contextMenuReady();
    void refreshTitle();
    void refreshIcons();
    void refreshToolTip();
    void refreshMenu();
    void refresh();
    void performRefresh();
    void syncStatus(QString);
    void refreshCallback(QDBusPendingCallWatcher *);
    void activateCallback(QDBusPendingCallWatcher *);

private:
    QPixmap KDbusImageStructToPixmap(const KDbusImageStruct &image) const;
    QIcon imageVectorToPixmap(const KDbusImageVector &vector) const;
    void overlayIcon(QIcon *icon, QIcon *overlay);
    KIconLoader *iconLoader() const;

    bool m_valid;
    QString m_typeId;
    QString m_name;
    QTimer m_refreshTimer;
    KIconLoader *m_customIconLoader;
    DBusMenuImporter *m_menuImporter;
    org::kde::StatusNotifierItem *m_statusNotifierItemInterface;
    bool m_refreshing : 1;
    bool m_needsReRefreshing : 1;
    bool m_titleUpdate : 1;
    bool m_iconUpdate : 1;
    bool m_tooltipUpdate : 1;
    bool m_statusUpdate : 1;
};

#endif // STATUSNOTIFIERITEMSOURCE_H
