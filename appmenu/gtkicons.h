/*
  This file is part of the KDE project.

  Copyright (c) 2011 Lionel Chauvin <megabigbug@yahoo.fr>
  Copyright (c) 2011,2012 Cédric Bellegarde <gnumdk@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
*/

#ifndef GTKICONS_H
#define GTKICONS_H

#include <QtCore/QMap>

class GtkIcons : public QMap<QString, QString>
{
    public:
    GtkIcons( void ) : QMap<QString, QString> ()
    {
        insert(QStringLiteral("gnome-fs-directory"), QStringLiteral("folder.png"));
        insert(QStringLiteral("gnome-fs-regular.png"), QStringLiteral("application-x-zerosize.png"));
        insert(QStringLiteral("gtk-about"), QStringLiteral("help-about.png"));
        insert(QStringLiteral("gtk-add"), QStringLiteral("list-add.png"));
        insert(QStringLiteral("gtk-apply"), QStringLiteral("dialog-ok-apply.png ok-apply.png apply.png"));
        insert(QStringLiteral("gtk-bold"), QStringLiteral("format-text-bold.png"));
        insert(QStringLiteral("gtk-cancel"), QStringLiteral("dialog-cancel.png cancel.png"));
        insert(QStringLiteral("gtk-cdrom"), QStringLiteral("media-optical.png"));
        insert(QStringLiteral("gtk-clear"), QStringLiteral("edit-clear.png"));
        insert(QStringLiteral("gtk-close"), QStringLiteral("window-close.png"));
        insert(QStringLiteral("gtk-color-picker"), QStringLiteral("color-picker.png"));
        insert(QStringLiteral("gtk-connect"), QStringLiteral("network-connect.png"));
        insert(QStringLiteral("gtk-convert"), QStringLiteral("document-export.png"));
        insert(QStringLiteral("gtk-copy"), QStringLiteral("edit-copy.png"));
        insert(QStringLiteral("gtk-cut"), QStringLiteral("edit-cut.png"));
        insert(QStringLiteral("gtk-delete"), QStringLiteral("edit-delete.png"));
        insert(QStringLiteral("gtk-dialog-authentication"), QStringLiteral("dialog-password.png document-encrypt.png object-locked.png"));
        insert(QStringLiteral("gtk-dialog-error"), QStringLiteral("dialog-error.png"));
        insert(QStringLiteral("gtk-dialog-info"), QStringLiteral("dialog-information.png"));
        insert(QStringLiteral("gtk-dialog-question"), QStringLiteral("dialog-information.png"));
        insert(QStringLiteral("gtk-dialog-warning"), QStringLiteral("dialog-warning.png"));
        insert(QStringLiteral("gtk-directory"), QStringLiteral("folder.png"));
        insert(QStringLiteral("gtk-disconnect"), QStringLiteral("network-disconnect.png"));
        insert(QStringLiteral("gtk-dnd"), QStringLiteral("application-x-zerosize.png"));
        insert(QStringLiteral("gtk-dnd-multiple"), QStringLiteral("document-multiple.png"));
        insert(QStringLiteral("gtk-edit"), QStringLiteral("document-properties.png"));
        insert(QStringLiteral("gtk-execute"), QStringLiteral("fork.png"));
        insert(QStringLiteral("gtk-file"), QStringLiteral("application-x-zerosize.png"));
        insert(QStringLiteral("gtk-find"), QStringLiteral("edit-find.png"));
        insert(QStringLiteral("gtk-find-and-replace"), QStringLiteral("edit-find-replace.png"));
        insert(QStringLiteral("gtk-floppy"), QStringLiteral("media-floppy.png"));
        insert(QStringLiteral("gtk-fullscreen"), QStringLiteral("view-fullscreen.png"));
        insert(QStringLiteral("gtk-goto-bottom"), QStringLiteral("go-bottom.png"));
        insert(QStringLiteral("gtk-goto-first"), QStringLiteral("go-first.png"));
        insert(QStringLiteral("gtk-goto-last"), QStringLiteral("go-last.png"));
        insert(QStringLiteral("gtk-goto-top"), QStringLiteral("go-top.png"));
        insert(QStringLiteral("gtk-go-back"), QStringLiteral("go-previous.png"));
        insert(QStringLiteral("gtk-go-back-ltr"), QStringLiteral("go-previous.png"));
        insert(QStringLiteral("gtk-go-back-rtl"), QStringLiteral("go-next.png"));
        insert(QStringLiteral("gtk-go-down"), QStringLiteral("go-down.png"));
        insert(QStringLiteral("gtk-go-forward"), QStringLiteral("go-next.png"));
        insert(QStringLiteral("gtk-go-forward-ltr"), QStringLiteral("go-next.png"));
        insert(QStringLiteral("gtk-go-forward-rtl"), QStringLiteral("go-previous.png"));
        insert(QStringLiteral("gtk-go-up"), QStringLiteral("go-up.png"));
        insert(QStringLiteral("gtk-harddisk"), QStringLiteral("drive-harddisk.png"));
        insert(QStringLiteral("gtk-help"), QStringLiteral("help-contents.png"));
        insert(QStringLiteral("gtk-home"), QStringLiteral("go-home.png"));
        insert(QStringLiteral("gtk-indent"), QStringLiteral("format-indent-more.png"));
        insert(QStringLiteral("gtk-index"), QStringLiteral("help-contents.png"));
        insert(QStringLiteral("gtk-info"), QStringLiteral("help-about.png"));
        insert(QStringLiteral("gtk-italic"), QStringLiteral("format-text-italic.png"));
        insert(QStringLiteral("gtk-jump-to"), QStringLiteral("go-jump.png"));
        insert(QStringLiteral("gtk-justify-center"), QStringLiteral("format-justify-center.png"));
        insert(QStringLiteral("gtk-justify-fill"), QStringLiteral("format-justify-fill.png"));
        insert(QStringLiteral("gtk-justify-left"), QStringLiteral("format-justify-left.png"));
        insert(QStringLiteral("gtk-justify-right"), QStringLiteral("format-justify-right.png"));
        insert(QStringLiteral("gtk-leave-fullscreen"), QStringLiteral("view-restore.png"));
        insert(QStringLiteral("gtk-media-forward"), QStringLiteral("media-seek-forward.png"));
        insert(QStringLiteral("gtk-media-next"), QStringLiteral("media-skip-forward.png"));
        insert(QStringLiteral("gtk-media-pause"), QStringLiteral("media-playback-pause.png"));
        insert(QStringLiteral("gtk-media-play"), QStringLiteral("media-playback-start.png"));
        insert(QStringLiteral("gtk-media-previous"), QStringLiteral("media-skip-backward.png"));
        insert(QStringLiteral("gtk-media-record"), QStringLiteral("media-record.png"));
        insert(QStringLiteral("gtk-media-rewind"), QStringLiteral("media-seek-backward.png"));
        insert(QStringLiteral("gtk-media-stop"), QStringLiteral("media-playback-stop.png"));
        insert(QStringLiteral("gtk-missing-image"), QStringLiteral("unknown.png"));
        insert(QStringLiteral("gtk-network"), QStringLiteral("network-server.png"));
        insert(QStringLiteral("gtk-new"), QStringLiteral("document-new.png"));
        insert(QStringLiteral("gtk-no"), QStringLiteral("edit-delete.png"));
        insert(QStringLiteral("gtk-ok"), QStringLiteral("dialog-ok.png ok.png"));
        insert(QStringLiteral("gtk-open"), QStringLiteral("document-open.png"));
        insert(QStringLiteral("gtk-paste"), QStringLiteral("edit-paste.png"));
        insert(QStringLiteral("gtk-preferences"), QStringLiteral("configure.png"));
        insert(QStringLiteral("gtk-print"), QStringLiteral("document-print.png"));
        insert(QStringLiteral("gtk-print-preview"), QStringLiteral("document-print-preview.png"));
        insert(QStringLiteral("gtk-properties"), QStringLiteral("document-properties.png"));
        insert(QStringLiteral("gtk-quit"), QStringLiteral("application-exit.png"));
        insert(QStringLiteral("gtk-redo"), QStringLiteral("edit-redo.png"));
        insert(QStringLiteral("gtk-refresh"), QStringLiteral("view-refresh.png"));
        insert(QStringLiteral("gtk-remove"), QStringLiteral("edit-delete.png"));
        insert(QStringLiteral("gtk-revert-to-saved"), QStringLiteral("document-revert.png"));
        insert(QStringLiteral("gtk-save"), QStringLiteral("document-save.png"));
        insert(QStringLiteral("gtk-save-as"), QStringLiteral("document-save-as.png"));
        insert(QStringLiteral("gtk-select-all"), QStringLiteral("edit-select-all.png"));
        insert(QStringLiteral("gtk-select-color"), QStringLiteral("color-picker.png"));
        insert(QStringLiteral("gtk-select-font"), QStringLiteral("preferences-desktop-font.png"));
        insert(QStringLiteral("gtk-sort-ascending"), QStringLiteral("view-sort-ascending.png"));
        insert(QStringLiteral("gtk-sort-descending"), QStringLiteral("view-sort-descending.png"));
        insert(QStringLiteral("gtk-spell-check"), QStringLiteral("tools-check-spelling.png"));
        insert(QStringLiteral("gtk-stop"), QStringLiteral("process-stop.png"));
        insert(QStringLiteral("gtk-strikethrough"), QStringLiteral("format-text-strikethrough.png"));
        insert(QStringLiteral("gtk-undelete"), QStringLiteral("edit-undo.png"));
        insert(QStringLiteral("gtk-underline"), QStringLiteral("format-text-underline.png"));
        insert(QStringLiteral("gtk-undo"), QStringLiteral("edit-undo.png"));
        insert(QStringLiteral("gtk-unindent"), QStringLiteral("format-indent-less.png"));
        insert(QStringLiteral("gtk-yes"), QStringLiteral("dialog-ok.png ok.png"));
        insert(QStringLiteral("gtk-zoom-100"), QStringLiteral("zoom-original.png"));
        insert(QStringLiteral("gtk-zoom-fit"), QStringLiteral("zoom-fit-best.png"));
        insert(QStringLiteral("gtk-zoom-in"), QStringLiteral("zoom-in.png"));
        insert(QStringLiteral("gtk-zoom-out"), QStringLiteral("zoom-out.png"));
        insert(QStringLiteral("stock_edit-bookmark"), QStringLiteral("bookmarks-organize.png"));
        insert(QStringLiteral("gimp-edit"), QStringLiteral("edit.png"));
        insert(QStringLiteral("gimp-info"), QStringLiteral("dialog-information.png"));
        insert(QStringLiteral("gimp-reset"), QStringLiteral("reload.png"));
        insert(QStringLiteral("gimp-warning"), QStringLiteral("dialog-warning.png"));
        insert(QStringLiteral("gimp-tool-options"), QStringLiteral("tool.png"));
        insert(QStringLiteral("gimp-images"), QStringLiteral("image.png"));
    }
};

#endif // GTKICONS_H