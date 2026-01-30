/*
 * Copyright © 2002 Keith Packard
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _DEFAULT_SOURCE
#include "xcursor.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * From libXcursor/include/X11/extensions/Xcursor.h
 */

#define XcursorTrue 1
#define XcursorFalse 0

/*
 * Cursor files start with a header.  The header
 * contains a magic number, a version number and a
 * table of contents which has type and offset information
 * for the remaining tables in the file.
 *
 * File minor versions increment for compatible changes
 * File major versions increment for incompatible changes (never, we hope)
 *
 * Chunks of the same type are always upward compatible.  Incompatible
 * changes are made with new chunk types; the old data can remain under
 * the old type.  Upward compatible changes can add header data as the
 * header lengths are specified in the file.
 *
 *  File:
 *	FileHeader
 *	LISTofChunk
 *
 *  FileHeader:
 *	CARD32		magic	    magic number
 *	CARD32		header	    bytes in file header
 *	CARD32		version	    file version
 *	CARD32		ntoc	    number of toc entries
 *	LISTofFileToc   toc	    table of contents
 *
 *  FileToc:
 *	CARD32		type	    entry type
 *	CARD32		subtype	    entry subtype (size for images)
 *	CARD32		position    absolute file position
 */

#define XCURSOR_MAGIC 0x72756358 /* "Xcur" LSBFirst */

/*
 * Current Xcursor version number.  Will be substituted by configure
 * from the version in the libXcursor configure.ac file.
 */

#define XCURSOR_LIB_MAJOR 1
#define XCURSOR_LIB_MINOR 1
#define XCURSOR_LIB_REVISION 13
#define XCURSOR_LIB_VERSION ((XCURSOR_LIB_MAJOR * 10000) + (XCURSOR_LIB_MINOR * 100) + (XCURSOR_LIB_REVISION))

/*
 * This version number is stored in cursor files; changes to the
 * file format require updating this version number
 */
#define XCURSOR_FILE_MAJOR 1
#define XCURSOR_FILE_MINOR 0
#define XCURSOR_FILE_VERSION ((XCURSOR_FILE_MAJOR << 16) | (XCURSOR_FILE_MINOR))
#define XCURSOR_FILE_HEADER_LEN (4 * 4)
#define XCURSOR_FILE_TOC_LEN (3 * 4)

typedef struct _XcursorFileToc {
    XcursorUInt type; /* chunk type */
    XcursorUInt subtype; /* subtype (size for images) */
    XcursorUInt position; /* absolute position in file */
} XcursorFileToc;

typedef struct _XcursorFileHeader {
    XcursorUInt magic; /* magic number */
    XcursorUInt header; /* byte length of header */
    XcursorUInt version; /* file version number */
    XcursorUInt ntoc; /* number of toc entries */
    XcursorFileToc *tocs; /* table of contents */
} XcursorFileHeader;

/*
 * The rest of the file is a list of chunks, each tagged by type
 * and version.
 *
 *  Chunk:
 *	ChunkHeader
 *	<extra type-specific header fields>
 *	<type-specific data>
 *
 *  ChunkHeader:
 *	CARD32	    header	bytes in chunk header + type header
 *	CARD32	    type	chunk type
 *	CARD32	    subtype	chunk subtype
 *	CARD32	    version	chunk type version
 */

#define XCURSOR_CHUNK_HEADER_LEN (4 * 4)

typedef struct _XcursorChunkHeader {
    XcursorUInt header; /* bytes in chunk header */
    XcursorUInt type; /* chunk type */
    XcursorUInt subtype; /* chunk subtype (size for images) */
    XcursorUInt version; /* version of this type */
} XcursorChunkHeader;

/*
 * Here's a list of the known chunk types
 */

/*
 * Comments consist of a 4-byte length field followed by
 * UTF-8 encoded text
 *
 *  Comment:
 *	ChunkHeader header	chunk header
 *	CARD32	    length	bytes in text
 *	LISTofCARD8 text	UTF-8 encoded text
 */

#define XCURSOR_COMMENT_TYPE 0xfffe0001
#define XCURSOR_COMMENT_VERSION 1
#define XCURSOR_COMMENT_HEADER_LEN (XCURSOR_CHUNK_HEADER_LEN + (1 * 4))
#define XCURSOR_COMMENT_COPYRIGHT 1
#define XCURSOR_COMMENT_LICENSE 2
#define XCURSOR_COMMENT_OTHER 3
#define XCURSOR_COMMENT_MAX_LEN 0x100000

typedef struct _XcursorComment {
    XcursorUInt version;
    XcursorUInt comment_type;
    char *comment;
} XcursorComment;

/*
 * Each cursor image occupies a separate image chunk.
 * The length of the image header follows the chunk header
 * so that future versions can extend the header without
 * breaking older applications
 *
 *  Image:
 *	ChunkHeader	header	chunk header
 *	CARD32		width	actual width
 *	CARD32		height	actual height
 *	CARD32		xhot	hot spot x
 *	CARD32		yhot	hot spot y
 *	CARD32		delay	animation delay
 *	LISTofCARD32	pixels	ARGB pixels
 */

#define XCURSOR_IMAGE_TYPE 0xfffd0002
#define XCURSOR_IMAGE_VERSION 1
#define XCURSOR_IMAGE_HEADER_LEN (XCURSOR_CHUNK_HEADER_LEN + (5 * 4))
#define XCURSOR_IMAGE_MAX_SIZE 0x7fff /* 32767x32767 max cursor size */

typedef struct _XcursorComments {
    int ncomment; /* number of comments */
    XcursorComment **comments; /* array of XcursorComment pointers */
} XcursorComments;

/*
 * From libXcursor/src/file.c
 */

static int _XcursorStdioFileRead(XcursorFile *file, unsigned char *buf, int len)
{
    FILE *f = file->closure;
    return (int)fread(buf, 1, (size_t)len, f);
}

static int _XcursorStdioFileWrite(XcursorFile *file, unsigned char *buf, int len)
{
    FILE *f = file->closure;
    return (int)fwrite(buf, 1, (size_t)len, f);
}

static int _XcursorStdioFileSeek(XcursorFile *file, long offset, int whence)
{
    FILE *f = file->closure;
    return fseek(f, offset, whence);
}

static void _XcursorStdioFileInitialize(FILE *stdfile, XcursorFile *file)
{
    file->closure = stdfile;
    file->read = _XcursorStdioFileRead;
    file->write = _XcursorStdioFileWrite;
    file->seek = _XcursorStdioFileSeek;
}

static XcursorImage *XcursorImageCreate(int width, int height)
{
    XcursorImage *image;

    if (width < 0 || height < 0)
        return NULL;
    if (width > XCURSOR_IMAGE_MAX_SIZE || height > XCURSOR_IMAGE_MAX_SIZE)
        return NULL;

    image = malloc(sizeof(XcursorImage) + width * height * sizeof(XcursorPixel));
    if (!image)
        return NULL;
    image->version = XCURSOR_IMAGE_VERSION;
    image->pixels = (XcursorPixel *)(image + 1);
    image->size = width > height ? width : height;
    image->width = width;
    image->height = height;
    image->delay = 0;
    return image;
}

void XcursorImageDestroy(XcursorImage *image)
{
    free(image);
}

static XcursorImages *XcursorImagesCreate(int size)
{
    XcursorImages *images;

    images = malloc(sizeof(XcursorImages) + size * sizeof(XcursorImage *));
    if (!images)
        return NULL;
    images->nimage = 0;
    images->images = (XcursorImage **)(images + 1);
    return images;
}

void XcursorImagesDestroy(XcursorImages *images)
{
    int n;

    if (!images)
        return;

    for (n = 0; n < images->nimage; n++)
        XcursorImageDestroy(images->images[n]);
    free(images);
}

static XcursorBool _XcursorReadUInt(XcursorFile *file, XcursorUInt *u)
{
    uint8_t bytes[4];

    if (!file || !u)
        return XcursorFalse;

    if ((*file->read)(file, bytes, 4) != 4)
        return XcursorFalse;

    *u = ((XcursorUInt)(bytes[0]) << 0) | ((XcursorUInt)(bytes[1]) << 8) | ((XcursorUInt)(bytes[2]) << 16) | ((XcursorUInt)(bytes[3]) << 24);
    return XcursorTrue;
}

static void _XcursorFileHeaderDestroy(XcursorFileHeader *fileHeader)
{
    free(fileHeader);
}

static XcursorFileHeader *_XcursorFileHeaderCreate(XcursorUInt ntoc)
{
    XcursorFileHeader *fileHeader;

    if (ntoc > 0x10000)
        return NULL;
    fileHeader = malloc(sizeof(XcursorFileHeader) + ntoc * sizeof(XcursorFileToc));
    if (!fileHeader)
        return NULL;
    fileHeader->magic = XCURSOR_MAGIC;
    fileHeader->header = XCURSOR_FILE_HEADER_LEN;
    fileHeader->version = XCURSOR_FILE_VERSION;
    fileHeader->ntoc = ntoc;
    fileHeader->tocs = (XcursorFileToc *)(fileHeader + 1);
    return fileHeader;
}

static XcursorFileHeader *_XcursorReadFileHeader(XcursorFile *file)
{
    XcursorFileHeader head, *fileHeader;
    XcursorUInt skip;
    unsigned int n;

    if (!file)
        return NULL;

    if (!_XcursorReadUInt(file, &head.magic))
        return NULL;
    if (head.magic != XCURSOR_MAGIC)
        return NULL;
    if (!_XcursorReadUInt(file, &head.header))
        return NULL;
    if (!_XcursorReadUInt(file, &head.version))
        return NULL;
    if (!_XcursorReadUInt(file, &head.ntoc))
        return NULL;
    skip = head.header - XCURSOR_FILE_HEADER_LEN;
    if (skip)
        if ((*file->seek)(file, skip, SEEK_CUR) == EOF)
            return NULL;
    fileHeader = _XcursorFileHeaderCreate(head.ntoc);
    if (!fileHeader)
        return NULL;
    fileHeader->magic = head.magic;
    fileHeader->header = head.header;
    fileHeader->version = head.version;
    fileHeader->ntoc = head.ntoc;
    for (n = 0; n < fileHeader->ntoc; n++) {
        if (!_XcursorReadUInt(file, &fileHeader->tocs[n].type))
            break;
        if (!_XcursorReadUInt(file, &fileHeader->tocs[n].subtype))
            break;
        if (!_XcursorReadUInt(file, &fileHeader->tocs[n].position))
            break;
    }
    if (n != fileHeader->ntoc) {
        _XcursorFileHeaderDestroy(fileHeader);
        return NULL;
    }
    return fileHeader;
}

static XcursorBool _XcursorSeekToToc(XcursorFile *file, XcursorFileHeader *fileHeader, int toc)
{
    if (!file || !fileHeader || (*file->seek)(file, fileHeader->tocs[toc].position, SEEK_SET) == EOF)
        return XcursorFalse;
    return XcursorTrue;
}

static XcursorBool _XcursorFileReadChunkHeader(XcursorFile *file, XcursorFileHeader *fileHeader, int toc, XcursorChunkHeader *chunkHeader)
{
    if (!file || !fileHeader || !chunkHeader)
        return XcursorFalse;
    if (!_XcursorSeekToToc(file, fileHeader, toc))
        return XcursorFalse;
    if (!_XcursorReadUInt(file, &chunkHeader->header))
        return XcursorFalse;
    if (!_XcursorReadUInt(file, &chunkHeader->type))
        return XcursorFalse;
    if (!_XcursorReadUInt(file, &chunkHeader->subtype))
        return XcursorFalse;
    if (!_XcursorReadUInt(file, &chunkHeader->version))
        return XcursorFalse;
    /* sanity check */
    if (chunkHeader->type != fileHeader->tocs[toc].type || chunkHeader->subtype != fileHeader->tocs[toc].subtype)
        return XcursorFalse;
    return XcursorTrue;
}

#define dist(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))

static XcursorDim _XcursorFindBestSize(XcursorFileHeader *fileHeader, XcursorDim size, int *nsizesp)
{
    unsigned int n;
    int nsizes = 0;
    XcursorDim bestSize = 0;
    XcursorDim thisSize;

    if (!fileHeader || !nsizesp)
        return 0;

    for (n = 0; n < fileHeader->ntoc; n++) {
        if (fileHeader->tocs[n].type != XCURSOR_IMAGE_TYPE)
            continue;
        thisSize = fileHeader->tocs[n].subtype;
        if (!bestSize || dist(thisSize, size) < dist(bestSize, size)) {
            bestSize = thisSize;
            nsizes = 1;
        } else if (thisSize == bestSize)
            nsizes++;
    }
    *nsizesp = nsizes;
    return bestSize;
}

static int _XcursorFindImageToc(XcursorFileHeader *fileHeader, XcursorDim size, int count)
{
    unsigned int toc;
    XcursorDim thisSize;

    if (!fileHeader)
        return 0;

    for (toc = 0; toc < fileHeader->ntoc; toc++) {
        if (fileHeader->tocs[toc].type != XCURSOR_IMAGE_TYPE)
            continue;
        thisSize = fileHeader->tocs[toc].subtype;
        if (thisSize != size)
            continue;
        if (!count)
            break;
        count--;
    }
    if (toc == fileHeader->ntoc)
        return -1;
    return toc;
}

static XcursorImage *_XcursorReadImage(XcursorFile *file, XcursorFileHeader *fileHeader, int toc)
{
    XcursorChunkHeader chunkHeader;
    XcursorImage head;
    XcursorImage *image;
    int n;
    XcursorPixel *p;

    if (!file || !fileHeader)
        return NULL;

    if (!_XcursorFileReadChunkHeader(file, fileHeader, toc, &chunkHeader))
        return NULL;
    if (!_XcursorReadUInt(file, &head.width))
        return NULL;
    if (!_XcursorReadUInt(file, &head.height))
        return NULL;
    if (!_XcursorReadUInt(file, &head.xhot))
        return NULL;
    if (!_XcursorReadUInt(file, &head.yhot))
        return NULL;
    if (!_XcursorReadUInt(file, &head.delay))
        return NULL;
    /* sanity check data */
    if (head.width > XCURSOR_IMAGE_MAX_SIZE || head.height > XCURSOR_IMAGE_MAX_SIZE)
        return NULL;
    if (head.width == 0 || head.height == 0)
        return NULL;
    if (head.xhot > head.width || head.yhot > head.height)
        return NULL;

    /* Create the image and initialize it */
    image = XcursorImageCreate(head.width, head.height);
    if (image == NULL)
        return NULL;
    if (chunkHeader.version < image->version)
        image->version = chunkHeader.version;
    image->size = chunkHeader.subtype;
    image->xhot = head.xhot;
    image->yhot = head.yhot;
    image->delay = head.delay;
    n = image->width * image->height;
    p = image->pixels;
    while (n--) {
        if (!_XcursorReadUInt(file, p)) {
            XcursorImageDestroy(image);
            return NULL;
        }
        p++;
    }
    return image;
}

static XcursorImage *_XcursorResizeImage(XcursorImage *src, int size)
{
    XcursorDim dest_y, dest_x;
    double scale = (double)size / src->size;
    XcursorImage *dest;

    dest = XcursorImageCreate((int)(src->width * scale), (int)(src->height * scale));
    if (!dest)
        return NULL;
    dest->size = (XcursorDim)size;
    dest->xhot = (XcursorDim)(src->xhot * scale);
    dest->yhot = (XcursorDim)(src->yhot * scale);
    dest->delay = src->delay;

    for (dest_y = 0; dest_y < dest->height; dest_y++) {
        XcursorDim src_y = (XcursorDim)(dest_y / scale);
        XcursorPixel *src_row = src->pixels + (src_y * src->width);
        XcursorPixel *dest_row = dest->pixels + (dest_y * dest->width);
        for (dest_x = 0; dest_x < dest->width; dest_x++) {
            XcursorDim src_x = (XcursorDim)(dest_x / scale);
            dest_row[dest_x] = src_row[src_x];
        }
    }
    return NULL;
}

static XcursorImage *_XcursorXcFileLoadImage(XcursorFile *file, int size, XcursorBool resize)
{
    XcursorFileHeader *fileHeader;
    XcursorDim bestSize;
    int nsize;
    int toc;
    XcursorImage *image;

    if (size < 0)
        return NULL;
    fileHeader = _XcursorReadFileHeader(file);
    if (!fileHeader)
        return NULL;
    bestSize = _XcursorFindBestSize(fileHeader, (XcursorDim)size, &nsize);
    if (!bestSize) {
        _XcursorFileHeaderDestroy(fileHeader);
        return NULL;
    }
    toc = _XcursorFindImageToc(fileHeader, bestSize, 0);
    if (toc < 0) {
        _XcursorFileHeaderDestroy(fileHeader);
        return NULL;
    }
    image = _XcursorReadImage(file, fileHeader, toc);
    _XcursorFileHeaderDestroy(fileHeader);

    if (resize && image != NULL && (image->size != (XcursorDim)size)) {
        XcursorImage *resized_image = _XcursorResizeImage(image, size);
        XcursorImageDestroy(image);
        image = resized_image;
    }

    return image;
}

XcursorImage *_XcursorFileLoadImage(FILE *file, int size, XcursorBool resize)
{
    XcursorFile f;

    if (!file)
        return NULL;

    _XcursorStdioFileInitialize(file, &f);
    return _XcursorXcFileLoadImage(&f, size, resize);
}

XcursorImage *XcursorXcFileLoadImage(XcursorFile *file, int size)
{
    return _XcursorXcFileLoadImage(file, size, XcursorFalse);
}

XcursorImage *XcursorFileLoadImage(FILE *file, int size)
{
    XcursorFile f;

    if (!file)
        return NULL;

    _XcursorStdioFileInitialize(file, &f);
    return XcursorXcFileLoadImage(&f, size);
}

XcursorImages *_XcursorXcFileLoadImages(XcursorFile *file, int size, XcursorBool resize)
{
    XcursorFileHeader *fileHeader;
    XcursorDim bestSize;
    int nsize;
    XcursorImages *images;
    int n;
    XcursorImage *image;

    if (!file || size < 0)
        return NULL;
    fileHeader = _XcursorReadFileHeader(file);
    if (!fileHeader)
        return NULL;
    bestSize = _XcursorFindBestSize(fileHeader, (XcursorDim)size, &nsize);
    if (!bestSize) {
        _XcursorFileHeaderDestroy(fileHeader);
        return NULL;
    }
    images = XcursorImagesCreate(nsize);
    if (!images) {
        _XcursorFileHeaderDestroy(fileHeader);
        return NULL;
    }
    for (n = 0; n < nsize; n++) {
        int toc = _XcursorFindImageToc(fileHeader, bestSize, n);
        if (toc < 0)
            break;
        image = _XcursorReadImage(file, fileHeader, toc);
        if (!image)
            break;
        if (resize && (image->size != (XcursorDim)size)) {
            XcursorImage *resized_image = _XcursorResizeImage(image, size);
            XcursorImageDestroy(image);
            image = resized_image;
            if (image == NULL)
                break;
        }
        images->images[images->nimage] = image;
        images->nimage++;
    }
    _XcursorFileHeaderDestroy(fileHeader);
    if (images != NULL && images->nimage != nsize) {
        XcursorImagesDestroy(images);
        images = NULL;
    }
    return images;
}

XcursorImages *XcursorXcFileLoadImages(XcursorFile *file, int size)
{
    return _XcursorXcFileLoadImages(file, size, XcursorFalse);
}

XcursorImages *XcursorFileLoadImages(FILE *file, int size)
{
    XcursorFile f;

    if (!file)
        return NULL;
    _XcursorStdioFileInitialize(file, &f);
    return XcursorXcFileLoadImages(&f, size);
}

XcursorImages *XcursorXcFileLoadAllImages(XcursorFile *file)
{
    XcursorFileHeader *fileHeader;
    XcursorImage *image;
    XcursorImages *images;
    int nimage;
    XcursorUInt n;
    XcursorUInt toc;

    if (!file)
        return NULL;

    fileHeader = _XcursorReadFileHeader(file);
    if (!fileHeader)
        return NULL;
    nimage = 0;
    for (n = 0; n < fileHeader->ntoc; n++) {
        switch (fileHeader->tocs[n].type) {
        case XCURSOR_IMAGE_TYPE:
            nimage++;
            break;
        }
    }
    images = XcursorImagesCreate(nimage);
    if (!images) {
        _XcursorFileHeaderDestroy(fileHeader);
        return NULL;
    }
    for (toc = 0; toc < fileHeader->ntoc; toc++) {
        switch (fileHeader->tocs[toc].type) {
        case XCURSOR_IMAGE_TYPE:
            image = _XcursorReadImage(file, fileHeader, (int)toc);
            if (image) {
                images->images[images->nimage] = image;
                images->nimage++;
            }
            break;
        }
    }
    _XcursorFileHeaderDestroy(fileHeader);
    if (images->nimage != nimage) {
        XcursorImagesDestroy(images);
        images = NULL;
    }
    return images;
}

XcursorImages *XcursorFileLoadAllImages(FILE *file)
{
    XcursorFile f;

    if (!file)
        return NULL;

    _XcursorStdioFileInitialize(file, &f);
    return XcursorXcFileLoadAllImages(&f);
}

XcursorImages *XcursorFilenameLoadAllImages(const char *file)
{
    FILE *f;
    XcursorImages *images;

    if (!file)
        return NULL;

    f = fopen(file, "re");
    if (!f)
        return NULL;
    images = XcursorFileLoadAllImages(f);
    fclose(f);
    return images;
}

/*
 * From libXcursor/src/library.c
 */

#ifndef ICONDIR
#define ICONDIR "/usr/X11R6/lib/X11/icons"
#endif

#ifndef XCURSORPATH
#define XCURSORPATH "~/.local/share/icons:~/.icons:/usr/share/icons:/usr/share/pixmaps:" ICONDIR
#endif

int XcursorLibraryShape(const char *library);

typedef struct XcursorInherit {
    char *line;
    const char *theme;
} XcursorInherit;

const char *XcursorLibraryPath(void)
{
    static const char *path;

    if (!path) {
        path = getenv("XCURSOR_PATH");
        if (!path)
            path = XCURSORPATH;
    }
    return path;
}

static void _XcursorAddPathElt(char *path, const char *elt, int len)
{
    size_t pathlen = strlen(path);

    /* append / if the path doesn't currently have one */
    if (path[0] == '\0' || path[pathlen - 1] != '/') {
        strcat(path, "/");
        pathlen++;
    }
    if (len == -1)
        len = (int)strlen(elt);
    /* strip leading slashes */
    while (len && elt[0] == '/') {
        elt++;
        len--;
    }
    strncpy(path + pathlen, elt, (size_t)len);
    path[pathlen + (size_t)len] = '\0';
}

static char *_XcursorBuildThemeDir(const char *dir, const char *theme)
{
    const char *colon;
    const char *tcolon;
    char *full;
    char *home;
    int dirlen;
    int homelen;
    int themelen;
    int len;

    if (!dir || !theme)
        return NULL;

    colon = strchr(dir, ':');
    if (!colon)
        colon = dir + strlen(dir);

    dirlen = (int)(colon - dir);

    tcolon = strchr(theme, ':');
    if (!tcolon)
        tcolon = theme + strlen(theme);

    themelen = (int)(tcolon - theme);

    home = NULL;
    homelen = 0;
    if (*dir == '~') {
        home = getenv("HOME");
        if (!home)
            return NULL;
        homelen = (int)strlen(home);
        dir++;
        dirlen--;
    }

    /*
     * add space for any needed directory separators, one per component,
     * and one for the trailing null
     */
    len = 1 + homelen + 1 + dirlen + 1 + themelen + 1;

    full = malloc((size_t)len);
    if (!full)
        return NULL;
    full[0] = '\0';

    if (home)
        _XcursorAddPathElt(full, home, -1);
    _XcursorAddPathElt(full, dir, dirlen);
    _XcursorAddPathElt(full, theme, themelen);
    return full;
}

static char *_XcursorBuildFullname(const char *dir, const char *subdir, const char *file)
{
    char *full;

    if (!dir || !subdir || !file)
        return NULL;

    full = malloc(strlen(dir) + 1 + strlen(subdir) + 1 + strlen(file) + 1);
    if (!full)
        return NULL;
    full[0] = '\0';
    _XcursorAddPathElt(full, dir, -1);
    _XcursorAddPathElt(full, subdir, -1);
    _XcursorAddPathElt(full, file, -1);
    return full;
}

static const char *_XcursorNextPath(const char *path)
{
    char *colon = strchr(path, ':');

    if (!colon)
        return NULL;
    return colon + 1;
}

/*
 * _XcursorThemeInherits, XcursorWhite, & XcursorSep are copied in
 * libxcb-cursor/cursor/load_cursor.c.  Please update that copy to
 * include any changes made to the code for those here.
 */

#define XcursorWhite(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')
#define XcursorSep(c) ((c) == ';' || (c) == ',')

static char *_XcursorThemeInherits(const char *full)
{
    char line[8192];
    char *result = NULL;
    FILE *f;

    if (!full)
        return NULL;

    f = fopen(full, "re");
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            if (!strncmp(line, "Inherits", 8)) {
                char *l = line + 8;
                while (*l == ' ')
                    l++;
                if (*l != '=')
                    continue;
                l++;
                while (*l == ' ')
                    l++;
                result = malloc(strlen(l) + 1);
                if (result) {
                    char *r = result;
                    while (*l) {
                        while (XcursorSep(*l) || XcursorWhite(*l))
                            l++;
                        if (!*l)
                            break;
                        if (r != result)
                            *r++ = ':';
                        while (*l && !XcursorWhite(*l) && !XcursorSep(*l))
                            *r++ = *l++;
                    }
                    *r++ = '\0';
                }
                break;
            }
        }
        fclose(f);
    }
    return result;
}

#define XCURSOR_SCAN_CORE ((FILE *)1)
#define MAX_INHERITS_DEPTH 32

static FILE *XcursorScanTheme(const char *theme, const char *name)
{
    FILE *f = NULL;
    char *full;
    char *dir;
    const char *path;
    XcursorInherit inherits[MAX_INHERITS_DEPTH + 1];
    int d;

    if (!theme || !name)
        return NULL;

    /*
     * XCURSOR_CORE_THEME is a magic name; cursors from the core set
     * are never found in any directory.  Instead, a magic value is
     * returned which truncates any search so that overlying functions
     * can switch to equivalent core cursors
     */
    if (!strcmp(theme, XCURSOR_CORE_THEME) && XcursorLibraryShape(name) >= 0)
        return XCURSOR_SCAN_CORE;

    memset(inherits, 0, sizeof(inherits));

    d = 0;
    inherits[d].theme = theme;

    while (f == NULL && d >= 0 && inherits[d].theme != NULL) {
        /*
         * Scan this theme
         */
        for (path = XcursorLibraryPath(); path && f == NULL; path = _XcursorNextPath(path)) {
            dir = _XcursorBuildThemeDir(path, inherits[d].theme);
            if (dir) {
                full = _XcursorBuildFullname(dir, "cursors", name);
                if (full) {
                    f = fopen(full, "re");
                    free(full);
                }
                if (!f && inherits[d + 1].line == NULL) {
                    if (d + 1 >= MAX_INHERITS_DEPTH) {
                        free(dir);
                        goto finish;
                    }
                    full = _XcursorBuildFullname(dir, "", "index.theme");
                    if (full) {
                        inherits[d + 1].line = _XcursorThemeInherits(full);
                        inherits[d + 1].theme = inherits[d + 1].line;
                        free(full);
                    }
                }
                free(dir);
            }
        }

        d++;
        while (d > 0 && inherits[d].theme == NULL) {
            free(inherits[d].line);
            inherits[d].line = NULL;

            if (--d == 0)
                inherits[d].theme = NULL;
            else
                inherits[d].theme = _XcursorNextPath(inherits[d].theme);
        }

        /*
         * Detect and break self reference loop early on.
         */
        if (inherits[d].theme != NULL && strcmp(inherits[d].theme, theme) == 0)
            break;
    }

finish:
    for (d = 1; d <= MAX_INHERITS_DEPTH; d++)
        free(inherits[d].line);

    return f;
}

XcursorImage *XcursorLibraryLoadImage(const char *file, const char *theme, int size)
{
    FILE *f = NULL;
    XcursorImage *image = NULL;

    if (!file)
        return NULL;

    if (theme)
        f = XcursorScanTheme(theme, file);
    if (!f)
        f = XcursorScanTheme("default", file);
    if (f != NULL && f != XCURSOR_SCAN_CORE) {
        image = XcursorFileLoadImage(f, size);
        fclose(f);
    }
    return image;
}

XcursorImages *XcursorLibraryLoadImages(const char *file, const char *theme, int size)
{
    FILE *f = NULL;
    XcursorImages *images = NULL;

    if (!file)
        return NULL;

    if (theme)
        f = XcursorScanTheme(theme, file);
    if (!f)
        f = XcursorScanTheme("default", file);
    if (f != NULL && f != XCURSOR_SCAN_CORE) {
        images = XcursorFileLoadImages(f, size);
        fclose(f);
    }
    return images;
}

static const char _XcursorStandardNames[] =
    "X_cursor\0"
    "arrow\0"
    "based_arrow_down\0"
    "based_arrow_up\0"
    "boat\0"
    "bogosity\0"
    "bottom_left_corner\0"
    "bottom_right_corner\0"
    "bottom_side\0"
    "bottom_tee\0"
    "box_spiral\0"
    "center_ptr\0"
    "circle\0"
    "clock\0"
    "coffee_mug\0"
    "cross\0"
    "cross_reverse\0"
    "crosshair\0"
    "diamond_cross\0"
    "dot\0"
    "dotbox\0"
    "double_arrow\0"
    "draft_large\0"
    "draft_small\0"
    "draped_box\0"
    "exchange\0"
    "fleur\0"
    "gobbler\0"
    "gumby\0"
    "hand1\0"
    "hand2\0"
    "heart\0"
    "icon\0"
    "iron_cross\0"
    "left_ptr\0"
    "left_side\0"
    "left_tee\0"
    "leftbutton\0"
    "ll_angle\0"
    "lr_angle\0"
    "man\0"
    "middlebutton\0"
    "mouse\0"
    "pencil\0"
    "pirate\0"
    "plus\0"
    "question_arrow\0"
    "right_ptr\0"
    "right_side\0"
    "right_tee\0"
    "rightbutton\0"
    "rtl_logo\0"
    "sailboat\0"
    "sb_down_arrow\0"
    "sb_h_double_arrow\0"
    "sb_left_arrow\0"
    "sb_right_arrow\0"
    "sb_up_arrow\0"
    "sb_v_double_arrow\0"
    "shuttle\0"
    "sizing\0"
    "spider\0"
    "spraycan\0"
    "star\0"
    "target\0"
    "tcross\0"
    "top_left_arrow\0"
    "top_left_corner\0"
    "top_right_corner\0"
    "top_side\0"
    "top_tee\0"
    "trek\0"
    "ul_angle\0"
    "umbrella\0"
    "ur_angle\0"
    "watch\0"
    "xterm";

static const unsigned short _XcursorStandardNameOffsets[] = {0,   9,   15,  32,  47,  52,  61,  80,  100, 112, 123, 134, 145, 152, 158, 169, 175, 189, 199, 213,
                                                             217, 224, 237, 249, 261, 272, 281, 287, 295, 301, 307, 313, 319, 324, 335, 344, 354, 363, 374, 383,
                                                             392, 396, 409, 415, 422, 429, 434, 449, 459, 470, 480, 492, 501, 510, 524, 542, 556, 571, 583, 601,
                                                             609, 616, 623, 632, 637, 644, 651, 666, 682, 699, 708, 716, 721, 730, 739, 748, 754};

#define NUM_STANDARD_NAMES (sizeof _XcursorStandardNameOffsets / sizeof _XcursorStandardNameOffsets[0])

#define STANDARD_NAME(id) _XcursorStandardNames + _XcursorStandardNameOffsets[id]

int XcursorLibraryShape(const char *library)
{
    int low, high;

    low = 0;
    high = NUM_STANDARD_NAMES - 1;
    while (low < high - 1) {
        int mid = (low + high) >> 1;
        int c = strcmp(library, STANDARD_NAME(mid));
        if (c == 0)
            return mid << 1;
        if (c > 0)
            low = mid;
        else
            high = mid;
    }
    while (low <= high) {
        if (!strcmp(library, STANDARD_NAME(low)))
            return (low << 1);
        low++;
    }
    return -1;
}
