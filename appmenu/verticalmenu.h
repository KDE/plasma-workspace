/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef VERTICALMENU_H
#define VERTICALMENU_H

#include <QMenu>
#include <QDBusObjectPath>

class VerticalMenu : public QMenu
{
Q_OBJECT
public:
    explicit VerticalMenu(QWidget * parent = nullptr);
    ~VerticalMenu() override;

    QString serviceName() const { return m_serviceName; }
    void setServiceName(const QString &serviceName) { m_serviceName = serviceName; }

    QDBusObjectPath menuObjectPath() const { return m_menuObjectPath; }
    void setMenuObjectPath(const QDBusObjectPath &menuObjectPath) { m_menuObjectPath = menuObjectPath; }

protected:
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;

private:
    QMenu *leafMenu();

    QString m_serviceName;
    QDBusObjectPath m_menuObjectPath;

};

#endif //VERTICALMENU_H
