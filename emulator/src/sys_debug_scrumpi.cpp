// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		sys_debug_scrumpi.c
//		Purpose:	Debugger Code (System Dependent)
//		Created:	12th July 2019
//		Author:		Paul Robson (paul@robsons->org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gfx.h"
#include "sys_processor.h"
#include "debugger.h"

#include "scmp/scmp_mnemonics.h"

#define DBGC_ADDRESS 	(0x0F0)														// Colour scheme.
#define DBGC_DATA 		(0x0FF)														// (Background is in main.c)
#define DBGC_HIGHLIGHT 	(0xFF0)

static int renderCount = 0;

// *******************************************************************************************************************************
//											This renders the debug screen
// *******************************************************************************************************************************

static const char *labels[] = { "A","E","S","P0","P1","P2","P3","CY","BK", NULL };

void DBGXRender(int *address,int showDisplay) {

	int n = 0;
	char buffer[32];
	CPUSTATUS *s = CPUGetStatus();
	GFXSetCharacterSize(28,24);
	DBGVerticalLabel(21,0,labels,DBGC_ADDRESS,-1);									// Draw the labels for the register


	#define DN(v,w) GFXNumber(GRID(24,n++),v,16,w,GRIDSIZE,DBGC_DATA,-1)			// Helper macro

	DN(s->a,2);DN(s->e,2);DN(s->s,2);
	DN(s->p0,4);DN(s->p1,4);DN(s->p2,4);DN(s->p3,4);
	DN(s->cycles,4);
	DN(address[3],4);

	n = 0;
	int a = address[1];																// Dump Memory.
	for (int row = 15;row < 23;row++) {
		GFXNumber(GRID(0,row),a,16,4,GRIDSIZE,DBGC_ADDRESS,-1);
		for (int col = 0;col < 8;col++) {
			GFXNumber(GRID(5+col*3,row),CPUReadMemory(a),16,2,GRIDSIZE,DBGC_DATA,-1);
			a = (a + 1) & 0xFFFF;
		}		
	}

	int p = address[0]+1;															// Dump program code. 
	int opc;

	for (int row = 0;row < 14;row++) {
		int isPC = (p == ((s->p0+1) & 0xFFFF));										// Tests.
		int isBrk = (p == (address[3]+1) & 0xFFFF);
		GFXNumber(GRID(0,row),p,16,4,GRIDSIZE,isPC ? DBGC_HIGHLIGHT:DBGC_ADDRESS,	// Display address / highlight / breakpoint
																	isBrk ? 0xF00 : -1);
		opc = CPUReadMemory(p);p = (p + 1) & 0xFFFF;								// Read opcode.
		strcpy(buffer,_mnemonics[opc]);												// Work out the opcode.
		char *at = strchr(buffer,'@');												// Look for '@'
		if (at != NULL) {															// Operand ?
			char hex[6],temp[32];	
			if (at[1] == '1') {
				sprintf(hex,"%02x",CPUReadMemory(p));
				p = (p+1) & 0xFFFF;
			}
			strcpy(temp,buffer);
			strcpy(temp+(at-buffer),hex);
			strcat(temp,at+2);
			strcpy(buffer,temp);
		}
		GFXString(GRID(5,row),buffer,GRIDSIZE,isPC ? DBGC_HIGHLIGHT:DBGC_DATA,-1);	// Print the mnemonic
	}

	int xs = 50;
	int ys = 32;
	int xSize = 3;
	int ySize = 3;
	if (showDisplay) {
		renderCount++;
		int x1 = WIN_WIDTH/2-xs*xSize*8/2;
		int y1 = WIN_HEIGHT/2-ys*ySize*8/2;
		int cursorPos = 0;
		SDL_Rect r;
		int b = 8;
		r.x = x1-b;r.y = y1-b;r.w = xs*xSize*8+b*2;r.h=ys*ySize*8+b*2;
		GFXRectangle(&r,0xFFFF);
		b = b - 4;
		r.x = x1-b;r.y = y1-b;r.w = xs*xSize*8+b*2;r.h=ys*ySize*8+b*2;
		GFXRectangle(&r,0);
		for (int x = 0;x < xs;x++) 
		{
			for (int y = 0;y < ys;y++)
		 	{
		 	}
		}
	}
}	
