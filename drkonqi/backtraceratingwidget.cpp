/*******************************************************************
* backtraceratingwidget.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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

#include "backtraceratingwidget.h"

#include <QPainter>
#include <QIcon>


BacktraceRatingWidget::BacktraceRatingWidget(QWidget * parent) :
        QWidget(parent),
        m_state(BacktraceGenerator::NotLoaded),
        m_star1(false),
        m_star2(false),
        m_star3(false)
{
    setMinimumSize(105, 24);

    m_starPixmap = QIcon::fromTheme("favorites").pixmap(QSize(22, 22));
    m_disabledStarPixmap = QIcon::fromTheme("favorites").pixmap(QSize(22, 22), QIcon::Disabled);
    m_errorPixmap = QIcon::fromTheme("dialog-error").pixmap(QSize(22, 22));
}

void BacktraceRatingWidget::setUsefulness(BacktraceParser::Usefulness usefulness)
{
    switch (usefulness) {
    case BacktraceParser::ReallyUseful: {
        m_star1 = true;
        m_star2 = true;
        m_star3 = true;
        break;
    }
    case BacktraceParser::MayBeUseful: {
        m_star1 = true;
        m_star2 = true;
        m_star3 = false;
        break;
    }
    case BacktraceParser::ProbablyUseless: {
        m_star1 = true;
        m_star2 = false;
        m_star3 = false;
        break;
    }
    case BacktraceParser::Useless:
    case BacktraceParser::InvalidUsefulness: {
        m_star1 = false;
        m_star2 = false;
        m_star3 = false;
        break;
    }
    }

    update();
}

void BacktraceRatingWidget::paintEvent(QPaintEvent * event)
{
    Q_UNUSED(event);

    QPainter p(this);

    p.drawPixmap(QPoint(30, 1) , m_star1 ? m_starPixmap : m_disabledStarPixmap);
    p.drawPixmap(QPoint(55, 1) , m_star2 ? m_starPixmap : m_disabledStarPixmap);
    p.drawPixmap(QPoint(80, 1) , m_star3 ? m_starPixmap : m_disabledStarPixmap);

    switch (m_state) {
    case BacktraceGenerator::Failed:
    case BacktraceGenerator::FailedToStart: {
        p.drawPixmap(QPoint(0, 1) , m_errorPixmap);
        break;
    }
    case BacktraceGenerator::Loading:
    case BacktraceGenerator::Loaded:
    default:
        break;
    }

    p.end();
}
