//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       hardware_kbd.cpp (SC/MP Scrumpi 3)
//      Purpose:    Keyboard Interface
//      Author:     Paul Robson
//      Date:       14th February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#include "sys_processor.h"
#include "hardware.h"
#include "gfx.h"

static int currentChar = 0;

//*******************************************************************************************************
//							Process keystrokes/releases from debugger
//*******************************************************************************************************

int HWProcessKey(int keyState,int runMode) {

	if (keyState >= 0 && runMode != 0) {
		int ch = GFXToASCII(keyState,-1);
		if (ch >= 'a' && ch <= 'z') ch = ch - 32;
		if (ch >= 32 && ch <= 96) currentChar = ch;
	} else {
		currentChar = 0;
	}
	//printf("%d\n",currentChar);
	return keyState;
}

//*******************************************************************************************************
//										Scan keyboard
//*******************************************************************************************************

BYTE8 HWReadKey(BYTE8 keyID) {
	BYTE8 key = 0;
	switch (keyID) {
		case HWK_INT:
			key = GFXIsKeyPressed(GFXKEY_RETURN);
			break;
		case HWK_USER:
			key = GFXIsKeyPressed(GFXKEY_CONTROL);
			break;
		case HWK_ALPHA1:
			key = (currentChar >= 64 && currentChar < 80);
			break;
		case HWK_ALPHA2:
			key = (currentChar >= 80 && currentChar < 96);
			break;
		case HWK_PUNC:
			key = (currentChar >= 32 && currentChar < 48);
			break;
		default:
			if (currentChar != 0) {
				key = (currentChar & 0x0F) == keyID;
			}
			break;
	}
	return key;
}