/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "appentry.h"
#include "actionlist.h"
#include "appsmodel.h"
#include "containmentinterface.h"
#include <config-workspace.h>

#include <QFileInfo>
#include <QProcess>
#include <QQmlPropertyMap>
#include <QStandardPaths>

#include <KActivities/ResourceInstance>
#include <KApplicationTrader>
#include <KConfigGroup>
#include <KIO/ApplicationLauncherJob>
#include <KJob>
#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KSharedConfig>
#include <KShell>
#include <KSycoca>
#include <KWindowSystem>

#include <Plasma/Plasma>

#ifdef HAVE_ICU
#include <unicode/translit.h>
#endif

namespace
{

#ifdef HAVE_ICU
std::unique_ptr<icu::Transliterator> getICUTransliterator(const QLocale &locale)
{
    // Only use transliterator for certain locales.
    // Because application name is a localized string, it would be really rare to
    // have Chinese/Japanese character on other locales. Even if that happens, it
    // is ok to use to the old 1 character strategy instead of using transliterator.
    icu::UnicodeString id;
    if (locale.language() == QLocale::Japanese) {
        id = "Katakana-Hiragana";
    } else if (locale.language() == QLocale::Chinese) {
        id = "Han-Latin; Latin-ASCII";
    }
    if (id.isEmpty()) {
        return nullptr;
    }
    auto ue = UErrorCode::U_ZERO_ERROR;
    auto transliterator = std::unique_ptr<icu::Transliterator>(icu::Transliterator::createInstance(id, UTRANS_FORWARD, ue));

    if (ue != UErrorCode::U_ZERO_ERROR) {
        return nullptr;
    }

    return transliterator;
}
#endif

QString groupName(const QString &name)
{
    if (name.isEmpty()) {
        return QString();
    }

    const QChar firstChar = name[0];

    // Put all applications whose names begin with numbers in group #
    if (firstChar.isDigit()) {
        return QStringLiteral("#");
    }

    // Put all applications whose names begin with punctuations/symbols/spaces in group &
    if (firstChar.isPunct() || firstChar.isSymbol() || firstChar.isSpace()) {
        return QStringLiteral("&");
    }

    // Here we will apply a locale based strategy for the first character.
    // If first character is hangul, run decomposition and return the choseong (consonants).
    if (firstChar.script() == QChar::Script_Hangul) {
        auto decomposed = firstChar.decomposition();
        if (decomposed.isEmpty()) {
            return name.left(1);
        }
        return decomposed.left(1);
    }
    const auto locale = QLocale::system();
    if (locale.language() == QLocale::Japanese) {
        // We do this here for Japanese locale because:
        // 1. it does not make much sense to have every different Kanji to have a different group.
        // 2. ICU transliterator can't yet convert Kanji to Hiragana.
        //    https://unicode-org.atlassian.net/browse/ICU-5874
        if (firstChar.script() == QChar::Script_Han) {
            // Unicode Han
            return QString::fromUtf8("\xe6\xbc\xa2");
        }
    }
#ifdef HAVE_ICU
    // Precondition to use transliterator.
    if ((locale.language() == QLocale::Chinese && firstChar.script() == QChar::Script_Han)
        || (locale.language() == QLocale::Japanese && firstChar.script() == QChar::Script_Katakana)) {
        static auto transliterator = getICUTransliterator(locale);

        if (transliterator) {
            icu::UnicodeString icuText(reinterpret_cast<const char16_t *>(name.data()), name.size());
            transliterator->transliterate(icuText);
            return QString::fromUtf16(icuText.getBuffer(), static_cast<int>(icuText.length())).left(1);
        }
    }
#endif
    return name.left(1);
}
}

AppEntry::AppEntry(AbstractModel *owner, KService::Ptr service, NameFormat nameFormat)
    : AbstractEntry(owner)
    , m_service(service)
{
    Q_ASSERT(service);
    init(nameFormat);
}

AppEntry::AppEntry(AbstractModel *owner, const QString &id)
    : AbstractEntry(owner)
{
    const QUrl url(id);
    if (url.scheme() == QLatin1String("preferred")) {
        m_service = defaultAppByName(url.host());
        m_id = id;
    } else {
        m_service = KService::serviceByStorageId(id);
    }
    if (!m_service) {
        m_service = new KService(QString());
    }

    m_con = QObject::connect(KSycoca::self(), &KSycoca::databaseChanged, owner, [this, owner, id]() {
        const QUrl url(id);
        if (url.scheme() == QLatin1String("preferred")) {
            KSharedConfig::openConfig()->reparseConfiguration();
            m_service = defaultAppByName(url.host());
            if (m_service) {
                init((NameFormat)owner->rootModel()->property("appNameFormat").toInt());
                m_icon = QIcon();
                Q_EMIT owner->layoutChanged();
            }
        } else {
            m_service = KService::serviceByStorageId(id);
        }
        if (!m_service) {
            m_service = new KService(QString());
        }
    });

    if (m_service->isValid()) {
        init((NameFormat)owner->rootModel()->property("appNameFormat").toInt());
    }
}

void AppEntry::init(NameFormat nameFormat)
{
    m_name = nameFromService(m_service, nameFormat);

    if (nameFormat == GenericNameOnly) {
        m_description = nameFromService(m_service, NameOnly);
    } else {
        m_description = nameFromService(m_service, GenericNameOnly);
    }
}

bool AppEntry::isValid() const
{
    return m_service->isValid();
}

QIcon AppEntry::icon() const
{
    if (m_icon.isNull()) {
        const QString serviceIcon = m_service->icon();

        // Check for absolute-path-ness this way rather than using
        // QFileInfo.isAbsolute() because that would perform a ton of unnecessary
        // filesystem checks, and most icons are not defined in apps' desktop
        // files with absolute paths.
        bool isAbsoluteFilePath = serviceIcon.startsWith(QLatin1String("/"));

        // Need to first check for whether the icon has an absolute path, because
        // otherwise if the icon is just a name, QFileInfo will treat it as a
        // relative path and return true if there randomly happens to be a file
        // with the name of an icon in the user's homedir and we'll go down the
        // wrong codepath and end up with a blank QIcon; See 457965.
        if (isAbsoluteFilePath && QFileInfo::exists(serviceIcon)) {
            m_icon = QIcon(serviceIcon);
        } else {
            m_icon = QIcon::fromTheme(serviceIcon, QIcon::fromTheme(QStringLiteral("unknown")));
        }
    }
    return m_icon;
}

QString AppEntry::name() const
{
    return m_name;
}

QString AppEntry::description() const
{
    return m_description;
}

KService::Ptr AppEntry::service() const
{
    return m_service;
}

QString AppEntry::group() const
{
    if (m_group.isNull()) {
        m_group = groupName(m_name);
        if (m_group.isNull()) {
            m_group = QLatin1String("");
        }
        Q_ASSERT(!m_group.isNull());
    }
    return m_group;
}

QString AppEntry::id() const
{
    if (!m_id.isEmpty()) {
        return m_id;
    }

    return m_service->storageId();
}

QString AppEntry::menuId() const
{
    return m_service->menuId();
}

QUrl AppEntry::url() const
{
    return QUrl::fromLocalFile(Kicker::resolvedServiceEntryPath(m_service));
}

bool AppEntry::hasActions() const
{
    return true;
}

QVariantList AppEntry::actions() const
{
    QVariantList actionList;

    actionList << Kicker::jumpListActions(m_service);
    if (!actionList.isEmpty()) {
        actionList << Kicker::createSeparatorActionItem();
    }

    QObject *appletInterface = m_owner->rootModel()->property("appletInterface").value<QObject *>();

    bool systemImmutable = false;
    if (appletInterface) {
        systemImmutable = (appletInterface->property("immutability").toInt() == Plasma::Types::SystemImmutable);
    }

    const QVariantList &addLauncherActions = Kicker::createAddLauncherActionList(appletInterface, m_service);
    if (!systemImmutable && !addLauncherActions.isEmpty()) {
        actionList << addLauncherActions;
    }

    const QVariantList &recentDocuments = Kicker::recentDocumentActions(m_service);
    if (!recentDocuments.isEmpty()) {
        actionList << recentDocuments << Kicker::createSeparatorActionItem();
    }

    const QVariantList &additionalActions = Kicker::additionalAppActions(m_service);
    if (!additionalActions.isEmpty()) {
        actionList << additionalActions << Kicker::createSeparatorActionItem();
    }

    // Don't allow adding launchers, editing, hiding, or uninstalling applications
    // when system is immutable.
    if (systemImmutable) {
        return actionList;
    }

    if (m_service->isApplication()) {
        actionList << Kicker::createSeparatorActionItem();
        actionList << Kicker::editApplicationAction(m_service);
        actionList << Kicker::appstreamActions(m_service);
    }

    if (appletInterface) {
        QQmlPropertyMap *appletConfig = qobject_cast<QQmlPropertyMap *>(appletInterface->property("configuration").value<QObject *>());

        if (appletConfig && appletConfig->contains(QStringLiteral("hiddenApplications")) && qobject_cast<AppsModel *>(m_owner)) {
            const QStringList &hiddenApps = appletConfig->value(QStringLiteral("hiddenApplications")).toStringList();

            if (!hiddenApps.contains(m_service->menuId())) {
                QVariantMap hideAction = Kicker::createActionItem(i18n("Hide Application"), QStringLiteral("view-hidden"), QStringLiteral("hideApplication"));
                actionList << hideAction;
            }
        }
    }

    return actionList;
}

bool AppEntry::run(const QString &actionId, const QVariant &argument)
{
    if (!m_service->isValid()) {
        return false;
    }

    if (actionId.isEmpty()) {
        auto *job = new KIO::ApplicationLauncherJob(m_service);
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->setRunFlags(KIO::ApplicationLauncherJob::DeleteTemporaryFiles);
        job->start();

        KActivities::ResourceInstance::notifyAccessed(QUrl(QStringLiteral("applications:") + m_service->storageId()), QStringLiteral("org.kde.plasma.kicker"));

        return true;
    }

    QObject *appletInterface = m_owner->rootModel()->property("appletInterface").value<QObject *>();

    if (Kicker::handleAddLauncherAction(actionId, appletInterface, m_service)) {
        return false; // We don't want to close Kicker, BUG: 390585
    } else if (Kicker::handleEditApplicationAction(actionId, m_service)) {
        return true;
    } else if (Kicker::handleAppstreamActions(actionId, m_service)) {
        return true;
    } else if (actionId == QLatin1String("_kicker_jumpListAction")) {
        auto *job = new KIO::ApplicationLauncherJob(argument.value<KServiceAction>());
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        return job->exec();
    } else if (Kicker::handleAdditionalAppActions(actionId, m_service, argument)) {
        return true;
    }

    return Kicker::handleRecentDocumentAction(m_service, actionId, argument);
}

QString AppEntry::nameFromService(const KService::Ptr &service, NameFormat nameFormat)
{
    const QString &name = service->name();
    QString genericName = service->genericName();

    if (genericName.isEmpty()) {
        genericName = service->comment();
    }

    if (nameFormat == NameOnly || genericName.isEmpty() || name == genericName) {
        return name;
    } else if (nameFormat == GenericNameOnly) {
        return genericName;
    } else if (nameFormat == NameAndGenericName) {
        return i18nc("App name (Generic name)", "%1 (%2)", name, genericName);
    } else {
        return i18nc("Generic name (App name)", "%1 (%2)", genericName, name);
    }
}

KService::Ptr AppEntry::defaultAppByName(const QString &name)
{
    if (name == QLatin1String("browser")) {
        KConfigGroup config(KSharedConfig::openConfig(), "General");
        QString browser = config.readPathEntry("BrowserApplication", QString());

        if (browser.isEmpty()) {
            return KApplicationTrader::preferredService(QStringLiteral("text/html"));
        } else if (browser.startsWith(QLatin1Char('!'))) {
            browser.remove(0, 1);
        }

        return KService::serviceByStorageId(browser);
    }

    return KService::Ptr();
}

AppEntry::~AppEntry()
{
    if (m_con) {
        QObject::disconnect(m_con);
    }
}

AppGroupEntry::AppGroupEntry(AppsModel *parentModel,
                             KServiceGroup::Ptr group,
                             bool paginate,
                             int pageSize,
                             bool flat,
                             bool sorted,
                             bool separators,
                             int appNameFormat)
    : AbstractGroupEntry(parentModel)
    , m_group(group)
{
    AppsModel *model = new AppsModel(group->entryPath(), paginate, pageSize, flat, sorted, separators, parentModel);
    model->setAppNameFormat(appNameFormat);
    m_childModel = model;

    QObject::connect(parentModel, &AppsModel::cleared, model, &AppsModel::deleteLater);

    QObject::connect(model, &AppsModel::countChanged, [parentModel, this] {
        if (parentModel) {
            parentModel->entryChanged(this);
        }
    });

    QObject::connect(model, &AppsModel::hiddenEntriesChanged, [parentModel, this] {
        if (parentModel) {
            parentModel->entryChanged(this);
        }
    });
}

QIcon AppGroupEntry::icon() const
{
    if (m_icon.isNull()) {
        m_icon = QIcon::fromTheme(m_group->icon(), QIcon::fromTheme(QStringLiteral("unknown")));
    }
    return m_icon;
}

QString AppGroupEntry::name() const
{
    return m_group->caption();
}

bool AppGroupEntry::hasChildren() const
{
    return m_childModel && m_childModel->count() > 0;
}

AbstractModel *AppGroupEntry::childModel() const
{
    return m_childModel;
}

Q_DECLARE_METATYPE(KServiceAction)
