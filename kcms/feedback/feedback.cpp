/*
 * Copyright (C) 2019 David Edmundson <davidedmundson@kde.org>
 * Copyright (C) 2019 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "feedback.h"

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <QVector>

#include <KUserFeedback/FeedbackConfigUiController>
#include <KUserFeedback/Provider>

#include "feedbackdata.h"
#include "feedbacksettings.h"

K_PLUGIN_FACTORY_WITH_JSON(FeedbackFactory, "kcm_feedback.json", registerPlugin<Feedback>(); registerPlugin<FeedbackData>();)

// Program to icon hash
static QHash<QString, QString> s_programs = {{"plasmashell", "plasmashell"}, {"plasma-discover", "plasmadiscover"}};

inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

Feedback::Feedback(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent)
    // UserFeedback.conf is used by KUserFeedback which uses QSettings and won't go through globals
    , m_data(new FeedbackData(this))
{
    Q_UNUSED(args)

    qmlRegisterType<FeedbackSettings>();

    setAboutData(new KAboutData(QStringLiteral("kcm_feedback"),
                                i18n("User Feedback"),
                                QStringLiteral("1.0"),
                                i18n("Configure user feedback settings"),
                                KAboutLicense::LGPL));

    QVector<QProcess *> processes;
    for (const auto &exec : s_programs.keys()) {
        QProcess *p = new QProcess(this);
        p->setProgram(exec);
        p->setArguments({QStringLiteral("--feedback")});
        p->start();
        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Feedback::programFinished);
        processes << p;
    }
}

Feedback::~Feedback() = default;

void Feedback::programFinished(int exitCode)
{
    auto mo = KUserFeedback::Provider::staticMetaObject;
    const int modeEnumIdx = mo.indexOfEnumerator("TelemetryMode");
    Q_ASSERT(modeEnumIdx >= 0);
    const auto modeEnum = mo.enumerator(modeEnumIdx);

    QProcess *p = qobject_cast<QProcess *>(sender());
    const QString program = p->program();

    if (exitCode) {
        qWarning() << "Could not check" << program;
        return;
    }

    QTextStream stream(p);
    for (QString line; stream.readLineInto(&line);) {
        int sepIdx = line.indexOf(QLatin1String(": "));
        if (sepIdx < 0) {
            break;
        }

        const QString mode = line.left(sepIdx);
        bool ok;
        const int modeValue = modeEnum.keyToValue(qPrintable(mode), &ok);
        if (!ok) {
            qWarning() << "error:" << mode << "is not a valid mode";
            continue;
        }

        const QString description = line.mid(sepIdx + 1);
        m_uses[modeValue][description] << s_programs[program];
    }
    p->deleteLater();
    m_feedbackSources = {};
    for (auto it = m_uses.constBegin(), itEnd = m_uses.constEnd(); it != itEnd; ++it) {
        const auto modeUses = *it;
        for (auto itMode = modeUses.constBegin(), itModeEnd = modeUses.constEnd(); itMode != itModeEnd; ++itMode) {
            m_feedbackSources << QJsonObject({{"mode", it.key()}, {"icons", *itMode}, {"description", itMode.key()}});
        }
    }
    std::sort(m_feedbackSources.begin(), m_feedbackSources.end(), [](const QJsonValue &valueL, const QJsonValue &valueR) {
        const QJsonObject objL(valueL.toObject()), objR(valueR.toObject());
        const auto modeL = objL["mode"].toInt(), modeR = objR["mode"].toInt();
        return modeL < modeR || (modeL == modeR && objL["description"].toString() < objR["description"].toString());
    });
    Q_EMIT feedbackSourcesChanged();
}

bool Feedback::feedbackEnabled() const
{
    KUserFeedback::Provider p;
    return p.isEnabled();
}

FeedbackSettings *Feedback::feedbackSettings() const
{
    return m_data->settings();
}

#include "feedback.moc"
