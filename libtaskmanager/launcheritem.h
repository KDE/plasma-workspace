/*****************************************************************

Copyright 2010 Anton Kreuzkamp <akreuzkamp@web.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/
#ifndef LAUNCHERITEM_H
#define LAUNCHERITEM_H

#include "abstractgroupableitem.h"
#include "taskmanager_export.h"

namespace TaskManager
{

class LauncherItemPrivate;
class GroupManager;

/**
 * An item shown in the taskmanager, in order to use it to launch the application (or file) the launcher is linked to.
 * If the Application is running the launcher gets hidden, in order to not waste space.
 */
class TASKMANAGER_EXPORT LauncherItem : public AbstractGroupableItem
{
    Q_OBJECT
public:
    /**
     * Creates a LauncherItem for a executable
     * @param url the URL to the application or file the launcher gets linked to
     */
    LauncherItem(QObject *parent, const QUrl &url);
    ~LauncherItem() override;

    /**
    * @deprecated: use itemType() instead
    **/
    TASKMANAGER_DEPRECATED bool isGroupItem() const override;
    ItemType itemType() const override;

    bool isValid() const;
    QIcon icon() const override;
    QString name() const override;
    QString genericName() const override;
    QString wmClass() const;

    void setIcon(const QIcon &icon);
    void setName(const QString &name);
    void setGenericName(const QString &genericName);
    void setWmClass(const QString &wmClass);

    // bookkeeping methods for showing/not showing
    /** Return true if this is a *new* association */
    bool associateItemIfMatches(AbstractGroupableItem *item);
    bool isAssociated(AbstractGroupableItem *item) const;
    void removeItemIfAssociated(AbstractGroupableItem *item);
    bool shouldShow(const GroupManager *manager) const;

    //reimplemented pure virtual methods from abstractgroupableitem
    bool isOnCurrentDesktop() const override;
    bool isOnAllDesktops() const override;
    int desktop() const override;
    bool isShaded() const override;
    bool isMaximized() const override;
    bool isMinimized() const override;
    bool isFullScreen() const override;
    bool isKeptBelowOthers() const override;
    bool isAlwaysOnTop() const override;
    bool isActionSupported(NET::Action) const override;
    bool isActive() const override;
    bool demandsAttention() const override;
    void addMimeData(QMimeData *mimeData) const override;
    QUrl launcherUrl() const override;
    void setLauncherUrl(const QUrl &url);

    //preferred applications hack
    static QString defaultApplication(const QUrl &url);

public Q_SLOTS:
    void toDesktop(int) override;

    void setShaded(bool) override;
    void toggleShaded() override;

    void setMaximized(bool) override;
    void toggleMaximized() override;

    void setMinimized(bool) override;
    void toggleMinimized() override;

    void setFullScreen(bool) override;
    void toggleFullScreen() override;

    void setKeptBelowOthers(bool) override;
    void toggleKeptBelowOthers() override;

    void setAlwaysOnTop(bool) override;
    void toggleAlwaysOnTop() override;

    void close() override;

    void launch();

Q_SIGNALS:
    void associationChanged();

private:
    friend class LauncherItemPrivate;
    LauncherItemPrivate * const d;

    Q_PRIVATE_SLOT(d, void associateDestroyed(QObject *obj))
};

} // TaskManager namespace

#endif
