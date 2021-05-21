/*
 *  Copyright (C) 2014 John Layt <john@layt.net>
 *  Copyright (C) 2018 Eike Hein <hein@kde.org>
 *  Copyright (C) 2021 Harald Sitter <sitter@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "translationsmodel.h"

#include <KLocalizedString>
#include <KOSRelease>

#include <QCollator>
#include <QDebug>
#include <QLocale>
#include <QMetaEnum>
#include <QMetaObject>
#include <QProcess>

#include "config-workspace.h"
#include "debug.h"

#ifdef HAVE_PACKAGEKIT
#include <PackageKit/Daemon>
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
    static CompletionCheck *create(Args &&... _args);
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
CompletionCheck *CompletionCheck::create(Args &&... _args)
{
#ifdef HAVE_PACKAGEKIT
    // Ubuntu completion depends on packagekit. When packagekit is not available there's no point supporting
    // completion checking as we'll have no way to complete the language if it is incomplete.
    KOSRelease os;
    if (os.id() == QLatin1String("ubuntu") || os.idLike().contains(QLatin1String("ubuntu"))) {
        return new UbuntuCompletionCheck(std::forward<Args>(_args)...);
    }
#endif
    return nullptr;
}

QStringList TranslationsModel::m_languages = QStringList();
QSet<QString> TranslationsModel::m_installedLanguages = QSet<QString>();

TranslationsModel::TranslationsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    if (m_installedLanguages.isEmpty()) {
        m_installedLanguages = KLocalizedString::availableDomainTranslations("plasmashell");
        m_languages = m_installedLanguages.values();
    }
}

QHash<int, QByteArray> TranslationsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    const auto e = QMetaEnum::fromType<AdditionalRoles>();
    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QVariant TranslationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_languages.count()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return languageCodeToName(m_languages.at(index.row()));
    } else if (role == LanguageCode) {
        return m_languages.at(index.row());
    } else if (role == IsMissing) {
        return false;
    }

    return QVariant();
}

int TranslationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_languages.count();
}

QString TranslationsModel::languageCodeToName(const QString &languageCode) const
{
    const QLocale locale(languageCode);
    const QString &languageName = locale.nativeLanguageName();

    if (languageName.isEmpty()) {
        return languageCode;
    }

    if (languageCode.contains(QLatin1Char('@'))) {
        return i18nc("%1 is language name, %2 is language code name", "%1 (%2)", languageName, languageCode);
    }

    if (locale.name() != languageCode && m_installedLanguages.contains(locale.name())) {
        // KDE languageCode got translated by QLocale to a locale code we also have on
        // the list. Currently this only happens with pt that gets translated to pt_BR.
        if (languageCode == QLatin1String("pt")) {
            return QLocale(QStringLiteral("pt_PT")).nativeLanguageName();
        }

        qCWarning(KCM_TRANSLATIONS) << "Language code morphed into another existing language code, please report!" << languageCode << locale.name();
        return i18nc("%1 is language name, %2 is language code name", "%1 (%2)", languageName, languageCode);
    }

    return languageName;
}

QVariant SelectedTranslationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_selectedLanguages.count()) {
        return QVariant();
    }

    const QString code = m_selectedLanguages.at(index.row());
    if (role == Qt::DisplayRole) {
        return languageCodeToName(code);
    } else if (role == LanguageCode) {
        return code;
    } else if (role == IsMissing) {
        return m_missingLanguages.contains(code);
    } else if (role == IsIncomplete) {
        return m_incompleteLanguagesWithPackages.contains(code);
    } else if (role == IsInstalling) {
        return m_installingLanguages.contains(code);
    }

    return QVariant();
}

int SelectedTranslationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_selectedLanguages.count();
}

QStringList SelectedTranslationsModel::selectedLanguages() const
{
    return m_selectedLanguages;
}

void SelectedTranslationsModel::setSelectedLanguages(const QStringList &languages)
{
    if (m_selectedLanguages == languages) {
        return;
    }

    QStringList missingLanguages;

    for (const QString &lang : languages) {
        reloadCompleteness(lang);
        if (!m_installedLanguages.contains(lang)) {
            missingLanguages << lang;
        }
    }

    missingLanguages.sort();

    if (missingLanguages != m_missingLanguages) {
        m_missingLanguages = missingLanguages;
        emit missingLanguagesChanged();
    }

    beginResetModel();

    m_selectedLanguages = languages;

    endResetModel();

    emit selectedLanguagesChanged(m_selectedLanguages);
}

QStringList SelectedTranslationsModel::missingLanguages() const
{
    return m_missingLanguages;
}

void SelectedTranslationsModel::reloadCompleteness(const QString &languageCode)
{
    auto *check = CompletionCheck::create(languageCode, this);
    if (!check) {
        return; // no checking support - default to assume complete
    }
    connect(check, &CompletionCheck::finished, this, [this, languageCode, check](CompletionCheck::Result result, const QStringList &missingPackages) {
        check->deleteLater();

        const int index = m_selectedLanguages.indexOf(languageCode);
        if (index < 0) { // removed since the check was started
            return;
        }

        switch (result) {
        case CompletionCheck::Result::Error:
            qCWarning(KCM_TRANSLATIONS) << "Failed to get completion status for" << languageCode;
            return;
        case CompletionCheck::Result::Incomplete: {
            // Cache this, we need to modify the data before marking the change on the model.
            const bool changed = !m_incompleteLanguagesWithPackages.contains(languageCode);
            m_incompleteLanguagesWithPackages[languageCode] = missingPackages;
            if (changed) {
                Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 0), {IsIncomplete});
            }
            return;
        }
        case CompletionCheck::Result::Complete:
            if (m_incompleteLanguagesWithPackages.remove(languageCode) > 0) {
                Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 0), {IsIncomplete});
            }
            return;
        }
    });
    check->start();
}

void SelectedTranslationsModel::completeLanguage(int index)
{
#ifdef HAVE_PACKAGEKIT
    const QString code = m_selectedLanguages.at(index);

    auto completer = new LanguageCompleter(m_incompleteLanguagesWithPackages.value(code), this);
    connect(completer, &LanguageCompleter::complete, this, [this, code] {
        sender()->deleteLater();
        const int index = m_selectedLanguages.indexOf(code);
        if (index < 0) {
            return; // entry was probably removed since the install was started
        }
        m_installingLanguages.removeAll(code);
        reloadCompleteness(code);
        Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 0), {IsInstalling});
    });
    m_incompleteLanguagesWithPackages.remove(code);
    m_installingLanguages << code;
    completer->start();
    Q_EMIT dataChanged(createIndex(index, 0), createIndex(index, 0), {IsIncomplete, IsInstalling});
#else
    Q_UNUSED(index);
#endif
}

void SelectedTranslationsModel::move(int from, int to)
{
    if (from >= m_selectedLanguages.count() || to >= m_selectedLanguages.count()) {
        return;
    }

    if (from == to) {
        return;
    }

    const int modelTo = to + (to > from ? 1 : 0);

    const bool ok = beginMoveRows(QModelIndex(), from, from, QModelIndex(), modelTo);

    if (ok) {
        m_selectedLanguages.move(from, to);

        endMoveRows();

        emit selectedLanguagesChanged(m_selectedLanguages);
    }
}

void SelectedTranslationsModel::remove(const QString &languageCode)
{
    if (languageCode.isEmpty()) {
        return;
    }

    int index = m_selectedLanguages.indexOf(languageCode);

    if (index < 0 || m_selectedLanguages.count() < 2) {
        return;
    }

    beginRemoveRows(QModelIndex(), index, index);

    m_selectedLanguages.removeAt(index);

    endRemoveRows();

    emit selectedLanguagesChanged(m_selectedLanguages);
}

QVariant AvailableTranslationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_availableLanguages.count()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return languageCodeToName(m_availableLanguages.at(index.row()));
    } else if (role == LanguageCode) {
        return m_availableLanguages.at(index.row());
    } else if (role == IsMissing) {
        return false;
    }

    return QVariant();
}

int AvailableTranslationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_availableLanguages.count();
}

QString AvailableTranslationsModel::langCodeAt(int row)
{
    if (row < 0 || row >= m_availableLanguages.count()) {
        return QString();
    }

    return m_availableLanguages.at(row);
}

void AvailableTranslationsModel::setSelectedLanguages(const QStringList &languages)
{
    beginResetModel();

    m_availableLanguages = (m_installedLanguages - QSet<QString>(languages.cbegin(), languages.cend())).values();

    QCollator c;
    c.setCaseSensitivity(Qt::CaseInsensitive);

    std::sort(m_availableLanguages.begin(), m_availableLanguages.end(), [this, &c](const QString &a, const QString &b) {
        return c.compare(languageCodeToName(a), languageCodeToName(b)) < 0;
    });

    endResetModel();
}

#include "translationsmodel.moc"
