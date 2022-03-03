/*
    SPDX-FileCopyrightText: 2007-2010 John Tapsell <johnflux@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#ifndef Q_WS_WIN

#include <KConfigGroup>
#include <KMainWindow>

class KSysGuardProcessList;

/** This creates a simple dialog box with a KSysguardProcessList
 *
 *  It remembers the size and position of the dialog, and sets
 *  the dialog to always be over the other windows
 */
class KSystemActivityDialog : public KMainWindow
{
    Q_OBJECT
public:
    explicit KSystemActivityDialog(QWidget *parent = nullptr);

    /** Show the dialog and set the focus
     *
     *  This can be called even when the dialog is already showing to bring it
     *  to the front again and move it to the current desktop etc.
     */
    void run();

    /** Set the text in the filter line in the process list widget */
    void setFilterText(const QString &filterText);
    QString filterText() const;

    QSize sizeHint() const override;

protected:
    /** Save the settings if the user clicks (x) button on the window */
    void closeEvent(QCloseEvent *event) override;

private:
    KSysGuardProcessList *m_processList = nullptr;
    KConfigGroup m_configGroup;
};
#endif // not Q_WS_WIN
