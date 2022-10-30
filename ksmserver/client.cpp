/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 2005 Lubos Lunak <l.lunak@kde.org>

    SPDX-FileContributor: Oswald Buddenhagen <ob6@inf.tu-dresden.de>

    some code taken from the dcopserver (part of the KDE libraries), which is
    SPDX-FileCopyrightText: 1999 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 1999 Preston Brown <pbrown@kde.org>

    SPDX-License-Identifier: MIT
*/

#include <config-workspace.h>

#include "client.h"

#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <time.h>

#include <QRandomGenerator>

extern KSMServer *the_server;

KSMClient::KSMClient(SmsConn conn)
    : smsConn(conn)
{
    id = nullptr;
    resetState();
}

KSMClient::~KSMClient()
{
    foreach (SmProp *prop, properties)
        SmFreeProperty(prop);
    if (id)
        free((void *)id);
}

SmProp *KSMClient::property(const char *name) const
{
    foreach (SmProp *prop, properties) {
        if (!qstrcmp(prop->name, name))
            return prop;
    }
    return nullptr;
}

void KSMClient::resetState()
{
    saveYourselfDone = false;
    pendingInteraction = false;
    waitForPhase2 = false;
    wasPhase2 = false;
}

/*
 * This fakes SmsGenerateClientID() in case we can't read our own hostname.
 * In this case SmsGenerateClientID() returns NULL, but we really want a
 * client ID, so we fake one.
 */
Q_GLOBAL_STATIC(QString, my_addr)
char *safeSmsGenerateClientID(SmsConn /*c*/)
{
    //  Causes delays with misconfigured network :-/.
    //    char *ret = SmsGenerateClientID(c);
    char *ret = nullptr;
    if (!ret) {
        if (my_addr->isEmpty()) {
            //           qCWarning(KSMSERVER, "Can't get own host name. Your system is severely misconfigured\n");

            /* Faking our IP address, the 0 below is "unknown" address format
               (1 would be IP, 2 would be DEC-NET format) */
            char hostname[256];
            if (gethostname(hostname, 255) != 0)
                *my_addr = QStringLiteral("0%1").arg(QRandomGenerator::global()->generate(), 8, 16);
            else {
                // create some kind of hash for the hostname
                int addr[4] = {0, 0, 0, 0};
                int pos = 0;
                for (unsigned int i = 0; i < strlen(hostname); ++i, ++pos)
                    addr[pos % 4] += hostname[i];
                *my_addr = QStringLiteral("0");
                for (int i = 0; i < 4; ++i)
                    *my_addr += QString::number(addr[i], 16);
            }
        }
        /* Needs to be malloc(), to look the same as libSM */
        ret = (char *)malloc(1 + my_addr->length() + 13 + 10 + 4 + 1 + /*safeness*/ 10);
        static int sequence = 0;

        if (ret == nullptr)
            return nullptr;

        sprintf(ret, "1%s%.13ld%.10d%.4d", my_addr->toLatin1().constData(), (long)time(nullptr), getpid(), sequence);
        sequence = (sequence + 1) % 10000;
    }
    return ret;
}

void KSMClient::registerClient(const char *previousId)
{
    id = previousId;
    if (!id)
        id = safeSmsGenerateClientID(smsConn);
    SmsRegisterClientReply(smsConn, (char *)id);
    SmsSaveYourself(smsConn, SmSaveLocal, false, SmInteractStyleNone, false);
    SmsSaveComplete(smsConn);
    the_server->clientRegistered(previousId);
}

QString KSMClient::program() const
{
    SmProp *p = property(SmProgram);
    if (!p || qstrcmp(p->type, SmARRAY8) || p->num_vals < 1)
        return QString();
    return QLatin1String((const char *)p->vals[0].value);
}

QStringList KSMClient::restartCommand() const
{
    QStringList result;
    SmProp *p = property(SmRestartCommand);
    if (!p || qstrcmp(p->type, SmLISTofARRAY8) || p->num_vals < 1)
        return result;
    for (int i = 0; i < p->num_vals; i++)
        result += QLatin1String((const char *)p->vals[i].value);
    return result;
}

QStringList KSMClient::discardCommand() const
{
    QStringList result;
    SmProp *p = property(SmDiscardCommand);
    if (!p || qstrcmp(p->type, SmLISTofARRAY8) || p->num_vals < 1)
        return result;
    for (int i = 0; i < p->num_vals; i++)
        result += QLatin1String((const char *)p->vals[i].value);
    return result;
}

int KSMClient::restartStyleHint() const
{
    SmProp *p = property(SmRestartStyleHint);
    if (!p || qstrcmp(p->type, SmCARD8) || p->num_vals < 1)
        return SmRestartIfRunning;
    return *((unsigned char *)p->vals[0].value);
}

QString KSMClient::userId() const
{
    SmProp *p = property(SmUserID);
    if (!p || qstrcmp(p->type, SmARRAY8) || p->num_vals < 1)
        return QString();
    return QLatin1String((const char *)p->vals[0].value);
}
