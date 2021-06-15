// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

#include "language.h"

#include <KOSRelease>

#include <QProcess>

#include "config-workspace.h"
#include "debug.h"

#ifdef HAVE_PACKAGEKIT
#include <PackageKit/Daemon>
#include <utility>
class LanguageCompleter : public QObject
{
    Q_OBJECT
public:
    explicit LanguageCompleter(const QStringList &packages, QObject *parent = nullptr)
        : QObject(parent)
        , m_packages(packages)
    {
    }

    void start()
    {
        auto transaction = PackageKit::Daemon::resolve(m_packages, PackageKit::Transaction::FilterNotInstalled | PackageKit::Transaction::FilterArch);
        connect(transaction,
                &PackageKit::Transaction::package,
                this,
                [this](PackageKit::Transaction::Info info, const QString &packageID, const QString &summary) {
                    Q_UNUSED(info);
                    Q_UNUSED(summary);
                    m_packageIDs << packageID;
                });
        connect(transaction, &PackageKit::Transaction::errorCode, this, [](PackageKit::Transaction::Error error, const QString &details) {
            qCDebug(KCM_TRANSLATIONS) << "resolve error" << error << details;
        });
        connect(transaction, &PackageKit::Transaction::finished, this, [this](PackageKit::Transaction::Exit status, uint code) {
            qCDebug(KCM_TRANSLATIONS) << "resolve finished" << status << code << m_packageIDs;
            if (m_packageIDs.size() != m_packages.size()) {
                qCWarning(KCM_TRANSLATIONS) << "Not all missing packages managed to resolve!" << m_packages << m_packageIDs;
            }
            install();
        });
    }

Q_SIGNALS:
    void complete();

private:
    void install()
    {
        auto transaction = PackageKit::Daemon::installPackages(m_packageIDs);
        connect(transaction, &PackageKit::Transaction::errorCode, this, [](PackageKit::Transaction::Error error, const QString &details) {
            qCDebug(KCM_TRANSLATIONS) << "install error:" << error << details;
        });
        connect(transaction, &PackageKit::Transaction::finished, this, [this](PackageKit::Transaction::Exit status, uint code) {
            qCDebug(KCM_TRANSLATIONS) << "install finished:" << status << code;
            Q_EMIT complete();
        });
    }

    const QStringList m_packages;
    QStringList m_packageIDs;
};
#endif

class CompletionCheck : public QObject
{
    Q_OBJECT
public:
    enum class Result { Error, Incomplete, Complete };

    template<typename... Args>
    static CompletionCheck *create(Args &&..._args);
    ~CompletionCheck() override = default;

    virtual void start() = 0;

Q_SIGNALS:
    void finished(Result result, QStringList missingPackages);

protected:
    explicit CompletionCheck(const QString &languageCode, QObject *parent = nullptr)
        : QObject(parent)
        , m_languageCode(languageCode)
    {
    }

    const QString m_languageCode;

private:
    Q_DISABLE_COPY_MOVE(CompletionCheck);
};

class UbuntuCompletionCheck : public CompletionCheck
{
public:
    using CompletionCheck::CompletionCheck;
    void start() override
    {
        proc.setProgram("/usr/bin/check-language-support");
        proc.setArguments({"--language", m_languageCode.left(m_languageCode.indexOf(QLatin1Char('@')))});
        connect(&proc, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this] {
            const QString output = QString::fromUtf8(proc.readAllStandardOutput().simplified());
            // Whenever we don't get packages back simply pretend the language is complete as we can't
            // give any useful information on what's wrong anyway.
            Q_EMIT finished(output.isEmpty() ? Result::Complete : Result::Incomplete, output.split(QLatin1Char(' ')));
        });
        proc.start();
    }

private:
    QProcess proc;
};

template<typename... Args>
CompletionCheck *CompletionCheck::create(Args &&..._args)
{
    KOSRelease os;
    if (os.id() == QLatin1String("ubuntu") || os.idLike().contains(QLatin1String("ubuntu"))) {
        return new UbuntuCompletionCheck(std::forward<Args>(_args)...);
    }
    return nullptr;
}

void Language::reloadCompleteness()
{
    auto *check = CompletionCheck::create(code, this);
    if (!check) {
        return; // no checking support - default to assume complete
    }
    connect(check, &CompletionCheck::finished, this, [this, check](CompletionCheck::Result result, const QStringList &missingPackages) {
        check->deleteLater();

        switch (result) {
        case CompletionCheck::Result::Error:
            qCWarning(KCM_TRANSLATIONS) << "Failed to get completion status for" << code;
            return;
        case CompletionCheck::Result::Incomplete: {
            // Cache this, we need to modify the data before marking the change on the model.
            const bool changed = (packages != missingPackages);
            state = Language::State::Incomplete;
            packages = missingPackages;
            if (changed) {
                Q_EMIT stateChanged();
            }
            return;
        }
        case CompletionCheck::Result::Complete:
            if (state != Language::State::Complete) {
                state = Language::State::Complete;
                packages.clear();
                Q_EMIT stateChanged();
            }
            return;
        }
    });
    check->start();
}

void Language::complete()
{
#ifdef HAVE_PACKAGEKIT
    auto completer = new LanguageCompleter(packages, this);
    connect(completer, &LanguageCompleter::complete, this, [completer, this] {
        completer->deleteLater();
        reloadCompleteness();
    });
    state = Language::State::Installing;
    packages.clear();
    completer->start();
    Q_EMIT stateChanged();
#endif
}

Language::Language(const QString &code_, State state_, QObject *parent)
    : QObject(parent)
    , code(code_)
    , state(state_)
{
}

Language::Language(const QString &code_, QObject *parent)
    : Language(code_, State::Complete, parent)
{
}

#include "language.moc"
