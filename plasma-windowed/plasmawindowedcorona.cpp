/*
    SPDX-FileCopyrightText: 2014 Bhushan Shah <bhush94@gmail.com>
    SPDX-FileCopyrightText: 2014 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "plasmawindowedcorona.h"
#include "plasmawindowedview.h"
#include <QAction>
#include <QCommandLineParser>
#include <QDebug>

#include <KActionCollection>
#include <KLocalizedString>

#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <Plasma/PluginLoader>

using namespace Qt::StringLiterals;

PlasmaWindowedCorona::PlasmaWindowedCorona(const QString &shell, QObject *parent)
    : Plasma::Corona(parent)
{
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Shell"));
    package.setPath(shell);
    setKPackage(package);
    // QMetaObject::invokeMethod(this, "load", Qt::QueuedConnection);
    load();
}

void PlasmaWindowedCorona::loadApplet(const QString &applet, const QVariantList &arguments)
{
    if (containments().isEmpty()) {
        return;
    }

    Plasma::Containment *cont = containments().first();

    // forbid more instances per applet (todo: activate the corresponding already loaded applet)
    for (Plasma::Applet *a : cont->applets()) {
        if (a->pluginMetaData().pluginId() == applet) {
            return;
        }
    }
    m_view = new PlasmaWindowedView();
    m_view->setHasStatusNotifier(m_hasStatusNotifier);
    m_view->show();

    KConfigGroup appletsGroup(KSharedConfig::openConfig(), QStringLiteral("Applets"));
    QString plugin;
    for (const QString &group : appletsGroup.groupList()) {
        KConfigGroup cg(&appletsGroup, group);
        plugin = cg.readEntry("plugin", QString());

        if (plugin == applet) {
            Plasma::Applet *a = Plasma::PluginLoader::self()->loadApplet(applet, group.toInt(), arguments);
            if (!a) {
                qWarning() << "Unable to load applet" << applet << "with arguments" << arguments;
                delete m_view;
                m_view = nullptr;
                return;
            }
            // Some applets call corona() in their restore function,
            // which requires a set parent.
            a->setParent(this);
            a->restore(cg);

            // Access a->config() before adding to containment
            // will cause applets to be saved in palsmawindowedrc
            // so applets will only be created on demand
            KConfigGroup cg2 = a->config();
            cont->addApplet(a);

            m_view->setApplet(a);
            return;
        }
    }

    Plasma::Applet *a = Plasma::PluginLoader::self()->loadApplet(applet, 0, arguments);
    if (!a) {
        qWarning() << "Unable to load applet" << applet << "with arguments" << arguments;
        delete m_view;
        m_view = nullptr;
        return;
    }

    // Access a->config() before adding to containment
    // will cause applets to be saved in palsmawindowedrc
    // so applets will only be created on demand
    KConfigGroup cg2 = a->config();
    cont->addApplet(a);

    m_view->setApplet(a);
}

void PlasmaWindowedCorona::activateRequested(const QStringList &arguments, const QString &workingDirectory)
{
    Q_UNUSED(workingDirectory)
    if (arguments.count() <= 1) {
        return;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("Plasma Windowed"));
    parser.addOption(
        QCommandLineOption(QStringLiteral("statusnotifier"), i18n("Makes the plasmoid stay alive in the Notification Area, even when the window is closed.")));
    parser.addPositionalArgument(QStringLiteral("applet"), i18n("The applet to open."));
    parser.addPositionalArgument(QStringLiteral("args"), i18n("Arguments to pass to the plasmoid."), QStringLiteral("[args...]"));
    QCommandLineOption configOpt(u"config"_s);
    parser.addOption(configOpt);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.process(arguments);

    if (parser.isSet(configOpt)) {
        m_view->showConfigurationInterface();
        return;
    }

    if (parser.positionalArguments().isEmpty()) {
        parser.showHelp(1);
    }

    const QStringList positionalArguments = parser.positionalArguments();

    QVariantList args;
    QStringList::const_iterator constIterator = positionalArguments.constBegin() + 1;
    for (; constIterator != positionalArguments.constEnd(); ++constIterator) {
        args << (*constIterator);
    }

    loadApplet(positionalArguments.first(), args);
}

QRect PlasmaWindowedCorona::screenGeometry(int id) const
{
    Q_UNUSED(id);
    // TODO?
    return QRect();
}

void PlasmaWindowedCorona::load()
{
    /*this won't load applets, since applets are in plasmawindowedrc*/
    loadLayout(QStringLiteral("plasmawindowed-appletsrc"));

    bool found = false;
    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Containment::Desktop) {
            found = true;
            break;
        }
    }

    if (!found) {
        qDebug() << "Loading default layout";
        createContainment(QStringLiteral("empty"));
        saveLayout(QStringLiteral("plasmawindowed-appletsrc"));
    }

    for (auto c : containments()) {
        if (c->containmentType() == Plasma::Containment::Desktop) {
            m_containment = c;
            m_containment->setFormFactor(Plasma::Types::Application);
            QAction *removeAction = c->internalAction(QStringLiteral("remove"));
            if (removeAction) {
                removeAction->deleteLater();
            }
            break;
        }
    }
}

void PlasmaWindowedCorona::setHasStatusNotifier(bool stay)
{
    m_hasStatusNotifier = stay;
}

#include "moc_plasmawindowedcorona.cpp"
