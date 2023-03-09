/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2019 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "feedback.h"
#include "kcm_feedback_debug.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QFileInfo>
#include <QVector>

#include <KUserFeedbackQt6/FeedbackConfigUiController>
#include <KUserFeedbackQt6/Provider>

#include "feedbackdata.h"
#include "feedbacksettings.h"

K_PLUGIN_FACTORY_WITH_JSON(FeedbackFactory, "kcm_feedback.json", registerPlugin<Feedback>(); registerPlugin<FeedbackData>();)

struct Information {
    QString icon;
    QString kuserfeedbackComponent;
};
static QHash<QString, Information> s_programs = {
    {"plasmashell", {"plasmashell", "plasmashell"}},
    {"plasma-discover", {"plasmadiscover", "discover"}},
};

inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
    QJsonValue temp(v1);
    v1 = QJsonValue(v2);
    v2 = temp;
}

Feedback::Feedback(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickManagedConfigModule(parent, data, args)
    // UserFeedback.conf is used by KUserFeedback which uses QSettings and won't go through globals
    , m_data(new FeedbackData(this))
{
    qmlRegisterAnonymousType<FeedbackSettings>("org.kde.userfeedback.kcm", 1);

    QVector<QProcess *> processes;
    for (const auto &exec : s_programs.keys()) {
        QProcess *p = new QProcess(this);
        p->setProgram(exec);
        p->setArguments({QStringLiteral("--feedback")});
        p->start();
        connect(p, &QProcess::finished, this, &Feedback::programFinished);
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
        qCWarning(KCM_FEEDBACK_DEBUG) << "Could not check" << program;
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
            qCWarning(KCM_FEEDBACK_DEBUG) << "error:" << mode << "is not a valid mode";
            continue;
        }

        const QString description = line.mid(sepIdx + 1);
        m_uses[modeValue][description] << s_programs[program].icon;
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

QJsonArray Feedback::audits() const
{
    QJsonArray ret;
    for (auto it = s_programs.constBegin(); it != s_programs.constEnd(); ++it) {
        QString feedbackLocation =
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + '/' + it->kuserfeedbackComponent + QStringLiteral("/kuserfeedback/audit");

        if (QFileInfo::exists(feedbackLocation)) {
            ret += QJsonObject{
                {"program", it.key()},
                {"audits", feedbackLocation},
            };
        }
    }
    return ret;
}

#include "feedback.moc"
#include "moc_feedback.cpp"
