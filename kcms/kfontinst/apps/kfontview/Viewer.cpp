/*
    SPDX-FileCopyrightText: 2003-2007 Craig Drummond <craig@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "Viewer.h"
#include "KfiConstants.h"
#include "config-workspace.h"
#include "kfontview_debug.h"

#include <KAboutData>
#include <KActionCollection>
#include <KConfigGroup>
#include <KDBusService>
#include <KParts/NavigationExtension>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <QAction>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileDialog>
#include <QUrl>

using namespace Qt::StringLiterals;

namespace KFI
{
CViewer::CViewer()
{
    const auto result = KPluginFactory::instantiatePlugin<KParts::ReadOnlyPart>(KPluginMetaData(QStringLiteral("kf6/parts/kfontviewpart")), this);

    if (!result) {
        qCWarning(KFONTVIEW_DEBUG) << "Error loading kfontviewpart:" << result.errorString;
        exit(1);
    }

    m_preview = result.plugin;

    m_openAct = actionCollection()->addAction(KStandardActions::Open, this, &CViewer::fileOpen);
    actionCollection()->addAction(KStandardActions::Quit, this, &CViewer::close);
    actionCollection()->addAction(KStandardActions::KeyBindings, this, &CViewer::configureKeys);
    m_printAct = actionCollection()->addAction(KStandardActions::Print, m_preview, [this] {
        QMetaObject::invokeMethod(m_preview, "print");
    });

    // Make tooltips more specific, instead of "document".
    m_openAct->setToolTip(i18n("Open an existing font file"));
    m_printAct->setToolTip(i18n("Print font preview"));

    m_printAct->setEnabled(false);

    if (m_preview->navigationExtension()) {
        connect(m_preview->navigationExtension(), &KParts::NavigationExtension::enableAction, this, &CViewer::enableAction);
    }

    setCentralWidget(m_preview->widget());
    createGUI(m_preview);

    setAutoSaveSettings();
    applyMainWindowSettings(KSharedConfig::openConfig()->group(QStringLiteral("MainWindow")));
}

void CViewer::fileOpen()
{
    QFileDialog dlg(this, i18n("Select Font to View"));
    dlg.setFileMode(QFileDialog::ExistingFile);
    dlg.setMimeTypeFilters(QStringList() << u"application/x-font-ttf"_s << u"application/x-font-otf"_s << u"application/x-font-type1"_s
                                         << u"application/x-font-bdf"_s << u"application/x-font-pcf"_s);
    if (dlg.exec() == QDialog::Accepted) {
        QUrl url = dlg.selectedUrls().value(0);
        if (url.isValid()) {
            showUrl(url);
        }
    }
}

void CViewer::showUrl(const QUrl &url)
{
    if (url.isValid()) {
        m_preview->openUrl(url);
    }
}

void CViewer::configureKeys()
{
    KShortcutsDialog dlg(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);

    dlg.addCollection(actionCollection());
    dlg.configure();
}

void CViewer::enableAction(const char *name, bool enable)
{
    if (0 == qstrcmp("print", name)) {
        m_printAct->setEnabled(enable);
    }
}

class ViewerApplication : public QApplication
{
public:
    ViewerApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
        cmdParser.addPositionalArgument(QLatin1String("[URL]"), i18n("URL to open"));
    }

    QCommandLineParser *parser()
    {
        return &cmdParser;
    }

public Q_SLOTS:
    void activate(const QStringList &args, const QString &workingDirectory)
    {
        auto *viewer = new KFI::CViewer;
        viewer->show();

        if (!args.isEmpty()) {
            cmdParser.process(args);
            bool first = true;
            for (const auto args = cmdParser.positionalArguments(); const QString &arg : args) {
                QUrl url(QUrl::fromUserInput(arg, workingDirectory));

                if (!first) {
                    viewer = new KFI::CViewer;
                    viewer->show();
                }
                viewer->showUrl(url);
                first = false;
            }
        }
    }

private:
    QCommandLineParser cmdParser;
};

}

int main(int argc, char **argv)
{
    KFI::ViewerApplication app(argc, argv);

    KLocalizedString::setApplicationDomain(KFI_CATALOGUE);
    KAboutData aboutData(u"kfontview"_s,
                         i18n("Font Viewer"),
                         QLatin1String(WORKSPACE_VERSION_STRING),
                         i18n("Simple font viewer"),
                         KAboutLicense::GPL,
                         i18n("(C) Craig Drummond, 2004-2007"));
    KAboutData::setApplicationData(aboutData);

    QCommandLineParser *parser = app.parser();
    aboutData.setupCommandLine(parser);
    parser->process(app);
    aboutData.processCommandLine(parser);

    KDBusService dbusService(KDBusService::Unique);
    QGuiApplication::setWindowIcon(QIcon::fromTheme(u"kfontview"_s));
    app.activate(app.arguments(), QDir::currentPath());
    QObject::connect(&dbusService, &KDBusService::activateRequested, &app, &KFI::ViewerApplication::activate);

    return app.exec();
}

#include "moc_Viewer.cpp"
