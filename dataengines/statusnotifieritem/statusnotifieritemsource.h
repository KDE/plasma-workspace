/*
    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

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
    void provideXdgActivationToken(const QString &token);

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
