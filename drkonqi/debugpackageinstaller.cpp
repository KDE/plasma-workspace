/*******************************************************************
* debugpackageinstaller.cpp
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

#include <config-drkonqi.h>

#include "debugpackageinstaller.h"

#include <QStandardPaths>
#include <KProcess>
#include <KLocalizedString>
#include <QProgressDialog>

#include "drkonqi.h"
#include "crashedapplication.h"

DebugPackageInstaller::DebugPackageInstaller(QObject *parent)
    : QObject(parent), m_installerProcess(0), m_progressDialog(0)
{
    m_executablePath = QStandardPaths::findExecutable(DEBUG_PACKAGE_INSTALLER_NAME); //defined from CMakeLists.txt
}

bool DebugPackageInstaller::canInstallDebugPackages() const
{
    return !m_executablePath.isEmpty();
}

void DebugPackageInstaller::setMissingLibraries(const QStringList & libraries)
{
    m_missingLibraries = libraries;
}

void DebugPackageInstaller::installDebugPackages()
{
    Q_ASSERT(canInstallDebugPackages());

    if (!m_installerProcess) {
        //Run process
        m_installerProcess = new KProcess(this);
        connect(m_installerProcess, static_cast<void (KProcess::*)(int, QProcess::ExitStatus)>(&KProcess::finished), this, &DebugPackageInstaller::processFinished);

        *m_installerProcess << m_executablePath
                            << DrKonqi::crashedApplication()->executable().absoluteFilePath()
                            << m_missingLibraries;
        m_installerProcess->start();

        //Show dialog
        m_progressDialog = new QProgressDialog(i18nc("@info:progress", "Requesting installation of missing " "debug symbols packages..."), i18n("Cancel"), 0, 0, qobject_cast<QWidget*>(parent()));
        connect(m_progressDialog, &QProgressDialog::canceled, this, &DebugPackageInstaller::progressDialogCanceled);
        m_progressDialog->setWindowTitle(i18nc("@title:window", "Missing debug symbols"));
        m_progressDialog->show();
    }
}

void DebugPackageInstaller::progressDialogCanceled()
{
    m_progressDialog->deleteLater();
    m_progressDialog = 0;

    if (m_installerProcess) {
        if (m_installerProcess->state() == QProcess::Running) {
            disconnect(m_installerProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                                            this, SLOT(processFinished(int,QProcess::ExitStatus)));
            m_installerProcess->kill();
            disconnect(m_installerProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
                                            m_installerProcess, SLOT(deleteLater()));
        }
        m_installerProcess = 0;
    }

    emit canceled();
}

void DebugPackageInstaller::processFinished(int exitCode, QProcess::ExitStatus)
{
    switch(exitCode) {
    case ResultInstalled:
    {
        emit packagesInstalled();
        break;
    }
    case ResultSymbolsNotFound:
    {
        emit error(i18nc("@info", "Could not find debug symbol packages for this application."));
        break;
    }
    case ResultCanceled:
    {
        emit canceled();
        break;
    }
    case ResultError:
    default:
    {
        emit error(i18nc("@info", "An error was encountered during the installation "
                                  "of the debug symbol packages."));
        break;
    }
    }

    m_progressDialog->reject();

    delete m_progressDialog;
    m_progressDialog = 0;

    delete m_installerProcess;
    m_installerProcess = 0;
}
