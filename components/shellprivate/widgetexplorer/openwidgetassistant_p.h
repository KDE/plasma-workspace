/*
 *   Copyright (C) 2008 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_OPENWIDGETASSISTANT_P_H
#define PLASMA_OPENWIDGETASSISTANT_P_H

#include <KAssistantDialog>
#include <KService>

class KFileWidget;
class QListWidget;

namespace Plasma
{

class OpenWidgetAssistant : public KAssistantDialog
{
    Q_OBJECT

public:
    explicit OpenWidgetAssistant(QWidget *parent);

protected Q_SLOTS:
    void finished();
    void slotHelpClicked();

private:
    KPageWidgetItem *m_filePage;
    KFileWidget *m_fileWidget;
    QWidget *m_filePageWidget;
};

} // Plasma namespace

#endif
