/*
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "timezonemodel.h"
#include "timezonesi18n.h"

#include <KLocalizedString>
#include <QDBusConnection>
#include <QStringMatcher>
#include <QTimeZone>

#include <unicode/localebuilder.h>
#include <unicode/tznames.h>
#include <unicode/unistr.h>

TimeZoneFilterProxy::TimeZoneFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_stringMatcher.setCaseSensitivity(Qt::CaseInsensitive);
}

bool TimeZoneFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!sourceModel() || (m_filterString.isEmpty() && !m_onlyShowChecked)) {
        return true;
    }

    const bool checked = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::CheckedRole).toBool();
    if (m_onlyShowChecked && !checked) {
        return false;
    }

    const QString city = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::CityRole).toString();
    const QString region = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::RegionRole).toString();
    const QString comment = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::CommentRole).toString();

    if (m_stringMatcher.indexIn(city) != -1 || m_stringMatcher.indexIn(region) != -1 || m_stringMatcher.indexIn(comment) != -1) {
        return true;
    }

    return false;
}

void TimeZoneFilterProxy::setFilterString(const QString &filterString)
{
    m_filterString = filterString;
    m_stringMatcher.setPattern(filterString);
    Q_EMIT filterStringChanged();
    invalidateFilter();
}

void TimeZoneFilterProxy::setOnlyShowChecked(const bool show)
{
    if (m_onlyShowChecked == show) {
        return;
    }
    m_onlyShowChecked = show;
    Q_EMIT onlyShowCheckedChanged();
}

//=============================================================================

TimeZoneModel::TimeZoneModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_timezonesI18n(new TimezonesI18n(this))
{
    update();

    QDBusConnection::sessionBus().connect(QString(),
                                          QStringLiteral("/org/kde/kcmshell_clock"), //
                                          QStringLiteral("org.kde.kcmshell_clock"),
                                          QStringLiteral("clockUpdated"),
                                          this,
                                          SLOT(slotUpdate()));
}

TimeZoneModel::~TimeZoneModel()
{
}

int TimeZoneModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count();
}

QVariant TimeZoneModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        TimeZoneData currentData = m_data.at(index.row());

        switch (role) {
        case TimeZoneIdRole:
            return currentData.id;
        case RegionRole:
            return currentData.region;
        case CityRole:
            return currentData.city;
        case CommentRole:
            return currentData.comment;
        case CheckedRole:
            return currentData.checked;
        case IsLocalTimeZoneRole:
            return currentData.isLocalTimeZone;
        }
    }

    return QVariant();
}

bool TimeZoneModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || value.isNull()) {
        return false;
    }

    if (role == CheckedRole) {
        m_data[index.row()].checked = value.toBool();
        Q_EMIT dataChanged(index, index);

        if (m_data[index.row()].checked) {
            m_selectedTimeZones.append(m_data[index.row()].id);
            m_offsetData.insert(m_data[index.row()].id, m_data[index.row()].offsetFromUtc);
        } else {
            m_selectedTimeZones.removeAll(m_data[index.row()].id);
            m_offsetData.remove(m_data[index.row()].id);
        }

        sortTimeZones();

        Q_EMIT selectedTimeZonesChanged();
        return true;
    }

    return false;
}

void TimeZoneModel::update()
{
    const auto locale = icu::Locale(QLocale::system().name().toLatin1());
    UErrorCode error = U_ZERO_ERROR;
    const std::unique_ptr<icu::TimeZoneNames> tzNames(icu::TimeZoneNames::createInstance(locale, error));
    if (!U_SUCCESS(error)) {
        qWarning() << "failed to create timezone names" << u_errorName(error);
        return;
    }
    icu::UnicodeString result;

    beginResetModel();
    m_data.clear();

    QTimeZone localZone = QTimeZone(QTimeZone::systemTimeZoneId());
    const icu::UnicodeString &localCity =
        tzNames->getExemplarLocationName(icu::UnicodeString::fromUTF8(icu::StringPiece(localZone.id().data(), localZone.id().size())), result);
    TimeZoneData local;
    local.isLocalTimeZone = true;
    local.id = QStringLiteral("Local");
    local.region = i18nc("This means \"Local Timezone\"", "Local");
    local.city = localCity.isBogus() ? QString() : QString::fromUtf16(localCity.getBuffer(), localCity.length());
    local.comment = i18n("System's local time zone");
    local.checked = false;

    m_data.append(local);

    const QList<QByteArray> systemTimeZones = QTimeZone::availableTimeZoneIds();

    for (const auto &id : systemTimeZones) {
        const QTimeZone zone(id);

        const icu::UnicodeString &exemplarCity = tzNames->getExemplarLocationName(icu::UnicodeString::fromUTF8(icu::StringPiece(id.data(), id.size())), result);
        const QString continent = m_timezonesI18n->i18nContinents(QString::fromUtf8(QByteArrayView(id).mid(0, id.indexOf('/'))));

        TimeZoneData newData;
        newData.isLocalTimeZone = false;
        newData.id = id;
        newData.city = exemplarCity.isBogus() ? zone.id() : QString::fromUtf16(exemplarCity.getBuffer(), exemplarCity.length());
        newData.region = zone.country() == QLocale::AnyCountry ? QString() : continent + QLatin1Char('/') + m_timezonesI18n->i18nCountry(zone.country());
        newData.comment = zone.comment();
        newData.checked = false;
        newData.offsetFromUtc = zone.offsetFromUtc(QDateTime::currentDateTimeUtc());
        m_data.append(newData);
    }

    std::sort(m_data.begin() + 1, m_data.end(), [](const TimeZoneData &left, const TimeZoneData &right) {
        return left.city.compare(right.city) < 0;
    });

    endResetModel();
}

void TimeZoneModel::setSelectedTimeZones(const QStringList &selectedTimeZones)
{
    m_selectedTimeZones = selectedTimeZones;
    for (int i = 0; i < m_data.size(); i++) {
        if (m_selectedTimeZones.contains(m_data.at(i).id)) {
            m_data[i].checked = true;
            m_offsetData.insert(m_data[i].id, m_data[i].offsetFromUtc);

            QModelIndex index = createIndex(i, 0);
            Q_EMIT dataChanged(index, index);
        }
    }

    sortTimeZones();
}

void TimeZoneModel::selectLocalTimeZone()
{
    m_data[0].checked = true;
    Q_EMIT dataChanged(index(0, 0), index(0, 0), QList<int>{CheckedRole});
}

QString TimeZoneModel::localTimeZoneCity()
{
    return m_data[0].city;
}

void TimeZoneModel::slotUpdate()
{
    update();
    setProperty("selectedTimeZones", m_selectedTimeZones);
}

QHash<int, QByteArray> TimeZoneModel::roleNames() const
{
    return QHash<int, QByteArray>({
        {TimeZoneIdRole, "timeZoneId"},
        {RegionRole, "region"},
        {CityRole, "city"},
        {CommentRole, "comment"},
        {CheckedRole, "checked"},
        {IsLocalTimeZoneRole, "isLocalTimeZone"},
    });
}

void TimeZoneModel::sortTimeZones()
{
    std::sort(m_selectedTimeZones.begin(), m_selectedTimeZones.end(), [this](const QString &a, const QString &b) {
        return m_offsetData.value(a) < m_offsetData.value(b);
    });
}
