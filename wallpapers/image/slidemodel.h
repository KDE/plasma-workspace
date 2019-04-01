#ifndef SLIDEMODEL_H
#define SLIDEMODEL_H

#include "backgroundlistmodel.h"

class SlideModel : public BackgroundListModel
{
    Q_OBJECT
public:
    using BackgroundListModel::BackgroundListModel;
    void reload(const QStringList &selected);
    void addDirs(const QStringList &selected);
    void removeDir(const QString &selected);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
private Q_SLOTS:
    void removeBackgrounds(const QStringList &paths, const QString &token);
    void backgroundsFound(const QStringList &paths, const QString &token);
};

#endif
