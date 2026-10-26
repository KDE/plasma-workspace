/*
    SPDX-FileCopyrightText: 2009 Esben Mose Hansen <kde@mosehansen.dk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clipcommandprocess.h"

#include <QMimeData>

#include <KMacroExpander>

#include "historyitem.h"
#include "historymodel.h"
#include "systemclipboard.h"
#include "urlgrabber.h"

ClipCommandProcess::ClipCommandProcess(const QStringList &actionCapturedTexts, const ClipCommand &command, const QString &clip, const QString &uuid)
    : KProcess()
    , m_model(HistoryModel::self())
    , m_clip(SystemClipboard::self())
    , m_newhistoryItem()
    , m_historyItemUuid(uuid)
{
    QHash<QChar, QString> map;
    map.insert(QLatin1Char('s'), clip);

    // support %u, %U (indicates url param(s)) and %f, %F (file param(s))
    map.insert(QLatin1Char('u'), clip);
    map.insert(QLatin1Char('U'), clip);
    map.insert(QLatin1Char('f'), clip);
    map.insert(QLatin1Char('F'), clip);

    // support only %0 and the first 9 matches...
    const int numMatches = qMin(10, actionCapturedTexts.size());
    for (int i = 0; i < numMatches; ++i) {
        map.insert(QChar('0' + i), actionCapturedTexts.at(i));
    }

    setOutputChannelMode(OnlyStdoutChannel);
    setShellCommand(KMacroExpander::expandMacrosShellQuote(command.command, map).trimmed());

    connect(this, SIGNAL(finished(int, QProcess::ExitStatus)), SLOT(slotFinished(int, QProcess::ExitStatus)));
    if (command.output != ClipCommand::IGNORE) {
        connect(this, &QIODevice::readyRead, this, &ClipCommandProcess::slotStdOutputAvailable);
    }
    if (command.output != ClipCommand::REPLACE) {
        m_historyItemUuid.clear();
    }
}

void ClipCommandProcess::slotFinished(int /*exitCode*/, QProcess::ExitStatus /*newState*/)
{
    if (!m_newhistoryItem.isEmpty()) {
        auto data = std::make_unique<QMimeData>();
        data->setText(m_newhistoryItem);
        m_clip->setMimeData(data.get(), SystemClipboard::SelectionMode(SystemClipboard::Selection | SystemClipboard::Clipboard));
        m_clip->checkClipData(QClipboard::Clipboard, data.get());
    }
    // If an history item was provided, remove it so that the new item can replace it
    if (!m_historyItemUuid.isEmpty()) {
        m_model->remove(m_historyItemUuid);
    }
    deleteLater();
}

void ClipCommandProcess::slotStdOutputAvailable()
{
    m_newhistoryItem.append(QString::fromLocal8Bit(this->readAllStandardOutput()));
}

#include "moc_clipcommandprocess.cpp"
