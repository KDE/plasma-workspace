/*
 *   Copyright (C) 2007-2010 John Tapsell <johnflux@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KSYSTEMACTIVITYDIALOG__H
#define KSYSTEMACTIVITYDIALOG__H

#ifndef Q_WS_WIN

#include <KDialog>

#include "processui/ksysguardprocesslist.h"

/** This creates a simple dialog box with a KSysguardProcessList
 *
 *  It remembers the size and position of the dialog, and sets
 *  the dialog to always be over the other windows
 */
class KSystemActivityDialog : public KDialog
{
    public:
        KSystemActivityDialog(QWidget *parent = NULL);

        /** Show the dialog and set the focus
         *
         *  This can be called even when the dialog is already showing to bring it
         *  to the front again and move it to the current desktop etc.
         */
        void run();

        /** Set the text in the filter line in the process list widget */
        void setFilterText(const QString &filterText);
        QString filterText() const;

        /** Save the settings if the user presses the ESC key */
        virtual void reject();

    protected:
        /** Save the settings if the user clicks (x) button on the window */
        void closeEvent(QCloseEvent *event);

    private:
        void saveDialogSettings();

        KSysGuardProcessList m_processList;
};
#endif // not Q_WS_WIN

#endif // KSYSTEMACTIVITYDIALOG__H
