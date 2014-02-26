/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef INTERACTIVECONSOLE
#define INTERACTIVECONSOLE

#include <QWeakPointer>
#include <QScriptValue>

#include <KDialog>

#include <KIO/Job>

class QSplitter;

class QAction;
class KFileDialog;
class QMenu;
class KTextEdit;
class KTextBrowser;

namespace KTextEditor
{
    class Document;
} // namespace KParts

namespace Plasma
{
    class Corona;
} // namespace Plasma

class ScriptEngine;

class InteractiveConsole : public KDialog
{
    Q_OBJECT

public:
    InteractiveConsole(Plasma::Corona *corona, QWidget *parent = 0);
    ~InteractiveConsole();

    void loadScript(const QString &path);
    enum ConsoleMode {
        PlasmaConsole,
        KWinConsole
    };
    void setMode(ConsoleMode mode);

protected:
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *event);

protected Q_SLOTS:
    void print(const QString &string);
    void reject();

private Q_SLOTS:
    void openScriptFile();
    void saveScript();
    void scriptTextChanged();
    void evaluateScript();
    void clearEditor();
    void clearOutput();
    void scriptFileDataRecvd(KIO::Job *job, const QByteArray &data);
    void scriptFileDataReq(KIO::Job *job, QByteArray &data);
    void reenableEditor(KJob *job);
    void saveScriptUrlSelected(int result);
    void openScriptUrlSelected(int result);
    void loadScriptFromUrl(const QUrl &url);
    void populateTemplatesMenu();
    void loadTemplate(QAction *);
    void useTemplate(QAction *);
    void modeChanged();

private:
    void onClose();
    void saveScript(const QUrl &url);

    Plasma::Corona *m_corona;
    QSplitter *m_splitter;
    KTextEditor::Document *m_editorPart;
    KTextEdit *m_editor;
    KTextBrowser *m_output;
    QAction *m_loadAction;
    QAction *m_saveAction;
    QAction *m_clearAction;
    QAction *m_executeAction;
    QAction *m_plasmaAction;
    QAction *m_kwinAction;
    QMenu *m_snippetsMenu;

    KFileDialog *m_fileDialog;
    QWeakPointer<KIO::Job> m_job;
    bool m_closeWhenCompleted;
    ConsoleMode m_mode;
};

#endif

