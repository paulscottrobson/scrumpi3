// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		hardware.h
//		Purpose:	Hardware Emulation Header
//		Created:	12th July 2019
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#ifndef _HARDWARE_H
#define _HARDWARE_H

#define HWK_PUNC 	(0xF0)
#define HWK_USER 	(0xF1)
#define HWK_ALPHA1 	(0xF2)
#define HWK_ALPHA2 	(0xF3)

BYTE8 HWReadKey(BYTE8 keyID);

void HWWrite8154(BYTE8 offset,BYTE8 data);
BYTE8 HWRead8154(BYTE8 offset);
void HWWriteUART(BYTE8 offset,BYTE8 data);
BYTE8 HWReadUART(BYTE8 offset);

#endif