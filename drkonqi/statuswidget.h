/*******************************************************************
* statuswidget.h
* Copyright 2009,2010    Dario Andres Rodriguez <andresbajotierra@gmail.com>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
******************************************************************/
#ifndef STATUSWIDGET__H
#define STATUSWIDGET__H

#include <QtCore/QEvent>
#include <QStackedWidget>
#include <QLabel>
#include <QTextDocument>

class WrapLabel;
class KPixmapSequenceWidget;
class QHideEvent;

class StatusWidget: public QStackedWidget
{
    Q_OBJECT
public:
    explicit StatusWidget(QWidget * parent = 0);

    void setBusy(const QString&);
    void setIdle(const QString&);

    void addCustomStatusWidget(QWidget *);

private:
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);

    void setBusyCursor();
    void setIdleCursor();

    WrapLabel *         m_statusLabel;

    KPixmapSequenceWidget * m_throbberWidget;
    WrapLabel *             m_busyLabel;

    QWidget *           m_statusPage;
    QWidget *           m_busyPage;

    int                 m_cursorStackCount;
    bool                m_busy;
};

//Dummy class to avoid a QLabel+wordWrap height bug
class WrapLabel: public QLabel
{
    Q_OBJECT
public:
    explicit WrapLabel(QWidget * parent = 0) : QLabel(parent){
        setWordWrap(true);
    }

    void setText(const QString & text) {
        QLabel::setText(text);
        adjustHeight();
    }

    bool event(QEvent * e) {
        if (e->type() == QEvent::ApplicationFontChange || e->type() == QEvent::Resize) {
            adjustHeight();
        }
        return QLabel::event(e);
    }

private:
    void adjustHeight() {
        QTextDocument document(text());
        document.setTextWidth(width());
        setMaximumHeight(document.size().height());
    }

};

#endif
