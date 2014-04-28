/* This file is part of the KDE libraries
    Copyright (C) 2001,2002 Ellis Whitehead <ellis@kde.org>
    Copyright (C) 2013 Martin Gräßlin <mgraesslin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kglobalaccel_x11.h"

#include "globalshortcutsregistry.h"
#include "kkeyserver.h"
#include <netwm.h>

#include <QDebug>

#include <QApplication>
#include <QWidget>
#include <QX11Info>

#include <X11/keysym.h>

// xcb
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>


// g_keyModMaskXAccel
//	mask of modifiers which can be used in shortcuts
//	(meta, alt, ctrl, shift)
// g_keyModMaskXOnOrOff
//	mask of modifiers where we don't care whether they are on or off
//	(caps lock, num lock, scroll lock)
static uint g_keyModMaskXAccel = 0;
static uint g_keyModMaskXOnOrOff = 0;

static void calculateGrabMasks()
{
	g_keyModMaskXAccel = KKeyServer::accelModMaskX();
	g_keyModMaskXOnOrOff =
			KKeyServer::modXLock() |
			KKeyServer::modXNumLock() |
			KKeyServer::modXScrollLock() |
			KKeyServer::modXModeSwitch();
	//qDebug() << "g_keyModMaskXAccel = " << g_keyModMaskXAccel
	//	<< "g_keyModMaskXOnOrOff = " << g_keyModMaskXOnOrOff << endl;
}

//----------------------------------------------------

KGlobalAccelImpl::KGlobalAccelImpl(GlobalShortcutsRegistry *owner)
    : QObject(owner)
    , m_owner(owner)
    , m_keySymbols(Q_NULLPTR)
{
	calculateGrabMasks();
    if (QX11Info::isPlatformX11()) {
        m_keySymbols = xcb_key_symbols_alloc(QX11Info::connection());
    }
}

KGlobalAccelImpl::~KGlobalAccelImpl()
{
    if (m_keySymbols) {
        xcb_key_symbols_free(m_keySymbols);
    }
}

bool KGlobalAccelImpl::grabKey( int keyQt, bool grab )
{
    if (!m_keySymbols) {
        return false;
    }
	if( !keyQt ) {
        qDebug() << "Tried to grab key with null code.";
		return false;
	}

	uint keyModX;
	xcb_keysym_t keySymX;

	// Resolve the modifier
	if( !KKeyServer::keyQtToModX(keyQt, &keyModX) ) {
		qDebug() << "keyQt (0x" << hex << keyQt << ") failed to resolve to x11 modifier";
		return false;
	}

	// Resolve the X symbol
	if( !KKeyServer::keyQtToSymX(keyQt, (int *)&keySymX) ) {
		qDebug() << "keyQt (0x" << hex << keyQt << ") failed to resolve to x11 keycode";
		return false;
	}

    xcb_keycode_t *keyCodes = xcb_key_symbols_get_keycode(m_keySymbols, keySymX);
    if (!keyCodes) {
        return false;
    }
    xcb_keycode_t keyCodeX = keyCodes[0];
    free(keyCodes);

	// Check if shift needs to be added to the grab since KKeySequenceWidget
	// can remove shift for some keys. (all the %&* and such)
	if( !(keyQt & Qt::SHIFT) &&
	    !KKeyServer::isShiftAsModifierAllowed( keyQt ) &&
	    keySymX != xcb_key_symbols_get_keysym(m_keySymbols, keyCodeX, 0) &&
	    keySymX == xcb_key_symbols_get_keysym(m_keySymbols, keyCodeX, 1) )
	{
		qDebug() << "adding shift to the grab";
		keyModX |= KKeyServer::modXShift();
	}

	keyModX &= g_keyModMaskXAccel; // Get rid of any non-relevant bits in mod

	if( !keyCodeX ) {
		qDebug() << "keyQt (0x" << hex << keyQt << ") was resolved to x11 keycode 0";
		return false;
	}

	// We'll have to grab 8 key modifier combinations in order to cover all
	//  combinations of CapsLock, NumLock, ScrollLock.
	// Does anyone with more X-savvy know how to set a mask on QX11Info::appRootWindow so that
	//  the irrelevant bits are always ignored and we can just make one XGrabKey
	//  call per accelerator? -- ellis
#ifndef NDEBUG
	QString sDebug = QString("\tcode: 0x%1 state: 0x%2 | ").arg(keyCodeX,0,16).arg(keyModX,0,16);
#endif
	uint keyModMaskX = ~g_keyModMaskXOnOrOff;
        QVector<xcb_void_cookie_t> cookies;
	for( uint irrelevantBitsMask = 0; irrelevantBitsMask <= 0xff; irrelevantBitsMask++ ) {
		if( (irrelevantBitsMask & keyModMaskX) == 0 ) {
#ifndef NDEBUG
			sDebug += QString("0x%3, ").arg(irrelevantBitsMask, 0, 16);
#endif
			if( grab )
                            cookies << xcb_grab_key_checked(QX11Info::connection(), true,
                                                            QX11Info::appRootWindow(), keyModX | irrelevantBitsMask,
                                                            keyCodeX, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_SYNC);
			else
                            cookies << xcb_ungrab_key_checked(QX11Info::connection(), keyCodeX,
                                                              QX11Info::appRootWindow(), keyModX | irrelevantBitsMask);
		}
	}

	bool failed = false;
	if( grab ) {
            for (int i = 0; i < cookies.size(); ++i) {
                QScopedPointer<xcb_generic_error_t, QScopedPointerPodDeleter> error(xcb_request_check(QX11Info::connection(), cookies.at(i)));
                if (!error.isNull()) {
                    failed = true;
                }
            }
		if( failed ) {
			qDebug() << "grab failed!\n";
			for( uint m = 0; m <= 0xff; m++ ) {
				if(( m & keyModMaskX ) == 0 )
                                    xcb_ungrab_key(QX11Info::connection(), keyCodeX, QX11Info::appRootWindow(), keyModX | m);
			}
		}
	}

	return !failed;
}

bool KGlobalAccelImpl::nativeEventFilter(const QByteArray &eventType, void *message, long *)
{
    if (eventType != QByteArrayLiteral("xcb_generic_event_t")) {
        return false;
    }
    xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
    const uint8_t responseType = event->response_type & ~0x80;
    switch (responseType) {
        case XCB_MAPPING_NOTIFY:
            qDebug() << "Got XMappingNotify event";
            xcb_refresh_keyboard_mapping(m_keySymbols, reinterpret_cast<xcb_mapping_notify_event_t*>(event));
            x11MappingNotify();
            return true;

        case XCB_KEY_PRESS:
            qDebug() << "Got XKeyPress event";
            return x11KeyPress(reinterpret_cast<xcb_key_press_event_t*>(event));

        default:
            // We get all XEvents. Just ignore them.
            return false;
    }

    Q_ASSERT(false);
    return false;
}

void KGlobalAccelImpl::x11MappingNotify()
{
	// Maybe the X modifier map has been changed.
	// uint oldKeyModMaskXAccel = g_keyModMaskXAccel;
	// uint oldKeyModMaskXOnOrOff = g_keyModMaskXOnOrOff;

	// First ungrab all currently grabbed keys. This is needed because we
	// store the keys as qt keycodes and use KKeyServer to map them to x11 key
	// codes. After calling KKeyServer::initializeMods() they could map to
	// different keycodes.
	m_owner->ungrabKeys();

	KKeyServer::initializeMods();
	calculateGrabMasks();

	m_owner->grabKeys();
}

bool KGlobalAccelImpl::x11KeyPress(xcb_key_press_event_t *pEvent)
{
	if (QWidget::keyboardGrabber() || QApplication::activePopupWidget()) {
		qWarning() << "kglobalacceld should be popup and keyboard grabbing free!";
	}

	// Keyboard needs to be ungrabed after XGrabKey() activates the grab,
	// otherwise it becomes frozen.
    xcb_connection_t *c = QX11Info::connection();
    xcb_ungrab_keyboard(c, XCB_TIME_CURRENT_TIME);
    xcb_flush(c);

    xcb_keycode_t keyCodeX = pEvent->detail;
    uint16_t keyModX = pEvent->state & (g_keyModMaskXAccel | KKeyServer::MODE_SWITCH);

    xcb_keysym_t keySymX = xcb_key_press_lookup_keysym(m_keySymbols, pEvent, 0);

	// If numlock is active and a keypad key is pressed, XOR the SHIFT state.
	//  e.g., KP_4 => Shift+KP_Left, and Shift+KP_4 => KP_Left.
    if (pEvent->state & KKeyServer::modXNumLock()) {
        xcb_keysym_t sym = xcb_key_symbols_get_keysym(m_keySymbols, keyCodeX, 0);
		// If this is a keypad key,
		if( sym >= XK_KP_Space && sym <= XK_KP_9 ) {
			switch( sym ) {

				// Leave the following keys unaltered
				// FIXME: The proper solution is to see which keysyms don't change when shifted.
				case XK_KP_Multiply:
				case XK_KP_Add:
				case XK_KP_Subtract:
				case XK_KP_Divide:
					break;

				default:
					keyModX ^= KKeyServer::modXShift();
			}
		}
	}

	int keyCodeQt;
	int keyModQt;
	KKeyServer::symXToKeyQt(keySymX, &keyCodeQt);
	KKeyServer::modXToQt(keyModX, &keyModQt);

	if( keyModQt & Qt::SHIFT && !KKeyServer::isShiftAsModifierAllowed( keyCodeQt ) ) {
		qDebug() << "removing shift modifier";
		keyModQt &= ~Qt::SHIFT;
	}

	int keyQt = keyCodeQt | keyModQt;

	// All that work for this hey... argh...
    if (NET::timestampCompare(pEvent->time, QX11Info::appTime()) > 0) {
        QX11Info::setAppTime(pEvent->time);
    }
	return m_owner->keyPressed(keyQt);
}

void KGlobalAccelImpl::setEnabled( bool enable )
{
    if (enable && qApp->platformName() == QByteArrayLiteral("xcb")) {
        qApp->installNativeEventFilter(this);
    } else {
        qApp->removeNativeEventFilter(this);
    }
}

void KGlobalAccelImpl::syncX()
{
    xcb_connection_t *c = QX11Info::connection();
    auto *value = xcb_get_input_focus_reply(c, xcb_get_input_focus_unchecked(c), nullptr);
    free(value);
}

#include "kglobalaccel_x11.moc"
