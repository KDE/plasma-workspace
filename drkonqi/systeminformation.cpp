/*******************************************************************
* systeminformation.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* Copyright 2009    George Kiagiadakis <gkiagia@users.sourceforge.net>
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

#include "systeminformation.h"

#ifdef HAVE_UNAME
# include <errno.h>
# include <sys/utsname.h>
#endif

#include <QtCore/QFile>

#include <KProcess>
#include <QDebug>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,20,0)
#include <KCoreAddons>
#endif
#include <QStandardPaths>

static const QString OS_UNSPECIFIED = QStringLiteral("unspecified");
static const QString PLATFORM_UNSPECIFIED = QStringLiteral("unspecified");

SystemInformation::SystemInformation(QObject * parent)
    : QObject(parent)
    , m_bugzillaOperatingSystem(OS_UNSPECIFIED)
    , m_bugzillaPlatform(PLATFORM_UNSPECIFIED)
{
    // NOTE: the relative order is important here
    m_bugzillaOperatingSystem = fetchOSBasicInformation();
    m_operatingSystem = fetchOSDetailInformation();

    tryToSetBugzillaPlatform();

    KConfigGroup config(KSharedConfig::openConfig(), "SystemInformation");
    m_compiledSources = config.readEntry("CompiledSources", false);
}

SystemInformation::~SystemInformation()
{
    KConfigGroup config(KSharedConfig::openConfig(), "SystemInformation");
    config.writeEntry("CompiledSources", m_compiledSources);
    config.sync();
}

void SystemInformation::tryToSetBugzillaPlatform()
{
    QString platform = PLATFORM_UNSPECIFIED;
    // first, try to guess bugzilla platfrom from the internal OS information
    // this should work for BSDs, solaris and windows.
    platform = guessBugzillaPlatform(m_bugzillaOperatingSystem);

    // if the internal information is not enough, refer to external information
    if (platform == PLATFORM_UNSPECIFIED) {
        tryToSetBugzillaPlatformFromExternalInfo();
    } else {
        setBugzillaPlatform(platform);
    }
}

void SystemInformation::tryToSetBugzillaPlatformFromExternalInfo()
{
    //Run lsb_release async
    QString lsb_release = QStandardPaths::findExecutable(QLatin1String("lsb_release"));
    if ( !lsb_release.isEmpty() ) {
        qDebug() << "found lsb_release";
        KProcess *process = new KProcess();
        process->setOutputChannelMode(KProcess::OnlyStdoutChannel);
        process->setEnv(QStringLiteral("LC_ALL"), QStringLiteral("C"));
        *process << lsb_release << QStringLiteral("-sd");
        connect(process, static_cast<void (KProcess::*)(int, QProcess::ExitStatus)>(&KProcess::finished), this, &SystemInformation::lsbReleaseFinished);
        process->start();
    } else {
        // when lsb_release is unavailable, turn to /etc/os-release
        const QString& osReleaseInfo = fetchOSReleaseInformation();
        const QString& platform = guessBugzillaPlatform(osReleaseInfo);
        setBugzillaPlatform(platform);
    }
}

void SystemInformation::lsbReleaseFinished()
{
    KProcess *process = qobject_cast<KProcess*>(sender());
    Q_ASSERT(process);
    m_lsbRelease = QString::fromLocal8Bit(process->readAllStandardOutput().trimmed());
    process->deleteLater();

    //Guess distro string
    QString platform = guessBugzillaPlatform(m_lsbRelease);

    // if lsb_release doesn't work well, turn to the /etc/os-release file
    if (platform == PLATFORM_UNSPECIFIED) {
        const QString& osReleaseInfo = fetchOSReleaseInformation();
        platform = guessBugzillaPlatform(osReleaseInfo);
    }

    setBugzillaPlatform(platform);
}

//this function maps the distribution information to an "Hardware Platform"    .
//value that is accepted by bugs.kde.org.  If the values change on the server    .
//side, they need to be updated here as well                                   .
QString SystemInformation::guessBugzillaPlatform(const QString& distroInfo) const
{
    if ( distroInfo.contains(QStringLiteral("suse"),Qt::CaseInsensitive) ) {
        return (QLatin1String("openSUSE RPMs"));
    } else if ( distroInfo.contains(QStringLiteral("mint"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Mint (Ubuntu Based)"));
    } else if ( distroInfo.contains(QStringLiteral("lmde"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Mint (Debian Based)"));
    } else if ( distroInfo.contains(QStringLiteral("ubuntu"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Ubuntu Packages"));
    } else if ( distroInfo.contains(QStringLiteral("fedora"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Fedora RPMs"));
    } else if ( distroInfo.contains(QStringLiteral("redhat"),Qt::CaseInsensitive) ) {
        return (QLatin1String("RedHat RPMs"));
    } else if ( distroInfo.contains(QStringLiteral("gentoo"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Gentoo Packages"));
    } else if ( distroInfo.contains(QStringLiteral("mandriva"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Mandriva RPMs"));
    } else if ( distroInfo.contains(QStringLiteral("mageia"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Mageia RPMs"));
    } else if ( distroInfo.contains(QStringLiteral("slack"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Slackware Packages"));
    } else if ( distroInfo.contains(QStringLiteral("pclinuxos"),Qt::CaseInsensitive) ) {
        return (QLatin1String("PCLinuxOS"));
    } else if ( distroInfo.contains(QStringLiteral("pardus"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Pardus Packages"));
    } else if ( distroInfo.contains(QStringLiteral("freebsd"),Qt::CaseInsensitive) ) {
        return (QLatin1String("FreeBSD Ports"));
    } else if ( distroInfo.contains(QStringLiteral("netbsd"),Qt::CaseInsensitive) ) {
        return (QLatin1String("NetBSD pkgsrc"));
    } else if ( distroInfo.contains(QStringLiteral("openbsd"),Qt::CaseInsensitive) ) {
        return (QLatin1String("OpenBSD Packages"));
    } else if ( distroInfo.contains(QStringLiteral("solaris"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Solaris Packages"));
    } else if ( distroInfo.contains(QStringLiteral("chakra"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Chakra"));
    } else if ( distroInfo.contains(QStringLiteral("ms windows"),Qt::CaseInsensitive) ) {
        return (QLatin1String("MS Windows"));
    } else if ( distroInfo.contains(QStringLiteral("arch"),Qt::CaseInsensitive) ) {
        return (QLatin1String("Archlinux Packages"));
    } else if ( distroInfo.contains(QStringLiteral("debian"),Qt::CaseInsensitive) ) {
        if ( distroInfo.contains(QStringLiteral("unstable"),Qt::CaseInsensitive) ) {
            return (QLatin1String("Debian unstable"));
        } else if ( distroInfo.contains(QStringLiteral("testing"),Qt::CaseInsensitive) ) {
            return (QLatin1String("Debian testing"));
        } else {
            return (QLatin1String("Debian stable"));
        }
    } else {
        return PLATFORM_UNSPECIFIED;
    }
}

//this function maps the operating system to an OS value that is accepted by bugs.kde.org.
//if the values change on the server side, they need to be updated here as well.
QString SystemInformation::fetchOSBasicInformation() const
{
    //krazy:excludeall=cpp
    //Get the base OS string (bugzillaOS)
#if defined(Q_OS_LINUX)
    return QLatin1String("Linux");
#elif defined(Q_OS_FREEBSD)
    return QLatin1String("FreeBSD");
#elif defined(Q_OS_NETBSD)
    return QLatin1String("NetBSD");
#elif defined(Q_OS_OPENBSD)
    return QLatin1String("OpenBSD");
#elif defined(Q_OS_AIX)
    return QLatin1String("AIX");
#elif defined(Q_OS_HPUX)
    return QLatin1String("HP-UX");
#elif defined(Q_OS_IRIX)
    return QLatin1String("IRIX");
#elif defined(Q_OS_OSF)
    return QLatin1String("Tru64");
#elif defined(Q_OS_SOLARIS)
    return QLatin1String("Solaris");
#elif defined(Q_OS_CYGWIN)
    return QLatin1String("Cygwin");
#elif defined(Q_OS_DARWIN)
    return QLatin1String("OS X");
#elif defined(Q_OS_WIN32)
    return QLatin1String("MS Windows");
#else
    return OS_UNSPECIFIED;
#endif

}

QString SystemInformation::fetchOSDetailInformation() const
{
    //Get complete OS string (and fallback to base string)
    QString operatingSystem = m_bugzillaOperatingSystem;

#ifdef HAVE_UNAME
    struct utsname buf;
    if (uname(&buf) == -1) {
        qDebug() << "call to uname failed" << errno;
    } else {
        operatingSystem = QString::fromLocal8Bit(buf.sysname) + ' '
            + QString::fromLocal8Bit(buf.release) + ' '
            + QString::fromLocal8Bit(buf.machine);
    }
#endif

    return operatingSystem;
}

QString SystemInformation::fetchOSReleaseInformation() const
{
    QFile data(QStringLiteral("/etc/os-release"));
    if (!data.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QMap<QString,QString> distroInfos;

    QTextStream in(&data);
    while (!in.atEnd()) {
        const QString line = in.readLine();

        // its format is one simple NAME=VALUE per line
        // don't use QString.split() here since its value might contain '=''
        const int index = line.indexOf('=');
        if ( index != -1 ) {
            const QString key = line.left(index);
            const QString value = line.mid(index+1);
            distroInfos.insert(key, value);
        }
    }

    // the PRETTY_NAME entry should be the most appropriate one,
    // but I could be wrong.
    const QString prettyName = distroInfos.value(QStringLiteral("PRETTY_NAME"), QStringLiteral("Linux"));
    return prettyName;
}

QString SystemInformation::operatingSystem() const
{
    return m_operatingSystem;
}

QString SystemInformation::bugzillaOperatingSystem() const
{
    return m_bugzillaOperatingSystem;
}

QString SystemInformation::bugzillaPlatform() const
{
    return m_bugzillaPlatform;
}

void SystemInformation::setBugzillaPlatform(const QString & platform)
{
    m_bugzillaPlatform = platform;
}

QString SystemInformation::lsbRelease() const
{
    return m_lsbRelease;
}

bool SystemInformation::compiledSources() const
{
    return m_compiledSources;
}

void SystemInformation::setCompiledSources(bool compiled)
{
    m_compiledSources = compiled;
}

QString SystemInformation::qtVersion() const
{
    return qVersion();
}

QString SystemInformation::frameworksVersion() const
{
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,20,0)
    return KCoreAddons::versionString();
#else
    return QStringLiteral("Unknown");
#endif
}
