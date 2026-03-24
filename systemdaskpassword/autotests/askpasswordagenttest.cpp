/*
    SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "askpasswordagent.h"

#include <KLocalizedString>
#include <KPasswordDialog>
#include <KPasswordLineEdit>

#include <QApplication>
#include <QChronoTimer>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextDocument>
#include <QTimer>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

class AskPasswordAgentTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        KLocalizedString::setApplicationDomain("plasma_systemdaskpassword");
        if (qEnvironmentVariableIsSet("ASK_PASSWORD_DISABLE_PIDFD")) {
            sock_filter rules[] = {
                BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
                BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_pidfd_open, 0, 1),
                BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
                BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
            };
            sock_fprog filter{4, rules};
            QCOMPARE(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0), 0);
            QCOMPARE(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &filter), 0);
        }
    }

    void init()
    {
        m_directory = std::make_unique<QTemporaryDir>(u"/var/tmp/askpasswordagenttest-XXXXXX"_s);
        QVERIFY(m_directory->isValid());
        m_socket = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        QVERIFY(m_socket >= 0);
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        const auto path = QFile::encodeName(m_directory->filePath(u"socket"_s));
        QVERIFY(size_t(path.size()) < sizeof(address.sun_path));
        memcpy(address.sun_path, path.constData(), path.size());
        QCOMPARE(bind(m_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
    }

    void cleanup()
    {
        if (m_socket >= 0) {
            close(m_socket);
            m_socket = -1;
        }
        m_directory.reset();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

    void emptyDirectory()
    {
        AskPasswordAgent agent(m_directory->path());
        QTest::qWait(50);
        QVERIFY(!bool(currentDialog()));
        QVERIFY(writeRequest(requestData()));
        QTRY_VERIFY(bool(currentDialog()));
    }

    void requesterExit()
    {
        const auto watched = m_directory->filePath(u"systemd/ask-password"_s);
        AskPasswordAgent agent(watched);
        QProcess requester;
        auto environment = QProcessEnvironment::systemEnvironment();
        environment.insert(u"XDG_RUNTIME_DIR"_s, m_directory->path());
        requester.setProcessEnvironment(environment);
        requester.start(u"systemd-ask-password"_s, {u"--user"_s, u"--no-tty"_s, u"--timeout=0"_s, u"Orphan request"_s});
        QVERIFY(requester.waitForStarted());
        QTRY_VERIFY(bool(currentDialog()));
        requester.kill();
        QVERIFY(requester.waitForFinished());
        QTRY_VERIFY(!bool(currentDialog()));
        QVERIFY(!QDir(watched).entryList({u"ask.*"_s}, QDir::Files).isEmpty());
        QVERIFY(writeRequest(requestData("Message=Live requester\n"), watched + u"/ask.live"_s));
        QTRY_VERIFY(bool(currentDialog()));
        QCOMPARE(currentDialog()->prompt(), u"Live requester"_s);
    }

    void requesterZombie_data()
    {
        QTest::addColumn<bool>("initiallyExited");
        QTest::newRow("exits-after-prompt") << false;
        QTest::newRow("already-exited") << true;
    }

    void requesterZombie()
    {
        QFETCH(bool, initiallyExited);
        const pid_t child = fork();
        if (child == 0) {
            prctl(PR_SET_NAME, "worker ) ( Z )", 0, 0, 0);
            for (;;) {
                pause();
            }
        }
        QVERIFY(child > 0);
        auto reap = qScopeGuard([child]() {
            kill(child, SIGKILL);
            while (waitpid(child, nullptr, 0) < 0 && errno == EINTR) {
            }
        });
        const auto killWithoutReaping = [child]() {
            if (kill(child, SIGKILL) < 0) {
                return false;
            }
            siginfo_t info{};
            return waitid(P_PID, child, &info, WEXITED | WNOWAIT) == 0 && info.si_pid == child && kill(child, 0) == 0;
        };
        if (initiallyExited) {
            QVERIFY(killWithoutReaping());
        }
        QVERIFY(writeRequest(requestData() + "PID=" + QByteArray::number(child) + "\nNotAfter=0\n"));
        AskPasswordAgent agent(m_directory->path());
        if (!initiallyExited) {
            QTRY_VERIFY(bool(currentDialog()));
            QVERIFY(killWithoutReaping());
            QTRY_VERIFY(!bool(currentDialog()));
        } else {
            QTest::qWait(100);
            QVERIFY(!bool(currentDialog()));
        }
        QVERIFY(writeRequest(requestData("Message=Live requester\n"), m_directory->filePath(u"ask.live"_s)));
        QTRY_VERIFY(bool(currentDialog()));
        QCOMPARE(currentDialog()->prompt(), u"Live requester"_s);
    }

    void eventLoopRemainsResponsive()
    {
        AskPasswordAgent agent(m_directory->path());
        QTest::qWait(50);
        std::atomic<bool> begin = false;
        std::atomic<int> started = 0;
        std::array<std::jthread, 8> producers;
        for (size_t i = 0; i < producers.size(); ++i) {
            const auto first = QFile::encodeName(m_directory->filePath(u"noise-%1-a"_s.arg(i)));
            const auto second = QFile::encodeName(m_directory->filePath(u"noise-%1-b"_s.arg(i)));
            producers[i] = std::jthread([&, first, second]() {
                ++started;
                while (!begin.load()) {
                    std::this_thread::yield();
                }
                const auto until = std::chrono::steady_clock::now() + 2s;
                int n = 0;
                while (std::chrono::steady_clock::now() < until) {
                    const auto &path = n++ % 2 ? first : second;
                    const int fd = open(path.constData(), O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
                    if (fd >= 0) {
                        close(fd);
                    }
                }
            });
        }
        while (started.load() != int(producers.size())) {
            std::this_thread::yield();
        }
        QElapsedTimer elapsed;
        elapsed.start();
        qint64 firedAt = -1;
        QTimer::singleShot(100ms, this, [&]() { firedAt = elapsed.elapsed(); });
        begin = true;
        QTest::qWait(2100);
        QVERIFY(firedAt >= 0 && firedAt < 1000);
    }

    void receiverGone_data()
    {
        QTest::addColumn<bool>("unlinkSocket");
        QTest::newRow("stale-path") << false;
        QTest::newRow("removed-path") << true;
    }

    void receiverGone()
    {
        QFETCH(bool, unlinkSocket);
        QVERIFY(writeRequest(requestData()));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        close(m_socket);
        m_socket = -1;
        if (unlinkSocket) {
            QVERIFY(QFile::remove(m_directory->filePath(u"socket"_s)));
        }
        currentDialog()->reject();
        QTRY_VERIFY(!bool(currentDialog()));
        QTest::qWait(50);
        QVERIFY(!currentDialog());
    }

    void literalMessages_data()
    {
        QTest::addColumn<QByteArray>("message");
        QTest::newRow("missing") << QByteArray();
        QTest::newRow("empty") << QByteArray("Message=\n");
        QTest::newRow("plain") << QByteArray("Message=Password\n");
        QTest::newRow("comma") << QByteArray("Message=Enter password, please:\n");
        QTest::newRow("backslash") << QByteArray("Message=Password for C:\\test:\n");
        QTest::newRow("quotes") << QByteArray("Message=Password for \"disk\":\n");
        QTest::newRow("utf8") << QByteArray("Message=Mot de passe du p\xc3\xa9riph\xc3\xa9rique:\n");
        QTest::newRow("equals") << QByteArray("Message=Password for device=name:\n");
        QTest::newRow("markup") << QByteArray("Message=Password for <b>disk</b>:\n");
        QTest::newRow("entity") << QByteArray("Message=Password for &amp;disk:\n");
        QTest::newRow("symbols") << QByteArray("Message=Password for <>&\"disk:\n");
    }

    void literalMessages()
    {
        QFETCH(QByteArray, message);
        QVERIFY(writeRequest(requestData(message)));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        auto *dialog = currentDialog();
        QVERIFY(dialog);
        if (message.isEmpty() || message == "Message=\n") {
            QVERIFY(!dialog->prompt().isEmpty());
        } else {
            QLabel *prompt = nullptr;
            for (auto *label : dialog->findChildren<QLabel *>()) {
                if (label->text() == dialog->prompt()) {
                    prompt = label;
                    break;
                }
            }
            QVERIFY(prompt);
            prompt->setTextInteractionFlags(Qt::TextSelectableByKeyboard);
            QTextDocument document;
            document.setHtml(prompt->text());
            prompt->setSelection(0, document.toPlainText().size());
            QCOMPARE(prompt->selectedText(), QString::fromUtf8(message.mid(8).trimmed()));
        }
    }

    void visibility_data()
    {
        QTest::addColumn<QByteArray>("flags");
        QTest::addColumn<QLineEdit::EchoMode>("mode");
        QTest::newRow("default") << QByteArray() << QLineEdit::Password;
        QTest::newRow("echo") << QByteArray("Echo=1\n") << QLineEdit::Normal;
        QTest::newRow("echo-boolean") << QByteArray("Echo=yes\n") << QLineEdit::Normal;
        QTest::newRow("masked") << QByteArray("Echo=0\nSilent=0\n") << QLineEdit::Password;
        QTest::newRow("silent") << QByteArray("Silent=1\n") << QLineEdit::NoEcho;
        QTest::newRow("silent-overrides-echo") << QByteArray("Echo=1\nSilent=1\n") << QLineEdit::NoEcho;
    }

    void visibility()
    {
        QFETCH(QByteArray, flags);
        QFETCH(QLineEdit::EchoMode, mode);
        QVERIFY(writeRequest(requestData() + flags));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        auto *dialog = currentDialog();
        QVERIFY(dialog);
        auto *edit = dialog->findChild<KPasswordLineEdit *>();
        QVERIFY(edit);
        QCOMPARE(edit->echoMode(), mode);
        if (mode == QLineEdit::NoEcho) {
            QCOMPARE(edit->revealPasswordMode(), KPassword::RevealMode::Never);
        }
    }

    void oversizedPassword_data()
    {
        QTest::addColumn<QString>("password");
        QTest::newRow("ascii") << QString(3000, u'x');
        QTest::newRow("utf8-bytes") << QString(LINE_MAX / 2 + 1, QChar(0xe9));
    }

    void oversizedPassword()
    {
        QFETCH(QString, password);
        QVERIFY(writeRequest(requestData()));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        auto *dialog = currentDialog();
        QVERIFY(dialog);
        QSignalSpy accepted(dialog, &KPasswordDialog::gotPassword);
        dialog->setPassword(password);
        dialog->buttonBox()->button(QDialogButtonBox::Ok)->click();
        QTest::qWait(50);
        QCOMPARE(accepted.count(), 0);
        QVERIFY(dialog->isVisible());
        QVERIFY(bool(currentDialog()));
        QVERIFY(response().isEmpty());
        dialog->setPassword(u"short password"_s);
        QTRY_VERIFY(dialog->buttonBox()->button(QDialogButtonBox::Ok)->isEnabled());
        dialog->buttonBox()->button(QDialogButtonBox::Ok)->click();
        QTRY_COMPARE(accepted.count(), 1);
        QTRY_VERIFY(!bool(currentDialog()));
        QCOMPARE(response(), QByteArray("+short password"));
    }

    void fullSocketQueue()
    {
        QList<int> senders;
        auto closeSenders = qScopeGuard([&]() {
            for (int fd : senders) {
                close(fd);
            }
        });
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        const auto path = QFile::encodeName(m_directory->filePath(u"socket"_s));
        memcpy(address.sun_path, path.constData(), path.size());
        bool full = false;
        for (int attempt = 0; attempt < 32; ++attempt) {
            const int sender = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
            QVERIFY(sender >= 0);
            senders.append(sender);
            int queued = 0;
            while (sendto(sender, "x", 1, 0, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 1) {
                ++queued;
            }
            QCOMPARE(errno, EAGAIN);
            if (queued == 0) {
                full = true;
                break;
            }
        }
        if (!full) {
            QSKIP("The socket queue exceeds the test's bounded buffer allocation.");
        }
        QVERIFY(writeRequest(requestData()));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        auto *dialog = currentDialog();
        QVERIFY(dialog);
        std::jthread drain([this]() {
            std::this_thread::sleep_for(500ms);
            while (!response().isEmpty()) {
            }
        });
        QElapsedTimer elapsed;
        elapsed.start();
        dialog->reject();
        const auto duration = elapsed.elapsed();
        drain.join();
        QVERIFY(duration < 250);
        QTRY_VERIFY(dialog->isVisible());
        dialog->reject();
        QTRY_VERIFY(!bool(currentDialog()));
        QCOMPARE(response(), QByteArray("-"));
    }

    void deadline()
    {
        const auto deadline = monotonicMicroseconds() + 300000;
        QVERIFY(writeRequest(requestData() + "NotAfter=" + QByteArray::number(deadline) + '\n'));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        const auto *timer = agent.findChild<QChronoTimer *>();
        QVERIFY(timer);
        QCOMPARE(timer->timerType(), Qt::PreciseTimer);
        QTRY_VERIFY(!bool(currentDialog()));
        QVERIFY(monotonicMicroseconds() >= deadline);
        QVERIFY(QFile::exists(m_directory->filePath(u"ask.test"_s)));
        QVERIFY(writeRequest("unrelated", m_directory->filePath(u"another-file"_s)));
        QTest::qWait(50);
        QVERIFY(!bool(currentDialog()));
    }

    void longDeadline()
    {
        const auto deadline = monotonicMicroseconds() + std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::days(30)).count();
        QVERIFY(writeRequest(requestData() + "NotAfter=" + QByteArray::number(deadline) + '\n'));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        const auto *timer = agent.findChild<QChronoTimer *>();
        QVERIFY(timer);
        QVERIFY(timer->isActive());
        QVERIFY(timer->interval() > std::chrono::days(29));
    }

    void expiredRequest()
    {
        QVERIFY(writeRequest(requestData() + "NotAfter=" + QByteArray::number(monotonicMicroseconds() - 1) + '\n'));
        AskPasswordAgent agent(m_directory->path());
        QTest::qWait(50);
        QVERIFY(!bool(currentDialog()));
        QVERIFY(!currentDialog());
    }

    void removedRequest()
    {
        QVERIFY(writeRequest(requestData()));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(bool(currentDialog()));
        QVERIFY(QFile::remove(m_directory->filePath(u"ask.test"_s)));
        QTRY_VERIFY(!bool(currentDialog()));
    }

    void nonregularEntry_data()
    {
        QTest::addColumn<int>("kind");
        QTest::newRow("directory") << 0;
        QTest::newRow("symlink") << 1;
        QTest::newRow("dangling-symlink") << 2;
        QTest::newRow("fifo") << 3;
    }

    void nonregularEntry()
    {
        QFETCH(int, kind);
        const auto entry = m_directory->filePath(u"ask.entry"_s);
        if (kind == 0) {
            QVERIFY(QDir().mkpath(entry));
        } else if (kind == 1) {
            QVERIFY(QFile::link(m_directory->filePath(u"socket"_s), entry));
        } else if (kind == 2) {
            QCOMPARE(symlink("missing", QFile::encodeName(entry).constData()), 0);
        } else {
            QCOMPARE(mkfifo(QFile::encodeName(entry).constData(), 0600), 0);
        }
        AskPasswordAgent agent(m_directory->path());
        QTest::qWait(50);
        QVERIFY(!bool(currentDialog()));
        if (kind == 0) {
            QVERIFY(QDir().rmdir(entry));
        } else {
            QVERIFY(QFile::remove(entry));
        }
    }

    void requestSizeLimit_data()
    {
        QTest::addColumn<int>("size");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("maximum") << 1024 * 1024 << true;
        QTest::newRow("oversized") << 1024 * 1024 + 1 << false;
    }

    void requestSizeLimit()
    {
        QFETCH(int, size);
        QFETCH(bool, accepted);
        const auto request = requestData();
        QVERIFY(writeRequest(request + QByteArray(size - request.size(), '#')));
        AskPasswordAgent agent(m_directory->path());
        QTest::qWait(50);
        QCOMPARE(bool(currentDialog()), accepted);
    }

    void directoryLifetime_data()
    {
        QTest::addColumn<int>("kind");
        QTest::newRow("removal") << 0;
        QTest::newRow("rename") << 1;
        QTest::newRow("ancestor") << 2;
        QTest::newRow("symlink") << 3;
        QTest::newRow("initial-obstruction") << 4;
    }

    void directoryLifetime()
    {
        QFETCH(int, kind);
        const auto parent = m_directory->filePath(u"systemd"_s);
        const auto watched = parent + u"/ask-password"_s;
        QVERIFY(QDir().mkpath(parent));
        if (kind == 3) {
            const auto target = m_directory->filePath(u"target"_s);
            QVERIFY(QDir().mkpath(target));
            QVERIFY(QFile::link(target, watched));
        } else if (kind == 4) {
            QVERIFY(writeRequest("obstruction", watched));
        } else {
            QVERIFY(QDir().mkpath(watched));
        }
        AskPasswordAgent agent(watched);
        if (kind != 4) {
            QVERIFY(writeRequest(requestData(), watched + u"/ask.old"_s));
            QTRY_VERIFY(currentDialog());
        }
        const QPointer<KPasswordDialog> original = currentDialog();
        if (kind == 0) {
            QVERIFY(QDir(watched).removeRecursively());
        } else if (kind == 1) {
            QVERIFY(QDir().rename(watched, watched + u"-old"_s));
        } else if (kind == 2) {
            QVERIFY(QDir().rename(parent, parent + u"-old"_s));
        } else if (kind == 3) {
            const auto target = m_directory->filePath(u"replacement"_s);
            QVERIFY(QDir().mkpath(target));
            const auto link = parent + u"/new-link"_s;
            QVERIFY(QFile::link(target, link));
            QCOMPARE(rename(QFile::encodeName(link).constData(), QFile::encodeName(watched).constData()), 0);
        } else {
            QVERIFY(QFile::remove(watched));
        }
        QVERIFY(QDir().mkpath(watched));
        QVERIFY(writeRequest(requestData("Message=Replacement\n"), watched + u"/ask.new"_s));
        QTRY_VERIFY(original.isNull());
        QTRY_VERIFY(currentDialog());
        QCOMPARE(currentDialog()->prompt(), u"Replacement"_s);
        currentDialog()->reject();
        QTRY_VERIFY(!currentDialog());
        QCOMPARE(response(), QByteArray("-"));
        QVERIFY(writeRequest(requestData("Message=Next request\n"), watched + u"/ask.next"_s));
        QTRY_VERIFY(currentDialog());
        QCOMPARE(currentDialog()->prompt(), u"Next request"_s);
    }

    void responses_data()
    {
        QTest::addColumn<bool>("accept");
        QTest::addColumn<QString>("password");
        QTest::newRow("cancel") << false << QString();
        QTest::newRow("password") << true << u"passphrase"_s;
        QTest::newRow("empty-password") << true << QString();
        QTest::newRow("maximum-password") << true << QString(LINE_MAX / 2, QChar(0xe9));
    }

    void responses()
    {
        QFETCH(bool, accept);
        QFETCH(QString, password);
        QVERIFY(writeRequest(requestData()));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(currentDialog());
        if (accept) {
            currentDialog()->setPassword(password);
            currentDialog()->buttonBox()->button(QDialogButtonBox::Ok)->click();
        } else {
            currentDialog()->reject();
        }
        QTRY_VERIFY(!currentDialog());
        QCOMPARE(response(), accept ? '+' + password.toUtf8() : QByteArray("-"));
        QVERIFY(writeRequest("noise", m_directory->filePath(u"unrelated"_s)));
        QTest::qWait(600);
        QVERIFY(!currentDialog());
        QVERIFY(QFile::remove(m_directory->filePath(u"ask.test"_s)));
        QVERIFY(writeRequest(requestData("Message=Next request\n")));
        QTRY_VERIFY(currentDialog());
        QCOMPARE(currentDialog()->prompt(), u"Next request"_s);
    }

    void queuedRequests()
    {
        QVERIFY(writeRequest(requestData("Message=First\n"), m_directory->filePath(u"ask.1"_s)));
        AskPasswordAgent agent(m_directory->path());
        QTRY_VERIFY(currentDialog());
        QVERIFY(writeRequest(requestData("Message=Second\n"), m_directory->filePath(u"ask.2"_s)));
        QVERIFY(writeRequest(requestData() + "NotAfter=" + QByteArray::number(monotonicMicroseconds() + 300000) + '\n',
                             m_directory->filePath(u"ask.3"_s)));
        QVERIFY(writeRequest(requestData("Message=Fourth\n"), m_directory->filePath(u"ask.4"_s)));
        QTest::qWait(600);
        QCOMPARE(visibleDialogs(), 1);
        QCOMPARE(currentDialog()->prompt(), u"First"_s);
        currentDialog()->reject();
        QTRY_VERIFY(currentDialog() && currentDialog()->prompt() == u"Second"_s);
        QCOMPARE(response(), QByteArray("-"));
        QCOMPARE(visibleDialogs(), 1);
        currentDialog()->reject();
        QTRY_VERIFY(currentDialog() && currentDialog()->prompt() == u"Fourth"_s);
        QCOMPARE(response(), QByteArray("-"));
        currentDialog()->reject();
        QTRY_VERIFY(!currentDialog());
        QCOMPARE(response(), QByteArray("-"));
    }

    void pidfdRequired()
    {
        if (!qEnvironmentVariableIsSet("ASK_PASSWORD_DISABLE_PIDFD")) {
            QProcess child;
            auto environment = QProcessEnvironment::systemEnvironment();
            environment.insert(u"ASK_PASSWORD_DISABLE_PIDFD"_s, u"1"_s);
            child.setProcessEnvironment(environment);
            child.start(QCoreApplication::applicationFilePath(), {u"pidfdRequired"_s});
            QVERIFY(child.waitForFinished());
            QCOMPARE(child.exitStatus(), QProcess::NormalExit);
            QVERIFY2(child.exitCode() == 0, child.readAll().constData());
            return;
        }
        QVERIFY(writeRequest(requestData()));
        AskPasswordAgent agent(m_directory->path());
        QTest::qWait(100);
        QVERIFY(!currentDialog());
    }

private:
    QByteArray requestData(const QByteArray &message = "Message=Password\n") const
    {
        return "[Ask]\nPID=" + QByteArray::number(QCoreApplication::applicationPid()) + "\nSocket="
            + QFile::encodeName(m_directory->filePath(u"socket"_s)) + '\n' + message;
    }

    bool writeRequest(const QByteArray &bytes, const QString &path = QString())
    {
        const auto destination = path.isEmpty() ? m_directory->filePath(u"ask.test"_s) : path;
        QFile file(destination + u".tmp"_s);
        if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size()) {
            return false;
        }
        file.close();
        return file.rename(destination);
    }

    QByteArray response() const
    {
        QByteArray result(LINE_MAX + 1, '\0');
        const auto size = recv(m_socket, result.data(), result.size(), 0);
        if (size < 0) {
            return {};
        }
        result.resize(size);
        return result;
    }

    static qsizetype visibleDialogs()
    {
        return std::ranges::count_if(QApplication::topLevelWidgets(), [](QWidget *widget) {
            return qobject_cast<KPasswordDialog *>(widget) && widget->isVisible();
        });
    }

    static KPasswordDialog *currentDialog()
    {
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (auto *dialog = qobject_cast<KPasswordDialog *>(widget); dialog && dialog->isVisible()) {
                return dialog;
            }
        }
        return nullptr;
    }

    static qint64 monotonicMicroseconds()
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    std::unique_ptr<QTemporaryDir> m_directory;
    int m_socket = -1;
};

QTEST_MAIN(AskPasswordAgentTest)

#include "askpasswordagenttest.moc"
