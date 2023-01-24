/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "JobRunner.h"
#include "ActionLabel.h"
#include "Fc.h"
#include "KfiConstants.h"
#include "Misc.h"
#include "config-fontinst.h"

#include <KConfigGroup>
#include <KIO/FileCopyJob>
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KSharedConfig>

#include <QCheckBox>
#include <QCloseEvent>
#include <QDBusServiceWatcher>
#include <QDebug>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOption>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrlQuery>
#include <QVBoxLayout>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif
#include <X11/Xlib.h>

#include <fontconfig/fontconfig.h>
#include <kio/global.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

#define CFG_GROUP "Runner Dialog"
#define CFG_DONT_SHOW_FINISHED_MSG "DontShowFinishedMsg"

namespace KFI
{
Q_GLOBAL_STATIC(FontInstInterface, theInterface)

FontInstInterface *CJobRunner::dbus()
{
    return theInterface;
}

QString CJobRunner::folderName(bool sys)
{
    if (!theInterface) {
        return QString();
    }

    QDBusPendingReply<QString> reply = theInterface->folderName(sys);

    reply.waitForFinished();
    return reply.isError() ? QString() : reply.argumentAt<0>();
}

void CJobRunner::startDbusService()
{
    if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(OrgKdeFontinstInterface::staticInterfaceName())) {
        const QString fontinst = QStringLiteral(KFONTINST_LIB_EXEC_DIR "/fontinst");
        qDebug() << "Service " << OrgKdeFontinstInterface::staticInterfaceName() << " not registered, starting" << fontinst;
        QProcess::startDetached(fontinst, QStringList());
    }
}

static const int constDownloadFailed = -1;
static const int constInterfaceCheck = 5 * 1000;

static void decode(const QUrl &url, Misc::TFont &font, bool &system)
{
    font = FC::decode(url);
    QUrlQuery query(url);
    system = (query.hasQueryItem("sys") && query.queryItemValue("sys") == QLatin1String("true"));
}

QUrl CJobRunner::encode(const QString &family, quint32 style, bool system)
{
    QUrl url(FC::encode(family, style));

    QUrlQuery query(url);
    query.addQueryItem(QStringLiteral("sys"), system ? QStringLiteral("true") : QStringLiteral("false"));
    url.setQuery(query);
    return url;
}

enum EPages {
    PAGE_PROGRESS,
    PAGE_SKIP,
    PAGE_ERROR,
    PAGE_CANCEL,
    PAGE_COMPLETE,
};

enum Response {
    RESP_CONTINUE,
    RESP_AUTO,
    RESP_CANCEL,
};

static void addIcon(QGridLayout *layout, QFrame *page, const char *iconName, int iconSize)
{
    QLabel *icon = new QLabel(page);
    icon->setPixmap(QIcon::fromTheme(iconName).pixmap(iconSize));
    icon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    layout->addWidget(icon, 0, 0);
}

CJobRunner::CJobRunner(QWidget *parent, int xid)
    : QDialog(parent)
    , m_it(m_urls.end())
    , m_end(m_it)
    , m_autoSkip(false)
    , m_cancelClicked(false)
    , m_modified(false)
    , m_tempDir(nullptr)
{
    setModal(true);

    if (nullptr == parent && 0 != xid) {
        XSetTransientForHint(QX11Info::display(), winId(), xid);
    }

    m_buttonBox = new QDialogButtonBox;
    connect(m_buttonBox, &QDialogButtonBox::clicked, this, &CJobRunner::slotButtonClicked);

    m_skipButton = new QPushButton(i18n("Skip"));
    m_buttonBox->addButton(m_skipButton, QDialogButtonBox::ActionRole);
    m_skipButton->hide();
    m_autoSkipButton = new QPushButton(i18n("AutoSkip"));
    m_buttonBox->addButton(m_autoSkipButton, QDialogButtonBox::ActionRole);
    m_autoSkipButton->hide();

    m_stack = new QStackedWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    mainLayout->addWidget(m_stack);
    mainLayout->addWidget(m_buttonBox);

    QStyleOption option;
    option.initFrom(this);
    int iconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, &option, this);

    QFrame *page = new QFrame(m_stack);
    QGridLayout *layout = new QGridLayout(page);
    m_statusLabel = new QLabel(page);
    m_progress = new QProgressBar(page);
    //     m_statusLabel->setWordWrap(true);
    layout->addWidget(m_actionLabel = new CActionLabel(this), 0, 0, 2, 1);
    layout->addWidget(m_statusLabel, 0, 1);
    layout->addWidget(m_progress, 1, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 2, 0);
    m_stack->insertWidget(PAGE_PROGRESS, page);

    page = new QFrame(m_stack);
    layout = new QGridLayout(page);
    m_skipLabel = new QLabel(page);
    m_skipLabel->setWordWrap(true);
    addIcon(layout, page, "dialog-error", iconSize);
    layout->addWidget(m_skipLabel, 0, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 1, 0);
    m_stack->insertWidget(PAGE_SKIP, page);

    page = new QFrame(m_stack);
    layout = new QGridLayout(page);
    m_errorLabel = new QLabel(page);
    m_errorLabel->setWordWrap(true);
    addIcon(layout, page, "dialog-error", iconSize);
    layout->addWidget(m_errorLabel, 0, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 1, 0);
    m_stack->insertWidget(PAGE_ERROR, page);

    page = new QFrame(m_stack);
    layout = new QGridLayout(page);
    QLabel *cancelLabel = new QLabel(i18n("<h3>Cancel?</h3><p>Are you sure you wish to cancel?</p>"), page);
    cancelLabel->setWordWrap(true);
    addIcon(layout, page, "dialog-warning", iconSize);
    layout->addWidget(cancelLabel, 0, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 1, 0);
    m_stack->insertWidget(PAGE_CANCEL, page);

    if (KSharedConfig::openConfig(KFI_UI_CFG_FILE)->group(CFG_GROUP).readEntry(CFG_DONT_SHOW_FINISHED_MSG, false)) {
        m_dontShowFinishedMsg = nullptr;
    } else {
        page = new QFrame(m_stack);
        layout = new QGridLayout(page);
        QLabel *finishedLabel = new QLabel(i18n("<h3>Finished</h3>"
                                                "<p>Please note that any open applications will need to be "
                                                "restarted in order for any changes to be noticed.</p>"),
                                           page);
        finishedLabel->setWordWrap(true);
        addIcon(layout, page, "dialog-information", iconSize);
        layout->addWidget(finishedLabel, 0, 1);
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 1, 0);
        m_dontShowFinishedMsg = new QCheckBox(i18n("Do not show this message again"), page);
        m_dontShowFinishedMsg->setChecked(false);
        layout->addItem(new QSpacerItem(0, layout->spacing(), QSizePolicy::Fixed, QSizePolicy::Fixed), 2, 0);
        layout->addWidget(m_dontShowFinishedMsg, 3, 1);
        layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 4, 0);
        m_stack->insertWidget(PAGE_COMPLETE, page);
    }

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(QLatin1String(OrgKdeFontinstInterface::staticInterfaceName()),
                                                           QDBusConnection::sessionBus(),
                                                           QDBusServiceWatcher::WatchForOwnerChange,
                                                           this);

    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &CJobRunner::dbusServiceOwnerChanged);
    connect(dbus(), &OrgKdeFontinstInterface::status, this, &CJobRunner::dbusStatus);
    setMinimumSize(420, 160);
}

CJobRunner::~CJobRunner()
{
    delete m_tempDir;
}

void CJobRunner::getAssociatedUrls(const QUrl &url, QList<QUrl> &list, bool afmAndPfm, QWidget *widget)
{
    QString ext(url.path());
    int dotPos(ext.lastIndexOf('.'));
    bool check(false);

    if (-1 == dotPos) { // Hmm, no extension - check anyway...
        check = true;
    } else // Cool, got an extension - see if it is a Type1 font...
    {
        ext = ext.mid(dotPos + 1);
        check = 0 == ext.compare("pfa", Qt::CaseInsensitive) || 0 == ext.compare("pfb", Qt::CaseInsensitive);
    }

    if (check) {
        const char *afm[] = {"afm", "AFM", "Afm", nullptr}, *pfm[] = {"pfm", "PFM", "Pfm", nullptr};
        bool gotAfm(false), localFile(url.isLocalFile());
        int e;

        for (e = 0; afm[e]; ++e) {
            QUrl statUrl(url);
            statUrl.setPath(Misc::changeExt(url.path(), afm[e]));

            bool urlExists = false;
            if (localFile) {
                urlExists = Misc::fExists(statUrl.toLocalFile());
            } else {
                auto job = KIO::stat(statUrl);
                KJobWidgets::setWindow(job, widget);
                job->exec();
                urlExists = !job->error();
            }

            if (urlExists) {
                list.append(statUrl);
                gotAfm = true;
                break;
            }
        }

        if (afmAndPfm || !gotAfm) {
            for (e = 0; pfm[e]; ++e) {
                QUrl statUrl(url);
                statUrl.setPath(Misc::changeExt(url.path(), pfm[e]));

                bool urlExists = false;
                if (localFile) {
                    urlExists = Misc::fExists(statUrl.toLocalFile());
                } else {
                    auto job = KIO::stat(statUrl);
                    KJobWidgets::setWindow(job, widget);
                    job->exec();
                    urlExists = !job->error();
                }

                if (urlExists) {
                    list.append(statUrl);
                    break;
                }
            }
        }
    }
}

static void addEnableActions(CJobRunner::ItemList &urls)
{
    CJobRunner::ItemList modified;
    CJobRunner::ItemList::ConstIterator it(urls.constBegin()), end(urls.constEnd());

    for (; it != end; ++it) {
        if ((*it).isDisabled) {
            CJobRunner::Item item(*it);
            item.fileName = QLatin1String("--");
            modified.append(item);
        }
        modified.append(*it);
    }

    urls = modified;
}

int CJobRunner::exec(ECommand cmd, const ItemList &urls, bool destIsSystem)
{
    m_autoSkip = m_cancelClicked = m_modified = false;

    switch (cmd) {
    case CMD_INSTALL:
        setWindowTitle(i18n("Installing"));
        break;
    case CMD_DELETE:
        setWindowTitle(i18n("Uninstalling"));
        break;
    case CMD_ENABLE:
        setWindowTitle(i18n("Enabling"));
        break;
    case CMD_MOVE:
        setWindowTitle(i18n("Moving"));
        break;
    case CMD_UPDATE:
        setWindowTitle(i18n("Updating"));
        m_modified = true;
        break;
    case CMD_REMOVE_FILE:
        setWindowTitle(i18n("Removing"));
        break;
    default:
    case CMD_DISABLE:
        setWindowTitle(i18n("Disabling"));
    }

    m_destIsSystem = destIsSystem;
    m_urls = urls;
    if (CMD_INSTALL == cmd) {
        std::sort(m_urls.begin(), m_urls.end()); // Sort list of fonts so that we have type1 fonts followed by their metrics...
    } else if (CMD_MOVE == cmd) {
        addEnableActions(m_urls);
    }
    m_it = m_urls.constBegin();
    m_end = m_urls.constEnd();
    m_prev = m_end;
    m_progress->setValue(0);
    m_progress->setRange(0, m_urls.count() + 1);
    m_progress->show();
    m_cmd = cmd;
    m_currentFile = QString();
    m_statusLabel->setText(QString());
    setPage(PAGE_PROGRESS);
    QTimer::singleShot(0, this, &CJobRunner::doNext);
    QTimer::singleShot(constInterfaceCheck, this, &CJobRunner::checkInterface);
    m_actionLabel->startAnimation();
    int rv = QDialog::exec();
    if (m_tempDir) {
        delete m_tempDir;
        m_tempDir = nullptr;
    }
    return rv;
}

void CJobRunner::doNext()
{
    if (m_it == m_end /* || CMD_UPDATE==m_cmd*/) {
        if (m_modified) {
            // Force reconfig if command was already set to update...
            dbus()->reconfigure(getpid(), CMD_UPDATE == m_cmd);
            m_cmd = CMD_UPDATE;
            m_statusLabel->setText(i18n("Updating font configuration. Please wait…"));
            m_progress->setValue(m_progress->maximum());
            Q_EMIT configuring();
        } else {
            m_actionLabel->stopAnimation();
            if (PAGE_ERROR != m_stack->currentIndex()) {
                reject();
            }
        }
    } else {
        Misc::TFont font;
        bool system;

        switch (m_cmd) {
        case CMD_INSTALL: {
            m_currentFile = fileName((*m_it));

            if (m_currentFile.isEmpty()) { // Failed to download...
                dbusStatus(getpid(), constDownloadFailed);
            } else {
                // Create AFM if this is a PFM, and the previous was not the AFM for this font...
                bool createAfm =
                    Item::TYPE1_PFM == (*m_it).type && (m_prev == m_end || (*m_it).fileName != (*m_prev).fileName || Item::TYPE1_AFM != (*m_prev).type);

                dbus()->install(m_currentFile, createAfm, m_destIsSystem, getpid(), false);
            }
            break;
        }
        case CMD_DELETE:
            decode(*m_it, font, system);
            dbus()->uninstall(font.family, font.styleInfo, system, getpid(), false);
            break;
        case CMD_ENABLE:
            decode(*m_it, font, system);
            dbus()->enable(font.family, font.styleInfo, system, getpid(), false);
            break;
        case CMD_DISABLE:
            decode(*m_it, font, system);
            dbus()->disable(font.family, font.styleInfo, system, getpid(), false);
            break;
        case CMD_MOVE:
            decode(*m_it, font, system);
            // To 'Move' a disabled font, we first need to enable it. To accomplish this, JobRunner creates a 'fake' entry
            // with the filename "--"
            if ((*m_it).fileName == QLatin1String("--")) {
                setWindowTitle(i18n("Enabling"));
                dbus()->enable(font.family, font.styleInfo, system, getpid(), false);
            } else {
                if (m_prev != m_end && (*m_prev).fileName == QLatin1String("--")) {
                    setWindowTitle(i18n("Moving"));
                }
                dbus()->move(font.family, font.styleInfo, m_destIsSystem, getpid(), false);
            }
            break;
        case CMD_REMOVE_FILE:
            decode(*m_it, font, system);
            dbus()->removeFile(font.family, font.styleInfo, (*m_it).fileName, system, getpid(), false);
            break;
        default:
            break;
        }
        m_statusLabel->setText(CMD_INSTALL == m_cmd ? (*m_it).url() : FC::createName(FC::decode(*m_it)));
        m_progress->setValue(m_progress->value() + 1);

        // Keep copy of this iterator - so that can check whether AFM should be created.
        m_prev = m_it;
    }
}

void CJobRunner::checkInterface()
{
    if (m_it == m_urls.constBegin() && !FontInst::isStarted(dbus())) {
        setPage(PAGE_ERROR, i18n("Unable to start backend."));
        m_actionLabel->stopAnimation();
        m_it = m_end;
    }
}

void CJobRunner::dbusServiceOwnerChanged(const QString &name, const QString &from, const QString &to)
{
    if (to.isEmpty() && !from.isEmpty() && name == QLatin1String(OrgKdeFontinstInterface::staticInterfaceName()) && m_it != m_end) {
        setPage(PAGE_ERROR, i18n("Backend died, but has been restarted. Please try again."));
        m_actionLabel->stopAnimation();
        m_it = m_end;
    }
}

void CJobRunner::dbusStatus(int pid, int status)
{
    if (pid != getpid()) {
        return;
    }

    if (CMD_UPDATE == m_cmd) {
        setPage(PAGE_COMPLETE);
        return;
    }

    m_lastDBusStatus = status;

    if (m_cancelClicked) {
        m_actionLabel->stopAnimation();
        setPage(PAGE_CANCEL);
        return;
        /*
        if(RESP_CANCEL==m_response)
            m_it=m_end;
        m_cancelClicked=false;
        setPage(PAGE_PROGRESS);
        m_actionLabel->startAnimation();
        */
    }

    // m_it will equal m_end if user decided to cancel the current op
    if (m_it == m_end) {
        doNext();
    } else if (0 == status) {
        m_modified = true;
        ++m_it;
        doNext();
    } else {
        bool cont(m_autoSkip && m_urls.count() > 1);
        QString currentName((*m_it).fileName);

        if (!cont) {
            m_actionLabel->stopAnimation();

            if (FontInst::STATUS_SERVICE_DIED == status) {
                setPage(PAGE_ERROR, errorString(status));
                m_it = m_end;
            } else {
                ItemList::ConstIterator lastPartOfCurrent(m_it), next(m_it == m_end ? m_end : m_it + 1);

                // If we're installing a Type1 font, and its already installed - then we need to skip past AFM/PFM
                if (next != m_end && Item::TYPE1_FONT == (*m_it).type && (*next).fileName == currentName
                    && (Item::TYPE1_AFM == (*next).type || Item::TYPE1_PFM == (*next).type)) {
                    next++;
                    if (next != m_end && (*next).fileName == currentName && (Item::TYPE1_AFM == (*next).type || Item::TYPE1_PFM == (*next).type)) {
                        next++;
                    }
                }
                if (1 == m_urls.count() || next == m_end) {
                    setPage(PAGE_ERROR, errorString(status));
                } else {
                    setPage(PAGE_SKIP, errorString(status));
                    return;
                }
            }
        }

        contineuToNext(cont);
    }
}

void CJobRunner::contineuToNext(bool cont)
{
    m_actionLabel->startAnimation();
    if (cont) {
        if (CMD_INSTALL == m_cmd && Item::TYPE1_FONT == (*m_it).type) // Did we error on a pfa/pfb? if so, exclude the afm/pfm...
        {
            QString currentName((*m_it).fileName);

            ++m_it;

            // Skip afm/pfm
            if (m_it != m_end && (*m_it).fileName == currentName && (Item::TYPE1_AFM == (*m_it).type || Item::TYPE1_PFM == (*m_it).type)) {
                ++m_it;
            }
            // Skip pfm/afm
            if (m_it != m_end && (*m_it).fileName == currentName && (Item::TYPE1_AFM == (*m_it).type || Item::TYPE1_PFM == (*m_it).type)) {
                ++m_it;
            }
        } else {
            ++m_it;
        }
    } else {
        m_it = m_end = m_urls.constEnd();
    }
    doNext();
}

void CJobRunner::slotButtonClicked(QAbstractButton *button)
{
    switch (m_stack->currentIndex()) {
    case PAGE_PROGRESS:
        if (m_it != m_end) {
            m_cancelClicked = true;
        }
        break;
    case PAGE_SKIP:
        setPage(PAGE_PROGRESS);
        if (button == m_skipButton) {
            contineuToNext(true);
        } else if (button == m_autoSkipButton) {
            m_autoSkip = true;
            contineuToNext(true);
        } else {
            contineuToNext(false);
        }

        break;
    case PAGE_CANCEL:
        if (button == m_buttonBox->button(QDialogButtonBox::Yes)) {
            m_it = m_end;
        }
        m_cancelClicked = false;
        setPage(PAGE_PROGRESS);
        m_actionLabel->startAnimation();
        // Now continue...
        dbusStatus(getpid(), m_lastDBusStatus);
        break;
    case PAGE_COMPLETE: {
        if (m_dontShowFinishedMsg) {
            KConfigGroup grp(KSharedConfig::openConfig(KFI_UI_CFG_FILE)->group(CFG_GROUP));
            grp.writeEntry(CFG_DONT_SHOW_FINISHED_MSG, m_dontShowFinishedMsg->isChecked());
        }
        QDialog::accept();
        break;
    }
    case PAGE_ERROR:
        QDialog::accept();
        break;
    default:
        break;
    }
}

void CJobRunner::closeEvent(QCloseEvent *e)
{
    if (PAGE_COMPLETE != m_stack->currentIndex()) {
        e->ignore();
        slotButtonClicked(PAGE_CANCEL == m_stack->currentIndex() ? m_buttonBox->button(QDialogButtonBox::No) : m_buttonBox->button(QDialogButtonBox::Cancel));
    }
}

void CJobRunner::setPage(int page, const QString &msg)
{
    m_stack->setCurrentIndex(page);

    switch (page) {
    case PAGE_PROGRESS:
        m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
        m_skipButton->hide();
        m_autoSkipButton->hide();
        break;
    case PAGE_SKIP:
        m_skipLabel->setText(i18n("<h3>Error</h3>") + QLatin1String("<p>") + msg + QLatin1String("</p>"));
        m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
        m_skipButton->show();
        m_autoSkipButton->show();
        break;
    case PAGE_ERROR:
        m_errorLabel->setText(i18n("<h3>Error</h3>") + QLatin1String("<p>") + msg + QLatin1String("</p>"));
        m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
        m_skipButton->hide();
        m_autoSkipButton->hide();
        break;
    case PAGE_CANCEL:
        m_buttonBox->setStandardButtons(QDialogButtonBox::Yes | QDialogButtonBox::No);
        m_skipButton->hide();
        m_autoSkipButton->hide();
        break;
    case PAGE_COMPLETE:
        if (!m_dontShowFinishedMsg || m_dontShowFinishedMsg->isChecked()) {
            QDialog::accept();
        } else {
            m_buttonBox->setStandardButtons(QDialogButtonBox::Close);
            m_skipButton->hide();
            m_autoSkipButton->hide();
        }
        break;
    }
}

QString CJobRunner::fileName(const QUrl &url)
{
    if (url.isLocalFile()) {
        return url.toLocalFile();
    } else {
        auto job = KIO::mostLocalUrl(url);
        job->exec();
        QUrl local = job->mostLocalUrl();

        if (local.isLocalFile()) {
            return local.toLocalFile(); // Yipee! no need to download!!
        } else {
            // Need to do actual download...
            if (!m_tempDir) {
                m_tempDir = new QTemporaryDir(QDir::tempPath() + "/fontinst");
                m_tempDir->setAutoRemove(true);
            }

            QString tempName(m_tempDir->filePath(Misc::getFile(url.path())));
            auto job = KIO::file_copy(url, QUrl::fromLocalFile(tempName), -1, KIO::Overwrite);
            if (job->exec()) {
                return tempName;
            } else {
                return QString();
            }
        }
    }
}

QString CJobRunner::errorString(int value) const
{
    Misc::TFont font(FC::decode(*m_it));
    QString urlStr;

    if (CMD_REMOVE_FILE == m_cmd) {
        urlStr = (*m_it).fileName;
    } else if (font.family.isEmpty()) {
        urlStr = (*m_it).url();
    } else {
        urlStr = FC::createName(font.family, font.styleInfo);
    }

    switch (value) {
    case constDownloadFailed:
        return i18n("Failed to download <i>%1</i>", urlStr);
    case FontInst::STATUS_SERVICE_DIED:
        return i18n("System backend died. Please try again.<br><i>%1</i>", urlStr);
    case FontInst::STATUS_BITMAPS_DISABLED:
        return i18n("<i>%1</i> is a bitmap font, and these have been disabled on your system.", urlStr);
    case FontInst::STATUS_ALREADY_INSTALLED:
        return i18n("<i>%1</i> contains the font <b>%2</b>, which is already installed on your system.", urlStr, FC::getName(m_currentFile));
    case FontInst::STATUS_NOT_FONT_FILE:
        return i18n("<i>%1</i> is not a font.", urlStr);
    case FontInst::STATUS_PARTIAL_DELETE:
        return i18n("Could not remove all files associated with <i>%1</i>", urlStr);
    case FontInst::STATUS_NO_SYS_CONNECTION:
        return i18n("Failed to start the system daemon.<br><i>%1</i>", urlStr);
    case KIO::ERR_FILE_ALREADY_EXIST: {
        QString name(Misc::modifyName(Misc::getFile((*m_it).fileName))), destFolder(Misc::getDestFolder(folderName(m_destIsSystem), name));
        return i18n("<i>%1</i> already exists.", destFolder + name);
    }
    case KIO::ERR_DOES_NOT_EXIST:
        return i18n("<i>%1</i> does not exist.", urlStr);
    case KIO::ERR_WRITE_ACCESS_DENIED:
        return i18n("Permission denied.<br><i>%1</i>", urlStr);
    case KIO::ERR_UNSUPPORTED_ACTION:
        return i18n("Unsupported action.<br><i>%1</i>", urlStr);
    case KIO::ERR_CANNOT_AUTHENTICATE:
        return i18n("Authentication failed.<br><i>%1</i>", urlStr);
    default:
        return i18n("Unexpected error while processing: <i>%1</i>", urlStr);
    }
}

CJobRunner::Item::Item(const QUrl &u, const QString &n, bool dis)
    : QUrl(u)
    , name(n)
    , fileName(Misc::getFile(u.path()))
    , isDisabled(dis)
{
    type = Misc::checkExt(fileName, "pfa") || Misc::checkExt(fileName, "pfb") ? TYPE1_FONT
        : Misc::checkExt(fileName, "afm")                                     ? TYPE1_AFM
        : Misc::checkExt(fileName, "pfm")                                     ? TYPE1_PFM
                                                                              : OTHER_FONT;

    if (OTHER_FONT != type) {
        int pos(fileName.lastIndexOf('.'));

        if (-1 != pos) {
            fileName.truncate(pos);
        }
    }
}

CJobRunner::Item::Item(const QString &file, const QString &family, quint32 style, bool system)
    : QUrl(CJobRunner::encode(family, style, system))
    , fileName(file)
    , type(OTHER_FONT)
{
}

bool CJobRunner::Item::operator<(const Item &o) const
{
    int nameComp(fileName.compare(o.fileName));

    return nameComp < 0 || (0 == nameComp && type < o.type);
}

}
