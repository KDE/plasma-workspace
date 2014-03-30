/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2002 Luboš Luňák <l.lunak@kde.org>
Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#ifndef SCREENLOCKER_GREETER_H
#define SCREENLOCKER_GREETER_H

#include <kgreeterplugin.h>

// forward declarations
class KGreetPlugin;
class KLibrary;
class QSocketNotifier;

struct GreeterPluginHandle {
    KLibrary *library;
    KGreeterPluginInfo *info;
};

namespace ScreenLocker
{
/**
 * @short Class which checks authentication through KGreeterPlugin framework.
 *
 * This class can be used to perform an authentication through the KGreeterPlugin framework.
 * It provides a QWidget containing the widgets provided by the various greeter.
 *
 * To perform an authentication through the greeter invoke the @link verify slot. The class
 * will emit either @link greeterAccepted or @link greeterFailed signals depending on whether
 * the authentication succeeded or failed.
 *
 * @author Oswald Buddenhagen <ossi@kde.org>
 **/
class Greeter : public QObject, public KGreeterPluginHandler
{
    Q_OBJECT
public:
    Greeter(QObject *parent);
    virtual ~Greeter();
    bool isValid() const {
        return m_valid;
    }
    // from KGreetPluginHandler
    virtual void gplugReturnText(const char *text, int tag);
    virtual void gplugReturnBinary(const char *data);
    virtual void gplugSetUser(const QString &);
    virtual void gplugStart();
    virtual void gplugChanged();
    virtual void gplugActivity();
    virtual void gplugMsgBox(QMessageBox::Icon type, const QString &text);
    virtual bool gplugHasNode(const QString &id);

    QWidget *greeterWidget() const {
        return m_greeterWidget;
    }

Q_SIGNALS:
    /**
     * Signal emitted in case the authentication through the greeter succeeded.
     **/
    void greeterAccepted();
    /**
     * Signal emitted in case the authentication through the greeter failed.
     */
    void greeterFailed();
    /**
     * Signal emitted when the greeter is ready to perform authentication.
     * E.g. after the timeout of a failed authentication attempt.
     **/
    void greeterReady();
    /**
     * Signal broadcasting any messages from the greeter.
     **/
    void greeterMessage(const QString &text);

public Q_SLOTS:
    /**
     * Invoke to perform an authentication through the greeter plugins.
     **/
    void verify();

    /**
     * Invoke to clear password field
     */
    void clear();

private Q_SLOTS:
    void handleVerify();
    void failedTimer();
private:
    void initialize();
    bool loadGreetPlugin();
    static QVariant getConf(void *ctx, const char *key, const QVariant &dflt);

    // kcheckpass interface
    int Reader(void *buf, int count);
    bool GRead(void *buf, int count);
    bool GWrite(const void *buf, int count);
    bool GSendInt(int val);
    bool GSendStr(const char *buf);
    bool GSendArr(int len, const char *buf);
    bool GRecvInt(int *val);
    bool GRecvArr(char **buf);
    void reapVerify();
    void cantCheck();

    GreeterPluginHandle m_pluginHandle;
    QWidget *m_greeterWidget;
    KGreeterPlugin *m_greet;
    QStringList m_plugins;
    QStringList m_pluginOptions;
    QString m_method;
    bool m_valid;
    // for kcheckpass
    int  m_pid;
    int m_fd;
    QSocketNotifier *m_notifier;
    bool m_failedLock;
};

} // end namespace
#endif
