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

#ifndef XCURSOR_H
#define XCURSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef int XcursorBool;
typedef uint32_t XcursorUInt;

typedef XcursorUInt XcursorDim;
typedef XcursorUInt XcursorPixel;

typedef struct _XcursorImage {
    XcursorUInt version; /* version of the image data */
    XcursorDim size; /* nominal size for matching */
    XcursorDim width; /* actual width */
    XcursorDim height; /* actual height */
    XcursorDim xhot; /* hot spot x (must be inside image) */
    XcursorDim yhot; /* hot spot y (must be inside image) */
    XcursorUInt delay; /* animation delay to next frame (ms) */
    XcursorPixel *pixels; /* pointer to pixels */
} XcursorImage;

/*
 * Other data structures exposed by the library API
 */
typedef struct _XcursorImages {
    int nimage; /* number of images */
    XcursorImage **images; /* array of XcursorImage pointers */
} XcursorImages;

typedef struct _XcursorFile XcursorFile;

struct _XcursorFile {
    void *closure;
    int (*read)(XcursorFile *file, unsigned char *buf, int len);
    int (*write)(XcursorFile *file, unsigned char *buf, int len);
    int (*seek)(XcursorFile *file, long offset, int whence);
};

#define XCURSOR_CORE_THEME "core"

XcursorImages *XcursorFilenameLoadAllImages(const char *file);

void XcursorImagesDestroy(XcursorImages *images);
void XcursorImageDestroy(XcursorImage *images);

/*
 * Library/Image APIs
 */
XcursorImage *XcursorLibraryLoadImage(const char *library, const char *theme, int size);

XcursorImages *XcursorLibraryLoadImages(const char *library, const char *theme, int size);

#ifdef __cplusplus
}
#endif

#endif
