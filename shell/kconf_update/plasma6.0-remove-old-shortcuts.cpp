/*
    SPDX-FileCopyrightText: 2023 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KConfigGroup>
#include <KGlobalAccel>
#include <KSharedConfig>
#include <cstdlib>

#include <QAction>
#include <QGuiApplication>

/**
 * In plasma5, every single contextual action was registered as a global shortcut, with
 * Often confusing and duplicated names. Now this capability has been dropped in Plasma6
 * and applets that want global shortcuts will have to do that explicitly
 *
 * @since 6.0
 */
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    const KSharedConfigPtr configPtr = KSharedConfig::openConfig(QString::fromLatin1("kglobalshortcutsrc"), KConfig::FullConfig);
    KConfigGroup plasmaGroup(configPtr, QStringLiteral("plasmashell"));

    // All of those actions
    // TODO: maybe they should eventually all be renamed to have applet plugin id prefix?
    const QSet<QString> allowedActionNames({QStringLiteral("clipboard_action"),
                                            QStringLiteral("cycle-panels"),
                                            QStringLiteral("cycleNextAction"),
                                            QStringLiteral("cyclePrevAction"),
                                            QStringLiteral("edit_clipboard"),
                                            QStringLiteral("clear-history"),
                                            QStringLiteral("manage activities"),
                                            QStringLiteral("next activity"),
                                            QStringLiteral("previous activity"),
                                            QStringLiteral("repeat_action"),
                                            QStringLiteral("show dashboard"),
                                            QStringLiteral("show-barcode"),
                                            QStringLiteral("show-on-mouse-pos"),
                                            QStringLiteral("stop current activity"),
                                            QStringLiteral("switch to next activity"),
                                            QStringLiteral("switch to previous activity"),
                                            QStringLiteral("toggle do not disturb")});

    QStringList filteredActionNames = plasmaGroup.keyList();

    filteredActionNames.erase(std::remove_if(filteredActionNames.begin(),
                                             filteredActionNames.end(),
                                             [allowedActionNames](const QString &str) {
                                                 return str == QStringLiteral("_k_friendly_name")
                                                     || str.startsWith(QStringLiteral("activate task manager entry"))
                                                     || str.startsWith(QStringLiteral("activate widget")) || allowedActionNames.contains(str);
                                             }),
                              filteredActionNames.end());

    for (const QString &actionName : filteredActionNames) {
        QAction action;
        qWarning() << actionName;
        action.setObjectName(actionName);
        action.setProperty("componentName", QStringLiteral("plasmashell"));
        KGlobalAccel::self()->setShortcut(&action, {QKeySequence()}, KGlobalAccel::NoAutoloading);
        KGlobalAccel::self()->removeAllShortcuts(&action);
    }

    return EXIT_SUCCESS;
}
