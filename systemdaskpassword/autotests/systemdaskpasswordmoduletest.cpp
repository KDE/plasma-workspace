/*
    SPDX-FileCopyrightText: 2026 Daan De Meyer <daan.j.demeyer@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KDEDModule>
#include <KLocalizedString>
#include <KPasswordDialog>
#include <KPluginFactory>
#include <KPluginMetaData>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

#include <cstring>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace Qt::StringLiterals;

class SystemdAskPasswordModuleTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void handlesRequests()
    {
        QTemporaryDir runtime(u"/var/tmp/systemdaskpasswordmoduletest-XXXXXX"_s);
        QVERIFY(runtime.isValid());
        qputenv("XDG_RUNTIME_DIR", QFile::encodeName(runtime.path()));
        KLocalizedString::setApplicationDomain("systemdaskpasswordmoduletest");
        const auto directory = runtime.filePath(u"systemd/ask-password"_s);
        QVERIFY(QDir().mkpath(directory));
        const int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        QVERIFY(fd >= 0);
        auto closer = qScopeGuard([fd]() { close(fd); });
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        const auto socketPath = QFile::encodeName(runtime.filePath(u"socket"_s));
        QVERIFY(size_t(socketPath.size()) < sizeof(address.sun_path));
        memcpy(address.sun_path, socketPath.constData(), socketPath.size());
        QCOMPARE(bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)), 0);
        const QByteArray contents = "[Ask]\nPID=" + QByteArray::number(QCoreApplication::applicationPid())
            + "\nMessage=Module request\nSocket=" + socketPath + '\n';
        QFile request(directory + u"/ask.test"_s);
        QVERIFY(request.open(QIODevice::WriteOnly));
        QCOMPARE(request.write(contents), contents.size());
        request.close();

        const KPluginMetaData metadata(qEnvironmentVariable("PASSWORD_MODULE"));
        QVERIFY(metadata.isValid());
        QVERIFY(metadata.value(u"X-KDE-Kded-autoload"_s, false));
        const auto result = KPluginFactory::instantiatePlugin<KDEDModule>(metadata);
        QVERIFY2(result, qPrintable(result.errorString));
        std::unique_ptr<KDEDModule> module(result.plugin);
        QTRY_VERIFY(currentDialog());
        QCOMPARE(currentDialog()->prompt(), u"Module request"_s);
        QCOMPARE(KLocalizedString::applicationDomain(), QByteArray("systemdaskpasswordmoduletest"));
        currentDialog()->reject();
        QTRY_VERIFY(!currentDialog());
        char response = 0;
        QCOMPARE(recv(fd, &response, sizeof(response), 0), 1);
        QCOMPARE(response, '-');
        QVERIFY(request.remove());
        QTest::qWait(50);
        QVERIFY(request.open(QIODevice::WriteOnly));
        QCOMPARE(request.write(contents), contents.size());
        request.close();
        QTRY_VERIFY(currentDialog());
        module.reset();
        QTRY_VERIFY(!currentDialog());
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

private:
    static KPasswordDialog *currentDialog()
    {
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (auto *dialog = qobject_cast<KPasswordDialog *>(widget); dialog && dialog->isVisible()) {
                return dialog;
            }
        }
        return nullptr;
    }
};

QTEST_MAIN(SystemdAskPasswordModuleTest)

#include "systemdaskpasswordmoduletest.moc"
