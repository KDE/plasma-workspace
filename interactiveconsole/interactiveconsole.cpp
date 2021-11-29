/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "interactiveconsole.h"

#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextBrowser>
#include <QToolButton>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KMessageBox>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KShell>
#include <KStandardAction>
#include <KTextEdit>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KToolBar>
#include <KWindowSystem>
#include <klocalizedstring.h>

#include <KPackage/Package>
#include <KPackage/PackageLoader>

// TODO:
// interactive help?
static const QString s_autosaveFileName(QStringLiteral("interactiveconsoleautosave.js"));
static const QString s_kwinService = QStringLiteral("org.kde.KWin");
static const QString s_plasmaShellService = QStringLiteral("org.kde.plasmashell");

InteractiveConsole::InteractiveConsole(ConsoleMode mode, QWidget *parent)
    : QDialog(parent)
    , m_splitter(new QSplitter(Qt::Vertical, this))
    , m_editorPart(nullptr)
    , m_editor(nullptr)
    , m_output(nullptr)
    , m_loadAction(KStandardAction::open(this, SLOT(openScriptFile()), this))
    , m_saveAction(KStandardAction::saveAs(this, SLOT(saveScript()), this))
    , m_clearAction(KStandardAction::clear(this, SLOT(clearEditor()), this))
    , m_executeAction(new QAction(QIcon::fromTheme(QStringLiteral("system-run")), i18n("&Execute"), this))
    , m_plasmaAction(new QAction(QIcon::fromTheme(QStringLiteral("plasma")), i18nc("Toolbar Button to switch to Plasma Scripting Mode", "Plasma"), this))
    , m_kwinAction(new QAction(QIcon::fromTheme(QStringLiteral("kwin")), i18nc("Toolbar Button to switch to KWin Scripting Mode", "KWin"), this))
    , m_snippetsMenu(new QMenu(i18n("Templates"), this))
    , m_fileDialog(nullptr)
    , m_closeWhenCompleted(false)
    , m_mode(mode)
{
    addAction(KStandardAction::close(this, SLOT(close()), this));
    addAction(m_saveAction);
    addAction(m_clearAction);

    setWindowTitle(i18n("Desktop Shell Scripting Console"));
    setAttribute(Qt::WA_DeleteOnClose);
    // setButtons(QDialog::None);

    QWidget *widget = new QWidget(m_splitter);
    QVBoxLayout *editorLayout = new QVBoxLayout(widget);

    QLabel *label = new QLabel(i18n("Editor"), widget);
    QFont f = label->font();
    f.setBold(true);
    label->setFont(f);
    editorLayout->addWidget(label);

    connect(m_snippetsMenu, &QMenu::aboutToShow, this, &InteractiveConsole::populateTemplatesMenu);

    QToolButton *loadTemplateButton = new QToolButton(this);
    loadTemplateButton->setPopupMode(QToolButton::InstantPopup);
    loadTemplateButton->setMenu(m_snippetsMenu);
    loadTemplateButton->setText(i18n("Load"));
    connect(loadTemplateButton, &QToolButton::triggered, this, &InteractiveConsole::loadTemplate);

    QToolButton *useTemplateButton = new QToolButton(this);
    useTemplateButton->setPopupMode(QToolButton::InstantPopup);
    useTemplateButton->setMenu(m_snippetsMenu);
    useTemplateButton->setText(i18n("Use"));
    connect(useTemplateButton, &QToolButton::triggered, this, &InteractiveConsole::useTemplate);

    QActionGroup *modeGroup = new QActionGroup(this);
    modeGroup->addAction(m_plasmaAction);
    modeGroup->addAction(m_kwinAction);
    m_plasmaAction->setCheckable(true);
    m_kwinAction->setCheckable(true);
    m_kwinAction->setChecked(mode == KWinConsole);
    m_plasmaAction->setChecked(mode == PlasmaConsole);
    connect(modeGroup, &QActionGroup::triggered, this, &InteractiveConsole::modeSelectionChanged);

    KToolBar *toolBar = new KToolBar(this, true, false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolBar->addAction(m_loadAction);
    toolBar->addAction(m_saveAction);
    toolBar->addAction(m_clearAction);
    toolBar->addAction(m_executeAction);
    toolBar->addAction(m_plasmaAction);
    toolBar->addAction(m_kwinAction);
    toolBar->addWidget(loadTemplateButton);
    toolBar->addWidget(useTemplateButton);

    editorLayout->addWidget(toolBar);

    auto tryLoadingKatePart = [=]() -> KTextEditor::Document * {
        const auto loadResult = KPluginFactory::instantiatePlugin<KTextEditor::Document>(KPluginMetaData(QStringLiteral("kf5/parts/katepart")), this);

        if (!loadResult) {
            qWarning() << "Error loading katepart plugin:" << loadResult.errorString;
            return nullptr;
        }

        KTextEditor::Document *result = loadResult.plugin;

        result->setHighlightingMode(QStringLiteral("JavaScript/PlasmaDesktop"));

        KTextEditor::View *view = result->createView(widget);
        view->setContextMenu(view->defaultContextMenu());

        KTextEditor::ConfigInterface *config = qobject_cast<KTextEditor::ConfigInterface *>(view);
        if (config) {
            config->setConfigValue(QStringLiteral("line-numbers"), true);
            config->setConfigValue(QStringLiteral("dynamic-word-wrap"), true);
        }

        editorLayout->addWidget(view);
        connect(result, &KTextEditor::Document::textChanged, this, &InteractiveConsole::scriptTextChanged);

        return result;
    };

    m_editorPart = tryLoadingKatePart();

    if (!m_editorPart) {
        m_editor = new KTextEdit(widget);
        editorLayout->addWidget(m_editor);
        connect(m_editor, &QTextEdit::textChanged, this, &InteractiveConsole::scriptTextChanged);
    }

    m_splitter->addWidget(widget);

    widget = new QWidget(m_splitter);
    QVBoxLayout *outputLayout = new QVBoxLayout(widget);

    label = new QLabel(i18n("Output"), widget);
    f = label->font();
    f.setBold(true);
    label->setFont(f);
    outputLayout->addWidget(label);

    KToolBar *outputToolBar = new KToolBar(widget, true, false);
    outputToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QAction *clearOutputAction = KStandardAction::clear(this, SLOT(clearOutput()), this);
    outputToolBar->addAction(clearOutputAction);
    outputLayout->addWidget(outputToolBar);

    m_output = new QTextBrowser(widget);
    outputLayout->addWidget(m_output);
    m_splitter->addWidget(widget);

    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(m_splitter);

    KConfigGroup cg(KSharedConfig::openConfig(), "InteractiveConsole");
    restoreGeometry(cg.readEntry<QByteArray>("Geometry", QByteArray()));

    m_splitter->setStretchFactor(0, 10);
    m_splitter->restoreState(cg.readEntry("SplitterState", QByteArray()));

    scriptTextChanged();

    connect(m_executeAction, &QAction::triggered, this, &InteractiveConsole::evaluateScript);
    m_executeAction->setShortcut(Qt::CTRL | Qt::Key_E);

    const QString autosave = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + s_autosaveFileName;
    if (QFile::exists(autosave)) {
        loadScript(autosave);
    }
}

InteractiveConsole::~InteractiveConsole()
{
    KConfigGroup cg(KSharedConfig::openConfig(), "InteractiveConsole");
    cg.writeEntry("Geometry", saveGeometry());
    cg.writeEntry("SplitterState", m_splitter->saveState());
}

void InteractiveConsole::setMode(const QString &mode)
{
    if (mode.toLower() == QLatin1String("desktop")) {
        m_plasmaAction->trigger();
    } else if (mode.toLower() == QLatin1String("windowmanager")) {
        m_kwinAction->trigger();
    }
}

void InteractiveConsole::modeSelectionChanged()
{
    if (m_plasmaAction->isChecked()) {
        m_mode = PlasmaConsole;
    } else if (m_kwinAction->isChecked()) {
        m_mode = KWinConsole;
    }

    Q_EMIT modeChanged();
}

QString InteractiveConsole::mode() const
{
    if (m_mode == KWinConsole) {
        return QStringLiteral("windowmanager");
    }

    return QStringLiteral("desktop");
}

void InteractiveConsole::setScriptInterface(QObject *obj)
{
    if (m_scriptEngine != obj) {
        if (m_scriptEngine) {
            disconnect(m_scriptEngine, nullptr, this, nullptr);
        }

        m_scriptEngine = obj;
        connect(m_scriptEngine, SIGNAL(print(QString)), this, SLOT(print(QString)));
        connect(m_scriptEngine, SIGNAL(printError(QString)), this, SLOT(print(QString)));
        Q_EMIT scriptEngineChanged();
    }
}

QObject *InteractiveConsole::scriptEngine() const
{
    return m_scriptEngine;
}

void InteractiveConsole::loadScript(const QString &script)
{
    if (m_editorPart) {
        m_editorPart->closeUrl(false);
        if (m_editorPart->openUrl(QUrl::fromLocalFile(script))) {
            m_editorPart->setHighlightingMode(QStringLiteral("JavaScript/PlasmaDesktop"));
            return;
        }
    } else {
        QFile file(KShell::tildeExpand(script));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            m_editor->setText(file.readAll());
            return;
        }
    }

    m_output->append(i18n("Unable to load script file <b>%1</b>", script));
}

void InteractiveConsole::showEvent(QShowEvent *)
{
    if (m_editorPart) {
        m_editorPart->views().constFirst()->setFocus();
    } else {
        m_editor->setFocus();
    }

    KWindowSystem::setOnDesktop(winId(), KWindowSystem::currentDesktop());
    Q_EMIT visibleChanged(true);
}

void InteractiveConsole::closeEvent(QCloseEvent *event)
{
    // need to save first!
    const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + s_autosaveFileName;
    m_closeWhenCompleted = true;
    saveScript(QUrl::fromLocalFile(path));
    QDialog::closeEvent(event);
    Q_EMIT visibleChanged(false);
}

void InteractiveConsole::reject()
{
    QDialog::reject();
    close();
}

void InteractiveConsole::print(const QString &string)
{
    m_output->append(string);
}

void InteractiveConsole::scriptTextChanged()
{
    const bool enable = m_editorPart ? !m_editorPart->isEmpty() : !m_editor->document()->isEmpty();
    m_saveAction->setEnabled(enable);
    m_clearAction->setEnabled(enable);
    m_executeAction->setEnabled(enable);
}

void InteractiveConsole::openScriptFile()
{
    delete m_fileDialog;

    m_fileDialog = new QFileDialog();
    m_fileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    m_fileDialog->setWindowTitle(i18n("Open Script File"));

    QStringList mimetypes;
    mimetypes << QStringLiteral("application/javascript");
    m_fileDialog->setMimeTypeFilters(mimetypes);

    connect(m_fileDialog, &QDialog::finished, this, &InteractiveConsole::openScriptUrlSelected);
    m_fileDialog->show();
}

void InteractiveConsole::openScriptUrlSelected(int result)
{
    if (!m_fileDialog) {
        return;
    }

    if (result == QDialog::Accepted) {
        const QUrl url = m_fileDialog->selectedUrls().constFirst();
        if (!url.isEmpty()) {
            loadScriptFromUrl(url);
        }
    }

    m_fileDialog->deleteLater();
    m_fileDialog = nullptr;
}

void InteractiveConsole::loadScriptFromUrl(const QUrl &url)
{
    if (m_editorPart) {
        m_editorPart->closeUrl(false);
        m_editorPart->openUrl(url);
        m_editorPart->setHighlightingMode(QStringLiteral("JavaScript/PlasmaDesktop"));
    } else {
        m_editor->clear();
        m_editor->setEnabled(false);

        if (m_job) {
            m_job.data()->kill();
        }

        auto job = KIO::get(url, KIO::Reload, KIO::HideProgressInfo);
        connect(job, &KIO::TransferJob::data, this, &InteractiveConsole::scriptFileDataRecvd);
        connect(job, &KJob::result, this, &InteractiveConsole::reenableEditor);
        m_job = job;
    }
}

void InteractiveConsole::populateTemplatesMenu()
{
    m_snippetsMenu->clear();
    auto templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), [](const KPluginMetaData &metaData) {
        return metaData.value(QStringLiteral("X-Plasma-Shell")) == qApp->applicationName();
    });
    std::sort(templates.begin(), templates.end(), [](const KPluginMetaData &left, const KPluginMetaData &right) {
        return left.name() < right.name();
    });
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LayoutTemplate"));
    for (const auto &templateMetaData : qAsConst(templates)) {
        package.setPath(templateMetaData.pluginId());
        const QString scriptFile = package.filePath("mainscript");
        if (!scriptFile.isEmpty()) {
            QAction *action = m_snippetsMenu->addAction(templateMetaData.name());
            action->setData(templateMetaData.pluginId());
        }
    }
}

void InteractiveConsole::loadTemplate(QAction *action)
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LayoutTemplate"), action->data().toString());
    const QString scriptFile = package.filePath("mainscript");
    if (!scriptFile.isEmpty()) {
        loadScriptFromUrl(QUrl::fromLocalFile(scriptFile));
    }
}

void InteractiveConsole::useTemplate(QAction *action)
{
    QString code("var template = loadTemplate('" + action->data().toString() + "')");
    if (m_editorPart) {
        const QList<KTextEditor::View *> views = m_editorPart->views();
        if (views.isEmpty()) {
            m_editorPart->insertLines(m_editorPart->lines(), QStringList() << code);
        } else {
            KTextEditor::Cursor cursor = views.at(0)->cursorPosition();
            m_editorPart->insertLines(cursor.line(), QStringList() << code);
            cursor.setLine(cursor.line() + 1);
            views.at(0)->setCursorPosition(cursor);
        }
    } else {
        m_editor->insertPlainText(code);
    }
}

void InteractiveConsole::scriptFileDataRecvd(KIO::Job *job, const QByteArray &data)
{
    Q_ASSERT(m_editor);

    if (job == m_job.data()) {
        m_editor->insertPlainText(data);
    }
}

void InteractiveConsole::saveScript()
{
    if (m_editorPart) {
        m_editorPart->documentSaveAs();
        return;
    }

    delete m_fileDialog;

    m_fileDialog = new QFileDialog();
    m_fileDialog->setAcceptMode(QFileDialog::AcceptSave);
    m_fileDialog->setWindowTitle(i18n("Save Script File"));

    QStringList mimetypes;
    mimetypes << QStringLiteral("application/javascript");
    m_fileDialog->setMimeTypeFilters(mimetypes);

    connect(m_fileDialog, &QDialog::finished, this, &InteractiveConsole::saveScriptUrlSelected);
    m_fileDialog->show();
}

void InteractiveConsole::saveScriptUrlSelected(int result)
{
    if (!m_fileDialog) {
        return;
    }

    if (result == QDialog::Accepted) {
        const QUrl url = m_fileDialog->selectedUrls().constFirst();
        if (!url.isEmpty()) {
            saveScript(url);
        }
    }

    m_fileDialog->deleteLater();
    m_fileDialog = nullptr;
}

void InteractiveConsole::saveScript(const QUrl &url)
{
    // create the folder to save if doesn't exists
    QFileInfo info(url.path());
    QDir dir;
    dir.mkpath(info.absoluteDir().absolutePath());

    if (m_editorPart) {
        m_editorPart->saveAs(url);
    } else {
        m_editor->setEnabled(false);

        if (m_job) {
            m_job.data()->kill();
        }

        auto job = KIO::put(url, -1, KIO::HideProgressInfo);
        connect(job, &KIO::TransferJob::dataReq, this, &InteractiveConsole::scriptFileDataReq);
        connect(job, &KJob::result, this, &InteractiveConsole::reenableEditor);
        m_job = job;
    }
}

void InteractiveConsole::scriptFileDataReq(KIO::Job *job, QByteArray &data)
{
    Q_ASSERT(m_editor);

    if (!m_job || m_job.data() != job) {
        return;
    }

    data.append(m_editor->toPlainText().toLocal8Bit());
    m_job.clear();
}

void InteractiveConsole::reenableEditor(KJob *job)
{
    Q_ASSERT(m_editor);
    if (m_closeWhenCompleted && job->error() != 0) {
        close();
    }

    m_closeWhenCompleted = false;
    m_editor->setEnabled(true);
}

void InteractiveConsole::evaluateScript()
{
    // qDebug() << "evaluating" << m_editor->toPlainText();
    m_output->moveCursor(QTextCursor::End);
    QTextCursor cursor = m_output->textCursor();
    m_output->setTextCursor(cursor);

    QTextCharFormat format;
    format.setFontWeight(QFont::Bold);
    format.setFontUnderline(true);

    if (cursor.position() > 0) {
        cursor.insertText(QStringLiteral("\n\n"));
    }

    QDateTime dt = QDateTime::currentDateTime();
    cursor.insertText(i18n("Executing script at %1", QLocale().toString(dt)));

    format.setFontWeight(QFont::Normal);
    format.setFontUnderline(false);
    QTextBlockFormat block = cursor.blockFormat();
    block.setLeftMargin(10);
    cursor.insertBlock(block, format);
    QElapsedTimer t;
    t.start();

    if (m_mode == PlasmaConsole) {
        QDBusMessage message = QDBusMessage::createMethodCall(s_plasmaShellService,
                                                              QStringLiteral("/PlasmaShell"),
                                                              QStringLiteral("org.kde.PlasmaShell"),
                                                              QStringLiteral("evaluateScript"));
        QList<QVariant> arguments;
        arguments << QVariant(m_editorPart->text());
        message.setArguments(arguments);
        QDBusMessage reply = QDBusConnection::sessionBus().call(message);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            print(reply.errorMessage());
        } else {
            print(reply.arguments().first().toString());
        }
    } else if (m_mode == KWinConsole) {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + s_autosaveFileName;
        saveScript(QUrl::fromLocalFile(path));

        QDBusMessage message = QDBusMessage::createMethodCall(s_kwinService, QStringLiteral("/Scripting"), QString(), QStringLiteral("loadScript"));
        QList<QVariant> arguments;
        arguments << QVariant(path);
        message.setArguments(arguments);
        QDBusMessage reply = QDBusConnection::sessionBus().call(message);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            print(reply.errorMessage());
        } else {
            const int id = reply.arguments().constFirst().toInt();
            QDBusConnection::sessionBus().connect(s_kwinService, "/" + QString::number(id), QString(), QStringLiteral("print"), this, SLOT(print(QString)));
            QDBusConnection::sessionBus()
                .connect(s_kwinService, "/" + QString::number(id), QString(), QStringLiteral("printError"), this, SLOT(print(QString)));
            message = QDBusMessage::createMethodCall(s_kwinService, "/" + QString::number(id), QString(), QStringLiteral("run"));
            reply = QDBusConnection::sessionBus().call(message);
            if (reply.type() == QDBusMessage::ErrorMessage) {
                print(reply.errorMessage());
            }
        }
    }

    cursor.insertText(QStringLiteral("\n\n"));
    format.setFontWeight(QFont::Bold);
    // xgettext:no-c-format
    cursor.insertText(i18n("Runtime: %1ms", QString::number(t.elapsed())), format);
    block.setLeftMargin(0);
    cursor.insertBlock(block);
    m_output->ensureCursorVisible();
}

void InteractiveConsole::clearEditor()
{
    if (m_editorPart) {
        m_editorPart->clear();
    } else {
        m_editor->clear();
    }
}

void InteractiveConsole::clearOutput()
{
    m_output->clear();
}
