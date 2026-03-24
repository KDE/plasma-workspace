/*
    SPDX-FileCopyrightText: 2025 Daan De Meyer <daan.j.demeyer@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "askpasswordagent.h"
#include "systemdaskpassword_debug.h"

#include <KDirWatch>
#include <KLocalizedString>
#include <KPasswordDialog>
#include <KPasswordLineEdit>

#include <QChronoTimer>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QSet>
#include <QSocketNotifier>
#include <QTimer>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

// glibc 2.44 omits C++ linkage guards from <sys/pidfd.h>.
extern "C" {
#include <sys/pidfd.h>
}

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

static constexpr qsizetype maxRequestSize = 1024 * 1024;

class AskPasswordDialog : public KPasswordDialog
{
protected:
    bool checkPassword() override
    {
        // systemd discards response datagrams larger than LINE_MAX + 1 bytes.
        if (password().toUtf8().size() > LINE_MAX) {
            showErrorMessage(i18ncp("@info", "The password must not exceed %1 byte.", "The password must not exceed %1 bytes.", LINE_MAX));
            return false;
        }
        return KPasswordDialog::checkPassword();
    }
};

// systemd writes request values literally.
// QSettings would alter prompt text by interpreting commas, quotes, and backslashes as INI syntax.
static QHash<QString, QString> readRequest(const QByteArray &contents)
{
    QHash<QString, QString> values;
    bool inAsk = false;
    static const QSet<QString> keys{u"Socket"_s, u"Message"_s, u"Icon"_s, u"PID"_s, u"NotAfter"_s, u"Echo"_s, u"Silent"_s};
    for (const auto &bytes : contents.split('\n')) {
        const QString line = QString::fromUtf8(bytes).trimmed();
        if (line.isEmpty() || line.startsWith(u'#') || line.startsWith(u';')) {
            continue;
        }
        if (line.startsWith(u'[')) {
            inAsk = line == u"[Ask]";
            continue;
        }
        if (inAsk) {
            const auto equals = line.indexOf(u'=');
            if (equals > 0) {
                const auto key = line.left(equals).trimmed();
                if (keys.contains(key)) {
                    values.insert(key, line.mid(equals + 1).trimmed());
                }
            }
        }
    }
    return values;
}

static bool booleanValue(const QString &value)
{
    const auto lower = value.toLower();
    return lower == u"1" || lower == u"yes" || lower == u"true" || lower == u"on";
}

static std::chrono::nanoseconds timeUntil(quint64 deadline)
{
    const auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    if (deadline <= quint64(now)) {
        return 0ns;
    }
    const auto maximum = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::nanoseconds::max()).count();
    return std::chrono::microseconds(std::min(deadline - quint64(now), quint64(maximum)));
}

AskPasswordAgent::AskPasswordAgent(const QString &directory, QObject *parent)
    : QObject(parent)
    , m_directory(directory)
    , m_watcher(new KDirWatch(this))
{
    if (!QDir().mkpath(m_directory)) {
        qCWarning(SystemdAskPassword) << "Failed to create ask-password directory:" << m_directory;
    }
    connect(m_watcher, &KDirWatch::dirty, this, &AskPasswordAgent::scanDirectory, Qt::QueuedConnection);
    connect(m_watcher, &KDirWatch::created, this, &AskPasswordAgent::scanDirectory, Qt::QueuedConnection);
    connect(m_watcher, &KDirWatch::deleted, this, &AskPasswordAgent::scanDirectory, Qt::QueuedConnection);
    auto *timer = new QTimer(this);
    timer->setInterval(1s);
    // KDirWatch can miss requests after a symlink target changes while the old target still exists.
    connect(timer, &QTimer::timeout, this, &AskPasswordAgent::scanDirectory);
    timer->start();
    QTimer::singleShot(0, this, &AskPasswordAgent::scanDirectory);
}

AskPasswordAgent::~AskPasswordAgent()
{
    finishRequest();
}

void AskPasswordAgent::scanDirectory()
{
    struct stat identity;
    if (!QDir().mkpath(m_directory) || stat(QFile::encodeName(m_directory).constData(), &identity) < 0) {
        finishRequest();
        m_seen.clear();
        return;
    }
    const FileIdentity directoryIdentity{identity.st_dev, identity.st_ino};
    // The active prompt and saved request identities refer to files in the original directory.
    if (directoryIdentity != m_directoryIdentity) {
        finishRequest();
        m_seen.clear();
        m_directoryIdentity = directoryIdentity;
        m_watcher->removeDir(m_directory);
        m_watcher->addDir(m_directory, KDirWatch::WatchFiles);
    }
    // Requesters remove their files after receiving a response.
    // Keeping completed requests in m_seen prevents another scan from displaying the same prompt.
    for (auto it = m_seen.begin(); it != m_seen.end();) {
        if (lstat(QFile::encodeName(it.key()).constData(), &identity) < 0 || FileIdentity{identity.st_dev, identity.st_ino} != it.value()) {
            if (it.key() == m_requestPath) {
                finishRequest();
            }
            it = m_seen.erase(it);
        } else {
            ++it;
        }
    }
    // Pending requests can expire or disappear while a prompt is open.
    // Delaying request parsing until a prompt can be shown avoids using stale request data.
    if (m_dialog) {
        return;
    }
    const QDir dir(m_directory);
    for (const auto &name : dir.entryList({u"ask.*"_s}, QDir::Files | QDir::NoSymLinks)) {
        const auto path = dir.absoluteFilePath(name);
        if (!m_seen.contains(path)) {
            processRequest(path);
            if (m_dialog) {
                break;
            }
        }
    }
}

void AskPasswordAgent::processRequest(const QString &filePath)
{
    const int fd = open(QFile::encodeName(filePath).constData(), O_RDONLY | O_CLOEXEC | O_NONBLOCK | O_NOFOLLOW);
    if (fd < 0) {
        return;
    }
    QFile file;
    if (!file.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) {
        close(fd);
        return;
    }
    struct stat identity;
    if (fstat(fd, &identity) < 0 || !S_ISREG(identity.st_mode)) {
        return;
    }
    m_seen.insert(filePath, {identity.st_dev, identity.st_ino});
    if (identity.st_size > maxRequestSize) {
        return;
    }
    const auto contents = file.read(maxRequestSize + 1);
    if (contents.size() > maxRequestSize || file.error() != QFileDevice::NoError) {
        return;
    }
    const auto values = readRequest(contents);
    const QString socketPath = values.value(u"Socket"_s);
    const auto socketBytes = QFile::encodeName(socketPath);
    if (!QDir::isAbsolutePath(socketPath) || socketBytes.contains('\0') || stat(socketBytes.constData(), &identity) < 0 || !S_ISSOCK(identity.st_mode)) {
        qCWarning(SystemdAskPassword) << "Invalid Socket= in" << filePath;
        return;
    }
    const auto deadline = values.value(u"NotAfter"_s).toULongLong();
    if (deadline > 0 && timeUntil(deadline) == 0ns) {
        return;
    }
    const auto pid = values.value(u"PID"_s).toInt();
    if (pid <= 0) {
        qCWarning(SystemdAskPassword) << "Invalid PID= in" << filePath;
        return;
    }
    const int pidfd = pidfd_open(pid, 0);
    if (pidfd < 0) {
        if (errno != ESRCH) {
            qCWarning(SystemdAskPassword) << "Failed to monitor password requester:" << strerror(errno);
        }
        return;
    }
    m_requestPath = filePath;
    m_socketPath = socketPath;
    m_request = new QObject(this);
    auto *notifier = new QSocketNotifier(pidfd, QSocketNotifier::Read, m_request);
    connect(notifier, &QObject::destroyed, this, [pidfd]() { close(pidfd); });
    connect(notifier, &QSocketNotifier::activated, this, [this]() {
        finishRequest();
        scanDirectory();
    });
    if (deadline > 0) {
        auto *timer = new QChronoTimer(m_request);
        timer->setSingleShot(true);
        timer->setTimerType(Qt::PreciseTimer);
        connect(timer, &QChronoTimer::timeout, this, [this, deadline, timer]() {
            const auto remaining = timeUntil(deadline);
            if (remaining > 0ns) {
                timer->setInterval(remaining);
                timer->start();
                return;
            }
            finishRequest();
            scanDirectory();
        });
        timer->setInterval(timeUntil(deadline));
        timer->start();
    }

    m_dialog = new AskPasswordDialog;
    m_dialog->setWindowTitle(i18nc("@title:window", "Password Required"));
    const auto message = values.value(u"Message"_s);
    m_dialog->setPrompt(message.isEmpty() ? i18nc("@label", "Password:") : message.toHtmlEscaped());
    const auto icon = QIcon::fromTheme(values.value(u"Icon"_s, u"dialog-password"_s));
    m_dialog->setIcon(icon);
    m_dialog->setWindowIcon(icon);
    auto *passwordEdit = m_dialog->findChild<KPasswordLineEdit *>();
    if (booleanValue(values.value(u"Silent"_s))) {
        passwordEdit->setEchoMode(QLineEdit::NoEcho);
        passwordEdit->setRevealPasswordMode(KPassword::RevealMode::Never);
    } else if (booleanValue(values.value(u"Echo"_s))) {
        passwordEdit->setEchoMode(QLineEdit::Normal);
    }
    connect(m_dialog, &KPasswordDialog::gotPassword, this, [this](const QString &password) {
        respond(password, true);
    });
    connect(m_dialog, &QDialog::rejected, this, [this]() {
        respond(QString(), false);
    });
    m_dialog->show();
}

void AskPasswordAgent::finishRequest()
{
    if (m_dialog) {
        m_dialog->disconnect(this);
        m_dialog->hide();
        m_dialog->deleteLater();
        m_dialog.clear();
    }
    delete m_request;
    m_request = nullptr;
    m_requestPath.clear();
    m_socketPath.clear();
}

void AskPasswordAgent::respond(const QString &password, bool success)
{
    const int result = sendResponse(password, success);
    if (result < 0 && result != -ENOENT && result != -ECONNREFUSED) {
        QTimer::singleShot(0, this, [this, dialog = m_dialog]() {
            if (dialog && m_dialog == dialog) {
                dialog->showErrorMessage(i18nc("@info", "Could not send the password response. Please try again."));
                dialog->show();
            }
        });
        return;
    }
    finishRequest();
    scanDirectory();
}

int AskPasswordAgent::sendResponse(const QString &password, bool success)
{
    const int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        const int error = errno;
        qCWarning(SystemdAskPassword) << "Failed to create password response socket:" << strerror(error);
        return -error;
    }
    auto closer = qScopeGuard([&] { close(fd); });
    const QByteArray path = QFile::encodeName(m_socketPath);
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (size_t(path.size()) >= sizeof(address.sun_path)) {
        qCWarning(SystemdAskPassword) << "Password response socket path is too long:" << m_socketPath;
        return -ENAMETOOLONG;
    }
    memcpy(address.sun_path, path.constData(), path.size());
    const QByteArray data = success ? '+' + password.toUtf8() : QByteArrayLiteral("-");
    if (sendto(fd, data.constData(), data.size(), 0, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) < 0) {
        const int error = errno;
        qCWarning(SystemdAskPassword) << "Failed to send password response:" << strerror(error);
        return -error;
    }
    return 0;
}

#include "moc_askpasswordagent.cpp"
