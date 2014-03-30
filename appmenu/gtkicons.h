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
        insert(QString("gnome-fs-directory"), QString("folder.png"));
        insert(QString("gnome-fs-regular.png"), QString("application-x-zerosize.png"));
        insert(QString("gtk-about"), QString("help-about.png"));
        insert(QString("gtk-add"), QString("list-add.png"));
        insert(QString("gtk-apply"), QString("dialog-ok-apply.png ok-apply.png apply.png"));
        insert(QString("gtk-bold"), QString("format-text-bold.png"));
        insert(QString("gtk-cancel"), QString("dialog-cancel.png cancel.png"));
        insert(QString("gtk-cdrom"), QString("media-optical.png"));
        insert(QString("gtk-clear"), QString("edit-clear.png"));
        insert(QString("gtk-close"), QString("window-close.png"));
        insert(QString("gtk-color-picker"), QString("color-picker.png"));
        insert(QString("gtk-connect"), QString("network-connect.png"));
        insert(QString("gtk-convert"), QString("document-export.png"));
        insert(QString("gtk-copy"), QString("edit-copy.png"));
        insert(QString("gtk-cut"), QString("edit-cut.png"));
        insert(QString("gtk-delete"), QString("edit-delete.png"));
        insert(QString("gtk-dialog-authentication"), QString("dialog-password.png document-encrypt.png object-locked.png"));
        insert(QString("gtk-dialog-error"), QString("dialog-error.png"));
        insert(QString("gtk-dialog-info"), QString("dialog-information.png"));
        insert(QString("gtk-dialog-question"), QString("dialog-information.png"));
        insert(QString("gtk-dialog-warning"), QString("dialog-warning.png"));
        insert(QString("gtk-directory"), QString("folder.png"));
        insert(QString("gtk-disconnect"), QString("network-disconnect.png"));
        insert(QString("gtk-dnd"), QString("application-x-zerosize.png"));
        insert(QString("gtk-dnd-multiple"), QString("document-multiple.png"));
        insert(QString("gtk-edit"), QString("document-properties.png"));
        insert(QString("gtk-execute"), QString("fork.png"));
        insert(QString("gtk-file"), QString("application-x-zerosize.png"));
        insert(QString("gtk-find"), QString("edit-find.png"));
        insert(QString("gtk-find-and-replace"), QString("edit-find-replace.png"));
        insert(QString("gtk-floppy"), QString("media-floppy.png"));
        insert(QString("gtk-fullscreen"), QString("view-fullscreen.png"));
        insert(QString("gtk-goto-bottom"), QString("go-bottom.png"));
        insert(QString("gtk-goto-first"), QString("go-first.png"));
        insert(QString("gtk-goto-last"), QString("go-last.png"));
        insert(QString("gtk-goto-top"), QString("go-top.png"));
        insert(QString("gtk-go-back"), QString("go-previous.png"));
        insert(QString("gtk-go-back-ltr"), QString("go-previous.png"));
        insert(QString("gtk-go-back-rtl"), QString("go-next.png"));
        insert(QString("gtk-go-down"), QString("go-down.png"));
        insert(QString("gtk-go-forward"), QString("go-next.png"));
        insert(QString("gtk-go-forward-ltr"), QString("go-next.png"));
        insert(QString("gtk-go-forward-rtl"), QString("go-previous.png"));
        insert(QString("gtk-go-up"), QString("go-up.png"));
        insert(QString("gtk-harddisk"), QString("drive-harddisk.png"));
        insert(QString("gtk-help"), QString("help-contents.png"));
        insert(QString("gtk-home"), QString("go-home.png"));
        insert(QString("gtk-indent"), QString("format-indent-more.png"));
        insert(QString("gtk-index"), QString("help-contents.png"));
        insert(QString("gtk-info"), QString("help-about.png"));
        insert(QString("gtk-italic"), QString("format-text-italic.png"));
        insert(QString("gtk-jump-to"), QString("go-jump.png"));
        insert(QString("gtk-justify-center"), QString("format-justify-center.png"));
        insert(QString("gtk-justify-fill"), QString("format-justify-fill.png"));
        insert(QString("gtk-justify-left"), QString("format-justify-left.png"));
        insert(QString("gtk-justify-right"), QString("format-justify-right.png"));
        insert(QString("gtk-leave-fullscreen"), QString("view-restore.png"));
        insert(QString("gtk-media-forward"), QString("media-seek-forward.png"));
        insert(QString("gtk-media-next"), QString("media-skip-forward.png"));
        insert(QString("gtk-media-pause"), QString("media-playback-pause.png"));
        insert(QString("gtk-media-play"), QString("media-playback-start.png"));
        insert(QString("gtk-media-previous"), QString("media-skip-backward.png"));
        insert(QString("gtk-media-record"), QString("media-record.png"));
        insert(QString("gtk-media-rewind"), QString("media-seek-backward.png"));
        insert(QString("gtk-media-stop"), QString("media-playback-stop.png"));
        insert(QString("gtk-missing-image"), QString("unknown.png"));
        insert(QString("gtk-network"), QString("network-server.png"));
        insert(QString("gtk-new"), QString("document-new.png"));
        insert(QString("gtk-no"), QString("edit-delete.png"));
        insert(QString("gtk-ok"), QString("dialog-ok.png ok.png"));
        insert(QString("gtk-open"), QString("document-open.png"));
        insert(QString("gtk-paste"), QString("edit-paste.png"));
        insert(QString("gtk-preferences"), QString("configure.png"));
        insert(QString("gtk-print"), QString("document-print.png"));
        insert(QString("gtk-print-preview"), QString("document-print-preview.png"));
        insert(QString("gtk-properties"), QString("document-properties.png"));
        insert(QString("gtk-quit"), QString("application-exit.png"));
        insert(QString("gtk-redo"), QString("edit-redo.png"));
        insert(QString("gtk-refresh"), QString("view-refresh.png"));
        insert(QString("gtk-remove"), QString("edit-delete.png"));
        insert(QString("gtk-revert-to-saved"), QString("document-revert.png"));
        insert(QString("gtk-save"), QString("document-save.png"));
        insert(QString("gtk-save-as"), QString("document-save-as.png"));
        insert(QString("gtk-select-all"), QString("edit-select-all.png"));
        insert(QString("gtk-select-color"), QString("color-picker.png"));
        insert(QString("gtk-select-font"), QString("preferences-desktop-font.png"));
        insert(QString("gtk-sort-ascending"), QString("view-sort-ascending.png"));
        insert(QString("gtk-sort-descending"), QString("view-sort-descending.png"));
        insert(QString("gtk-spell-check"), QString("tools-check-spelling.png"));
        insert(QString("gtk-stop"), QString("process-stop.png"));
        insert(QString("gtk-strikethrough"), QString("format-text-strikethrough.png"));
        insert(QString("gtk-undelete"), QString("edit-undo.png"));
        insert(QString("gtk-underline"), QString("format-text-underline.png"));
        insert(QString("gtk-undo"), QString("edit-undo.png"));
        insert(QString("gtk-unindent"), QString("format-indent-less.png"));
        insert(QString("gtk-yes"), QString("dialog-ok.png ok.png"));
        insert(QString("gtk-zoom-100"), QString("zoom-original.png"));
        insert(QString("gtk-zoom-fit"), QString("zoom-fit-best.png"));
        insert(QString("gtk-zoom-in"), QString("zoom-in.png"));
        insert(QString("gtk-zoom-out"), QString("zoom-out.png"));
        insert(QString("stock_edit-bookmark"), QString("bookmarks-organize.png"));
        insert(QString("gimp-edit"), QString("edit.png"));
        insert(QString("gimp-info"), QString("dialog-information.png"));
        insert(QString("gimp-reset"), QString("reload.png"));
        insert(QString("gimp-warning"), QString("dialog-warning.png"));
        insert(QString("gimp-tool-options"), QString("tool.png"));
        insert(QString("gimp-images"), QString("image.png"));
    }
};

#endif // GTKICONS_H