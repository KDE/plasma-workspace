/*
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <Plasma/Applet>

#include <QPointer>

#include <KPropertiesDialog>

class KFileItemActions;

class QMenu;

namespace TaskManager
{
class StartupTasksModel;
}

class IconApplet : public Plasma::Applet
{
    Q_OBJECT

    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)

    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY iconNameChanged)
    Q_PROPERTY(QString genericName READ genericName NOTIFY genericNameChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY isValidChanged)

public:
    explicit IconApplet(QObject *parent, const KPluginMetaData &data, const QVariantList &args);
    ~IconApplet() override;

    void init() override;
    void configChanged() override;

    QUrl url() const;
    void setUrl(const QUrl &url);

    QString name() const;
    QString iconName() const;
    QString genericName() const;
    bool isValid() const;

    QList<QAction *> contextualActions() override;

    Q_INVOKABLE void run();
    Q_INVOKABLE void processDrop(QObject *dropEvent);
    Q_INVOKABLE void configure();

    Q_INVOKABLE bool isAcceptableDrag(QObject *dropEvent);

Q_SIGNALS:
    void urlChanged(const QUrl &url);

    void nameChanged(const QString &name);
    void iconNameChanged(const QString &iconName);
    void genericNameChanged(const QString &genericName);
    void isValidChanged();
    void jumpListActionsChanged(const QVariantList &jumpListActions);

private:
    void setIconName(const QString &iconName);

    static QList<QUrl> urlsFromDrop(QObject *dropEvent);
    static bool isExecutable(const QMimeType &mimeType);

    void populate();
    void populateFromDesktopFile(const QString &path);

    QString localPath() const;
    void setLocalPath(const QString &localPath);

    QUrl m_url;
    QString m_localPath;

    QString m_name;
    QString m_iconName;
    QString m_genericName;

    QList<QAction *> m_jumpListActions;
    QAction *m_separatorAction = nullptr;
    QList<QAction *> m_openWithActions;

    QAction *m_openContainingFolderAction = nullptr;

    KFileItemActions *m_fileItemActions = nullptr;
    std::unique_ptr<QMenu> m_openWithMenu;

    QPointer<KPropertiesDialog> m_configDialog;

    TaskManager::StartupTasksModel *m_startupTasksModel = nullptr;
};
