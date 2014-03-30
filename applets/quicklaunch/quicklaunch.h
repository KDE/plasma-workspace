/***************************************************************************
 *   Copyright (C) 2008 by Lukas Appelhans <l.appelhans@gmx.de>            *
 *   Copyright (C) 2010 - 2011 by Ingomar Wesp <ingomar@wesp.name>         *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef QUICKLAUNCH_QUICKLAUNCH_H
#define QUICKLAUNCH_QUICKLAUNCH_H

#include "ui_quicklaunchConfig.h"

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QObject>
#include <QtCore/QPoint>
#include <QtCore/QPointF>
#include <QtCore/QSize>
#include <QtCore/QSizeF>
#include <QtCore/QStringList>

// Plasma
#include <Plasma/Applet>

class KUrl;

namespace Plasma
{
    class Dialog;
    class IconWidget;
}

class QEvent;
class QGraphicsLinearLayout;
class QGraphicsSceneContextMenuEvent;
class QPoint;

class KConfigGroup;

using Plasma::Constraints;

namespace Quicklaunch {

class LauncherGrid;
class Popup;

class Quicklaunch : public Plasma::Applet
{
    Q_OBJECT

public:
    Quicklaunch(QObject *parent, const QVariantList &args);
    ~Quicklaunch();

    void init();

    void createConfigurationInterface(KConfigDialog *parent);
    bool eventFilter(QObject *watched, QEvent *event);

protected:
    void constraintsEvent(Constraints constraints);
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

private Q_SLOTS:
    void configChanged();
    void iconSizeChanged();
    void onConfigAccepted();
    void onLaunchersChanged();
    void onPopupTriggerClicked();
    void onAddLauncherAction();
    void onEditLauncherAction();
    void onRemoveLauncherAction();

private:
    void showContextMenu(
        const QPoint& screenPos,
        bool onPopup,
        int iconIndex);

    void initActions();
    void initPopup();
    void updatePopupTrigger();
    void deletePopup();

    static QStringList defaultLaunchers();
    static QString defaultBrowserPath();
    static QString defaultFileManagerPath();
    static QString defaultEmailClientPath();

    static QString determineNewDesktopFilePath(const QString &baseName);

    Ui::quicklaunchConfig uiConfig;

    LauncherGrid *m_launcherGrid;

    QGraphicsLinearLayout *m_layout;
    Plasma::IconWidget *m_popupTrigger;
    Popup *m_popup;

    QAction* m_addLauncherAction;
    QAction* m_editLauncherAction;
    QAction* m_removeLauncherAction;

    bool m_contextMenuTriggeredOnPopup;
    int m_contextMenuLauncherIndex;
};
}

#endif /* QUICKLAUNCH_QUICKLAUNCH_H */
