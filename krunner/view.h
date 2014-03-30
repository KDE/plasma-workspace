/*
 *  Copyright 2014 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef VIEW_H
#define VIEW_H

#include <QtQuick/QQuickView>

#include "dialog.h"

namespace KDeclarative {
    class QmlObject;
}

class ViewPrivate;

class View : public PlasmaQuick::Dialog
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.krunner.App")

public:
    explicit View(QWindow *parent = 0);
    virtual ~View();

    void positionOnScreen();

protected:
    virtual void showEvent(QShowEvent *event);

public Q_SLOTS:
    void display();
    void displaySingleRunner(const QString &runnerName);
    void displayWithClipboardContents();
    void query(const QString &term);
    void querySingleRunner(const QString &runnerName, const QString &term);
    void switchUser();
    void initializeStartupNotification();

protected Q_SLOTS:
    void screenResized(int screen);
    void screenGeometryChanged(int screenCount);
    void resetScreenPos();
    void displayOrHide();

private:
    QPoint m_customPos;
    KDeclarative::QmlObject *m_qmlObj;
    int m_shownOnScreen;
    qreal m_offset;
    bool m_floating : 1;
};


#endif // View_H
