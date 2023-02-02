/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "job_p.h"

#include "debug.h"

#include <QDBusConnection>
#include <QDebug>

#include <KFilePlacesModel>
#include <KLocalizedString>
#include <KShell>

#include <kio/global.h>

#include "jobviewv2adaptor.h"
#include "jobviewv3adaptor.h"

using namespace NotificationManager;

JobPrivate::JobPrivate(uint id, QObject *parent)
    : QObject(parent)
    , m_id(id)
{
    m_showTimer.setSingleShot(true);
    connect(&m_showTimer, &QTimer::timeout, this, &JobPrivate::requestShow);

    m_objectPath.setPath(QStringLiteral("/org/kde/notificationmanager/jobs/JobView_%1").arg(id));

    // TODO also v1? it's identical to V2 except it doesn't have setError method so supporting it should be easy
    new JobViewV2Adaptor(this);
    new JobViewV3Adaptor(this);

    QDBusConnection::sessionBus().registerObject(m_objectPath.path(), this);
}

JobPrivate::~JobPrivate() = default;

void JobPrivate::requestShow()
{
    if (!m_showRequested) {
        m_showRequested = true;
        Q_EMIT showRequested();
    }
}

QDBusObjectPath JobPrivate::objectPath() const
{
    return m_objectPath;
}

std::shared_ptr<KFilePlacesModel> JobPrivate::createPlacesModel()
{
    static std::shared_ptr<KFilePlacesModel> s_instance;
    if (!s_instance) {
        s_instance = std::make_shared<KFilePlacesModel>();
    }
    return s_instance;
}

QUrl JobPrivate::localFileOrUrl(const QString &urlString)
{
    QUrl url(urlString);
    if (url.scheme().isEmpty()) {
        url = QUrl::fromLocalFile(urlString);
    }
    return url;
}

QString JobPrivate::linkify(const QUrl &url, const QString &caption)
{
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(url.toString(QUrl::PrettyDecoded), caption.toHtmlEscaped());
}

QUrl JobPrivate::destUrl() const
{
    QUrl url = m_destUrl;
    // In case of a single file and no destUrl, try using the second label (most likely "Destination")...
    if (!url.isValid() && m_totalFiles == 1) {
        url = localFileOrUrl(m_descriptionValue2).adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
    }
    return url;
}

QString JobPrivate::prettyUrl(const QUrl &_url) const
{
    QUrl url(_url);

    if (!url.isValid()) {
        return QString();
    }

    if (url.path().endsWith(QLatin1String("/."))) {
        url.setPath(url.path().chopped(2));
    }

    if (!m_placesModel) {
        m_placesModel = createPlacesModel();
    }

    // Mimic KUrlNavigator and show a pretty place name,
    // for example Documents/foo/bar rather than /home/user/Documents/foo/bar
    const QModelIndex closestIdx = m_placesModel->closestItem(url);
    if (closestIdx.isValid()) {
        const QUrl placeUrl = m_placesModel->url(closestIdx);

        QString text = m_placesModel->text(closestIdx);

        QString pathInsidePlace = url.path().mid(placeUrl.path().length());

        if (!pathInsidePlace.startsWith(QLatin1Char('/'))) {
            pathInsidePlace.prepend(QLatin1Char('/'));
        }

        if (pathInsidePlace != QLatin1Char('/')) {
            // Avoid "500 GiB Internal Hard Drive/foo/bar" when path originates directly from root.
            const bool isRoot = placeUrl.isLocalFile() && placeUrl.path() == QLatin1Char('/');
            if (isRoot) {
                text = pathInsidePlace;
            } else {
                text.append(pathInsidePlace);
            }
        }

        return text;
    }

    if (url.isLocalFile()) {
        return KShell::tildeCollapse(url.toLocalFile());
    }

    return url.toDisplayString(QUrl::RemoveUserInfo);
}

void JobPrivate::updateHasDetails()
{
    // clang-format off
    const bool hasDetails = m_totalBytes > 0
        || m_totalFiles > 0
        || m_totalDirectories > 0
        || m_totalItems > 0
        || m_processedBytes > 0
        || m_processedFiles > 0
        || m_processedDirectories > 0
        || m_processedItems > 0
        || !m_descriptionValue1.isEmpty()
        || !m_descriptionValue2.isEmpty()
        || m_speed > 0;
    // clang-format on

    if (m_hasDetails != hasDetails) {
        m_hasDetails = hasDetails;
        Q_EMIT static_cast<Job *>(parent())->hasDetailsChanged();
    }
}

QString JobPrivate::text() const
{
    if (!m_errorText.isEmpty()) {
        return m_errorText;
    }

    if (!m_infoMessage.isEmpty()) {
        return m_infoMessage;
    }

    const QUrl destUrl = this->destUrl();
    const QString prettyDestUrl = prettyUrl(destUrl);

    QString destUrlString;
    if (!prettyDestUrl.isEmpty()) {
        // Turn destination into a clickable hyperlink
        destUrlString = linkify(destUrl, prettyDestUrl);
    }

    if (m_totalFiles > 1) {
        if (!destUrlString.isEmpty()) {
            if (m_processedFiles > 0 && m_processedFiles <= m_totalFiles) {
                return i18ncp("Copying n of m files to locaton", "%2 of %1 file to %3", "%2 of %1 files to %3", m_totalFiles, m_processedFiles, destUrlString);
            }
            return i18ncp("Copying n files to location",
                          "%1 file to %2",
                          "%1 files to %2",
                          m_processedFiles > 0 ? m_processedFiles : m_totalFiles,
                          destUrlString);
        }

        if (m_processedFiles > 0 && m_processedFiles <= m_totalFiles) {
            return i18ncp("Copying n of m files", "%2 of %1 file", "%2 of %1 files", m_totalFiles, m_processedFiles);
        }

        return i18ncp("Copying n files", "%1 file", "%1 files", m_processedFiles > 0 ? m_processedFiles : m_totalFiles);
    } else if (m_totalItems > 1) {
        // TODO support destUrl text as well (once someone actually uses that)

        if (m_processedItems <= m_totalItems) {
            return i18ncp("Copying n of m items", "%2 of %1 item", "%2 of %1 items", m_totalItems, m_processedItems);
        }

        return i18ncp("Copying n items", "%1 item", "%1 items", m_processedItems > 0 ? m_processedItems : m_totalItems);
    } else if (m_totalFiles == 1 || m_totalItems == 1) {
        const QUrl url = descriptionUrl();

        if (!destUrlString.isEmpty()) {
            const QString currentFileName = url.fileName().toHtmlEscaped();
            if (!currentFileName.isEmpty()) {
                return i18nc("Copying file to location", "%1 to %2", currentFileName, destUrlString);
            } else {
                if (m_totalItems) {
                    return i18ncp("Copying n items to location", "%1 file to %2", "%1 items to %2", m_totalItems, destUrlString);
                } else {
                    return i18ncp("Copying n files to location", "%1 file to %2", "%1 files to %2", m_totalFiles, destUrlString);
                }
            }
        } else if (url.isValid()) {
            // If no destination, show full URL instead of just the file name
            return linkify(url, prettyUrl(url));
        } else {
            if (m_totalItems) {
                return i18ncp("Copying n items", "%1 item", "%1 items", m_totalItems);
            } else {
                return i18ncp("Copying n files", "%1 file", "%1 files", m_totalFiles);
            }
        }
    } else if (m_totalFiles == 0) {
        if (!destUrlString.isEmpty()) {
            if (m_processedFiles > 0) {
                return i18ncp("Copying n files to location", "%1 file to %2", "%1 files to %2", m_processedFiles, destUrlString);
            }
            return i18nc("Copying unknown amount of files to location", "to %1", destUrlString);
        } else if (m_processedFiles > 0) {
            return i18ncp("Copying n files", "%1 file", "%1 files", m_processedFiles);
        }
    }

    qCInfo(NOTIFICATIONMANAGER) << "Failed to generate job text for job with following properties:";
    qCInfo(NOTIFICATIONMANAGER).nospace() << "  processedFiles = " << m_processedFiles << ", totalFiles = " << m_totalFiles;
    qCInfo(NOTIFICATIONMANAGER).nospace() << "  processedItems = " << m_processedItems << ", totalItems = " << m_totalItems;
    qCInfo(NOTIFICATIONMANAGER).nospace() << "  current file name = " << descriptionUrl().fileName();
    qCInfo(NOTIFICATIONMANAGER).nospace() << "  destination url = " << destUrl;
    qCInfo(NOTIFICATIONMANAGER).nospace() << "  label1 = " << m_descriptionLabel1 << ", value1 = " << m_descriptionValue1;
    qCInfo(NOTIFICATIONMANAGER).nospace() << "  label2 = " << m_descriptionLabel2 << ", value2 = " << m_descriptionValue2;

    return QString();
}

void JobPrivate::delayedShow(std::chrono::milliseconds delay, ShowConditions showConditions)
{
    m_showConditions = showConditions;

    if (showConditions.testFlag(ShowCondition::OnTimeout)) {
        m_showTimer.start(delay);
    }
}

void JobPrivate::kill()
{
    Q_EMIT cancelRequested();

    // In case the application doesn't respond, remove the job
    if (!m_killTimer) {
        m_killTimer = new QTimer(this);
        m_killTimer->setSingleShot(true);
        connect(m_killTimer, &QTimer::timeout, this, [this] {
            qCWarning(NOTIFICATIONMANAGER) << "Application" << m_applicationName << "failed to respond to a cancel request in time";
            Job *job = static_cast<Job *>(parent());
            job->setError(KIO::ERR_USER_CANCELED);
            job->setState(Notifications::JobStateStopped);
            finish();
        });
    }

    if (!m_killTimer->isActive()) {
        m_killTimer->start(2000);
    }
}

QUrl JobPrivate::descriptionUrl() const
{
    QUrl url = localFileOrUrl(m_descriptionValue2);
    if (!url.isValid()) {
        url = localFileOrUrl(m_descriptionValue1);
    }
    return url;
}

void JobPrivate::finish()
{
    // Unregister the dbus service since the client is done with it
    QDBusConnection::sessionBus().unregisterObject(m_objectPath.path());

    // When user canceled job or a transient job finished successfully, remove it without notice
    if (m_error == KIO::ERR_USER_CANCELED || (!m_error && m_transient)) {
        Q_EMIT closed();
        return;
    }

    if (m_killTimer) {
        m_killTimer->stop();
    }

    Job *job = static_cast<Job *>(parent());
    // update timestamp
    job->resetUpdated();
    // when it was hidden in history, bring it up again
    job->setDismissed(false);
}

// JobViewV2
void JobPrivate::terminate(const QString &errorMessage)
{
    Job *job = static_cast<Job *>(parent());
    // forward to JobViewV3. In V2 we get a setError before a terminate
    // so we want to forward the current error to the V3 call.
    terminate(job->error(), errorMessage, {});
}

void JobPrivate::setSuspended(bool suspended)
{
    Job *job = static_cast<Job *>(parent());
    if (suspended) {
        job->setState(Notifications::JobStateSuspended);
    } else {
        job->setState(Notifications::JobStateRunning);
    }
}

void JobPrivate::setTotalAmount(quint64 amount, const QString &unit)
{
    if (unit == QLatin1String("bytes")) {
        updateField(amount, m_totalBytes, &Job::totalBytesChanged);
    } else if (unit == QLatin1String("files")) {
        updateField(amount, m_totalFiles, &Job::totalFilesChanged);
    } else if (unit == QLatin1String("dirs")) {
        updateField(amount, m_totalDirectories, &Job::totalDirectoriesChanged);
    } else if (unit == QLatin1String("items")) {
        updateField(amount, m_totalItems, &Job::totalItemsChanged);
    }
    updateHasDetails();
}

void JobPrivate::setProcessedAmount(quint64 amount, const QString &unit)
{
    if (unit == QLatin1String("bytes")) {
        updateField(amount, m_processedBytes, &Job::processedBytesChanged);
    } else if (unit == QLatin1String("files")) {
        updateField(amount, m_processedFiles, &Job::processedFilesChanged);
    } else if (unit == QLatin1String("dirs")) {
        updateField(amount, m_processedDirectories, &Job::processedDirectoriesChanged);
    } else if (unit == QLatin1String("items")) {
        updateField(amount, m_processedItems, &Job::processedItemsChanged);
    }
    updateHasDetails();
}

void JobPrivate::setPercent(uint percent)
{
    const int percentage = static_cast<int>(percent);
    if (m_percentage != percentage) {
        m_percentage = percentage;
        Q_EMIT static_cast<Job *>(parent())->percentageChanged(percentage);
    }
}

void JobPrivate::setSpeed(quint64 bytesPerSecond)
{
    updateField(bytesPerSecond, m_speed, &Job::speedChanged);
    updateHasDetails();
}

// NOTE infoMessage isn't supposed to be the "Copying..." heading but e.g. a "Connecting to server..." status message
// JobViewV1/V2 got that wrong but JobView3 uses "title" and "infoMessage" correctly respectively.
void JobPrivate::setInfoMessage(const QString &infoMessage)
{
    updateField(infoMessage, m_summary, &Job::summaryChanged);
}

bool JobPrivate::setDescriptionField(uint number, const QString &name, const QString &value)
{
    bool dirty = false;
    if (number == 0) {
        dirty |= updateField(name, m_descriptionLabel1, &Job::descriptionLabel1Changed);
        dirty |= updateField(value, m_descriptionValue1, &Job::descriptionValue1Changed);
    } else if (number == 1) {
        dirty |= updateField(name, m_descriptionLabel2, &Job::descriptionLabel2Changed);
        dirty |= updateField(value, m_descriptionValue2, &Job::descriptionValue2Changed);
    }
    if (dirty) {
        Q_EMIT static_cast<Job *>(parent())->descriptionUrlChanged();
        updateHasDetails();
    }

    return false;
}

void JobPrivate::clearDescriptionField(uint number)
{
    setDescriptionField(number, QString(), QString());
}

void JobPrivate::setDestUrl(const QDBusVariant &urlVariant)
{
    QUrl destUrl = QUrl(urlVariant.variant().toUrl().adjusted(QUrl::StripTrailingSlash)); // urgh
    if (destUrl.scheme().isEmpty()) {
        qCInfo(NOTIFICATIONMANAGER) << "Job from" << m_applicationName << "set a destUrl" << destUrl
                                    << "without a scheme (assuming 'file'), this is an application bug!";
        destUrl.setScheme(QStringLiteral("file"));
    }

    updateField(destUrl, m_destUrl, &Job::destUrlChanged);
}

void JobPrivate::setError(uint errorCode)
{
    static_cast<Job *>(parent())->setError(errorCode);
}

// JobViewV3
void JobPrivate::terminate(uint errorCode, const QString &errorMessage, const QVariantMap &hints)
{
    Q_UNUSED(hints) // reserved for future extension

    Job *job = static_cast<Job *>(parent());
    job->setError(errorCode);
    job->setErrorText(errorMessage);

    // Request show just before changing state to stopped, so we're not discarded
    if (m_showConditions.testFlag(ShowCondition::OnTermination)) {
        requestShow();
    }

    job->setState(Notifications::JobStateStopped);
    finish();
}

void JobPrivate::update(const QVariantMap &properties)
{
    auto end = properties.end();

    auto it = properties.find(QStringLiteral("title"));
    if (it != end) {
        updateField(it->toString(), m_summary, &Job::summaryChanged);
    }

    it = properties.find(QStringLiteral("infoMessage"));
    if (it != end) {
        // InfoMessage is exposed via text()/BodyRole, not via public API, hence no public signal
        const QString infoMessage = it->toString();
        if (m_infoMessage != infoMessage) {
            m_infoMessage = it->toString();
            Q_EMIT infoMessageChanged();
        }
    }

    it = properties.find(QStringLiteral("percent"));
    if (it != end) {
        setPercent(it->toUInt());
    }

    it = properties.find(QStringLiteral("destUrl"));
    if (it != end) {
        const QUrl destUrl = QUrl(it->toUrl().adjusted(QUrl::StripTrailingSlash)); // urgh
        updateField(destUrl, m_destUrl, &Job::destUrlChanged);
    }

    it = properties.find(QStringLiteral("speed"));
    if (it != end) {
        setSpeed(it->value<qulonglong>());
    }

    updateFieldFromProperties(properties, QStringLiteral("processedFiles"), m_processedFiles, &Job::processedFilesChanged);
    updateFieldFromProperties(properties, QStringLiteral("processedBytes"), m_processedBytes, &Job::processedBytesChanged);
    updateFieldFromProperties(properties, QStringLiteral("processedDirectories"), m_processedDirectories, &Job::processedDirectoriesChanged);
    updateFieldFromProperties(properties, QStringLiteral("processedItems"), m_processedItems, &Job::processedItemsChanged);

    updateFieldFromProperties(properties, QStringLiteral("totalFiles"), m_totalFiles, &Job::totalFilesChanged);
    updateFieldFromProperties(properties, QStringLiteral("totalBytes"), m_totalBytes, &Job::totalBytesChanged);
    updateFieldFromProperties(properties, QStringLiteral("totalDirectories"), m_totalDirectories, &Job::totalDirectoriesChanged);
    updateFieldFromProperties(properties, QStringLiteral("totalItems"), m_totalItems, &Job::totalItemsChanged);

    updateFieldFromProperties(properties, QStringLiteral("descriptionLabel1"), m_descriptionLabel1, &Job::descriptionLabel1Changed);
    updateFieldFromProperties(properties, QStringLiteral("descriptionValue1"), m_descriptionValue1, &Job::descriptionValue1Changed);
    updateFieldFromProperties(properties, QStringLiteral("descriptionLabel2"), m_descriptionLabel2, &Job::descriptionLabel2Changed);
    updateFieldFromProperties(properties, QStringLiteral("descriptionValue2"), m_descriptionValue2, &Job::descriptionValue2Changed);

    it = properties.find(QStringLiteral("suspended"));
    if (it != end) {
        setSuspended(it->toBool());
    }

    updateHasDetails();

    if (!m_summary.isEmpty() && m_showConditions.testFlag(ShowCondition::OnSummary)) {
        requestShow();
    }
}
