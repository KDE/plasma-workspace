/*******************************************************************
* debugpackageinstaller.h
* Copyright  2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
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
#ifndef DEBUGPACKAGEINSTALLER__H
#define DEBUGPACKAGEINSTALLER__H

#include <QtCore/QObject>
#include <QtCore/QProcess>

class KProcess;
class QProgressDialog;

class DebugPackageInstaller: public QObject
{
    Q_OBJECT

    enum Results { ResultInstalled = 0, ResultError = 1,
                   ResultSymbolsNotFound = 2, ResultCanceled = 3 };

    public:
        explicit DebugPackageInstaller(QObject *parent = 0);
        bool canInstallDebugPackages() const;
        void setMissingLibraries(const QStringList &);
        void installDebugPackages();

    private Q_SLOTS:
        void processFinished(int, QProcess::ExitStatus);
        void progressDialogCanceled();

    Q_SIGNALS:
        void packagesInstalled();
        void error(const QString &);
        void canceled();

    private:
        KProcess *              m_installerProcess;
        QProgressDialog *       m_progressDialog;
        QString                 m_executablePath;
        QStringList             m_missingLibraries;
};

#endif
