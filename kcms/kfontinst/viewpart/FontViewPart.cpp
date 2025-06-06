/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "FontViewPart.h"
#include "Fc.h"
#include "FcEngine.h"
#include "FontInst.h"
#include "FontInstInterface.h"
#include "KfiConstants.h"
#include "PreviewSelectAction.h"
#include "config-workspace.h"
#include <KAboutData>
#include <KActionCollection>
#include <KIO/Job>
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KMessageBox>
#include <QBoxLayout>
#include <QFile>
#include <QGroupBox>
#include <QGuiApplication>
#include <QIcon>
#include <QInputDialog>
#include <QLabel>
#include <QMimeDatabase>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
// #include <KFileMetaInfo>
#include "config-fontinst.h"
#include <KPluginFactory>
#include <KStandardActions>
#include <KWaylandExtras>
#include <KWindowSystem>
#include <KZip>
#include <QTemporaryDir>

using namespace Qt::StringLiterals;

// Enable the following to allow printing of non-installed fonts. Does not seem to work :-(
// #define KFI_PRINT_APP_FONTS

namespace KFI
{
static QString getFamily(const QString &font)
{
    int commaPos = font.lastIndexOf(u',');
    return -1 == commaPos ? font : font.left(commaPos);
}

K_PLUGIN_CLASS_WITH_JSON(CFontViewPart, "kfontviewpart.json")

CFontViewPart::CFontViewPart(QWidget *parentWidget, QObject *parent, const KPluginMetaData &data)
    : KParts::ReadOnlyPart(parent, data)
    , m_config(KSharedConfig::openConfig())
    , m_proc(nullptr)
    , m_tempDir(nullptr)
    , m_interface(new FontInstInterface())
    , m_opening(false)
{
    // create browser extension (for printing when embedded into browser)
    m_extension = new BrowserExtension(this);

    m_frame = new QFrame(parentWidget);

    QFrame *previewFrame = new QFrame(m_frame);
    QWidget *controls = new QWidget(m_frame);

    m_faceWidget = new QWidget(controls);

    QBoxLayout *mainLayout = new QBoxLayout(QBoxLayout::TopToBottom, m_frame);

    QBoxLayout *previewLayout = new QBoxLayout(QBoxLayout::LeftToRight, previewFrame), *controlsLayout = new QBoxLayout(QBoxLayout::LeftToRight, controls),
               *faceLayout = new QBoxLayout(QBoxLayout::LeftToRight, m_faceWidget);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);
    faceLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(0);

    m_frame->setFrameShape(QFrame::NoFrame);
    m_frame->setFocusPolicy(Qt::ClickFocus);
    previewFrame->setFrameShape(QFrame::StyledPanel);
    previewFrame->setFrameShadow(QFrame::Sunken);

    m_preview = new CFontPreview(previewFrame);
    m_preview->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_faceLabel = new QLabel(i18n("Show Face:"), m_faceWidget);
    m_faceSelector = new QSpinBox(m_faceWidget);
    m_faceSelector->setValue(1);
    m_installButton = new QPushButton(i18n("Install…"), controls);
    m_installButton->setEnabled(false);
    previewLayout->addWidget(m_preview);
    faceLayout->addWidget(m_faceLabel);
    faceLayout->addWidget(m_faceSelector);
    faceLayout->addItem(new QSpacerItem(faceLayout->spacing(), 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
    m_faceWidget->hide();

    m_preview->engine()->readConfig(*m_config);

    controlsLayout->addWidget(m_faceWidget);
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(m_installButton);
    mainLayout->addWidget(previewFrame);
    mainLayout->addWidget(controls);
    connect(m_preview, &CFontPreview::status, this, &CFontViewPart::previewStatus);
    connect(m_installButton, &QAbstractButton::clicked, this, &CFontViewPart::install);
    connect(m_faceSelector, SIGNAL(valueChanged(int)), SLOT(showFace(int)));

    m_changeTextAction = actionCollection()->addAction(u"changeText"_s);
    m_changeTextAction->setIcon(QIcon::fromTheme(u"edit-rename"_s));
    m_changeTextAction->setText(i18n("Change Text…"));
    connect(m_changeTextAction, &QAction::triggered, this, &CFontViewPart::changeText);

    CPreviewSelectAction *displayTypeAction = new CPreviewSelectAction(this, CPreviewSelectAction::BlocksAndScripts);
    actionCollection()->addAction(u"displayType"_s, displayTypeAction);
    connect(displayTypeAction, &CPreviewSelectAction::range, this, &CFontViewPart::displayType);

    QAction *zoomIn = actionCollection()->addAction(KStandardActions::ZoomIn, m_preview, &CFontPreview::zoomIn),
            *zoomOut = actionCollection()->addAction(KStandardActions::ZoomOut, m_preview, &CFontPreview::zoomOut);

    connect(m_preview, &CFontPreview::atMax, zoomIn, &QAction::setDisabled);
    connect(m_preview, &CFontPreview::atMin, zoomOut, &QAction::setDisabled);

    setXMLFile(u"kfontviewpart.rc"_s);
    setWidget(m_frame);
    m_extension->enablePrint(false);

    FontInst::registerTypes();
    connect(m_interface, &OrgKdeFontinstInterface::status, this, &CFontViewPart::dbusStatus);
    connect(m_interface, &OrgKdeFontinstInterface::fontStat, this, &CFontViewPart::fontStat);
}

CFontViewPart::~CFontViewPart()
{
    delete m_tempDir;
    m_tempDir = nullptr;
    delete m_interface;
    m_interface = nullptr;
}

static inline QUrl mostLocalUrl(const QUrl &url, QWidget *widget)
{
    auto job = KIO::mostLocalUrl(url);
    KJobWidgets::setWindow(job, widget);
    job->exec();
    return job->mostLocalUrl();
}

bool CFontViewPart::openUrl(const QUrl &url)
{
    if (!url.isValid() || !closeUrl()) {
        return false;
    }

    m_fontDetails = FC::decode(url);
    if (!m_fontDetails.family.isEmpty() || QLatin1String(KFI_KIO_FONTS_PROTOCOL) == url.scheme() || mostLocalUrl(url, m_frame).isLocalFile()) {
        setUrl(url);
        Q_EMIT started(nullptr);
        setLocalFilePath(this->url().path());
        bool ret = openFile();
        if (ret) {
            Q_EMIT completed();
        }
        return ret;
    } else {
        return ReadOnlyPart::openUrl(url);
    }
}

bool CFontViewPart::openFile()
{
    // NOTE: Can't do the real open here, as we don't seem to be able to use KIO::NetAccess functions
    // during initial start-up. Bug report 111535 indicates that calling "konqueror <font>" crashes.
    m_installButton->setEnabled(false);
    QTimer::singleShot(0, this, &CFontViewPart::timeout);
    return true;
}

static inline bool statUrl(const QUrl &url, KIO::UDSEntry *udsEntry)
{
    auto job = KIO::stat(url);
    job->exec();
    if (job->error()) {
        return false;
    }
    *udsEntry = job->statResult();
    return true;
}

void CFontViewPart::timeout()
{
    if (!m_installButton) {
        return;
    }

    bool isFonts(QLatin1String(KFI_KIO_FONTS_PROTOCOL) == url().scheme()), showFs(false), package(false);
    int fileIndex(-1);
    QString fontFile;

    delete m_tempDir;
    m_tempDir = nullptr;

    m_opening = true;

    if (!m_fontDetails.family.isEmpty()) {
        Q_EMIT setWindowCaption(FC::createName(m_fontDetails.family, m_fontDetails.styleInfo));
        fontFile = FC::getFile(url());
        fileIndex = FC::getIndex(url());
    } else if (isFonts) {
        KIO::UDSEntry udsEntry;
        bool found = statUrl(url(), &udsEntry);

        if (!found) {
            // Check if url is "fonts:/<font> if so try fonts:/System/<font>, then fonts:/Personal<font>
            QStringList pathList(url().adjusted(QUrl::StripTrailingSlash).path().split(QLatin1Char('/'), Qt::SkipEmptyParts));

            if (pathList.count() == 1) {
                found = statUrl(QUrl(QString("fonts:/"_L1 + KFI_KIO_FONTS_SYS.toString() + QLatin1Char('/') + pathList[0])), &udsEntry);
                if (!found) {
                    found = statUrl(QUrl(QString("fonts:/"_L1 + KFI_KIO_FONTS_USER.toString() + QLatin1Char('/') + pathList[0])), &udsEntry);
                }
            }
        }

        if (found) {
            if (udsEntry.numberValue(KIO::UDSEntry::UDS_HIDDEN, 0)) {
                fontFile = udsEntry.stringValue(UDS_EXTRA_FILE_NAME);
                fileIndex = udsEntry.numberValue(UDS_EXTRA_FILE_FACE, 0);
            }
            m_fontDetails.family = getFamily(udsEntry.stringValue(KIO::UDSEntry::UDS_NAME));
            m_fontDetails.styleInfo = udsEntry.numberValue(UDS_EXTRA_FC_STYLE);
            Q_EMIT setWindowCaption(udsEntry.stringValue(KIO::UDSEntry::UDS_NAME));
        } else {
            previewStatus(false);
            return;
        }
    } else {
        QString path(localFilePath());

        // Is this a application/vnd.kde.fontspackage file? If so, extract 1 scalable font...
        if ((package = Misc::isPackage(path))) {
            KZip zip(path);

            if (zip.open(QIODevice::ReadOnly)) {
                const KArchiveDirectory *zipDir = zip.directory();

                if (zipDir) {
                    QStringList fonts(zipDir->entries());

                    if (!fonts.isEmpty()) {
                        QStringList::ConstIterator it(fonts.begin()), end(fonts.end());

                        for (; it != end; ++it) {
                            const KArchiveEntry *entry = zipDir->entry(*it);

                            if (entry && entry->isFile()) {
                                delete m_tempDir;
                                m_tempDir = new QTemporaryDir(QDir::tempPath() + QDir::separator() + KFI_TMP_DIR_PREFIX);
                                m_tempDir->setAutoRemove(true);

                                ((KArchiveFile *)entry)->copyTo(m_tempDir->path());

                                QMimeDatabase db;
                                QString mime(db.mimeTypeForFile(m_tempDir->filePath(entry->name())).name());

                                if (mime == u"font/ttf" || mime == u"font/otf" || mime == u"application/x-font-ttf" || mime == u"application/x-font-otf"
                                    || mime == u"application/x-font-type1") {
                                    fontFile = m_tempDir->filePath(entry->name());
                                    break;
                                } else {
                                    ::unlink(QFile::encodeName(m_tempDir->filePath(entry->name())).data());
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    m_installButton->setEnabled(false);

    if (m_fontDetails.family.isEmpty()) {
        Q_EMIT setWindowCaption(url().toDisplayString());
    } else {
        FcInitReinitialize();
    }

    m_preview->showFont(!package && m_fontDetails.family.isEmpty() ? localFilePath()
                            : fontFile.isEmpty()                   ? m_fontDetails.family
                                                                   : fontFile,
                        m_fontDetails.styleInfo,
                        fileIndex);

    if (!isFonts && m_preview->engine()->getNumIndexes() > 1) {
        showFs = true;
        m_faceSelector->setRange(1, m_preview->engine()->getNumIndexes());
        m_faceSelector->setSingleStep(1);
        m_faceSelector->blockSignals(true);
        m_faceSelector->setValue(1);
        m_faceSelector->blockSignals(false);
    }

    m_faceWidget->setVisible(showFs);
}

void CFontViewPart::previewStatus(bool st)
{
    if (m_opening) {
        bool printable(false);

        if (st) {
            checkInstallable();
            if (Misc::app(KFI_PRINTER).isEmpty()) {
                printable = false;
            }
            if (QLatin1String(KFI_KIO_FONTS_PROTOCOL) == url().scheme()) {
                printable = !Misc::isHidden(url());
            } else if (!FC::decode(url()).family.isEmpty()) {
                printable = !Misc::isHidden(FC::getFile(url()));
            }
#ifdef KFI_PRINT_APP_FONTS
            else {
                // TODO: Make this work! Plus, printing of disabled TTF/OTF's should also be possible!
                KMimeType::Ptr mime = KMimeType::findByUrl(QUrl::fromLocalFile(localFilePath()), 0, false, true);

                printable = mime->is("application/x-font-ttf") || mime->is("application/x-font-otf");
            }
#endif
        }
        m_extension->enablePrint(st && printable);
        m_opening = false;
    }
    m_changeTextAction->setEnabled(st);

    if (!st) {
        KMessageBox::error(m_frame, i18n("Could not read font."));
    }
}

void CFontViewPart::install()
{
    if (!m_proc || QProcess::NotRunning == m_proc->state()) {
        if (!m_proc) {
            m_proc = new QProcess(this);
        } else {
            m_proc->kill();
        }

        auto runFontInst = [this](const QString &windowHandle) {
            QString title = QGuiApplication::applicationDisplayName();
            if (title.isEmpty()) {
                title = QCoreApplication::applicationName();
            }

            QStringList args;
            args << u"--embed"_s << windowHandle << u"--qwindowtitle"_s << title << u"--qwindowicon"_s << u"kfontview"_s << url().toDisplayString();

            connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(installlStatus()));
            m_proc->start(Misc::app(KFI_INSTALLER), args);
            m_installButton->setEnabled(false);
        };

        if (KWindowSystem::isPlatformWayland()) {
            connect(
                KWaylandExtras::self(),
                &KWaylandExtras::windowExported,
                this,
                [runFontInst](QWindow * /*window*/, const QString &handle) {
                    runFontInst(handle);
                },
                Qt::SingleShotConnection);
            KWaylandExtras::exportWindow(m_frame->window()->windowHandle());
        } else {
            runFontInst(QStringLiteral("0x%1").arg((unsigned int)m_frame->window()->winId(), 0, 16));
        }
    }
}

void CFontViewPart::installlStatus()
{
    checkInstallable();
}

void CFontViewPart::dbusStatus(int pid, int status)
{
    if (pid == getpid() && FontInst::STATUS_OK != status) {
        m_installButton->setEnabled(false);
    }
}

void CFontViewPart::fontStat(int pid, const KFI::Family &font)
{
    if (pid == getpid()) {
        m_installButton->setEnabled(!Misc::app(KFI_INSTALLER).isEmpty() && font.styles().isEmpty());
    }
}

void CFontViewPart::changeText()
{
    bool status;
    QString oldStr(m_preview->engine()->getPreviewString()),
        newStr(QInputDialog::getText(m_frame, i18n("Preview String"), i18n("Please enter new string:"), QLineEdit::Normal, oldStr, &status));

    if (status && newStr != oldStr) {
        m_preview->engine()->setPreviewString(newStr);
        m_preview->engine()->writeConfig(*m_config);
        m_preview->showFont();
    }
}

void CFontViewPart::print()
{
    QStringList args;

    QString title = QGuiApplication::applicationDisplayName();
    if (title.isEmpty()) {
        title = QCoreApplication::applicationName();
    }

    if (!m_fontDetails.family.isEmpty()) {
        args << u"--embed"_s << QStringLiteral("0x%1").arg((unsigned int)m_frame->window()->winId(), 0, 16) << u"--qwindowtitle"_s << title
             << u"--qwindowicon"_s << u"kfontview"_s << u"--size"_s << u"0"_s << u"--pfont"_s
             << QString(m_fontDetails.family + u',' + QString().setNum(m_fontDetails.styleInfo));
    }
#ifdef KFI_PRINT_APP_FONTS
    else
        args << "--embed" << QStringLiteral("0x%1").arg((unsigned int)m_frame->window()->winId(), 0, 16) << "--qwindowtitle" << title << "--qwindowicon"
             << "kfontview"
             << "--size "
             << "0" << localFilePath() << QString().setNum(KFI_NO_STYLE_INFO);
#endif

    if (!args.isEmpty()) {
        QProcess::startDetached(Misc::app(KFI_PRINTER), args);
    }
}

void CFontViewPart::displayType(const QList<CFcEngine::TRange> &range)
{
    m_preview->setUnicodeRange(range);
    m_changeTextAction->setEnabled(0 == range.count());
}

void CFontViewPart::showFace(int face)
{
    m_preview->showFace(face - 1);
}

void CFontViewPart::checkInstallable()
{
    if (m_fontDetails.family.isEmpty()) {
        if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(QLatin1String(OrgKdeFontinstInterface::staticInterfaceName()))) {
            QProcess::startDetached(QLatin1String(KFONTINST_LIB_EXEC_DIR "/fontinst"), QStringList());
        }
        m_installButton->setEnabled(false);
        m_interface->statFont(m_preview->engine()->descriptiveName(), FontInst::SYS_MASK | FontInst::USR_MASK, getpid());
    }
}

BrowserExtension::BrowserExtension(CFontViewPart *parent)
    : KParts::NavigationExtension(parent)
{
    setURLDropHandlingEnabled(true);
}

void BrowserExtension::enablePrint(bool enable)
{
    if (enable != isActionEnabled("print") && (!enable || !Misc::app(KFI_PRINTER).isEmpty())) {
        Q_EMIT enableAction("print", enable);
    }
}

void BrowserExtension::print()
{
    if (!Misc::app(KFI_PRINTER).isEmpty()) {
        static_cast<CFontViewPart *>(parent())->print();
    }
}
}

#include "FontViewPart.moc"

#include "moc_FontViewPart.cpp"
