/*
    localegeneratorubuntu.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <PackageKit/Daemon>

#include <QStandardPaths>

#include <KLocalizedString>

#include "kcm_regionandlang_debug.h"
#include "localegeneratorubuntu.h"

void LocaleGeneratorUbuntu::localesGenerate(const QStringList &list)
{
    ubuntuInstall(list);
}

void LocaleGeneratorUbuntu::ubuntuInstall(const QStringList &locales)
{
    m_packageIDs.clear();
    if (!m_proc) {
        m_proc = new QProcess(this);
    }
    QStringList args;
    args.reserve(locales.size());
    for (const auto &locale : locales) {
        auto localeStripped = locale;
        localeStripped.remove(QStringLiteral(".UTF-8"));
        args.append(QStringLiteral("--language=%1").arg(localeStripped));
    }
    const QString binaryPath = QStandardPaths::findExecutable(QStringLiteral("check-language-support"));
    if (!binaryPath.isEmpty()) {
        m_proc->setProgram(binaryPath);
        m_proc->setArguments(args);
        connect(m_proc, &QProcess::finished, this, &LocaleGeneratorUbuntu::ubuntuLangCheck);
        m_proc->start();
    } else {
        Q_EMIT userHasToGenerateManually(i18nc("@info:warning", "Can't locate executable `check-language-support`"));
    }
}

void LocaleGeneratorUbuntu::ubuntuLangCheck(int statusCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)
    if (statusCode != 0) {
        // Something wrong with this Ubuntu, don't try further
        Q_EMIT userHasToGenerateManually(i18nc("the arg is the output of failed check-language-support call",
                                               "check-language-support failed, output: %1",
                                               QString::fromUtf8(m_proc->readAllStandardOutput())));
        return;
    }
    const QString output = QString::fromUtf8(m_proc->readAllStandardOutput().simplified());
    QStringList packages = output.split(QLatin1Char(' '));
    packages.erase(std::remove_if(packages.begin(),
                                  packages.end(),
                                  [](QString &i) {
                                      return i.isEmpty();
                                  }),
                   packages.end());

    if (!packages.isEmpty()) {
        auto transaction = PackageKit::Daemon::resolve(packages, PackageKit::Transaction::FilterNotInstalled | PackageKit::Transaction::FilterArch);
        connect(transaction,
                &PackageKit::Transaction::package,
                this,
                [this](PackageKit::Transaction::Info info, const QString &packageID, const QString &summary) {
                    Q_UNUSED(info);
                    Q_UNUSED(summary);
                    m_packageIDs << packageID;
                });
        connect(transaction, &PackageKit::Transaction::errorCode, this, [](PackageKit::Transaction::Error error, const QString &details) {
            qCDebug(KCM_REGIONANDLANG) << "resolve error" << error << details;
        });
        connect(transaction, &PackageKit::Transaction::finished, this, [packages, this](PackageKit::Transaction::Exit status, uint code) {
            qCDebug(KCM_REGIONANDLANG) << "resolve finished" << status << code << m_packageIDs;
            if (m_packageIDs.size() != packages.size()) {
                qCWarning(KCM_REGIONANDLANG) << "Not all missing packages managed to resolve!" << packages << m_packageIDs;
                const QString packagesString = packages.join(QLatin1Char(';'));
                Q_EMIT userHasToGenerateManually(i18nc("%1 is a list of package names", "Not all missing packages managed to resolve! %1", packagesString));
                return;
            }
            auto transaction = PackageKit::Daemon::installPackages(m_packageIDs);
            connect(transaction, &PackageKit::Transaction::errorCode, this, [this](PackageKit::Transaction::Error error, const QString &details) {
                qCDebug(KCM_REGIONANDLANG) << "install error:" << error << details;
                Q_EMIT userHasToGenerateManually(i18nc("%1 is a list of package names", "Failed to install package %1", details));
            });
            connect(transaction, &PackageKit::Transaction::finished, this, [this](PackageKit::Transaction::Exit status, uint code) {
                qCDebug(KCM_REGIONANDLANG) << "install finished:" << status << code;
                if (status == PackageKit::Transaction::Exit::ExitSuccess) {
                    Q_EMIT success();
                }
            });
        });
    } else {
        Q_EMIT success();
    }
}
