//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       Core_sdl.C (SC/MP Scrumpi 3)
//      Purpose:    CPU Core (SDL Specific Code, some debugger code.)
//      Author:     Paul Robson
//      Date:       14th February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include "general.h"
#include "system.h"
#include "debugger.h"
#include "core.h"

//*******************************************************************************************************
//									Get Instruction Mnemonic and Length
//*******************************************************************************************************

#include "scmp_mnemonics.h"

int CPUGetInstructionInfo(int address,char *buffer) {
	CPUSTATUS *s = CPUGetStatus(NULL);
	int opcode = s->readCodeMemory(address); 										// Get the opcode
	char *p = _mnemonics[opcode];													// Get the mnemonic
	int c = (opcode & 0x80) ? 2 : 1;
	if (buffer != NULL) {
		strcpy(buffer,p);											
		if (*p == '\0') {
			sprintf(buffer,".%02x",opcode);
			c = 1;
		}
	}
	return c;
}

//*******************************************************************************************************
//									Get code and data address masks
//*******************************************************************************************************

int CPUGetCodeAddressMask(void) { return 0xFFFF; }
int CPUGetDataAddressMask(void) { return 0xFFFF; }

//*******************************************************************************************************
//											CPU Debug Renderer
//*******************************************************************************************************

void CPURenderDebug(SDL_Rect *rcDisplay,int codeAddress,int dataAddress) {
	rcDisplay->x = 58;rcDisplay->y = 2;rcDisplay->w = 40;rcDisplay->h = 50; 		// Set rectangle for screen in percentages.
	char *labels[] = { "A","E","S","P0","P1","P2","P3","BP","CY","OV","SB","SA","IE","F2","F1","F0",NULL };
	CPUSTATUS *s = CPUGetStatus(NULL);
	DBGUtilsLabels(16,0,labels,2);
	int n = 0;
	SYSWriteHex(19,n++,s->a,3,2);
	SYSWriteHex(19,n++,s->e,3,2);
	SYSWriteHex(19,n++,s->s,3,2);
	SYSWriteHex(19,n++,s->p0,3,4);
	SYSWriteHex(19,n++,s->p1,3,4);
	SYSWriteHex(19,n++,s->p2,3,4);
	SYSWriteHex(19,n++,s->p3,3,4);
	SYSWriteHex(19,n++,DBGGetCurrentBreakpoint(),3,4);
	for (int i = 0;i < 8;i++) {
		int flag = (s->s << i) & 0x80;
		SYSWriteCharacter(19,n+i,flag ? '1':'0',flag ? 6 : 5);
	}
	DBGUtilsDisassemble(0,0,15,codeAddress,s->p0+1);
	DBGUtilsDisplay(1,17,24,dataAddress,8,2,TRUE,-1);
}

static DisplaySurface mainDisplay;													// Surface for screen display.
static DisplaySurface fontDisplay;

//*******************************************************************************************************
//									Initialise resources for the display
//*******************************************************************************************************

#include "font_large.h"																// From DM8678 is 7 x 9 6 bit font.

void CPUInitialiseResources(void) {
	SYSCreateSurface(&mainDisplay,32*9,8*16);										// Display is 32 x 8 lots of 7 x 9 characters in a 9 x 16 box.
	SYSCreateSurface(&fontDisplay,64*9,2*16);										// 64 character font, 7 x 9, normal and inverse.
	for (int x = 0;x < 64*9;x++)
		for (int y = 0;y < 2*16;y++) {
			int ch = (x/9);															// Character Number 
			int bt = 0;																// Bit pattern.
			if (x % 9 > 0 && x % 9 < 8 && y % 16 < 9) {								// If not border
				bt = _largeFont[(ch & 0x3F)*9+(y % 16)] << (x % 9-1); 				// Bit from Font Image
			}
			if (y >= 16) bt = bt ^ 0xFF;											// The second row is reversed.
			if (bt & 0x80) {
				SDL_Rect rc;
				rc.x = x;rc.y = y;rc.w = rc.h = 1;
				SDL_FillRect(fontDisplay.surface,&rc,SDL_MapRGB(fontDisplay.surface->format,0,255,0));
			}
		}
	for (int x = 0;x < 32;x++)
		for (int y = 0;y < 8;y++){
			int a = x+y*32;
			CPUXWriteScreen(x,y,rand());
		}
}

//*******************************************************************************************************
//										Get the Display Surface
//*******************************************************************************************************

DisplaySurface *CPUGetDisplaySurface(void) {
    return &mainDisplay;
}

void CPUXWriteScreen(BYTE8 x,BYTE8 y,BYTE8 ch) {
	if (x >= 32 || y >= 8) return;
	SDL_Rect rcSrc,rcTgt;
	rcSrc.x = (ch & 0x3F) * 9;rcSrc.y = (ch & 0x80) ? 16 : 0;rcSrc.w = 9;rcSrc.h = 16;
	rcTgt.x = x * 9;rcTgt.y = y * 16;rcTgt.w = 9;rcTgt.h = 16;
	SDL_BlitSurface(fontDisplay.surface,&rcSrc,mainDisplay.surface,&rcTgt);
}

//*******************************************************************************************************
//									Any special ASCII keyboard keys ?
//*******************************************************************************************************

int CPUScanCodeToASCIICheck(int scanCode) {
	return -1;
}

//*******************************************************************************************************
//									Pause for this many milliseconds
//*******************************************************************************************************

void CPUXDelay(WORD16 ms) {
	LONG32 delayTime = SYSGetTime() + ms; 												// Delay until this time.
	while (SYSGetTime() < delayTime) {} 												// Pause for that many ms.
}

//*******************************************************************************************************
//								Synchronise with an external clock
//*******************************************************************************************************

void CPUXSynchronise(WORD16 frequency) {
	SYSSynchronise(frequency);
}

//*******************************************************************************************************
//										Read keyboard key.
//*******************************************************************************************************

static BYTE8CONST keyTable[] = { 	SDLK_1,SDLK_2,SDLK_3,SDLK_4,						// Keys for the Scrumpi 3 keypad. 
							   		SDLK_q,SDLK_w,SDLK_e,SDLK_r,
							   		SDLK_a,SDLK_s,SDLK_d,SDLK_f,
							   		SDLK_z,SDLK_x,SDLK_c,SDLK_v };

BYTE8 CPUXReadKey(BYTE8 keyID) {
	BYTE8 keyStatus = 0;

	switch (keyID) {
		
		case CPUX_INT:																	// INT == CR
			keyStatus = SYSIsKeyPressed(SDLK_RETURN);
			break;

		case CPUX_ALPHA1:																// Alpha 1 is either shift
			keyStatus = SYSIsKeyPressed(SDLK_LSHIFT)||SYSIsKeyPressed(SDLK_RSHIFT);
			break;

		case CPUX_ALPHA2:																// Alpha 2 is either control
			keyStatus = SYSIsKeyPressed(SDLK_LCTRL)||SYSIsKeyPressed(SDLK_RCTRL);
			break;

		case CPUX_PUNC:																	// Punc is either alt
			keyStatus = SYSIsKeyPressed(SDLK_LALT)||SYSIsKeyPressed(SDLK_RALT);
			break;

		case CPUX_USER:
			keyStatus = SYSIsKeyPressed(SDLK_u);										// User is U
			break;

		default:
			keyStatus = SYSIsKeyPressed(keyTable[keyID & 0xF]);							// Everything else.
			break;
	}
	return (keyStatus != 0);
}