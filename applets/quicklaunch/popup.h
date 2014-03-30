/***************************************************************************
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
#ifndef QUICKLAUNCH_POPUP_H
#define QUICKLAUNCH_POPUP_H

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QObject>

// KDE
#include <Plasma/Dialog>

using Plasma::Dialog;

namespace Quicklaunch {

class PopupLauncherList;
class Quicklaunch;

class Popup : public Dialog {

    Q_OBJECT

public:
    Popup(Quicklaunch *applet);
    ~Popup();

    PopupLauncherList *launcherList();
    void show();
    bool eventFilter(QObject *watched, QEvent *event);

private Q_SLOTS:
    void onAppletGeometryChanged();
    void onLauncherClicked();

private:
    void syncSizeAndPosition();

    Quicklaunch *m_applet;
    PopupLauncherList *m_launcherList;
};
}

#endif /* QUICKLAUNCH_POPUP_H */
