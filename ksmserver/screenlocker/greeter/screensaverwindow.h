/*
 *   Copyright 1999 Martin R. Jones <mjones@kde.org>
 *   Copyright 2003 Oswald Buddenhagen <ossi@kde.org>
 *   Copyright 2008 Chani Armitage <chanika@gmail.com>
 *   Copyright 2012 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCREENSAVERWINDOW_H
#define SCREENSAVERWINDOW_H

#include <QWidget>

#include <KProcess>

class QMouseEvent;
class QTimer;

namespace ScreenLocker
{

class ScreenSaverWindow : public QWidget
{
    Q_OBJECT
public:
    ScreenSaverWindow(QWidget *parent = 0);
    virtual ~ScreenSaverWindow();

    QPixmap background() const;
    void setBackground(const QPixmap &pix);

protected:
    void mousePressEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

Q_SIGNALS:
    void hidden();

private:
    bool startXScreenSaver();
    void stopXScreenSaver();
    void readSaver();

    KProcess m_ScreenSaverProcess;
    QPoint m_startMousePos;
    QString m_saver;
    QString m_saverExec;
    QPixmap m_background;

    QTimer *m_reactivateTimer;

    bool m_forbidden : 1;
    bool m_openGLVisual : 1;
};

} // end namespace
#endif
