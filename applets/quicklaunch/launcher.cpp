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
#include "launcher.h"

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtGui/QGraphicsSceneContextMenuEvent>
#include <QtGui/QGraphicsSceneDragDropEvent>

// KDE
#include <KRun>

// Plasma
#include <Plasma/ToolTipContent>
#include <Plasma/ToolTipManager>

// Own
#include "launcherdata.h"

using Plasma::ToolTipManager;
using Plasma::ToolTipContent;

namespace Quicklaunch {

Launcher::Launcher(const LauncherData &data, QGraphicsItem *parent)
  : Plasma::IconWidget(parent),
    m_data(data),
    m_nameVisible(false)
{
    setIcon(data.icon());

    ToolTipManager::self()->registerWidget(this);
    connect(this, SIGNAL(clicked()), SLOT(execute()));
}

void Launcher::setNameVisible(bool enable)
{
    if (enable == m_nameVisible) {
        return;
    }

    m_nameVisible = enable;

    if (enable) {
        setText(m_data.name());
    } else {
        setText(QString());
    }
}

bool Launcher::isNameVisible() const
{
    return m_nameVisible;
}

void Launcher::setLauncherData(const LauncherData &data)
{
    setIcon(data.icon());
    if (m_nameVisible) {
        setText(data.name());
    }

    if (ToolTipManager::self()->isVisible(this)) {
        updateToolTipContent();
    }

    m_data = data;
}

LauncherData Launcher::launcherData() const
{
    return m_data;
}

KUrl Launcher::url() const
{
    return m_data.url();
}

void Launcher::execute()
{
    new KRun(m_data.url(), 0);
}

void Launcher::toolTipAboutToShow()
{
    updateToolTipContent();
}

void Launcher::toolTipHidden()
{
    ToolTipManager::self()->clearContent(this);
}

void Launcher::updateToolTipContent()
{
    ToolTipContent toolTipContent;
    toolTipContent.setMainText(m_data.name());
    toolTipContent.setSubText(m_data.description());
    toolTipContent.setImage(icon());

    ToolTipManager::self()->setContent(this, toolTipContent);
}
}
