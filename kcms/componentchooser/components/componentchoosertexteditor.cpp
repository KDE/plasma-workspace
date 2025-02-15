/*
    SPDX-FileCopyrightText: 2022 Thiago Sueto <herzenschein@gmail.com>
    SPDX-FileCopyrightText: 2022 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "componentchoosertexteditor.h"

ComponentChooserTextEditor::ComponentChooserTextEditor(QObject *parent)
    : ComponentChooser(parent,
                       QStringLiteral("text/plain"),
                       QStringLiteral("TextEditor"),
                       QStringLiteral("org.kde.kate.desktop"),
                       i18n("Select default text editor"))
{
}

static const QStringList textEditorMimetypes{QStringLiteral("text/plain"),
                                             QStringLiteral("text/x-cmake"),
                                             QStringLiteral("text/markdown"),
                                             QStringLiteral("application/x-docbook+xml"),
                                             QStringLiteral("application/json"),
                                             QStringLiteral("application/x-yaml")};

QStringList ComponentChooserTextEditor::mimeTypes() const
{
    return textEditorMimetypes;
}
