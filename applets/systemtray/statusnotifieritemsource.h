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
    void provideXdgActivationToken(const QString &token);

    KIconLoader *iconLoader() const;
    QString attentionIconName() const;
    QIcon attentionIconPixmap() const;
    QString attentionMovieName() const;
    QString category() const;
    QString iconName() const;
    QIcon iconPixmap() const;
    QString id() const;
    bool itemIsMenu() const;
    QString overlayIconName() const;
    QIcon overlayIconPixmap() const;
    QString status() const;
    QString title() const;
    QString toolTipIconName() const;
    QIcon toolTipIconPixmap() const;
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

    bool m_valid;
    QString m_servicename;
    QTimer m_refreshTimer;
    KIconLoader *m_customIconLoader;
    DBusMenuImporter *m_menuImporter;
    org::kde::StatusNotifierItem *m_statusNotifierItemInterface;
    bool m_refreshing : 1;
    bool m_needsReRefreshing : 1;

    QString m_attentionIconName;
    QIcon m_attentionIconPixmap;
    QString m_attentionMovieName;
    QString m_category;
    QString m_iconName;
    QIcon m_iconPixmap;
    QString m_iconThemePath;
    QString m_id;
    bool m_itemIsMenu;
    QString m_overlayIconName;
    QIcon m_overlayIconPixmap;
    QString m_status;
    QString m_title;
    QString m_toolTipIconName;
    QIcon m_toolTipIconPixmap;
    QString m_toolTipSubTitle;
    QString m_toolTipTitle;
    QString m_windowId;
};
