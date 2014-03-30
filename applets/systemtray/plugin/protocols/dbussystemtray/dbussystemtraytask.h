/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2009 Marco Martin <notmart@gmail.com>                   *
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

#ifndef DBUSSYSTEMTRAYTASK_H
#define DBUSSYSTEMTRAYTASK_H

#include "../../task.h"

#include <Plasma/DataEngine>

class KIconLoader;
class KJob;

namespace Plasma
{

class Service;

}

namespace SystemTray
{

class DBusSystemTrayTaskPrivate;

class DBusSystemTrayTask : public Task
{
    Q_OBJECT

    Q_PROPERTY(QIcon icon READ icon NOTIFY changedIcons)
    Q_PROPERTY(QIcon attIcon READ attIcon NOTIFY changedIcons)
    Q_PROPERTY(QString overlayIconName READ overlayIconName NOTIFY changedOverlayIconName)
    Q_PROPERTY(QString iconName READ iconName NOTIFY changedIconName)
    Q_PROPERTY(QString attIconName READ attIconName NOTIFY changedAttIconName)
    Q_PROPERTY(QString moviePath READ moviePath NOTIFY changedMoviePath)
    Q_PROPERTY(bool isMenu READ isMenu NOTIFY changedIsMenu)
    Q_PROPERTY(QString title READ title NOTIFY changedTitle)
    Q_PROPERTY(QString tooltipTitle READ tooltipTitle NOTIFY changedTooltipTitle)
    Q_PROPERTY(QString tooltipText READ tooltipText NOTIFY changedTooltipText)
    Q_PROPERTY(QIcon tooltipIcon READ tooltipIcon NOTIFY changedTooltip)
    // property tooltipIconName was introduced to make available some icons in tooltip
    // while PlasmaCore.ToolTip doesn't provide property for icon (not only name of an icon)
    Q_PROPERTY(QString tooltipIconName READ tooltipIconName NOTIFY changedTooltipIconName)
    Q_PROPERTY(QString shortcut READ shortcut NOTIFY changedShortcut)

    friend class DBusSystemTrayProtocol;

public:
    DBusSystemTrayTask(const QString &name, Plasma::DataEngine *service, QObject *parent);
    ~DBusSystemTrayTask();

    bool isValid() const;
    bool isEmbeddable() const;
    virtual QString taskId() const;
    virtual QIcon icon() const;
    virtual bool isWidget() const;
    virtual TaskType type() const { return TypeStatusItem; }

    QString iconName() const { return m_iconName; }
    QIcon   attIcon() const { return m_attentionIcon; }
    QString attIconName() const { return m_attentionIconName; }
    QString moviePath() const { return m_moviePath; }
    QString overlayIconName() const { return m_overlayIconName; }
    QString title() const { return name(); }
    bool    isMenu() const { return m_isMenu; }
    QString tooltipTitle() const { return m_tooltipTitle; }
    QString tooltipText() const { return m_tooltipText; }
    QIcon   tooltipIcon() const { return m_tooltipIcon; }
    QString tooltipIconName() const { return m_tooltipIcon.name(); }
    QString shortcut() const { return m_shortcut; }
    void    setShortcut(QString text);

    Q_INVOKABLE void activateContextMenu(int x, int y) const;
    Q_INVOKABLE void activate1(int x, int y) const;
    Q_INVOKABLE void activate2(int x, int y) const;
    Q_INVOKABLE void activateVertScroll(int delta) const;
    Q_INVOKABLE void activateHorzScroll(int delta) const;
    Q_INVOKABLE QVariant customIcon(QVariant variant) const;

Q_SIGNALS:
    void changedIcons(); // if icons, icon names, movie path are changed
    void changedIconName(); // if icon name changed
    void changedAttIconName(); // if attention icon name is changed
    void changedMoviePath();
    void changedOverlayIconName();
    void changedIsMenu();
    void changedTitle();
    void changedTooltip();
    void changedTooltipTitle();
    void changedTooltipText();
    void changedTooltipIconName();
    void changedShortcut();
    void showContextMenu(int x, int y, QVariant menu);

private:
    void syncToolTip(const QString &title, const QString &subTitle, const QIcon &toolTipIcon);
    void syncIcons(const Plasma::DataEngine::Data &properties);
    void _activateScroll(int delta, QString direction) const;

private Q_SLOTS:
    void syncStatus(QString status);
    void dataUpdated(const QString &taskName, const Plasma::DataEngine::Data &taskData);
    void _onContextMenu(KJob*);

private:
    QString m_serviceName;
    QString m_taskId;
    QIcon m_icon;
    QString m_iconName;
    QIcon m_attentionIcon;
    QString m_attentionIconName;
    QString m_shortcut;
    QString m_moviePath;
    QString m_overlayIconName;
    QString m_iconThemePath;
    QString m_tooltipTitle;
    QString m_tooltipText;
    QIcon   m_tooltipIcon;

    KIconLoader *m_customIconLoader;

    Plasma::DataEngine *m_dataEngine;
    Plasma::Service *m_service;
    bool m_isMenu;
    bool m_valid;
};

}


#endif
