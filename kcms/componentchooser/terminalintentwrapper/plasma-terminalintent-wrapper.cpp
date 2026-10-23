#include <KApplicationTrader>
#include <KDBusService>
#include <KTerminalLauncherJob>

#include <QCoreApplication>
#include <QDBusAbstractAdaptor>
#include <QDBusMetaType>
#include <kterminallauncherjob.h>
#include <qdbusmessage.h>
#include <qprocess.h>

#include "terminal_settings.h"

using namespace Qt::StringLiterals;

class Terminal1IntentHandler : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Terminal1")
public:
    Terminal1IntentHandler(QObject *parent)
        : QDBusAbstractAdaptor(parent)
    {
    }

public Q_SLOTS:
    void LaunchCommand(const QList<QVariantMap> &commands,
                       [[maybe_unused]] const QByteArray &desktop_entry,
                       [[maybe_unused]] const QVariantMap &options,
                       const QVariantMap &platformData,
                       const QDBusMessage &message)
    {
        // Long-term move non-intent path from terminal launcher job here
        for (const auto &command : commands) {
            const auto exec = qdbus_cast<QList<QByteArray>>(command.value(u"exec"_s).value<QDBusArgument>());
            auto job = KTerminalLauncherJob(QString::fromUtf8(exec.join(' ')), QCoreApplication::instance());
            job.setWorkingDirectory(QString::fromUtf8(command.value(u"working_directory"_s).toByteArray()));
            const auto env = qdbus_cast<QList<QByteArray>>(command.value(u"env"_s).value<QDBusArgument>());
            auto environment = QProcessEnvironment::systemEnvironment();
            for (const auto &envVar : env) {
                const auto equals = envVar.indexOf('=');
                if (equals != -1) {
                    environment.insert(QString::fromUtf8(envVar.first(equals)), QString::fromUtf8(envVar.sliced(equals + 1)));
                }
            }
            job.setStartupId(platformData.value(u"activation-token"_s).toByteArray());
            qDebug() << QString::fromUtf8(exec.join(' '));
            if (!job.exec()) {
                message.setDelayedReply(true);
                message.createErrorReply(QDBusError::Failed, job.errorText());
                return;
            }
        }
        QCoreApplication::quit();
    }
};

int main(int argc, char **argv)
{
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QList<QByteArray>>();
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationDomain(u"kde.org"_s);
    QCoreApplication::setApplicationName(u"plasma-terminalintent-wrapper"_s);
    auto dbusService = KDBusService::withExtraAdaptors<Terminal1IntentHandler>(KDBusService::Unique);
    return QCoreApplication::exec();
}

#include "plasma-terminalintent-wrapper.moc"
