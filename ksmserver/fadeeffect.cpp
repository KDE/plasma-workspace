/*
 * Copyright © 2008 Fredrik Höglund <fredrik@kde.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <QThread>
#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QtX11Extras/QX11Info>
#include <QDebug>

#include <solid/device.h>
#include <solid/processor.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <string>

#if defined(__INTEL_COMPILER)
#  define HAVE_MMX
#  define HAVE_SSE2
#elif defined(__GNUC__)
#  if defined(__MMX__)
#    define HAVE_MMX
#  endif
#  if defined(__SSE2__) && __GNUC__ > 3
#    define HAVE_SSE2
#  endif
#endif

#ifdef HAVE_MMX
#  include <mmintrin.h>
#endif

#ifdef HAVE_SSE2
#  include <emmintrin.h>
#endif

#include "fadeeffect.h"
#include "fadeeffect.moc"


#ifndef HAVE_SSE2
static inline void *_mm_malloc(size_t size, int)
{
    return malloc(size);
}

static inline void _mm_free(void *p)
{
    free(p);
}
#endif


static inline int multiply(int a, int b)
{
    int res = a * b + 0x80;
    return (res + (res >> 8)) >> 8;
}


static inline void load(const quint32 src, int *r, int *g, int *b)
{
    *r = (src >> 16) & 0xff;
    *g = (src >> 8) & 0xff;
    *b = src & 0xff;
}


static inline void load16(const quint16 src, int *r, int *g, int *b)
{
    *r = ((src >> 8) & 0x00f8) | ((src >> 13) & 0x0007);
    *g = ((src >> 3) & 0x00fc) | ((src >>  9) & 0x0003);
    *b = ((src << 3) & 0x00f8) | ((src >>  2) & 0x0007);
}


static inline quint32 store(const int r, const int g, const int b)
{
    return (r << 16) | (g << 8) | b | 0xff000000;
}


static inline quint16 store16(const int r, const int g, const int b)
{
    return (((r << 8) | (b >> 3)) & 0xf81f) | ((g << 3) & 0x07e0);
}


static void scanline_blend(const quint32 *over, const quint8 alpha, const quint32 *under,
                           quint32 *result, uint length)
{
    for (uint i = 0; i < length; ++i)
    {
        int sr, sg, sb, dr, dg, db;	

        load(over[i],  &sr, &sg, &sb);
        load(under[i], &dr, &dg, &db);

        dr = multiply((sr - dr), alpha) + dr;
        dg = multiply((sg - dg), alpha) + dg;
        db = multiply((sb - db), alpha) + db;

        result[i] = store(dr, dg, db);
    }
}


static void scanline_blend_16(const quint16 *over, const quint8 alpha, const quint16 *under,
                              quint16 *result, uint length)
{
    for (uint i = 0; i < length; ++i)
    {
        int sr, sg, sb, dr, dg, db;	

        load16(over[i],  &sr, &sg, &sb);
        load16(under[i], &dr, &dg, &db);

        dr = multiply((sr - dr), alpha) + dr;
        dg = multiply((sg - dg), alpha) + dg;
        db = multiply((sb - db), alpha) + db;

        result[i] = store16(dr, dg, db);
    }
}



// ----------------------------------------------------------------------------



#ifdef HAVE_MMX
static inline __m64 multiply(const __m64 m1, const __m64 m2)
{
    __m64 res = _mm_mullo_pi16(m1, m2);
    res = _mm_adds_pi16(res, _mm_set1_pi16 (0x0080));
    res = _mm_adds_pi16(res, _mm_srli_pi16 (res, 8));
    return _mm_srli_pi16(res, 8);
}


static inline __m64 add(const __m64 m1, const __m64 m2)
{
    return _mm_adds_pi16(m1, m2);
}


static inline __m64 load(const quint32 pixel, const __m64 zero)
{
    __m64 m = _mm_cvtsi32_si64(pixel);
    return _mm_unpacklo_pi8(m, zero);
}

static inline quint32 store(const __m64 pixel, const __m64 zero)
{
    __m64 packed = _mm_packs_pu16(pixel, zero);
    return _mm_cvtsi64_si32(packed);
}


static void scanline_blend_mmx(const quint32 *over, const quint8 a, const quint32 *under,
                               quint32 *result, uint length)
{
    register const __m64 alpha    = _mm_set1_pi16(quint16 (a));
    register const __m64 negalpha = _mm_xor_si64(alpha, _mm_set1_pi16 (0x00ff));
    register const __m64 zero     = _mm_setzero_si64();

    for (uint i = 0; i < length; ++i)
    {
        __m64 src = load(over[i],  zero);
        __m64 dst = load(under[i], zero);

        src = multiply(src, alpha);
        dst = multiply(dst, negalpha);
        dst = add(src, dst);

        result[i] = store(dst, zero);
    }

    _mm_empty();
}
#endif // HAVE_MMX


// ----------------------------------------------------------------------------


#ifdef HAVE_SSE2
static inline __m128i multiply(const __m128i m1, const __m128i m2)
{
    __m128i res = _mm_mullo_epi16(m1, m2);
    res = _mm_adds_epi16(res, _mm_set1_epi16 (0x0080));
    res = _mm_adds_epi16(res, _mm_srli_epi16 (res, 8));
    return _mm_srli_epi16(res, 8);
}


static inline __m128i add(const __m128i m1, const __m128i m2)
{
    return _mm_adds_epi16(m1, m2);
}


static inline __m128i lower(__m128i m)
{
    return _mm_unpacklo_epi8(m, _mm_setzero_si128 ());
}


static inline __m128i upper(__m128i m)
{
    return _mm_unpackhi_epi8(m, _mm_setzero_si128 ());
}


void scanline_blend_sse2(const __m128i *over, const quint8 a, const __m128i *under,
                         __m128i *result, uint length)
{
    length = (length + 15) >> 4;
    register const __m128i alpha    = _mm_set1_epi16(__uint16_t (a));
    register const __m128i negalpha = _mm_xor_si128(alpha, _mm_set1_epi16 (0x00ff));

    for (uint i = 0; i < length; i++)
    {
        __m128i squad = _mm_load_si128(over  + i);
        __m128i dquad = _mm_load_si128(under + i);

        __m128i src1 = lower(squad);
        __m128i dst1 = lower(dquad);
        __m128i src2 = upper(squad);
        __m128i dst2 = upper(dquad);

        squad = add(multiply(src1, alpha), multiply(dst1, negalpha));
        dquad = add(multiply(src2, alpha), multiply(dst2, negalpha));

        dquad = _mm_packus_epi16(squad, dquad);
        _mm_store_si128(result + i, dquad);
    }
}
#endif // HAVE_SSE2



// ----------------------------------------------------------------------------



class BlendingThread : public QThread
{
public:
    BlendingThread(QObject *parent);
    ~BlendingThread();

    void setImage(XImage *image);
    void setAlpha(int alpha) { m_alpha = alpha; }

private:
    void toGray16(quint8 *data);
    void toGray32(quint8 *data);

    void blend16();
    void blend32();
    void blend32_mmx();
    void blend32_sse2();

protected:
    void run();

private:
    bool have_mmx;
    bool have_sse2;
    int m_alpha;
    XImage *m_image;
    quint8 *m_original;
    quint8 *m_final;
};


BlendingThread::BlendingThread(QObject *parent)
    : QThread(parent)
{
    // Check if the CPU supports MMX and SSE2.
    // We only check the first CPU on an SMP system, and assume all CPU's support the same features.
    QList<Solid::Device> list = Solid::Device::listFromType(Solid::DeviceInterface::Processor, QString());
    if (list.size() > 0)
    {
        Solid::Processor::InstructionSets features = list[0].as<Solid::Processor>()->instructionSets();
        have_mmx  = features & Solid::Processor::IntelMmx;
        have_sse2 = features & Solid::Processor::IntelSse2;
    }
    else
    {
        // Can happen if e.g. there is no usable backend for Solid.  Err on the side of caution.
        // (c.f. bug:163112)
        have_mmx  = false;
        have_sse2 = false;
    }

    m_final    = NULL;
    m_original = NULL;
}


BlendingThread::~BlendingThread()
{
    _mm_free(m_final);
    _mm_free(m_original);
}


void BlendingThread::setImage(XImage *image)
{
    m_image = image;
    int size = m_image->bytes_per_line * m_image->height;

    // We need the data to be aligned on a 128 bit (16 byte) boundary for SSE2
    m_original = (quint8*) _mm_malloc(size, 16);
    m_final    = (quint8*) _mm_malloc(size, 16);

    memcpy((void*)m_original, (const void*)m_image->data, size);
    memcpy((void*)m_final,    (const void*)m_image->data, size);

    if (m_image->depth > 16) {
        // Make sure that the alpha channel is initialized to 0xff
        for (int y = 0; y < image->height; y++) {
            quint32 *pixels = (quint32*)(m_original + (m_image->bytes_per_line * y));
            for (int x = 0; x < image->width; x++)
                pixels[x] |= 0xff000000;
        }
    }

    if (m_image->depth != 16)
        toGray32(m_final);
    else
        toGray16(m_final);
}


void BlendingThread::toGray16(quint8 *data)
{
    for (int y = 0; y < m_image->height; y++)
    {
        quint16 *pixels = (quint16*)(data + (m_image->bytes_per_line * y));
        for (int x = 0; x < m_image->width; x++)
        {
            int red, green, blue;
            load16(pixels[x], &red, &green, &blue);

            // Make sure the 3 least significant bits are 0, so the red, green and blue
            // channels really have the same value when packed in a 5/6/5 representation.
            int val = int(red * .299 + green * .587 + blue * .114) & 0xf8;
            pixels[x] = store16(val, val, val);
        }
    }
}


void BlendingThread::toGray32(quint8 *data)
{
    for (int y = 0; y < m_image->height; y++)
    {
        quint32 *pixels = (quint32*)(data + (m_image->bytes_per_line * y));
        for (int x = 0; x < m_image->width; x++)
        {
            int red, green, blue;
            load(pixels[x], &red, &green, &blue);

            int val = int(red * .299 + green * .587 + blue * .114);
            pixels[x] = store(val, val, val);
        }
    }
}


void BlendingThread::blend16()
{
    for (int y = 0; y < m_image->height; y++)
    {
        uint start = m_image->bytes_per_line * y;
        quint16 *over   = (quint16*)(m_original + start);
        quint16 *under  = (quint16*)(m_final + start);
        quint16 *result = (quint16*)(m_image->data + start);

        scanline_blend_16(over, m_alpha, under, result, m_image->width);
    }
}


void BlendingThread::blend32()
{
    for (int y = 0; y < m_image->height; y++)
    {
        int start = m_image->bytes_per_line * y;
        quint32 *over   = (quint32*)(m_original + start);
        quint32 *under  = (quint32*)(m_final + start);
        quint32 *result = (quint32*)(m_image->data + start);

        scanline_blend(over, m_alpha, under, result, m_image->width);
    }
}


void BlendingThread::blend32_mmx()
{
#ifdef HAVE_MMX
    for (int y = 0; y < m_image->height; y++)
    {
        int start = m_image->bytes_per_line * y;
        quint32 *over   = (quint32*)(m_original + start);
        quint32 *under  = (quint32*)(m_final + start);
        quint32 *result = (quint32*)(m_image->data + start);

        scanline_blend_mmx(over, m_alpha, under, result, m_image->width);
    }
#endif
}


void BlendingThread::blend32_sse2()
{
#ifdef HAVE_SSE2
    uint length = m_image->bytes_per_line * m_image->height;

    __m128i *over   = (__m128i*)(m_original);
    __m128i *under  = (__m128i*)(m_final);
    __m128i *result = (__m128i*)(m_image->data);

    scanline_blend_sse2(over, m_alpha, under, result, length);
#endif
}


void BlendingThread::run()
{
    if (m_image->depth != 16)
    {
#ifdef HAVE_SSE2
        if (have_sse2)
            blend32_sse2();
        else
#endif
#ifdef HAVE_MMX
       if (have_mmx)
            blend32_mmx();
       else
#endif
            blend32();
    }
    else
        blend16();
}



// ----------------------------------------------------------------------------



FadeEffect::FadeEffect(QWidget *parent, QPixmap *pixmap)
    : LogoutEffect(parent, pixmap), blender(NULL) 
{
    Display *dpy = parent->x11Info().display();

    image = XCreateImage(dpy, (Visual*)pixmap->x11Info().visual(), pixmap->depth(),
                         ZPixmap, 0, NULL, pixmap->width(), pixmap->height(), 32, 0);

    // Allocate the image data on 16 byte boundary for SSE2
    image->data = (char*)_mm_malloc(image->bytes_per_line * image->height, 16);

    gc = XCreateGC(dpy, pixmap->handle(), 0, NULL);

    blender = new BlendingThread(this);
    currentY = 0;
}


FadeEffect::~FadeEffect()
{
    blender->wait();
    _mm_free(image->data);
    image->data = NULL;
    XDestroyImage(image);
    XFreeGC(QX11Info::display(), gc);
}


void FadeEffect::start()
{
    done = false;
    alpha = 255;

    // Start by grabbing the screenshot
    grabImageSection();
}


void FadeEffect::grabImageSection()
{
    const int sectionHeight = 64;
    int h = (currentY + sectionHeight > image->height) ? image->height - currentY : sectionHeight;

    XGetSubImage(QX11Info::display(), QX11Info::appRootWindow(), 0, currentY, image->width, h,
                 AllPlanes, ZPixmap, image, 0, currentY);

    // Continue until we have the whole image
    currentY += sectionHeight;
    if (currentY < image->height)
    {
        QTimer::singleShot(1, this, SLOT(grabImageSection()));
        return;
    }

    // Let the owner know we're done.
    emit initialized();

    // Start the fade effect
    blender->setImage(image);
    blender->setAlpha(alpha);
    blender->start();
    time.start();

    QTimer::singleShot(10, this, SLOT(nextFrame()));
}


void FadeEffect::nextFrame()
{
    const qreal runTime = 2000; // milliseconds

    if (!blender->isFinished())
    {
        QTimer::singleShot(10, this, SLOT(nextFrame()));
        return;
    }

    XPutImage(QX11Info::display(), pixmap->handle(), gc, image, 0, 0, 0, 0, image->width, image->height);
    parent->update();

    alpha = qRound(qMax(255. - (255. * (qreal(time.elapsed() / runTime))), 0.0));

    if (!done)
    {
        blender->setAlpha(alpha);
        blender->start();

        // Make sure we don't send frames faster than the X server can process them
        XSync(QX11Info::display(), False);
        QTimer::singleShot(1, this, SLOT(nextFrame()));
    }

    if (alpha == 0)
        done = true;
}

