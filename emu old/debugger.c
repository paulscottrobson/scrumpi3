//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       System.C
//      Purpose:    Emulator Control
//      Author:     Paul Robson
//      Date:       2nd January 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "general.h"
#include "system.h"
#include "core.h"
#include "debugger.h"

static void DBGProcessKey(char command);

static BOOL inDebugMode = TRUE;
static BOOL bInCodeMode = TRUE;
static WORD16 breakpoint = 0xFFFF;
static WORD16 codeAddress = 0;
static WORD16 dataAddress = 0;

//*******************************************************************************************************
//											Process a Frame
//*******************************************************************************************************

void DBGProcessFrame(void) {
	if (!inDebugMode) {																			// In run mode
		inDebugMode = CPUExecute(FALSE);														// Do a single frame.
		if (SYSIsMonitorBreak()) inDebugMode = TRUE; 											// Break to monitor ?
		if (inDebugMode) codeAddress = CPUGetStatus(NULL)->pc; 									// Code address to current address if in debug mode now.
	} else {
		BYTE8 currentKey = SYSReadKeyboard();													// Read Keyboard
		if (currentKey) DBGProcessKey(toupper(currentKey)); 									// If key pressed do command.
	}
}

//*******************************************************************************************************
//									Render the debugger display
//*******************************************************************************************************

void DBGRenderDisplay(DisplaySurface *drawSurface) {
	DisplaySurface *screen = CPUGetDisplaySurface();											// Get current display surface.
	if (screen != NULL) SYSUpdateSurface(screen);												// Update the texture
	if (inDebugMode) { 																			// If in debug mode
		SDL_Rect rc;
		SYSClearScreen();																		// Clear screen 
    	CPURenderDebug(&rc,codeAddress,dataAddress); 											// Render the CPU Debugging Information & get screen rectangle
    	rc.x = rc.x * drawSurface->width / 100;													// Convert percentages to real rectangle.
    	rc.w = rc.w * drawSurface->width / 100;
    	rc.y = rc.y * drawSurface->height / 100;
    	rc.h = rc.h * drawSurface->height / 100;
    	if (screen != NULL) {
    		SDL_Rect rFrame;
    		rFrame.x = rc.x-4;rFrame.y = rc.y-4;rFrame.w = rc.w+8;rFrame.h = rc.h+8;			// Frame the display
    		SDL_FillRect(drawSurface->surface,&rFrame,
    						SDL_MapRGB(drawSurface->surface->format,255,255,0));
    		SDL_BlitScaled(screen->surface,NULL,drawSurface->surface,&rc);						// Render the display
    	}
    } else { 																					// Otherwise display full screen.
    	if (screen != NULL) SDL_BlitScaled(screen->surface,NULL,drawSurface->surface,NULL);
    }
}

//*******************************************************************************************************
//										Get current breakpoint
//*******************************************************************************************************

WORD16 DBGGetCurrentBreakpoint(void) {
	return breakpoint;
}

//*******************************************************************************************************
//										Set the code address
//*******************************************************************************************************

void DBGSetCodeAddress(WORD16 c) {
	codeAddress = c;
}

//*******************************************************************************************************
//										Process a key stroke
//*******************************************************************************************************

static void DBGProcessKey(char command) {

	switch(command) {

		case SDLK_TAB:																			// TAB Switches Code/Data
			bInCodeMode = !bInCodeMode;
			break;

		case 'M':																				// (M)emory data mode.
			bInCodeMode = FALSE;
			break;

		case 'P':																				// (P)rogram mode.
			bInCodeMode = TRUE;
			break;

		case 'K':																				// (K) Set Break Point
			breakpoint = codeAddress;
			break;

		case 'H':																				// (H) Go Home
			codeAddress = CPUGetStatus(NULL)->pc;
			break;

		case 'S':																				// (S) Single Step
			CPUExecute(TRUE);																	// One Instruction
			codeAddress = CPUGetStatus(NULL)->pc;
			break;

		case 'V':																				// (V) Step Over.
			inDebugMode = FALSE;
			int addr = CPUGetStatus(NULL)->pc; 													// Get program counter.
			addr = (addr + CPUGetInstructionInfo(addr,NULL)) & CPUGetCodeAddressMask(); 		// Work out the instruction after this one.
			breakpoint = addr; 																	// Set the breakpoint.
			break;

		case 'G':																				// (G) Go to Breakpoint or Monitor
			inDebugMode = FALSE;
			break;

		default:																				// Check 0-9, A-F
			if (isxdigit(command)) {
				int digit = command - (isdigit(command) ? '0':'A'-10);							// Convert to number.
				WORD16 *wPtr = (bInCodeMode) ? &codeAddress : &dataAddress; 					// Value to change
				*wPtr = (*wPtr << 4) | digit; 													// Change it.		
				*wPtr &= (bInCodeMode) ? CPUGetCodeAddressMask(): CPUGetDataAddressMask();		// Mask it to a legal address.		
			}
			break;
	}
}

//*******************************************************************************************************
//										Display vertical label list
//*******************************************************************************************************

void DBGUtilsLabels(int x,int y,char *items[],int colour) {
	int n = 0;
	while (items[n] != NULL) {
		SYSWriteString(x,y++,items[n++],colour);
	}
}

//*******************************************************************************************************
//									Disassemble a chunk of code.
//*******************************************************************************************************

void DBGUtilsDisassemble(int x,int y1,int y2,int address,int programCounter) {
	char buffer[32],buffer2[4],*p;
	int size;
	while (y1 <= y2) {																			// Work down screen
		size = CPUGetInstructionInfo(address,buffer); 											// Get size and mnemonics.
		SYSWriteHex(x,y1,address,(address == programCounter) ? 6 : (bInCodeMode ? 3 : 2),4); 	// Display the instruction.
		for (int i = 1;i <= 4;i++) {
			sprintf(buffer2,"@%d",i);															// Matching thing.
			p = strstr(buffer,buffer2);															// is it present ?
			if (p != NULL) {
				int a = (address + i) & CPUGetCodeAddressMask();								// operand address
				sprintf(buffer2,"%02x",CPUGetStatus(NULL)->readCodeMemory(a));					// replacement bit
				p[0] = buffer2[0];p[1] = buffer2[1];											// copy it in.
			}
		}
		while (p = strchr(buffer,'|'), p != NULL) *p = '@';										// Substitute @ for | for instruction sets using @
		int col = 3;																			// Yellow normal
		if (address == breakpoint) col = 1;														// Break red
		if (address == programCounter) col = 6;													// Current Addr Cyan
		SYSWriteString(x+5,y1,buffer,col);														// Display the disassembly.
		y1++;																					// Down one line
		address = (address + size) & CPUGetCodeAddressMask();									// Next instruction.
	}
}

//*******************************************************************************************************
//											Show a data area
//*******************************************************************************************************

void DBGUtilsDisplay(int x,int y1,int y2,int address,int bytesPerLine,int formatWidth,int showASCII,int addressHighlight) {
	while (y1 <= y2) {
		SYSWriteHex(x,y1,address,bInCodeMode ? 2 : 3,4);
		for (int i = 0;i < bytesPerLine;i++) {
			int data = CPUGetStatus(NULL)->readDataMemory(address); 							// Read the data at that address.
			SYSWriteHex(x + 5 + i * (formatWidth+1),y1,data,(address == addressHighlight) ? 6 : 3,formatWidth);
			if (showASCII) SYSWriteCharacter(x + 5 + 8 * (formatWidth+1) + i,y1,data,(addressHighlight == address) ? 6 : 3);
			address = (address + 1) & CPUGetDataAddressMask();									// next data byte.
		}
		y1++;
	}
}
