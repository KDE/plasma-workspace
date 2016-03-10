/*******************************************************************
* aboutbugreportingdialog.h
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

#ifndef ABOUTBUGREPORTINGDIALOG__H
#define ABOUTBUGREPORTINGDIALOG__H

#include <QDialog>

class QTextBrowser;

class AboutBugReportingDialog: public QDialog
{
    Q_OBJECT

public:
    explicit AboutBugReportingDialog(QWidget * parent = 0);
    ~AboutBugReportingDialog() override;
    void showSection(const QString&);

private Q_SLOTS:
    void handleInternalLinks(const QUrl& url);

private:
    QTextBrowser * m_textBrowser;
};

#endif
